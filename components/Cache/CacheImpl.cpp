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

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <components/Cache/Cache.hpp>

#include <boost/scoped_ptr.hpp>
#include <core/performance/profile.hpp>
#include <core/simics/configuration_api.hpp>

#include "CacheController.hpp"

  #define DBG_DefineCategories Cache
  #define DBG_SetDefaultOps AddCat(Cache)
  #include DBG_Control()
  #define DBG_DefineCategories MissTracking
  #include DBG_Control()


#define FLEXUS_BEGIN_COMPONENT Cache
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()


namespace nCache {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

using boost::scoped_ptr;


class FLEXUS_COMPONENT(Cache) {
  FLEXUS_COMPONENT_IMPL(Cache);

 private:

  std::auto_ptr<CacheController> theController; //Deleted automagically when the cache goes away

  Stat::StatCounter theBusDeadlocks;

 public:
  FLEXUS_COMPONENT_CONSTRUCTOR(Cache)
    : base ( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theBusDeadlocks( statName() + "-BusDeadlocks" )
    , theBusDirection(kIdle)
    {}

  enum eDirection
    { kIdle
    , kToBackSideIn_Reply
    , kToBackSideIn_Request
    , kToBackSideOut_Request
    , kToBackSideOut_Prefetch
    , kToBackSideOut_Snoop
    };

  unsigned int theBusTxCountdown;
  eDirection theBusDirection;
  MemoryTransport theBusContents;
  boost::optional< MemoryTransport> theBackSideInReplyBuffer;
  boost::optional< MemoryTransport> theBackSideInRequestBuffer;
  std::list<MemoryTransport> theBackSideInReplyInfiniteQueue;
  std::list<MemoryTransport> theBackSideInRequestInfiniteQueue;

  bool isQuiesced() const {
    return theBusDirection == kIdle
      && !theBackSideInReplyBuffer
      && !theBackSideInRequestBuffer
      && (theController.get() ? theController->isQuiesced() : true )
      ;
  }

  void saveState(std::string const & aDirName) {
    theController->saveState( aDirName );
  }

  void loadState(std::string const & aDirName) {
    theController->loadState( aDirName );
  }

  // Initialization
  void initialize() {
    theBusTxCountdown = 0;
    theBusDirection = kIdle;

    // Cache Placement Policy
    tPlacement aPlacement;
    /* CMU-ONLY-BLOCK-BEGIN */
    if (cfg.Placement == "R-NUCA") {
      aPlacement = kRNUCACache;
    }
    else
    /* CMU-ONLY-BLOCK-END */
    if (cfg.Placement == "private") {
      aPlacement = kPrivateCache;
    }
    else if (cfg.Placement == "shared") {
      aPlacement = kSharedCache;
    }
    /* CMU-ONLY-BLOCK-BEGIN */
    else if (cfg.Placement == "ASR") {
      aPlacement = kASRCache;
      DBG_Assert(false, ( << "Use 'private' with asr_enable set to 1") );
    }
    /* CMU-ONLY-BLOCK-END */
    else {
      DBG_Assert(false, ( << "Unknown cache placement policy") );
      aPlacement = kSharedCache; // compiler happy
    }

    theController.reset(new CacheController(statName(),
                                            cfg.Cores,
                                            cfg.Size,
                                            cfg.Associativity,
                                            cfg.BlockSize,
                                            cfg.PageSize,
                                            cfg.Banks,
                                            cfg.Ports,
                                            cfg.TagLatency,
                                            cfg.TagIssueLatency,
                                            1,
                                            1,
                                            cfg.DataLatency,
                                            cfg.DataIssueLatency,
                                            (int)flexusIndex(),
                                            cfg.CacheLevel,
                                            cfg.QueueSizes,
                                            cfg.PreQueueSizes,
                                            cfg.MAFSize,
                                            cfg.MAFTargetsPerRequest,
                                            cfg.EvictBufferSize,
                                            cfg.ProbeFetchMiss,
                                            cfg.EvictClean,
                                            kBaselineCache,
                                            cfg.TraceAddress,
                                            false, /* cfg.IsPiranhaCache */
                                            1, // Number of slices 
                                            0, // Slice number
                                            aPlacement,
                                            false, /* cfg.PrivateWithASR */ /* CMU-ONLY */
                                            false /* cfg.AllowOffChipStreamFetch */
                                            ));

    DBG_Assert( cfg.BusTime_Data > 0);
    DBG_Assert( cfg.BusTime_NoData > 0);
  }

  // Ports
  //======

  //FrontSideIn_Snoop
  //-----------------
  bool available( interface::FrontSideIn_Snoop const &,
                  index_t anIndex )
  {
      return ! theController->FrontSideIn_Snoop[0].full();
  }
  void push( interface::FrontSideIn_Snoop const &,
             index_t           anIndex,
             MemoryTransport & aMessage )
  {
      DBG_Assert(! theController->FrontSideIn_Snoop[0].full());
      aMessage[MemoryMessageTag]->coreIdx() = anIndex;
      DBG_(Trace, Comp(*this) ( << "Received on Port FrontSideIn(Snoop) [" << anIndex << "]: " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
      if (aMessage[TransactionTrackerTag]) {
        aMessage[TransactionTrackerTag]->setDelayCause(name(), "Front Rx");
      }

      theController->FrontSideIn_Snoop[0].enqueue(aMessage);
  }

  //FrontSideIn_Prefetch
  //-------------------
  bool available( interface::FrontSideIn_Prefetch const &,
                  index_t anIndex)
  {
      return ! theController->FrontSideIn_Prefetch[0].full();
  }
  void push( interface::FrontSideIn_Prefetch const &,
             index_t           anIndex,
             MemoryTransport & aMessage)
  {
      DBG_Assert(! theController->FrontSideIn_Prefetch[0].full(), ( << statName() ) );
      aMessage[MemoryMessageTag]->coreIdx() = anIndex;
      DBG_(Trace, Comp(*this) ( << "Received on Port FrontSideIn(Prefetch) [" << anIndex << "]: " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
      if (aMessage[TransactionTrackerTag]) {
        aMessage[TransactionTrackerTag]->setDelayCause(name(), "Front Rx");
      }

      theController->FrontSideIn_Prefetch[0].enqueue(aMessage);
  }

  /* CMU-ONLY-BLOCK-BEGIN */
  //FrontSideIn_Purge
  //-------------------
  bool available( interface::FrontSideIn_Purge const &,
                  index_t anIndex)
  {
      return ! theController->FrontSideIn_Purge[0].full();
  }
  void push( interface::FrontSideIn_Purge const &,
             index_t           anIndex,
             MemoryTransport & aMessage)
  {
      DBG_Assert(! theController->FrontSideIn_Purge[0].full(), ( << statName() ) );
      aMessage[MemoryMessageTag]->coreIdx() = anIndex;
      DBG_(Trace, Comp(*this) ( << "Received on Port FrontSideIn(Purge) [" << anIndex << "]: " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
      if (aMessage[TransactionTrackerTag]) {
        aMessage[TransactionTrackerTag]->setDelayCause(name(), "Front Rx");
      }

      theController->FrontSideIn_Purge[0].enqueue(aMessage);
  }
  /* CMU-ONLY-BLOCK-END */

  //FrontSideIn_Request
  //-------------------
  bool available( interface::FrontSideIn_Request const &,
                  index_t anIndex)
  {
      return ! theController->FrontSideIn_Request[0].full();
  }
  void push( interface::FrontSideIn_Request const &,
             index_t           anIndex,
             MemoryTransport & aMessage)
  {
      DBG_Assert(! theController->FrontSideIn_Request[0].full(), ( << statName() ) );
      aMessage[MemoryMessageTag]->coreIdx() = anIndex;
      DBG_(Trace, Comp(*this) ( << "Received on Port FrontSideIn(Request) [" << anIndex << "]: " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
      if (aMessage[TransactionTrackerTag]) {
        aMessage[TransactionTrackerTag]->setDelayCause(name(), "Front Rx");
      }

      theController->FrontSideIn_Request[0].enqueue(aMessage);
      
      // Notify the PowerTracker that there was a cache access
      bool garbage = true;
      FLEXUS_CHANNEL( RequestSeen) << garbage;
  }

  //BackSideIn_Reply
  //-----------
  bool available( interface::BackSideIn_Reply const &) {
    return ! theBackSideInReplyBuffer;
  }
  void push( interface::BackSideIn_Reply const &, MemoryTransport & aMessage) {
    DBG_Assert(!aMessage[MemoryMessageTag]->isRequest(), Comp(*this) (<< " message: " << *aMessage[MemoryMessageTag]));
    if (cfg.NoBus) {
      aMessage[MemoryMessageTag]->coreIdx() = 0;
      theBackSideInReplyInfiniteQueue.push_back(aMessage);
      return;
    }
    DBG_Assert(! theBackSideInReplyBuffer);
    // In non-piranha caches, the core index should always be zero, since
    // things above (such as the processor) do not want to know about core numbers
    aMessage[MemoryMessageTag]->coreIdx() = 0;
    theBackSideInReplyBuffer = aMessage;
  }

  //BackSideIn_Request
  //-----------
  bool available( interface::BackSideIn_Request const &) {
    return ! theBackSideInReplyBuffer;
  }
  void push( interface::BackSideIn_Request const &, MemoryTransport & aMessage) {
    DBG_Assert(aMessage[MemoryMessageTag]->isRequest(), Comp(*this) (<< " message: " << *aMessage[MemoryMessageTag]));
    if (cfg.NoBus) {
      aMessage[MemoryMessageTag]->coreIdx() = 0;
      theBackSideInReplyInfiniteQueue.push_back(aMessage);
      return;
    }
    DBG_Assert(! theBackSideInReplyBuffer);
    // In non-piranha caches, the core index should always be zero, since
    // things above (such as the processor) do not want to know about core numbers
    aMessage[MemoryMessageTag]->coreIdx() = 0;
    theBackSideInReplyBuffer = aMessage;
    
    // Notify the PowerTracker that there was a cache access
    bool garbage = true;
    FLEXUS_CHANNEL( RequestSeen) << garbage;
 }

  //CacheDrive
  //----------
  void drive(interface::CacheDrive const &) {
      DBG_(VVerb, Comp(*this) (<< "CacheDrive" ) ) ;
      theController->processMessages();
      busCycle();
  }

  unsigned int transferTime(const MemoryTransport & trans) {
    if (theFlexus->isFastMode()) {
      return 0;
    }
    return ( (trans[MemoryMessageTag]->reqSize() > 0) ? cfg.BusTime_Data: cfg.BusTime_NoData) - 1;
  }

  void busCycle() {
    FLEXUS_PROFILE();
    DBG_(VVerb, Comp(*this) ( << "bus cycle" ) );

    for ( int i = 0; i < cfg.Cores; i++ ) {
      //Send as much on FrontSideOut as possible
      while(! theController->FrontSideOut[i].empty() 
            && FLEXUS_CHANNEL_ARRAY(FrontSideOut_Reply, i).available()
            && FLEXUS_CHANNEL_ARRAY(FrontSideOut_Reply, i).available())
      {
        MemoryTransport transport = theController->FrontSideOut[i].dequeue();
        if (transport[MemoryMessageTag]->isRequest()) {
          DBG_(Trace, Comp(*this) (<< "Sent on Port FrontSideOut_Reply [" << i << "]: " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
          FLEXUS_CHANNEL_ARRAY(FrontSideOut_Reply, i) << transport;
        } else {
          DBG_(Trace, Comp(*this) (<< "Sent on Port FrontSideOut_Reply [" << i << "]: " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
          FLEXUS_CHANNEL_ARRAY(FrontSideOut_Reply, i) << transport;
        }
      }
    }

    if (cfg.NoBus) {
      while( !theController->BackSideIn_Reply[0].full() && !theBackSideInReplyInfiniteQueue.empty() ) {
        theController->BackSideIn_Reply[0].enqueue( theBackSideInReplyInfiniteQueue.front() );
        theBackSideInReplyInfiniteQueue.pop_front();
      }
      while( !theController->BackSideIn_Request[0].full() && !theBackSideInRequestInfiniteQueue.empty() ) {
        theController->BackSideIn_Request[0].enqueue( theBackSideInRequestInfiniteQueue.front() );
        theBackSideInRequestInfiniteQueue.pop_front();
      }
      while( !theController->BackSideOut_Request.empty() && FLEXUS_CHANNEL(BackSideOut_Request).available()
             && (theController->BackSideOut_Snoop.empty()
                 || theController->BackSideOut_Snoop.headTimestamp() > theController->BackSideOut_Request.headTimestamp()) ) {
        MemoryTransport transport = theController->BackSideOut_Request.dequeue();
        FLEXUS_CHANNEL(BackSideOut_Request) << transport;
      }
      while( !theController->BackSideOut_Prefetch.empty() && FLEXUS_CHANNEL(BackSideOut_Prefetch).available()
             && (theController->BackSideOut_Snoop.empty()
                 || theController->BackSideOut_Snoop.headTimestamp() > theController->BackSideOut_Prefetch.headTimestamp()) ) {
        MemoryTransport transport = theController->BackSideOut_Prefetch.dequeue();
        FLEXUS_CHANNEL(BackSideOut_Prefetch) << transport;
      }
      while( !theController->BackSideOut_Snoop.empty() && FLEXUS_CHANNEL(BackSideOut_Snoop).available() ) {
        MemoryTransport transport = theController->BackSideOut_Snoop.dequeue();
        FLEXUS_CHANNEL(BackSideOut_Snoop) << transport;
      }
      return;
    }

    //If we want fast clean evicts, propagate as many CleanEvicts as possible
    //regardless of bus state.  We use this hack with SimplePrefetchController
    //because it uses clean evict messages to maintain duplicate tags, but the
    //CleanEvicts would not be propagated over the bus in the real system
    if (cfg.FastEvictClean) {
      if (theBusDirection == kIdle) {
        if ( !theController->BackSideOut_Snoop.empty() && FLEXUS_CHANNEL(BackSideOut_Snoop).available()) {
          MemoryMessage::MemoryMessageType type = theController->BackSideOut_Snoop.peek()[MemoryMessageTag]->type();
          if (type == MemoryMessage::EvictClean || type == MemoryMessage::EvictWritable ) {
            MemoryTransport transport = theController->BackSideOut_Snoop.dequeue();
            DBG_(Trace, Comp(*this) (<< "Fast Transmit evict on port BackSideOut(Snoop): " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
            FLEXUS_CHANNEL(BackSideOut_Snoop) << transport;
          }
        }
      }
    }



    //If a bus transfer is in process, continue it
    if (theBusTxCountdown > 0) {
      DBG_Assert( theBusDirection != kIdle );
      --theBusTxCountdown;
    } else if (theBusDirection == kIdle) {
      //The bus is available, initiate a transfer, if there are any to start


      //Back side reply buffer gets max priority if it can transfer
      if ( theBackSideInReplyBuffer
           && !theController->BackSideIn_Reply[0].full()
         )
      {
        theBusContents = *theBackSideInReplyBuffer;
        theBackSideInReplyBuffer = boost::none;
        theBusDirection = kToBackSideIn_Reply;
        theBusTxCountdown = transferTime( theBusContents );
      }
      //then check the back side request buffer 
      else if ( theBackSideInRequestBuffer
                && !theController->BackSideIn_Request[0].full()
              )
      {
        theBusContents = *theBackSideInRequestBuffer;
        theBackSideInRequestBuffer = boost::none;
        theBusDirection = kToBackSideIn_Request;
        theBusTxCountdown = transferTime( theBusContents );

      }
      //Prefer a request message if it is older than any snoop messages
      else if ( !theController->BackSideOut_Request.empty()
                && FLEXUS_CHANNEL(BackSideOut_Request).available()
                && (theController->BackSideOut_Snoop.empty() || theController->BackSideOut_Snoop.headTimestamp() > theController->BackSideOut_Request.headTimestamp() )
              )
      {
        theBusDirection = kToBackSideOut_Request;
        theBusTxCountdown = transferTime( theController->BackSideOut_Request.peek() );
      }
      //Prefer a prefetch message if it is older than any snoop messages and there are no requests
      else if ( theController->BackSideOut_Request.empty()
                && !theController->BackSideOut_Prefetch.empty()
                && FLEXUS_CHANNEL(BackSideOut_Prefetch).available()
                && (theController->BackSideOut_Snoop.empty() || theController->BackSideOut_Snoop.headTimestamp() > theController->BackSideOut_Prefetch.headTimestamp() )
              )
      {
        theBusDirection = kToBackSideOut_Prefetch;
        theBusTxCountdown = transferTime( theController->BackSideOut_Prefetch.peek() );
      }
      //Take any snoop message
      else if ( !theController->BackSideOut_Snoop.empty()
                && FLEXUS_CHANNEL(BackSideOut_Snoop).available()
              )
      {
        theBusDirection = kToBackSideOut_Snoop;
        theBusTxCountdown = transferTime( theController->BackSideOut_Snoop.peek() );
      }
    }

    if ( theBusTxCountdown == 0 ) {
      switch (theBusDirection) {
        case kIdle:
          break; //Nothing to do
        case kToBackSideIn_Reply:
          if ( ! theController->BackSideIn_Reply[0].full()) {
            DBG_(Trace, Comp(*this) (<< "Received on Port BackSideInReply: " << *(theBusContents[MemoryMessageTag]) ) Addr(theBusContents[MemoryMessageTag]->address()) );
            theController->BackSideIn_Reply[0].enqueue( theBusContents );
            theBusDirection = kIdle;
          }
          break;
        case kToBackSideIn_Request:
          if ( ! theController->BackSideIn_Request[0].full()) {
            DBG_(Trace, Comp(*this) (<< "Received on Port BackSideInRequest: " << *(theBusContents[MemoryMessageTag]) ) Addr(theBusContents[MemoryMessageTag]->address()) );
            theController->BackSideIn_Request[0].enqueue( theBusContents );
            theBusDirection = kIdle;
          }
          break;
        case kToBackSideOut_Snoop:
          if ( FLEXUS_CHANNEL(BackSideOut_Snoop).available() ) {
            DBG_Assert( !theController->BackSideOut_Snoop.empty() );
            theBusContents = theController->BackSideOut_Snoop.dequeue();
            DBG_(Trace, Comp(*this) (<< "Sent on Port BackSideOut(Snoop): " << *(theBusContents[MemoryMessageTag]) ) Addr(theBusContents[MemoryMessageTag]->address()) );
            FLEXUS_CHANNEL(BackSideOut_Snoop) << theBusContents;
            theBusDirection = kIdle;
          } else {
            //Bus is deadlocked - need to retry
            ++theBusDeadlocks;
            theBusDirection = kIdle;
          }
          break;
        case kToBackSideOut_Request:
          if ( FLEXUS_CHANNEL(BackSideOut_Request).available() ) {
            DBG_Assert( !theController->BackSideOut_Request.empty() );
            theBusContents = theController->BackSideOut_Request.dequeue();
            DBG_(Trace, Comp(*this) (<< "Sent on Port BackSideOut(Request): " << *(theBusContents[MemoryMessageTag]) ) Addr(theBusContents[MemoryMessageTag]->address()) );
            FLEXUS_CHANNEL(BackSideOut_Request) << theBusContents;
            theBusDirection = kIdle;
          } else {
            //Bus is deadlocked - need to retry
            ++theBusDeadlocks;
            theBusDirection = kIdle;
          }
          break;
        case kToBackSideOut_Prefetch:
          if ( FLEXUS_CHANNEL(BackSideOut_Prefetch).available() ) {
            DBG_Assert( !theController->BackSideOut_Prefetch.empty() );
            theBusContents = theController->BackSideOut_Prefetch.dequeue();
            DBG_(Trace, Comp(*this) (<< "Sent on Port BackSideOut(Prefetch): " << *(theBusContents[MemoryMessageTag]) ) Addr(theBusContents[MemoryMessageTag]->address()) );
            FLEXUS_CHANNEL(BackSideOut_Prefetch) << theBusContents;
            theBusDirection = kIdle;
          } else {
            //Bus is deadlocked - need to retry
            ++theBusDeadlocks;
            theBusDirection = kIdle;
          }
          break;
      }
    }

  }

};

} //End Namespace nCache

FLEXUS_COMPONENT_INSTANTIATOR( Cache, nCache );

FLEXUS_PORT_ARRAY_WIDTH( Cache, FrontSideOut_Reply )     { return (cfg.Cores); }
FLEXUS_PORT_ARRAY_WIDTH( Cache, FrontSideOut_Request )   { return (cfg.Cores); }

FLEXUS_PORT_ARRAY_WIDTH( Cache, FrontSideIn_Snoop )      { return (cfg.Cores); }
FLEXUS_PORT_ARRAY_WIDTH( Cache, FrontSideIn_Request )    { return (cfg.Cores); }
FLEXUS_PORT_ARRAY_WIDTH( Cache, FrontSideIn_Prefetch )   { return (cfg.Cores); }
FLEXUS_PORT_ARRAY_WIDTH( Cache, FrontSideIn_Purge )      { return (cfg.Cores); } /* CMU-ONLY */


#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT Cache

  #define DBG_Reset
  #include DBG_Control()
