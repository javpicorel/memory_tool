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

#ifndef _TSRF_H_
#define _TSRF_H_

#include <vector>
#include <list>

#include <core/debug/debug.hpp>

#include <core/boost_extensions/intrusive_ptr.hpp>

#include "ProtSharedTypes.hpp"

#include "util.hpp"



namespace nProtocolEngine {

enum tThreadState {
  eNoThread,          // No Thread : there is no thread here
  eWaiting,           // Waiting   : corresponding thread waiting for packet
  eSuspendedForLock,  // Suspended : corresponding thread waiting for lock (suspended)
  eSuspendedForDir,   // Suspended : corresponding thread waiting for directory (suspended)
  eRunnable,          // Active    : work on it
  eComplete,          // Complete  : thread is done and needs to be recycled
  eBlocked            // Blocked   : corresponding thread waiting for another thread to complete
};

std::ostream & operator << (std::ostream & anOstream, tThreadState x);

class tTsrfEntry {

    // ***** Transaction info *****
    unsigned                    thePC;                        // program counter
                                                              // NOTE: with MASM, there is a one-to-one relation
                                                              // between packet type and microcode PC, so the
                                                              // packet type can be stored here when the thread
                                                              // is suspended and added later. We decouple them
                                                              // however, for the sake of clarity and abstraction.
    tThreadState                theState;                     // state of this entry
    boost::optional<unsigned>   theInvalidationCount;         // counts invalidations sent or InvalAcks received
    boost::optional<unsigned>   theInvalidationReceived;      // 2-bit reg indicates the type of invalidation received (local/remote/nothing)
    boost::optional<node_id_t>  theInvAckReceiver;            // node which is supposed to receive the invAck.
    boost::optional<node_id_t>  theFwdRequester;              // node which is supposed to receive the ReadFwd/WriteFwd reply.

    boost::intrusive_ptr<tPacket> thePacket;

    bool                        theInvAckForFwdExpected;
    bool                        theDowngradeAckExpected;
    bool                        theRequestReplyToRacer;
    bool                        theFwdReqOrStaleAckExpected;
    bool                        theWritebackAckExpected;
    bool                        theAnyInvalidations;

    bool                        thePrefetchRead;              // flag indicating the read req flavor

    // ***** other internal registers *****
    boost::optional<unsigned long> theTempReg;                // scratch internal register
    tVC                         theVC;                        // remember which queue the request (or reply) came from
    boost::optional<unsigned>   theDeferredJump;              // support for 2-level receives: implement the 1st level with
                                                              // the receipt of a dummy message, which causes the microcode
                                                              // to use the 2nd level receive instruction. That one uses
                                                              // the DeferredJump temp register to implement the actual jump.

    // ***** message info *****
    tAddress                    theAddress;                   // address message pertains to
    tMessageType                theType;                      // request type
                                                              // NOTE: in a real world implementation this is
                                                              // not needed. Look at the comment for the pc.
    boost::optional<node_id_t>  theRequester;                 // request originating node (thread creator)
    boost::optional<node_id_t>  theRespondent;                // reply originating node (response creator)
    boost::optional<node_id_t>  theRacer;                     // racing node (in a race situation, the node
                                                              // that beat us)

    // ***** Directory info *****
    //
    // This is used only by the home engine.
    // NOTE: It is not necessary to store the directory info in the tsrf.
    // If we do not, the execution engine may have to re-read the directory
    // every time the suspended/waiting thread returns to active state.
    //
    boost::intrusive_ptr<tDirEntry> theDirEntry;              // directory entry
    tDirState                   theDirState;
    unsigned int                theOwner;
    unsigned long long          theSharers;

    // ***** for stats *****
    unsigned long long          theThreadStartTimestamp;      // timestamp (cycle count) taken when thread began executing
    unsigned long long          theThreadCreationTime;
    unsigned                    theEntryPointPC;              // entry point pc for categorization
    int                         theBlockedOn;                 // thread on which this thread is blocked

    boost::intrusive_ptr<TransactionTracker> theTransactionTracker;

    int theStallCycles;
    int theCPI;
    unsigned int theUopCount;

  public:
    tTsrfEntry() {
      vacate();
    }

    void vacate() {
      thePC = 0;
      theState = eNoThread;
      theInvalidationCount.reset(0);
      theInvalidationReceived.reset();
      theInvAckReceiver.reset();
      theFwdRequester.reset();
      theInvAckForFwdExpected = false;
      theDowngradeAckExpected = false;
      theRequestReplyToRacer = false;
      theFwdReqOrStaleAckExpected = false;
      theWritebackAckExpected = false;
      thePrefetchRead = false;
      theAnyInvalidations = false;
      theTempReg.reset(0);
      theVC = VC0;
      theDeferredJump.reset();
      theAddress = tAddress(0);
      theType = Protocol::ProtocolError;
      theRequester.reset();
      theRespondent.reset();
      theRacer.reset();
      theDirEntry = 0;
      theDirState = DIR_STATE_INVALID;
      theOwner = 0;
      theSharers = 0;
      theThreadStartTimestamp = 0;
      theEntryPointPC = 0;
      theBlockedOn = 0;
      theTransactionTracker = 0;
      theThreadCreationTime = 0;
      thePacket = 0;
      theUopCount = 0;
    }

    tThreadState state() const { return theState; }
    unsigned programCounter() const { DBG_Assert(thePC < 2048); return thePC; }
    node_id_t requester() const { DBG_Assert(theRequester); return *theRequester; }
    node_id_t respondent() const { DBG_Assert(theRespondent); return *theRespondent; }
    node_id_t fwdRequester() const { DBG_Assert(theFwdRequester); return *theFwdRequester; }
    node_id_t racer() const { DBG_Assert(theRacer); return *theRacer; }
    node_id_t invAckReceiver() const { DBG_Assert(theInvAckReceiver); return *theInvAckReceiver; }
    tAddress address() const { return theAddress; }
    unsigned invalidationCount() const { DBG_Assert(theInvalidationCount); return *theInvalidationCount;  }

    tDirEntry const & dirEntry() const { DBG_Assert(theDirEntry); return *theDirEntry; }
    bool isSharer(node_id_t aNode) const {
      return (theSharers & ( 1ULL << (unsigned long long)aNode) );
    }
    bool isOnlySharer(node_id_t aNode) const {
      return (theSharers == ( 1ULL << (unsigned long long)aNode) );
    }
    bool wasSharer(node_id_t aNode) const {
      DBG_Assert( theDirEntry );
      return (theDirEntry->getPastReaders() & ( 1ULL << (unsigned long long)aNode) );
    }
    bool wasNoOtherSharer(node_id_t aNode) const {
      DBG_Assert( theDirEntry );
      return ((theDirEntry->getPastReaders() & ~(1ULL << (unsigned long long)aNode) ) == 0);
    }

    node_id_t owner() const {
      return theOwner;
    }

    bool anyInvalidations() const {
      return theAnyInvalidations;
    }
    void setAnyInvalidations(bool aFlag) {
      theAnyInvalidations = aFlag;
    }

    unsigned tempReg() const { DBG_Assert(theTempReg); return *theTempReg;  }
    bool isPrefetch() const { return thePrefetchRead;  }
    tVC VC() const { return theVC; }
    tMessageType type() const { return theType; }
    unsigned long long startTime() const { return theThreadStartTimestamp; }
    unsigned long long creationTime() const { return theThreadCreationTime; }
    unsigned entryPointPC() const { return theEntryPointPC; }
    unsigned invReceived() const { DBG_Assert(theInvalidationReceived); return *theInvalidationReceived; }
    bool invAckForFwdExpected() const { return theInvAckForFwdExpected; }
    bool downgradeAckExpected() const { return theDowngradeAckExpected; }
    bool requestReplyToRacer() const { return theRequestReplyToRacer ; }
    bool fwdReqOrStaleAckExpected() const { return theFwdReqOrStaleAckExpected ; }
    bool writebackAckExpected() const { return theWritebackAckExpected ; }
    unsigned deferredJump() { DBG_Assert(theDeferredJump); return *theDeferredJump; }
    boost::intrusive_ptr<TransactionTracker> transactionTracker() const { return theTransactionTracker; }
    boost::intrusive_ptr<tPacket> packet() { return thePacket; }
    int blockedOn() { return theBlockedOn; }

    bool wasModified() {
      if (theDirState == DIR_STATE_MODIFIED) {
        return true;
      }
      DBG_Assert(theDirEntry);
      return theDirEntry->wasModified();
    }

    void updateDirEntry() {
      DBG_Assert(theDirEntry);
      bool wasInvalid = (theDirEntry->state() == DIR_STATE_INVALID);
      theDirEntry->setState(theDirState);
      if (theDirState == DIR_STATE_INVALID && ! wasInvalid) {
        theDirEntry->markModified();
      }
      if (theDirState == DIR_STATE_MODIFIED) {
        theDirEntry->setOwner(theOwner);
        theDirEntry->markModified();
      } else {
        theDirEntry->setSharers(theSharers);
      }
    }

    void setDirState(tDirState aState) {
      theDirState = aState;
    }
    void setDirEntry(boost::intrusive_ptr<tDirEntry> & aDirEntry) {
      DBG_Assert(aDirEntry);
      theDirEntry = aDirEntry;
      theDirState = theDirEntry->state();
      if (theDirState == DIR_STATE_MODIFIED) {
        theOwner =  theDirEntry->owner();
      } else {
        theSharers =  theDirEntry->sharers();
      }
    }
    void setOwner(node_id_t aNode) {
      theOwner = aNode;
    }
    void clearSharers() {
      theSharers = 0;
    }
    void setAllSharers() {
      theDirEntry->setAllSharers();
    }
    void addSharer(node_id_t aNode) {
      theSharers |= (0x1ULL << (unsigned long long)aNode);
    }
    void clearSharer(node_id_t aNode) {
      theSharers &= ~(0x1ULL << (unsigned long long)aNode);
    }

    void setProgramCounter(unsigned aPC) { DBG_Assert(aPC < 2048); thePC = aPC; }
    void setAddress(tAddress anAddr) { theAddress = anAddr; }
    void setInvalidationCount(unsigned aCount) { theInvalidationCount.reset(aCount); }
    void setPrefetch(bool aFlag) { thePrefetchRead = aFlag; }
    void setTempReg(unsigned aVal) { theTempReg.reset(aVal); }
    void setState(tThreadState aState) { theState = aState; }
    void setType(tMessageType aType) { theType = aType; }
    void setRequester(node_id_t aNode) { theRequester.reset(aNode); }
    void setRespondent(node_id_t aNode) { theRespondent.reset(aNode); }
    void setRacer(node_id_t aNode) { theRacer.reset(aNode); }
    void setFwdRequester(node_id_t aNode) { theFwdRequester.reset(aNode); }
    void setInvAckReceiver(node_id_t aNode) { theInvAckReceiver.reset(aNode); }
    void setStartTime(unsigned long long aTime) { theThreadStartTimestamp = aTime; }
    void setCreationTime(unsigned long long aTime) { theThreadCreationTime = aTime; }
    void setEntryPC(unsigned aPC) { theEntryPointPC = aPC; }
    void setInvAckForFwdExpected(bool aFlag) { theInvAckForFwdExpected = aFlag; }
    void setDowngradeAckExpected(bool aFlag) { theDowngradeAckExpected = aFlag; }
    void setRequestReplyToRacer(bool aFlag) { theRequestReplyToRacer = aFlag; }
    void setFwdReqOrStaleAckExpected(bool aFlag) { theFwdReqOrStaleAckExpected = aFlag; }
    void setWritebackAckExpected(bool aFlag) { theWritebackAckExpected = aFlag; }
    void setInvReceived(unsigned aVal) { theInvalidationReceived.reset(aVal); }
    void setDeferredJump(unsigned aPCDelta) { theDeferredJump.reset(aPCDelta); }
    void setVC(tVC aVC) { theVC = aVC;  }
    void setBlockedOn(int aThreadId) { theBlockedOn = aThreadId;  }
    void setTransactionTracker(boost::intrusive_ptr<TransactionTracker> aTracker){ theTransactionTracker = aTracker; }
    void setPacket(boost::intrusive_ptr<tPacket> aPacket) { thePacket = aPacket; }
    void setCPI(int cpi) { theCPI = cpi; }
    int getCPI() { return theCPI; }
    void resetStallCycles() { theStallCycles = theCPI - 1; }
    int getStallCycles() { return theStallCycles; }
    void decStallCycles() { theStallCycles--; }

    unsigned int uopCount() { return theUopCount; }
    void incrUopCount() { theUopCount++; }
};

class tTsrf {

  std::string theEngineName;

  std::vector<tTsrfEntry> theTSRF;
  std::list<tTsrfEntry *> theFreeList;

  // TSRF entry reserved for writebacks
  tTsrfEntry * theReseveredForWBEntry;
  bool theReseveredForWBEntry_isAvail;

  // TSRF entry reserved for local requests
  tTsrfEntry * theReseveredForLocalEntry;
  bool theReseveredForLocalEntry_isAvail;

 public :
  bool isQuiesced() const {
    return (theFreeList.size() == theTSRF.size() - 2) && theReseveredForWBEntry_isAvail &&  theReseveredForWBEntry_isAvail;
  }

  // constructor
  tTsrf(std::string const & anEngineName, unsigned int aSize);

  enum eEntryRequestType
    { eNormalRequest
    , eRequestWBEntry
    , eRequestLocalEntry
    };

  bool isEntryAvail(eEntryRequestType aType = eNormalRequest);
  tTsrfEntry * allocate(eEntryRequestType aType = eNormalRequest);
  void free(tTsrfEntry * anEntry);

};


}  // namespace nProtocolEngine

#endif // _TSRF_H_
