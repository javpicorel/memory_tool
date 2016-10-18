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
#include "PredicatedSemanticAction.hpp"

  #define DBG_DeclareCategories v9Decoder
  #define DBG_SetDefaultOps AddCat(v9Decoder)
  #include DBG_Control()

namespace nv9Decoder {

using namespace nuArch;




struct AnnulAction : public PredicatedSemanticAction {
  eOperandCode theRegisterCode;
  eOperandCode theOutputCode;
  bool theMOVConnected;
  bool theTriggered;

  AnnulAction( SemanticInstruction * anInstruction, eOperandCode aRegisterCode, eOperandCode anOutputCode )
   : PredicatedSemanticAction( anInstruction, 1, false )
   , theRegisterCode( aRegisterCode )
   , theOutputCode( anOutputCode )
   , theMOVConnected( false )
   {}

  void doEvaluate() {
    if ( thePredicate ) {
      mapped_reg name = theInstruction->operand< mapped_reg > (theRegisterCode);
      eResourceStatus status;
      if (!theMOVConnected) {
        status = core()->requestRegister( name, theInstruction->makeInstructionDependance(dependance(0)) );
        //theInstruction->addRetirementEffect( disconnectRegister( theInstruction, theRegisterCode ) );
        //theInstruction->addSquashEffect( disconnectRegister( theInstruction, theRegisterCode ) );
        theMOVConnected = true;
      } else {
        status = core()->requestRegister( name );
        DBG_( Verb, ( << *this << " scheduled. " <<  theRegisterCode << "(" << name << ")" << " is " << status ) );
      }
      if (kReady == status) {
        setReady(0, true);
      }
      if (ready()) {
        Operand aValue = core()->readRegister( name );
        theInstruction->setOperand(theOutputCode, aValue);
        DBG_( Verb, ( << *this << " read " << theRegisterCode << "(" << name << ") = " << aValue << " written to " << theOutputCode ) );
        theInstruction->setExecuted(true);
        satisfyDependants();
      }
    }
  }


  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " Annul " << theRegisterCode << " store in " << theOutputCode;
  }
};

predicated_action annulAction
  ( SemanticInstruction * anInstruction
  , eRegisterType aType
  ) {
    AnnulAction * act;
    if (aType == ccBits) {
      act = new(anInstruction->icb()) AnnulAction( anInstruction, kPCCpd, kResultCC);
    } else {
      act = new(anInstruction->icb()) AnnulAction( anInstruction, kPPD, kResult);
    }
    return predicated_action( act, act->predicate() );
}

predicated_action annulRD1Action( SemanticInstruction * anInstruction ) {
  AnnulAction * act = new(anInstruction->icb()) AnnulAction( anInstruction, kPPD1, kResult1);
  return predicated_action( act, act->predicate() );
}

predicated_action annulXTRA
  ( SemanticInstruction * anInstruction
  ) {
    AnnulAction * act = new(anInstruction->icb()) AnnulAction( anInstruction, kXTRAppd, kXTRAout);
    return predicated_action( act, act->predicate() );
}

predicated_action floatingAnnulAction
  ( SemanticInstruction * anInstruction
  , int anIndex
  ) {
    AnnulAction * act = new(anInstruction->icb()) AnnulAction( anInstruction, eOperandCode( kPPFD0 + anIndex), eOperandCode(kfResult0 + anIndex));
    return predicated_action( act, act->predicate() );
}



} //nv9Decoder
