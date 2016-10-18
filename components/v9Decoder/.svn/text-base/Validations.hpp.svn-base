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

#ifndef FLEXUS_v9DECODER_VALIDATIONS_HPP_INCLUDED
#define FLEXUS_v9DECODER_VALIDATIONS_HPP_INCLUDED

#include <boost/tuple/tuple.hpp>

#include <components/Common/Slices/MemOp.hpp>
#include "OperandCode.hpp"
#include "SemanticInstruction.hpp"

namespace nv9Decoder {

struct SemanticInstruction;

struct overrideRegister {
  unsigned int theReg;
  eOperandCode theOperandCode;
  SemanticInstruction * theInstruction;

  overrideRegister( unsigned int aReg, eOperandCode anOperand, SemanticInstruction * anInstruction )
   : theReg(aReg)
   , theOperandCode(anOperand)
   , theInstruction(anInstruction)
   { }

  void operator () ();
};

struct overrideFloatSingle {
  unsigned int theReg;
  eOperandCode theOperandCode;
  SemanticInstruction * theInstruction;

  overrideFloatSingle( unsigned int aReg, eOperandCode anOperand, SemanticInstruction * anInstruction )
   : theReg(aReg)
   , theOperandCode(anOperand)
   , theInstruction(anInstruction)
   { }

  void operator () ();
};

struct overrideFloatDouble {
  unsigned int theReg;
  eOperandCode theOperandCodeHi;
  eOperandCode theOperandCodeLo;
  SemanticInstruction * theInstruction;

  overrideFloatDouble( unsigned int aReg, eOperandCode anOperandHi, eOperandCode anOperandLo, SemanticInstruction * anInstruction )
   : theReg(aReg)
   , theOperandCodeHi(anOperandHi)
   , theOperandCodeLo(anOperandLo)
   , theInstruction(anInstruction)
   { }

  void operator () ();
};

struct validateRegister {
  unsigned int theReg;
  eOperandCode theOperandCode;
  SemanticInstruction * theInstruction;

  validateRegister( unsigned int aReg, eOperandCode anOperand, SemanticInstruction * anInstruction )
   : theReg(aReg)
   , theOperandCode(anOperand)
   , theInstruction(anInstruction)
   { }

  bool operator () ();
};

struct validateFRegister {
  unsigned int theReg;
  eOperandCode theOperandCode;
  SemanticInstruction * theInstruction;

  validateFRegister( unsigned int aReg, eOperandCode anOperand, SemanticInstruction * anInstruction )
   : theReg(aReg)
   , theOperandCode(anOperand)
   , theInstruction(anInstruction)
   { }

  bool operator () ();
};

struct validateCC {
  eOperandCode theOperandCode;
  SemanticInstruction * theInstruction;

  validateCC( eOperandCode anOperand, SemanticInstruction * anInstruction )
   : theOperandCode(anOperand)
   , theInstruction(anInstruction)
   {}

  bool operator () ();
};

struct validateFCC {
  unsigned int theFCC;
  eOperandCode theOperandCode;
  SemanticInstruction * theInstruction;

  validateFCC( unsigned int anFCC, eOperandCode anOperand, SemanticInstruction * anInstruction )
   : theFCC(anFCC)
   , theOperandCode(anOperand)
   , theInstruction(anInstruction)
   {}

  bool operator () ();
};

struct validateMemory {
  eOperandCode theAddressCode;
  eOperandCode theASICode;
  int theAddrOffset;
  eOperandCode theValueCode;
  nuArch::eSize theSize;
  SemanticInstruction * theInstruction;

  validateMemory( eOperandCode anAddressCode, eOperandCode anASICode, eOperandCode aValueCode, nuArch::eSize aSize, SemanticInstruction * anInstruction )
   : theAddressCode(anAddressCode)
   , theASICode(anASICode)
   , theAddrOffset(0)
   , theValueCode(aValueCode)
   , theSize(aSize)
   , theInstruction(anInstruction)
   {}

  validateMemory( eOperandCode anAddressCode, int anAddrOffset, eOperandCode anASICode, eOperandCode aValueCode, nuArch::eSize aSize, SemanticInstruction * anInstruction )
   : theAddressCode(anAddressCode)
   , theASICode(anASICode)
   , theAddrOffset(anAddrOffset)
   , theValueCode(aValueCode)
   , theSize(aSize)
   , theInstruction(anInstruction)
   {}

  bool operator () ();
};

struct validatePR {
  SemanticInstruction * theInstruction;
  unsigned int thePR;
  eOperandCode theOperandCode;
  validatePR( SemanticInstruction * anInstruction, unsigned int aPR, eOperandCode anOperandCode)
   : theInstruction(anInstruction)
   , thePR(aPR)
   , theOperandCode(anOperandCode)
   {}
  bool operator () ();

};

struct validateFPRS {
  SemanticInstruction * theInstruction;
  validateFPRS( SemanticInstruction * anInstruction)
   : theInstruction(anInstruction)
   {}
  bool operator () ();
};

struct validateFSR {
  SemanticInstruction * theInstruction;
  validateFSR( SemanticInstruction * anInstruction)
   : theInstruction(anInstruction)
   {}
  bool operator () ();
};

struct validateTPR {
  SemanticInstruction * theInstruction;
  unsigned int thePR;
  eOperandCode theOperandCode;
  eOperandCode theTLOperandCode;
  validateTPR( SemanticInstruction * anInstruction, unsigned int aPR, eOperandCode anOperandCode, eOperandCode aTLOperandCode)
   : theInstruction(anInstruction)
   , thePR(aPR)
   , theOperandCode(anOperandCode)
   , theTLOperandCode(aTLOperandCode)
   {}
  bool operator () ();

};

struct validateSaveRestore {
  SemanticInstruction * theInstruction;
  validateSaveRestore( SemanticInstruction * anInstruction)
   : theInstruction(anInstruction)
   {}
  bool operator () ();
};


} //nv9Decoder

#endif //FLEXUS_v9DECODER_VALIDATIONS_HPP_INCLUDED
