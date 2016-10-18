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

#include <components/MemoryLoopback/MemoryLoopback.hpp>

#include <components/Common/MessageQueues.hpp>

#define FLEXUS_BEGIN_COMPONENT MemoryLoopback
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()


  #define DBG_DefineCategories Memory
  #define DBG_SetDefaultOps AddCat(Memory)
  #include DBG_Control()

#include <components/Common/MemoryMap.hpp>
#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>
#include <components/Common/Slices/ExecuteState.hpp>


namespace nMemoryLoopback {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

using Flexus::SharedTypes::MemoryMap;


using boost::intrusive_ptr;

class FLEXUS_COMPONENT(MemoryLoopback) {
  FLEXUS_COMPONENT_IMPL(MemoryLoopback );

  boost::intrusive_ptr<MemoryMap> theMemoryMap;
  unsigned int interleavingBlockShiftBits;

 public:
   FLEXUS_COMPONENT_CONSTRUCTOR( MemoryLoopback )
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    {}

  bool isQuiesced() const {
    if (! outQueue) {
      return true; //Quiesced before initialization
    }
    return outQueue->empty();
  }

  // Initialization
  void initialize() {
    if(cfg.Delay < 1) {
      std::cout << "Error: memory access time must be greater than 0." << std::endl;
      throw FlexusException();
    }
    theMemoryMap = MemoryMap::getMemoryMap(flexusIndex());
    outQueue.reset( new nMessageQueues::DelayFifo< MemoryTransport >(cfg.MaxRequests));

    unsigned int i;
  	
  	interleavingBlockShiftBits = 0;
  	
  	for (i = 1; i < cfg.L2InterleavingGranularity; i*=2) {
  		++interleavingBlockShiftBits;
  	}    	
  }

    // fixme: add a map to identify eCold
    void fillTracker(MemoryTransport & aMessageTransport) {
      if (aMessageTransport[TransactionTrackerTag]) {
        aMessageTransport[TransactionTrackerTag]->setFillLevel(eLocalMem);
        if (!aMessageTransport[TransactionTrackerTag]->fillType() ) {
          aMessageTransport[TransactionTrackerTag]->setFillType(eReplacement);
        }
        aMessageTransport[TransactionTrackerTag]->setResponder(flexusIndex());
        aMessageTransport[TransactionTrackerTag]->setNetworkTrafficRequired(false);
      }
    }

  //LoopBackIn PushInput Port
  //=========================
    bool available(interface::LoopbackIn const &
                   , index_t anIndex
                  )
    {
      return ! outQueue->full() ;
    }

    void push(interface::LoopbackIn const &
              , index_t anIndex
              , MemoryTransport & aMessageTransport
             )
    {
      PhysicalMemoryAddress anAddr = aMessageTransport[MemoryMessageTag]->address();
      const int anL2 = getL2Tile(anAddr, anIndex);  // remember the L2 that sent the request
      DBG_(Iface,
           Comp(*this)
           Addr(anAddr)
           ( << "request received from port[" << anIndex << "]: " << *aMessageTransport[MemoryMessageTag] )
          );
      intrusive_ptr<MemoryMessage> reply;
      switch (aMessageTransport[MemoryMessageTag]->type()) {
        case MemoryMessage::LoadReq:
          theMemoryMap->recordAccess( anAddr, MemoryMap::Read);
          reply = new MemoryMessage(*aMessageTransport[MemoryMessageTag]);
          reply->type() = MemoryMessage::LoadReply;
          reply->reqSize() = 64;
          fillTracker(aMessageTransport);
          break;
        case MemoryMessage::FetchReq:
          theMemoryMap->recordAccess( anAddr, MemoryMap::Read);
          reply = new MemoryMessage(*aMessageTransport[MemoryMessageTag]);
          reply->type() = MemoryMessage::FetchReply;
          reply->reqSize() = 64;
          fillTracker(aMessageTransport);
          break;
        case MemoryMessage::StoreReq:
          theMemoryMap->recordAccess( anAddr, MemoryMap::Write);
          reply = new MemoryMessage(*aMessageTransport[MemoryMessageTag]);
          reply->type() = MemoryMessage::StoreReply;
          reply->reqSize() = 0;
          fillTracker(aMessageTransport);
          break;
        case MemoryMessage::StorePrefetchReq:
          theMemoryMap->recordAccess( anAddr, MemoryMap::Write);
          reply = new MemoryMessage(*aMessageTransport[MemoryMessageTag]);
          reply->type() = MemoryMessage::StorePrefetchReply;
          reply->reqSize() = 0;
          fillTracker(aMessageTransport);
          break;
        case MemoryMessage::CmpxReq:
          theMemoryMap->recordAccess( anAddr, MemoryMap::Write);
          reply = new MemoryMessage(*aMessageTransport[MemoryMessageTag]);
          reply->type() = MemoryMessage::CmpxReply;
          reply->reqSize() = 64;
          fillTracker(aMessageTransport);
          break;

        case MemoryMessage::ReadReq:
          theMemoryMap->recordAccess( anAddr, MemoryMap::Read);
          reply = new MemoryMessage(*aMessageTransport[MemoryMessageTag]);
          reply->type() = MemoryMessage::MissReplyWritable;
          reply->reqSize() = 64;
          fillTracker(aMessageTransport);
          break;
        case MemoryMessage::WriteReq:
        case MemoryMessage::WriteAllocate:
          theMemoryMap->recordAccess( anAddr, MemoryMap::Write);
          reply = new MemoryMessage(*aMessageTransport[MemoryMessageTag]);
          reply->type() = MemoryMessage::MissReplyWritable;
          reply->reqSize() = 64;
          fillTracker(aMessageTransport);
          break;
        case MemoryMessage::NonAllocatingStoreReq:
          theMemoryMap->recordAccess( aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Write);
          reply = aMessageTransport[MemoryMessageTag];
          reply->type() = MemoryMessage::NonAllocatingStoreReply;
          fillTracker(aMessageTransport);
          break;

        case MemoryMessage::UpgradeReq:
        case MemoryMessage::UpgradeAllocate:
          theMemoryMap->recordAccess( anAddr, MemoryMap::Write);
          reply = new MemoryMessage(*aMessageTransport[MemoryMessageTag]);
          reply->type() = MemoryMessage::UpgradeReply;
          reply->reqSize() = 0;
          fillTracker(aMessageTransport);
          break;
        case MemoryMessage::FlushReq:
        case MemoryMessage::Flush:
        case MemoryMessage::EvictDirty:
        case MemoryMessage::EvictWritable:
        case MemoryMessage::EvictClean:
          // no reply required
          if (aMessageTransport[TransactionTrackerTag]) {
            aMessageTransport[TransactionTrackerTag]->setFillLevel(eLocalMem);
            aMessageTransport[TransactionTrackerTag]->setFillType(eReplacement);
            aMessageTransport[TransactionTrackerTag]->complete();
          }
          return;
        case MemoryMessage::PrefetchReadAllocReq:
        case MemoryMessage::PrefetchReadNoAllocReq:
          theMemoryMap->recordAccess( anAddr, MemoryMap::Read);
          reply = new MemoryMessage(*aMessageTransport[MemoryMessageTag]);
          reply->type() = MemoryMessage::PrefetchWritableReply;
          reply->reqSize() = 64;
          fillTracker(aMessageTransport);
          break;
        case MemoryMessage::StreamFetch:
          theMemoryMap->recordAccess( aMessageTransport[MemoryMessageTag]->address(), MemoryMap::Read);
          reply = new MemoryMessage(MemoryMessage::StreamFetchWritableReply, aMessageTransport[MemoryMessageTag]->address());
          reply->reqSize() = 64;
          fillTracker(aMessageTransport);
          break;
        case MemoryMessage::PrefetchInsert:
          // should never happen
          DBG_Assert(false, Component(*this) (<< "MemoryLoopback received PrefetchInsert request") );
          break;
        case MemoryMessage::PrefetchInsertWritable:
          // should never happen
          DBG_Assert(false, Component(*this) (<< "MemoryLoopback received PrefetchInsertWritable request") );
          break;
        default:
          DBG_Assert(false, Component(*this) (<< "Don't know how to handle message: " << (*aMessageTransport[MemoryMessageTag]) << "  No reply sent." ) );
          return;
      }
      DBG_(VVerb,
           Comp(*this)
           Addr(anAddr)
           (<< "Queing reply: " << *reply)
           );
      reply->coreNumber() = anL2; // overload coreNumber to remember which L2 interface to send the reply to
      aMessageTransport.set(MemoryMessageTag, reply);

      // account for the one cycle delay inherent from Flexus when sending a
      // response back up the hierarchy: actually stall for one less cycle
      // than the configuration calls for
      outQueue->enqueue(aMessageTransport,cfg.Delay -1);
    }

  //Drive Interfaces
  void drive(interface::LoopbackDrive const &) {
    MemoryTransport trans;
    
    if (!outQueue->ready()) {
  		return;
  	}
    
    trans = outQueue->peek();
    
    DBG_(VVerb, Comp(*this)
                Addr(trans[MemoryMessageTag]->address())
                ( << "Will peek for L2[" << trans[MemoryMessageTag]->coreNumber() << "]: " << *(trans[MemoryMessageTag]))
        );
    while(FLEXUS_CHANNEL_ARRAY(LoopbackOut, trans[MemoryMessageTag]->coreNumber()).available()) {
      DBG_(Iface, Comp(*this)
                  Addr(trans[MemoryMessageTag]->address())
                  ( << "Sending reply to L2[" << trans[MemoryMessageTag]->coreNumber() << "]: " << *(trans[MemoryMessageTag]))
          );
      FLEXUS_CHANNEL_ARRAY(LoopbackOut, trans[MemoryMessageTag]->coreNumber()) << trans;
      outQueue->dequeue();
      
      if (outQueue->ready()) {
      	trans = outQueue->peek();
      }
      else {
      	return;
      }
      DBG_(VVerb, Comp(*this)
                  Addr(trans[MemoryMessageTag]->address())
                  ( << "Will peek for L2[" << trans[MemoryMessageTag]->coreNumber() << "]: " << *(trans[MemoryMessageTag]))
          );
    }
  }

private:
  boost::scoped_ptr< nMessageQueues::DelayFifo< MemoryTransport > > outQueue;

  unsigned int getL2Tile(PhysicalMemoryAddress theAddress, index_t anIndex) {
	// return (theAddress >> interleavingBlockShiftBits) % cfg.NumL2Tiles;
    return anIndex;
  }
};

} //End Namespace nMemoryLoopback

FLEXUS_COMPONENT_INSTANTIATOR( MemoryLoopback, nMemoryLoopback );

FLEXUS_PORT_ARRAY_WIDTH( MemoryLoopback, LoopbackOut ) { return cfg.NumL2TilesPerMC ; } //fixme
FLEXUS_PORT_ARRAY_WIDTH( MemoryLoopback, LoopbackIn ) { return cfg.NumL2TilesPerMC ; } //fixme


#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT MemoryLoopback

  #define DBG_Reset
  #include DBG_Control()

