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
/*! \file BaseCacheController.cpp
 * \brief
 *
 *  This file contains the implementation of the BaseCacheController.  Alternate
 *  or extended definitions should be provided elsewhere.  This component
 *  is a main Flexus entity that is created in the wiring, and provides
 *  the guts of a full cache model.
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

#include <core/performance/profile.hpp>
#include <core/metaprogram.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>
#include <components/Common/TraceTracker.hpp>

using namespace boost::multi_index;
using namespace Flexus;

#include "BaseCacheControllerImpl.hpp"
#include "CacheControllerImpl.hpp"
#include "PiranhaCacheControllerImpl.hpp"

/* CMU-ONLY-BLOCK-BEGIN */
#ifdef FLEXUS_FCMP
  #include "FCMPCacheControllerImpl.hpp"
#endif
/* CMU-ONLY-BLOCK-END */

#include <core/debug/debug.hpp>
  #define DBG_DeclareCategories Cache
  #define DBG_SetDefaultOps AddCat(Cache) Set( (CompName) << theInit->theName ) Set( (CompIdx) << theInit->theNodeId )
  #include DBG_Control()

namespace nCache {

  namespace Stat = Flexus::Stat;

  std::ostream & operator << ( std::ostream & s, const Action action )
  {
    return s << action.theAction;
  }

  std::ostream & operator << ( std::ostream & s, const enumAction eAction )
  {
    const char * actions[] =
      {
        "kNoAction",
        "kSend",
        "kInsertMAF_WaitAddress",
        "kInsertMAF_WaitResponse",
        "kInsertMAF_WaitProbe",
        "kReplyAndRemoveMAF",
        "kReplyAndRemoveResponseMAF",
      };
    return s << actions[eAction];
  }

  eCacheType cacheTypeName ( std::string type )
  {
    if ( type == "baseline" )
      return kBaselineCache;
    if ( type == "piranha" )
      return kPiranhaCache;
/* CMU-ONLY-BLOCK-BEGIN */
#ifdef FLEXUS_FCMP
    if ( type == "fcmp" )
      return kFCMPCache;
#endif
/* CMU-ONLY-BLOCK-END */

    DBG_ ( Crit, NoDefaultOps() ( << " Unknown cache type specified: " << type ) );
    DBG_Assert ( false, NoDefaultOps()  );

    return kBaselineCache;
  }

  BaseCacheControllerImpl * BaseCacheControllerImpl::construct
  ( BaseCacheController * aController,
    CacheInitInfo       * aInfo,
    eCacheType            type )
  {
    switch ( type ) {
    case kBaselineCache:  return new CacheControllerImpl        ( aController, aInfo );
    case kPiranhaCache:   return new PiranhaCacheControllerImpl ( aController, aInfo );
    #ifdef FLEXUS_FCMP
      case kFCMPCache:      return new FCMPCacheControllerImpl    ( aController, aInfo );
    #endif //FLEXUS_FCMP
    default:
      DBG_ ( Crit, NoDefaultOps() ( << "Cannot construct cache: unknown cache type: " << (int)type ) );
      DBG_Assert ( false, NoDefaultOps()  );
    return NULL;
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // BaseCacheControllerImpl method definitions

  BaseCacheControllerImpl::BaseCacheControllerImpl ( BaseCacheController * aController,
                                                     CacheInitInfo       * aInit )
    : theController ( aController )
      , theArray ( aInit->theCacheSize,
                   aInit->theAssociativity,
                   aInit->theBlockSize,
                   /* CMU-ONLY-BLOCK-BEGIN */
                   //fixme
                   // another way to support R-NUCA
                   // (aInit->theCacheLevel == eL1 ? 1 : aInit->Slices), // slices are handled in the CacheController. This is for the cache index.
                   /* CMU-ONLY-BLOCK-END */
                   1, // slices are handled in the CacheController, index bits are shifted by msg->idxExtraOffsetBits
                   aInit->theSliceNumber,
                   aInit->theReplacementPolicy )
      , theEvictBuffer(aInit->theEBSize)
      , theInit(aInit)
      , theReadTracker( aInit->theName )
      , accesses(aInit->theName + "-Accesses")
      , accesses_user_I(aInit->theName + "-Accesses:User:I")
      , accesses_user_D(aInit->theName + "-Accesses:User:D")
      , accesses_system_I(aInit->theName + "-Accesses:System:I")
      , accesses_system_D(aInit->theName + "-Accesses:System:D")
      , requests(aInit->theName + "-Requests")
      , hits(aInit->theName + "-Hits")
      , hits_user_I(aInit->theName + "-Hits:User:I")
      , hits_user_D(aInit->theName + "-Hits:User:D")
      , hits_user_D_Read(aInit->theName + "-Hits:User:D:Read")
      , hits_user_D_Write(aInit->theName + "-Hits:User:D:Write")
      , hits_user_D_PrefetchRead(aInit->theName + "-Hits:User:D:PrefetchRead")
      , hits_user_D_PrefetchWrite(aInit->theName + "-Hits:User:D:PrefetchWrite")
      , hits_system_I(aInit->theName + "-Hits:System:I")
      , hits_system_D(aInit->theName + "-Hits:System:D")
      , hits_system_D_Read(aInit->theName + "-Hits:System:D:Read")
      , hits_system_D_Write(aInit->theName + "-Hits:System:D:Write")
      , hits_system_D_PrefetchRead(aInit->theName + "-Hits:System:D:PrefetchRead")
      , hits_system_D_PrefetchWrite(aInit->theName + "-Hits:System:D:PrefetchWrite")
      , hitsEvict(aInit->theName + "-HitsEvictBuf")
      , misses(aInit->theName + "-Misses")
      , misses_user_I(aInit->theName + "-Misses:User:I")
      , misses_user_D(aInit->theName + "-Misses:User:D")
      , misses_user_D_Read(aInit->theName + "-Misses:User:D:Read")
      , misses_user_D_Write(aInit->theName + "-Misses:User:D:Write")
      , misses_user_D_PrefetchRead(aInit->theName + "-Misses:User:D:PrefetchRead")
      , misses_user_D_PrefetchWrite(aInit->theName + "-Misses:User:D:PrefetchWrite")
      , misses_system_I(aInit->theName + "-Misses:System:I")
      , misses_system_D(aInit->theName + "-Misses:System:D")
      , misses_system_D_Read(aInit->theName + "-Misses:System:D:Read")
      , misses_system_D_Write(aInit->theName + "-Misses:System:D:Write")
      , misses_system_D_PrefetchRead(aInit->theName + "-Misses:System:D:PrefetchRead")
      , misses_system_D_PrefetchWrite(aInit->theName + "-Misses:System:D:PrefetchWrite")
      , misses_peerL1_system_I(aInit->theName + "-Misses:PeerL1:System:I")
      , misses_peerL1_system_D(aInit->theName + "-Misses:PeerL1:System:D")
      , misses_peerL1_user_I(aInit->theName + "-Misses:PeerL1:User:I")
      , misses_peerL1_user_D(aInit->theName + "-Misses:PeerL1:User:D")
      , upgrades(aInit->theName + "-Upgrades")
      , fills(aInit->theName + "-Fills")
      , upg_replies(aInit->theName + "-UpgradeReplies")
      , evicts_clean(aInit->theName + "-EvictsClean")
      , evicts_dirty(aInit->theName + "-EvictsDirty")
      , snoops(aInit->theName + "-Snoops")
      , purges(aInit->theName + "-Purges") /* CMU-ONLY */
      , probes(aInit->theName + "-Probes")
      , iprobes(aInit->theName + "-Iprobes")
      , lockedAccesses(aInit->theName + "-LockedAccesses")
      , tag_match_invalid(aInit->theName + "-TagMatchesInvalid")
      , prefetchReads(aInit->theName + "-PrefetchReads")
      , prefetchHitsRead(aInit->theName + "-PrefetchHitsRead")
      , prefetchHitsPrefetch(aInit->theName + "-PrefetchHitsPrefetch")
      , prefetchHitsWrite(aInit->theName + "-PrefetchHitsWrite")
      , prefetchHitsButUpgrade(aInit->theName + "-PrefetchHitsButUpgrade")
      , prefetchEvicts(aInit->theName + "-PrefetchEvicts")
      , prefetchInvals(aInit->theName + "-PrefetchInvals")
      , atomicPreloadWrites(aInit->theName + "-AtomicPreloadWrites")
  {}

  ///////////////////////////
  // State management

  void BaseCacheControllerImpl::saveState(std::string const & aDirName)
  {
    std::string fname( aDirName);
    fname += "/" + theInit->theName;
    std::ofstream ofs(fname.c_str());

    theArray.saveState ( ofs );

    ofs.close();

    fname = aDirName + "/" + theInit->theName + "-evictbuffer";
    std::ofstream ebofs(fname.c_str());
    theEvictBuffer.saveState(ebofs);
    ebofs.close();
  }

  void BaseCacheControllerImpl::loadState(std::string const & aDirName)
  {
    std::string fname( aDirName);
    fname += "/" + theInit->theName;

    std::ifstream ifs(fname.c_str());
    if (! ifs.good()) {
      DBG_(Dev, ( << theInit->theName << " saved checkpoint state " << fname << " not found.  Resetting to empty cache. " )  );
    } else {
      ifs >> std::skipws;

      if ( theArray.loadState ( ifs ) ) {
        DBG_ ( Dev, ( << theInit->theName 
                      << " Error loading checkpoint state from file: " << fname <<
                       ".  Make sure your checkpoints match your current cache configuration." ) );
        DBG_Assert ( false );
      }

      ifs.close();

      fname = aDirName + "/" + theInit->theName + "-evictbuffer";
      std::ifstream ebifs(fname.c_str());
      if (ebifs) {
        theEvictBuffer.loadState(ebifs);
        ebifs.close();
      }
    }
  }

  ///////////////////////////
  // Eviction Processing

  // This queues a block for eviction
  void BaseCacheControllerImpl::evictBlock(Block & aBlock, MemoryAddress aBlockAddress, bool evictable) {
    DBG_(Iface, Addr(aBlockAddress) ( << theInit->theName << " added " << aBlockAddress << " to evict buffer" ) );
      // copy block data to request from cache

    if ( aBlock.dirty() ) {
      theEvictBuffer.allocEntry(aBlockAddress, MemoryMessage::EvictDirty, evictable);
    } else if ( aBlock.modifiable()) {
      theEvictBuffer.allocEntry(aBlockAddress, MemoryMessage::EvictWritable, evictable);
    } else {
      theEvictBuffer.allocEntry(aBlockAddress, MemoryMessage::EvictClean, evictable);
    }

    if ( aBlock.prefetched() ) {
      prefetchEvicts++;
    }

    theTraceTracker.eviction(theInit->theNodeId, theInit->theCacheLevel, aBlockAddress, false);

    // mark as clean so we know if the block is dirtied again before
    // the writeback is acknowledged
    aBlock.setDirty ( false );

  }  // evictBlock()

  Action BaseCacheControllerImpl::doEviction() {
    Action action(kSend);
    MemoryMessage_p msg( theEvictBuffer.pop() );
    msg->reqSize() = theInit->theBlockSize;

    DBG_(Trace, Addr(msg->address()) ( << theInit->theName << " Queuing eviction " << *msg ) );

    if(msg->type() == MemoryMessage::EvictDirty) {
      evicts_dirty++;
    } else {
      evicts_clean++;
    }

    //theTraceTracker.eviction(theInit->theNodeId, theInit->theCacheLevel, msg->address(), false);

    //Create a transaction tracker for the eviction

    TransactionTracker_p tracker = new TransactionTracker;
    tracker->setAddress(msg->address());
    tracker->setInitiator(theInit->theNodeId);
    tracker->setSource(theInit->theName + " Evict");
    tracker->setDelayCause(theInit->theName, "Evict");

    action.addOutputMessage ( msg );
    action.theOutputTracker = tracker;
    action.theRequiresData = false;

    return action;
  }

  // Handles a Request process from the front side request channel
  // Returns MAF entry action, depending on what should be done to the request
  Action
  BaseCacheControllerImpl::handleRequestMessage ( MemoryMessage_p      msg,
                                                  TransactionTracker_p tracker,
                                                  bool                 has_maf_entry )
  {
    DBG_( Trace, Addr(msg->address()) ( << theInit->theName << " Handle Request: " << *msg) );

    // check invariants for handleRequestMessage
    DBG_Assert(msg);
    DBG_Assert( msg->isRequest() );
    DBG_Assert(! msg->usesSnoopChannel(), ( << *msg ) );
    accesses++;
    if (tracker && tracker->isFetch() && *tracker->isFetch()) {
      if (tracker->OS() && *tracker->OS()) {
        accesses_system_I++;
      } else {
        accesses_user_I++;
      }
    } else {
      if (tracker->OS() && *tracker->OS()) {
        accesses_system_D++;
      } else {
        accesses_user_D++;
      }
    }

    // if this is a prefetch read request, it should not get a MAF entry
    if(( msg->type() == MemoryMessage::PrefetchReadNoAllocReq || msg->type() == MemoryMessage::PrefetchReadAllocReq ) && has_maf_entry) {
      // consider a hit in the MAF to be similar to a hit in the
      // array - redundant for both
      msg->type() = MemoryMessage::PrefetchReadRedundant;
      return Action( kSend );
    }

    // this is an ordinary request - check if there are other transactions
    // outstanding for this address
    return examineRequest(msg, tracker, has_maf_entry);
  }  // handleRequestMessage

  Action
  BaseCacheControllerImpl::wakeMaf ( MemoryMessage_p     msg,
                                    TransactionTracker_p tracker,
                                    TransactionTracker_p aWakingTracker)
  {
    DBG_( Trace, Addr(msg->address()) ( << theInit->theName << " Wake MAF: " << *msg) );

    accesses++;
    if (tracker && tracker->isFetch() && *tracker->isFetch()) {
      if (tracker->OS() && *tracker->OS()) {
        accesses_system_I++;
      } else {
        accesses_user_I++;
      }
    } else {
      if (tracker->OS() && *tracker->OS()) {
        accesses_system_D++;
      } else {
        accesses_user_D++;
      }
    }

    if (tracker) {
      tracker->setBlockedInMAF(true);
    }

    return examineRequest(msg, tracker, false, aWakingTracker);
    //Possible return values:
    //ReplyAndRemoveMAF - this message is handled, process the next waiting MAF
    //kInsertMAF_WaitResponse - the request caused a miss.  Put it in WaitResponse state and stop waking.
  }  // wakeMaf()


  // Examine a request, given that the related set is not locked
  // - returns the action to be taken by the CacheController
  Action
  BaseCacheControllerImpl::examineRequest ( MemoryMessage_p      msg,
                                            TransactionTracker_p tracker,
                                            bool                 has_maf_entry,
                                            TransactionTracker_p aWakingTracker )
  {
    DBG_(Trace, Addr(msg->address()) ( << theInit->theName << " Examine Request " << *msg ) );
    if (tracker) {
      tracker->setDelayCause(theInit->theName, "Processing");
    }

    bool was_write = msg->isWrite();

    LookupResult result = theArray.lookup(msg->address(), (theInit->theCacheLevel == eL1) ? 0 : msg->idxExtraOffsetBits());
    requests++;

    // look the address up in the array
    if(result.hit()) {
      bool prefetched = result.block().prefetched();

      // possible cache hit -  may still miss if permissions are bad
      Action act = performOperation(msg, tracker, result, true /* hit */, false /* anyInvs */, has_maf_entry);
      if (act.theAction == kReplyAndRemoveMAF) {

        //TraceTracker hit
        if(tracker->originatorLevel() && (*tracker->originatorLevel() == eL2Prefetcher)) {
          // this should probably be a redundant prefetch
          //theTraceTracker.prefetch(theInit->theNodeId, theInit->theCacheLevel, msg->address());
        } else {
          theTraceTracker.access(theInit->theNodeId, theInit->theCacheLevel, msg->address(), msg->pc(), prefetched, was_write, false, msg->isPriv(),(tracker->logicalTimestamp() ? *tracker->logicalTimestamp() : 0 ));
        }

        DBG_(Trace, Addr(msg->address()) ( << theInit->theName << " Hit: " << *msg ) );
        hits++;
        if (tracker && tracker->isFetch() && *tracker->isFetch()) {
          if (tracker->OS() && *tracker->OS()) {
            hits_system_I++;
          } else {
            hits_user_I++;
          }
        } else if (msg->type() == MemoryMessage::PrefetchReadRedundant || (tracker->originatorLevel() && (*tracker->originatorLevel() == eL2Prefetcher))) {
          if (tracker->OS() && *tracker->OS()) {
            hits_system_D++;
            hits_system_D_PrefetchRead++;
          } else {
            hits_user_D++;
            hits_user_D_PrefetchRead++;
          }
        } else if (msg->type() == MemoryMessage::StorePrefetchReply ) {
          if (tracker->OS() && *tracker->OS()) {
            hits_system_D++;
            hits_system_D_PrefetchWrite++;
          } else {
            hits_user_D++;
            hits_user_D_PrefetchWrite++;
          }
        } else {
          if (was_write) {
            if (tracker->OS() && *tracker->OS()) {
              hits_system_D++;
              hits_system_D_Write++;
            } else {
              hits_user_D++;
              hits_user_D_Write++;
            }
          } else {
            if (tracker->OS() && *tracker->OS()) {
              hits_system_D++;
              hits_system_D_Read++;
            } else {
              hits_user_D++;
              hits_user_D_Read++;
            }
          }
        }

        if (tracker && aWakingTracker) {
          //This request is being serviced via MAF wakeup.  We should record
          //the transaction fill details from the original request into this
          //request, since both transaction had to wait on the same memory
          //system events
          if (aWakingTracker->networkTrafficRequired()) {
            tracker->setNetworkTrafficRequired(*aWakingTracker->networkTrafficRequired());
          }
          if (aWakingTracker->responder()) {
            tracker->setResponder(*aWakingTracker->responder());
          }
          if (aWakingTracker->fillLevel()) {
            if(aWakingTracker->originatorLevel() && (*aWakingTracker->originatorLevel() == eL2Prefetcher)) {
              tracker->setFillLevel(ePrefetchBuffer);
            } else {
              tracker->setFillLevel(*aWakingTracker->fillLevel());
            }
          }
          if (aWakingTracker->fillType()) {
            tracker->setFillType(*aWakingTracker->fillType());
          }
          if (aWakingTracker->previousState()) {
            tracker->setPreviousState(*aWakingTracker->previousState());
          }
        }
      }
      return act;
    } else {
      DBG_(Trace, Addr(msg->address()) ( << theInit->theName << " Miss: " << *msg ) );
      if(result.found()) {
        // tag match, but invalid
        tag_match_invalid++;
        if (theInit->theCacheLevel == eL2) {
          DBG_( VVerb, ( << "Tag Match invalid: " << *msg ) );
        }
      }
      return handleMiss( msg, tracker, result, has_maf_entry, theInit->theProbeOnIfetchMiss);
    }
  }  // examineRequest()




  // This initiates a snoop request.  As this original request travels
  // up the hierarchy, it interacts with the cache array, and marks this
  // transaction in the snoop buffer, possibly with dirty data from the array
  void
  BaseCacheControllerImpl::initializeSnoop ( MemoryMessage_p        msg,
                                             TransactionTracker_p   tracker,
                                             LookupResult                             & lookup )
  {

    // an entry needs to go into the snoop buffer... however, there may
    // already be an entry for this address.  If it is a probe entry, we
    // should clobber it here.  If it is a real snoop request type, then
    // there is no need to create a new one (also, in this case, the line
    // should not be dirty in the array)
    SnoopEntry_p snEntry = theSnoopBuffer.findEntry(msg->address());
    if(snEntry) {
      if(snEntry->message->isProbeType()) {
        // set the entry type to the appropriate snoop
        snEntry->message->type() = msg->type();
      }
      else {
        DBG_Assert(snEntry->message->isSnoopType());
        if (lookup.hit()) {
          DBG_Assert(!lookup.block().dirty(), ( << theInit->theName << " msg:" << *msg << " snEntry->message:" << *snEntry->message));
        }
      }
    }
    else {
      snEntry = theSnoopBuffer.allocEntry(new MemoryMessage(*msg));
      DBG_(Trace, Addr(msg->address()) ( << theInit->theName << " message creating entry in snoop buffer: " << *msg ) );
    }

    // perform the snoop with respect to the cache array
    if(lookup.hit()) {
      // found the line in the cache

      // check if there is dirty data to put aside
      if(lookup.block().dirty()) {
        // store this in the snoop buffer to be collected when the Ack
        // travels back down the hierarchy
        // TODO: copy data to snoop buffer message
        // mark the message as containing data
        snEntry->message->reqSize() = theInit->theBlockSize;
        DBG_(Trace, Addr(msg->address()) ( << theInit->theName << " reply will collect data for 0x" << &std::hex << msg->address() << " msg: " << *msg));
      }
      else {
        DBG_(Trace, Addr(msg->address()) ( << theInit->theName 
                                           << " reply hit in the array for " << &std::hex << msg->address()
                                           << " but the line is not dirty for msg: " << *msg));
      }

      if(lookup.block().prefetched()) {
        prefetchInvals++;
      }

      // use the access function to perform the side effects
      lookup.access(frontAccessTranslator::getAccessType(*msg));

      if( msg->type() == MemoryMessage::Invalidate ) {
        theTraceTracker.invalidTagCreate(theInit->theNodeId, theInit->theCacheLevel, msg->address());
      }

    } else {
      DBG_(Trace, Addr(msg->address()) ( << theInit->theName << " Snoop lookup was a miss: " << *msg ) );
    }

    // now perform the snoop with respect to the evict buffer
    if( msg->type() == MemoryMessage::Invalidate ) {

      theTraceTracker.invalidation(theInit->theNodeId, theInit->theCacheLevel, msg->address());

      // an invalidate should remove all evict types from the buffer
      EvictBuffer::iterator evEntry( theEvictBuffer.find(msg->address()) );
      if(evEntry != theEvictBuffer.end()) {
        switch(evEntry->type()) {
        case MemoryMessage::EvictDirty:
          // TODO: copy data to snoop entry from evict buffer entry
          snEntry->message->reqSize() = theInit->theBlockSize;
          DBG_(Trace, Addr(evEntry->address()) ( << theInit->theName 
                                                 << " EvictDirty processed in initialize snoop for " << *msg
                                                 << " evEntry: " << *evEntry));
          break;

        case MemoryMessage::EvictWritable:
        case MemoryMessage::EvictClean:

          // do nothing - these don't contain dirty data and the entry
          // will be removed from the buffer
          DBG_(Trace, Addr(evEntry->address()) ( << theInit->theName 
                                                 << " EvictClean/Writable processed in initialize snoop for " << *msg
                                                 << " evEntry: " << *evEntry));
          break;

        default:
          DBG_Assert(false, ( << theInit->theName << " Invalid message in evict buffer during snoop: " << *(evEntry)) );
        }
        theEvictBuffer.remove(evEntry);
      } //if evEntry != end
    } else if(msg->type() == MemoryMessage::Downgrade) {
      // a downgrade need only take away dirty data and modify permission
      EvictBuffer::iterator evEntry( theEvictBuffer.find(msg->address()) );
      if(evEntry != theEvictBuffer.end()) {
        switch(evEntry->type()) {

        case MemoryMessage::EvictDirty:
          // unmark the buffer entry as containing dirty data
          evEntry->type() = MemoryMessage::EvictClean;
          // TODO: copy data to snoop entry from evict buffer entry
          snEntry->message->reqSize() = theInit->theBlockSize;
          DBG_(Trace, Addr(evEntry->address()) ( << theInit->theName 
                                                 << " EvictDirty processed in initialize probe for " << *msg
                                                 << " evEntry: " << *evEntry));
          break;

        case MemoryMessage::EvictWritable:
        case MemoryMessage::EvictClean:
          // remove modify permission
          evEntry->type() = MemoryMessage::EvictClean;
          DBG_(Trace, Addr(evEntry->address()) ( << theInit->theName 
                                                 << " EvictClean/Writable processed in initialize probe for " << *msg
                                                 << " evEntry: " << *evEntry));
          break;

        default:
          DBG_Assert(false, ( << theInit->theName << " Invalid message in evict buffer during initialize snoop: " << *evEntry) );
        }
      }
    }
    else {
      // return req: hit and do nothing
      DBG_Assert(msg->type() == MemoryMessage::ReturnReq);
    }
  }  // initializeSnoop()

  // This initiates a probe request.  As this original request travels
  // up the hierarchy, it marks itself in the snoop buffer
  void
  BaseCacheControllerImpl::initializeProbe ( MemoryMessage_p        msg,
                                             TransactionTracker_p   tracker,
                                             LookupResult                             & lookup )
  {
    // an entry needs to go into the snoop buffer... however, if there is
    // already an entry for this address, there is no need to do anything now
    SnoopEntry_p snEntry = theSnoopBuffer.findEntry(msg->address());
    if(!snEntry) {
      theSnoopBuffer.allocEntry( new MemoryMessage(*msg) );
    }
  }  // initializeProbe()


  // This performs a probe.  The original request must have already passed
  // up the hierarchy and done nothing.  Now, on its way down, the array
  // is inspected and the probe response type changed as necessary.
  Action
  BaseCacheControllerImpl::finalizeProbe ( MemoryMessage_p        msg,
                                           TransactionTracker_p   tracker,
                                           LookupResult                             & lookup )
  {
    // look in the MAF for allocation requests
    std::list< MemoryMessage_p > messages = theController->getAllUncompletedMessages(msg->address());
    std::list< MemoryMessage_p >::iterator iter, end;

    for(iter = messages.begin(), end = messages.end(); iter != end; ++iter) {
      // handle different situations based on the MAF request type
      switch((*iter)->type()) {

      case MemoryMessage::LoadReq:
      case MemoryMessage::AtomicPreloadReq:
      case MemoryMessage::StoreReq:
      case MemoryMessage::StorePrefetchReq:
      case MemoryMessage::NonAllocatingStoreReq:
      case MemoryMessage::FetchReq:
      case MemoryMessage::RMWReq:
      case MemoryMessage::CmpxReq:
      case MemoryMessage::FlushReq:
      case MemoryMessage::ReadReq:
      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:
      case MemoryMessage::UpgradeReq:
      case MemoryMessage::UpgradeAllocate:
      case MemoryMessage::PrefetchReadNoAllocReq:
      case MemoryMessage::PrefetchReadAllocReq:
        // these requests contain no data, so they cannot affect the
        // probe result
        break;

      case MemoryMessage::Flush:
      case MemoryMessage::EvictDirty:
      case MemoryMessage::PurgeIReq: // nikos: may contain dirty data /* CMU-ONLY */
      case MemoryMessage::PurgeDReq: // nikos: may contain dirty data /* CMU-ONLY */
        // these contain dirty data
        msg->type() = MemoryMessage::ProbedDirty;
        break;

      case MemoryMessage::EvictWritable:
      case MemoryMessage::PrefetchInsertWritable:
        // these contain clean data with modify permission
        msg->type() = MemoryMessage::maxProbe(MemoryMessage::ProbedWritable, msg->type());
        break;

      case MemoryMessage::EvictClean:
      case MemoryMessage::PrefetchInsert:
        // these contain clean data (non-modifiable)
        msg->type() = MemoryMessage::maxProbe(MemoryMessage::ProbedClean, msg->type());
        break;

      default:
        DBG_Assert(false, ( << theInit->theName << " Invalid message in MAF during probe: " << *(iter) ));
      }
    }

    // look in the array
    if(lookup.hit()) {
      if(lookup.block().dirty()) {
        msg->type() = MemoryMessage::ProbedDirty;
      }
      else if(lookup.block().modifiable()) {
        msg->type() = MemoryMessage::maxProbe(MemoryMessage::ProbedWritable, msg->type());
      }
      else {
        msg->type() = MemoryMessage::maxProbe(MemoryMessage::ProbedClean, msg->type());
      }
    }

    // look in the evict buffer
    EvictBuffer::iterator evEntry( theEvictBuffer.find(msg->address()) );
    if(evEntry != theEvictBuffer.end()) {
      switch(evEntry->type()) {
        // "upgrade" the probe response if necessary

      case MemoryMessage::EvictDirty:
        msg->type() = MemoryMessage::ProbedDirty;
        break;

      case MemoryMessage::EvictWritable:
        msg->type() = MemoryMessage::maxProbe(MemoryMessage::ProbedWritable, msg->type());
        break;

      case MemoryMessage::EvictClean:
        msg->type() = MemoryMessage::maxProbe(MemoryMessage::ProbedClean, msg->type());
        break;

      default:
        DBG_Assert(false, ( << theInit->theName << " Invalid message in evict buffer during probe: " << *evEntry) );
      }
    }

    // finally look through the snoop buffer
    SnoopEntry_p snEntry( theSnoopBuffer.findEntry(msg->address()) );
    if(snEntry) {
      // handle different situations based on the buffer message type
      switch(snEntry->message->type()) {

      case MemoryMessage::Probe:
        // no invalidates or downgrades came through since the probe began
        msg->type() = MemoryMessage::maxProbe(snEntry->state, msg->type());
        // now remove the entry
        theSnoopBuffer.removeEntry(snEntry);
        break;

      case MemoryMessage::Invalidate:
      case MemoryMessage::Downgrade:
      case MemoryMessage::ReturnReq: 
        if(snEntry->message->reqSize() > 0) {
          // the entry contains dirty data, so "upgrade" the probe response
          msg->type() = MemoryMessage::ProbedDirty;
        }
        // now update the probe response based on the probe state
        msg->type() = MemoryMessage::maxProbe(snEntry->state, msg->type());
        // ... and reset the state
        snEntry->state = MemoryMessage::ProbedNotPresent;
        break;

      default:
        DBG_Assert(false, ( << theInit->theName << " Invalid message in snoop buffer during snoop: " << *(snEntry->message)) );
      }
    }

    // pass the message along
    return Action(kSend);
  }  // finalizeProbe()

  // This completes a snoop request.  The original request must have
  // already passed up the memory hierarchy and performed upon the cache
  // array.  As this Ack now travels back down the hierarchy, it interacts
  // with the MAF and the snoop and evict buffers
  Action
  BaseCacheControllerImpl::finalizeSnoop ( MemoryMessage_p        msg,
                                           TransactionTracker_p   tracker,
                                           LookupResult                             & lookup )
  {
    Action action(kSend);

    // perform the snoop with respect to the MAF - there may be more than
    // one entry that matches this address - handle them all
    std::list< MemoryMessage_p > messages = theController->getAllUncompletedMessages(msg->address());
    std::list< MemoryMessage_p >::iterator iter, end;

    for(iter = messages.begin(), end = messages.end(); iter != end; ++iter) {
      // handle different situations based on the MAF request type
      switch ((*iter)->type()) {
      case MemoryMessage::LoadReq:
      case MemoryMessage::AtomicPreloadReq:
      case MemoryMessage::StoreReq:
      case MemoryMessage::StorePrefetchReq:
      case MemoryMessage::NonAllocatingStoreReq:
      case MemoryMessage::FetchReq:
      case MemoryMessage::RMWReq:
      case MemoryMessage::CmpxReq:
      case MemoryMessage::ReadReq:
      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:
      case MemoryMessage::PrefetchReadNoAllocReq:
      case MemoryMessage::PrefetchReadAllocReq:
      case MemoryMessage::PurgeDReq: /* CMU-ONLY */
      case MemoryMessage::PurgeIReq: /* CMU-ONLY */
        // no changes required for these requests
        break;

      case MemoryMessage::UpgradeReq:
        // if the snoop is an invalidation, convert this upgrade (no data)
        // to a write (no data)
        if(    (msg->type() == MemoryMessage::InvalidateAck)
            || (msg->type() == MemoryMessage::InvUpdateAck)
            || (msg->type() == MemoryMessage::PurgeAck) /* CMU-ONLY */
          )
        {
          (*iter)->type() = MemoryMessage::WriteReq;
        }
        break;
      case MemoryMessage::UpgradeAllocate:
        // if the snoop is an invalidation, convert this upgrade (with data)
        // to a write (with data)
        if(    (msg->type() == MemoryMessage::InvalidateAck)
            || (msg->type() == MemoryMessage::InvUpdateAck)
            || (msg->type() == MemoryMessage::PurgeAck) /* CMU-ONLY */
          )
        {
          (*iter)->type() = MemoryMessage::WriteAllocate;
        }
        break;

      case MemoryMessage::Flush:
      case MemoryMessage::EvictDirty:
      case MemoryMessage::FlushReq:
      case MemoryMessage::EvictWritable:
      case MemoryMessage::EvictClean:
      case MemoryMessage::PrefetchInsert:
      case MemoryMessage::PrefetchInsertWritable:
      default:
        DBG_Assert(false, ( << theInit->theName << " Invalid message in MAF during snoop: " << **iter ) );
      }
    }

    // if the line exists in the array, it may need to be updated
    if(lookup.hit()) {
      // update the cache data for a downgrade if necessary
      if(msg->type() == MemoryMessage::DownUpdateAck) {
        // TODO: copy data to cache
        action.theRequiresData = true;
      }
    }

    // now perform the snoop with respect to the evict buffer
    if(    (msg->type() == MemoryMessage::InvalidateAck)
        || (msg->type() == MemoryMessage::InvUpdateAck)
        || (msg->type() == MemoryMessage::PurgeAck) /* CMU-ONLY */
      )
    {

      theTraceTracker.invalidAck(theInit->theNodeId, theInit->theCacheLevel, msg->address());

      // an invalidate should have already removed any evict entries
      EvictBuffer::iterator evEntry( theEvictBuffer.find(msg->address()) );
      DBG_Assert(evEntry == theEvictBuffer.end(),
                 ( << theInit->theName 
                   << " Evict entry found in finalize snoop for " << *msg
                   << " evEntry: " << *evEntry) );
    }
    else {
      // a downgrade may need to update data in the evict buffer
      EvictBuffer::iterator evEntry( theEvictBuffer.find(msg->address()) );
      if(evEntry != theEvictBuffer.end()) {
        switch(evEntry->type()) {

        case MemoryMessage::EvictDirty:
        case MemoryMessage::EvictWritable:
          DBG_Assert(false, ( << theInit->theName 
                              << " EvictDirty/Writable found in finalize snoop for " << *msg
                              << " evEntry: " << *evEntry) );
          break;

        case MemoryMessage::EvictClean:
          if(msg->type() == MemoryMessage::DownUpdateAck) {
            // TODO: copy data to evict buffer entry
          }
          DBG_(Trace, Addr(evEntry->address()) ( << theInit->theName 
                                                 << " EvictClean processed in finalize snoop for " << *msg
                                                 << " evEntry: " << *evEntry));
          break;

        default:
          DBG_Assert(false, ( << theInit->theName << " Invalid message in evict buffer during snoop: " << *evEntry) );
        }
      }
    }

    // finally look through the snoop buffer
    SnoopEntry_p snEntry( theSnoopBuffer.findEntry(msg->address()) );
    if(snEntry) {
      // handle different situations based on the buffer message type
      switch(snEntry->message->type()) {

      case MemoryMessage::Probe:
        // don't remove the entry, but just make sure the probe state isn't
        // set to something it shouldn't be
        DBG_Assert( (snEntry->state == MemoryMessage::ProbedNotPresent) ||
                    (snEntry->state == MemoryMessage::ProbedClean) );

        // clean up for invalidations (it's unclear if this is necessary)
        if(    (msg->type() == MemoryMessage::InvalidateAck)
            || (msg->type() == MemoryMessage::InvUpdateAck)
            || (msg->type() == MemoryMessage::PurgeAck) /* CMU-ONLY */
          )
        {
          snEntry->state = MemoryMessage::ProbedNotPresent;
        }
        break;

      case MemoryMessage::Invalidate:
      case MemoryMessage::Downgrade:
      case MemoryMessage::ReturnReq:
        if(snEntry->message->reqSize() > 0) {
          // TODO: copy data to message from snoop buffer entry
          // "upgrade" the message type if necessary
          if(msg->type() == MemoryMessage::InvalidateAck) {
            DBG_(Trace, Addr(msg->address()) ( << theInit->theName 
                                               << " Invalidate ack changed to invalidate update ack for " << *msg
                                               << " snEntry: " << *(snEntry->message)));
            msg->type() = MemoryMessage::InvUpdateAck;
          }
          if(msg->type() == MemoryMessage::DowngradeAck) {
            msg->type() = MemoryMessage::DownUpdateAck;
            DBG_(Trace, Addr(msg->address()) ( << theInit->theName 
                                               << " Downgrade ack changed to downgrade update ack for " << *msg
                                               << " snEntry: " << *(snEntry->message)));
          }
          if(msg->type() == MemoryMessage::ReturnReply) {
            DBG_(Trace, Addr(msg->address()) ( << theInit->theName 
                                               << " ReturnReply ack remained the same for " << *msg
                                               << " snEntry: " << *(snEntry->message)));
          }
          /* CMU-ONLY-BLOCK-BEGIN */
          if(msg->type() == MemoryMessage::PurgeAck) {
            DBG_(Trace, Addr(msg->address()) ( << theInit->theName 
                                               << " Purge ack remained the same for " << *msg
                                               << " snEntry: " << *(snEntry->message)));
          }
          /* CMU-ONLY-BLOCK-END */
          if(tracker) {
            tracker->setPreviousState(eModified);
          }
        }
        // preserve snoop core and L2 node numbers
        /* CMU-ONLY-BLOCK-BEGIN */
        if (msg->type() == MemoryMessage::PurgeAck) {
          msg->coreNumber() = snEntry->message->coreNumber();
          msg->l2Number() = snEntry->message->l2Number();
        }
        /* CMU-ONLY-BLOCK-END */
        DBG_(Trace, Addr(msg->address()) ( << theInit->theName 
                                           << " Downgrade/Invalidate ack processed in finalize probe for " << *msg
                                           << " snEntry: " << *(snEntry->message)));
        theSnoopBuffer.removeEntry(snEntry);
        break;

      default:
        DBG_Assert(false, ( << theInit->theName << " Invalid message in snoop buffer during snoop: " << *(snEntry->message)) );
      }
    }

    // send the message on its way
    return action;
  }  // finalizeSnoop()


  // END BaseCacheControllerImpl method definitions
  ////////////////////////////////////////////////////////////////////////////////

}; // namespace nCache
