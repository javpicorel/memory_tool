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

#ifndef FLEXUS_v9DECODER_EFFECTS_HPP_INCLUDED
#define FLEXUS_v9DECODER_EFFECTS_HPP_INCLUDED

#include <iostream>
#include <core/boost_extensions/intrusive_ptr.hpp>

#include <core/types.hpp>

#include "OperandCode.hpp"
#include <components/uArch/RegisterType.hpp>
#include <components/Common/Slices/MemOp.hpp>
#include <components/uFetch/uFetchTypes.hpp>
#include "Conditions.hpp"
#include "InstructionComponentBuffer.hpp"
#include "Interactions.hpp"

namespace nuArch {
  struct uArch;
  struct SemanticAction;
}

namespace nv9Decoder {


using Flexus::SharedTypes::VirtualMemoryAddress;
using nuArch::uArch;
using nuArch::eRegisterType;
using nuArch::eOperation;
using nuArch::SemanticAction;
using nuArch::eSize;


struct BaseSemanticAction;
struct SemanticInstruction;

struct Effect : UncountedComponent {
  Effect * theNext;
  Effect() : theNext(0) {}
  virtual ~Effect() {}
  virtual void invoke(SemanticInstruction & anInstruction) {
    if (theNext) {
      theNext->invoke(anInstruction);
    }
  }
  virtual void describe(std::ostream & anOstream) const {
    if (theNext) {
      theNext->describe(anOstream);
    }
  }
  //NOTE: No virtual destructor because effects are never destructed.
};

struct EffectChain {
  Effect * theFirst;
  Effect * theLast;
  EffectChain();
  void invoke(SemanticInstruction & anInstruction);
  void describe(std::ostream & anOstream) const ;
  void append(Effect * anEffect);
  bool empty() const { return theFirst == 0; }
};

struct DependanceTarget {
  void invokeSatisfy( int anArg ) {
    satisfy(anArg);
  }
  void invokeSquash( int anArg ) {
    squash(anArg);
  }
  virtual ~DependanceTarget() {}
  virtual void satisfy( int anArg) = 0 ;
  virtual void squash( int anArg ) = 0;

  protected:
    DependanceTarget() {}
};

struct InternalDependance {
  DependanceTarget * theTarget;
  int theArg;
  InternalDependance( )
   : theTarget( 0 )
   , theArg( 0 )
   {}
  InternalDependance( InternalDependance const & id)
   : theTarget( id.theTarget )
   , theArg( id.theArg)
   {}
  InternalDependance( DependanceTarget * tgt, int arg)
   : theTarget(tgt)
   , theArg(arg)
   {}
  void satisfy() {
    theTarget->satisfy(theArg);
  }
  void squash() {
    theTarget->squash(theArg);
  }
};

Effect * mapSource( SemanticInstruction * inst, eOperandCode anInputCode, eOperandCode anOutputCode);
Effect * freeMapping( SemanticInstruction * inst, eOperandCode aMapping);
Effect * disconnectRegister( SemanticInstruction * inst, eOperandCode aMapping);
Effect * mapDestination( SemanticInstruction * inst, eRegisterType aMapTable );
Effect * mapRD1Destination(SemanticInstruction * inst);
Effect * mapDestination_NoSquashEffects( SemanticInstruction * inst, eRegisterType aMapTable );
Effect * unmapDestination( SemanticInstruction * inst, eRegisterType aMapTable );
Effect * mapFDestination( SemanticInstruction * inst, int anIndex );
Effect * unmapFDestination( SemanticInstruction * inst, int anIndex );
Effect * restorePreviousDestination( SemanticInstruction * inst, eRegisterType aMapTable );
Effect * satisfy( SemanticInstruction * inst, InternalDependance const & aDependance);
Effect * squash( SemanticInstruction * inst, InternalDependance const & aDependance);
Effect * annulNext(SemanticInstruction * inst);
Effect * branch(SemanticInstruction * inst, VirtualMemoryAddress aTarget);
Effect * returnFromTrap(SemanticInstruction * inst,  bool isDone);
Effect * branchAfterNext(SemanticInstruction * inst, VirtualMemoryAddress aTarget);
Effect * branchAfterNext(SemanticInstruction * inst, eOperandCode aCode);
Effect * branchConditionally(SemanticInstruction * inst, VirtualMemoryAddress aTarget, bool anAnnul, unsigned int aCondition, bool isFloating);
Effect * branchRegConditionally(SemanticInstruction * inst, VirtualMemoryAddress aTarget, bool anAnnul, unsigned int aCondition);
Effect * allocateLoad(SemanticInstruction * inst, eSize aSize, InternalDependance const &  aDependance);
Effect * allocateCAS(SemanticInstruction * inst, eSize aSize, InternalDependance const & aDependance);
Effect * allocateRMW(SemanticInstruction * inst, eSize aSize, InternalDependance const & aDependance);
Effect * eraseLSQ(SemanticInstruction * inst);
Effect * allocateStore(SemanticInstruction * inst, eSize aSize, bool aBypassSB);
Effect * allocateMEMBAR(SemanticInstruction * inst);
Effect * retireMem(SemanticInstruction * inst);
Effect * commitStore(SemanticInstruction * inst);
Effect * accessMem(SemanticInstruction * inst);
Effect * saveWindow(SemanticInstruction * inst);
Effect * restoreWindow(SemanticInstruction * inst);
Effect * saveWindowPriv(SemanticInstruction * inst);
Effect * restoreWindowPriv(SemanticInstruction * inst);
Effect * saveWindowTrap(SemanticInstruction * inst);
Effect * restoreWindowTrap(SemanticInstruction * inst);
Effect * flushWTrap(SemanticInstruction * inst);
Effect * savedWindow(SemanticInstruction * inst);
Effect * restoredWindow(SemanticInstruction * inst);
Effect * updateConditional(SemanticInstruction * inst);
Effect * updateUnconditional(SemanticInstruction * inst, VirtualMemoryAddress aTarget);
Effect * updateUnconditional(SemanticInstruction * inst, eOperandCode anOperandCode);
Effect * updateCall(SemanticInstruction * inst, VirtualMemoryAddress aTarget);
Effect * updateNonBranch(SemanticInstruction * inst);
Effect * readPR(SemanticInstruction * inst, unsigned int aPR);
Effect * writePR(SemanticInstruction * inst, unsigned int aPR);
Effect * mapXTRA(SemanticInstruction * inst);
Effect * forceResync(SemanticInstruction * inst);
Effect * immuException(SemanticInstruction * inst);
Effect * dmmuTranslationCheck(SemanticInstruction * inst);
Effect * tccEffect(SemanticInstruction * inst);
Effect * updateFPRS(SemanticInstruction * inst, unsigned int aDestReg);
Effect * writeFPRS(SemanticInstruction * inst);
Effect * recordFPRS(SemanticInstruction * inst);
Effect * readFPRS(SemanticInstruction * inst);
Effect * writeFSR(SemanticInstruction * inst, eSize aSize);
Effect * storeFSR(SemanticInstruction * inst, eSize aSize);

} //nv9Decoder

#endif //FLEXUS_v9DECODER_EFFECTS_HPP_INCLUDED
