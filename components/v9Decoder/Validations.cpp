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
#include <bitset>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
namespace ll = boost::lambda;

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>

#include <components/uArch/uArchInterfaces.hpp>
#include <core/simics/mai_api.hpp>

#include "SemanticInstruction.hpp"
#include "Effects.hpp"
#include "SemanticActions.hpp"
#include "Validations.hpp"

  #define DBG_DeclareCategories v9Decoder
  #define DBG_SetDefaultOps AddCat(v9Decoder)
  #include DBG_Control()

namespace nv9Decoder {

using namespace nuArch;


static const int kY = 34;
static const int kCCR = 35;
static const int kFPRS = 36;
static const int kFSR = 37;
static const int kASI = 38;
static const int kGSR = 40;
static const int kPSTATE = 45;
  static const int kAG = 0x1;
  static const int kMG = 0x400;
  static const int kIG = 0x800;
static const int kTL = 46;
static const int kPIL = 47;
static const int kTPC1 = 48;
static const int kTNPC1 = 58;
static const int kTSTATE1 = 68;
static const int kTT1 = 78;
static const int kTBA = 88;

static const int kCWP = 90;
static const int kCANSAVE = 91;
static const int kCANRESTORE = 92;
static const int kOTHERWIN = 93;
static const int kWSTATE = 94;
static const int kCLEANWIN = 95;

void overrideRegister::operator () () {
  unsigned long long flexus = theInstruction->operand< unsigned long long > (theOperandCode);
  unsigned long long simics = Flexus::Simics::Processor::getProcessor(theInstruction->cpu())->readRegister( theReg );
  if (flexus != simics) {
    DBG_( Verb, ( << *theInstruction << " overriding simics register " << theReg << " = " << std::hex << simics << " with flexus " << theOperandCode << " = " << flexus << std::dec ));
    Flexus::Simics::Processor::getProcessor(theInstruction->cpu())->writeRegister( theReg, flexus );
  }
}

void overrideFloatSingle::operator () () {
  unsigned long long flexus = theInstruction->operand< unsigned long long > (theOperandCode);
  unsigned long long simics = Flexus::Simics::Processor::getProcessor(theInstruction->cpu())->readF( theReg & (~1) );
  unsigned long long flexus_align = simics;
  if (theReg & 1) {
    flexus_align = ( (flexus_align &  0xFFFFFFFF00000000ULL) | (flexus & 0xFFFFFFFFULL) );
    if (flexus_align != simics) {
      DBG_( Verb, ( << *theInstruction << " overriding simics f-register " << theReg << " = " << std::hex << simics << " with flexus " << theOperandCode << " = " << flexus_align << std::dec ));
      Flexus::Simics::Processor::getProcessor(theInstruction->cpu())->writeF( theReg, flexus_align );
    }
  } else {
    flexus_align = ( (flexus_align &  0xFFFFFFFFULL) | (flexus << 32) );
    if (flexus_align != simics) {
      DBG_( Verb, ( << *theInstruction << " overriding simics f-register " << theReg << " = " << std::hex << simics << " with flexus " << theOperandCode << " = " << flexus_align << std::dec ));
      Flexus::Simics::Processor::getProcessor(theInstruction->cpu())->writeF( theReg, flexus_align );
    }
  }
}

void overrideFloatDouble::operator () () {
  unsigned long long flexus_hi = theInstruction->operand< unsigned long long > (theOperandCodeHi);
  unsigned long long flexus_lo = theInstruction->operand< unsigned long long > (theOperandCodeLo);
  unsigned long long simics = Flexus::Simics::Processor::getProcessor(theInstruction->cpu())->readF( theReg );
  unsigned long long flexus_align = ( ( flexus_hi << 32 ) | (flexus_lo & 0xFFFFFFFFULL) );
  if (flexus_align != simics) {
    DBG_( Verb, ( << *theInstruction << " overriding simics f-register " << theReg << " = " << std::hex << simics << " with flexus " << theOperandCodeHi << ":" << theOperandCodeLo << " = " << flexus_align << std::dec ));
    Flexus::Simics::Processor::getProcessor(theInstruction->cpu())->writeF( theReg, flexus_align );
  }
}


bool validateRegister::operator () () {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; //Don't check
  }
  if (theInstruction->raised()) {
    DBG_( Verb, ( << " Not performing register validation for " << theReg << " because of exception. " << *theInstruction ) );
    return true;
  }
  unsigned long long flexus = theInstruction->operand< unsigned long long > (theOperandCode);
  unsigned long long simics = 0;
  if (theReg == nuArch::kRegY) {
    simics = Flexus::Simics::Processor::getProcessor(theInstruction->cpu())->readRegister( kY );
  } else if (theReg == nuArch::kRegASI) {
    simics = Flexus::Simics::Processor::getProcessor(theInstruction->cpu())->readRegister( kASI );
  } else if (theReg == nuArch::kRegGSR) {
    simics = Flexus::Simics::Processor::getProcessor(theInstruction->cpu())->readRegister( kGSR );
  } else {
    simics = Flexus::Simics::Processor::getProcessor(theInstruction->cpu())->readRegister( theReg );
  }
  DBG_( Dev, Condition( flexus != simics) ( << "Validation Mismatch for reg " << theReg << " flexus=" << std::hex <<flexus << " simics=" << simics << std::dec << "\n" << std::internal << *theInstruction ) );
  return (flexus == simics);
}

bool validateFRegister::operator () () {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; //Don't check
  }
  unsigned long long flexus = theInstruction->operand< unsigned long long > (theOperandCode);
  unsigned long long simics = Flexus::Simics::Processor::getProcessor(theInstruction->cpu())->readF( theReg & (~1) );
  if (theReg & 1) {
    simics &= 0xFFFFFFFFULL;
  } else {
    simics >>= 32;
  }
  DBG_( Dev, Condition( flexus != simics) ( << "Validation Mismatch for reg " << theReg << " flexus=" << std::hex <<flexus << " simics=" << simics << std::dec << "\n" << std::internal << *theInstruction ) );
  return (flexus == simics);
}


bool validateCC::operator () () {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; //Don't check
  }
  std::bitset<8> flexus( theInstruction->operand< std::bitset<8> > (theOperandCode) );
  std::bitset<8> simics( Flexus::Simics::Processor::getProcessor(theInstruction->cpu())->readRegister( kCCR ) );
  DBG_( Dev, Condition( flexus != simics) ( << "Validation Mismatch for CC bits flexus=" << flexus << " simics=" << simics << "\n" << std::internal << *theInstruction ) );
  return (flexus == simics);
}

bool validateFCC::operator () () {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; //Don't check
  }
  std::bitset<8> flexus( theInstruction->operand< std::bitset<8> > (theOperandCode) );
  unsigned int flexus_fcc = flexus.to_ulong() & 3;
  unsigned long long fsr = Flexus::Simics::Processor::getProcessor(theInstruction->cpu())->readRegister( kFSR );
  unsigned int simics_fcc = 0;
  switch (theFCC) {
    case 0:
      simics_fcc = ( fsr >> 10) & 3;
      break;
    case 1:
      simics_fcc = ( fsr >> 32) & 3;
      break;
    case 2:
      simics_fcc = ( fsr >> 34) & 3;
      break;
    case 3:
      simics_fcc = ( fsr >> 36) & 3;
      break;
    default:
      simics_fcc = 0xFFFF; //Force failure;
  }
  DBG_( Dev, Condition( flexus_fcc != simics_fcc) ( << "Validation Mismatch for FCC val flexus=" << fccVals(flexus_fcc) << " simics=" << fccVals(simics_fcc) << "\n" << std::internal << *theInstruction ) );
  return (flexus_fcc == simics_fcc);
}

bool validateMemory::operator () () {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; //Don't check
  }
  VirtualMemoryAddress flexus_addr(theInstruction->operand< unsigned long long > (theAddressCode));
  if (theInstruction->hasOperand( kUopAddressOffset ) ) {
    unsigned long long offset = theInstruction->operand< unsigned long long > (kUopAddressOffset);
    flexus_addr += offset;
  }

  int asi( theInstruction->operand< unsigned long long > (theASICode) );
  flexus_addr += theAddrOffset;
  unsigned long long flexus_value = theInstruction->operand< unsigned long long > (theValueCode);
  switch (theSize) {
      case kByte:
        flexus_value &= 0xFFULL;
        break;
      case kHalfWord:
        flexus_value &= 0xFFFFULL;
        break;
      case kWord:
        flexus_value &= 0xFFFFFFFFULL;
        break;
      case kDoubleWord:
      default:
        break;
  }

  Flexus::Simics::Processor c = Flexus::Simics::Processor::getProcessor(theInstruction->cpu());
  Flexus::Simics::Translation xlat;
  xlat.theVaddr = flexus_addr;
  xlat.theType = Flexus::Simics::Translation::eLoad;
  xlat.theTL = c->readRegister( kTL );
  xlat.thePSTATE = c->readRegister( kPSTATE );
  xlat.theASI = asi;
  c->translate(xlat, false);
  if (xlat.isMMU()) {
    //bool mmu_ok = Flexus::Simics::Processor::getProcessor(theInstruction->cpu())->validateMMU();
    //if (! mmu_ok) {
      //DBG_( Dev, Condition( ! mmu_ok) ( << "Validation Mismatch for MMUs\n" << std::internal << *theInstruction ) );
      //Flexus::Simics::Processor::getProcessor(theInstruction->cpu())->dumpMMU();
    //}
    return true;
  } else if (xlat.thePaddr > 0x400000000LL) {
    DBG_( Iface, ( << "Non-memory store " << std::hex << asi<< " flexus=" << flexus_value << " Insn: " << *theInstruction ) );
    return true;
  } else if (xlat.isTranslating() && !xlat.isSideEffect()) {
    unsigned long long simics_value = c->readVAddrXendian_SimicsImpl( xlat.theVaddr, xlat.theASI, static_cast<int>(theSize) );
    DBG_( Dev, Condition( flexus_value != simics_value) ( << "Validation Mismatch for address " << flexus_addr << " flexus=" << std::hex << flexus_value << " simics=" << simics_value << std::dec << "\n" << std::internal << *theInstruction ) );
    return (flexus_value == simics_value);
  } else {
    DBG_( Iface, ( << "No validation available for ASI 0x" << std::hex << asi<< " flexus=" << flexus_value << " Insn: " << *theInstruction ) );
    return true;
  }
}

unsigned int simicsReg( unsigned int aPR) {
     switch (aPR) {
      case 0: //TPC
        return  kTPC1;
      case 1: //NTPC
        return  kTNPC1;
      case 2: //TSTATE
        return  kTSTATE1;
      case 3: //TT
        return  kTT1;
      case 5: //TBA
        return  kTBA;
      case 6: //PSTATE
        return  kPSTATE;
      case 7: //TL
        return  kTL;
      case 8: //PIL
        return  kPIL;
      case 9: //CWP
        return  kCWP;
      case 10: //CANSAVE
        return  kCANSAVE;
      case 11: //CANRESTORE
        return  kCANRESTORE;
      case 12: //CLEANWIN
        return  kCLEANWIN;
      case 13: //OTHERWIN
        return  kOTHERWIN;
      case 14: //WSTATE
        return  kWSTATE;
      default:
        DBG_( Verb, ( << "Validate invalid PR: " << aPR ) );
        return 0;
        break;
    }
}

bool validateTPR::operator () () {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; //Don't check
  }
  unsigned long long flexus_value = theInstruction->operand< unsigned long long > (theOperandCode);
  unsigned long long tl = theInstruction->operand< unsigned long long > (theTLOperandCode);
  unsigned long long simics_value = Flexus::Simics::Processor::getProcessor(theInstruction->cpu())->readRegister( simicsReg( thePR ) + tl - 1);
  DBG_( Dev, Condition( flexus_value != simics_value) ( << "Validation Mismatch for " << theInstruction->core()->prName( thePR ) <<" flexus=" << std::hex << flexus_value << " simics=" << simics_value << std::dec << "\n" << std::internal << *theInstruction ) );
  return (flexus_value == simics_value);
}

bool doValidatePR( SemanticInstruction * theInstruction, eOperandCode theOperandCode, unsigned int thePR) {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; //Don't check
  }
  unsigned long long flexus_value = theInstruction->operand< unsigned long long > (theOperandCode);
  unsigned long long simics_value = Flexus::Simics::Processor::getProcessor(theInstruction->cpu())->readRegister( simicsReg( thePR )  );
  DBG_( Dev, Condition( flexus_value != simics_value) ( << "Validation Mismatch for " << theInstruction->core()->prName( thePR ) <<" flexus=" << std::hex << flexus_value << " simics=" << simics_value << std::dec << "\n" << std::internal << *theInstruction ) );
  return (flexus_value == simics_value);
}

bool validatePR::operator () () {
  return doValidatePR( theInstruction, theOperandCode, thePR);
}


bool validateSaveRestore::operator () () {
  bool ok = true;
  if (theInstruction->raised()) {
    DBG_( Verb, ( << "Save or restore instruction raised: \n" << *theInstruction ) );
    return true;
  } else {
    ok = ok && doValidatePR( theInstruction, kocCWP, 9 /*CWP*/ );
    ok = ok && doValidatePR( theInstruction, kocCANSAVE, 10 /*CANSAVE*/ );
    ok = ok && doValidatePR( theInstruction, kocCANRESTORE, 11 /*CANRESTORE*/ );
    return ok;
  }
}

bool validateFPRS::operator () () {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; //Don't check
  }

  unsigned long long flexus_value = theInstruction->operand< unsigned long long > (kocFPRS);
  unsigned long long simics_value = Flexus::Simics::Processor::getProcessor(theInstruction->cpu())->readRegister( kFPRS );
  DBG_( Dev, Condition( flexus_value != simics_value) ( << "Validation Mismatch for FPRS flexus=" << std::hex << flexus_value << " simics=" << simics_value << std::dec << "\n" << std::internal << *theInstruction ) );
  return (flexus_value == simics_value);
}

bool validateFSR::operator () () {
  if (theInstruction->isSquashed() || theInstruction->isAnnulled()) {
    return true; //Don't check
  }
  unsigned long long flexus_value = theInstruction->operand< unsigned long long > (kocFSR);
  unsigned long long simics_value = Flexus::Simics::Processor::getProcessor(theInstruction->cpu())->readRegister( kFSR );
  DBG_( Dev, Condition( flexus_value != simics_value) ( << "Validation Mismatch for FSR flexus=" << std::hex << flexus_value << " simics=" << simics_value << std::dec << "\n" << std::internal << *theInstruction ) );
  return (flexus_value == simics_value);
}

} //nv9Decoder
