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

#include <stdlib.h> // for random()

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <core/debug/debug.hpp>
  #define DBG_DeclareCategories Cache
  #define DBG_SetDefaultOps AddCat(Cache)
  #include DBG_Control()

#include "PiranhaCacheControllerImpl.hpp"

namespace nCache {

  // Utility functions

  // Get an appropriate reply for a given message.  Be careful with
  // messages that can return multiple replies (i.e., probes, downgrades),
  // because the response messages may be different.
  const MemoryMessage::MemoryMessageType
  memoryReply
  ( MemoryMessage::MemoryMessageType req )
  {
    switch ( req )
      {
      case MemoryMessage::LoadReq:                 return MemoryMessage::LoadReply;
      case MemoryMessage::ReadReq:                 return MemoryMessage::MissReply;
      case MemoryMessage::StreamFetch:             return MemoryMessage::StreamFetchWritableReply;
      case MemoryMessage::FetchReq:                return MemoryMessage::FetchReply;
      case MemoryMessage::StoreReq:                return MemoryMessage::StoreReply;
      case MemoryMessage::StorePrefetchReq:        return MemoryMessage::StorePrefetchReply;
      case MemoryMessage::NonAllocatingStoreReq:   return MemoryMessage::NonAllocatingStoreReply;
      case MemoryMessage::RMWReq:                  return MemoryMessage::RMWReply;
      case MemoryMessage::CmpxReq:                 return MemoryMessage::CmpxReply;
      case MemoryMessage::AtomicPreloadReq:        return MemoryMessage::AtomicPreloadReply;
      case MemoryMessage::WriteReq:                return MemoryMessage::MissReplyWritable;
      case MemoryMessage::WriteAllocate:           return MemoryMessage::MissReplyWritable;
      case MemoryMessage::UpgradeReq:              return MemoryMessage::UpgradeReply;
      case MemoryMessage::UpgradeAllocate:         return MemoryMessage::UpgradeReply;
      case MemoryMessage::Downgrade:               return MemoryMessage::DowngradeAck;
      case MemoryMessage::Invalidate:              return MemoryMessage::InvalidateAck;
      case MemoryMessage::Probe:                   return MemoryMessage::ProbedNotPresent;
      case MemoryMessage::ReturnReq:               return MemoryMessage::ReturnReply;
      case MemoryMessage::PrefetchReadNoAllocReq:  return MemoryMessage::PrefetchReadReply;
      case MemoryMessage::PurgeIReq:               return MemoryMessage::PurgeAck;           /* CMU-ONLY */
      case MemoryMessage::PurgeDReq:               return MemoryMessage::PurgeAck;           /* CMU-ONLY */
      default:
        {
          MemoryMessage_p msg = new MemoryMessage ( req );
          DBG_(Crit, ( << " memoryReply received unknown memory request type: " <<  *msg ) );

          DBG_Assert ( 0 );
        }
      };

    return MemoryMessage::CmpxReq;
  }

  ////////////////////////////////////////////////
  // Piranha Directory utility functions
  std::ostream & operator << ( std::ostream & s, PiranhaDirState const & state )
  {
    const char * state_names[] =
      {
      "D_I",
      "D_M",
      "D_O",
      "D_S",
      "D_MMW",
      "D_MMU",
      "D_S2MW",
      "D_S2MU",
      "D_SFWD",
      "D_M2O",
      "D_ExtInvalidation",
      "D_ExtDowngrade",
      "D_GetExtShared",
      "D_GetExtModified",
      "D_GetExtModifiedInvalidationPending",
      "D_EvictWait",
      "D_IPR",
      "D_IPW"
      };
    return s << "PiranhaDirState[" << state_names[state] << "]";
  }

  std::ostream & operator << ( std::ostream & s, PiranhaDirEntry const & dirEntry )
  {
    return s << " PDEntry: 0x" << std::hex << dirEntry.address() << std::dec
             <<  " dir state: " << dirEntry.state() << " #sharers: "
             << dirEntry.sharersCount() << std::hex << " sharersList: "
             << dirEntry.sharersList() << std::dec << " Owner: " << dirEntry.owner()
             << " acount: " << dirEntry.aCount();
  }


  ////////////////////////////////////////////////
  // PiranhaCacheControllerImpl member functions

  PiranhaCacheControllerImpl::PiranhaCacheControllerImpl
  ( BaseCacheController * aController,
    CacheInitInfo       * aInit )
    : BaseCacheControllerImpl ( aController,
                                aInit )
  {
    theDir = new PiranhaDir ( aInit->theBlockSize );

    /* CMU-ONLY-BLOCK-BEGIN */
    theASR_ReplicationThresholds[0] = 0; // these levels are 1 higher than in the paper to avoid corner cases
    theASR_ReplicationThresholds[1] = 0;
    theASR_ReplicationThresholds[2] = 4;
    theASR_ReplicationThresholds[3] = 16;
    theASR_ReplicationThresholds[4] = 64;
    theASR_ReplicationThresholds[5] = 128;
    theASR_ReplicationThresholds[6] = 256;
    theASR_ReplicationThresholds[7] = 256;
    /* CMU-ONLY-BLOCK-END */

    // 2D torus connectivity (for steering ReturnReq)
    theNumCoresPerTorusRow = 4; // fixme cfg.NumCoresPerTorusRow;
    theNTileID.reserve(aInit->theCores);
    int currID = aInit->theCores - theNumCoresPerTorusRow;
    for (int i=0; i < aInit->theCores; i++) {
      theNTileID [i] = currID;
      currID ++;
      if (currID == aInit->theCores) {
        currID = 0;
      }
    }
    theSTileID.reserve(aInit->theCores);
    currID = theNumCoresPerTorusRow;
    for (int i=0; i < aInit->theCores; i++) {
      theSTileID [i] = currID;
      currID ++;
      if (currID == aInit->theCores) {
        currID = 0;
      }
    }
    theWTileID.reserve(aInit->theCores);
    for (unsigned int row=0; row < aInit->theCores/theNumCoresPerTorusRow;  row++) {
      unsigned int currID = (row + 1) * theNumCoresPerTorusRow - 1;
      for (unsigned int i=0; i < theNumCoresPerTorusRow; i++) {
        theWTileID [row * theNumCoresPerTorusRow + i] = currID;
        currID ++;
        if (currID == (row + 1) * theNumCoresPerTorusRow) {
          currID -= theNumCoresPerTorusRow;
        }
      }
    }
    theETileID.reserve(aInit->theCores);
    for (unsigned int row=0; row < aInit->theCores/theNumCoresPerTorusRow;  row++) {
      unsigned int currID = row * theNumCoresPerTorusRow + 1;
      for (unsigned int i=0; i < theNumCoresPerTorusRow; i++) {
        theETileID [row * theNumCoresPerTorusRow + i] = currID;
        currID ++;
        if (currID == (row + 1) * theNumCoresPerTorusRow) {
          currID -= theNumCoresPerTorusRow;
        }
      }
    }
    // debugging
    for (unsigned int i=0; i < static_cast<unsigned int>( aInit->theCores ); i++) {
      DBG_Assert(i == theSTileID[theNTileID[i]]);
      DBG_Assert(i == theETileID[theWTileID[i]]);
    }

  }


  tFillType
  PiranhaCacheControllerImpl::pageStateToFillType
  ( const tPageState aPageState )
    const
  {
    switch ( aPageState ) {
      case kPageStateInvalid: return eFetch;
      case kPageStatePrivate: return eDataPrivate;
      case kPageStateShared:  return eDataSharedRW; // fixme: for now, assume all shared data are RW
      default:                DBG_Assert( false ); return eFetch; // compiler happy
    }
  }

  Action
  PiranhaCacheControllerImpl::performOperation
  ( MemoryMessage_p        msg,
    TransactionTracker_p   tracker,
    LookupResult                             & l2Lookup,
    bool                                       wasHit__,
    bool                                       anyInvs,
    bool                                       blockAddressOnMiss )
  {
    PiranhaDirEntry_p
      dirEntry = theDir->lookup ( msg->address() );

    bool
      inEvictBuffer = ( theEvictBuffer.find ( msg->address() ) != theEvictBuffer.end() );

    Action
      action ( kNoAction, tracker );

    DBG_Assert  ( dirEntry );

    // fixup transaction tracker fillLevel
    const bool wasExtReqMsg = ( msg->type()==MemoryMessage::ReturnReq || msg->type()==MemoryMessage::Invalidate || msg->type()==MemoryMessage::Downgrade );

    DBG_ ( Trace,
           Addr(msg->address()) ( << theInit->theName
                                  << " Perform op " << *msg
                                  << " dir_p: " << dirEntry
                                  << " dir state: " << dirEntry->state()
                                  << " #sharers: " << dirEntry->sharersCount()
                                  << std::hex << " sharersList: " << dirEntry->sharersList() << std::dec
                                  << " Owner: " << dirEntry->owner()
                                  << " acount: " << dirEntry->aCount()
                                ) );

    switch ( dirEntry->state() ) {
    case D_I:                handle_D_I          ( msg, action, l2Lookup, dirEntry, inEvictBuffer );   break;
    case D_M:                handle_D_M          ( msg, action, l2Lookup, dirEntry, inEvictBuffer );   break;
    case D_O:                handle_D_O          ( msg, action, l2Lookup, dirEntry, inEvictBuffer );   break;
    case D_S:                handle_D_S          ( msg, action, l2Lookup, dirEntry, inEvictBuffer );   break;
    case D_IPR:
    case D_IPW:
    case D_MMW:
    case D_MMU:
    case D_S2MW:
    case D_S2MU:
    case D_SFWD:
    case D_M2O:
    case D_EvictWait:        handle_D_Transient  ( msg, action, l2Lookup, dirEntry, inEvictBuffer ); break;
    case D_ExtInvalidation:
    case D_ExtDowngrade:     handle_D_ExtMessage ( msg, action, l2Lookup, dirEntry, inEvictBuffer ); break;
    case D_GetExtShared:
    case D_GetExtModified:   handle_D_ExtGet     ( msg, action, l2Lookup, dirEntry, inEvictBuffer ); break;
    case D_GetExtModifiedInvalidationPending:  DBG_ ( Iface,
                                                      Addr(msg->address()) ( << theInit->theName
                                                                             << " handler for " << dirEntry->state()
                                                                             << " not implemented yet"
                                                    ));
                                               break;
    default:
      DBG_Assert ( false, ( << theInit->theName << " Unrecognized directory state: " << dirEntry->state() << " msg: " << *msg ) );
    };

    // fixme: assumes the directory interleaving for private-nuca is the cache line size
    int ChipWideDirID = ((msg->address() >> theInit->theBlockSizeLog2) % theInit->theSlices);
    if ((theInit->thePlacement == kPrivateCache) && (ChipWideDirID == theInit->theNodeId)) {
      action.theRequiresDuplicateTag++; // extra latency for the private-nuca directory
    }

    DBG_ ( Trace, Addr(msg->address()) ( << theInit->theName << " Completing processing by telling MAF to: " << action.theAction << " for " << *msg ) );

    // update the memory transaction
    switch(msg->type()) {
    case MemoryMessage::LoadReply:
    case MemoryMessage::StoreReply:
    case MemoryMessage::StorePrefetchReply:
    case MemoryMessage::FetchReply:
    case MemoryMessage::NonAllocatingStoreReply:
    case MemoryMessage::RMWReply:
    case MemoryMessage::CmpxReply:
    case MemoryMessage::AtomicPreloadReply:
    case MemoryMessage::MissReply:
    case MemoryMessage::MissReplyWritable:
    case MemoryMessage::MissReplyDirty:
    case MemoryMessage::UpgradeReply:
    case MemoryMessage::PrefetchReadReply:
    case MemoryMessage::PrefetchWritableReply:
    case MemoryMessage::StreamFetchWritableReply:
    case MemoryMessage::PrefetchDirtyReply:
    case MemoryMessage::PrefetchReadRedundant:

      // If we haven't already set this fill, set it now
      tFillType fillType;
      // fixme
      // if ((theInit->thePlacement == kPrivateCache) && ((msg->coreIdx() >> 1) != theInit->theNodeId)) { // peerL2 (in private) is a coherence fill
      //   fillType = eCoherence; 
      // }
      if (msg->type() == MemoryMessage::FetchReply) {
        fillType = eFetch;
      }
      else if (tracker && tracker->fillType()) {
        fillType = *tracker->fillType();
      }
      else {
        fillType = pageStateToFillType( msg->pageState() );
      }
      updateStatsHitLocal ( tracker
                            , fillType
                          );
      break;
    default:
      // do nothing
      break;
    }

    // fixup transaction tracker fillLevel
    if (wasExtReqMsg && action.theAction==kReplyAndRemoveMAF) {
      tFillType fillType;
      if (msg->type() == MemoryMessage::FetchReply) {
        fillType = eFetch;
      }
      else if (tracker && tracker->fillType()) {
        fillType = *tracker->fillType();
      }
      else {
        fillType = pageStateToFillType( msg->pageState() );
      }
      overwriteStatsHitLocal(tracker, fillType);
    }

    return action;
  }


  void
  PiranhaCacheControllerImpl::handle_D_I ( MemoryMessage_p        msg,
                                           Action               & action,
                                           LookupResult         & l2Lookup,
                                           PiranhaDirEntry_p      dirEntry,
                                           bool                   inEvictBuffer )
  {
    action.theAction = kNoAction;
    action.theRequiresDuplicateTag++;

    DBG_ ( Trace, Addr(msg->address()) ( << theInit->theName << " handleD_I received: " << *msg ) );

    intrusive_ptr<MemoryMessage> request;

    DBG_Assert ( !l2Lookup.hit(), ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );

    switch ( msg->type() ) {
 
    case MemoryMessage::StreamFetch:
      if (! theInit->theAllowOffChipStreamFetch) {      
        msg->type() = MemoryMessage::StreamFetchRejected;
        action.theAction = kReplyAndRemoveMAF;
        action.theRequiresTag++;
        break;
      } // else fall through to next case...        

    case MemoryMessage::AtomicPreloadReq:
    case MemoryMessage::LoadReq:
    case MemoryMessage::ReadReq:
    case MemoryMessage::FetchReq:
    case MemoryMessage::PrefetchReadNoAllocReq:
      if ( !inEvictBuffer ) {
        MemoryMessage::MemoryMessageType req(MemoryMessage::ReadReq);
        if(msg->type() == MemoryMessage::FetchReq) {
          req = MemoryMessage::FetchReq;
        } else if (msg->type() == MemoryMessage::StreamFetch) {
          req = MemoryMessage::StreamFetch;          
        } else if (msg->type() == MemoryMessage::PrefetchReadNoAllocReq ) {
          req = MemoryMessage::PrefetchReadNoAllocReq;          
        }
        setupExternalRequest ( msg, req, action, dirEntry, D_GetExtShared, D_S );
        theDir->insert ( dirEntry );
        action.theAction = kInsertMAF_WaitResponse;
      } else {
        resurrectEvictedBlock ( msg, l2Lookup, dirEntry );

        msg->type() = memoryReply ( msg->type() );
        msg->reqSize() = theInit->theBlockSize;
        dirEntry->addSharer ( msg->coreIdx() );
        dirEntry->setOwner ( msg->coreIdx() );

        action.theAction = kReplyAndRemoveMAF;
      }
      break;

    case MemoryMessage::NonAllocatingStoreReq:
      if ( !inEvictBuffer ) {
        action.theAction = kSend;
      } else {
        resurrectEvictedBlock ( msg, l2Lookup, dirEntry );

        if ( dirEntry->state() == D_S ) {
          // Request the modifiable permissions for the block by running
          // the protocol around again, with the Shared permissions this time.
          action = handleBackMessage ( msg, action.theOutputTracker );
        } else {
          // Respond to the request
          msg->type() = memoryReply ( msg->type() );
          msg->reqSize() = theInit->theBlockSize;
          dirEntry->addSharer ( msg->coreIdx() );
          dirEntry->setOwner ( msg->coreIdx() );
          action.theAction = kReplyAndRemoveMAF;
        }
      }
      break;

    case MemoryMessage::NonAllocatingStoreReply:
      action.theAction = kSend;
      break;

    case MemoryMessage::StoreReq:
    case MemoryMessage::StorePrefetchReq:
    case MemoryMessage::RMWReq:
    case MemoryMessage::CmpxReq:
    case MemoryMessage::WriteReq:
    case MemoryMessage::WriteAllocate:
    case MemoryMessage::UpgradeReq:
    case MemoryMessage::UpgradeAllocate:
      if ( !inEvictBuffer ) {
        setupExternalRequest ( msg, MemoryMessage::WriteReq, action, dirEntry, D_GetExtModified, D_M );
        theDir->insert ( dirEntry );
        action.theAction = kInsertMAF_WaitResponse;
      } else {
        resurrectEvictedBlock ( msg, l2Lookup, dirEntry );

        if ( dirEntry->state() == D_S ) {
          // Request the modifiable permissions for the block by running
          // the protocol around again, with the Shared permissions this time.
          action = handleBackMessage ( msg, action.theOutputTracker );
        } else {
          // Respond to the request
          msg->type() = memoryReply ( msg->type() );
          msg->reqSize() = theInit->theBlockSize;
          dirEntry->addSharer ( msg->coreIdx() );
          dirEntry->setOwner ( msg->coreIdx() );
          action.theAction = kReplyAndRemoveMAF;
        }
      }
      break;

    case MemoryMessage::Invalidate:
    {
      DBG_Assert ( !l2Lookup.hit(), ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );
      DBG_Assert ( dirEntry->sharersCount() == 0, ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );

      if ( inEvictBuffer ) {
        EvictBuffer::iterator evEntry ( theEvictBuffer.find ( msg->address() ) );
        hitsEvict++;

        switch ( evEntry->type() ) {
          case MemoryMessage::EvictClean:    msg->type() = MemoryMessage::InvalidateAck; break;
          case MemoryMessage::EvictWritable:
          case MemoryMessage::EvictDirty:    msg->type() = MemoryMessage::InvUpdateAck; msg->reqSize() = theInit->theBlockSize; break;
          default:                           DBG_Assert( false );
        }

        DBG_(Trace,
             Addr(msg->address())
             ( << theInit->theName << " in D_I with an invalidate -- removing EvictBuf entry: " << *msg));

        theEvictBuffer.remove( evEntry );
        dirEntry->evictionQueued() = false;
        inEvictBuffer = false;
      }
      else {
        msg->type() = memoryReply ( msg->type() );
      }

      DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " in D_I with an invalidate -- sending reply: " << *msg));
      action.theAction = kReplyAndRemoveMAF;
      break;
    }

    case MemoryMessage::Downgrade:
    {
      DBG_Assert ( !l2Lookup.hit(), ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );
      DBG_Assert ( dirEntry->sharersCount() == 0, ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );

      if ( inEvictBuffer ) {
        DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " in D_I with a downgrade -- changing EvictBuf entry to EvictClean: " << *msg));
        hitsEvict++;

        EvictBuffer::iterator evEntry ( theEvictBuffer.find ( msg->address() ) );
        evEntry->type() = MemoryMessage::EvictClean;

        msg->type() = MemoryMessage::DownUpdateAck;
        msg->reqSize() = theInit->theBlockSize;
      }
      else {
        msg->type() = memoryReply ( msg->type() );
      }

      DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " in D_I with a downgrade -- sending reply: " << *msg));
      action.theAction = kReplyAndRemoveMAF;
      break;
    }

    case MemoryMessage::ReturnReq:
    {
      DBG_Assert ( !l2Lookup.hit(), ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );
      DBG_Assert ( dirEntry->sharersCount() == 0, ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );

      if ( inEvictBuffer ) {
        hitsEvict++;
        msg->reqSize() = theInit->theBlockSize;
      }

      msg->type() = memoryReply ( msg->type() );
      DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " in D_I with a ReturnReq -- sending reply: " << *msg));
      action.theAction = kReplyAndRemoveMAF;
      break;
    }

    case MemoryMessage::Probe:
    {
      DBG_Assert ( !l2Lookup.hit(), ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );
      DBG_Assert ( dirEntry->sharersCount() == 0, ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );

      DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " in D_I with a Probe -- preparing reply: " << *msg));

      msg->type() = memoryReply ( msg->type() );
      if ( inEvictBuffer ) {
        hitsEvict++;
        EvictBuffer::iterator evEntry ( theEvictBuffer.find ( msg->address() ) );
        switch ( evEntry->type() ) {
          case MemoryMessage::EvictClean:    msg->type() = MemoryMessage::ProbedClean; break;
          case MemoryMessage::EvictWritable: msg->type() = MemoryMessage::ProbedWritable; break;
          case MemoryMessage::EvictDirty:    msg->type() = MemoryMessage::ProbedDirty; break;
          default:                           DBG_Assert( false );
        }
      }

      DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " in D_I with a Probe -- sending reply: " << *msg));
      action.theAction = kReplyAndRemoveMAF;
      break;
    }

    /* CMU-ONLY-BLOCK-BEGIN */
    case MemoryMessage::PurgeIReq:
    case MemoryMessage::PurgeDReq:
      DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " in D_I with a purge: " << *msg));

      DBG_Assert ( !l2Lookup.hit(), ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );
      DBG_Assert ( dirEntry->sharersCount() == 0, ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );

      action.theRequiresDuplicateTag--; //purges are issued only to the owner node for private lines, so the DupTags are not used.
      msg->type() = memoryReply ( msg->type() );  // FIXME: this may break localengine -- assumes original msg type isn't changed
      action.theAction = kReplyAndRemoveMAF;
      break;
    /* CMU-ONLY-BLOCK-END */

    case MemoryMessage::EvictClean:
      // A request raced with an evict and won the race,
      // so an L1 back-to-back sequence of events of the form
      //                   evict - Rd request - evict
      // is seen by L2 as
      //                   Rd request - evict - evict.
      // The second evict is seen as an eviction in D_I which is illegal.
      // For now, just drop the message as this is a rare event. YUCK!
      //fixme
      action.theAction = kNoAction;
      break;

    default:
      DBG_Assert ( false, ( << theInit->theName << " Unhandled message type in D_I " << *dirEntry << " " << *msg ) );
      break;
    }
  }

  /* CMU-ONLY-BLOCK-BEGIN */
  bool
  PiranhaCacheControllerImpl::skipAllocateInLocalSlice()
  {
    int aThresholdOfAllocation(theASR_ReplicationThresholds[theInit->theASR_ReplicationLevel[theInit->theNodeId]]);
    int aRandomNumber( (rand_r(&theInit->theASRRandomContext) >> 6) % 256 );
    return (aRandomNumber >= aThresholdOfAllocation);
  }
  /* CMU-ONLY-BLOCK-END */

  void
  PiranhaCacheControllerImpl::handle_D_S ( MemoryMessage_p        msg,
                                           Action               & action,
                                           LookupResult         & l2Lookup,
                                           PiranhaDirEntry_p      dirEntry,
                                           bool                   inEvictBuffer )
  {
    action.theAction = kInsertMAF_WaitResponse;

    DBG_ ( Trace, Addr(msg->address()) ( << theInit->theName << " handleD_S received: " << *msg ) );

    switch ( msg->type() ) {

    case MemoryMessage::LoadReq:
    case MemoryMessage::StreamFetch:
    case MemoryMessage::ReadReq:
    case MemoryMessage::FetchReq:
    case MemoryMessage::AtomicPreloadReq:
      if (msg->coreIdx() == dirEntry->isSharer(msg->coreIdx())) {
        // A request raced with an evict and won the race,
        // so an L1 back-to-back sequence of events of the form
        //                   evict - Rd or Wr request - evict
        // is seen by L2 as
        //                   Rd or Wr request - evict - evict.
        //fixme
        DBG_( VVerb, Addr(msg->address()) ( << theInit->theName << " received RdReq from sharer in D_S. EvictClean must have lost the race. " << *msg ));
      }

      if ( dirEntry->directoryOwned() ) {
        DBG_Assert ( l2Lookup.hit(), ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );
        // We hit within the L2 and we're safe to supply the data
        msg->type() = memoryReply ( msg->type() );
        msg->reqSize() = theInit->theBlockSize;
        dirEntry->addSharer ( msg->coreIdx() );
        l2Lookup.access ( EVICT_CLEAN );
        action.theAction = kReplyAndRemoveMAF;
        action.theRequiresData++;
      } else {
        sendOwnerRequest ( msg,
                           MemoryMessage::ReturnReq,
                           action,
                           dirEntry,
                           D_SFWD,
                           D_S );
        updateStatsHitPeerL1 ( action.theOutputTracker, eShared );
        action.theAction = kInsertMAF_WaitResponse;
        action.theRequiresDuplicateTag++;
      }
      break;

    case MemoryMessage::PrefetchReadNoAllocReq:
        msg->type() = MemoryMessage::PrefetchReadRedundant;
        action.theAction = kReplyAndRemoveMAF;
        action.theRequiresTag++;
        if ( ! dirEntry->directoryOwned() ) {
          action.theRequiresDuplicateTag++;
        }
      break;

    case MemoryMessage::StoreReq:
    case MemoryMessage::StorePrefetchReq:
    case MemoryMessage::RMWReq:
    case MemoryMessage::CmpxReq:
    case MemoryMessage::WriteReq:
    case MemoryMessage::WriteAllocate:
    case MemoryMessage::NonAllocatingStoreReq:
      if (msg->coreIdx() == dirEntry->isSharer(msg->coreIdx())) {
        // A request raced with an evict and won the race,
        // so an L1 back-to-back sequence of events of the form
        //                   evict - Rd or Wr request - evict
        // is seen by L2 as
        //                   Rd or Wr request - evict - evict.
        //fixme
        DBG_( VVerb, Addr(msg->address()) ( << theInit->theName << " received WrReq from sharer in D_S. EvictClean must have lost the race. " << *msg ));
      }

      action.theRequiresDuplicateTag++;

      if ( dirEntry->directoryOwned() && dirEntry->sharersCount() == 0 ) {
        // Get the modifiable permission from outside, but go directly to D_M
        setupExternalRequest ( msg,
                               MemoryMessage::WriteReq,
                               action,
                               dirEntry,
                               D_GetExtModified,
                               D_M );
        action.theAction = kInsertMAF_WaitResponse;
      } else {
        // do an external request first, but go to a transient state
        setupExternalRequest ( msg,
                               MemoryMessage::WriteReq,
                               action,
                               dirEntry,
                               D_GetExtModified,
                               D_S2MW,
                               D_M );
        dirEntry->nextOwner() = msg->coreIdx();
        action.theAction = kInsertMAF_WaitResponse;
      }

      break;

    case MemoryMessage::NonAllocatingStoreReply:
      action.theAction = kSend;
      break;

    case MemoryMessage::UpgradeReq:
    case MemoryMessage::UpgradeAllocate:
      action.theRequiresDuplicateTag++;

      if ( (  (dirEntry->sharersCount() == 1) && dirEntry->isSharer ( msg->coreIdx() ))
           || (dirEntry->sharersCount() == 0)
         )
      {
        // Get the modifiable permission from outside, but go directly to D_M
        setupExternalRequest ( msg,
                               MemoryMessage::UpgradeReq,
                               action,
                               dirEntry,
                               D_GetExtModified,
                               D_M );
        action.theAction = kInsertMAF_WaitResponse;

      } else {
        // do an external request first, but go to a transient state
        setupExternalRequest ( msg,
                               MemoryMessage::UpgradeReq,
                               action,
                               dirEntry,
                               D_GetExtModified,
                               D_S2MU,
                               D_M );
        dirEntry->nextOwner() = msg->coreIdx();
        action.theAction = kInsertMAF_WaitResponse;
      }

      break;

    case MemoryMessage::Invalidate:
    {
      action.theRequiresDuplicateTag++;

      if ( dirEntry->sharersCount() == 0 ) {
        msg->type() = MemoryMessage::InvUpdateAck;

        dirEntry->setState ( D_I );
        theDir->remove ( dirEntry->address() );
        action.theAction = kReplyAndRemoveMAF;
      } else {
        DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " in D_S with some sharers -- TESTING: " << *msg));
        bool ret = sendBroadcast ( msg, msg->type(), action, dirEntry );
        DBG_Assert( ret,
                    ( << theInit->theName
                      << std::hex <<" sharersCount=" << dirEntry->sharersCount()
                      << " sharers=" << dirEntry->sharersList()
                      << " owner=" << dirEntry->owner()
                      << " coreIdx=" << msg->coreIdx()
                      << " Message: " << *msg
                  ) ); // at least one invalidation must be sent

        dirEntry->setState( D_ExtInvalidation );
        dirEntry->nextState() = D_I;
        action.theAction = kInsertMAF_WaitResponse;
      }
      if (l2Lookup.found()) {
        l2Lookup.access ( INVALIDATE );
      }
     break;
    }

    /* CMU-ONLY-BLOCK-BEGIN */
    case MemoryMessage::PurgeIReq: 
    case MemoryMessage::PurgeDReq: 
      DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " in D_S with a purge : " << *msg));
      action.theRequiresDuplicateTag++;
      msg->type() = memoryReply ( msg->type() );  // FIXME: this may break localengine -- assumes original msg type isn't changed

      dirEntry->setState ( D_I );
      theDir->remove ( dirEntry->address() );
      if (l2Lookup.found()) {
        l2Lookup.access ( INVALIDATE );
      }
      action.theAction = kReplyAndRemoveMAF;
      break;
    /* CMU-ONLY-BLOCK-END */

    case MemoryMessage::InvalidateAck:
      DBG_Assert(false, (<< theInit->theName << " InvalidateAck in D_S: " << *msg));
      dirEntry->setState ( D_I );
      theDir->remove ( dirEntry->address() );
      action.theAction = kSend;
      break;

    case MemoryMessage::Downgrade:
      // We are already in the shared state throughout the CMP
      DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " downgrade in D_S: " << *msg));
      msg->type() = memoryReply ( msg->type() );  // FIXME: may break local engine's assumption that orig msg type unchanged
      action.theAction = kReplyAndRemoveMAF;
      break;

    case MemoryMessage::ReturnReq:
      // We are already in the shared state throughout the CMP
      //fixme: shouldn't we check if it is valid and send the request to L1 if not?
      DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " return req in D_S: " << *msg));
      msg->type() = memoryReply ( msg->type() );  // FIXME: may break local engine's assumption that orig msg type unchanged
      action.theAction = kReplyAndRemoveMAF;
      break;

    case MemoryMessage::SVBClean:
      action.theRequiresTag++;
      action.theRequiresDuplicateTag++;
      dirEntry->removeSharer ( msg->coreIdx() );
      if ( dirEntry->owner() == msg->coreIdx() || dirEntry->sharersCount() == 0 ) {
        dirEntry->setState ( D_I );
        theDir->remove ( dirEntry->address() );
        l2Lookup.access ( INVALIDATE );        
      }
      action.theAction = kNoAction;
      break;

    case MemoryMessage::EvictClean:
      action.theRequiresTag++;
      action.theRequiresDuplicateTag++;
      dirEntry->removeSharer ( msg->coreIdx() );

      // If there are no sharers left, we need to make space in the directory for it
      // (possibly evicting a line)
      if ( dirEntry->sharersCount() == 0 ) {

        if ( !l2Lookup.hit() ) {
          if ((theInit->thePlacement == kPrivateCache) && ((msg->coreIdx() >> 1) != theInit->theNodeId)) {
            // in kPrivateCache do not allocate evicts from remote L2s at home node
            DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " Evict in D_S dropped on the floor: " << *msg));
            dirEntry->setState ( D_I );
            theDir->remove ( dirEntry->address() );
          }
          else {

            /* CMU-ONLY-BLOCK-BEGIN */
            if (theInit->thePrivateWithASR && skipAllocateInLocalSlice()) {
              // this is a cache miss, ASR coin flip says that we should not allocate locally
              DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " Evict in D_S bypasses local L2: " << *msg));

              // the local L2 doesn't have a copy, so remove the local directory entry
              dirEntry->setState ( D_I );
              theDir->remove ( dirEntry->address() );

              // pass the evict down to the chip-wide directory
              action.theAction = kSend;
              break;
            }
            /* CMU-ONLY-BLOCK-END */

            // Place the victim in this cache and set us as the owner
            setupEviction ( l2Lookup,
                            EVICT_CLEAN,
                            msg );
            action.theRequiresData += 2;
            dirEntry->setOwner ( kDirectoryOwner );
          }
        }
        else {
          // Victim already in this cache. Set us as the owner
          dirEntry->setOwner ( kDirectoryOwner );
        }
      }
      // if the sharer was the owner, select a new owner
      else if ( dirEntry->owner() == msg->coreIdx() ) {
        dirEntry->setOwner ( dirEntry->firstSharer() );
        DBG_Assert ( !dirEntry->directoryOwned(), ( << theInit->theName << " Failed: " << *dirEntry ) );
      }

      action.theAction = kNoAction;
      break;

    default:
      DBG_Assert ( false, ( << theInit->theName << " Unhandled message type in D_S: " << *dirEntry << " " << *msg ) );
    }
  }

  void
  PiranhaCacheControllerImpl::handle_D_M ( MemoryMessage_p        msg,
                                           Action               & action,
                                           LookupResult         & l2Lookup,
                                           PiranhaDirEntry_p      dirEntry,
                                           bool                   inEvictBuffer )
  {
    action.theAction = kNoAction;

    DBG_ ( Trace, Addr(msg->address()) ( << theInit->theName << " handleD_M received: " << *msg ) );

    intrusive_ptr<MemoryMessage>
      request;

    switch ( msg->type() ) {
    case MemoryMessage::LoadReq:
    case MemoryMessage::ReadReq:
    case MemoryMessage::FetchReq:
    case MemoryMessage::StreamFetch:
    case MemoryMessage::AtomicPreloadReq:
      if (msg->coreIdx() == dirEntry->owner()) {
        // A request raced with an evict and won the race,
        // so an L1 back-to-back sequence of events of the form
        //                   evict - Rd or Wr request - evict
        // is seen by L2 as
        //                   Rd or Wr request - evict - evict.
        //fixme
        DBG_( VVerb, Addr(msg->address()) ( << theInit->theName << " received RdReq from owner in D_M. EvictDirty must have lost the race. " << *msg ));
      }

      DBG_ ( Trace, Addr(msg->address()) ( << theInit->theName << " sharers: " << dirEntry->sharersCount() << " owner: " << dirEntry->owner() ) );

      action.theRequiresDuplicateTag++;

      if ( dirEntry->sharersCount() == 0 ) {
        // We hit within the L2 and we're save to supply the data
        action.theRequiresData++;
        DBG_Assert ( l2Lookup.hit(), ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );
        DBG_Assert ( dirEntry->directoryOwned(), ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );
        msg->type() = memoryReply ( msg->type() );
        msg->reqSize() = theInit->theBlockSize;
        dirEntry->setState ( D_O );
        dirEntry->addSharer ( msg->coreIdx() );
        dirEntry->setOwner ( msg->coreIdx() );
        l2Lookup.access ( EVICT_DIRTY );
        action.theAction = kReplyAndRemoveMAF;
      } else {
        sendOwnerRequest ( msg,
                           MemoryMessage::Downgrade,
                           action,
                           dirEntry,
                           D_M2O,
                           D_O,
                           msg->coreIdx() );
        dirEntry->nextOwner() = msg->coreIdx();
        updateStatsHitPeerL1 ( action.theOutputTracker, eModified );
        action.theAction = kInsertMAF_WaitResponse;
      }
      break;

    case MemoryMessage::PrefetchReadNoAllocReq:
      action.theRequiresTag++;
      msg->type() = MemoryMessage::PrefetchReadRedundant;
      if ( ! dirEntry->directoryOwned() ) {
        action.theRequiresDuplicateTag++;
      }
      action.theAction = kReplyAndRemoveMAF;
      break;

    case MemoryMessage::StoreReq:
    case MemoryMessage::StorePrefetchReq:
    case MemoryMessage::RMWReq:
    case MemoryMessage::CmpxReq:
    case MemoryMessage::WriteReq:
    case MemoryMessage::WriteAllocate:
    case MemoryMessage::NonAllocatingStoreReq:
      if (msg->coreIdx() == dirEntry->owner()) {
        // A request raced with an evict and won the race,
        // so an L1 back-to-back sequence of events of the form
        //                   evict - Rd or Wr request - evict
        // is seen by L2 as
        //                   Rd or Wr request - evict - evict.
        //fixme
        DBG_( VVerb, Addr(msg->address()) ( << theInit->theName << " received WrReq from owner in D_M. EvictDirty must have lost the race. " << *msg ));
      }

      action.theRequiresDuplicateTag++;

      if ( dirEntry->directoryOwned() ) {
        DBG_Assert ( dirEntry->sharersCount() == 0, ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );
        // We hit within the L2 and we're safe to supply the data
        action.theRequiresData++;

        msg->type() = memoryReply ( msg->type() );
        msg->reqSize() = theInit->theBlockSize;
        dirEntry->setState ( D_M );
        dirEntry->addSharer ( msg->coreIdx() );
        dirEntry->setOwner ( msg->coreIdx() );
        l2Lookup.access ( EVICT_DIRTY );
        action.theAction = kReplyAndRemoveMAF;

      } else {

        sendOwnerRequest ( msg,
                           MemoryMessage::Invalidate,
                           action,
                           dirEntry,
                           D_MMW,
                           D_M,
                           msg->coreIdx() );
        updateStatsHitPeerL1 ( action.theOutputTracker, eModified );
        action.theAction = kInsertMAF_WaitResponse;
      }
      break;

    case MemoryMessage::NonAllocatingStoreReply:
      action.theAction = kSend;
      break;

    // Nikos: when a D_O line with one sharer is evicted from the Piranha cache,
    // the dir state goes to D_M with the old D_O owner as the new D_M owner.
    // However, the line at the owner is still in read-only mode, thus it is
    // possible to get upgrade requests. If the cache above is another Piranha
    // cache, it is possible to get write requests as well.
    case MemoryMessage::UpgradeReq:
    case MemoryMessage::UpgradeAllocate:
    {
      action.theRequiresDuplicateTag++;

      if ( dirEntry->directoryOwned() || dirEntry->owner()==msg->coreIdx()) {
        if ( dirEntry->directoryOwned() ) {
          DBG_Assert ( dirEntry->sharersCount() == 0, ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );
          DBG_Assert ( l2Lookup.hit(), ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );
        }
        msg->type() = memoryReply ( msg->type() );
        dirEntry->setState ( D_M );
        dirEntry->setOwner ( msg->coreIdx() );
        dirEntry->addSharer ( msg->coreIdx() );
        if (l2Lookup.found()) {
          l2Lookup.access ( EVICT_DIRTY );
        }
        action.theAction = kReplyAndRemoveMAF;
      } else {
        bool ret = sendBroadcast ( msg,
                                   MemoryMessage::Invalidate,
                                   action,
                                   dirEntry );
        DBG_Assert( ret,
                    ( << theInit->theName
                      << std::hex <<" sharersCount=" << dirEntry->sharersCount()
                      << " sharers=" << dirEntry->sharersList()
                      << " owner=" << dirEntry->owner()
                      << " coreIdx=" << msg->coreIdx()
                      << " Message: " << *msg
                  ) ); // at least one invalidation must be sent, otherwise it would have been an upgrade

        dirEntry->nextOwner() = msg->coreIdx();
        dirEntry->setState ( D_S2MU );
        dirEntry->nextState() = D_M;

        updateStatsHitPeerL1 ( action.theOutputTracker, eModified );
        action.theAction = kInsertMAF_WaitResponse;
      }
      break;
    }

    case MemoryMessage::Invalidate:
    {
      action.theRequiresDuplicateTag++;
      if ( dirEntry->sharersCount() == 0 ) {

        action.theRequiresData++;
        msg->type() = MemoryMessage::InvUpdateAck;

        dirEntry->setState ( D_I );
        theDir->remove ( dirEntry->address() );
        action.theAction = kReplyAndRemoveMAF;
      } else {
        DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " in D_M with some sharers -- TESTING: " << *msg));
        DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " set isModified: " << *msg));
        
        // STOPPED HERE -- isModified is getting set, but we're not changing it to an Update?
        //fixme
        
        dirEntry->isModified() = true;
        bool ret = sendBroadcast ( msg, msg->type(), action, dirEntry );
        DBG_Assert( ret,
                    ( << theInit->theName
                      << std::hex <<" sharersCount=" << dirEntry->sharersCount()
                      << " sharers=" << dirEntry->sharersList()
                      << " owner=" << dirEntry->owner()
                      << " coreIdx=" << msg->coreIdx()
                      << " Message: " << *msg
                  ) ); // at least one invalidation must be sent
        dirEntry->setState( D_ExtInvalidation );
        dirEntry->nextState() = D_I;
        action.theAction = kInsertMAF_WaitResponse;
      }
      if (l2Lookup.found()) {
        l2Lookup.access ( INVALIDATE );
      }
      break;
    }

    /* CMU-ONLY-BLOCK-BEGIN */
    case MemoryMessage::PurgeIReq: 
    case MemoryMessage::PurgeDReq: 
      DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " in D_M with a purge: " << *msg));
      DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " set isModified: " << *msg));

      action.theRequiresDuplicateTag++;
      action.theRequiresData++;
      msg->type() = memoryReply ( msg->type() );  // FIXME: this may break localengine -- assumes original msg type isn't changed

      dirEntry->setState ( D_I );
      theDir->remove ( dirEntry->address() );
      if (l2Lookup.found()) {
        l2Lookup.access ( INVALIDATE );
      }

      action.theAction = kReplyAndRemoveMAF;
      break;
    /* CMU-ONLY-BLOCK-END */

    case MemoryMessage::Downgrade:
    {
      action.theRequiresDuplicateTag++;
      if ( dirEntry->sharersCount() == 0 ) {

        action.theRequiresData++;

        msg->type() = MemoryMessage::DownUpdateAck;

        dirEntry->setState ( D_S );
        action.theAction = kReplyAndRemoveMAF;
      } else {
        DBG_(Trace, (<< theInit->theName << " in D_M with some sharers -- TESTING: " << *msg));
        DBG_(Trace, (<< theInit->theName << " set isModified: " << *msg));
        
        // STOPPED HERE -- isModified is getting set, but we're not changing it to an Update?
        //fixme
        
        dirEntry->isModified() = true;
        bool ret = sendBroadcast ( msg, msg->type(), action, dirEntry );
        DBG_Assert( ret,
                    ( << theInit->theName
                      << std::hex <<" sharersCount=" << dirEntry->sharersCount()
                      << " sharers=" << dirEntry->sharersList()
                      << " owner=" << dirEntry->owner()
                      << " coreIdx=" << msg->coreIdx()
                      << " Message: " << *msg
                  ) ); // at least one invalidation must be sent
        dirEntry->setState( D_ExtDowngrade );
        dirEntry->nextState() = D_S;
        action.theAction = kInsertMAF_WaitResponse;
      }
      if ( l2Lookup.found() ) {
        l2Lookup.access ( DOWNGRADE );
      }
      break;
    }

    case MemoryMessage::DowngradeAck:
      msg->type() = MemoryMessage::DownUpdateAck;

      dirEntry->setState ( D_S );
      action.theAction = kSend;
      break;

    case MemoryMessage::EvictDirty:
    case MemoryMessage::EvictWritable:
      action.theRequiresDuplicateTag++;
      dirEntry->removeSharer ( msg->coreIdx() );
      dirEntry->setOwner ( kDirectoryOwner );

      if ( l2Lookup.hit() ) {
        l2Lookup.access ( EVICT_DIRTY );
        action.theRequiresData++;

        DBG_Assert ( dirEntry->sharersCount() == 0, ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );
        action.theAction = kNoAction;
      } else {

        if ((theInit->thePlacement == kPrivateCache) && ((msg->coreIdx() >> 1) != theInit->theNodeId)) {
          // in kPrivateCache do not allocate evicts from remote L2s at home node
          DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " Evict in D_M sent to mem: " << *msg));
          dirEntry->setState ( D_I );
          theDir->remove ( dirEntry->address() );
          action.theAction = kSend;
        }
        else {
          action.theRequiresDuplicateTag++;
          action.theRequiresData++;
          setupEviction ( l2Lookup,
                          frontAccessTranslator::getAccessType(*msg),
                          msg );
          action.theAction = kNoAction;
        }
      }

      break;


    case MemoryMessage::SVBClean:
    case MemoryMessage::EvictClean:
      // This can happen when an eviction and an upgrade race.  YUCK!
      // Nikos: also when a DirtyEvict is resurrected by a RdReq,
      // which puts the line in D_M at L2 and a MissReply is returned to the requesting L1d,
      // and later on the clean line is evicted from L1d.
      // Nikos: also, when a D_O line with one sharer is evicted from the Piranha cache.
      // The dir state goes to D_M with the old read-only owner as the new owner.
      // However, the line at the old owner is still in read-only mode, thus it is
      // possible to get upgrades and clean evictions. 
      // Nikos: also when a sharer evicts a line and immediately issues a WrReq. The WrReq outruns
      // the evict and reaches the directory whics is in D_S. The directory broadcasts an invalidate
      // to all but the requestor, gets the acks, sends a reply and goes to D_M.
      // Then, the forgotten EvictClean arrives while in D_M.
      // Nikos: also, when an EvictClean -> RdReq -> EvictClean by the sole sharer in D_O is seen as
      // RdReq -> EvictClean -> EvictClean by the directory. The first EvictClean puts the line in
      // D_M if there are no other sharers, and then the second EvictClean comes.
      // We just drop the message
      action.theAction = kNoAction;
      break;

    default:
      // give up
      DBG_Assert ( false, ( << theInit->theName << " Unhandled message type in D_M: " << *dirEntry << " " << *msg ) );
      break;
    }
  }

  void
  PiranhaCacheControllerImpl::handle_D_O ( MemoryMessage_p        msg,
                                           Action               & action,
                                           LookupResult         & l2Lookup,
                                           PiranhaDirEntry_p      dirEntry,
                                           bool                   inEvictBuffer )
  {
    action.theAction = kInsertMAF_WaitResponse;

    DBG_ ( Trace, Addr(msg->address()) ( << theInit->theName << " handleD_O received: " << *msg ) );

    PDState
      nextState;

    switch ( msg->type() ) {
    case MemoryMessage::LoadReq:
    case MemoryMessage::ReadReq:
    case MemoryMessage::FetchReq:
    case MemoryMessage::StreamFetch:
    case MemoryMessage::AtomicPreloadReq:
      if (msg->coreIdx() == dirEntry->isSharer(msg->coreIdx())) {
        // A request raced with an evict and won the race,
        // so an L1 back-to-back sequence of events of the form
        //                   evict - Rd or Wr request - evict
        // is seen by L2 as
        //                   Rd or Wr request - evict - evict.
        //fixme
        DBG_( VVerb, Addr(msg->address()) ( << theInit->theName << " received RdReq from sharer in D_O. EvictClean must have lost the race. " << *msg ));
      }
      if ( l2Lookup.hit() ) {

        action.theRequiresData++;
        msg->type() = memoryReply ( msg->type() );
        msg->reqSize() = theInit->theBlockSize;
        dirEntry->addSharer ( msg->coreIdx() );
        l2Lookup.access ( EVICT_DIRTY );
        action.theAction = kReplyAndRemoveMAF;

      } else {

        action.theRequiresDuplicateTag++;
        sendOwnerRequest ( msg,
                           MemoryMessage::ReturnReq,
                           action,
                           dirEntry,
                           D_M2O,
                           D_O,
                           msg->coreIdx() );
        dirEntry->nextOwner() = msg->coreIdx();
        updateStatsHitPeerL1 ( action.theOutputTracker, eModified );

        action.theAction = kInsertMAF_WaitResponse;
      }

      break;

    case MemoryMessage::PrefetchReadNoAllocReq:
      action.theRequiresTag++;
      msg->type() = MemoryMessage::PrefetchReadRedundant;
      if ( ! l2Lookup.hit()  ) {
        action.theRequiresDuplicateTag++;
      }
      action.theAction = kReplyAndRemoveMAF;
      break;

    case MemoryMessage::StoreReq:
    case MemoryMessage::StorePrefetchReq:
    case MemoryMessage::RMWReq:
    case MemoryMessage::CmpxReq:
    case MemoryMessage::WriteReq:
    case MemoryMessage::WriteAllocate:
      if (msg->coreIdx() == dirEntry->isSharer(msg->coreIdx())) {
        // A request raced with an evict and won the race,
        // so an L1 back-to-back sequence of events of the form
        //                   evict - Rd or Wr request - evict
        // is seen by L2 as
        //                   Rd or Wr request - evict - evict.
        //fixme
        DBG_( VVerb, Addr(msg->address()) ( << theInit->theName << " received WrReq from sharer in D_O. EvictClean must have lost the race. " << *msg ));
      }
      // intentional fall-through

    case MemoryMessage::UpgradeReq:
    case MemoryMessage::UpgradeAllocate:
    case MemoryMessage::NonAllocatingStoreReq:
      action.theRequiresDuplicateTag++;

      if ( msg->type() == MemoryMessage::UpgradeReq ||
           msg->type() == MemoryMessage::UpgradeAllocate
         )
        nextState = D_S2MU;
      else
        nextState = D_S2MW;

      if ( dirEntry->sharersCount() == 1
           && dirEntry->isSharer ( msg->coreIdx() )
         )
      {
        dirEntry->setState ( D_M );
        dirEntry->setOwner ( msg->coreIdx() );
        dirEntry->addSharer ( msg->coreIdx() );
        msg->type() = memoryReply ( msg->type() );
        msg->reqSize() = theInit->theBlockSize;

        if ( l2Lookup.hit() )
          l2Lookup.access ( EVICT_DIRTY );

        action.theAction = kReplyAndRemoveMAF;

      } else {

        DBG_Assert ( dirEntry->sharersCount() > 0, ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );

        bool ret = sendBroadcast ( msg,
                                   MemoryMessage::Invalidate,
                                   action,
                                   dirEntry,
                                   msg->coreIdx() );
        DBG_Assert( ret,
                    ( << theInit->theName
                      << std::hex <<" sharersCount=" << dirEntry->sharersCount()
                      << " sharers=" << dirEntry->sharersList()
                      << " owner=" << dirEntry->owner()
                      << " coreIdx=" << msg->coreIdx()
                      << " Message: " << *msg
                  ) ); // at least one invalidation must be sent, otherwise it would have been an upgrade

        if ( l2Lookup.hit() )
          l2Lookup.access ( EVICT_DIRTY );

        dirEntry->setState ( nextState );
        dirEntry->nextState() = D_M;
        dirEntry->nextOwner() = msg->coreIdx();

        if ( l2Lookup.hit() || dirEntry->isSharer ( msg->coreIdx() ) ) {
          tFillType fillType;
          // fixme
          // if ((theInit->thePlacement == kPrivateCache) && ((msg->coreIdx() >> 1) != theInit->theNodeId)) { // peerL2 (in private) is a coherence fill
          //   fillType = eCoherence;
          // }
          if (action.theOutputTracker && action.theOutputTracker->fillType()) {
            fillType = *action.theOutputTracker->fillType();
          }
          else {
            fillType = pageStateToFillType( msg->pageState() );
          }
          updateStatsHitLocal ( action.theOutputTracker
                                , fillType
                              );
        }
        else {
          updateStatsHitPeerL1 ( action.theOutputTracker, eModified );
        }

        action.theAction = kInsertMAF_WaitResponse;
      }

      break;

    case MemoryMessage::NonAllocatingStoreReply:
      action.theAction = kSend;
      break;

    case MemoryMessage::Invalidate:
    {
      action.theRequiresDuplicateTag++;
      if ( dirEntry->sharersCount() == 0 ) {

        action.theRequiresData++;
        msg->type() = MemoryMessage::InvUpdateAck;
        dirEntry->setState ( D_I );
        theDir->remove ( dirEntry->address() );
        action.theAction = kReplyAndRemoveMAF;
      } else {
        DBG_(Trace, (<< theInit->theName << " in D_O with some sharers -- TESTING: " << *msg));
        DBG_(Trace, (<< theInit->theName << " set isModified: " << *msg));
        dirEntry->isModified() = true;
        bool ret = sendBroadcast ( msg, msg->type(), action, dirEntry );
        DBG_Assert( ret,
                    ( << theInit->theName
                      << std::hex <<" sharersCount=" << dirEntry->sharersCount()
                      << " sharers=" << dirEntry->sharersList()
                      << " owner=" << dirEntry->owner()
                      << " coreIdx=" << msg->coreIdx()
                      << " Message: " << *msg
                  ) ); // at least one invalidation must be sent

        dirEntry->setState( D_ExtInvalidation );
        dirEntry->nextState() = D_I;
        action.theAction = kInsertMAF_WaitResponse;
      }
      if (l2Lookup.found()) {
        l2Lookup.access ( INVALIDATE );
      }
      break;
    }

    /* CMU-ONLY-BLOCK-BEGIN */
    case MemoryMessage::PurgeIReq: 
    case MemoryMessage::PurgeDReq: 
      DBG_(Trace, (<< theInit->theName << " in D_O with a purge: " << *msg));
      DBG_(Trace, (<< theInit->theName << " set isModified: " << *msg));

      action.theRequiresDuplicateTag++;
      action.theRequiresData++;
      msg->type() = memoryReply ( msg->type() );  // FIXME: this may break localengine -- assumes original msg type isn't changed

      dirEntry->setState ( D_I );
      theDir->remove ( dirEntry->address() );
      if (l2Lookup.found()) {
        l2Lookup.access ( INVALIDATE );
      }
        
      action.theAction = kReplyAndRemoveMAF;
      break;
    /* CMU-ONLY-BLOCK-END */

    case MemoryMessage::Downgrade:
    {
      action.theRequiresDuplicateTag++;
      if ( dirEntry->sharersCount() == 0 ) {

        action.theRequiresData++;
        msg->type() = MemoryMessage::DownUpdateAck;
        dirEntry->setState ( D_S );
        action.theAction = kReplyAndRemoveMAF;
      } else {
        DBG_(Trace, (<< theInit->theName << " in D_O with some sharers -- TESTING: " << *msg));
        DBG_(Trace, (<< theInit->theName << " set isModified: " << *msg));
        dirEntry->isModified() = true;
        bool ret = sendBroadcast ( msg, msg->type(), action, dirEntry );
        DBG_Assert( ret,
                    ( << theInit->theName
                      << std::hex <<" sharersCount=" << dirEntry->sharersCount()
                      << " sharers=" << dirEntry->sharersList()
                      << " owner=" << dirEntry->owner()
                      << " coreIdx=" << msg->coreIdx()
                      << " Message: " << *msg
                  ) ); // at least one invalidation must be sent
        dirEntry->setState( D_ExtDowngrade );
        dirEntry->nextState() = D_S;
        action.theAction = kInsertMAF_WaitResponse;
      }
      if ( l2Lookup.found() ) {
        l2Lookup.access ( DOWNGRADE );
      }
      break;
    }

    case MemoryMessage::ReturnReq:
    {
      action.theRequiresDuplicateTag++;
      DBG_(Trace, (<< theInit->theName << " in D_O with some sharers -- TESTING: " << *msg));
      DBG_(Trace, (<< theInit->theName << " set isModified: " << *msg));
      //fixme: do not broadcast; send the ReturnReq to the owner only
      bool ret = sendBroadcast ( msg, msg->type(), action, dirEntry );
      DBG_Assert( ret,
                  ( << theInit->theName
                    << std::hex <<" sharersCount=" << dirEntry->sharersCount()
                    << " sharers=" << dirEntry->sharersList()
                    << " owner=" << dirEntry->owner()
                    << " coreIdx=" << msg->coreIdx()
                    << " Message: " << *msg
                ) ); // at least one invalidation must be sent
      action.theAction = kInsertMAF_WaitResponse;
      break;
    }

    case MemoryMessage::SVBClean:
    case MemoryMessage::EvictClean:
      action.theAction = kNoAction;

      action.theRequiresDuplicateTag++;
      action.theRequiresData++;
      dirEntry->removeSharer ( msg->coreIdx() );

      if ( dirEntry->sharersCount() == 0 ) {
        dirEntry->setState ( D_M );
        dirEntry->setOwner ( kDirectoryOwner );

        if ( !l2Lookup.hit() ) {
          if ((theInit->thePlacement == kPrivateCache) && ((msg->coreIdx() >> 1) != theInit->theNodeId)) {
            // in kPrivateCache do not allocate evicts from remote L2s at home node
            DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " Evict in D_O sent to mem: " << *msg));
            dirEntry->setState ( D_I );
            theDir->remove ( dirEntry->address() );
            msg->type() = MemoryMessage::EvictDirty;
            action.theAction = kSend;
          }
          else {
            // Allocate a block in the L2
            setupEviction ( l2Lookup,
                            EVICT_DIRTY,
                            msg );
            action.theRequiresData++;
            action.theRequiresTag++;
          }
        }
      } else if ( dirEntry->owner() == msg->coreIdx() ) {
        dirEntry->setOwner ( dirEntry->firstSharer() );
        DBG_Assert ( !dirEntry->directoryOwned(), ( << theInit->theName << " Failed: " << *dirEntry ) );
      }


      break;

    default:
      DBG_Assert ( false, ( << theInit->theName << " Unhandled message type in D_O: " << *dirEntry << " " << *msg ) );
    };

  }

  void
  PiranhaCacheControllerImpl::handle_D_ExtGet
  ( MemoryMessage_p        msg,
    Action               & action,
    LookupResult         & l2Lookup,
    PiranhaDirEntry_p      dirEntry,
    bool                   inEvictBuffer )
  {
    action.theAction = kInsertMAF_WaitAddress;

    DBG_ ( Trace, Addr(msg->address()) ( << theInit->theName << " handleD_GetExt received: " << *msg ) );

    MemoryMessage_p      original_miss;
    TransactionTracker_p original_tracker;
    boost::tie ( original_miss, original_tracker ) =
      theController->getWaitingMAFEntry ( msg->address() );

    switch ( msg->type() ) {
    case MemoryMessage::LoadReq:
    case MemoryMessage::ReadReq:
    case MemoryMessage::StreamFetch:
    case MemoryMessage::FetchReq:
    case MemoryMessage::StoreReq:
    case MemoryMessage::StorePrefetchReq:
    case MemoryMessage::NonAllocatingStoreReq:
    case MemoryMessage::RMWReq:
    case MemoryMessage::CmpxReq:
    case MemoryMessage::AtomicPreloadReq:
    case MemoryMessage::WriteReq:
    case MemoryMessage::WriteAllocate:
    case MemoryMessage::UpgradeReq:
    case MemoryMessage::UpgradeAllocate:
    case MemoryMessage::PrefetchReadNoAllocReq:        // ** insert elsewhere, too
    case MemoryMessage::PrefetchReadAllocReq:   // ** insert elsewhere, too
    case MemoryMessage::PrefetchInsert:         // ** insert elsewhere, too
    case MemoryMessage::PrefetchReadRedundant:  // ** insert elsewhere, too
      action.theAction = kInsertMAF_WaitAddress;
      break;

    case MemoryMessage::NonAllocatingStoreReply:
      action.theAction = kSend;
      break;

    case MemoryMessage::LoadReply:
    case MemoryMessage::FetchReply:
    case MemoryMessage::MissReply:
    case MemoryMessage::StoreReply:
    case MemoryMessage::StorePrefetchReply:
    case MemoryMessage::RMWReply:
    case MemoryMessage::CmpxReply:
    case MemoryMessage::AtomicPreloadReply:
    case MemoryMessage::MissReplyWritable:
    case MemoryMessage::MissReplyDirty:
    case MemoryMessage::UpgradeReply:
    case MemoryMessage::PrefetchReadReply:  
    case MemoryMessage::PrefetchWritableReply:  
    case MemoryMessage::StreamFetchWritableReply:  
    case MemoryMessage::PrefetchDirtyReply:  
      {
        //Pass the tracker to theReadTracker for accounting, in case we are
        //finishing a tracked operation
        theReadTracker.finishMiss(original_tracker);

        action.theRequiresDuplicateTag++;

        dirEntry->addSharer ( original_miss->coreIdx() );
        dirEntry->setOwner ( original_miss->coreIdx() );
        dirEntry->setState ( dirEntry->extNextState() );
        original_miss->reqSize() = theInit->theBlockSize;

        // Depending on the transition after the external request,
        // we are required to do a number of different things
        switch ( dirEntry->extNextState() ) {

        case D_S:

          if ( msg->type() == MemoryMessage::StoreReply
               || msg->type() == MemoryMessage::StorePrefetchReply
               || msg->type() == MemoryMessage::RMWReply
               || msg->type() == MemoryMessage::CmpxReply
               || msg->type() == MemoryMessage::MissReplyWritable
               || msg->type() == MemoryMessage::MissReplyDirty
               || msg->type() == MemoryMessage::UpgradeReply
               || msg->type() == MemoryMessage::PrefetchWritableReply
               || msg->type() == MemoryMessage::StreamFetchWritableReply
               || msg->type() == MemoryMessage::PrefetchDirtyReply  
             )
          {
            // The MemoryController thinks it's being smart by sending us a
            // modifiable line.  We'll accept it as such and change the directory state
            // accordingly
            DBG_ ( Trace, Addr(msg->address()) ( << theInit->theName << " Writable reply to shared request, updating directory" ) );
            dirEntry->setState ( D_M );

            // Intentional fallthrough
          }

        case D_M:

          // We can reply now to the request

          if (   original_miss->type() == MemoryMessage::UpgradeReq 
              || original_miss->type() == MemoryMessage::NonAllocatingStoreReq
             )
          {
            original_miss->type() = memoryReply ( original_miss->type() );
          } else {
            original_miss->type() = msg->type();
          }

          DBG_ ( Trace, Addr(msg->address()) ( << theInit->theName << " handleD_GetExt setting original miss type to : " << *original_miss ) );
          DBG_Assert ( dirEntry->sharersCount() == 1, ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );
          //action.theAction =  kReplyAndRemoveMAF;
          action.theAction = kReplyAndRemoveResponseMAF;
          break;

        case D_S2MW:

          // We need to invalidate all sharers now
          DBG_Assert ( msg->type() == MemoryMessage::MissReplyWritable ||
                       msg->type() == MemoryMessage::MissReplyDirty    ||
                       msg->type() == MemoryMessage::UpgradeReply      ||
                       msg->type() == MemoryMessage::StorePrefetchReply,
                       ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );

          if ( dirEntry->sharersCount() == 1 ) {
            // It's possible for the transient condition to go away if there has been an
            // eviction in the meantime
            DBG_Assert ( dirEntry->owner() == original_miss->coreIdx() );

            if (   original_miss->type() == MemoryMessage::UpgradeReq 
                || original_miss->type() == MemoryMessage::NonAllocatingStoreReq
               )
            {
              original_miss->type() = memoryReply ( original_miss->type() );
            } else {
              original_miss->type() = msg->type();
            }

            dirEntry->setState ( D_M );

            DBG_ ( Trace, Addr(msg->address()) ( << theInit->theName << " handleD_GetExt setting original miss (" << *original_miss << " to " << *msg ) );
            action.theAction = kReplyAndRemoveResponseMAF;

          } else {

            sendBroadcast ( msg,
                            MemoryMessage::Invalidate,
                            action,
                            dirEntry,
                            original_miss->coreIdx() );
            action.theAction = kNoAction;
          }
          break;

        case D_S2MU:

          // We need to invalidate all sharers except the one who requested the upgrade
          DBG_Assert ( msg->type() == MemoryMessage::MissReplyWritable ||
                       msg->type() == MemoryMessage::MissReplyDirty    ||
                       msg->type() == MemoryMessage::UpgradeReply      ||
                       msg->type() == MemoryMessage::StorePrefetchReply,
                       ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );

          if ( dirEntry->sharersCount() == 1 ) {
            // It's possible for the transient condition to go away if there has been an
            // eviction in the meantime
            DBG_Assert ( dirEntry->owner() == original_miss->coreIdx() );

            if (   original_miss->type() == MemoryMessage::UpgradeReq 
                || original_miss->type() == MemoryMessage::NonAllocatingStoreReq
               )
            {
              original_miss->type() = memoryReply ( original_miss->type() );
            }
            else {
              original_miss->type() = msg->type();
            }

            dirEntry->setState ( D_M );

            DBG_ ( Trace, Addr(msg->address()) ( << theInit->theName << " handleD_GetExt setting original miss (" << *original_miss << " to " << *msg ) );
            action.theAction = kReplyAndRemoveResponseMAF;

          } else {
            sendBroadcast ( msg,
                            MemoryMessage::Invalidate,
                            action,
                            dirEntry,
                            original_miss->coreIdx() );
            action.theAction = kNoAction;
          }

          break;

        default:
          DBG_Assert ( false , ( << theInit->theName
                                 << " Handle_D_GetExt is transitioning to an unhandled next state: "
                                 << *dirEntry << " " << *msg ));
        };
        dirEntry->extNextState() = D_I;
      }
    break;

    case MemoryMessage::SVBClean:
    case MemoryMessage::EvictClean:
    case MemoryMessage::EvictDirty:
    case MemoryMessage::EvictWritable:

      dirEntry->removeSharer ( msg->coreIdx() );
      if ( dirEntry->owner() == msg->coreIdx() ) {
        dirEntry->setOwner ( kDirectoryOwner );
      }

      if ( !l2Lookup.hit() ) {
          if ((theInit->thePlacement == kPrivateCache) && ((msg->coreIdx() >> 1) != theInit->theNodeId)) {
            // in kPrivateCache do not allocate evicts from remote L2s at home node
            if (msg->type() == MemoryMessage::EvictClean) {
              DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " EvictClean in D_GetExt dropped on the floor: " << *msg));
            }
            else {
              // fixme: so, what happened to the data?
              DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " EvictDirty in D_GetExt dropped on the floor!!! " << *msg));
            }
          }
          else {
            setupEviction ( l2Lookup,
                            frontAccessTranslator::getAccessType ( *msg ),
                            msg );
          }
      } else {
        l2Lookup.access ( frontAccessTranslator::getAccessType ( *msg ) );
      }
      action.theAction = kNoAction;

      break;

    case MemoryMessage::Invalidate:
    case MemoryMessage::Downgrade:
    case MemoryMessage::ReturnReq:
      // fixme: send inval to L1. Also, send returnReq to L1 if !l2Lookup.hit()
      // Think how to use D_GetExtModifiedInvalidationPending
      msg->type() = memoryReply ( msg->type() );  // FIXME: this may break the local engine assumption that orig msg type unchanged
      action.theAction = kSend;
      break;

    default:
      // give up
      DBG_Assert ( false, ( << theInit->theName << " Unhandled message type in D_GetExt: " << *dirEntry << " " << *msg ) );
      break;
    }
  }

  void
  PiranhaCacheControllerImpl::handle_D_ExtMessage
  ( MemoryMessage_p        msg,
    Action               & action,
    LookupResult         & l2Lookup,
    PiranhaDirEntry_p      dirEntry,
    bool                   inEvictBuffer )
  {
    action.theAction = kNoAction;

    MemoryMessage_p      original_miss;
    TransactionTracker_p original_tracker;
    boost::tie ( original_miss, original_tracker ) =
      theController->getWaitingMAFEntry ( msg->address() );

    DBG_( Trace, Addr(msg->address()) ( << theInit->theName << " handle_D_ExtMessage: testing support for DSMs right now " << *dirEntry << " " << *msg ));
    switch ( msg->type() ) {
    case MemoryMessage::DowngradeAck:
    case MemoryMessage::DownUpdateAck:
    
      dirEntry->decrementAcount();

      if ( dirEntry->aCount() == 0  ) {
        if (dirEntry->isModified() && msg->type() == MemoryMessage::DowngradeAck) {
          //fixme
          DBG_(Verb, Addr(msg->address()) (<< theInit->theName << " changing DowngradeAck to DownUpdateAck: " << *msg));
          msg->type() = MemoryMessage::DownUpdateAck;
        }  
        dirEntry->isModified() = false; 
        dirEntry->setState ( D_S );
        original_miss->type() = msg->type();
        action.theAction = kReplyAndRemoveResponseMAF;
      } else {
        action.theAction = kNoAction;
      }
      break;

    case MemoryMessage::InvalidateAck:  
    case MemoryMessage::InvUpdateAck:
        
      dirEntry->decrementAcount();
      dirEntry->removeSharer ( msg->coreIdx() );
      dirEntry->setOwner ( kDirectoryOwner );  // entry should get removed soon, not sure if this matters
      
      if ( dirEntry->aCount() == 0 ) {
        if (dirEntry->isModified() && msg->type() == MemoryMessage::InvalidateAck)  {
          //fixme
          DBG_(Verb, Addr(msg->address()) (<< theInit->theName << " changing InvalidateAck to InvUpdateAck: " << *msg));
          msg->type() = MemoryMessage::InvUpdateAck;
        }   
        dirEntry->isModified() = false; 
        dirEntry->setState ( D_I );
        theDir->remove ( dirEntry->address() );
        original_miss->type() = msg->type();
        if (l2Lookup.hit()) l2Lookup.access ( INVALIDATE );
        action.theAction = kReplyAndRemoveResponseMAF;
      } else {
        action.theAction = kNoAction;
      }
      break;

    case MemoryMessage::ReturnReq:
    case MemoryMessage::ReturnReply:
    
      dirEntry->decrementAcount();

      if ( dirEntry->aCount() == 0  ) {
        dirEntry->isModified() = false; 
        dirEntry->setState ( D_S );
        original_miss->type() = msg->type();
        action.theAction = kReplyAndRemoveResponseMAF;
      } else {
        action.theAction = kNoAction;
      }
      break;

    case MemoryMessage::LoadReq:
    case MemoryMessage::ReadReq:
    case MemoryMessage::StreamFetch:
    case MemoryMessage::FetchReq:
    case MemoryMessage::StoreReq:
    case MemoryMessage::StorePrefetchReq:
    case MemoryMessage::NonAllocatingStoreReq:
    case MemoryMessage::RMWReq:
    case MemoryMessage::CmpxReq:
    case MemoryMessage::AtomicPreloadReq:
    case MemoryMessage::WriteReq:
    case MemoryMessage::WriteAllocate:
    case MemoryMessage::UpgradeReq:
    case MemoryMessage::UpgradeAllocate:
    case MemoryMessage::PrefetchReadNoAllocReq:
    case MemoryMessage::PrefetchReadAllocReq:  
    case MemoryMessage::PrefetchInsert:       
    case MemoryMessage::PrefetchReadRedundant: 
      action.theAction = kInsertMAF_WaitAddress;
      break;

    case MemoryMessage::EvictClean:
    case MemoryMessage::EvictDirty:
    case MemoryMessage::EvictWritable:
      // eviction races (with purge? do we need this? nikos)          /* CMU-ONLY */
      // Write the data to L2, if this is a dirty evict, and consume the message
      action.theAction = kNoAction;
      break;

    default:
      DBG_Assert ( false, ( << theInit->theName << " Unhandled message type in D_ExtMessage: " << *dirEntry << " " << *msg ) );
    }
  }

  ///////////////////////////////////////////////////////////////////
  // Handles the transient states by waiting for the right number of
  // acknowledgements and transitioning to the stable state when
  // they have been received.
  //
  // Other requests should not be handled here...
  //

  void
  PiranhaCacheControllerImpl::handle_D_Transient
  ( MemoryMessage_p        msg,
    Action               & action,
    LookupResult         & l2Lookup,
    PiranhaDirEntry_p      dirEntry,
    bool                   inEvictBuffer )
  {
    action.theAction = kNoAction;

    MemoryMessage_p      original_miss;
    TransactionTracker_p original_tracker;

    DBG_ ( Trace, Addr(msg->address()) ( << theInit->theName << " handle_D_Transient received: " << *msg << " in state: " << *dirEntry) );

    DBG_Assert ( dirEntry->aCount() > 0, ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg ) );

    switch ( msg->type() ) {
    case MemoryMessage::EvictDirty:
    case MemoryMessage::EvictWritable:
    case MemoryMessage::EvictClean:
    case MemoryMessage::SVBClean:
    case MemoryMessage::InvalidateAck:
    case MemoryMessage::InvUpdateAck:
      // For these messages, the block is no longer shared.
      // Note: intentional fallthrough to common handler below
      dirEntry->removeSharer ( msg->coreIdx() );
      if ( dirEntry->owner() == msg->coreIdx() )
        dirEntry->setOwner ( kDirectoryOwner );

    case MemoryMessage::ReturnReply:
    case MemoryMessage::DowngradeAck:
    case MemoryMessage::DownUpdateAck:
      {

        action.theRequiresDuplicateTag++;
        action.theRequiresData++;

        // The eviction messages do not count as acknowledgements
        // in this system.
        if ( msg->type() != MemoryMessage::EvictDirty &&
             msg->type() != MemoryMessage::SVBClean &&
             msg->type() != MemoryMessage::EvictClean &&
             msg->type() != MemoryMessage::EvictWritable ) {
          dirEntry->decrementAcount();
        }

        // Do we need to allocate a block?
        // Do NOT do this if we are in the process of evicting.
        if ( !l2Lookup.hit() &&
             (    msg->type() == MemoryMessage::EvictDirty
               || msg->type() == MemoryMessage::EvictClean
               || msg->type() == MemoryMessage::SVBClean
               || msg->type() == MemoryMessage::EvictWritable
               // || msg->type() == MemoryMessage::InvUpdateAck
               || msg->type() == MemoryMessage::ReturnReply
               || msg->type() == MemoryMessage::InvalidateAck
               || msg->type() == MemoryMessage::DowngradeAck
               || msg->type() == MemoryMessage::DownUpdateAck
           ))
        {
            DBG_ ( Trace, Addr(msg->address()) ( << theInit->theName << " handle_D_Transient: " << *msg << " will setup eviction") );

            AccessType
              accessType = EVICT_CLEAN;

            {
              switch ( msg->type() ) {
              case MemoryMessage::ReturnReply:   accessType = EVICT_CLEAN;     break;
              case MemoryMessage::EvictClean:    accessType = EVICT_CLEAN;     break;
              case MemoryMessage::SVBClean:      accessType = EVICT_CLEAN;     break;
              case MemoryMessage::EvictDirty:    accessType = EVICT_DIRTY;     break;
              case MemoryMessage::EvictWritable: accessType = EVICT_WRITABLE;  break;
                //case MemoryMessage::InvUpdateAck:  accessType = EVICT_DIRTY;     break;
              case MemoryMessage::InvalidateAck: accessType = EVICT_DIRTY;     break;
              case MemoryMessage::DowngradeAck:  accessType = EVICT_CLEAN;     break;
              case MemoryMessage::DownUpdateAck: accessType = EVICT_DIRTY;     break;
              default:
                DBG_Assert ( false , ( << theInit->theName
                                       << " Unhandled message type in converting to UniAccessType in: "
                                       << *dirEntry << " " << *msg ));
              };
            }

            // The block may have been evicted while we were waiting
            // Let's bring it back to life.
            if ( inEvictBuffer ) {
              EvictBuffer::iterator
                evEntry ( theEvictBuffer.find ( msg->address() ) );
              hitsEvict++;
              theEvictBuffer.remove( evEntry );
              dirEntry->evictionQueued() = false;
              inEvictBuffer = false;
            }

            if ((theInit->thePlacement == kPrivateCache) && ((msg->coreIdx() >> 1) != theInit->theNodeId)) {
              // in kPrivateCache do not allocate evicts from remote L2s at home node
              DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " Evict in D_Transient allocates block: " << *msg));
            }
            setupEviction ( l2Lookup,
                            accessType,
                            msg );
            action.theRequiresData++;
            action.theRequiresTag++;
        }

        if ( dirEntry->aCount() == 0 ) {

          // fixme BTG: may need the following?
          //if (dirEntry->state() == D_O) {
          //  if (msg->type() == MemoryMessage::DowngradeAck) msg->type() = MemoryMessage::DownUpdateAck;
          //  else if (msg->type() == MemoryMessage::InvalidateAck) msg->type() = MemoryMessage::InvUpdateAck;
          //}  

          // PiranhaDirState oldState = dirEntry->state();
          dirEntry->setState ( dirEntry->nextState() );
          dirEntry->nextState() = D_I;

          // Set permissions based upon the next state that we're entering
          switch ( dirEntry->state() ) {
          case D_I:
            {
              // We are done with this cache block
              DBG_Assert ( dirEntry->sharersCount() == 0, ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg )   );
              if ( l2Lookup.hit() ) {
                // Remove the cache block from the L2, if it exists
                l2Lookup.access ( INVALIDATE );
              }
              theEvictBuffer.setEvictable ( msg->address(), true );
              theDir->remove ( dirEntry->address() );
              action.theAction = kNoAction;
            }
            break;

          case D_S:
            {
              boost::tie ( original_miss, original_tracker ) =
                theController->getWaitingMAFEntry ( msg->address() );

              dirEntry->setOwner ( kDirectoryOwner );
              dirEntry->nextOwner() = kDirectoryOwner;
              dirEntry->addSharer ( original_miss->coreIdx() );
              original_miss->type() = memoryReply ( original_miss->type() );
              original_miss->reqSize() = theInit->theBlockSize;
              action.theAction = kReplyAndRemoveResponseMAF;
            }
            break;

          case D_M:
            {
              boost::tie ( original_miss, original_tracker ) =
                theController->getWaitingMAFEntry ( msg->address() );

              DBG_ ( Trace, Addr(msg->address()) ( << theInit->theName << " original miss: " << *original_miss << " Dir_next_owner: " << dirEntry->nextOwner() ) );
              DBG_Assert ( dirEntry->nextOwner() == original_miss->coreIdx() );

              dirEntry->setOwner ( original_miss->coreIdx() );
              dirEntry->nextOwner() = kDirectoryOwner;
              dirEntry->addSharer ( original_miss->coreIdx() );
              original_miss->type() = memoryReply ( original_miss->type() );
              original_miss->reqSize() = theInit->theBlockSize;
              action.theAction = kReplyAndRemoveResponseMAF;
            }
            break;

          case D_O:
            {
              boost::tie ( original_miss, original_tracker ) =
                theController->getWaitingMAFEntry ( msg->address() );

              dirEntry->setOwner ( original_miss->coreIdx() );
              dirEntry->addSharer ( original_miss->coreIdx() );
              original_miss->type() = memoryReply ( original_miss->type() );
              original_miss->reqSize() = theInit->theBlockSize;
              dirEntry->nextOwner() = kDirectoryOwner;

              action.theAction = kReplyAndRemoveResponseMAF;
            }
            break;

          case D_GetExtShared:
          case D_GetExtModified:
          default:
            DBG_Assert ( false, ( << theInit->theName << " Unhandled next state in handle_D_Transient(): " << *dirEntry << " " << *msg )  );
          };

        } else {
          action.theAction = kNoAction;
        }
      }
    break;

    case MemoryMessage::Invalidate:
    case MemoryMessage::Downgrade:
    case MemoryMessage::ReturnReq:
      DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " D_Transient got inv/down. dir: " << *dirEntry << " msg: " << *msg));
      // don't break
    case MemoryMessage::ReadReq:
    case MemoryMessage::StreamFetch:
    case MemoryMessage::LoadReq:
    case MemoryMessage::FetchReq:
    case MemoryMessage::StoreReq:
    case MemoryMessage::StorePrefetchReq:
    case MemoryMessage::NonAllocatingStoreReq:
    case MemoryMessage::PrefetchReadNoAllocReq:
    case MemoryMessage::RMWReq:
    case MemoryMessage::CmpxReq:
    case MemoryMessage::AtomicPreloadReq:
    case MemoryMessage::WriteReq:
    case MemoryMessage::WriteAllocate:
    case MemoryMessage::UpgradeReq:
    case MemoryMessage::UpgradeAllocate:
    case MemoryMessage::Probe:
      action.theAction = kInsertMAF_WaitAddress;
      break;

    case MemoryMessage::NonAllocatingStoreReply:
      action.theAction = kSend;
      break;

    default:
      DBG_Assert ( false, ( << theInit->theName << " Unhandled message type in: " << *dirEntry << " " << *msg )  );
    }
  }

  ////////////////////////////////////////////////////////////////////////////////////
  // redirector functions
  Action
  PiranhaCacheControllerImpl::handleMiss
 ( MemoryMessage_p        msg,
   TransactionTracker_p   tracker,
   LookupResult                             & lookup,
   bool                                       address_conflict_on_miss,
   bool                                       probeIfetchMiss )
  {
    bool
      inEvictBuffer = ( theEvictBuffer.find ( msg->address() ) != theEvictBuffer.end() );

    DBG_ ( Trace, Addr(msg->address()) ( << theInit->theName << " handlemiss received: " << *msg ) );

    if ( address_conflict_on_miss ) {
      lockedAccesses++;
      if (tracker) {
        tracker->setDelayCause(theInit->theName, "Address Conflict");
      }
    } else if ( inEvictBuffer ) {

      hitsEvict++;

    } else {
      updateMissStats ( msg,
                        tracker,
                        lookup,
                        theDir->lookup ( msg->address() ),
                        inEvictBuffer );
    }

    return performOperation ( msg, tracker, lookup, false, msg->anyInvs(), true );
  }

  Action
  PiranhaCacheControllerImpl::handleBackMessage
  ( MemoryMessage_p      msg,
    TransactionTracker_p tracker )
  {
    DBG_ ( Trace, Addr(msg->address()) ( << theInit->theName << " handleBackMessage received: " << *msg ) );
    LookupResult lookup = theArray.lookup(msg->address(), msg->idxExtraOffsetBits());

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

    return performOperation ( msg, tracker, lookup, false, msg->anyInvs(), true );
  }

  Action
  PiranhaCacheControllerImpl::handleSnoopMessage
  ( MemoryMessage_p      msg,
    TransactionTracker_p tracker)
  {
    DBG_ ( Trace, Addr(msg->address()) ( << theInit->theName << " handleSnoopMessage received: " << *msg ) );

    accesses++;
    switch ( msg->type() ) {
    case MemoryMessage::ProbedNotPresent:
    case MemoryMessage::ProbedClean:
    case MemoryMessage::ProbedWritable:
    case MemoryMessage::ProbedDirty:
      probes++;
      break;

    case MemoryMessage::InvalidateAck:
    case MemoryMessage::InvUpdateAck:
    case MemoryMessage::DowngradeAck:
    case MemoryMessage::DownUpdateAck:
    case MemoryMessage::ReturnReply:
      snoops++;
      break;
    default:
      break;
    }

    LookupResult lookup = theArray.lookup(msg->address(), msg->idxExtraOffsetBits());
    return performOperation ( msg, tracker, lookup, false, msg->anyInvs(), true );
  }


  // Pull a block from the evict buffer and place it back in the shared cache.
  // This can bring the block to either the shared or modified states.  The
  // directory entry should be freshly allocated.
  bool
  PiranhaCacheControllerImpl::resurrectEvictedBlock
  ( MemoryMessage_p      msg,
    LookupResult      & l2Lookup,
    PiranhaDirEntry_p   dirEntry )
  {
    EvictBuffer::iterator
      evEntry ( theEvictBuffer.find ( msg->address() ) );

    AccessType
      access = EVICT_CLEAN;

    DBG_Assert ( evEntry != theEvictBuffer.end(), ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg )  );
    DBG_Assert ( dirEntry->state() == D_I, ( << theInit->theName << " Failed: " << *dirEntry << " " << *msg )  );

    hitsEvict++;

    theDir->insert ( dirEntry );

    if ( evEntry->type() == MemoryMessage::EvictClean ) {
      dirEntry->setState ( D_S );
      access = EVICT_CLEAN;
    } else {
      dirEntry->setState ( D_M );
      access = EVICT_DIRTY;
    }

    theEvictBuffer.remove ( evEntry );

    if ((theInit->thePlacement == kPrivateCache) && ((msg->coreIdx() >> 1) != theInit->theNodeId)) {
      // in kPrivateCache do not allocate evicts from remote L2s at home node
      DBG_(Trace, Addr(msg->address()) (<< theInit->theName << " resurrectEvictedBlock allocates block: " << *msg));
    }
    setupEviction ( l2Lookup,
                    access,
                    msg );

    return false;
  }

  bool
  PiranhaCacheControllerImpl::updateMissStats
  ( MemoryMessage_p        msg,
    TransactionTracker_p   tracker,
    LookupResult         & l2Lookup,
    PiranhaDirEntry_p      dirEntry,
    bool                   inEvictBuffer )
  {
    // count miss stats only if this is an access to the chip-wide directory
    // fixme: assumes the directory interleaving for private-nuca is the cache line size
    int ChipWideDirID = ((msg->address() >> theInit->theBlockSizeLog2) % theInit->theSlices);
    if ((theInit->thePlacement == kPrivateCache) && (ChipWideDirID != theInit->theNodeId)) {
      return false;
    }

    misses++;
    if ( tracker && tracker->isFetch() && *tracker->isFetch() ) {
      if (tracker->OS() && *tracker->OS()) {
        misses_system_I++;
      } else {
        misses_user_I++;
      }
    } else {
      if ( msg->type() == MemoryMessage::StorePrefetchReq ) {
        if (tracker->OS() && *tracker->OS()) {
          misses_system_D++;
          misses_system_D_PrefetchWrite++;
        } else {
          misses_user_D++;
          misses_user_D_PrefetchWrite++;
        }
      } else if ( msg->type() == MemoryMessage::PrefetchReadNoAllocReq || msg->type() == MemoryMessage::PrefetchReadAllocReq) {

        if (tracker->OS() && *tracker->OS()) {
          misses_system_D++;
          misses_system_D_PrefetchRead++;
        } else {
          misses_user_D++;
          misses_user_D_PrefetchRead++;
        }

      } else if ( msg->isWrite() ) {
        if (tracker->OS() && *tracker->OS()) {
          misses_system_D++;
          misses_system_D_Write++;
        } else {
          misses_user_D++;
          misses_user_D_Write++;
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

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////////
  // Directory utility functions

  // Send a set of independent messages to all sharers (except if matching skipCore)
  // and set the acknowledgement count for that directory entry.
  bool
  PiranhaCacheControllerImpl::sendBroadcast
  ( MemoryMessage_p                    origMessage,
    MemoryMessage::MemoryMessageType   type,
    Action                           & action,
    PiranhaDirEntry_p                  dirEntry,
    int                                skipCore )
  {
    const MemoryAddress address = getBlockAddress( *origMessage );
    SharersList list = dirEntry->sharersList();

    int i;

    MemoryMessage_p msg;

    DBG_ ( Trace,
           Addr(address)
           ( << theInit->theName
             << " Enqueueing broadcast for: " << address
             << " " << type
             << " to all cores in 0x" << std::hex << list << std::dec
             << " except: " << skipCore ) );

    // We cannot have any outstanding requests when starting a broadcast
    // because we only have one counter per directory entry.
    DBG_Assert ( dirEntry->aCount() == 0 );
    DBG_Assert ( dirEntry->sharersCount() > 0 );

    for ( i = 0; list != 0; i++ ) {

      // Treat the sharers list as a bitmask and look through each bit,
      // until we run out of sharers.
      //
      // If this is a valid sharer, send a message and increment
      // the acknowledgement counter
      if ( ( list & 1 ) && ( i != skipCore )  ) {

        msg = new MemoryMessage ( type, address );
        msg->idxExtraOffsetBits() = origMessage->idxExtraOffsetBits();
        msg->pageState() = origMessage->pageState();
        msg->dstMC() = origMessage->dstMC();
        msg->coreIdx() = i;
        msg->coreNumber() = origMessage->coreNumber();  // src
        msg->l2Number() = origMessage->l2Number();      // src of purge

        action.addOutputMessage ( msg );

        dirEntry->incrementAcount();

        DBG_ ( Trace, Addr(address) ( << theInit->theName
                                      << " Enqueued : " << *msg
                                      << " to " << i ) );

      }

      list = list >> 1;
    }

    return (dirEntry->aCount() > 0);
  }

  // Initiate a new request to an external node and set the directory
  // to an intermediate state
  bool
  PiranhaCacheControllerImpl::setupExternalRequest
  ( MemoryMessage_p                  originalRequest,
    MemoryMessage::MemoryMessageType type,
    Action                         & action,
    PiranhaDirEntry_p                dirEntry,
    PiranhaDirState                  extState,       // Which ext state do we go to
    PiranhaDirState                  extNextState,   // Which state do we go to after ext (transient or stable)
    PiranhaDirState                  nextState )     // Which state do we go to after extNextState (stable, if prev was transient)
  {
    MemoryMessage_p
      request = new MemoryMessage ( type, getBlockAddress ( *originalRequest ) );
    request->idxExtraOffsetBits() = originalRequest->idxExtraOffsetBits();
    request->pageState() = originalRequest->pageState();
    request->dstMC() = originalRequest->dstMC();

    //twenisch - preserve coreIdx on external requests.
    request->coreIdx() = originalRequest->coreIdx();
    request->coreNumber() = originalRequest->coreNumber();  // src
    request->l2Number() = originalRequest->l2Number();      // src of purge

    DBG_Assert ( request->isRequest() );
    DBG_Assert ( originalRequest->coreIdx() != kDirectoryOwner );

    dirEntry->setState ( extState );
    dirEntry->extNextState() = extNextState;
    dirEntry->nextState()    = nextState;
    dirEntry->nextOwner() = originalRequest->coreIdx();

    action.addOutputMessage ( request );

    return false;
  }

  bool
  PiranhaCacheControllerImpl::sendOwnerRequest
  ( MemoryMessage_p                    origMessage,
    MemoryMessage::MemoryMessageType   type,
    Action                           & action,
    PiranhaDirEntry_p                  dirEntry,
    PiranhaDirState                    nextState,
    PiranhaDirState                    nextNextState,
    int                                nextOwner )
  {
    DBG_Assert ( dirEntry->owner() != kDirectoryOwner );

    const MemoryAddress address = getBlockAddress( *origMessage );

    // Need to ask the owner to send the data back to us
    MemoryMessage_p request = new MemoryMessage ( type, address );

    request->idxExtraOffsetBits() = origMessage->idxExtraOffsetBits();
    request->pageState() = origMessage->pageState();
    request->dstMC() = origMessage->dstMC();
    request->dstream() = origMessage->isDstream();

    // dest
    int dest;
    /* CMU-ONLY-BLOCK-BEGIN */
    if (theInit->thePlacement == kRNUCACache && type == MemoryMessage::ReturnReq) {
      const int NCoreID = theNTileID [ origMessage->coreNumber() ] * 2 + origMessage->isDstream();
      const int ECoreID = theETileID [ origMessage->coreNumber() ] * 2 + origMessage->isDstream();
      const int WCoreID = theWTileID [ origMessage->coreNumber() ] * 2 + origMessage->isDstream();
      const int SCoreID = theSTileID [ origMessage->coreNumber() ] * 2 + origMessage->isDstream();
      if ( dirEntry->isSharer(NCoreID) ) {
        dest = NCoreID;
      }
      else if ( dirEntry->isSharer(ECoreID) ) {
        dest = ECoreID;
      }
      else if ( dirEntry->isSharer(WCoreID) ) {
        dest = WCoreID;
      }
      else if ( dirEntry->isSharer(SCoreID) ) {
        dest = SCoreID;
      }
      else {
        dest = dirEntry->owner();
      }
    }
    else
    /* CMU-ONLY-BLOCK-END */
    {
      dest = dirEntry->owner();
    }
    request->coreIdx() = dest;

    request->coreNumber() = origMessage->coreNumber();  // src
    request->l2Number() = origMessage->l2Number();      // src of purge

    action.addOutputMessage ( request );

    dirEntry->setState ( nextState );
    dirEntry->nextState() = nextNextState;
    dirEntry->nextOwner() = nextOwner;

    // Generally, we're waiting for a return or an invalidation
    dirEntry->setAcount ( 1 );

    DBG_ ( Trace, Addr(address) ( << theInit->theName << " Sending a " << *request << " to owner core " << request->coreIdx() ) );

    return false;
  }


  /* CMU-ONLY-BLOCK-BEGIN */
  bool
  PiranhaCacheControllerImpl::sendPurgeRequest (   MemoryMessage_p                    origMessage
                                                 , MemoryMessage::MemoryMessageType   type
                                                 , Action                           & action
                                                 , PiranhaDirEntry_p                  dirEntry
                                                 , PiranhaDirState                    nextState
                                                 , PiranhaDirState                    nextNextState
                                               )
  {
    dirEntry->setState ( nextState );
    dirEntry->nextState() = nextNextState;
    dirEntry->nextOwner() = kDirectoryOwner;
    dirEntry->setAcount ( theInit->thePageSize / theInit->theBlockSize ); // We're waiting for a PurgeAck from every line in the page

    const MemoryAddress origAddr = getBlockAddress( *origMessage );
    const unsigned long long pageAddr = (origAddr & ~(theInit->thePageSize-1));
    for (unsigned long long purgeAddr = pageAddr; purgeAddr < pageAddr + theInit->thePageSize; purgeAddr += theInit->theBlockSize) {
      MemoryMessage_p request = new MemoryMessage ( type, MemoryAddress(purgeAddr) );

      if ( type == MemoryMessage::PurgeDReq ) {
        request->idxExtraOffsetBits() = origMessage->idxExtraOffsetBits();
        request->pageState() = origMessage->pageState();
        request->dstMC() = origMessage->dstMC();
        request->dstream() = true;
      }
      else {
        request->idxExtraOffsetBits() = origMessage->idxExtraOffsetBits();
        request->pageState() = origMessage->pageState();
        request->dstMC() = origMessage->dstMC();
        request->dstream() = false;
      }
      request->coreIdx() = origMessage->coreIdx();        // src of ld/st
      request->coreNumber() = origMessage->coreNumber();  // src
      request->l2Number() = origMessage->l2Number();      // src of purge

      action.addOutputMessage ( request );

      DBG_ ( Trace, Addr(purgeAddr) ( << theInit->theName
                                      << " Sending a " << *request
                                      << " to L2 " << request->coreIdx()
                                      << " while in state " << dirEntry->state()
                                      << " #sharers: " << dirEntry->sharersCount()
                                      << std::hex << " sharersList: " << dirEntry->sharersList() << std::dec
                                      << " Owner: " << dirEntry->owner()
                                      << " acount: " << dirEntry->aCount()
                                    ) );
    }

    return false;
  }
  /* CMU-ONLY-BLOCK-END */


  bool
  PiranhaCacheControllerImpl::setupEviction
  ( LookupResult    & l2Lookup,
    AccessType        accessType,
    MemoryMessage_p   replacement )
  {
    DBG_Assert ( !l2Lookup.hit() );

    Block &
      victim = l2Lookup.victim ( accessType );

    PiranhaDirEntry_p
      victimDirEntry = theDir->lookup ( l2Lookup.blockAddress() );

    if ( victim.valid() ) {

      bool dropBlock = true;

      // Now start the eviction process.  We put dirty blocks in the evict buffer
      // and drop clean blocks.
      switch ( victimDirEntry->state() ) {

      case D_S:
        if ( victimDirEntry->sharersCount() > 0 ) {
          victimDirEntry->setOwner ( victimDirEntry->firstSharer() );
          DBG_Assert ( !victimDirEntry->directoryOwned(), ( << theInit->theName << " Failed: " << *victimDirEntry ) );
        } else {
          victimDirEntry->setState ( D_I );
          theDir->remove ( victimDirEntry->address() );
          // NUCA-private requires that no victims are dropped on the floor. For now, just set it to false.
          if (theInit->theDoCleanEvictions) {
            dropBlock = false;
            DBG_(VVerb, ( << theInit->theName << " Will send clean evict " << *victimDirEntry ));
          }
        }
        break;

      case D_M:
        if ( victimDirEntry->directoryOwned() ) {
          DBG_Assert ( victimDirEntry->sharersCount() == 0, ( << theInit->theName << " Failed: " << *victimDirEntry ) );
          victimDirEntry->setState ( D_I );
          theDir->remove ( victimDirEntry->address() );
          dropBlock = false;
        } else {
          // Block is owned in an L1 now
        }
        break;

      case D_O:
        DBG_Assert ( victimDirEntry->sharersCount() > 0 );

        // Our checkpoints allowed the owner to be something other than the directory
        // We are now going to allow the D_O state to have one owner and a bunch of sharers
        // with nothing in the L2
        if ( victimDirEntry->directoryOwned() ) {
          victimDirEntry->setOwner ( victimDirEntry->firstSharer() );
        }

        if ( victimDirEntry->sharersCount() == 1 ) {
          //fixme: shouldn't we tell the cache above that now it has write permission?
          victimDirEntry->setState ( D_M );
        }

        DBG_Assert ( !victimDirEntry->directoryOwned(), ( << theInit->theName << " Failed: " << *victimDirEntry ) );
        break;

      default:
        break;
      }

      // Dirty block needs to be evicted
      if ( !dropBlock ) {
        DBG_ ( Trace, Addr(l2Lookup.blockAddress()) ( << theInit->theName << "  Evicting dirty victim :  " << l2Lookup.blockAddress() ) );
        evictBlock ( victim, l2Lookup.blockAddress() );
        l2Lookup.access ( INVALIDATE );

      } else {
        DBG_ ( Trace, Addr(l2Lookup.blockAddress()) ( << theInit->theName << "  Dropping clean victim :  " << l2Lookup.blockAddress() ) );
      }

    } // valid eviction block

    l2Lookup.access ( accessType );
    l2Lookup.block().tag() = getTag ( replacement->address() );
    DBG_ ( Trace, Addr(replacement->address()) ( << theInit->theName << " Adding new block to the cache: " << *replacement ) );

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////////////
  // Checkpoint functions

  // Copy the ChipDir sharers assuming they are for data lines; the directories at the remote L2s will fix that.
  // Respect I/D sharers if the directory falls at a current sharer.
  void reconcileChipDirSharers( PiranhaDirEntry_p thePiranhaDirEntry, const unsigned long long theChipDirSharers )
  {
    DBG_(VVerb,
         Addr(thePiranhaDirEntry->address())
         ( << std::hex
           << " reconciling ChipDirShr=" << theChipDirSharers
           << " dir: " << *thePiranhaDirEntry
        ));
    // for each possible sharer (bit in the sharers bitmask:
    for (unsigned int i=0; i < (sizeof(unsigned long long)*8); i++) {
      // if it is a sharer in the chip-wide directory...
      if ((theChipDirSharers & (1ULL << i)) != 0ULL) {
        // ...and not already a sharer in the private L2 directory
        // (inherited from the CmpCache that implements each private L2 slice)...
        if ( !thePiranhaDirEntry->isSharer( i*2 ) && !thePiranhaDirEntry->isSharer( i*2+1 ) ) {
          // ...add it
          thePiranhaDirEntry->addSharer( i*2+1 );
        }
      }
    }
  }

  template<typename _t>
  void loadVector( std::ifstream& ifs, std::string const & aName, std::vector<_t> & aVector ) {
    std::string theName;
    ifs >> theName;
    DBG_Assert(aName == theName,( << "Attempted to load state for " << theName << " while expecting " << aName));
    int aSize;
    _t temp;
    ifs >> aSize;
    aVector.clear();
    while(aSize--) {
      ifs >> temp;
      aVector.push_back(temp);
    }
    char tmpc;
    ifs >> tmpc;
    DBG_Assert(tmpc == '-',(<< "End of list marker not found, found: " << tmpc));
    DBG_(Dev,(<< "Loaded " << aName));
  }

  void PiranhaCacheControllerImpl:: saveState ( std::string const & aDirName )
  {
    BaseCacheControllerImpl::saveState ( aDirName );

    std::string fname ( aDirName );
    fname += "/" + theInit->theName + "-PiranhaDirectory";

    std::ofstream ofs ( fname.c_str() );
    theDir->saveState ( ofs  );
    ofs.close();
  }

  void PiranhaCacheControllerImpl::loadState ( std::string const & aDirName )
  {
    BaseCacheControllerImpl::loadState ( aDirName );

    /* CMU-ONLY-BLOCK-BEGIN */
    if (theInit->thePrivateWithASR) {
      std::string fnameASR( aDirName );
      fnameASR += "/sys-network-control-ASR";
      std::ifstream ifsASR( fnameASR.c_str() );
      ifsASR >> fnameASR;
      ifsASR >> theInit->theASRRandomContext;
      loadVector<int>(ifsASR,"theASR_ReplicationLevel",theInit->theASR_ReplicationLevel);
    }
    /* CMU-ONLY-BLOCK-END */

    std::string fname ( aDirName );
    fname += "/" + theInit->theName + "-PiranhaDirectory";

    std::ifstream ifs ( fname.c_str() );
    theDir->loadState ( ifs );
    ifs.close();

    //////// reconcile piranhaDir state with ChipDir state from TraceNUCAFlex to support kPrivateCache ckpt save/restore
    std::string fnameChipDir ( aDirName );
    fnameChipDir += "/sys-network-control-ChipDir"; //fixme: hardcoded component name
    std::ifstream ifsChipDir ( fnameChipDir.c_str() );

    if (! ifsChipDir.good()) {
      DBG_( Crit, ( << " saved checkpoint state " << fnameChipDir << " not found.  Resetting to empty ChipDir for private-nuca. " )  );
    }
    else {
      ifsChipDir >> std::skipws; // skip white space

      // Read the ChipDir map info
      int chip_dir_size = 0;
      unsigned int aL2Design;
      unsigned int aSharersBitmaskComponents;
      ifsChipDir >> aL2Design
          >> aSharersBitmaskComponents
          >> chip_dir_size
        ;

      // make sure the configurations match
      DBG_Assert(   (theInit->thePlacement == (tPlacement) aL2Design)
                 || (theInit->thePlacement == kPrivateCache && aL2Design == kASRCache)  /* CMU-ONLY */
                );
      // DBG_Assert(aSharersBitmaskComponents == theSharersBitmaskComponents);
      // if simulating private, the page map should be non-empty
      DBG_Assert ( (theInit->thePlacement != kPrivateCache) || (chip_dir_size >= 0) );

      // Read in each entry
      for ( int i=0; i < chip_dir_size; i++ ) {
        char paren1;
        unsigned long long theAddress;
        unsigned int theChipDirState;
        unsigned long long theChipDirSharers_0;
        unsigned long long theChipDirSharers_1;
        char paren2;

        ifsChipDir >> paren1;
        DBG_Assert ( (paren1 == '{'), ( << theInit->theName << "Expected '{' when loading thePageTable from checkpoint" ) );

        ifsChipDir >> theAddress
                   >> theChipDirState
                   >> theChipDirSharers_0
                   >> theChipDirSharers_1
          ;
        DBG_Assert( (theChipDirSharers_0 & ~0xffffffffULL) == 0ULL,
                    ( << theInit->theName
                      << " Timing doesn't support > 32 nodes yet: "
                      << std::hex << (theChipDirSharers_0 & ~0xffffffffULL)
                  ));
        DBG_Assert( theChipDirSharers_1 == 0,
                    ( << theInit->theName
                      << " Timing doesn't support > 32 nodes yet."
                  ));

        ifsChipDir >> paren2;
        DBG_Assert ( (paren2 == '}'), ( << theInit->theName << "Expected '}' when loading thePageTable from checkpoint" ) );

        if ( ((theAddress >> theInit->theBlockSizeLog2) % theInit->theSlices) != (unsigned int) theInit->theNodeId ) {
          // this is a different interleave. Go to the next one
          continue;
        }

        DBG_ ( VVerb,
               Addr(theAddress)
               ( << theInit->theName
                 << std::hex
                 << " Address: "        << theAddress
                 << " ChipDirState: "   << theChipDirState
                 << " ChipDirSharers: " << theChipDirSharers_0
             ) );

        // ChipDir states:
        static const unsigned long long kStateMask =        0x0007ULL; // ----  ----  ----  -111
        static const unsigned long long kChipDirInvalid =   0x0000ULL; // ----  ----  ----  -000
        static const unsigned long long kChipDirShared =    0x0001ULL; // ----  ----  ----  -001
        static const unsigned long long kChipDirOwned =     0x0002ULL; // ----  ----  ----  -010
        static const unsigned long long kChipDirModified =  0x0003ULL; // ----  ----  ----  -011

        theChipDirState &= kStateMask; // clean up the other crap, let the state shine

        PiranhaDirEntry_p thePiranhaDirEntry = theDir->lookup( MemoryAddress(theAddress) );

        if ( thePiranhaDirEntry->state() == D_I ) {
          ////////// add new piranha directory entry for chip-wide directory.
          DBG_Assert( thePiranhaDirEntry->address() == theAddress );
          switch (theChipDirState) {
            case kChipDirInvalid:
            {
              DBG_Assert(false, ( << theInit->theName << "How is this possible?????? Will ignore it and move to the next entry." ));
              break;
            }
            case kChipDirShared:
            {
              thePiranhaDirEntry->setState( D_S );
              reconcileChipDirSharers( thePiranhaDirEntry, theChipDirSharers_0 );
              thePiranhaDirEntry->setOwner( thePiranhaDirEntry->firstSharer() );
              break;
            }
            case kChipDirOwned:
            {
              thePiranhaDirEntry->setState( D_O );
              reconcileChipDirSharers( thePiranhaDirEntry, theChipDirSharers_0 );
              thePiranhaDirEntry->setOwner( thePiranhaDirEntry->firstSharer() );
              break;
            }
            case kChipDirModified:
            {
              thePiranhaDirEntry->setState( D_M );
              DBG_Assert(theChipDirSharers_0 < 32);
              reconcileChipDirSharers( thePiranhaDirEntry, (1ULL << theChipDirSharers_0) ); // note: assumes it is a data line
              thePiranhaDirEntry->setOwner( theChipDirSharers_0*2+1 ); // note: assumes it is a data line
              break;
            }
            default:
            {
              DBG_Assert( false , ( << std::hex << theChipDirState ) );
            }
          }
          // thePiranhaDirEntry->sharersCount() is taken care of at reconcileChipDirSharers()
          // thePiranhaDirEntry->setAcount( 0 ) is taken care of at the constructor
          thePiranhaDirEntry->nextOwner()      = thePiranhaDirEntry->owner();
          thePiranhaDirEntry->nextState()      = thePiranhaDirEntry->state();
          // thePiranhaDirEntry->nextMsg() is take care of at constructor
          thePiranhaDirEntry->extNextState()   = thePiranhaDirEntry->state();
          thePiranhaDirEntry->evictionQueued() = false;
          thePiranhaDirEntry->isModified()     = false;

          theDir->insert( thePiranhaDirEntry );
        }

        else {
          DBG_(VVerb, Addr(thePiranhaDirEntry->address()) ( << theInit->theName << " Orinigal state: " << *thePiranhaDirEntry ));

          ////////// reconcile
          DBG_Assert( thePiranhaDirEntry->address() == theAddress );
          switch (theChipDirState) {
            case kChipDirInvalid:
            {
              DBG_Assert(false, ( << theInit->theName << "How is this possible??????" ));
              break;
            }
            case kChipDirShared:
            {
              switch (thePiranhaDirEntry->state())
              {
                case D_S:
                {
                  // thePiranhaDirEntry->setState( D_S );
                  reconcileChipDirSharers(thePiranhaDirEntry, theChipDirSharers_0 );
                  thePiranhaDirEntry->setOwner( thePiranhaDirEntry->firstSharer() );
                  break;
                }
                case D_I:
                case D_O:
                case D_M:
                default:
                {
                  DBG_Assert( false );
                }
              }
              break;
            }
            case kChipDirOwned:
            {
              switch (thePiranhaDirEntry->state())
              {
                case D_S:
                  thePiranhaDirEntry->setState( D_O );
                  // intentional fall-through
                case D_O:
                {
                  // thePiranhaDirEntry->setState( D_O );
                  reconcileChipDirSharers( thePiranhaDirEntry, theChipDirSharers_0 );
                  thePiranhaDirEntry->setOwner( thePiranhaDirEntry->firstSharer() );
                  break;
                }
                case D_I:
                case D_M:
                default:
                {
                  DBG_Assert( false );
                }
              }
              break;
            }
            case kChipDirModified:
            {
              switch (thePiranhaDirEntry->state())
              {
                case D_O:
                {
                  thePiranhaDirEntry->setState( D_M );
                  if (thePiranhaDirEntry->owner() != kDirectoryOwner) {
                    DBG_Assert( thePiranhaDirEntry->isSharer(theChipDirSharers_0*2) || thePiranhaDirEntry->isSharer(theChipDirSharers_0*2+1),
                                ( << theInit->theName
                                  << std::hex
                                  << " ChipDirState=0x" << theChipDirState
                                  << " ChipDirShr=0x" << theChipDirSharers_0
                                  << " " << *thePiranhaDirEntry
                              ));
                    DBG_Assert( thePiranhaDirEntry->owner() == (int) theChipDirSharers_0*2 || thePiranhaDirEntry->owner() == (int) theChipDirSharers_0*2+1,
                                ( << theInit->theName
                                  << std::hex
                                  << " ChipDirState=0x" << theChipDirState
                                  << " ChipDirShr=0x" << theChipDirSharers_0
                                  << " " << *thePiranhaDirEntry
                              ));
                  }
                  break;
                }
                case D_M:
                {
                  // thePiranhaDirEntry->setState( D_M );
                  if (thePiranhaDirEntry->owner() != kDirectoryOwner) {
                    DBG_Assert( thePiranhaDirEntry->isSharer(theChipDirSharers_0*2+1),
                                ( << theInit->theName
                                  << std::hex
                                  << " ChipDirState=0x" << theChipDirState
                                  << " ChipDirShr=0x" << theChipDirSharers_0
                                  << " " << *thePiranhaDirEntry
                              ));
                    DBG_Assert( thePiranhaDirEntry->owner() == (int) theChipDirSharers_0*2+1,
                                ( << theInit->theName
                                  << std::hex
                                  << " ChipDirState=0x" << theChipDirState
                                  << " ChipDirShr=0x" << theChipDirSharers_0
                                  << " " << *thePiranhaDirEntry
                              ));
                  }
                  break;
                }
                case D_I:
                case D_S:
                default:
                {
                  DBG_Assert( false,
                              ( << theInit->theName
                                << std::hex
                                << " ChipDirState=0x" << theChipDirState
                                << " ChipDirShr=0x" << theChipDirSharers_0
                                << " " << *thePiranhaDirEntry
                             ));
                }
              }
              break;
            }
            default:
            {
              DBG_Assert( false );
            }
          }
          // thePiranhaDirEntry->sharersCount() is taken care of at reconcileChipDirSharers()
          // thePiranhaDirEntry->setAcount( 0 ) is taken care of at the constructor
          thePiranhaDirEntry->nextOwner()      = thePiranhaDirEntry->owner();
          thePiranhaDirEntry->nextState()      = thePiranhaDirEntry->state();
          // thePiranhaDirEntry->nextMsg() is take care of at constructor
          thePiranhaDirEntry->extNextState()   = thePiranhaDirEntry->state();
          thePiranhaDirEntry->evictionQueued() = false;
          thePiranhaDirEntry->isModified()     = false;
        }

        DBG_(VVerb, Addr(thePiranhaDirEntry->address()) ( << theInit->theName << " Final state: " << *thePiranhaDirEntry ));
      }
      ifsChipDir.close();
      DBG_( Dev, ( << theInit->theName << " ChipDir loaded." ));
    }
  }

  void PiranhaDir::loadState ( std::ifstream & ifs )
  {
    int
      dirSize = 0;

    clear();
    ifs >> blockMask >> dirSize;

    DBG_Assert ( dirSize >= 0 );

    // Read in each block
    for ( int i = 0; i < dirSize; i++ ) {
      insert ( new PiranhaDirEntry ( ifs ) );
    }
  }

  void PiranhaDir::saveState ( std::ofstream & ofs )
  {
    // Write the block mask and the number of entries
    ofs << blockMask << " " << theDir.size() << std::endl;

    // Write out each directory entry
    DirMap::iterator
      iter = theDir.begin();

    while ( iter != theDir.end() ) {
      iter->second->saveState ( ofs );
      iter++;
    }
  }

  void PiranhaDirEntry::saveState ( std::ofstream & ofs )
  {
    ofs << "{ "
        << (unsigned long long)theAddress           << " "
        << (int)theState        << " "
        << (unsigned long long)theSharersList       << " "
        << theSharersCount      << " "
        << 0                    << " " // fixme: trace supports 64 nodes (128 bits of sharers list). Timing doesn't yet.
        << theOwner             << " "
        << theAcount            << " "
        << theNextOwner         << " "
        << (int)theNextState    << " "
        << (int)theNextMsg      << " "
        << (int)theExtNextState << " "
        << theEvictionQueued    << " }"
        << std::endl;
  }

  PiranhaDirEntry::PiranhaDirEntry ( std::ifstream & ifs )
  {
    char
      paren;

    ifs >> paren;
    DBG_Assert ( paren == '{' );

    unsigned long long theSharersList_2;
    ifs >> theAddress
        >> (int&)theState
        >> theSharersList
        >> theSharersList_2 // fixme: trace supports 64 nodes (128 bits of sharers list). Timing doesn't yet.
        >> theSharersCount
        >> theOwner
        >> theAcount
        >> theNextOwner
        >> (int&)theNextState
        >> (int&)theNextMsg
        >> (int&)theExtNextState
        >> theEvictionQueued;

    DBG_(VVerb,
         ( << std::hex
           << " addr=" << theAddress
           << " state=" << theState
           << " sharers=" << theSharersList
           << " cnt=" << theSharersCount
           << " owner=" << theOwner
           << " acnt=" << theAcount
        ));

    ifs >> paren;
    DBG_Assert ( paren == '}' );
  }


  ////////////////////////////////////////////////////////////////
  // Stats functions
  void
  PiranhaCacheControllerImpl::overwriteStatsHitLocal
  ( TransactionTracker_p tracker
    , tFillType fillType )
  {
    if ( tracker ) {
      tracker->setNetworkTrafficRequired ( false );
      tracker->setResponder ( theInit->theNodeId );
      tracker->setFillLevel ( theInit->theCacheLevel );
      tracker->setFillType ( fillType );
      DBG_ ( VVerb, ( << theInit->theName << " Setting local stats for " << tracker << " FillLevel=" << theInit->theCacheLevel << " FillType=" << fillType ) );
    }
    else {
      DBG_ ( VVerb, ( << theInit->theName << " NULL tracker " ) );
    }
  }

  void
  PiranhaCacheControllerImpl::updateStatsHitLocal
  ( TransactionTracker_p tracker
    , tFillType fillType )
  {
    if ( tracker && !tracker->fillLevel() ) {
      tracker->setNetworkTrafficRequired ( false );
      tracker->setResponder ( theInit->theNodeId );
      tracker->setFillLevel ( theInit->theCacheLevel );
      tracker->setFillType ( fillType );
      DBG_ ( VVerb, ( << theInit->theName << " Setting local stats for " << tracker << " FillLevel=" << theInit->theCacheLevel << " FillType=" << fillType ) );
    }
    else {
      DBG_ ( VVerb, ( << theInit->theName << " Already have local stats for " << tracker << " FillLevel=" << theInit->theCacheLevel << " FillType=" << fillType ) );
    }
  }

  void
  PiranhaCacheControllerImpl::updateStatsHitPeerL1
  ( TransactionTracker_p tracker
    , tStateType stateType )
  {
    // Override any other cache that might have set t
    if ( tracker /*&& !tracker->fillLevel()*/ ) {
      tracker->setNetworkTrafficRequired ( false );
      tracker->setResponder ( theInit->theNodeId );
      tracker->setFillLevel ( ePeerL1Cache );
      tracker->setFillType ( eCoherence );
      tracker->setPreviousState ( stateType );

      if (tracker->isFetch() && *tracker->isFetch()) {
        if (tracker->OS() && *tracker->OS()) {
          misses_peerL1_system_I++;
        } else {
          misses_peerL1_user_I++;
        }
      } else {
        if (tracker->OS() && *tracker->OS()) {
          misses_peerL1_system_D++;
        } else {
          misses_peerL1_user_D++;
        }
      }
    }
  }

  Action PiranhaCacheControllerImpl::handleIprobe ( bool aHit,
                           MemoryMessage_p        fetchReq,
                           TransactionTracker_p   tracker ) {
    DBG_Assert(false);
    return Action(kNoAction);
  }

};  // namespace nCache
