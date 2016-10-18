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

#include "FCMPCacheControllerImpl.hpp"

  #define DBG_DeclareCategories Cache
  #define DBG_SetDefaultOps AddCat(Cache)
  #include DBG_Control()

namespace nCache
{

  //////////////////////////////////////////////////////
  // FCMPCacheController member functions

  FCMPCacheControllerImpl::FCMPCacheControllerImpl
  ( BaseCacheController * aController,
    CacheInitInfo       * aInit )
    : PiranhaCacheControllerImpl ( aController,
                                   aInit )
  {}


  void
  FCMPCacheControllerImpl::handle_D_M
  ( MemoryMessage_p        msg,
    Action               & action,
    LookupResult         & l2Lookup,
    PiranhaDirEntry_p      dirEntry,
    bool                   inEvictBuffer )
  {
    action.theAction = kNoAction;

    DBG_ ( Trace, ( << "FCMP::handleD_M received: " << *msg ) );

    intrusive_ptr<MemoryMessage>
      request;

    switch ( msg->type() ) {
    case MemoryMessage::LoadReq:
    case MemoryMessage::ReadReq:
    case MemoryMessage::FetchReq:
    case MemoryMessage::AtomicPreloadReq:

      if ( dirEntry->sharersCount() == 0 ) {
        DBG_Assert ( l2Lookup.hit() );
        DBG_Assert ( dirEntry->directoryOwned() );
        msg->type() = memoryReply ( msg->type() );
        dirEntry->setState ( D_O );
        dirEntry->addSharer ( msg->coreIdx() );
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
        action.theAction = kInsertMAF_WaitResponse;
      }
      break;

    case MemoryMessage::StoreReq:
    case MemoryMessage::StorePrefetchReq:
    case MemoryMessage::RMWReq:
    case MemoryMessage::CmpxReq:
    case MemoryMessage::WriteReq:
    case MemoryMessage::WriteAllocate:

      if ( dirEntry->owner() == getPartnerCore ( msg->coreIdx() ) ) {
        // Prefer to get it from the pair processor
        DBG_Assert ( dirEntry->sharersCount() == 1 );

        sendOwnerRequest ( msg,
                           MemoryMessage::ReturnReq,
                           action,
                           dirEntry,
                           D_MMW,
                           D_M,
                           msg->coreIdx() );

      } else if ( dirEntry->directoryOwned() ) {

        DBG_Assert ( dirEntry->sharersCount() == 0 );

        // Service from the L2, if we have it there
        msg->type() = memoryReply ( msg->type() );
        dirEntry->setState ( D_M );
        dirEntry->addSharer ( msg->coreIdx() );
        l2Lookup.access ( EVICT_DIRTY );
        action.theAction = kReplyAndRemoveMAF;

      } else {
        // Otherwise get it from somewhere else

        sendOwnerRequest ( msg,
                           MemoryMessage::Invalidate,
                           action,
                           dirEntry,
                           D_MMW,
                           D_M,
                           msg->coreIdx() );
        action.theAction = kInsertMAF_WaitResponse;
      }

      break;

    case MemoryMessage::UpgradeReq:
    case MemoryMessage::UpgradeAllocate:

      if ( dirEntry->owner() == getPartnerCore ( msg->coreIdx() ) ) {
        // Prefer to get it from the pair processor

        sendOwnerRequest ( msg,
                           MemoryMessage::ReturnReq,
                           action,
                           dirEntry,
                           D_MMU,
                           D_M,
                           msg->coreIdx() );

      } else if ( dirEntry->directoryOwned() ) {

        // Service from the L2, if we have it there
        msg->type() = memoryReply ( msg->type() );
        dirEntry->setState ( D_M );
        dirEntry->addSharer ( msg->coreIdx() );
        l2Lookup.access ( EVICT_DIRTY );
        action.theAction = kReplyAndRemoveMAF;

      } else {
        // Otherwise get it from somewhere else

        sendOwnerRequest ( msg,
                           MemoryMessage::Invalidate,
                           action,
                           dirEntry,
                           D_MMU,
                           D_M,
                           msg->coreIdx() );
        action.theAction = kInsertMAF_WaitResponse;
      }
      break;


    case MemoryMessage::Invalidate:
    case MemoryMessage::Downgrade:
    case MemoryMessage::ReturnReq:

      if ( dirEntry->sharersCount() == 0 ) {

        if ( msg->type() == MemoryMessage::Invalidate ) {
          msg->type() = MemoryMessage::InvUpdateAck;
          dirEntry->setState ( D_I );
          theDir->remove ( dirEntry->address() );
          l2Lookup.access ( INVALIDATE );
        } else {
          msg->type() = MemoryMessage::DownUpdateAck;
          dirEntry->setState ( D_S );
          l2Lookup.access ( DOWNGRADE );
        }

        action.theAction = kReplyAndRemoveMAF;
      } else {

        sendBroadcast ( msg, msg->type(), action, dirEntry );

        if ( msg->type() == MemoryMessage::Invalidate ) {
          dirEntry->nextState() = D_I;
        } else {
          dirEntry->nextState() = D_S;
        }

        action.theAction = kInsertMAF_WaitResponse;
      }

      break;


    case MemoryMessage::EvictDirty:
    case MemoryMessage::EvictWritable:

      if ( isVocal ( msg->coreIdx() ) ) {
        dirEntry->removeSharer ( msg->coreIdx() );
        dirEntry->setOwner ( kDirectoryOwner );

        if ( l2Lookup.hit() ) {
          l2Lookup.access ( EVICT_DIRTY );

          DBG_Assert ( dirEntry->sharersCount() == 0 );
        } else {
          setupEviction ( l2Lookup,
                          frontAccessTranslator::getAccessType(*msg),
                          msg );
        }
      }

      action.theAction = kNoAction;

      break;

    case MemoryMessage::EvictClean:
      // This can happen when an eviction and an upgrade race.  YUCK!
      // We just drop the message
      action.theAction = kNoAction;
      break;

    default:
      DBG_ ( Crit, ( << "unhandled message type in FCMP::D_M: "
                     << *msg << *dirEntry ) );
      DBG_Assert ( false );
      break;
    };
  }

  bool
  FCMPCacheControllerImpl::sendOwnerRequest
  ( MemoryMessage_p                  origMessage,
    MemoryMessage::MemoryMessageType type,
    Action                         & action,
    PiranhaDirEntry_p                dirEntry,
    PDState                          nextState,
    PDState                          nextNextState,
    int                              nextOwner )
  {
    DBG_Assert ( dirEntry->owner() != kDirectoryOwner );
    DBG_Assert ( dirEntry->isSharer ( dirEntry->owner() ) );
    DBG_Assert ( dirEntry->sharersCount() <= 2 );

    const MemoryAddress address = getBlockAddress( *origMessage );

    int
      aCount = 0;

    // Need to ask the owner to send the data back to us
    {
      MemoryMessage_p
        request = new MemoryMessage ( type, address );
      request->idxExtraOffsetBits() = origMessage->idxExtraOffsetBits();

      request->coreIdx() = dirEntry->owner();

      action.addOutputMessage ( request );

      aCount++;
      DBG_ ( Trace, (<< "Sending a " << *request << " to owner core "
                     << request->coreIdx() ) );
    }

    // And ask the partner core, if it is a sharer
    if ( dirEntry->isSharer ( getPartnerCore ( dirEntry->owner() ) ) ) {
      MemoryMessage_p
        request = new MemoryMessage ( type, address );
      request->idxExtraOffsetBits() = origMessage->idxExtraOffsetBits();

      request->coreIdx() = getPartnerCore ( dirEntry->owner() );

      action.addOutputMessage ( request );

      aCount++;
      DBG_ ( Trace, (<< "Sending a " << *request << " to partner owner core "
                     << getPartnerCore ( request->coreIdx() ) ) );
    }

    dirEntry->setState ( nextState );
    dirEntry->nextState() = nextNextState;
    dirEntry->nextOwner() = nextOwner;

    // Generally, we're waiting for a return or an invalidation
    dirEntry->setAcount ( aCount );

    return false;
  }

}; // namespace nCache
