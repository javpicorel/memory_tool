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



struct ReadRegisterAction : public BaseSemanticAction {
  eOperandCode theRegisterCode;
  eOperandCode theOperandCode;
  bool theConnected;

  ReadRegisterAction( SemanticInstruction * anInstruction, eOperandCode aRegisterCode, eOperandCode anOperandCode )
   : BaseSemanticAction( anInstruction, 1 )
   , theRegisterCode( aRegisterCode )
   , theOperandCode( anOperandCode )
   , theConnected(false)
   {}


  bool bypass(register_value aValue) {
    if ( cancelled() || theInstruction->isRetired() || theInstruction->isSquashed() ) {
      return true;
    }

    if ( !signalled() ) {
      theInstruction->setOperand(theOperandCode, aValue);
      DBG_( Verb, ( << *this << " bypassed " << theRegisterCode << " = " << aValue << " written to " << theOperandCode ) );
      satisfyDependants();
      setReady(0, true);
    }
    return false;
  }

  void doEvaluate() {
    if (! theConnected) {
      mapped_reg name = theInstruction->operand< mapped_reg > (theRegisterCode);
      setReady( 0, core()->requestRegister( name, theInstruction->makeInstructionDependance(dependance()) ) == kReady );
      //theInstruction->addSquashEffect( disconnectRegister( theInstruction, theRegisterCode ) );
      //theInstruction->addRetirementEffect( disconnectRegister( theInstruction, theRegisterCode ) );
      core()->connectBypass( name, theInstruction, ll::bind( &ReadRegisterAction::bypass, this, ll::_1) );
      theConnected = true;
    }
    if (! signalled() ) {
      mapped_reg name = theInstruction->operand< mapped_reg > (theRegisterCode);
      eResourceStatus status = core()->requestRegister( name );
      if (status == kReady) {
        mapped_reg name = theInstruction->operand< mapped_reg > (theRegisterCode);
        DBG_( Verb, ( << *this << " read " << theRegisterCode << "(" << name << ")" ) );
        Operand aValue = core()->readRegister( name );
        theInstruction->setOperand(theOperandCode, aValue);
        DBG_( Verb, ( << *this << " read " << theRegisterCode << "(" << name << ") = " << aValue << " written to " << theOperandCode ) );
        satisfyDependants();
      } else {
        setReady( 0, false );
      }
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " ReadRegister " << theRegisterCode << " store in " << theOperandCode;
  }
};

simple_action readRegisterAction
  ( SemanticInstruction * anInstruction
  , eOperandCode aRegisterCode
  , eOperandCode anOperandCode
  ) {
    return new(anInstruction->icb()) ReadRegisterAction( anInstruction, aRegisterCode, anOperandCode);
}


} //nv9Decoder
