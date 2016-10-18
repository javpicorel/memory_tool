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

#ifndef FLEXUS_TMSCONTROLLER_TMSPOLICIES_HPP_INCLUDED
#define FLEXUS_TMSCONTROLLER_TMSPOLICIES_HPP_INCLUDED

#include <list>
#include <core/stats.hpp>

#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/Slices/PrefetchMessage.hpp>

namespace nTMSController {

using namespace Flexus::Core;
namespace Stat = Flexus::Stat;
using namespace Flexus::SharedTypes;

typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

struct TMSStats {

  Stat::StatCounter statCompletedPrefetches;
  Stat::StatCounter statRejectedPrefetches;
  Stat::StatCounter statDiscardedLines;
  Stat::StatCounter statReplacedLines;
  Stat::StatCounter statInvalidatedLines;
  Stat::StatCounter statHits;
  Stat::StatCounter statHits_Partial;
  Stat::StatCounter statHits_User;
  Stat::StatCounter statHits_System;

  Stat::StatCounter statWatchRedundant;
  Stat::StatCounter statWatchRemoved;
  Stat::StatCounter statWatchReplaced;



  Stat::StatLog2Histogram statPrefetchLatency;
  Stat::StatCounter statPrefetchLatencySum;
  Stat::StatLog2Histogram statLifetimeToHit;
  Stat::StatLog2Histogram statLifetimeToDiscard;


  Stat::StatCounter statCMOBAppends;
  Stat::StatCounter statCMOBRequests;
  Stat::StatCounter statTrainingIndexInserts;
  Stat::StatCounter statTrainingIndexRandomlyDroppedInserts;
  Stat::StatCounter statTrainingIndexLookups;
  Stat::StatCounter statTrainingIndexMatches;
  Stat::StatCounter statTrainingIndexNoMatches;

  Stat::StatCounter statPersistentIndexUpdates;
  Stat::StatCounter statPersistentIndexInserts;
  Stat::StatCounter statPersistentIndexLookups;
  Stat::StatCounter statPersistentIndexMatches;
  Stat::StatCounter statPersistentIndexNoMatches;


  Stat::StatCounter statNoLookup_Pending;
  Stat::StatCounter statNoLookup_RecentPrefetch;
  Stat::StatCounter statNoLookup_InQueue;
  Stat::StatCounter statNoLookup_TooMany;

  Stat::StatCounter statLookups;

  Stat::StatCounter statMatchDiscarded_Pending;
  Stat::StatCounter statMatchDiscarded_RecentPrefetch;
  Stat::StatCounter statMatchDiscarded_NearExisting;

  Stat::StatCounter statNewStreamRequests;

  Stat::StatCounter statPokes;

  Stat::StatCounter statListDiscarded_Stale;
  Stat::StatCounter statListDiscarded_NoAddresses;
  Stat::StatCounter statListDiscarded_NearExisting;

  Stat::StatCounter statExtendDiscarded_NoQueue;
  Stat::StatCounter statExtendQueue;

  Stat::StatCounter statStreamQueuesAllocated;
  Stat::StatCounter statStreamQueuesDied;
  Stat::StatCounter statStreamQueuesKilled;
  Stat::StatCounter statStreamQueuesKilled_FailedExtend;

  Stat::StatCounter statPrefetchDropped_Pending;
  Stat::StatCounter statPrefetchDropped_RecentPrefetch;
  Stat::StatCounter statPrefetchDropped_RecentRequest;

  Stat::StatCounter statPins_Prefetch;
  Stat::StatCounter statPins_Read;
  Stat::StatCounter statOnChip_Read;
  Stat::StatCounter statCommit_ObservedRead;
  Stat::StatCounter statCommit_SVBHit;

  Stat::StatLog2Histogram statIndexLookupTime;
  Stat::StatLog2Histogram statStreamTimeToFirstPrefetch;
  Stat::StatLog2Histogram statStreamTimeToFirstHit;
  Stat::StatLog2Histogram statStreamTimeToDeath;

  Stat::StatInstanceCounter<long long> statStreamLength;

  TMSStats(std::string const & statName)
     : statCompletedPrefetches          ( statName + "-Prefetches:Completed")
     , statRejectedPrefetches           ( statName + "-Prefetches:Rejected")
     , statDiscardedLines               ( statName + "-Discards")
     , statReplacedLines                ( statName + "-Discards:Replaced")
     , statInvalidatedLines             ( statName + "-Discards:Invalidated")
     , statHits                         ( statName + "-Hits")
     , statHits_Partial                 ( statName + "-Hits:Partial")
     , statHits_User                    ( statName + "-Hits:User")
     , statHits_System                  ( statName + "-Hits:System")
     , statWatchRedundant               ( statName + "-Watch:Redundant")
     , statWatchRemoved                 ( statName + "-Watch:Removed")
     , statWatchReplaced                ( statName + "-Watch:Replaced")
     , statPrefetchLatency              ( statName + "-PrefetchLatency")
     , statPrefetchLatencySum           ( statName + "-PrefetchLatencySum")
     , statLifetimeToHit                ( statName + "-Lifetime:Hit")
     , statLifetimeToDiscard            ( statName + "-Lifetime:Discard")
    , statCMOBAppends                   ( statName + "-CMOBAppends")
    , statCMOBRequests                  ( statName + "-CMOBRequests")
    , statTrainingIndexInserts          ( statName + "-TrainingIndex:Inserts")
    , statTrainingIndexRandomlyDroppedInserts ( statName + "-TrainingIndex:Inserts:RndDropped")
    , statTrainingIndexLookups          ( statName + "-TrainingIndex:Lookups")
    , statTrainingIndexMatches          ( statName + "-TrainingIndex:Matches")
    , statTrainingIndexNoMatches        ( statName + "-TrainingIndex:NoMatches")
    , statPersistentIndexUpdates          ( statName + "-PersistentIndex:Updates")
    , statPersistentIndexInserts          ( statName + "-PersistentIndex:Inserts")
    , statPersistentIndexLookups          ( statName + "-PersistentIndex:Lookups")
    , statPersistentIndexMatches          ( statName + "-PersistentIndex:Matches")
    , statPersistentIndexNoMatches        ( statName + "-PersistentIndex:NoMatches")
    , statNoLookup_Pending                ( statName + "-NoLookup:Pending")
    , statNoLookup_RecentPrefetch         ( statName + "-NoLookup:RecentPrefetch")
    , statNoLookup_InQueue                ( statName + "-NoLookup:InQueue")
    , statNoLookup_TooMany                ( statName + "-NoLookup:TooMany")
    , statLookups                         ( statName + "-Lookups")
    , statMatchDiscarded_Pending          ( statName + "-MatchDiscarded:Pending")
    , statMatchDiscarded_RecentPrefetch   ( statName + "-MatchDiscarded:RecentPrefetch")
    , statMatchDiscarded_NearExisting     ( statName + "-MatchDiscarded:NearExisting")
    , statNewStreamRequests               ( statName + "-NewStreamRequests")
    , statPokes                           ( statName + "-Pokes")
    , statListDiscarded_Stale             ( statName + "-ListDiscarded:Stale")
    , statListDiscarded_NoAddresses       ( statName + "-ListDiscarded:NoAddresses")
    , statListDiscarded_NearExisting      ( statName + "-ListDiscarded:NearExisting")
    , statExtendDiscarded_NoQueue         ( statName + "-ExtendDiscarded:NoQueue")
    , statExtendQueue                     ( statName + "-ExtendQueue")
    , statStreamQueuesAllocated         ( statName + "-StreamQueues:Allocated")
    , statStreamQueuesDied              ( statName + "-StreamQueues:Died")
    , statStreamQueuesKilled            ( statName + "-StreamQueues:Killed")
    , statStreamQueuesKilled_FailedExtend( statName + "-StreamQueues:Killed:FailedExtend")
    , statPrefetchDropped_Pending         ( statName + "-PrefetchDropped:Pending")
    , statPrefetchDropped_RecentPrefetch  ( statName + "-PrefetchDropped:RecentPrefetch")
    , statPrefetchDropped_RecentRequest   ( statName + "-PrefetchDropped:RecentRequest")
    , statPins_Prefetch                 ( statName + "-Pins:PrefetchObserved" )
    , statPins_Read                     ( statName + "-Pins:ReadObserved ")
    , statOnChip_Read                     ( statName + "-OnChip:ReadObserved ")
    , statCommit_ObservedRead           ( statName + "-Commit:ObservedRead" )
    , statCommit_SVBHit                 ( statName + "-Commit:SVBHitObserved" )
    , statIndexLookupTime               ( statName + "-Stream:Time:IndexLookup" )
    , statStreamTimeToFirstPrefetch     ( statName + "-Stream:Time:To1stPrefetch" )
    , statStreamTimeToFirstHit          ( statName + "-Stream:Time:To1stHit" )
    , statStreamTimeToDeath             ( statName + "-Stream:Time:ToDeath" )
    , statStreamLength                  ( statName + "-Stream:Length" )
  { }
};

static const int kTrainingIndex = 0;
static const int kPersistentIndex = 1;
static const int kPersistentIndex_Update = 1;
static const int kPersistentIndex_Insert = 2;

struct StreamQueueBase : boost::counted_base {};

struct Controller {
  virtual ~Controller() {}
  virtual void prefetch( MemoryAddress anAddress, boost::intrusive_ptr<StreamQueueBase> ) = 0;  

  virtual long getNextCMOBIndex( ) = 0;  
  virtual void appendToCMOB( MemoryAddress anAddress, bool wasHit ) = 0;  
  virtual void recordMapping( int anIndex, MemoryAddress anAddress, int aCMOB, long aLocation ) = 0;
  virtual void requestMapping( int anIndex, MemoryAddress anAddress, unsigned long long aStartTime = 0) = 0;
  virtual void requestCMOBRange( int aCMOB, int aStartOffset, long anId ) = 0;

  virtual bool addressPending( MemoryAddress anAddress ) const = 0;

  virtual bool isFull_BufferOut() const = 0;
  virtual bool mayPrefetch() const = 0;
  virtual bool mayRequestAddresses() const = 0;
  virtual bool usePIndex() const = 0;
  virtual unsigned int maxStreamQueues() const = 0;
  virtual unsigned int minBufferCap() const = 0;
  virtual unsigned int maxBufferCap() const = 0;
  virtual unsigned int initBufferCap() const = 0;
  virtual unsigned int fetchCMOBAt() const = 0;
 
};

struct PrefetchPolicy {
  static PrefetchPolicy * createTMSPolicy(int anIndex, TMSStats &, Controller & );
  static PrefetchPolicy * createOnChipTMSPolicy(int anIndex, TMSStats &, Controller & );
  virtual ~PrefetchPolicy() {}

  virtual void process() {}

  //Queue-specific notifications
  virtual void completePrefetch(MemoryAddress const & anAddress, boost::intrusive_ptr<StreamQueueBase> aQ) {}
  virtual void cancelPrefetch(MemoryAddress const & anAddress, boost::intrusive_ptr<StreamQueueBase> aQ) = 0;
  virtual void removeLine(MemoryAddress const & anAddress, bool isReplacement, boost::intrusive_ptr<StreamQueueBase> aQ) = 0;  
  virtual void hit(MemoryAddress const & anAddress, boost::intrusive_ptr<StreamQueueBase> aQ, bool wasPartial) = 0;

  virtual void watchRequested( MemoryAddress anAddress ) {}
  virtual void watchKill( MemoryAddress anAddress, PrefetchMessage::PrefetchMessageType aReason ) {}

  //All these default to do nothing for most policies

  virtual void observe( MemoryMessage const & aMessage ) = 0;
  virtual void notifyRead( MemoryAddress const & anAddress, tFillLevel aFillLevel, bool wasPredicted) = 0;
  virtual void lookupMatch( MemoryAddress const & anAddress, int aCMOB, long aLocation, unsigned long long aStartTime)= 0;
  virtual void lookupNoMatch( ) = 0;
  virtual void handleCMOBReply( int aCMOB, long aLocation, long anId, std::vector<MemoryAddress> & anAddressLise, std::vector<bool> & aWasHit ) = 0;
  virtual void recentPBRequest( MemoryAddress const & anAddress) = 0;


    
};

} //nTMSControler


#endif //FLEXUS_TMSCONTROLLER_TMSPOLICIES_HPP_INCLUDED

