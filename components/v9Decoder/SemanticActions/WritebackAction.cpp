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
#include "RegisterValueExtractor.hpp"

  #define DBG_DeclareCategories v9Decoder
  #define DBG_SetDefaultOps AddCat(v9Decoder)
  #include DBG_Control()

namespace nv9Decoder {

using namespace nuArch;




struct WritebackAction : public BaseSemanticAction {
  eOperandCode theResult;
  eOperandCode theRd;


  WritebackAction ( SemanticInstruction * anInstruction, eOperandCode aResult, eOperandCode anRd )
   : BaseSemanticAction ( anInstruction, 1 )
   , theResult( aResult )
   , theRd( anRd )
  { }

  void squash(int anArg) {
    if (!cancelled()) {
      mapped_reg name = theInstruction->operand< mapped_reg > (theRd);
      core()->squashRegister( name );
      DBG_( Verb, ( << *this << " squashing register rd=" << name) );
    }
    BaseSemanticAction::squash(anArg);
  }

  void doEvaluate() {
    if (ready()) {
      DBG_( Verb, ( << *this << " rd: " << theRd << " result: " << theResult) );
      mapped_reg name = theInstruction->operand< mapped_reg > (theRd);
      register_value result = boost::apply_visitor( register_value_extractor(), theInstruction->operand( theResult ) );
      core()->writeRegister( name, result );
      DBG_( Verb, ( << *this << " rd= " << name << " result=" << result ) );
      core()->bypass( name, result );
      satisfyDependants();
    }
  }

  void describe( std::ostream & anOstream) const {
    anOstream << theInstruction->identify() << " WritebackAction";
  }
};

dependant_action writebackAction
  ( SemanticInstruction * anInstruction
  , eRegisterType aType
  ) {
  WritebackAction * act;
  if (aType == ccBits) {
    act = new(anInstruction->icb()) WritebackAction( anInstruction, kResultCC, kCCpd);
  } else {
    act = new(anInstruction->icb()) WritebackAction( anInstruction, kResult, kPD);
  }
  return dependant_action( act, act->dependance() );
}

dependant_action writebackRD1Action ( SemanticInstruction * anInstruction ) {
  WritebackAction * act = new(anInstruction->icb()) WritebackAction( anInstruction, kResult1, kPD1);
  return dependant_action( act, act->dependance() );
}

dependant_action writeXTRA
  ( SemanticInstruction * anInstruction ) {
  WritebackAction *  act = new(anInstruction->icb()) WritebackAction( anInstruction, kXTRAout, kXTRApd);
  return dependant_action( act, act->dependance() );
}

dependant_action floatingWritebackAction
  ( SemanticInstruction * anInstruction
  , int anIndex
  ) {
  WritebackAction *  act(new(anInstruction->icb()) WritebackAction( anInstruction, eOperandCode(kfResult0 + anIndex), eOperandCode(kPFD0 + anIndex) ) );
  return dependant_action( act, act->dependance() );
}


} //nv9Decoder
