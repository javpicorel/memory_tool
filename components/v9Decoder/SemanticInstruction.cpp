// DO-NOT-REMOVE begin-copyright-block 
//                                     
// Redistributions of any form whatsoever must retain and/or include the     
// following acknowledgment, notices and disclaimer:                         
//                                                                           
// This product includes software developed by Carnegie Mellon University.   
//                                                                           
// Copyright 2006 - 2008 by Eric Chung, Michael Ferdman, Brian Gold, Nikos   
// Hardavellas, Jangwoo Kim, Ippokratis Pandis, Minglong Shao, Jared Smolens,
// Stephen Somogyi, Evangelos Vlachos, Tom Wenisch, Anastassia Ailamaki,     
// Babak Falsafi and James C. Hoe for the SimFlex Project, Computer          
// Architecture Lab at Carnegie Mellon, Carnegie Mellon University.          
//                                                                           
// For more information, see the SimFlex project website at:                 
//   http://www.ece.cmu.edu/~simflex                                         
//                                                                           
// You may not use the name 'Carnegie Mellon University' or derivations      
// thereof to endorse or promote products derived from this software.        
//                                                                           
// If you modify the software you must place a notice on or within any       
// modified version provided or made available to any third party stating    
// that you have modified the software.  The notice shall include at least   
// your name, address, phone number, email address and the date and purpose  
// of the modification.                                                      
//                                                                           
// THE SOFTWARE IS PROVIDED 'AS-IS' WITHOUT ANY WARRANTY OF ANY KIND, EITHER 
// EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY  
// THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY 
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,  
// TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY 
// BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT, 
// SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN   
// ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY, 
// CONTRACT, TORT OR OTHERWISE).                                             
//                                     
// DO-NOT-REMOVE end-copyright-block   

#include <list>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/lambda/lambda.hpp>

namespace ll = boost::lambda;

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>
#include <core/stats.hpp>
#include <core/performance/profile.hpp>
#include <core/simics/mai_api.hpp>
#include <components/uArch/uArchInterfaces.hpp>
#include <components/uFetch/uFetchTypes.hpp>


#include "SemanticInstruction.hpp"
#include "OperandMap.hpp"
#include "Effects.hpp"
#include "SemanticActions.hpp"

  #define DBG_DeclareCategories v9Decoder
  #define DBG_SetDefaultOps AddCat(v9Decoder)
  #include DBG_Control()

namespace nv9Decoder {


unsigned long theInsnCount;
Flexus::Stat::StatCounter theICBs("sys-ICBs");
Flexus::Stat::StatMax thePeakInsns("sys-PeakSemanticInsns");

#ifdef TRACK_INSNS
std::set< SemanticInstruction *> theGlobalLiveInsns;
long theLastPrintCount = 0;
#endif //TRACK_INSNS

SemanticInstruction::SemanticInstruction(VirtualMemoryAddress aPC, VirtualMemoryAddress aNextPC, Opcode anOpcode, boost::intrusive_ptr<BPredState> aBPState, unsigned int aCPU, long long aSequenceNo)
  : v9Instruction( aPC, aNextPC, anOpcode, aBPState, aCPU, aSequenceNo)
  , theOverrideSimics(false)
  , thePrevalidationsPassed(false)
  , theRetireDepCount(0)
  , theIsMicroOp(false)
  , theRetirementTarget( *this )
  , theCanRetireCounter(0)
{
  ++theInsnCount;
  thePeakInsns << theInsnCount;
  for (int i = 0; i < 4; ++i) {
    theRetirementDepends[i] = true;
  }

#ifdef TRACK_INSNS
  if (theLastPrintCount >= 10000) {
    DBG_( Dev, ( << "Live Insn Count: " << theInsnCount) );
    theLastPrintCount = 0;
    if (theInsnCount > 10000) {
      DBG_( Dev, ( << "Identifying oldest live instruction.") );
      DBG_Assert( ! theGlobalLiveInsns.empty() );
      SemanticInstruction * oldest = *theGlobalLiveInsns.begin();
      std::set<SemanticInstruction *>::iterator iter = theGlobalLiveInsns.begin();
      while (iter != theGlobalLiveInsns.end()) {
        if ((*iter)->sequenceNo() < oldest->sequenceNo()) {
          oldest = *iter;
        }
        ++iter;
      }
      DBG_( Dev, ( << "Oldest live insn: " << *oldest) );
      DBG_( Dev, ( << "   Ref count: " << oldest->theRefCount) );
      DBG_( Dev, ( << "Oldest live insn internals: " << std::internal <<*oldest) );
    }
  }
  ++theLastPrintCount;
  theGlobalLiveInsns.insert(this);
#endif //TRACK_INSNS
}

SemanticInstruction::~SemanticInstruction() {
  --theInsnCount;

#ifdef TRACK_INSNS
  theGlobalLiveInsns.erase(this);
#endif //TRACK_INSNS

}

nuArch::InstructionDependance SemanticInstruction::makeInstructionDependance( InternalDependance const & aDependance) {
  DBG_Assert( reinterpret_cast<long>(aDependance.theTarget) != 0x1 );
  nuArch::InstructionDependance ret_val;
  ret_val.instruction = boost::intrusive_ptr<nuArch::Instruction>(this);
  ret_val.satisfy = ll::bind( &DependanceTarget::invokeSatisfy, aDependance.theTarget, aDependance.theArg);
  ret_val.squash = ll::bind( &DependanceTarget::invokeSquash, aDependance.theTarget, aDependance.theArg);
  return ret_val;
}


void SemanticInstruction::setMayRetire(int aBit, bool aFlag) {
  DBG_Assert( aBit < theRetireDepCount, ( << "setMayRetire setting a bit that is out of range: " << aBit << "\n" << std::internal << *this ) );
  bool may_retire = mayRetire();
  theRetirementDepends[aBit] = aFlag;
  if (mayRetire() && ! may_retire ) {
    DBG_( Verb, ( << identify() << " may retire" ) );
  } else if ( ! mayRetire() && may_retire ) {
    DBG_( Verb, ( << identify() << " may not retire" ) );
  }
}

bool SemanticInstruction::mayRetire() const {
  FLEXUS_PROFILE();
  bool ok = theRetirementDepends[0] && theRetirementDepends[1] && theRetirementDepends[2] && theRetirementDepends[3];
   for (
    std::list< boost::function< bool()> >::const_iterator iter = theRetirementConstraints.begin(),
    end = theRetirementConstraints.end();
    ok && (iter != end) ;
    ++iter) {
    ok &= (*iter)();
  }

  ok &= (theCanRetireCounter == 0);

  return ok; //May retire if no dependence bit is clear
}

void SemanticInstruction::setCanRetireCounter(const unsigned int numCycles) {
	theCanRetireCounter = numCycles;
}

void SemanticInstruction::decrementCanRetireCounter() {
	if (theCanRetireCounter > 0) {
		--theCanRetireCounter;	
	}
}

InternalDependance SemanticInstruction::retirementDependance() {
  DBG_Assert( theRetireDepCount < 4 );
  theRetirementDepends[theRetireDepCount] = false;
  return InternalDependance( &theRetirementTarget, theRetireDepCount++);
}

bool SemanticInstruction::preValidate() {
  FLEXUS_PROFILE();
  bool ok = true;
  while (ok && !thePreValidations.empty()) {
    ok &= thePreValidations.front()();
    thePreValidations.pop_front();
  }
  thePrevalidationsPassed = ok;
  return ok;
}

bool SemanticInstruction::advancesSimics() const {
  return (! theAnnulled && !theIsMicroOp);
}

void SemanticInstruction::overrideSimics() {
  theOverrideSimics = true;
}

bool SemanticInstruction::postValidate() {
  FLEXUS_PROFILE();
  if ( theRaisedException != willRaise() ) {
    DBG_( Dev, ( << *this << " PostValidation failed: Exception mismatch flexus=" << std::hex << willRaise() << " simics=" << theRaisedException << std::dec ) );
    return false;
  }
  if ( Flexus::Simics::Processor::getProcessor(theCPU)->getPC()  != theNPC && ! theRaisedException ) {
    DBG_( Dev, ( << *this << " PostValidation failed: NPC mismatch flexus=" << theNPC << " simics=" << Flexus::Simics::Processor::getProcessor(theCPU)->getPC() ) );
    return false;
  }

  if (thePrevalidationsPassed && theOverrideSimics && ! theOverrideFns.empty() && !theRaisedException && ! theAnnulled) {
    DBG_( Verb, ( << *this << " overrideSimics flag set and override conditions passed.") );
    while (!theOverrideFns.empty()) {
      theOverrideFns.front() ();
      theOverrideFns.pop_front();
    }
  }
  bool ok = true;
  while (ok && !thePostValidations.empty()) {
    ok &= thePostValidations.front()();
    thePostValidations.pop_front();
  }
  return ok;
}

void SemanticInstruction::addPrevalidation( boost::function< bool() > aValidation) {
  thePreValidations.push_back(aValidation);
}

void SemanticInstruction::addPostvalidation( boost::function< bool() > aValidation) {
  thePostValidations.push_back(aValidation);
}

void SemanticInstruction::addOverride( boost::function< void() > anOverride) {
  theOverrideFns.push_back(anOverride);
}

void SemanticInstruction::addRetirementConstraint( boost::function< bool() > aConstraint) {
  theRetirementConstraints.push_back(aConstraint);
}


void SemanticInstruction::annul() {
  FLEXUS_PROFILE();
  if ( ! theAnnulled ) {
    v9Instruction::annul();
    theAnnulmentEffects.invoke(*this);
  }
}

void SemanticInstruction::reinstate() {
  FLEXUS_PROFILE();
  if ( theAnnulled ) {
    v9Instruction::reinstate();
    theReinstatementEffects.invoke(*this);
  }
}

void SemanticInstruction::doDispatchEffects() {
  FLEXUS_PROFILE();
  v9Instruction::doDispatchEffects();
  while (! theDispatchActions.empty()) {
    core()->create( theDispatchActions.front() );
    theDispatchActions.pop_front();
  }
  theDispatchEffects.invoke(*this);
};

void SemanticInstruction::doRetirementEffects() {
  FLEXUS_PROFILE();
  theRetirementEffects.invoke(*this);
  theRetired = true;
  //Clear predecessor to avoid leaking instructions
  thePredecessor = 0;
};

void SemanticInstruction::checkTraps() {
  FLEXUS_PROFILE();
  theCheckTrapEffects.invoke(*this);
};

void SemanticInstruction::doCommitEffects() {
  FLEXUS_PROFILE();
  theCommitEffects.invoke(*this);
  //Clear predecessor to avoid leaking instructions
  thePredecessor = 0;
};

void SemanticInstruction::squash() {
  FLEXUS_PROFILE();
  DBG_( Verb, ( <<  *this << " squashed" ) );
  theSquashed = true;
  theSquashEffects.invoke(*this);
  //Clear predecessor to avoid leaking instructions
  thePredecessor = 0;
};

void SemanticInstruction::addDispatchEffect( Effect * anEffect) {
  theDispatchEffects.append( anEffect );
}

void SemanticInstruction::addDispatchAction( simple_action const & anAction) {
  theDispatchActions.push_back( anAction.action );
}

void SemanticInstruction::addRetirementEffect( Effect * anEffect) {
  theRetirementEffects.append( anEffect );
}

void SemanticInstruction::addCheckTrapEffect( Effect * anEffect) {
  theCheckTrapEffects.append( anEffect );
}

void SemanticInstruction::addCommitEffect( Effect * anEffect) {
  theCommitEffects.append( anEffect );
}

void SemanticInstruction::addSquashEffect( Effect * anEffect) {
  theSquashEffects.append( anEffect );
}

void SemanticInstruction::addAnnulmentEffect( Effect * anEffect) {
  theAnnulmentEffects.append( anEffect );
}

void SemanticInstruction::addReinstatementEffect( Effect * anEffect) {
  theReinstatementEffects.append( anEffect );
}


void SemanticInstruction::describe(std::ostream & anOstream) const {
  bool internals = false;
  if (anOstream.flags() & std::ios_base::internal) {
    anOstream << std::left;
    internals = true;
  }
  v9Instruction::describe(anOstream);
  if (theRetired) {
    anOstream << "\t{retired}";
  } else if (theSquashed) {
    anOstream << "\t{squashed}";
  } else if (mayRetire()) {
    anOstream << "\t{completed}";
  } else if (hasExecuted()) {
    anOstream << "\t{executed}";
  }
  if (theAnnulled) {
    anOstream << "\t{annulled}";
  }
  if (theIsMicroOp) {
    anOstream << "\t{micro-op}";
  }

  if (internals) {
    anOstream << std::endl;
    anOstream << "\tOperandMap:\n" ;
    theOperands.dump(anOstream);

    if (! theDispatchEffects.empty() ) {
      anOstream << "\tDispatchEffects:\n";
      theDispatchEffects.describe(anOstream);
    }

    if (! theAnnulmentEffects.empty() ) {
      anOstream << "\tAnnulmentEffects:\n" ;
      theAnnulmentEffects.describe(anOstream);
    }

    if (! theReinstatementEffects.empty() ) {
      anOstream << "\tReinstatementEffects:\n" ;
      theReinstatementEffects.describe(anOstream);
    }

    if (! theSquashEffects.empty() ) {
      anOstream << "\tSquashEffects:\n" ;
      theSquashEffects.describe(anOstream);
    }

    if (! theRetirementEffects.empty() ) {
      anOstream << "\tRetirementEffects:\n" ;
      theRetirementEffects.describe(anOstream);
    }

    if (! theCommitEffects.empty() ) {
      anOstream << "\tCommitEffects:\n" ;
      theCommitEffects.describe(anOstream);
    }
  }
}

int SemanticInstruction::retryTranslation() {
  Flexus::Simics::Translation xlat;
  xlat.theVaddr = pc();
  xlat.theTL = core()->getTL();
  xlat.thePSTATE = core()->getPSTATE();
  xlat.theType = Flexus::Simics::Translation::eFetch;
  Flexus::Simics::Processor::getProcessor(theCPU)->translate(xlat, true);
  return xlat.theException;
}


} //nv9Decoder
