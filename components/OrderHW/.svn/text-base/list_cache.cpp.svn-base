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

#include "list_cache.hpp"

#ifdef ENABLE_DEPRECATED

#include <sstream>
#include <fstream>

#define __STDC_CONSTANT_MACROS
#include <boost/date_time/posix_time/posix_time.hpp>



extern StatCounter overallConsumptions;
extern StatCounter overallConsumptions_User;
extern StatCounter overallConsumptions_System;
extern StatCounter overallHits;
extern StatCounter overallHits_User;
extern StatCounter overallHits_System;

StatCounter overallForwards("overall.Forwards");
StatCounter overallDuplicateForwards("overall.DupForwards");
StatCounter overallDiscards("overall.Discards");
StatCounter overallLRUDiscards("overall.LRUDiscards");
StatCounter overallInvalidateDiscards("overall.InvDiscards");
StatCounter overallCacheDiscards("overall.CacheDiscards");
StatCounter overallHitsMyOrder("overall.HitsMyOrder");
OLD_STAT( StatCounter overallHitSameStreamSameOrder("overall.HitSameStreamSameOrder"); )
OLD_STAT( StatCounter overallHitSameStreamDiffOrder("overall.HitSameStreamDiffOrder"); )
StatCounter overallHitDiffStreamSameOrder("overall.HitDiffStreamSameOrder");
StatCounter overallHitDiffStreamDiffOrder("overall.HitDiffStreamDiffOrder");
StatCounter overallHitDiffStreamNoPrevOrder("overall.HitDiffStreamNoPrevOrder");
OLD_STAT( StatCounter overallMissExistingHandled("overall.MissExistingHandled"); )
extern StatCounter overallMissCreateStream;
extern StatCounter overallMissNoStream;
StatCounter overallEvictedStream("overall.EvictedStream");
OLD_STAT( StatCounter overallOrphanedHits("overall.OrphanedHits"); )
StatWeightedLog2Histogram overallConsecutiveStreamHits("overall.ConsecutiveStreamHits");
StatInstanceCounter<long long> overallHitInstances_User("overall.HitInstancesUser");
StatInstanceCounter<long long> overallHitInstances_System("overall.HitInstancesSystem");

extern unsigned long kPullStreamBlockSize;

ListCache::ListCache(std::string aName, int anId, int aNumNodes, unsigned long aCapacity, unsigned long aBlockSize, unsigned long aMaxStreams, unsigned long anIntersectDistance, Predicate anInvalidatePredicate, Predicate aConsumePredicate, PullStreamRequestFn aStreamRequestFn, Directory* aDirectory)
 : theName(aName)
 , theId(anId)
 , theNextStreamId(0)
 , theCapacity(aCapacity)
 , theBlockSize(aBlockSize)
 , theMaxStreams(aMaxStreams)
 , theIntersectDistance(anIntersectDistance)
 , theInvalidatePredicate(anInvalidatePredicate)
 , theConsumePredicate(aConsumePredicate)
 , theStreamRequestFn(aStreamRequestFn)
 , theDirectory(aDirectory)
 , theConsecutiveStreamHits(0)
 , theForwards( aName + ".Forwards")
 , theDuplicateForwards( aName + ".DupForwards")
 , theDiscards( aName + ".Discards")
 , theLRUDiscards( aName + ".LRUDiscards")
 , theInvalidateDiscards( aName + ".InvDiscards")
 , theCacheDiscards( aName + ".CacheDiscards")
 , theConsumptions( aName + ".Consumptions")
 , theConsumptions_User( aName + ".ConsumptionsUser")
 , theConsumptions_System( aName + ".ConsumptionsSystem")
 , theHits( aName + ".Hits")
 , theHits_User( aName + ".HitsUser")
 , theHits_System( aName + ".HitsSystem")
 , theHitsMyOrder( aName + ".HitsMyOrder")
 , theHitSameStreamSameOrder( aName + ".HitSameStreamSameOrder")
 , theHitSameStreamDiffOrder( aName + ".HitSameStreamDiffOrder")
 , theHitDiffStreamSameOrder( aName + ".HitDiffStreamSameOrder")
 , theHitDiffStreamDiffOrder( aName + ".HitDiffStreamDiffOrder")
 , theHitDiffStreamNoPrevOrder( aName + ".HitDiffStreamNoPrevOrder")
 , theMissExistingHandled( aName + ".MissExistingHandled")
 , theMissCreateStream( aName + ".MissCreateStream")
 , theMissNoStream( aName + ".MissNoStream")
 , theEvictedStream( aName + ".EvictedStream" )
 , theOrphanedHits( aName + ".OrphanedHits" )
 {
  kPullStreamBlockSize = theBlockSize;
   // nothing at the moment
 }

ListCache::~ListCache() {
  boost::posix_time::ptime now(boost::posix_time::second_clock::local_time());
  DBG_(Dev, ( << "Destructing list cache " << theName << " at " << now ) );
  theCache.clear();
  theStreams.clear();

  thePreviousStreamAccess.reset();
}

void ListCache::finalize() {
  DBG_(Dev, ( << "Finalizing list cache " << theName ) );

  // count remaining forwarded blocks as discards
  OLD_STAT( theCacheDiscards += theCache.size(); overallCacheDiscards += theCache.size(); )
  OLD_STAT( theDiscards += theCache.size(); overallDiscards += theCache.size(); )

  if(thePreviousStreamAccess) {
    thePreviousStreamAccess->finalize();
  }

  cache_table::index<by_LRU>::type::iterator cache_iter = theCache.get<by_LRU>().begin();
  while(cache_iter != theCache.get<by_LRU>().end()) {
    cache_iter->finalize();
    ++cache_iter;
  }

  stream_table::index<by_stream>::type::iterator stream_iter = theStreams.get<by_stream>().begin();
  while(stream_iter != theStreams.get<by_stream>().end()) {
    stream_iter->finalize();
    ++stream_iter;
  }
}


bool ListCache::forward( tAddress anAddress, boost::shared_ptr<SuperStream> aStream, long anIndex, long long aSequenceNo, long long aHeadSequenceNo ) {
  //DBG_(Dev, ( << "forward addr " << std::hex << anAddress << " from node " << std::dec << aStream->id(0) ) );

  anAddress &= (~(theBlockSize - 1));

  cache_table::index<by_address>::type::iterator addr_iter = theCache.get<by_address>().find(anAddress);
  if (addr_iter != theCache.get<by_address>().end()) {
    ++theDuplicateForwards; ++overallDuplicateForwards;
    // on a duplicate forward, keep the older block (i.e. reject this block)
    return false;
  }

  ++theForwards; ++overallForwards;
  //drop(anAddress, kDuplicateForward);
  if (theCache.size() >= theCapacity) {
    OLD_STAT( ++theLRUDiscards; ++overallLRUDiscards; )
    OLD_STAT( ++theDiscards; ++overallDiscards; )
    theCache.get<by_LRU>().pop_front();
  }

  // check for this stream in the list
  typedef stream_table::index<by_stream>::type stream_index_t;
  stream_index_t::iterator strIter = theStreams.get<by_stream>().find(aStream);
  if(strIter != theStreams.get<by_stream>().end()) {
    bool ok = theStreams.get<by_stream>().modify(strIter, ll::bind(&PullStreamStorageEntry::addBlock, ll::_1, theNextStreamId++) );
    DBG_Assert(ok);
  }

  theCache.get<by_LRU>().push_back(ListCacheEntry(anAddress, aStream, anIndex, aSequenceNo, aHeadSequenceNo));

  return true;
}

void ListCache::event( TraceData & anEvent) {
  if (theInvalidatePredicate(anEvent)) {
    invalidate(anEvent.theAddress);
  }
  if (theConsumePredicate(anEvent)) {
#ifdef ENABLE_ENTRY_STATE
    anEvent.theWasConsumed = true;
    #ifdef ENABLE_COMBO
      bool wasHit = consume(anEvent.theAddress, anEvent.thePC, anEvent.theOS, &anEvent.theHitId, &anEvent.theHitSeqNo);
    #else
      bool wasHit = consume(anEvent.theAddress, anEvent.thePC, anEvent.theOS, 0, 0);
    #endif //ENABLE_COMBO
    anEvent.theWasHit = wasHit;
#else
    consume(anEvent.theAddress, anEvent.thePC, anEvent.theOS, 0, 0);
#endif //ENABLE_ENTRY_STATE
  }
}

void ListCache::invalidate( tAddress anAddress) {
  DBG_(Verb, ( << theName << ": Invalidate @" << std::hex << anAddress << std::dec ) );
  anAddress &= (~(theBlockSize - 1));

  cache_table::index<by_address>::type::iterator iter = theCache.get<by_address>().find(anAddress);
  if (iter != theCache.get<by_address>().end()) {
    OLD_STAT( ++theDiscards; ++overallDiscards; )
    OLD_STAT( ++theInvalidateDiscards; ++overallInvalidateDiscards; )

    typedef stream_table::index<by_stream>::type stream_index_t;
    stream_index_t::iterator strIter = theStreams.get<by_stream>().find(iter->theStream);
    if(strIter != theStreams.get<by_stream>().end()) {
      bool ok = theStreams.get<by_stream>().modify(strIter, ll::bind(&PullStreamStorageEntry::removeBlock, ll::_1) );
      DBG_Assert(ok);
    }

    theCache.get<by_address>().erase(iter);
  }
}

bool ListCache::consume( tAddress anAddress, tAddress aPC, bool anOS, tID * hitId, long long * hitSeqNo ) {
  bool wasHit = false;

  tAddress block_addr = anAddress & (~(theBlockSize - 1));

  ++theConsumptions; ++overallConsumptions;
  if (anOS) {
    ++theConsumptions_System; ++overallConsumptions_System;
  } else {
    ++theConsumptions_User; ++overallConsumptions_User;
  }
  cache_table::index<by_address>::type::iterator  iter = theCache.get<by_address>().find(block_addr);
  if (iter != theCache.get<by_address>().end()) {
    DBG_( Trace, ( << theName << ": Consume @" << std::hex << anAddress << std::dec << " Hit"));
    wasHit = true;

    boost::shared_ptr<SuperStream> stream = iter->theStream;
    long strIndex = iter->theStreamIndex;
    long long seqNo = iter->theSequenceNo;
    long long headSeqNo = iter->theHeadSequenceNo;
    //tID directory = theDirectory->node(block_addr);
    tID strId = stream->id(strIndex);
    theCache.get<by_address>().erase(iter);

    //DBG_(Dev, ( << std::hex << anAddress << " " << std::dec << strId) );

#ifdef ENABLE_COMBO
    *hitId = strId;
    *hitSeqNo = seqNo;
#endif //ENABLE_COMBO


    ++theHits; ++overallHits;
    if (anOS) {
      ++theHits_System; ++overallHits_System;
      //overallHitInstances_System << std::make_pair( static_cast<long long>(anAddress), 1LL);
    } else {
      ++theHits_User; ++overallHits_User;
      //overallHitInstances_User << std::make_pair( static_cast<long long>(anAddress), 1LL);
    }
    if(strId == theId) {
      OLD_STAT( ++theHitsMyOrder; ++overallHitsMyOrder; )
    }

    long currBlocks = streamConsume(true, stream);

    stream->notifyHit(anAddress, aPC, strIndex, seqNo, headSeqNo, currBlocks);

    #ifdef OLD_STATS
    if(stream == thePreviousStreamAccess) {
      if(thePreviousStreamAccess->id(thePreviousStreamIndex) == strId) {
        ++theHitSameStreamSameOrder; ++overallHitSameStreamSameOrder;
      } else {
        ++theHitSameStreamDiffOrder; ++overallHitSameStreamDiffOrder;
        thePreviousStreamIndex = strIndex;
      }
      overallConsecutiveStreamHits >> std::make_pair( static_cast<long long>(theConsecutiveStreamHits),
                                                      static_cast<long long>(theConsecutiveStreamHits) );
      ++theConsecutiveStreamHits;
      overallConsecutiveStreamHits << std::make_pair( static_cast<long long>(theConsecutiveStreamHits),
                                                      static_cast<long long>(theConsecutiveStreamHits) );
    } else {
      if(thePreviousStreamAccess) {
        if(thePreviousStreamAccess->id(thePreviousStreamIndex) == strId) {
          ++theHitDiffStreamSameOrder; ++overallHitDiffStreamSameOrder;
        } else {
          ++theHitDiffStreamDiffOrder; ++overallHitDiffStreamDiffOrder;
        }
      }
      else {
        ++theHitDiffStreamNoPrevOrder; ++overallHitDiffStreamNoPrevOrder;
      }
      theConsecutiveStreamHits = 0;
      thePreviousStreamAccess = stream;
      thePreviousStreamIndex = strIndex;
    }
    #endif //ENABLE_OLD_STATS
  } else {
    DBG_( Trace, ( << theName << ": Consume @" << std::hex << anAddress << std::dec << " Miss"));
    boost::shared_ptr<SuperStream> newStream;

    // first check if this miss will be handled by an existing stream
    bool handled = false;
    stream_table::index<by_stream>::type::iterator stream_iter = theStreams.get<by_stream>().begin();
    while(stream_iter != theStreams.get<by_stream>().end()) {
      handled |= stream_iter->theStream->notifyMiss(anAddress, aPC);
      ++stream_iter;
    }

    if(handled) {
      OLD_STAT( ++theMissExistingHandled; ++overallMissExistingHandled; )
    }
    else {
      newStream = theStreamRequestFn(this, anAddress, aPC);
      if(newStream) {
        //DBG_(Dev, ( << "stream created on miss address " << std::hex << anAddress << " from node " << std::dec << newStream->id(0) ) );
        streamConsume(false, newStream);
        ++theMissCreateStream; ++overallMissCreateStream;
        newStream->beginForward();
      }
      else {
        ++theMissNoStream; ++overallMissNoStream;
      }
    }

    theConsecutiveStreamHits = 0;
    thePreviousStreamAccess = newStream;
    thePreviousStreamIndex = 0;
  }

  return wasHit;
}

long ListCache::streamConsume(bool isHit, boost::shared_ptr<SuperStream> aStream) {
  bool found = false;
  long blocks = -1;

  if(isHit) {
    stream_table::index<by_stream>::type::iterator strIter = theStreams.get<by_stream>().find(aStream);
    if(strIter != theStreams.get<by_stream>().end()) {
      found = true;
      bool ok = theStreams.get<by_stream>().modify(strIter, ll::bind(&PullStreamStorageEntry::hitBlock, ll::_1, theNextStreamId++) );
      DBG_Assert(ok);
      blocks = strIter->thePresentBlocks;
    }
  }

  if(!found) {
    if(isHit) {
      OLD_STAT( ++theOrphanedHits; ++overallOrphanedHits; )
    }
    while(theStreams.size() >= theMaxStreams) {
      theStreams.get<by_replacement>().erase(theStreams.get<by_replacement>().begin());
      OLD_STAT( ++theEvictedStream; ++overallEvictedStream; )
    }
    // insert new entry - it will be positioned automagically
    bool ok = theStreams.insert( PullStreamStorageEntry(aStream,theNextStreamId++) ).second;
    DBG_Assert(ok, ( << theName << "Insert new stream failed") );
  }

  return blocks;
}

#else //!ENABLE_DEPRECATED

ListCache::ListCache(std::string aName, int anId, int aNumNodes, unsigned long aCapacity, unsigned long aBlockSize, unsigned long aMaxStreams, unsigned long anIntersectDistance, Predicate anInvalidatePredicate, Predicate aConsumePredicate, PullStreamRequestFn aStreamRequestFn, Directory* aDirectory)
 : theForwards( aName + ".Forwards")
 , theDuplicateForwards( aName + ".DupForwards")
 , theDiscards( aName + ".Discards")
 , theLRUDiscards( aName + ".LRUDiscards")
 , theInvalidateDiscards( aName + ".InvDiscards")
 , theCacheDiscards( aName + ".CacheDiscards")
 , theConsumptions( aName + ".Consumptions")
 , theConsumptions_User( aName + ".ConsumptionsUser")
 , theConsumptions_System( aName + ".ConsumptionsSystem")
 , theHits( aName + ".Hits")
 , theHits_User( aName + ".HitsUser")
 , theHits_System( aName + ".HitsSystem")
 , theHitsMyOrder( aName + ".HitsMyOrder")
 , theHitSameStreamSameOrder( aName + ".HitSameStreamSameOrder")
 , theHitSameStreamDiffOrder( aName + ".HitSameStreamDiffOrder")
 , theHitDiffStreamSameOrder( aName + ".HitDiffStreamSameOrder")
 , theHitDiffStreamDiffOrder( aName + ".HitDiffStreamDiffOrder")
 , theHitDiffStreamNoPrevOrder( aName + ".HitDiffStreamNoPrevOrder")
 , theMissExistingHandled( aName + ".MissExistingHandled")
 , theMissCreateStream( aName + ".MissCreateStream")
 , theMissNoStream( aName + ".MissNoStream")
 , theEvictedStream( aName + ".EvictedStream" )
 , theOrphanedHits( aName + ".OrphanedHits" )
{
   // nothing at the moment
}

ListCache::~ListCache() {
}

void ListCache::finalize() {
}


bool ListCache::forward( tAddress anAddress, boost::shared_ptr<SuperStream> aStream, long anIndex, long long aSequenceNo, long long aHeadSequenceNo ) {
  return true;
}

void ListCache::event( TraceData & anEvent) { }

void ListCache::invalidate( tAddress anAddress) { }

bool ListCache::consume( tAddress anAddress, tAddress aPC, bool anOS, tID * hitId, long long * hitSeqNo ) { return false; }

long ListCache::streamConsume(bool isHit, boost::shared_ptr<SuperStream> aStream) { return 0; }

#endif //ENABLE_DEPRECATED
