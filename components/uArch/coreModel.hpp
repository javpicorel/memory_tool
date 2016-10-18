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

#ifndef FLEXUS_uARCH_COREMODEL_HPP_INCLUDED
#define FLEXUS_uARCH_COREMODEL_HPP_INCLUDED

#include "uArchInterfaces.hpp"

#include <components/uFetch/uFetchTypes.hpp>
#include <components/Common/Slices/PredictorMessage.hpp> /* CMU-ONLY */

namespace Flexus {
namespace Simics {
struct Translation;
} //Simics
} //Flexus

namespace nuArch {

enum eLoseWritePermission 
  { eLosePerm_Invalidate
  , eLosePerm_Downgrade
  , eLosePerm_Replacement
  };

struct v9State {
  unsigned long long theGlobalRegs[32];
  unsigned long long theWindowRegs[128];
  unsigned long long theFPRegs[64];
  unsigned long long thePC;
  unsigned long long theNPC;
  unsigned long long theTBA;
  unsigned long long theCWP;
  unsigned long long theCCR;
  unsigned long long theFPRS;
  unsigned long long theFSR;
  unsigned long long thePSTATE;
  unsigned long long theASI;
  unsigned long long theGSR;
  unsigned long long theTL;
  unsigned long long thePIL;
  unsigned long long theCANSAVE;
  unsigned long long theCANRESTORE;
  unsigned long long theCLEANWIN;
  unsigned long long theOTHERWIN;
  unsigned long long theSOFTINT;
  unsigned long long theWSTATE;
  unsigned long long theY;
  unsigned long long theGLOBALS;
  unsigned long long theTICK;
  unsigned long long theTICK_CMPR;
  unsigned long long theSTICK;
  unsigned long long theSTICK_CMPR;
  struct tte {
    unsigned long long theTPC;
    unsigned long long theTNPC;
    unsigned long long theTSTATE;
    unsigned long long theTT;
  } theTTE[5];
};


struct CoreModel : public uArch {
  static CoreModel * construct(uArchOptions_t options
                              , boost::function< void (Flexus::Simics::Translation &, bool) > translate
                              , boost::function< int(bool) > advance
                              , boost::function< void(eSquashCause) > squash
                              , boost::function< void(VirtualMemoryAddress, VirtualMemoryAddress) > redirect
                              , boost::function< void(int, int) > change_mode
                              , boost::function< void( boost::intrusive_ptr<BranchFeedback> )> feedback
                              , boost::function< void (PredictorMessage::tPredictorMessageType, PhysicalMemoryAddress, boost::intrusive_ptr<TransactionTracker> )> notifyTMS /* CMU-ONLY */
                              , boost::function< void( bool )> signalStoreForwardingHit
                              );

  //Interface to mircoArch
  virtual void initializeRegister(mapped_reg aRegister, register_value aValue) = 0;
  virtual register_value readArchitecturalRegister( unmapped_reg aRegister, bool aRotate ) = 0;
  virtual int selectedRegisterSet() const = 0;
  virtual void setRoundingMode(unsigned int aRoundingMode) = 0;


  virtual void setTPC( unsigned long long aTPC, unsigned int aTL) = 0;
  virtual void setTNPC( unsigned long long aTNPC, unsigned int aTL) = 0;
  virtual void setTSTATE( unsigned long long aTSTATE, unsigned int aTL) = 0;
  virtual void setTT( unsigned int aTT, unsigned int aTL) = 0;
  virtual void setTBA( unsigned long long aTBA) = 0;
  virtual void setPSTATE( unsigned long long aPSTATE) = 0;
  virtual unsigned long long getPSTATE( ) = 0;
  virtual void setTL( unsigned int aTL) = 0;
  virtual unsigned int getTL( ) = 0;
  virtual void setPIL( unsigned int aPIL) = 0;
  virtual void setCWP( unsigned int aCWP ) = 0;
  virtual void setCLEANWIN( unsigned int aCLEANWIN ) = 0;
  virtual void setCANSAVE( unsigned int aCANSAVE ) = 0;
  virtual void setCANRESTORE( unsigned int aCANRESTORE) = 0;
  virtual void setOTHERWIN( unsigned int aOTHERWIN) = 0;
  virtual void setWSTATE( unsigned int aWSTATE ) = 0;
  virtual void setVER( unsigned long long aVER ) = 0;
  virtual void setTICK( unsigned long long aTick ) = 0;
  virtual void setSTICK( unsigned long long aStick ) = 0;
  virtual void setSTICKInterval( unsigned long long aStickInterval ) = 0;
  virtual void setAG( bool ) = 0;
  virtual void setIG( bool ) = 0;
  virtual void setMG( bool ) = 0;
  virtual void setFSR( unsigned long long anFSR) = 0;

  virtual void getv9State( v9State & aState) = 0;
  virtual void restorev9State( v9State & aState) = 0;
  virtual void setPC( unsigned long long aPC) = 0;
  virtual void setNPC( unsigned long long anNPC) = 0;
  virtual unsigned long long pc() const = 0;
  virtual unsigned long long npc() const = 0;


  virtual void dumpActions() = 0;
  virtual void reset() = 0;
  virtual void resetv9() = 0;

  virtual int availableROB() const = 0;
  virtual bool isSynchronized() const = 0;
  virtual bool isStalled() const = 0;
  virtual int iCount() const = 0;
  virtual bool isQuiesced() const = 0;
  virtual void dispatch(boost::intrusive_ptr<Instruction>) = 0;

  virtual void skipCycle() = 0;
  virtual void cycle(int aPendingInterrupt) = 0;

  virtual void pushMemOp(boost::intrusive_ptr< MemOp >) = 0;
  virtual bool canPushMemOp() = 0;
  virtual boost::intrusive_ptr<MemOp> popMemOp() = 0;
  virtual boost::intrusive_ptr<MemOp> popSnoopOp() = 0;
  virtual boost::intrusive_ptr<MemOp> popPurgeOp() = 0; /* CMU-ONLY */
  virtual void printROB() = 0;
  virtual void printSRB() = 0;
  virtual void printMemQueue() = 0;
  virtual void printMSHR() = 0;

  virtual void printRegMappings(std::string) = 0;
  virtual void printRegFreeList(std::string) = 0;
  virtual void printRegReverseMappings(std::string) = 0;
  virtual void printAssignments(std::string) = 0;

  virtual void loseWritePermission( eLoseWritePermission aReason, PhysicalMemoryAddress anAddress) = 0;

  /* CMU-ONLY-BLOCK-BEGIN */
  // Purge implementation for R-NUCA
  virtual void initializePurge( boost::intrusive_ptr< MemOp > anOp ) = 0;
  virtual void ackPurge( boost::intrusive_ptr< MemOp > anOp ) = 0;
  virtual void finalizePurge( boost::intrusive_ptr< MemOp > anOp ) = 0;
  /* CMU-ONLY-BLOCK-END */

};

struct ResynchronizeWithSimicsException {
  bool expected;
  ResynchronizeWithSimicsException(bool was_expected = false )  : expected(was_expected) {}
};

} //nuArch

#endif //FLEXUS_uARCH_COREMODEL_HPP_INCLUDED
