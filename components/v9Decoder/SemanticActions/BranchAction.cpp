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

#include <iostream>
#include <iomanip>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
namespace ll = boost::lambda;

#include <boost/none.hpp>

#include <boost/dynamic_bitset.hpp>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>

#include <components/uArch/uArchInterfaces.hpp>

#include "../SemanticInstruction.hpp"
#include "../Effects.hpp"
#include "../SemanticActions.hpp"

  #define DBG_DeclareCategories v9Decoder
  #define DBG_SetDefaultOps AddCat(v9Decoder)
  #include DBG_Control()

namespace nv9Decoder {

using namespace nuArch;



struct BranchCCAction : public BaseSemanticAction {

  VirtualMemoryAddress theTarget;
  bool theAnnul;
  unsigned int theCondition;
  bool isFloating;
  unsigned int theFeedbackCount;


  BranchCCAction( SemanticInstruction * anInstruction, VirtualMemoryAddress aTarget, bool anAnnul, unsigned int aCondition, bool floating)
    : BaseSemanticAction ( anInstruction, 1 )
    , theTarget(aTarget)
    , theAnnul(anAnnul)
    , theCondition(aCondition)
    , isFloating(floating)
    , theFeedbackCount(0)
  {
    theInstruction->setExecuted(false);
  }


  void doEvaluate() {
    if (ready() ) {
      if (theInstruction->hasPredecessorExecuted()) {

        std::bitset<8> cc = theInstruction->operand< std::bitset<8> > (kOperand1);

        boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
        feedback->thePC = theInstruction->pc();
        feedback->theActualType = kConditional;
        feedback->theActualTarget = theTarget;
        feedback->theBPState = theInstruction->bpState();

        bool result;
        if (isFloating) {
          FCondition cond = fcondition( theCondition );
          result = cond(cc);
          DBG_( Verb, ( << *this << " Floating condition: " << theCondition << " cc: " << cc.to_ulong() << " result: " << result ) );
        } else {
          Condition cond = condition( theCondition );
          result = cond(cc);
        }

        if ( result ) {
          //Taken
          DBG_( Verb, ( << *this<< " conditional branch CC: " << cc << " TAKEN" ) );
          core()->applyToNext( theInstruction, branchInteraction(theTarget) );
          feedback->theActualDirection = kTaken;

          if (theAnnul) {
            DBG_( Verb, ( << *this << " Reinstate Next Instruction") );
            core()->applyToNext( theInstruction , reinstateInstructionInteraction() );
            theInstruction->redirectNPC( theInstruction->pc() + 4 );
          }

        } else {
          //Not Taken
          DBG_( Verb, ( << *this << " conditional branch CC: " << cc << " NOT TAKEN" ) );
          feedback->theActualDirection = kNotTaken;

          if (theAnnul) {
            DBG_( Verb, ( << *this << " Annul Next Instruction") );
            core()->applyToNext( theInstruction, annulInstructionInteraction() );
            theInstruction->redirectNPC( theInstruction->pc() + 8, theInstruction->pc() + 4);
          }
          core()->applyToNext( theInstruction, branchInteraction( theInstruction->pc() + 8  ) );

        }
        theInstruction->setBranchFeedback(feedback);

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_( Verb, ( << *this << " waiting for predecessor ") );
        reschedule();
      }
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " BranchCC Action";
  }
};

dependant_action branchCCAction
  ( SemanticInstruction * anInstruction, VirtualMemoryAddress aTarget, bool anAnnul, unsigned int aCondition, bool floating) {
    BranchCCAction * act(new(anInstruction->icb()) BranchCCAction ( anInstruction, aTarget, anAnnul, aCondition, floating ) );

  return dependant_action( act, act->dependance() );
}

struct BranchRegAction : public BaseSemanticAction {

  VirtualMemoryAddress theTarget;
  bool theAnnul;
  unsigned int theCondition;
  unsigned int theFeedbackCount;


  BranchRegAction( SemanticInstruction * anInstruction, VirtualMemoryAddress aTarget, bool anAnnul, unsigned int aCondition)
    : BaseSemanticAction ( anInstruction, 1 )
    , theTarget(aTarget)
    , theAnnul(anAnnul)
    , theCondition(aCondition)
    , theFeedbackCount(0)
  {
    theInstruction->setExecuted(false);
  }


  void doEvaluate() {
    if (ready() ) {
      if (theInstruction->hasPredecessorExecuted()) {
        unsigned long long val = theInstruction->operand< unsigned long long > (kOperand1);

        boost::intrusive_ptr<BranchFeedback> feedback( new BranchFeedback() );
        feedback->thePC = theInstruction->pc();
        feedback->theActualType = kConditional;
        feedback->theActualTarget = theTarget;
        feedback->theBPState = theInstruction->bpState();

        RCondition cond( rcondition(theCondition) );

        if ( cond(val) ) {
          //Taken
          DBG_( Verb, ( << *this<< " conditional branch val: " << val << " TAKEN" ) );
          core()->applyToNext( theInstruction, branchInteraction(theTarget) );
          feedback->theActualDirection = kTaken;

          if (theAnnul) {
            DBG_( Verb, ( << *this << " Annul Next Instruction") );
            core()->applyToNext( theInstruction , reinstateInstructionInteraction() );
            theInstruction->redirectNPC( theInstruction->pc() + 4 );
          }

        } else {
          //Not Taken
          DBG_( Verb, ( << *this << " conditional branch val: " << val << " NOT TAKEN" ) );
          core()->applyToNext( theInstruction, branchInteraction( VirtualMemoryAddress(0) ) );
          feedback->theActualDirection = kNotTaken;

          if (theAnnul) {
            DBG_( Verb, ( << *this << " Annul Next Instruction") );
            core()->applyToNext( theInstruction, annulInstructionInteraction() );
            theInstruction->redirectNPC( theInstruction->pc() + 8, theInstruction->pc() + 4);
          }
        }
        theInstruction->setBranchFeedback(feedback);

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_( Verb, ( << *this << " waiting for predecessor ") );
        reschedule();
      }
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " BranchCC Action";
  }
};

dependant_action branchRegAction
  ( SemanticInstruction * anInstruction, VirtualMemoryAddress aTarget, bool anAnnul, unsigned int aCondition) {
    BranchRegAction * act(new(anInstruction->icb()) BranchRegAction ( anInstruction, aTarget, anAnnul, aCondition) );

  return dependant_action( act, act->dependance() );
}

struct BranchToCalcAddressAction : public BaseSemanticAction {
  eOperandCode theTarget;
  unsigned int theFeedbackCount;

  BranchToCalcAddressAction( SemanticInstruction * anInstruction, eOperandCode aTarget)
    : BaseSemanticAction ( anInstruction, 1 )
    , theTarget(aTarget)
    , theFeedbackCount(0)
  {
    theInstruction->setExecuted(false);
  }


  void doEvaluate() {
    if (ready() ) {
      if (theInstruction->hasPredecessorExecuted()) {

        //Feedback is taken care of by the updateUncoditional effect at retirement
        unsigned long long target = theInstruction->operand< unsigned long long > (theTarget);
        VirtualMemoryAddress target_addr(target);
        DBG_( Verb, ( << *this<< " branc to reg target: " << target_addr ) );
        core()->applyToNext( theInstruction, branchInteraction(target_addr) );

        satisfyDependants();
        theInstruction->setExecuted(true);
      } else {
        DBG_( Verb, ( << *this << " waiting for predecessor ") );
        reschedule();
      }
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " BranchToCalcAddress Action";
  }
};

dependant_action branchToCalcAddressAction
  ( SemanticInstruction * anInstruction) {
    BranchToCalcAddressAction * act(new(anInstruction->icb()) BranchToCalcAddressAction ( anInstruction, kAddress ) );

  return dependant_action( act, act->dependance() );
}


} //nv9Decoder
