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

#ifndef FLEXUS_PROTOCOL_ENGINE_THREAD_HPP
#define FLEXUS_PROTOCOL_ENGINE_THREAD_HPP

#include <core/debug/debug.hpp>

#include "tsrf.hpp"

namespace nProtocolEngine {

struct tThread {
  private:
    tTsrfEntry * theTSRFEntry;
    int theThreadID;

  public:
    tThread()
      : theTSRFEntry(0)
      , theThreadID(-1)
      {}

    tThread(tTsrfEntry * anEntry, int aThreadID, long long aCreationTime)
      : theTSRFEntry(anEntry)
      , theThreadID(aThreadID)
    {
      if (theTSRFEntry) {
        theTSRFEntry->setCreationTime(aCreationTime);
      }
    }

    bool operator ==( tThread const & anOther) const {
      return theTSRFEntry == anOther.theTSRFEntry;
    }
    bool operator <(tThread const & anOther) const {
      return theTSRFEntry < anOther.theTSRFEntry;
    }

    //Thread state & TSRF inquiry
      bool isValid() const { return theTSRFEntry != 0; }
      bool isState(tThreadState aState) const { DBG_Assert(isValid()); return theTSRFEntry->state() == aState; }
      bool isRunnable() const { return isValid() && (theTSRFEntry->state() == eRunnable ); }
      bool isWaiting() const { return isValid() && (theTSRFEntry->state() == eWaiting); }
      bool isSuspendedForLock() const { return isValid() && (theTSRFEntry->state() == eSuspendedForLock); }
      bool isBlocked() const { return isValid() && (theTSRFEntry->state() == eBlocked); }
      tThreadState state() const { DBG_Assert(isValid()); return theTSRFEntry->state(); }

      unsigned int programCounter() const { DBG_Assert(isValid()); return theTSRFEntry->programCounter(); }
      int threadID() const { DBG_Assert(isValid()); return theThreadID; }
      node_id_t requester() const { DBG_Assert(isValid()); return theTSRFEntry->requester(); }
      node_id_t respondent() const { DBG_Assert(isValid()); return theTSRFEntry->respondent(); }
      node_id_t fwdRequester() const { DBG_Assert(isValid()); return theTSRFEntry->fwdRequester(); }
      node_id_t racer() const { DBG_Assert(isValid()); return theTSRFEntry->racer(); }
      node_id_t invAckReceiver() const { DBG_Assert(isValid()); return theTSRFEntry->invAckReceiver(); }
      tAddress address() const { DBG_Assert(isValid()); return theTSRFEntry->address(); }
      unsigned invalidationCount() const { DBG_Assert(isValid()); return theTSRFEntry->invalidationCount(); }
      unsigned invReceived() const { DBG_Assert(isValid()); return theTSRFEntry->invReceived(); }
      bool anyInvalidations() const { DBG_Assert(isValid()); return theTSRFEntry->anyInvalidations(); }
      node_id_t owner() const { DBG_Assert(isValid()); return theTSRFEntry->owner(); }
      unsigned tempReg() const { DBG_Assert(isValid()); return theTSRFEntry->tempReg(); }
      bool isPrefetch() const { DBG_Assert(isValid()); return theTSRFEntry->isPrefetch(); }
      bool isSharer(node_id_t aNode) const { DBG_Assert(isValid()); return theTSRFEntry->isSharer(aNode); }
      bool isOnlySharer(node_id_t aNode) const { DBG_Assert(isValid()); return theTSRFEntry->isOnlySharer(aNode); }
      tVC VC() const { DBG_Assert(isValid()); return theTSRFEntry->VC(); }
      tDirEntry const & dirEntry() const { DBG_Assert(isValid()); return theTSRFEntry->dirEntry(); }
      tDirState dirState() const { DBG_Assert(isValid()); return theTSRFEntry->dirEntry().state(); }
      tMessageType type() const { DBG_Assert(isValid()); return theTSRFEntry->type(); }
      unsigned long long startTime() const { DBG_Assert(isValid()); return theTSRFEntry->startTime(); }
      unsigned long long creationTime() const { DBG_Assert(isValid()); return theTSRFEntry->creationTime(); }
      unsigned entryPointPC() const { DBG_Assert(isValid()); return theTSRFEntry->entryPointPC(); }
      bool invAckForFwdExpected() const { DBG_Assert(isValid()); return theTSRFEntry->invAckForFwdExpected(); }
      bool downgradeAckExpected() const { DBG_Assert(isValid()); return theTSRFEntry->downgradeAckExpected(); }
      bool requestReplyToRacer() const { DBG_Assert(isValid()); return theTSRFEntry->requestReplyToRacer(); }
      bool fwdReqOrStaleAckExpected() const { DBG_Assert(isValid()); return theTSRFEntry->fwdReqOrStaleAckExpected(); }
      bool writebackAckExpected() const { DBG_Assert(isValid()); return theTSRFEntry->writebackAckExpected(); }
      boost::intrusive_ptr<TransactionTracker> transactionTracker() const { DBG_Assert(isValid()); return theTSRFEntry->transactionTracker();}
      boost::intrusive_ptr<tPacket> packet() { DBG_Assert(isValid()); return theTSRFEntry->packet();}
      int blockedOn() const { DBG_Assert(isBlocked()); return theTSRFEntry->blockedOn(); }

      bool wasModified() const { DBG_Assert(isValid()); return theTSRFEntry->wasModified(); }
      bool wasSharer(node_id_t aNode) const { DBG_Assert(isValid()); return theTSRFEntry->wasSharer(aNode); }
      bool wasNoOtherSharer(node_id_t aNode) const { DBG_Assert(isValid()); return theTSRFEntry->wasNoOtherSharer(aNode); }


    //Modify TSRF
      void setProgramCounter(unsigned aPC) {
          DBG_Assert(isValid());
          theTSRFEntry->setProgramCounter(aPC);
          if (aPC == 0) {
            //Setting the program counter to 0 halts the thread
            halt();
          }
      }
      void setAddress(tAddress anAddr) { DBG_Assert(isValid()); theTSRFEntry->setAddress(anAddr); }
      void setType(tMessageType aType) { DBG_Assert(isValid()); theTSRFEntry->setType(aType); }
      void setInvalidationCount(unsigned aCount) { DBG_Assert(isValid()); theTSRFEntry->setInvalidationCount(aCount); }
      void setAnyInvalidations(bool aFlag) { DBG_Assert(isValid()); theTSRFEntry->setAnyInvalidations(aFlag); }
      void setPrefetch(bool aFlag) { DBG_Assert(isValid()); theTSRFEntry->setPrefetch(aFlag); }
      void setTempReg(unsigned aVal) { DBG_Assert(isValid()); theTSRFEntry->setTempReg(aVal); }
      void relativeJump(unsigned aPCDelta) {
          DBG_Assert(isValid());
          setProgramCounter( programCounter() + aPCDelta );
      }
      void deferJump(unsigned aPCDelta) { DBG_Assert(isValid()); theTSRFEntry->setDeferredJump(aPCDelta); }
      void resolveDeferredJump() {
          DBG_Assert(isValid());
          setProgramCounter( programCounter() + theTSRFEntry->deferredJump());
      }
      void setDirEntry(boost::intrusive_ptr<tDirEntry> & aDirEntry) { DBG_Assert(isValid()); theTSRFEntry->setDirEntry(aDirEntry); }
      void setDirState(tDirState aState) { DBG_Assert(isValid()); theTSRFEntry->setDirState(aState); }
      void setRespondent(node_id_t aNode) { DBG_Assert(isValid()); theTSRFEntry->setRespondent(aNode); }
      void setRequester(node_id_t aNode) { DBG_Assert(isValid()); theTSRFEntry->setRequester(aNode); }
      void setRacer(node_id_t aNode) { DBG_Assert(isValid()); theTSRFEntry->setRacer(aNode); }
      void setOwner(node_id_t aNode) { DBG_Assert(isValid()); theTSRFEntry->setOwner(aNode); }
      void setFwdRequester(node_id_t aNode) { DBG_Assert(isValid()); theTSRFEntry->setFwdRequester(aNode); }
      void setInvAckReceiver(node_id_t aNode) { DBG_Assert(isValid()); theTSRFEntry->setInvAckReceiver(aNode); }
      void clearSharers() { DBG_Assert(isValid()); theTSRFEntry->clearSharers(); }
      void setAllSharers() { DBG_Assert(isValid()); theTSRFEntry->setAllSharers(); }
      void addSharer(node_id_t aNode) { DBG_Assert(isValid()); theTSRFEntry->addSharer(aNode); }
      void clearSharer(node_id_t aNode) { DBG_Assert(isValid()); theTSRFEntry->clearSharer(aNode); }
      void setStartTime(unsigned long long aTime) { DBG_Assert(isValid()); theTSRFEntry->setStartTime(aTime); }
      void setEntryPC(unsigned aPC) { DBG_Assert(isValid()); theTSRFEntry->setEntryPC(aPC); }
      void setInvAckForFwdExpected(bool aVal) { DBG_Assert(isValid()); theTSRFEntry->setInvAckForFwdExpected(aVal); }
      void setDowngradeAckExpected(bool aVal) { DBG_Assert(isValid()); theTSRFEntry->setDowngradeAckExpected(aVal); }
      void setRequestReplyToRacer(bool aVal) { DBG_Assert(isValid()); theTSRFEntry->setRequestReplyToRacer(aVal); }
      void setFwdReqOrStaleAckExpected(bool aVal) { DBG_Assert(isValid()); theTSRFEntry->setFwdReqOrStaleAckExpected(aVal); }
      void setWritebackAckExpected(bool aVal) { DBG_Assert(isValid()); theTSRFEntry->setWritebackAckExpected(aVal); }
      void setInvReceived(unsigned aVal) { DBG_Assert(isValid()); theTSRFEntry->setInvReceived(aVal); }
      void setVC(tVC aVC) { DBG_Assert(isValid()); theTSRFEntry->setVC(aVC); }
      void setTransactionTracker(boost::intrusive_ptr<TransactionTracker> aTracker) { DBG_Assert(isValid()); theTSRFEntry->setTransactionTracker(aTracker); }
      void setDelayCause(std::string aComponent, std::string aDelayCause) {
        if (transactionTracker()) {
          transactionTracker()->setDelayCause(aComponent, aDelayCause);
        }
      }
      void setPacket(boost::intrusive_ptr<tPacket> aPacket) { DBG_Assert(isValid()); theTSRFEntry->setPacket(aPacket); }
      int  getStallCycles() { DBG_Assert(isValid()); return theTSRFEntry->getStallCycles(); }
      void resetStallCycles() { DBG_Assert(isValid()); theTSRFEntry->resetStallCycles(); }
      void decStallCycles() { DBG_Assert(isValid()); theTSRFEntry->decStallCycles(); }
      void setCPI(int cpi) { DBG_Assert(isValid()); theTSRFEntry->setCPI(cpi); }
      int  getCPI() { DBG_Assert(isValid()); return theTSRFEntry->getCPI(); }

      unsigned int uopCount() const { DBG_Assert(isValid()); return theTSRFEntry->uopCount(); }
      void incrUopCount() { DBG_Assert(isValid()); theTSRFEntry->incrUopCount(); }
      void updateDirEntry() { DBG_Assert(isValid()); theTSRFEntry->updateDirEntry(); }


    //Change thread state
      tTsrfEntry * vacate() {
        tTsrfEntry * temp(theTSRFEntry);
        DBG_Assert(isValid());
        theTSRFEntry->vacate();
        theTSRFEntry = 0;
        theThreadID = 0;
        return temp;
      }

      void halt() {
        DBG_Assert(isValid());
        DBG_Assert(programCounter() == 0);
        theTSRFEntry->setState(eComplete);
      }

      void cancel() {
        DBG_Assert(isSuspendedForLock() || isBlocked());
        theTSRFEntry->setProgramCounter(0);
        theTSRFEntry->setState(eComplete);
      }

      void wait() {
        DBG_Assert(isValid());
        theTSRFEntry->setState(eWaiting);
      }

      void block(tThread aBlockOn) {
        DBG_Assert(isValid());
        theTSRFEntry->setState(eBlocked);
        theTSRFEntry->setBlockedOn(aBlockOn.threadID());
      }

      void waitForLock() {
        DBG_Assert(isValid());
        theTSRFEntry->setState(eSuspendedForLock);
      }

      void waitForDirectory() {
        DBG_Assert(isValid());
        theTSRFEntry->setState(eSuspendedForDir);
      }

      void markRunnable() {
        DBG_Assert(isValid());
        theTSRFEntry->setState(eRunnable);
      }

      friend std::ostream & operator << ( std::ostream & anOstream, tThread const & aThread);
};


}  // namespace nProtocolEngine

#endif // FLEXUS_PROTOCOL_ENGINE_THREAD_HPP
