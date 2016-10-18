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
/*! \file CacheController.cpp
 * \brief
 *
 *  This file contains the implementation of the CacheController.  Alternate
 *  or extended definitions can be provided here as well.  This component
 *  is a main Flexus entity that is created in the wiring, and provides
 *  a full cache model.
 *
 * Revision History:
 *     ssomogyi    17 Feb 03 - Initial Revision
 *     twenisch    23 Feb 03 - Integrated with CacheImpl.hpp
 */

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <deque>
#include <vector>
#include <iterator>
#include <fstream>

#include <core/metaprogram.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <components/Common/TraceTracker.hpp>

using namespace boost::multi_index;

#include <core/performance/profile.hpp>
#include <core/types.hpp>
#include <core/stats.hpp>

using namespace Flexus;

#include "CacheControllerImpl.hpp"
#include "NewCacheArray.hpp"
#include "CacheBuffers.hpp"
#include "MissTracker.hpp"

#include <core/debug/debug.hpp>
  #define DBG_DeclareCategories Cache
  #define DBG_SetDefaultOps AddCat(Cache) Set( (CompName) << theInit->theName ) Set( (CompIdx) << theInit->theNodeId )
  #include DBG_Control()


namespace nCache {

  namespace Stat = Flexus::Stat;

  ////////////////////////////////////////////////////////////////////////////////
  // CacheControllerImpl class

  CacheControllerImpl::CacheControllerImpl ( BaseCacheController * aController,
                                             CacheInitInfo       * aInit )
    : BaseCacheControllerImpl ( aController,
                                aInit )
  {
    if (   aInit->theCores == 0
        || ((aInit->theCores == 1) && aInit->theIsPiranhaCache) )
    {
      DBG_ ( Crit, ( << "cache does not support cfg.Cores==" << aInit->theCores
                     << " && theIsPranhaCache==" << aInit->theIsPiranhaCache
                     << "! Bailing out!" ) );
      DBG_Assert ( false );
    }
  }



    // Perform a specific operation for the front side interface,
    // and sends the appropriate response out the front side.
    // Assumes that the block in the LookupResult is valid.
    // Returns the action to be done by the cache controller as a result of
    // this operation
  Action CacheControllerImpl::performOperation(MemoryMessage_p msg, TransactionTracker_p tracker, LookupResult & lookup, bool wasHit, bool anyInvs, bool blockAddressOnMiss) {
    Action action(kReplyAndRemoveMAF, true);
    // since the message type is altered below, translate the access
      // type based on the original message
      AccessType access = frontAccessTranslator::getAccessType(*msg);

      DBG_( Trace, Addr(msg->address()) ( << theInit->theName << " Perform op: " << *msg ) );

      // handle each operation slightly differently
      bool pref_hit_read = false;
      bool pref_hit_write = false;
      switch(msg->type()) {
        // the access policy will take care of the cache state (e.g. dirty/valid
        // bits) - we just need to read/write data and set the reply type

      case MemoryMessage::LoadReq:
        if(lookup.block().prefetched()) {
          pref_hit_read = true;
        }
        // TODO: copy word data to reply from cache
        msg->type() = MemoryMessage::LoadReply;
        break;

      case MemoryMessage::AtomicPreloadReq:
        if(lookup.block().prefetched()) {
          pref_hit_read = true;
        }
        // TODO: copy word data to reply from cache
        msg->type() = MemoryMessage::AtomicPreloadReply;
        break;

      case MemoryMessage::StoreReq:
        if(!lookup.block().modifiable()) {
          return handleMiss( msg, tracker, lookup, blockAddressOnMiss, theInit->theProbeOnIfetchMiss);
        }
        if(lookup.block().prefetched()) {
          pref_hit_write = true;
        }
        // TODO: copy word data to cache
        msg->type() = MemoryMessage::StoreReply;
        break;

      case MemoryMessage::NonAllocatingStoreReq:
        if(!lookup.block().modifiable()) {
          return handleMiss( msg, tracker, lookup, blockAddressOnMiss, theInit->theProbeOnIfetchMiss);
        }
        if(lookup.block().prefetched()) {
          pref_hit_write = true;
        }
        // TODO: copy word data to cache
        msg->type() = MemoryMessage::NonAllocatingStoreReply;
        break;
      
      case MemoryMessage::StorePrefetchReq:
        if(!lookup.block().modifiable()) {
          return handleMiss( msg, tracker, lookup, blockAddressOnMiss, theInit->theProbeOnIfetchMiss);
        }
        if(lookup.block().prefetched()) {
          pref_hit_write = true;
        }
        // TODO: copy word data to cache
        msg->type() = MemoryMessage::StorePrefetchReply;
        break;

      case MemoryMessage::FetchReq:
        if(lookup.block().prefetched()) {
          pref_hit_read = true;
        }
        // TODO: copy word data to reply from cache
        msg->type() = MemoryMessage::FetchReply;
        break;

      case MemoryMessage::CmpxReq:
        if(!lookup.block().modifiable()) {
          return handleMiss( msg, tracker, lookup, blockAddressOnMiss, theInit->theProbeOnIfetchMiss);
        }
        if(lookup.block().prefetched()) {
          pref_hit_write = true;
        }
        // TODO: swap data between cache and message
        msg->type() = MemoryMessage::CmpxReply;
        break;

      case MemoryMessage::RMWReq:
        if(!lookup.block().modifiable()) {
          return handleMiss( msg, tracker, lookup, blockAddressOnMiss, theInit->theProbeOnIfetchMiss);
        }
        if(lookup.block().prefetched()) {
          pref_hit_write = true;
        }
        // TODO: swap data between cache and message
        msg->type() = MemoryMessage::RMWReply;
        break;


      case MemoryMessage::ReadReq:
        if(lookup.block().prefetched()) {
          if ( tracker->originatorLevel() && (*tracker->originatorLevel() == eL2Prefetcher) ) {
            prefetchHitsPrefetch++;
            tracker->setFillLevel(eLocalMem);
            tracker->setNetworkTrafficRequired(false);
            tracker->setResponder(theInit->theNodeId);
          } else {
            pref_hit_read = true;
          }
        }
        // copy block data to reply from cache
        if(lookup.block().dirty()) {
          msg->type() = MemoryMessage::MissReplyDirty;
          DBG_Assert(lookup.block().modifiable());
        }
        else if(lookup.block().modifiable()) {
          msg->type() = MemoryMessage::MissReplyWritable;
        }
        else {
          msg->type() = MemoryMessage::MissReply;
        }
        break;

      case MemoryMessage::WriteReq:
        if(!lookup.block().modifiable()) {
          return handleMiss( msg, tracker, lookup, blockAddressOnMiss, theInit->theProbeOnIfetchMiss);
        }
        if(lookup.block().prefetched()) {
          pref_hit_write = true;
        }
        // copy block data to reply from cache
        if(lookup.block().dirty()) {
          msg->type() = MemoryMessage::MissReplyDirty;
        }
        else {
          msg->type() = MemoryMessage::MissReplyWritable;
        }
        break;

      case MemoryMessage::WriteAllocate:
        if(!lookup.block().modifiable()) {
          return handleMiss( msg, tracker, lookup, blockAddressOnMiss, theInit->theProbeOnIfetchMiss);
        }
        if(lookup.block().prefetched()) {
          pref_hit_write = true;
        }
        // first copy word/block data to cache from message
        // then copy (updated) block data to reply from cache
        // this makes sense if the size of the included data
        // is smaller than the size of the requested data
        if(lookup.block().dirty()) {
          msg->type() = MemoryMessage::MissReplyDirty;
        }
        else {
          msg->type() = MemoryMessage::MissReplyWritable;
        }
        break;

      case MemoryMessage::UpgradeReq:
        if(lookup.block().dirty()) {
          // if an address it dirty, it must be dirty at the highest
          // level that it is present in the hierarchy, and thus
          // there is no higher read-only copy to generate an upgrade
          // request like this one
          DBG_Assert(false);
        }
        if(!lookup.block().modifiable()) {
          return handleMiss( msg, tracker, lookup, blockAddressOnMiss, theInit->theProbeOnIfetchMiss);
        }
        if(lookup.block().prefetched()) {
          pref_hit_write = true;
        }
        msg->type() = MemoryMessage::UpgradeReply;
        action.theRequiresData--;
        break;

      case MemoryMessage::UpgradeAllocate:
        if(lookup.block().dirty()) {
          // see commment above for UpgradeReq
          DBG_Assert(false);
        }
        if(!lookup.block().modifiable()) {
          return handleMiss( msg, tracker, lookup, blockAddressOnMiss, theInit->theProbeOnIfetchMiss);
        }
        if(lookup.block().prefetched()) {
          pref_hit_write = true;
        }
        // copy block data to cache from message
        msg->type() = MemoryMessage::UpgradeReply;
        break;

      case MemoryMessage::PrefetchReadNoAllocReq:
        // to get here, the block must be present in this cache, which
        // means the prefetch was unnecessary
        msg->type() = MemoryMessage::PrefetchReadRedundant;
        action.theRequiresData--;
        break;

      case MemoryMessage::PrefetchReadAllocReq:
        if (! wasHit) {
          if(lookup.block().modifiable()) {
            msg->type() = MemoryMessage::PrefetchWritableReply;
          } else {
            msg->type() = MemoryMessage::PrefetchReadReply;
          }
        } else {
          msg->type() = MemoryMessage::PrefetchReadRedundant;
          action.theRequiresData--;
        }
        break;

      case MemoryMessage::ReturnReq:
        if ( lookup.block().dirty() ) {
          DBG_Assert ( false );
        }
        // Now handle in the processor
        break;

      /* CMU-ONLY-BLOCK-BEGIN */
      case MemoryMessage::PurgeIReq:
      case MemoryMessage::PurgeDReq:
        return handlePurge(msg, tracker, lookup);
        break;
      /* CMU-ONLY-BLOCK-END */

      case MemoryMessage::EvictDirty:     //Should no longer reach this function
      case MemoryMessage::EvictWritable:  //Should no longer reach this function
      case MemoryMessage::EvictClean:     //Should no longer reach this function
      case MemoryMessage::PrefetchInsertWritable:  //Should no longer reach this function
      case MemoryMessage::PrefetchInsert: //Should no longer reach this function
      case MemoryMessage::FlushReq:       //Should no longer reach this function
      case MemoryMessage::Flush:          //Should no longer reach this function
      default:
        DBG_Assert(false, ( << theInit->theName << " Got invalid message: " << *msg) );
      }
      if(anyInvs) {
        msg->setInvs();
      }

      if(pref_hit_read || pref_hit_write) {
        if(pref_hit_read) {
          prefetchHitsRead++;
        } else {
          prefetchHitsWrite++;
        }
        theTraceTracker.prefetchHit(theInit->theNodeId, theInit->theCacheLevel, getBlockAddress(*msg), pref_hit_write);
      }

      // handle the side effects of this access
      lookup.access(access);

      // update the memory transaction
      switch(msg->type()) {
      case MemoryMessage::LoadReply:
      case MemoryMessage::StoreReply:
      case MemoryMessage::StorePrefetchReply:
      case MemoryMessage::NonAllocatingStoreReply:
      case MemoryMessage::FetchReply:
      case MemoryMessage::RMWReply:
      case MemoryMessage::CmpxReply:
      case MemoryMessage::AtomicPreloadReply:
      case MemoryMessage::MissReply:
      case MemoryMessage::MissReplyWritable:
      case MemoryMessage::MissReplyDirty:
      case MemoryMessage::UpgradeReply:
        if (tracker && !tracker->fillLevel()) {
          tracker->setNetworkTrafficRequired(false);
          tracker->setResponder(theInit->theNodeId);
          tracker->setFillLevel(theInit->theCacheLevel);
        }
        break;
      default:
        // do nothing
        break;
      }

      // check in the snoop buffer
      SnoopEntry_p snEntry( theSnoopBuffer.findEntry( getBlockAddress(*msg)) );
      if(snEntry) {
        // an entry exists - update the probe state to represent the state
        // this cache array used to be in, so that if the probe response
        // is coming down the hierarchy and is already past the next higher
        // cache level, it does not miss information on the true state of the
        // hierarchy (because the probe will not see this response message,
        // which might contain the only dirty token, for example).
        switch(msg->type()) {
        case MemoryMessage::MissReplyDirty:
          snEntry->state = MemoryMessage::ProbedDirty;
          break;
        case MemoryMessage::MissReplyWritable:
        case MemoryMessage::UpgradeReply:
          snEntry->state = MemoryMessage::ProbedWritable;
          break;
        case MemoryMessage::MissReply:
          snEntry->state = MemoryMessage::ProbedClean;
          break;
        default:
          // do nothing
          break;
        }
      }

      // for false sharing
      switch(msg->type()) {
      case MemoryMessage::LoadReply:
      case MemoryMessage::AtomicPreloadReply:
        theTraceTracker.accessLoad(theInit->theNodeId, theInit->theCacheLevel, getBlockAddress(*msg), getBlockOffset(*msg), msg->reqSize());
        break;
      case MemoryMessage::StoreReply:
        theTraceTracker.accessStore(theInit->theNodeId, theInit->theCacheLevel, getBlockAddress(*msg), getBlockOffset(*msg), msg->reqSize());
        break;
      case MemoryMessage::FetchReply:
        theTraceTracker.accessFetch(theInit->theNodeId, theInit->theCacheLevel, getBlockAddress(*msg), getBlockOffset(*msg), msg->reqSize());
        break;
      case MemoryMessage::RMWReply:
      case MemoryMessage::CmpxReply:
        theTraceTracker.accessAtomic(theInit->theNodeId, theInit->theCacheLevel, getBlockAddress(*msg), getBlockOffset(*msg), msg->reqSize());
        break;
      default:
        break;
      }
     return action;
    }  // performOperation()


  // This is the miss handler - takes care of sending a request to fill the
  // missing block, and saving the miss request to the MAF so that
  // it can be satisfied once the fill occurs

  Action CacheControllerImpl::handleMiss ( MemoryMessage_p        msg,
                                           TransactionTracker_p   tracker,
                                           LookupResult         & lookup,
                                           bool                   address_conflict_on_miss,
                                           bool                   probeIfetchMiss )
  {
    if (address_conflict_on_miss) {
      DBG_(VVerb, Addr(msg->address()) ( << theInit->theName << " address locked" ) );
      lockedAccesses++;
      if (tracker) {
        tracker->setDelayCause(theInit->theName, "Address Conflict");
      }
      // put the request into the MAF, marking it to wait for this address
      return Action( kInsertMAF_WaitAddress );
    }

    /* CMU-ONLY-BLOCK-BEGIN */
    // handle purges 
    if (msg->isPurge()) {
        return handlePurge(msg, tracker, lookup);
    }
    /* CMU-ONLY-BLOCK-END */

    AccessType access = frontAccessTranslator::getAccessType(*msg);
    bool performUpgrade = false;

    bool was_write = msg->isWrite();
    bool was_prefetchwrite = (access == STORE_PREFETCH_REQ);
    bool was_prefetchreadalloc = (access == PREFETCH_READ_ALLOC_REQ || (tracker->originatorLevel() && (*tracker->originatorLevel() == eL2Prefetcher)) );

    // check if this was actually a hit, and it just needs an upgrade
    if(lookup.hit()) {
      performUpgrade = true;
      if(lookup.block().prefetched()) {
        theTraceTracker.prefetchHit(theInit->theNodeId, theInit->theCacheLevel, getBlockAddress(*msg), true);
        prefetchHitsButUpgrade++;
        lookup.block().setPrefetched(false);
      }
    } else {
      // look in the evict buffer now - if present, move the data to the victim
      // block (use the block address since evict entries must be aligned on
      // a cache block boundary)
      EvictBuffer::iterator evictee = theEvictBuffer.find(getBlockAddress(*msg));
      if(evictee != theEvictBuffer.end()) {
        Tag newTag = getTag(*msg);
        // Select a victim to swap with the evictee
        Block & victim = lookup.victim(access);
        if(victim.valid()) {
          if(victim.dirty()) {
            evictBlock(victim, lookup.blockAddress());
            DBG_(Trace, Addr(msg->address()) ( << theInit->theName << "  evicting dirty victim :  " << *msg));
          } else if ( theInit->theDoCleanEvictions ) {
            evictBlock(victim, lookup.blockAddress());
            DBG_(Trace, Addr(msg->address()) ( << theInit->theName << "  evicting clean victim :  " << *msg));
          } else {
            if(victim.prefetched()) {
              prefetchEvicts++;
            }
            theTraceTracker.eviction(theInit->theNodeId, theInit->theCacheLevel, lookup.blockAddress(), true);
            evicts_clean++;
          }
        }
        else {
          if(victim.tag() == newTag) {
            theTraceTracker.invalidTagRefill(theInit->theNodeId, theInit->theCacheLevel, lookup.blockAddress());
          } else {
            theTraceTracker.invalidTagReplace(theInit->theNodeId, theInit->theCacheLevel, lookup.blockAddress());
          }
        }

        // handle the side effects of reallocating in this cache
        AccessType evictAcc = frontAccessTranslator::getAccessType(evictee->type());
        lookup.access(evictAcc);
        // set the new tag
        lookup.block().tag() = newTag;
        // TODO: copy data to cache block from eviction message

        bool evictee_write = false;
        if(evictee->type() == MemoryMessage::EvictDirty) {
          evictee_write = true;
        }
        theTraceTracker.fill(theInit->theNodeId, theInit->theCacheLevel, getBlockAddress(*msg), theInit->theCacheLevel, false, evictee_write);

        //Note: performOperation will recursively call handleMiss if we are
        //unable to satisfy the request based on the contents of the evict
        //buffer
        Action act = performOperation(msg, tracker, lookup, true, false, false);
        if( act.theAction == kReplyAndRemoveMAF ) {
          DBG_(Trace, Addr(msg->address()) ( << theInit->theName << " got a hit after evict allocation for 0x" << &std::hex << msg->address()) );
        } else if ( act.theAction == kSend ) {
          DBG_(Dev, Addr(msg->address()) ( << theInit->theName << " got a send after evect alloc " << *msg ) );
          if(tracker->originatorLevel() && (*tracker->originatorLevel() == eL2Prefetcher)) {
            // this should probably also be a redundant prefetch
            //theTraceTracker.prefetch(theInit->theNodeId, theInit->theCacheLevel, msg->address());
          } else {
            theTraceTracker.access(theInit->theNodeId, theInit->theCacheLevel, msg->address(), msg->pc(), false, was_write, false, msg->isPriv(), (tracker->logicalTimestamp() ? *tracker->logicalTimestamp() : 0 ) );
          }
          hitsEvict++;
        } else if ( act.theAction == kInsertMAF_WaitResponse) {
          DBG_(Trace, Addr(msg->address()) ( << theInit->theName << " got a hit after evict allocation for 0x" << &std::hex << msg->address()) );
        } else if ( act.theAction == kSend ) {
          DBG_(Dev, (  << "Got a send after evect alloc " << *msg ) );
        } else {
          DBG_Assert( false, ( << theInit->theName << " Invalid action: " << act ) );
        }
        theEvictBuffer.remove( evictee );
        return act;
      }
    }

    //Pass on prefetch reads if they haven't been satisfied by the array or
    //evict buffer
    //Note: PrefetchReadAllocReq is not handled here - we map it to a read
    //request to the rest of the heirarchy if it misses in this cache level
    if (msg->type() == MemoryMessage::PrefetchReadNoAllocReq ) {
      misses++;
      if (tracker->OS() && *tracker->OS()) {
        misses_system_D++;
        misses_system_D_PrefetchRead++;
      } else {
        misses_user_D++;
        misses_user_D_PrefetchRead++;
      }
      return Action(kSend,false);
    }

    //Pass on NAWs if they haven't been satisfied by the array or
    //evict buffer
    if (msg->type() == MemoryMessage::NonAllocatingStoreReq ) {
      misses++;
      if (tracker->OS() && *tracker->OS()) {
        misses_system_D++;
        misses_system_D_Write++;
      } else {
        misses_user_D++;
        misses_user_D_Write++;
      }
      return Action(kSend,false);
    }

    //Instruction fetches may need to be probed
    if(msg->type() == MemoryMessage::FetchReq && probeIfetchMiss) {
      DBG_Assert(!performUpgrade);
      Action probe(kInsertMAF_WaitProbe, false);
      intrusive_ptr<MemoryMessage> req( new MemoryMessage(MemoryMessage::Probe,getBlockAddress(*msg)) );
      req->idxExtraOffsetBits() = msg->idxExtraOffsetBits();
      req->pageState() = msg->pageState();
      req->dstMC() = msg->dstMC();
      probe.addOutputMessage ( req );
      probe.theOutputTracker = tracker;
      return probe;
    }

    //TraceTracker miss (I-probes will fall through the cracks)
    if(!msg->isPrefetchType()) {
      theTraceTracker.access(theInit->theNodeId, theInit->theCacheLevel, msg->address(), msg->pc(), false, msg->isWrite(), !performUpgrade, msg->isPriv(), (tracker->logicalTimestamp() ? *tracker->logicalTimestamp() : 0 ));
    }

    // now generate the request message
    Action act(kInsertMAF_WaitResponse, false);
    intrusive_ptr<MemoryMessage> request;
    if(performUpgrade) {
      DBG_Assert(  access == STORE_REQ   || access == CMP_SWAP_REQ ||
                   access == RMW_REQ     || access == STORE_PREFETCH_REQ ||
                   access == WRITE_REQ   || access == WRITE_ALLOC  ||
                   access == UPGRADE_REQ || access == UPGRADE_ALLOC );

      // send an upgrade request for this block to the back side
      request = new MemoryMessage(MemoryMessage::UpgradeReq, getBlockAddress(*msg), msg->pc());
      request->idxExtraOffsetBits() = msg->idxExtraOffsetBits();
      request->pageState() = msg->pageState();
      request->dstMC() = msg->dstMC();
    } else {
      // send a fill request for this block to the back side
      switch(access) {
      case PREFETCH_READ_ALLOC_REQ:
        prefetchReads++;
      case LOAD_REQ:
      case READ_REQ:
        request = new MemoryMessage(MemoryMessage::ReadReq, getBlockAddress(*msg), msg->pc());
        break;
      case FETCH_REQ:
        request = new MemoryMessage(MemoryMessage::FetchReq, getBlockAddress(*msg));
        break;
      case ATOMIC_PRELOAD_REQ: //Atomic preloads map to writes if they miss in the array
        atomicPreloadWrites++;
      case STORE_REQ:
      case STORE_PREFETCH_REQ:
      case RMW_REQ:
      case CMP_SWAP_REQ:
      case WRITE_REQ:
      case WRITE_ALLOC:
      case UPGRADE_REQ:
      case UPGRADE_ALLOC:
      case EVICT_DIRTY:
        request = new MemoryMessage(MemoryMessage::WriteReq, getBlockAddress(*msg), msg->pc());
        break;
      default:
        DBG_Assert(false, ( << theInit->theName << " Got invalid message in miss handler: " << *msg) );
      }
    }
    request->reqSize() = theInit->theBlockSize;
    request->idxExtraOffsetBits() = msg->idxExtraOffsetBits();
    request->pageState() = msg->pageState();
    request->dstMC() = msg->dstMC();

    // for general purposes, this request should now be considered a miss
    misses++;
    if(performUpgrade) {
      upgrades++;
    }
    if (tracker && tracker->isFetch() && *tracker->isFetch()) {
      if (tracker->OS() && *tracker->OS()) {
        misses_system_I++;
      } else {
        misses_user_I++;
      }
    } else {
      if (was_prefetchwrite) {
        if (tracker->OS() && *tracker->OS()) {
          misses_system_D++;
          misses_system_D_PrefetchWrite++;
        } else {
          misses_user_D++;
          misses_user_D_PrefetchWrite++;
        }
      } else if (was_write) {
        if (tracker->OS() && *tracker->OS()) {
          misses_system_D++;
          misses_system_D_Write++;
        } else {
          misses_user_D++;
          misses_user_D_Write++;
        }
      } else if (was_prefetchreadalloc) {
        if (tracker->OS() && *tracker->OS()) {
          misses_system_D++;
          misses_system_D_PrefetchRead++;
        } else {
          misses_user_D++;
          misses_user_D_PrefetchRead++;
        }
      } else {
        if (tracker->OS() && *tracker->OS()) {
          misses_system_D++;
          misses_system_D_Read++;
        } else {
          misses_user_D++;
          misses_user_D_Read++;
        }
        theReadTracker.startMiss(tracker);
      }
    }

    DBG_( Trace, Addr(request->address()) ( << theInit->theName << " Adding output msg " << *request ));

    act.addOutputMessage ( request );
    act.theOutputTracker = tracker;
    act.theRequiresData = false;

    return act;
  }  // handleMiss()



    // Handles a MemoryTransport from the back side
    Action CacheControllerImpl::handleBackMessage(MemoryMessage_p msg, TransactionTracker_p tracker) {
      DBG_( Trace, Addr(msg->address()) ( << theInit->theName << " Handle BackProcess: " << *msg) );

      accesses++;
      if (tracker && tracker->isFetch() && *tracker->isFetch()) {
        if (tracker->OS() && *tracker->OS()) {
          accesses_system_I++;
        } else {
          accesses_user_I++;
        }
      } else {
        if (tracker && tracker->OS() && *tracker->OS()) {
          accesses_system_D++;
        } else {
          accesses_user_D++;
        }
      }

      if(msg->isRequest()) {
        Action request_act(kSend);
        DBG_( Trace, Addr(msg->address()) ( << theInit->theName << " IsRequest: " << *msg) );
        LookupResult result = theArray.lookup(msg->address(), (theInit->theCacheLevel == eL1) ? 0 : msg->idxExtraOffsetBits());
        if(msg->isProbeType()) {
          initializeProbe(msg, tracker, result);
        }
        else if(msg->isSnoopType()) {
          initializeSnoop(msg, tracker, result);
          request_act.theRequiresData = true;
        }
        else if(msg->type() == MemoryMessage::ReturnReq ) {
          // Handled in the processor now...
        } else {
          DBG_Assert(false, ( << theInit->theName << " Got invalid request message in back side: " << *msg ) );
        }
        return request_act;
      }

      // everything else must be a reply
      DBG_( Trace, Addr(msg->address()) ( << theInit->theName << " Switch: " << *msg) );
      switch(msg->type()) {

      case MemoryMessage::MissReply:
      case MemoryMessage::MissReplyWritable:
      case MemoryMessage::MissReplyDirty:
      case MemoryMessage::FetchReply:
      case MemoryMessage::UpgradeReply:
      {
        DBG_( Trace, Addr(msg->address()) ( << theInit->theName << " Reply branch: " << *msg) );

        MemoryMessage_p original_miss;
        TransactionTracker_p original_tracker;
        boost::tie( original_miss, original_tracker) = theController->getWaitingMAFEntry( msg->address() );

        //Get a lookup for the cache set filled by this reply
        LookupResult lookup = theArray.lookup(msg->address(), (theInit->theCacheLevel == eL1) ? 0 : msg->idxExtraOffsetBits());

        //TraceTracker miss
        bool prefetched = false;
        if (lookup.hit()) {
          if (lookup.block().prefetched()) {
            prefetched = true;
          }
        }
        bool tt_miss = (msg->type() != MemoryMessage::UpgradeReply);
        bool is_prefetch = false;
        if(original_tracker->originatorLevel() && (*original_tracker->originatorLevel() == eL2Prefetcher)) {
          is_prefetch = true;
          theTraceTracker.prefetch(theInit->theNodeId, theInit->theCacheLevel, original_miss->address());
        } else {
          theTraceTracker.access(theInit->theNodeId, theInit->theCacheLevel, original_miss->address(), original_miss->pc(), prefetched, original_miss->isWrite(), tt_miss, original_miss->isPriv(), (original_tracker->logicalTimestamp() ? *original_tracker->logicalTimestamp() : 0 ));
        }

        if(msg->type() == MemoryMessage::UpgradeReply) {
          upg_replies++;
        } else {
          fills++;
        }

        //If the lookup is a miss, allocate a block and move the victim to
        //the evict buffer
        if (! lookup.hit() ) {
          DBG_( Trace, Addr(msg->address()) ( << theInit->theName << " Missed! : " << *msg) );
          DBG_Assert (msg->reqSize() == theInit->theBlockSize, ( << theInit->theName << " Message: " << *msg << " theBlockSize=" << theInit->theBlockSize));
          if(msg->type() == MemoryMessage::UpgradeReply) {
            //fixme
            DBG_(Crit, ( << theInit->theName << " Upgrade reply does not contain data" ) );
          }
          Tag newTag = getTag(*msg);
          // the size of the request matches the array block size, so
          // we can allocate in this cache without initiating any
          // further requests
          Block & victim = lookup.victim(frontAccessTranslator::getAccessType(*msg));
          if(victim.valid()) {
            if(victim.dirty()) {
              DBG_( Trace, Addr(msg->address()) ( << theInit->theName << " Evicting dirty victim! : " << *msg) );
              evictBlock(victim, lookup.blockAddress());
            } else if ( theInit->theDoCleanEvictions ) {
              DBG_(Trace, Addr(msg->address()) ( << theInit->theName << "  Evicting clean victim! : " << *msg) );
              evictBlock(victim, lookup.blockAddress());
            } else {
              if(victim.prefetched()) {
                prefetchEvicts++;
              }
              theTraceTracker.eviction(theInit->theNodeId, theInit->theCacheLevel, lookup.blockAddress(), true);
              evicts_clean++;
            }
          }
          else {
            if(victim.tag() == newTag) {
              theTraceTracker.invalidTagRefill(theInit->theNodeId, theInit->theCacheLevel, lookup.blockAddress());
            } else {
              theTraceTracker.invalidTagReplace(theInit->theNodeId, theInit->theCacheLevel, lookup.blockAddress());
            }
          }
          lookup.block().tag() = getTag(msg->address());
          bool ifetch = false;
          if(original_miss->type() == MemoryMessage::FetchReq) {
            ifetch = true;
          }
          bool write = true;
          switch(original_miss->type()) {
            case MemoryMessage::FetchReq:
            case MemoryMessage::LoadReq:
            case MemoryMessage::ReadReq:
              write = false;
              break;
            case MemoryMessage::PrefetchReadAllocReq:
              write = false;
              is_prefetch = true;
              break;
            default:
              break;
          }
          if(is_prefetch) {
            theTraceTracker.prefetchFill(theInit->theNodeId, theInit->theCacheLevel, msg->address(), *tracker->fillLevel());
          } else {
            theTraceTracker.fill(theInit->theNodeId, theInit->theCacheLevel, msg->address(), *tracker->fillLevel(), ifetch, write);
          }
        }


        //Pass the tracker to theReadTracker for accounting, in case we are
        //finishing a tracked operation
        theReadTracker.finishMiss(original_tracker);

        DBG_Assert( original_miss , ( << theInit->theName << " did not find original miss processing back message: " << *msg ) );
        DBG_( Trace, Addr(msg->address()) ( << theInit->theName << " Original miss: " << *original_miss ) );


        // set the block state based on this response
        lookup.access(frontAccessTranslator::getAccessType(*msg));

        // determine if we need to set the prefetched bit (can't do this in the access function
        // because we only set the bit if the prefetch results in a fill)
        bool markPrefetched = false;
        if(   original_miss->type() == MemoryMessage::PrefetchReadAllocReq
              ||  (original_tracker->originatorLevel() && (*original_tracker->originatorLevel() == eL2Prefetcher))
              ) {
          markPrefetched = true;
        }
        DBG_Assert(markPrefetched == is_prefetch, ( << theInit->theName << " mark = " << markPrefetched ) );

        // now respond to the message that prompted this
        Action act = performOperation(original_miss, original_tracker, lookup, false, msg->anyInvs(), false);

        // set the prefetched bit after performOperation(); otherwise, the original access
        // would (incorrectly) clear the prefetched flag
        if(markPrefetched) {
          lookup.block().setPrefetched(true);
          DBG_(Iface, Addr(msg->address()) ( << theInit->theName << " setting prefetched bit for " << std::hex << msg->address() ) );
        }

        return act;
      }

      case MemoryMessage::PrefetchReadReply:
      case MemoryMessage::PrefetchWritableReply:
      case MemoryMessage::PrefetchReadRedundant:
        DBG_( Trace, Addr(msg->address()) ( << theInit->theName << " Prefetch? : " << *msg) );
        // just pass the message along
        return Action(kSend);

     case MemoryMessage::NonAllocatingStoreReply:
        // just pass the message along
        return Action(kSend);

     /* CMU-ONLY-BLOCK-BEGIN */
     case MemoryMessage::PurgeAck:
     {
        DBG_( Trace, Addr(msg->address()) ( << theInit->theName << " Reply branch: " << *msg) );

        MemoryMessage_p original_miss;
        TransactionTracker_p original_tracker;
        boost::tie( original_miss, original_tracker ) = theController->getWaitingMAFEntry( msg->address() );
        DBG_Assert( original_miss , ( << theInit->theName << " did not find original miss processing back message: " << *msg ) );
        DBG_( Trace, Addr(msg->address()) ( << theInit->theName << " Original miss: " << *original_miss ) );

        //Get a lookup for the cache set corresponding to this reply
        LookupResult lookup = theArray.lookup(msg->address(), (theInit->theCacheLevel == eL1) ? 0 : msg->idxExtraOffsetBits());
        DBG_Assert( !lookup.hit(), ( << theInit->theName << " Original miss: " << *original_miss ) ); // must be invalid
        DBG_( Trace, Addr(msg->address()) ( << theInit->theName << " Original miss: " << *original_miss ) );

        // now respond to the message that prompted this
        original_miss->type() = MemoryMessage::PurgeAck;
        if (original_tracker) {
          if (tracker && tracker->fillLevel()) {
            original_tracker->setFillLevel(*tracker->fillLevel());
          }
          else {
            original_tracker->setFillLevel(theInit->theCacheLevel);
          }
          original_tracker->setResponder(theInit->theNodeId);
          original_tracker->complete();
        }
        Action act(kReplyAndRemoveMAF);
        // act.theRequiresTag = false;  //FIXME PurgeAcks only need to go through the MAF pipelines

        return act;
     }
     /* CMU-ONLY-BLOCK-END */

      default:
        DBG_Assert(false, ( << theInit->theName << " Got invalid message in back side: " << *msg ) );
        return Action(kNoAction);
      }

   }  // handleBackSideMessage()


    // Handles a Snoop process from the front side snoop channel
    Action CacheControllerImpl::handleSnoopMessage(MemoryMessage_p msg, TransactionTracker_p tracker) {
      DBG_(Trace, Addr(msg->address()) ( << theInit->theName << " Handle snoop message: " << *msg ) );
      DBG_Assert(msg);
      DBG_Assert(msg->usesSnoopChannel());

      accesses++;
      LookupResult result = theArray.lookup(msg->address(), (theInit->theCacheLevel == eL1) ? 0 : msg->idxExtraOffsetBits());

      switch(msg->type()) {
        //Down Probes
          case MemoryMessage::DownProbe:
            return handleDownProbe(msg, tracker, result);

        //Probes
          case MemoryMessage::ProbedNotPresent:
          case MemoryMessage::ProbedClean:
          case MemoryMessage::ProbedWritable:
          case MemoryMessage::ProbedDirty:
              probes++;
              return finalizeProbe(msg, tracker, result);

        //Snoops
          case MemoryMessage::InvalidateAck:
          case MemoryMessage::InvUpdateAck:
          case MemoryMessage::DowngradeAck:
          case MemoryMessage::DownUpdateAck:
            snoops++;
            return finalizeSnoop(msg, tracker, result);

        //Flushes
          case MemoryMessage::FlushReq:
          case MemoryMessage::Flush:
            return handleFlush(msg, tracker, result);
            //Need to think about meaning of done
            break;

        //Allocations
          case MemoryMessage::EvictDirty:
          case MemoryMessage::EvictWritable:
          case MemoryMessage::EvictClean:
          case MemoryMessage::PrefetchInsert:
          case MemoryMessage::PrefetchInsertWritable:
            // these are similar in that they try to allocate in the array
            return handleAlloc(msg, tracker, result);

        //Data value returns
      case MemoryMessage::ReturnReply:
        msg->reqSize() = theInit->theBlockSize;
        return Action ( kSend, tracker );

        //Anything else is an error
          default:
            DBG_Assert( false,  ( << theInit->theName << " Invalid message processed in handleSnoopMessage: " << *msg) );
      }

      DBG_Assert( false );
      return Action(kSend); //Suppress compiler warning
    }

    // This handles a probe that was generated as a result of a Fetch miss
    // at this cache level.
    Action CacheControllerImpl::handleIprobe(bool aHit, MemoryMessage_p fetchReq, TransactionTracker_p tracker) {
      accesses++;
      iprobes++;
      LookupResult lookup = theArray.lookup(fetchReq->address(), (theInit->theCacheLevel == eL1) ? 0 : fetchReq->idxExtraOffsetBits());

      DBG_Assert(fetchReq->type() == MemoryMessage::FetchReq);
      if(!lookup.hit() && !aHit) {
        // this is an Ifetch miss
        Action act = handleMiss(fetchReq, tracker, lookup, false, false);
        DBG_Assert(act.theAction == kInsertMAF_WaitResponse);
        return act;
      }

      // consider this as an Ifetch hit - either the line is present in a
      // higher cache level, or it was allocated at this cache level after
      // the original Fetch request was handled (and the probe sent out)
      fetchReq->type() = MemoryMessage::FetchReply;
      if (tracker && !tracker->fillLevel()) {
        tracker->setNetworkTrafficRequired(false);
        tracker->setResponder(theInit->theNodeId);
        tracker->setFillLevel(theInit->theCacheLevel);
      }
      hits++;
      if (tracker->OS() && *tracker->OS()) {
        hits_system_I++;
      } else {
        hits_user_I++;
      }
      return Action(kReplyAndRemoveMAF, 1);
    }

    Action CacheControllerImpl::handleDownProbe(MemoryMessage_p msg, TransactionTracker_p tracker, LookupResult & lookup) {
      msg->type() = MemoryMessage::DownProbeNotPresent;
      if (lookup.hit()) {
        msg->type() = MemoryMessage::DownProbePresent;
      } else {
        //Check the evict buffer
        EvictBuffer::iterator evEntry( theEvictBuffer.find(msg->address()) );
        if(evEntry != theEvictBuffer.end()) {
          msg->type() = MemoryMessage::DownProbePresent;
        }
      }
      return Action(kSend);
    }

    // This is the miss handler for flushes
    Action CacheControllerImpl::handleFlush(MemoryMessage_p msg, TransactionTracker_p tracker, LookupResult & lookup) {
      AccessType access = frontAccessTranslator::getAccessType(*msg);
      bool passFlush = true;

      // look in the evict buffer for a matching entry - if found, always remove
      // it because the flush will take over
      EvictBuffer::iterator evEntry( theEvictBuffer.find(msg->address()) );
      if(evEntry != theEvictBuffer.end()) {
        switch(evEntry->type()) {

        case MemoryMessage::EvictDirty:
          // the flush can take over for the eviction
          // copy data (as/if necessary) from the evict buffer entry
          // mark the flush as containing data
          msg->type() = MemoryMessage::Flush;
          msg->reqSize() = theInit->theBlockSize;
          break;

        case MemoryMessage::EvictWritable:
        case MemoryMessage::EvictClean:
          // do nothing, since these should not change the flush, and they
          // have already been removed from the evict buffer,
          // unless we are allowing clean evictions

          if ( theInit->theDoCleanEvictions ) {
            msg->type() = MemoryMessage::Flush;
            msg->reqSize() = theInit->theBlockSize;
          }
          break;

        default:
          DBG_Assert(false, ( << theInit->theName << " Invalid message in evict buffer during miss/flush: " << *(evEntry)) );
        }
        theEvictBuffer.remove(evEntry);
      }

      // now look in the snoop buffer for a matching entry
      SnoopEntry_p snEntry( theSnoopBuffer.findEntry(msg->address()) );
      if(snEntry) {
        // first "downgrade" the probe state remembered in the snoop buffer
        if(snEntry->state != MemoryMessage::ProbedNotPresent) {
          // present, writable, and dirty must all be marked as clean
          snEntry->state = MemoryMessage::ProbedClean;
        }

        // now handle each snoop entry type differently
        switch(snEntry->message->type()) {

        case MemoryMessage::Probe:
        case MemoryMessage::ReturnReq:
          // the probe state is already changed; that is all we need to do here
          break;

        case MemoryMessage::Invalidate:
        case MemoryMessage::Downgrade:
          // these snoops guarantee to remove dirty data from all levels
          // of the hierarchy (much like a flush) so there is no need
          // to propagate the flush
          passFlush = false;
          if(msg->type() == MemoryMessage::Flush) {
            // this flush contains newer data, so update the WB entry
            // TODO: copy data (set reqSize if necessary)
            snEntry->message->reqSize() = theInit->theBlockSize;
            DBG_(Trace, Addr(msg->address()) ( << theInit->theName << " reply will collect flush data for 0x" << &std::hex << msg->address() << " msg: " << *msg));
          }
          break;

        default:
          DBG_Assert(false, ( << theInit->theName << " Invalid message in snoop buffer during miss/flush: " << *(snEntry->message)) );
        }
      }

      if(evEntry == theEvictBuffer.end()) {
        // otherwise look in the array (we do it in this order, because
        // if there is a evict entry, the array will at most contain a
        // clean non-modifiable block, which is uninteresting to a flush)
        if(lookup.hit()) {
          if(lookup.block().dirty()) {
            // TODO: copy block data to message
            // mark the message as containing data
            msg->type() = MemoryMessage::Flush;
            msg->reqSize() = theInit->theBlockSize;
          }
          // perform side effects
          lookup.access(access);
        }
      }

      if(passFlush) {
        return Action(kSend);
      } else {
        if (tracker) {
            tracker->setFillLevel(theInit->theCacheLevel);
            tracker->setResponder(theInit->theNodeId);
            tracker->complete();
        }
        return Action(kNoAction);
      }
    }  // handleFlush()


    /* CMU-ONLY-BLOCK-BEGIN */
    // This is the miss handler for purges
    Action CacheControllerImpl::handlePurge(  MemoryMessage_p msg
                                              , TransactionTracker_p tracker
                                              , LookupResult & lookup
                                           )
    {
      DBG_(Trace,
           Addr(msg->address())
           ( << theInit->theName 
             << " handlePurge for " << *msg 
             << (lookup.hit() ? " cache hit" : " cache miss")
          ));

      purges++;

      Action act(kInsertMAF_WaitResponse);

      AccessType access = frontAccessTranslator::getAccessType(*msg);
      bool passPurge = true;

      // look in the evict buffer for a matching entry - if found, always remove
      // it because the purge will take over
      EvictBuffer::iterator evEntry( theEvictBuffer.find(msg->address()) );
      if(evEntry != theEvictBuffer.end()) {
        switch(evEntry->type()) {

        case MemoryMessage::EvictDirty:
          // the purge can take over for the eviction
          // copy data (as/if necessary) from the evict buffer entry
          // mark the Purge as containing data
          // msg->type() = MemoryMessage::Flush;
          msg->reqSize() = theInit->theBlockSize;
          break;

        case MemoryMessage::EvictWritable:
        case MemoryMessage::EvictClean:
          // do nothing, since these should not change the purge, and they
          // have already been removed from the evict buffer,
          // unless we are allowing clean evictions

          if ( theInit->theDoCleanEvictions ) {
            // msg->type() = MemoryMessage::Flush;
          }
          break;

        default:
          DBG_Assert(false, ( << theInit->theName << " Invalid message in evict buffer during miss/purge: " << *(evEntry)) );
        }
        theEvictBuffer.remove(evEntry);
      }

      // now look in the snoop buffer for a matching entry
      SnoopEntry_p snEntry( theSnoopBuffer.findEntry(msg->address()) );
      if(snEntry) {
        // first "downgrade" the probe state remembered in the snoop buffer
        if(snEntry->state != MemoryMessage::ProbedNotPresent) {
          // present, writable, and dirty must all be marked as clean
          snEntry->state = MemoryMessage::ProbedClean;
        }

        // now handle each snoop entry type differently
        switch(snEntry->message->type()) {

        case MemoryMessage::Probe:
        case MemoryMessage::ReturnReq:
          // the probe state is already changed; that is all we need to do here
          break;

        case MemoryMessage::Invalidate:
        case MemoryMessage::Downgrade:
          // these snoops guarantee to remove dirty data from all levels
          // of the hierarchy (much like a purge) so there is no need
          // to propagate the purge
          passPurge = false;
          if(msg->type() == MemoryMessage::Flush) {
            // this purge contains newer data, so update the WB entry
            // TODO: copy data (set reqSize if necessary)
            snEntry->message->reqSize() = theInit->theBlockSize;
            DBG_(Trace, Addr(msg->address()) ( << theInit->theName << " reply will collect purge data for 0x" << &std::hex << msg->address() << " msg: " << *msg));
          }
          break;

        default:
          DBG_Assert(false, ( << theInit->theName << " Invalid message in snoop buffer during miss/purge: " << *(snEntry->message)) );
        }
      }

      if(lookup.hit()) {
        if(lookup.block().dirty()) {
          // TODO: copy block data to message
          // mark the message as containing data
          // msg->type() = MemoryMessage::Flush;
          msg->reqSize() = theInit->theBlockSize;
          act.theRequiresData = true; // may need to send the data down the cache hierarchy
        }
        // perform side effects
        lookup.access(access);
      }

      if (passPurge) {
        intrusive_ptr<MemoryMessage> req( new MemoryMessage(*msg) );
        act.addOutputMessage ( req );
        act.theOutputTracker = tracker;
        return act;
      } else {
        if (tracker) {
            tracker->setFillLevel(theInit->theCacheLevel);
            tracker->setResponder(theInit->theNodeId);
            tracker->complete();
        }
        msg->type() = MemoryMessage::PurgeAck;
        return Action(kReplyAndRemoveMAF);
      }
    }  // handlePurge()
    /* CMU-ONLY-BLOCK-END */

    //Handles allocation messages in the case the block is present in the cache array
    Action CacheControllerImpl::handleAlloc(MemoryMessage_p msg, TransactionTracker_p tracker, LookupResult & lookup ) {
      if (! lookup.hit()) {
        return handleAlloc_Miss( msg, tracker, lookup );
      }

      switch (msg->type()) {
        case MemoryMessage::EvictDirty:
          if(!lookup.block().modifiable()) {
            // this can only occur if a downgrade is in progress
            DBG_(VVerb, Addr(msg->address()) ( << theInit->theName
                                               << " Evict on non-modifiable block for 0x" << &std::hex << msg->address()
                                               << " on the floor, msg " << *msg));
            return handleAlloc_Miss( msg, tracker, lookup );
          }
          // TODO: copy block data to cache from message
          DBG_(VVerb, Addr(msg->address()) ( << theInit->theName 
                                             << " Set evict access bits for 0x" << &std::hex << msg->address()
                                             << " on the floor, msg " << *msg));
          // no response is necessary
          break;

        case MemoryMessage::EvictWritable:
          // the block is present here, and it was modifiable above, so it
          // should be modifiable here as well
          DBG_Assert(lookup.block().modifiable());
          // intentional fall-through...
        case MemoryMessage::EvictClean:
          // nothing to do, and no reponse necessary, so return from here
           break;

        case MemoryMessage::PrefetchInsertWritable:
          // the block is present here, and it was modifiable above, so it
          // should be modifiable here as well
          DBG_Assert(lookup.block().modifiable());
          // intentional fall-through...
        case MemoryMessage::PrefetchInsert:
          // to get here, the block must be present in this cache, which
          // means the insert is useless
          break;
        default:
          DBG_Assert(false, ( << theInit->theName << " Invalid message in call to handleAlloc: " << *(msg)) );
      }

      // handle the side effects of this access
      AccessType access = frontAccessTranslator::getAccessType(*msg);
      lookup.access(access);

      return Action(kNoAction);
    }

    //Handles allocation messages in the case the block is NOT present in the cache array
    Action CacheControllerImpl::handleAlloc_Miss(MemoryMessage_p msg, TransactionTracker_p tracker, LookupResult & lookup ) {
      AccessType access = frontAccessTranslator::getAccessType(*msg);
      bool allocate = true;

      // first look in the evict buffer - always remove a matching entry
      // (see explanation below)
      EvictBuffer::iterator evEntry( theEvictBuffer.find(msg->address()) );
      if(evEntry != theEvictBuffer.end()) {
        // there is an eviction waiting for the bus to become available...
        // the entry should be killed and we should allocate in this cache.
        // The incoming data must be at least as recent as the evict entry,
        // so we can truly ignore the eviction altogether.
        DBG_Assert(evEntry->type() != MemoryMessage::EvictDirty);
        theEvictBuffer.remove(evEntry);
      }

      // now check the snoop buffer
      SnoopEntry_p snEntry( theSnoopBuffer.findEntry(msg->address()) );
      if(snEntry) {
        switch(msg->type()) {

        case MemoryMessage::EvictDirty:
          switch(snEntry->message->type()) {
          case MemoryMessage::Invalidate:
            // an invalidation is in progress, so update data in the
            // SB entry with this most recent eviction data, but don't
            // allocate in the array
            // TODO: copy data to snoop entry
            // mark the entry as containing data
            snEntry->message->reqSize() = theInit->theBlockSize;
            DBG_(Trace, Addr(msg->address()) ( << theInit->theName << " reply will collect eviction data for 0x" << &std::hex << msg->address() << " msg: " << *msg));
            allocate = false;
            break;
          case MemoryMessage::Downgrade:
            // a downgrade is in progress, so update data in the SB entry
            // with this most recent eviction data... if the block is
            // present in the array, update its data; otherwise allocate
            // in the array (as clean non-modifiable)
            // TODO: copy data to snoop entry
            // mark the entry as containing data
            snEntry->message->reqSize() = theInit->theBlockSize;
            DBG_(Trace, Addr(msg->address()) ( << theInit->theName << " reply will collect eviction data for 0x" << &std::hex << msg->address() << " msg: " << *msg));
            if(lookup.hit()) {
              allocate = false;
              // TODO: copy data to array
            } else {
              allocate = true;
              // the eviction becomes clean
              msg->type() = MemoryMessage::EvictClean;
            }
            break;
          case MemoryMessage::Probe:
            // a pending probe does not affect the eviction, except the probe
            // state should be cleared, because the eviction will make the
            // correct state well-known
            snEntry->state = MemoryMessage::ProbedNotPresent;
            break;
          default:
            DBG_Assert(false, ( << theInit->theName << " Invalid message in snoop buffer during miss/evict: " << *(snEntry->message)) );
          }
          break;

        case MemoryMessage::EvictWritable:
        case MemoryMessage::EvictClean:
          switch(snEntry->message->type()) {
          case MemoryMessage::Invalidate:
            // an invalidation is in progress, so these evictions must
            // not be allowed to allocate
            allocate = false;
            break;
          case MemoryMessage::Downgrade:
          case MemoryMessage::ReturnReq:
            // a downgrade is in progress, so we can still allocate in
            // the array, but not with modify permission
            msg->type() = MemoryMessage::EvictClean;
            break;
          case MemoryMessage::Probe:
            // a pending probe does not affect these evictions, except the
            // probe state should be cleared, because the eviction will make
            // the correct state well-known
            snEntry->state = MemoryMessage::ProbedNotPresent;
            break;
          default:
            DBG_Assert(false, ( << theInit->theName << " Invalid message in snoop buffer during miss/insert: " << *(snEntry->message)) );
          }
          break;

        case MemoryMessage::PrefetchInsert:
        case MemoryMessage::PrefetchInsertWritable:
          switch(snEntry->message->type()) {
          case MemoryMessage::Invalidate:
            // an invalidation is in progress, so don't allocate in
            // the array... the snoop entry should not contain data
            DBG_Assert(snEntry->message->reqSize() == 0);
            allocate = false;
            break;
          case MemoryMessage::Downgrade:
            // a downgrade is in progress, so we can still allocate in
            // the array... the snoop entry should not contain data
            DBG_Assert(snEntry->message->reqSize() == 0);
            // TODO: verify that the data is identical
            msg->type() = MemoryMessage::PrefetchInsert;
            break;
          case MemoryMessage::ReturnReq:
            // a return req is in progress, so we can still allocate in
            // the array... the snoop entry should not contain data
            DBG_Assert(snEntry->message->reqSize() == 0);
            break;
          case MemoryMessage::Probe:
            // a pending probe does not affect the insert, except the probe
            // state should be cleared, because the insert will make the
            // correct state well-known
            snEntry->state = MemoryMessage::ProbedNotPresent;
            break;
          default:
            DBG_Assert(false, ( << theInit->theName << " Invalid message in snoop buffer during miss/insert: " << *(snEntry->message)) );
          }
          break;

        default:
          DBG_Assert(false, ( << theInit->theName << " Invalid MemoryMessage during miss/insert: " << *msg ) );
        }  // end switch msg->type()

      }  // end if snEntry

      if(allocate) {
        // the block cannot be in the array - it is obviously not present
        // and modifiable (or this miss handler would not have been called),
        // nor can it normally be present and non-modifiable (for a non-dirty
        // insert/evict, we should not get here; and for dirty insert/evict,
        // every place this block is present in the hierarchy, it must be
        // marked as modifiable)
        DBG_Assert(!lookup.hit(), ( << theInit->theName << " Invalid allocation while present (block state=" << lookup.block().state() << ") in array: " << *msg ) );
        if(msg->reqSize() == theInit->theBlockSize) {
          Tag newTag = getTag(*msg);
          // the size of the request matches the array block size, so
          // we can allocate in this cache without initiating any
          // further requests
          Block & victim = lookup.victim(access);
          if(victim.valid()) {
            if(victim.dirty()) {
              DBG_(Trace, Addr(msg->address()) ( << theInit->theName << "  Evicting dirty victim :  " << *msg));
              evictBlock(victim, lookup.blockAddress());
            } else if ( theInit->theDoCleanEvictions ) {
              evictBlock(victim, lookup.blockAddress());
              DBG_(Trace, Addr(msg->address()) ( << theInit->theName << "  Evicting clean victim :  " << *msg));
            } else {
              if(victim.prefetched()) {
                prefetchEvicts++;
              }
              theTraceTracker.eviction(theInit->theNodeId, theInit->theCacheLevel, lookup.blockAddress(), true);
              evicts_clean++;
            }
          }
          else {
            if(victim.tag() == newTag) {
              theTraceTracker.invalidTagRefill(theInit->theNodeId, theInit->theCacheLevel, lookup.blockAddress());
            } else {
              theTraceTracker.invalidTagReplace(theInit->theNodeId, theInit->theCacheLevel, lookup.blockAddress());
            }
          }
          // handle the side effects of this access
          lookup.access(access);
          // save the new tag - allocation is now done
          lookup.block().tag() = getTag(msg->address());
          theTraceTracker.insert(theInit->theNodeId, theInit->theCacheLevel, msg->address());
        }
      }

      if (tracker) {
            tracker->setFillLevel(theInit->theCacheLevel);
            tracker->setResponder(theInit->theNodeId);
            tracker->complete();
      }

      return Action(kNoAction, (allocate?1:0) );
    }  // handleAlloc()

}  // end namespace nCache
