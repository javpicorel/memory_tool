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

#include <components/TMSController/TMSController.hpp>

#define FLEXUS_BEGIN_COMPONENT TMSController
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <components/TMSController/TMSPolicies.hpp>
#include <components/Common/MessageQueues.hpp>


#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>

#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/random.hpp>

#include <core/performance/profile.hpp>

  #define DBG_DefineCategories TMSController, TMSTrace
  #define DBG_SetDefaultOps AddCat(TMSController)
  #include DBG_Control()

namespace nTMSController {

using namespace boost::multi_index;
using namespace nMessageQueues;


using namespace Flexus::Core;
namespace Stat = Flexus::Stat;
using namespace Flexus::SharedTypes;

using boost::intrusive_ptr;


struct by_order {};
struct by_address {};
struct by_queue {};
struct by_replacement {};
struct by_source {};
struct by_watch {};

struct PrefetchEntry {
  MemoryAddress theAddress;
  boost::intrusive_ptr<StreamQueueBase> theQueue;
  long long theInsertTime;

  PrefetchEntry( MemoryAddress anAddress, boost::intrusive_ptr<StreamQueueBase> aQueue)
    : theAddress(anAddress)
    , theQueue(aQueue)
    , theInsertTime( theFlexus->cycleCount() )
  { }
};


typedef multi_index_container
  < PrefetchEntry
  , indexed_by
    < ordered_unique
        < tag< by_address >
        , member< PrefetchEntry, MemoryAddress, &PrefetchEntry::theAddress >
        >
    >
  >
  PrefetchBuffer;

boost::mt19937 theRNG(0xdeadbeef);                 
boost::uniform_01< boost::mt19937 > getRandom(theRNG);   


class FLEXUS_COMPONENT(TMSController), public Controller {
  FLEXUS_COMPONENT_IMPL(TMSController);

  MessageQueue< PrefetchTransport > theBufferMessagesIn;
  MessageQueue< PrefetchTransport > theBufferMessagesOut;

  std::list<PredictorTransport> theCoreMessagesIn;

  std::list<IndexMessage> theTIndexMessagesIn;
  std::list<IndexMessage> theTIndexMessagesOut;
  std::list<IndexMessage> thePIndexMessagesIn;
  std::list<IndexMessage> thePIndexMessagesOut;

  PrefetchBuffer theBufferContents;
  PrefetchBuffer thePendingPrefetches;
  int thePendingCMOBRequests;

  std::list<MemoryAddress> theOffChipReads;

  TMSStats theStats;

  boost::scoped_ptr< PrefetchPolicy > thePrefetchPolicy;

 public:
   FLEXUS_COMPONENT_CONSTRUCTOR(TMSController)
     : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
     , thePendingCMOBRequests(0)
     , theStats(statName())
  { }

  bool isQuiesced() const {
    return  theBufferMessagesIn.empty()
      &&    theBufferMessagesOut.empty()
      &&    thePendingPrefetches.empty()
      ;
  }

  void saveState(std::string const & aDirName) { }

  void loadState(std::string const & aDirName) {  }


  // Initialization
  void initialize() {
    if (cfg.OnChipTMS) {
      thePrefetchPolicy.reset( PrefetchPolicy::createOnChipTMSPolicy(flexusIndex(), theStats, *this ));      
    } else {
      thePrefetchPolicy.reset( PrefetchPolicy::createTMSPolicy(flexusIndex(), theStats, *this ));
    }
    theBufferMessagesIn.setSize( cfg.QueueSize  );
  }

  // Ports
  bool available(interface::FromSVB const &) {
      return ! theBufferMessagesIn.full();
  }
  void push( interface::FromSVB const&, PrefetchTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received FromSVB: " << *(aMessage[PrefetchMessageTag]) ) Addr(aMessage[PrefetchMessageTag]->address()) );
    if (cfg.EnableTMS) {
      enqueue(aMessage);
    }
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(CoreNotify);
  void push( interface::CoreNotify const&, PredictorTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received FromCore: " << *(aMessage[PredictorMessageTag]) ) Addr(aMessage[PredictorMessageTag]->address()) );
    if (cfg.EnableTMS) {
      enqueue(aMessage);
    }
    FLEXUS_CHANNEL(CoreNotifyOut) << aMessage;
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(RecentRequests);
  void push( interface::RecentRequests const&, MemoryAddress & anAddress) {
    thePrefetchPolicy->recentPBRequest( anAddress );
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(MonitorMemRequests);
  void push( interface::MonitorMemRequests const&, MemoryTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received MonitorMemRequests: " << *( aMessage[MemoryMessageTag]) ) );
    if (cfg.EnableTMS) {
      if (aMessage[MemoryMessageTag]->fillLevel() == eUnknown && aMessage[TransactionTrackerTag] && aMessage[TransactionTrackerTag]->fillLevel()) {
         aMessage[MemoryMessageTag]->fillLevel() = *aMessage[TransactionTrackerTag]->fillLevel();
      }
      thePrefetchPolicy->observe(*aMessage[MemoryMessageTag]);
    }
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(FromTIndex);
  void push(interface::FromTIndex const &, IndexMessage & aMessage) {
    DBG_(Iface, Comp(*this) ( << aMessage) );
    enqueueT(aMessage);    
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(FromPIndex);
  void push(interface::FromPIndex const &, IndexMessage & aMessage) {
    DBG_(Iface, Comp(*this) ( << aMessage) );
    enqueueP(aMessage);    
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(CMOBReply);
  void push(interface::CMOBReply const &, cmob_reply_t & aMessage) {
    --thePendingCMOBRequests;
    thePrefetchPolicy->handleCMOBReply(aMessage.get<0>(), aMessage.get<1>(), aMessage.get<2>(), aMessage.get<3>(), aMessage.get<4>());    
  }



  // Drive Interfaces
  void drive(interface::TMSControllerDrive const &) {
    DBG_(VVerb, Comp(*this) (<< "TMSControllerDrive" ) ) ;
    if (cfg.EnableTMS) {
       process();
    }
  }

  private:

  void enqueue(PrefetchTransport & aMessage) {
    DBG_Assert( ! theBufferMessagesIn.full() );
    theBufferMessagesIn.enqueue(aMessage);
  }

  void enqueue(PredictorTransport & aMessage) {
    theCoreMessagesIn.push_back(aMessage);
  }

  void enqueueT(IndexMessage & aMessage) {
    theTIndexMessagesIn.push_back(aMessage);
  }
  void enqueueP(IndexMessage & aMessage) {
    thePIndexMessagesIn.push_back(aMessage);
  }

  void process() {
    FLEXUS_PROFILE();
    //First, process messages arriving from the PrefetchBuffer indicating
    //actions it took last cycle
    processPrefetchBufferReplies();

    processCoreMessages();

    processPIndexMessages();
    processTIndexMessages();
   
    //Figure out if we should prefetch anything this cycle
    thePrefetchPolicy->process();
   
    //Send outgoing messages to the prefetch buffer and network
    sendMessages();

  }

  void sendMessages() {
    FLEXUS_PROFILE();

    //Send to PrefetchBuffer
    while ( FLEXUS_CHANNEL(ToSVB).available() && !theBufferMessagesOut.empty()) {
      PrefetchTransport trans(theBufferMessagesOut.dequeue());
      DBG_(Iface, Comp(*this) ( << "Sending ToSVB: "  << *(trans[PrefetchMessageTag]) ));
      FLEXUS_CHANNEL(ToSVB) << trans;
    }

    while ( FLEXUS_CHANNEL(ToTIndex).available() && !theTIndexMessagesOut.empty()) {
      DBG_(Iface, Comp(*this) ( << "Sending ToTIndex: "  << (theTIndexMessagesOut.front()) ));
      FLEXUS_CHANNEL(ToTIndex) << theTIndexMessagesOut.front();
      theTIndexMessagesOut.pop_front();
    }

    while ( FLEXUS_CHANNEL(ToPIndex).available() && !thePIndexMessagesOut.empty()) {
      DBG_(Iface, Comp(*this) ( << "Sending ToPIndex: "  << (thePIndexMessagesOut.front()) ));
      FLEXUS_CHANNEL(ToPIndex) << thePIndexMessagesOut.front();
      thePIndexMessagesOut.pop_front();
    }
  }

  void processPrefetchBufferReplies() {
    FLEXUS_PROFILE();
    while ( ! theBufferMessagesIn.empty()) {
      PrefetchTransport trans(theBufferMessagesIn.dequeue());      
      boost::intrusive_ptr<PrefetchMessage> msg = trans[PrefetchMessageTag];
      DBG_Assert(msg);
      DBG_(VVerb, Comp(*this) ( << "Processing PrefetchBuffer reply: "  << *msg) Addr(msg->address() ) );
      switch (msg->type()) {
         case PrefetchMessage::PrefetchComplete:
           //Move the prefetch from theInProcessPrefetches to the theBufferContents
           completePrefetch(msg->address());
           break;
         case PrefetchMessage::PrefetchRedundant:
           //Discard the prefetch from theInProcessPrefetches
           cancelPrefetch(msg->address());
           break;

         case PrefetchMessage::LineHit:
           //Remove the address from theBufferContents and send a hit notification
           recordHit(msg->address(), trans[TransactionTrackerTag], false);
           break;

         case PrefetchMessage::LineHit_Partial:
           //Remove the address from theBufferContents and send a hit notification
           recordHit(msg->address(), trans[TransactionTrackerTag], true);
           break;

         case PrefetchMessage::LineReplaced:
           //Remove the address from theBufferContents
           removeLine(msg->address(), true);
           break;

         case PrefetchMessage::LineRemoved:
           //Remove the address from theBufferContents
           removeLine(msg->address(), false);
           break;

         case PrefetchMessage::WatchPresent:
         case PrefetchMessage::WatchRequested:
           watchRequested(msg->address());
           break;

         case PrefetchMessage::WatchRedundant:
         case PrefetchMessage::WatchRemoved:
         case PrefetchMessage::WatchReplaced:
           watchKill(msg->address(), msg->type());
           break;

         default:
           DBG_Assert(false, ( << "PrefetchListener received illegal PrefetchMessage: " << *msg ) );
      }
    }
  }

  void processCoreMessages() {
    FLEXUS_PROFILE();
    while ( ! theCoreMessagesIn.empty()) {
       boost::intrusive_ptr<PredictorMessage> msg = theCoreMessagesIn.front()[PredictorMessageTag];
       tFillLevel fill_level = eUnknown;
       if (theCoreMessagesIn.front()[TransactionTrackerTag]->fillLevel()) {
          fill_level = *theCoreMessagesIn.front()[TransactionTrackerTag]->fillLevel();
       }
       DBG_Assert(msg);
       DBG_(VVerb, Comp(*this) ( << "Processing Core message: "  << *msg) Addr(msg->address() ) );
       switch (msg->type()) {
          case PredictorMessage::PredictorMessage::eReadPredicted:
            thePrefetchPolicy->notifyRead( msg->address(), fill_level, true);
            break;
          
          case PredictorMessage::PredictorMessage::eReadNonPredicted:
            thePrefetchPolicy->notifyRead( msg->address(), fill_level, false);
            break;

          default:
            DBG_Assert(false, ( << "received illegal PredictorMessage: " << *msg ) );
       }
       theCoreMessagesIn.pop_front();
    }
  }

  void processPIndexMessages() {
    FLEXUS_PROFILE();

    while ( ! thePIndexMessagesIn.empty()) {
      IndexMessage msg(thePIndexMessagesIn.front());
      thePIndexMessagesIn.pop_front();
      unsigned long long delay = theFlexus->cycleCount() - msg.theStartTime;
      switch( msg.theCommand ) {
        case IndexCommand::eMatchReply: {
          ++theStats.statPersistentIndexMatches;
          theStats.statIndexLookupTime << delay;
          //Confirm that the address is not Buffered or in flight
          thePrefetchPolicy->lookupMatch(msg.theAddress, msg.theCMOB, msg.theCMOBOffset, msg.theStartTime);
          break;
        }
        case IndexCommand::eNoMatchReply:
          ++theStats.statPersistentIndexNoMatches;
          ++theStats.statTrainingIndexLookups;
          //Do lookup in training index
          requestMapping( kTrainingIndex, msg.theAddress, msg.theStartTime);          
          break;
        case IndexCommand::eNoUpdateReply:          
          ++theStats.statTrainingIndexInserts;
          recordMapping( kTrainingIndex, msg.theAddress, msg.theCMOB, msg.theCMOBOffset);      
          break;          
        default:
          DBG_Assert(false); 
      }
    }
  }

  void processTIndexMessages() {
    FLEXUS_PROFILE();

    while ( ! theTIndexMessagesIn.empty()) {
      IndexMessage msg(theTIndexMessagesIn.front());
      theTIndexMessagesIn.pop_front();
      unsigned long long delay = theFlexus->cycleCount() - msg.theStartTime;
      theStats.statIndexLookupTime << delay;
      switch( msg.theCommand ) {
        case IndexCommand::eMatchReply: {
          ++theStats.statTrainingIndexMatches;
          thePrefetchPolicy->lookupMatch(msg.theAddress, msg.theCMOB, msg.theCMOBOffset, msg.theStartTime);
          break;
        }
        case IndexCommand::eNoMatchReply:
          ++theStats.statTrainingIndexNoMatches;
          thePrefetchPolicy->lookupNoMatch();
          break;
        default:
          DBG_Assert(false); 
      }
    }
  }

  void completePrefetch(MemoryAddress const & anAddress) {
    FLEXUS_PROFILE();
    DBG_(VVerb, Comp(*this) ( << "Completing prefetch to : "  << & std::hex << anAddress << & std::dec) Addr(anAddress) );
    PrefetchBuffer::iterator iter = thePendingPrefetches.find(anAddress);
    DBG_Assert( iter != thePendingPrefetches.end(), Comp(*this) ( << "Completed a prefetch for @" << anAddress << " but no prefetch was InProcess"));

    DBG_(Verb, ( << statName() << " Prefetched " << anAddress << " (orphaned) " ) );
    ++theStats.statCompletedPrefetches;
    if (iter->theInsertTime != 0) {
      theStats.statPrefetchLatency << Flexus::Core::theFlexus->cycleCount() - iter->theInsertTime;
      theStats.statPrefetchLatencySum += Flexus::Core::theFlexus->cycleCount() - iter->theInsertTime;
    }

    thePrefetchPolicy->completePrefetch(anAddress, iter->theQueue );

    //Move the block to theBuffer from thePending
    theBufferContents.insert( *iter);
    thePendingPrefetches.erase(iter);
    
  }

  void cancelPrefetch(MemoryAddress const & anAddress) {
    FLEXUS_PROFILE();
    DBG_(VVerb, Comp(*this) ( << "Cancelling prefetch to : "  << & std::hex << anAddress << & std::dec) Addr(anAddress) );

    PrefetchBuffer::iterator iter = thePendingPrefetches.find(anAddress);
    DBG_Assert( iter != thePendingPrefetches.end(), Comp(*this) ( << "Attempted to cancel a prefetch for @" << anAddress << " but no prefetch was Pending"));

    DBG_(Verb, ( << statName() << " Rejected " << anAddress << " (orphaned)" ) );
    ++theStats.statRejectedPrefetches;

    thePrefetchPolicy->cancelPrefetch(anAddress, iter->theQueue );

    thePendingPrefetches.erase(iter);
  }

  void removeLine(MemoryAddress const & anAddress, bool isReplacement) {
    FLEXUS_PROFILE();
    DBG_(VVerb, Comp(*this) ( << "Removing line from theBufferContents: "  << anAddress ) Addr(anAddress) );

    PrefetchBuffer::iterator iter = theBufferContents.find(anAddress);
    DBG_Assert( iter != theBufferContents.end(), Comp(*this) ( << "Attempted to drop a prefetch line for @" << & std::hex << anAddress << & std::dec << " but address was not in theBufferContents"));

    DBG_(Verb, ( << statName() << " Remove " << anAddress << " (orphaned) after " << Flexus::Core::theFlexus->cycleCount() - iter->theInsertTime << " Replacement: " << isReplacement  ) );
    ++theStats.statDiscardedLines;

    if (isReplacement) {
      ++theStats.statReplacedLines;
    } else {
      ++theStats.statInvalidatedLines;
    }

    if (iter->theInsertTime != 0) {
      theStats.statLifetimeToDiscard << Flexus::Core::theFlexus->cycleCount() - iter->theInsertTime;
    }

    thePrefetchPolicy->removeLine(anAddress, isReplacement, iter->theQueue );

    theBufferContents.erase(iter);
  }

  void recordHit(MemoryAddress const & anAddress, boost::intrusive_ptr<TransactionTracker> aTracker, bool wasPartial) {
    FLEXUS_PROFILE();
    DBG_(VVerb, Comp(*this) ( << "Processing hit on address: "  << anAddress ) Addr(anAddress) );

    PrefetchBuffer::iterator iter = theBufferContents.find(anAddress);
    DBG_Assert( iter != theBufferContents.end(), Comp(*this) ( << "Attempted to process hit for @" << anAddress << " but address was not in theBufferContents"));

#ifdef FLEXUS_TRACK_PBHITS
    if (aTracker->OS() && *aTracker->OS()) {
      thePBHits_System << std::make_pair (anAddress & (~ 63LL), 1LL);
    } else {
      thePBHits_User << std::make_pair (anAddress & (~ 63LL), 1LL);
    }
#endif //FLEXUS_TRACK_PBHITS

    DBG_(Verb, ( << statName() << " HIT " << anAddress << " (orphaned) after " << Flexus::Core::theFlexus->cycleCount() - iter->theInsertTime  << " cycles" ) );
    ++theStats.statHits;
    if (wasPartial) {
      ++theStats.statHits_Partial;      
    }
    if (aTracker->OS() && *aTracker->OS()) {
      ++theStats.statHits_System;
    } else {
      ++theStats.statHits_User;
    }
    
    if (iter->theInsertTime != 0) {
      theStats.statLifetimeToHit << Flexus::Core::theFlexus->cycleCount() - iter->theInsertTime;
    }

    thePrefetchPolicy->hit(anAddress, iter->theQueue, wasPartial );

    //Remove the Hit line from the buffer
    theBufferContents.erase(iter);
  }


  void watchRequested( MemoryAddress anAddress ) {
    FLEXUS_PROFILE();
    //We hit on a watch.  Let's walk through the queues and make any of them
    //that match hot
    DBG_(Verb, ( << statName() << " Watch-Present-or-Requested " << anAddress ) );
    
    thePrefetchPolicy->watchRequested(anAddress);
  }

  void watchKill( MemoryAddress anAddress, PrefetchMessage::PrefetchMessageType aReason ) {
    FLEXUS_PROFILE();

    switch( aReason) {
      case PrefetchMessage::WatchRedundant:
        DBG_(Verb, ( << statName() << " Watch-Redundant " << anAddress ) );
        ++theStats.statWatchRedundant;
        break;
      case PrefetchMessage::WatchRemoved:
        DBG_(Verb, ( << statName() << " Watch-Removed" << anAddress ) );
        ++theStats.statWatchRemoved;
        break;
      case PrefetchMessage::WatchReplaced:
        DBG_(Verb, ( << statName() << " Watch-Replaced" << anAddress  ) );
        ++theStats.statWatchReplaced;
        break;
      default:
        DBG_Assert(false);
        break;
    };    

    thePrefetchPolicy->watchKill(anAddress, aReason);
  }  




  bool isFull_BufferOut() const { return theBufferMessagesOut.full(); }

  bool mayPrefetch() const {
    return ( ! isFull_BufferOut() &&  thePendingPrefetches.size() < cfg.MaxPrefetches );
  }

  bool mayRequestAddresses() const {
    return thePendingCMOBRequests < 8;
  }

  unsigned int maxStreamQueues() const {
    return cfg.StreamQueues;
  }

  unsigned int minBufferCap() const {
    return cfg.MinBufferCap;    
  }

  unsigned int initBufferCap() const {
    return cfg.InitBufferCap;    
  }

  unsigned int maxBufferCap() const {
    return cfg.MaxBufferCap;    
  }

  bool usePIndex() const {
    return cfg.UsePIndex;
  }
  
  unsigned int fetchCMOBAt() const {
    return cfg.FetchCMOBAt;    
  }

  void prefetch( MemoryAddress anAddress, boost::intrusive_ptr<StreamQueueBase> aQ) {
    FLEXUS_PROFILE();
    if (thePendingPrefetches.count(anAddress) + theBufferContents.count(anAddress) > 0) {
      ++theStats.statRejectedPrefetches;
      thePrefetchPolicy->cancelPrefetch(anAddress, aQ );      
    } else {    
      //Queue a new prefetch command
      boost::intrusive_ptr<PrefetchMessage> prefetch (new PrefetchMessage(PrefetchMessage::PrefetchReq, anAddress));
      PrefetchTransport transport;
      transport.set(PrefetchMessageTag, prefetch);
      theBufferMessagesOut.enqueue(transport);
      
      //Record that the prefetch is pending
      thePendingPrefetches.insert( PrefetchEntry( anAddress, aQ) );
    }
  }


  long getNextCMOBIndex( ) {
    long index = 0;
    DBG_Assert(FLEXUS_CHANNEL(NextCMOBIndex).available());
    FLEXUS_CHANNEL(NextCMOBIndex) >> index;
    return index;
  }  
  void appendToCMOB( MemoryAddress anAddress, bool wasHit ) {
    std::pair< MemoryAddress, bool> append( anAddress, wasHit);
    DBG_Assert(FLEXUS_CHANNEL(CMOBAppend).available());
    FLEXUS_CHANNEL(CMOBAppend) << append;    
  }  

  void requestCMOBRange( int aCMOB, int aStartOffset, long anId ) {
    ++thePendingCMOBRequests;
    cmob_request_t request( aCMOB, aStartOffset, anId);
    FLEXUS_CHANNEL(CMOBRequest) << request;    
  }

  bool addressPending( MemoryAddress anAddress ) const {
    return (thePendingPrefetches.count(anAddress) + theBufferContents.count(anAddress) > 0);
  }


  void recordMapping( int anIndex, MemoryAddress anAddress, int aCMOB, long aLocation ) {
    DBG_(Verb, Comp(*this) ( << "indexWrite " << anIndex << " " << anAddress << " -> " << flexusIndex() << ":" << aLocation) );


    IndexMessage msg;
    msg.theAddress = anAddress;
    msg.theTMSc = flexusIndex();
    msg.theCMOB = aCMOB;
    msg.theCMOBOffset = aLocation;
    if (anIndex == kTrainingIndex ) {

      if (cfg.TIndexInsertProb < 1.0 ) {
        if (getRandom() > cfg.TIndexInsertProb) {
          ++theStats.statTrainingIndexRandomlyDroppedInserts;
          return;    
        }
      } 

      msg.theCommand = IndexCommand::eInsert;      
      theTIndexMessagesOut.push_back(msg);
    } else if (anIndex == kPersistentIndex_Update ) {
      msg.theCommand = IndexCommand::eUpdate;      
      thePIndexMessagesOut.push_back(msg);
    } else if (anIndex == kPersistentIndex_Insert ) {
      msg.theCommand = IndexCommand::eInsert;      
      thePIndexMessagesOut.push_back(msg);      
    }
  }

  void requestMapping( int anIndex, MemoryAddress anAddress, unsigned long long aStartTime) {
    DBG_(Iface, Comp(*this) ( << "indexRequest " << anIndex << " for " << anAddress ) );

    IndexMessage msg;
    msg.theCommand = IndexCommand::eLookup;
    msg.theAddress = anAddress;
    msg.theTMSc = flexusIndex();
    msg.theCMOB = -1;
    msg.theCMOBOffset = -1;
    if (aStartTime == 0) {
      msg.theStartTime = Flexus::Core::theFlexus->cycleCount();
    } else {
      msg.theStartTime = aStartTime;      
    }
    if (anIndex == kTrainingIndex  ) {
      theTIndexMessagesOut.push_back(msg);
    } else {
      thePIndexMessagesOut.push_back(msg);      
    }
  }

  

};


} //End Namespace nTMSController

FLEXUS_COMPONENT_INSTANTIATOR( TMSController, nTMSController);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT TMSController

  #define DBG_Reset
  #include DBG_Control()

