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

#ifndef FLEXUS_uARCH_microARCH_HPP_INCLUDED
#define FLEXUS_uARCH_microARCH_HPP_INCLUDED

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <components/Common/Slices/MemOp.hpp>
#include <components/Common/Slices/PredictorMessage.hpp> /* CMU-ONLY */

namespace Flexus {
namespace SharedTypes {
  struct BranchFeedback;
}
}

namespace nuArch {

using Flexus::SharedTypes::VirtualMemoryAddress;


struct microArch {
  static boost::shared_ptr<microArch>
    construct( uArchOptions_t options
             , boost::function< void(eSquashCause)> squash
             , boost::function< void(VirtualMemoryAddress, VirtualMemoryAddress)> redirect
             , boost::function< void(int, int)> changeState
             , boost::function< void( boost::intrusive_ptr<BranchFeedback> )> feedback
             , boost::function< void (PredictorMessage::tPredictorMessageType, PhysicalMemoryAddress, boost::intrusive_ptr<TransactionTracker> )> notifyTMS /* CMU-ONLY */
             , boost::function< void(bool) > aStoreForwardingHitFunction
             );

  virtual int availableROB() = 0;
  virtual bool isSynchronized() = 0;
  virtual bool isQuiesced() = 0;
  virtual bool isStalled() = 0;
  virtual int iCount() = 0;
  virtual void dispatch(boost::intrusive_ptr< AbstractInstruction >) = 0;
  virtual void skipCycle() = 0;
  virtual void cycle() = 0;
  virtual void pushMemOp(boost::intrusive_ptr< MemOp >) = 0;
  virtual bool canPushMemOp() = 0;
  virtual boost::intrusive_ptr<MemOp> popMemOp() = 0;
  virtual boost::intrusive_ptr<MemOp> popSnoopOp() = 0;
  virtual boost::intrusive_ptr<MemOp> popPurgeOp() = 0; /* CMU-ONLY */
  virtual ~microArch() {}
  virtual void testCkptRestore() = 0;
  virtual void printROB() = 0;
  virtual void printSRB() = 0;
  virtual void printMemQueue() = 0;
  virtual void printMSHR() = 0;
  virtual void pregs() = 0;
  virtual void pregsAll() = 0;
  virtual void resynchronize() = 0;
  virtual void printRegMappings(std::string) = 0;
  virtual void printRegFreeList(std::string) = 0;
  virtual void printRegReverseMappings(std::string) = 0;
  virtual void printAssignments(std::string) = 0;
  virtual void writePermissionLost(PhysicalMemoryAddress anAddress) = 0;

  /* CMU-ONLY-BLOCK-BEGIN */
  // Purge implementation for R-NUCA
  virtual void initializePurge( boost::intrusive_ptr< MemOp > anOp ) = 0;
  virtual void ackPurge( boost::intrusive_ptr< MemOp > anOp ) = 0;
  virtual void finalizePurge( boost::intrusive_ptr< MemOp > anOp ) = 0;
  /* CMU-ONLY-BLOCK-END */

};

} //nuArch

#endif //FLEXUS_uARCH_microARCH_HPP_INCLUDED
