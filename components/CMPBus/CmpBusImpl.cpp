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


#include <components/CMPBus/CmpBus.hpp>

#include <components/Common/MessageQueues.hpp>

#include <core/stats.hpp>
#include <boost/optional.hpp>
#include <boost/none.hpp>


  #define DBG_DefineCategories CmpBus
  #define DBG_SetDefaultOps AddCat(CmpBus)
  #include DBG_Control()


#define FLEXUS_BEGIN_COMPONENT CmpBus
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <core/simics/configuration_api.hpp>
#include <core/simics/mai_api.hpp>

#include <components/DecoupledFeeder/SimicsTracer.hpp>

namespace Flexus {
namespace Simics {
namespace API {
extern "C" {
#include FLEXUS_SIMICS_API_HEADER(types)
#define restrict
#include FLEXUS_SIMICS_API_HEADER(memory)
#undef restrict

#include FLEXUS_SIMICS_API_ARCH_HEADER

#include FLEXUS_SIMICS_API_HEADER(configuration)
#include FLEXUS_SIMICS_API_HEADER(processor)
#include FLEXUS_SIMICS_API_HEADER(front)
#include FLEXUS_SIMICS_API_HEADER(event)
#undef printf
#include FLEXUS_SIMICS_API_HEADER(callbacks)
#include FLEXUS_SIMICS_API_HEADER(breakpoints)
} //extern "C"
} //namespace API
} //namespace Simics
} //namespace Flexus


namespace nCmpBus {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

using namespace Flexus::Core;
namespace Stat = Flexus::Stat;
namespace API = Flexus::Simics::API;

using namespace nMessageQueues;

class DMATracerImpl {
    API::conf_object_t * theUnderlyingObject;
    API::conf_object_t * theMapObject;


    boost::function< void(boost::intrusive_ptr<MemoryMessage>) > toDMA;

  public:
    DMATracerImpl(API::conf_object_t * anUnderlyingObjec)
      : theUnderlyingObject(anUnderlyingObjec)
      {}

    // Initialize the tracer to the desired CPU
    void init(API::conf_object_t * aMapObject,boost::function< void(boost::intrusive_ptr<MemoryMessage> ) > aToDMA) {
      theMapObject = aMapObject;
      toDMA = aToDMA;

      API::attr_value_t attr;
      attr.kind = API::Sim_Val_Object;
      attr.u.object = theUnderlyingObject;

      /* Tell memory we have a mem hier */
      API::SIM_set_attribute(theMapObject, "timing_model", &attr);
    }

    API::cycles_t dma_mem_hier_operate(API::conf_object_t *space, API::map_list_t *map, API::generic_transaction_t *aMemTrans) {
      Simics::APIFwd::memory_transaction_t * mem_trans = reinterpret_cast<Simics::APIFwd::memory_transaction_t *>(aMemTrans);

      const int k_no_stall = 0;

      //debugTransaction(mem_trans);
      mem_trans->s.block_STC = 1;

      boost::intrusive_ptr< MemoryMessage > msg;
      if (API::SIM_mem_op_is_write(&mem_trans->s)) {
        DBG_( Verb, ( << "DMA To Mem: " << std::hex << mem_trans->s.physical_address  << std::dec << " Size: " << mem_trans->s.size ) );
        msg = new MemoryMessage( MemoryMessage::Invalidate, PhysicalMemoryAddress( mem_trans->s.physical_address & ~63 ) );
      } else {
        DBG_( Verb, ( << "DMA From Mem: " << std::hex << mem_trans->s.physical_address  << std::dec << " Size: " << mem_trans->s.size ) );
        msg = new MemoryMessage( MemoryMessage::Downgrade, PhysicalMemoryAddress( mem_trans->s.physical_address & ~63 ) );
      }

      toDMA(msg);
      return k_no_stall; //Never stalls
    }

};  // class DMATracerImpl


class DMATracer : public Simics::AddInObject <DMATracerImpl> {
    typedef Simics::AddInObject<DMATracerImpl> base;
   public:
    static const Simics::Persistence  class_persistence = Simics::Session;
    static std::string className() { return "DMATracer"; }
    static std::string classDescription() { return "Flexus's DMA tracer."; }

    DMATracer() : base() { }
    DMATracer(Simics::API::conf_object_t * aSimicsObject) : base(aSimicsObject) {}
    DMATracer(DMATracerImpl * anImpl) : base(anImpl) {}
};


class FLEXUS_COMPONENT(CmpBus) {
  FLEXUS_COMPONENT_IMPL(CmpBus);

   DMATracer theDMATracer;

 public:
  FLEXUS_COMPONENT_CONSTRUCTOR(CmpBus)
    : base ( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theCycles_Idle( statName() + "-Cycles:Idle")
    , theCycles_Cache_Data( statName() + "-Cycles:Cache:Data")
    , theCycles_Cache_Control( statName() + "-Cycles:Cache:Control")
    , theCycles_Index_Data( statName() + "-Cycles:Index:Data")
    , theCycles_Index_Control( statName() + "-Cycles:Index:Control")
    , theCycles_CMOB_Data( statName() + "-Cycles:CMOB:Data")
    , theCycles_CMOB_Control( statName() + "-Cycles:CMOB:Control")
    , theCycles_byType_LoadReq                 ( statName() + "-Cycles:ByType:LoadReq")
    , theCycles_byType_StoreReq                ( statName() + "-Cycles:ByType:StoreReq")
    , theCycles_byType_StorePrefetchReq        ( statName() + "-Cycles:ByType:StorePrefetchReq")
    , theCycles_byType_FetchReq                ( statName() + "-Cycles:ByType:FetchReq")
    , theCycles_byType_NonAllocatingStoreReq   ( statName() + "-Cycles:ByType:NonAllocatingStoreReq")
    , theCycles_byType_StreamFetch             ( statName() + "-Cycles:ByType:StreamFetch")
    , theCycles_byType_RMWReq                  ( statName() + "-Cycles:ByType:RMWReq")
    , theCycles_byType_CmpxReq                 ( statName() + "-Cycles:ByType:CmpxReq")
    , theCycles_byType_AtomicPreloadReq        ( statName() + "-Cycles:ByType:AtomicPreloadReq")
    , theCycles_byType_FlushReq                ( statName() + "-Cycles:ByType:FlushReq")
    , theCycles_byType_ReadReq                 ( statName() + "-Cycles:ByType:ReadReq")
    , theCycles_byType_WriteReq                ( statName() + "-Cycles:ByType:WriteReq")
    , theCycles_byType_WriteAllocate           ( statName() + "-Cycles:ByType:WriteAllocate")
    , theCycles_byType_UpgradeReq              ( statName() + "-Cycles:ByType:UpgradeReq")
    , theCycles_byType_UpgradeAllocate         ( statName() + "-Cycles:ByType:UpgradeAllocate")
    , theCycles_byType_Flush                   ( statName() + "-Cycles:ByType:Flush")
    , theCycles_byType_EvictDirty              ( statName() + "-Cycles:ByType:EvictDirty")
    , theCycles_byType_EvictWritable           ( statName() + "-Cycles:ByType:EvictWritable")
    , theCycles_byType_EvictClean              ( statName() + "-Cycles:ByType:EvictClean")
    , theCycles_byType_SVBClean                ( statName() + "-Cycles:ByType:SVBClean")
    , theCycles_byType_LoadReply               ( statName() + "-Cycles:ByType:LoadReply")
    , theCycles_byType_StoreReply              ( statName() + "-Cycles:ByType:StoreReply")
    , theCycles_byType_StorePrefetchReply      ( statName() + "-Cycles:ByType:StorePrefetchReply")
    , theCycles_byType_FetchReply              ( statName() + "-Cycles:ByType:FetchReply")
    , theCycles_byType_RMWReply                ( statName() + "-Cycles:ByType:RMWReply")
    , theCycles_byType_CmpxReply               ( statName() + "-Cycles:ByType:CmpxReply")
    , theCycles_byType_AtomicPreloadReply      ( statName() + "-Cycles:ByType:AtomicPreloadReply")
    , theCycles_byType_MissReply               ( statName() + "-Cycles:ByType:MissReply")
    , theCycles_byType_MissReplyWritable       ( statName() + "-Cycles:ByType:MissReplyWritable")
    , theCycles_byType_MissReplyDirty          ( statName() + "-Cycles:ByType:MissReplyDirty")
    , theCycles_byType_UpgradeReply            ( statName() + "-Cycles:ByType:UpgradeReply")
    , theCycles_byType_NonAllocatingStoreReply ( statName() + "-Cycles:ByType:NonAllocatingStoreReply")
    , theCycles_byType_PrefetchReadNoAllocReq  ( statName() + "-Cycles:ByType:PrefetchReadNoAllocReq")
    , theCycles_byType_PrefetchReadAllocReq    ( statName() + "-Cycles:ByType:PrefetchReadAllocReq")
    , theCycles_byType_PrefetchReadReply       ( statName() + "-Cycles:ByType:PrefetchReadReply")
    , theCycles_byType_PrefetchWritableReply   ( statName() + "-Cycles:ByType:PrefetchWritableReply")
    , theCycles_byType_StreamFetchWritableReply( statName() + "-Cycles:ByType:StreamFetchWritableReply")
    , theCycles_byType_PrefetchDirtyReply      ( statName() + "-Cycles:ByType:PrefetchDirtyReply")
    , theBusDirection(kIdle)
    {}

  static const int kSource_Cache = 1;

  static const int kSource_Index = 2;
  static const int kSource_CMOB = 10;

  enum eDirection
    { kIdle
    , kToCache
    , kToMemory
    , kToIndex
    , kToCMOB
    };

  Stat::StatCounter theCycles_Idle;
  Stat::StatCounter theCycles_Cache_Data;
  Stat::StatCounter theCycles_Cache_Control;
  Stat::StatCounter theCycles_Index_Data;
  Stat::StatCounter theCycles_Index_Control;
  Stat::StatCounter theCycles_CMOB_Data;
  Stat::StatCounter theCycles_CMOB_Control;

  Stat::StatCounter theCycles_byType_LoadReq;
  Stat::StatCounter theCycles_byType_StoreReq;
  Stat::StatCounter theCycles_byType_StorePrefetchReq;
  Stat::StatCounter theCycles_byType_FetchReq;
  Stat::StatCounter theCycles_byType_NonAllocatingStoreReq;
  Stat::StatCounter theCycles_byType_StreamFetch;
  Stat::StatCounter theCycles_byType_RMWReq;
  Stat::StatCounter theCycles_byType_CmpxReq;
  Stat::StatCounter theCycles_byType_AtomicPreloadReq;
  Stat::StatCounter theCycles_byType_FlushReq;
  Stat::StatCounter theCycles_byType_ReadReq;
  Stat::StatCounter theCycles_byType_WriteReq;
  Stat::StatCounter theCycles_byType_WriteAllocate;
  Stat::StatCounter theCycles_byType_UpgradeReq;
  Stat::StatCounter theCycles_byType_UpgradeAllocate;
  Stat::StatCounter theCycles_byType_Flush;
  Stat::StatCounter theCycles_byType_EvictDirty;
  Stat::StatCounter theCycles_byType_EvictWritable;
  Stat::StatCounter theCycles_byType_EvictClean;
  Stat::StatCounter theCycles_byType_SVBClean;
  Stat::StatCounter theCycles_byType_LoadReply;
  Stat::StatCounter theCycles_byType_StoreReply;
  Stat::StatCounter theCycles_byType_StorePrefetchReply;
  Stat::StatCounter theCycles_byType_FetchReply;
  Stat::StatCounter theCycles_byType_RMWReply;
  Stat::StatCounter theCycles_byType_CmpxReply;
  Stat::StatCounter theCycles_byType_AtomicPreloadReply;
  Stat::StatCounter theCycles_byType_MissReply;
  Stat::StatCounter theCycles_byType_MissReplyWritable;
  Stat::StatCounter theCycles_byType_MissReplyDirty;
  Stat::StatCounter theCycles_byType_UpgradeReply;
  Stat::StatCounter theCycles_byType_NonAllocatingStoreReply;
  Stat::StatCounter theCycles_byType_PrefetchReadNoAllocReq;
  Stat::StatCounter theCycles_byType_PrefetchReadAllocReq;
  Stat::StatCounter theCycles_byType_PrefetchReadReply;
  Stat::StatCounter theCycles_byType_PrefetchWritableReply;
  Stat::StatCounter theCycles_byType_StreamFetchWritableReply;
  Stat::StatCounter theCycles_byType_PrefetchDirtyReply;


  unsigned int theBusCountdown;
  eDirection theBusDirection;
  boost::optional<MemoryTransport> theBusContents;

  boost::optional< MemoryTransport > theFromCache_RequestBuffer;
  boost::optional< MemoryTransport > theFromCache_PrefetchBuffer;
  boost::optional< MemoryTransport > theFromCache_SnoopBuffer;
  boost::optional< MemoryTransport > theToCacheBuffer;
  boost::optional< MemoryTransport > theFromMemoryBuffer;
  boost::optional< MemoryTransport > theToMemoryBuffer;

  MessageQueue< MemoryTransport > theFromMemory_ToCacheBuffer;
  MessageQueue< MemoryTransport > theFromMemory_ToCMOBBuffer;
  MessageQueue< MemoryTransport > theFromMemory_ToIndexBuffer;

  boost::optional< MemoryTransport > theFromIndexBuffer;
  boost::optional< MemoryTransport > theToIndexBuffer;

  std::list< boost::intrusive_ptr<MemoryMessage> > theDMAMessages;

  std::vector< boost::optional< MemoryTransport > > theToCMOBBuffer;
  std::vector< boost::optional< MemoryTransport > > theFromCMOBBuffer;

  bool isQuiesced() const {
    return theBusDirection == kIdle;
  }

  void saveState(std::string const & aName) { }

  void loadState(std::string const & aName) { }

  // Initialization
  void initialize() {
    theBusCountdown = 0;
    theBusDirection = kIdle;

    DBG_Assert( cfg.BusTime_Data > 0);
    DBG_Assert( cfg.BusTime_NoData > 0);
    theToCMOBBuffer.resize( Flexus::Core::ComponentManager::getComponentManager().systemWidth() );
    theFromCMOBBuffer.resize( Flexus::Core::ComponentManager::getComponentManager().systemWidth() );
    theFromMemory_ToCacheBuffer.setSize(cfg.QueueSize); 
    theFromMemory_ToCMOBBuffer.setSize(cfg.QueueSize); 
    theFromMemory_ToIndexBuffer.setSize(cfg.QueueSize); 

    if (cfg.EnableDMA) {
      createDMATracer();
    }
 
  }

  //Ports
  //======
  //CacheIn_Snoop
  //---------------------------------------
  bool available( interface::FromCache_Snoop const &) {
    return ! theFromCache_SnoopBuffer;
  }

  void push( interface::FromCache_Snoop  const &, MemoryTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received on Port FromCache_Snoop: " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    // Set the message as coming from this core
    aMessage.set( BusTag, new Mux(kSource_Cache));
    theFromCache_SnoopBuffer = aMessage;
  }

  //CacheIn_Request
  //---------------------------------------
  bool available( interface::FromCache_Request const &) {
    return ! theFromCache_RequestBuffer;
  }

  void push( interface::FromCache_Request const &, MemoryTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received on Port FromCache_Request : " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    // Set the message as coming from this core
    aMessage.set( BusTag, new Mux(kSource_Cache));
    theFromCache_RequestBuffer = aMessage;
  }

  bool available( interface::FromCache_Prefetch const &) {
    return ! theFromCache_PrefetchBuffer;
  }

  void push( interface::FromCache_Prefetch const &, MemoryTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received on Port FromCache_Prefetch : " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    // Set the message as coming from this core
    aMessage.set( BusTag, new Mux(kSource_Cache));
    theFromCache_PrefetchBuffer = aMessage;
  }

  //FromIndex
  //---------------------------------------
  bool available( interface::FromIndex const &) {
    return ! theFromIndexBuffer;
  }

  void push( interface::FromIndex const &, MemoryTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received on Port FromIndex: " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    // Set the message as coming from this core
    aMessage.set( BusTag, new Mux(kSource_Index));
    theFromIndexBuffer = aMessage;
  }

  //MemoryIn
  //---------------------------------------
  bool available( interface::FromMemory const &) {
    return (! theFromMemory_ToCacheBuffer.full() && ! theFromMemory_ToCMOBBuffer.full() && ! theFromMemory_ToIndexBuffer.full() );
  }

  void push( interface::FromMemory const &, MemoryTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received on Port FromMemory : " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    // Set the message as coming from this core
    DBG_Assert (aMessage[BusTag]->source != 0 );
    if (aMessage[BusTag]->source == kSource_Cache) {
      theFromMemory_ToCacheBuffer.enqueue( aMessage );
    } else if (aMessage[BusTag]->source == kSource_Index) {
      theFromMemory_ToIndexBuffer.enqueue( aMessage );
    } else if (aMessage[BusTag]->source >= kSource_CMOB ) {
      theFromMemory_ToCMOBBuffer.enqueue( aMessage );
    }  
  }

  //CMOBIn
  //---------------------------------------
  bool available( interface::FromCMOB const &, index_t anIndex) {
    return ! theFromCMOBBuffer[anIndex];
  }

  void push( interface::FromCMOB const &, index_t anIndex, MemoryTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received on Port FromCMOB : " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    // Set the message as coming from this core
    aMessage.set( BusTag, new Mux(kSource_CMOB + anIndex));
    theFromCMOBBuffer[anIndex] = aMessage;
  }

  //Drive
  //----------
  void drive(interface::BusDrive const &) {
      DBG_(Verb, Comp(*this) (<< "BusDrive" ) ) ;
      sendOutgoingBuffers();
      busCycle();
  }

  bool hasData( MemoryMessage::MemoryMessageType aType) {
    switch (aType) {
      case MemoryMessage::Flush:                 
      case MemoryMessage::EvictDirty:            
      case MemoryMessage::EvictWritable:         
      case MemoryMessage::EvictClean:            
      case MemoryMessage::LoadReply:             
      case MemoryMessage::StoreReply:            
      case MemoryMessage::StorePrefetchReply:    
      case MemoryMessage::FetchReply:            
      case MemoryMessage::NonAllocatingStoreReq:        
      case MemoryMessage::StreamFetchWritableReply:            
      case MemoryMessage::RMWReply:              
      case MemoryMessage::CmpxReply:             
      case MemoryMessage::AtomicPreloadReply:    
      case MemoryMessage::MissReply:             
      case MemoryMessage::MissReplyWritable:     
      case MemoryMessage::MissReplyDirty:        
      case MemoryMessage::PrefetchReadReply:     
      case MemoryMessage::PrefetchWritableReply: 
      case MemoryMessage::PrefetchDirtyReply:    
      case MemoryMessage::PrefetchInsert:
      case MemoryMessage::PrefetchInsertWritable:
      case MemoryMessage::ReturnReply:
      case MemoryMessage::InvUpdateAck:
      case MemoryMessage::DownUpdateAck:
        return true;
      case MemoryMessage::LoadReq:               
      case MemoryMessage::StoreReq:              
      case MemoryMessage::StorePrefetchReq:      
      case MemoryMessage::FetchReq:              
      case MemoryMessage::RMWReq:                
      case MemoryMessage::CmpxReq:               
      case MemoryMessage::AtomicPreloadReq:      
      case MemoryMessage::FlushReq:              
      case MemoryMessage::ReadReq:               
      case MemoryMessage::WriteReq:              
      case MemoryMessage::WriteAllocate:         
      case MemoryMessage::NonAllocatingStoreReply:        
      case MemoryMessage::UpgradeReq:            
      case MemoryMessage::UpgradeAllocate:       
      case MemoryMessage::SVBClean:              
      case MemoryMessage::UpgradeReply:          
      case MemoryMessage::PrefetchReadNoAllocReq:
      case MemoryMessage::PrefetchReadAllocReq:  
      case MemoryMessage::ProbedNotPresent:
      case MemoryMessage::ProbedClean:
      case MemoryMessage::ProbedWritable:
      case MemoryMessage::ProbedDirty:
      case MemoryMessage::DownProbePresent:
      case MemoryMessage::DownProbeNotPresent:
      case MemoryMessage::StreamFetch:
      case MemoryMessage::Invalidate:
      case MemoryMessage::Downgrade:
      case MemoryMessage::Probe:
      case MemoryMessage::DownProbe:
      case MemoryMessage::ReturnReq:
      case MemoryMessage::InvalidateAck:
      case MemoryMessage::DowngradeAck:
      case MemoryMessage::PrefetchReadRedundant:
        return false;
	  default: return false;
    }
    return false;
  }

  unsigned int transferTime(MemoryTransport & trans) {
    if (theFlexus->isFastMode()) {
      return 0;
    }
    if (hasData( trans[MemoryMessageTag]->type() ) ) {
        return cfg.BusTime_Data - 1;             
    } else {
        return cfg.BusTime_NoData - 1;                   
    }
  }

  void sendOutgoingBuffers() {
    if ( theToCacheBuffer && FLEXUS_CHANNEL( ToCache ).available() ) {
      DBG_(Iface, Comp(*this) (<< "Sent on Port ToCache: " << *((*theToCacheBuffer)[MemoryMessageTag]) ) Addr((*theToCacheBuffer)[MemoryMessageTag]->address()) );
      FLEXUS_CHANNEL( ToCache ) << *theToCacheBuffer;
      theToCacheBuffer = boost::none;
    }
    while ( !theDMAMessages.empty() && FLEXUS_CHANNEL( ToCache ).available() ) {
      MemoryTransport transport;
      transport.set(MemoryMessageTag, theDMAMessages.front());       
      
      boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
      tracker->setAddress( theDMAMessages.front()->address() );
      transport.set(TransactionTrackerTag, tracker);

      DBG_(Iface, Comp(*this) (<< "Sent on Port ToCache: " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
      FLEXUS_CHANNEL( ToCache ) << transport;
      theDMAMessages.pop_front();
    }

    if ( theToMemoryBuffer && FLEXUS_CHANNEL( ToMemory ).available() ) {
      DBG_(Iface, Comp(*this) (<< "Sent on Port ToMemory: " << *((*theToMemoryBuffer)[MemoryMessageTag]) ) Addr((*theToMemoryBuffer)[MemoryMessageTag]->address()) );
      FLEXUS_CHANNEL( ToMemory ) << *theToMemoryBuffer;
      theToMemoryBuffer = boost::none;
    }
    if ( theToIndexBuffer && FLEXUS_CHANNEL( ToIndex).available() ) {
      DBG_(Iface, Comp(*this) (<< "Sent on Port ToIndex: " << *((*theToIndexBuffer)[MemoryMessageTag]) ) Addr((*theToIndexBuffer)[MemoryMessageTag]->address()) );
      FLEXUS_CHANNEL( ToIndex ) << *theToIndexBuffer;
      theToIndexBuffer = boost::none;
    }
    for (unsigned int i = 0 ; i < theToCMOBBuffer.size(); ++i) {
      if ( theToCMOBBuffer[i] && FLEXUS_CHANNEL_ARRAY( ToCMOB, i).available() ) {
        DBG_(Iface, Comp(*this) (<< "Sent on Port ToIndex: " << *((*theToCMOBBuffer[i])[MemoryMessageTag]) ) Addr((*theToCMOBBuffer[i])[MemoryMessageTag]->address()) );
        FLEXUS_CHANNEL_ARRAY( ToCMOB, i ) << *theToCMOBBuffer[i];
        theToCMOBBuffer[i] = boost::none;
      }
    }
  }

  void busUtilization() {
    switch (theBusDirection) {
      case kIdle:
        ++theCycles_Idle;
        break;
      case kToCache:
        if ( hasData((*theBusContents)[MemoryMessageTag]->type()) ) {
          ++theCycles_Cache_Data;
        } else {
          ++theCycles_Cache_Control;
        }
        break;
      case kToIndex:
        if (  hasData((*theBusContents)[MemoryMessageTag]->type()) ) {
          ++theCycles_Index_Data;
        } else {
          ++theCycles_Index_Control;
        }
        break;
      case kToCMOB:
        if ( hasData((*theBusContents)[MemoryMessageTag]->type())) {
          ++theCycles_CMOB_Data;
        } else {
          ++theCycles_CMOB_Control;
        }
        break;
      case kToMemory:
        switch((*theBusContents)[BusTag]->source) {
          case kSource_Cache:
            if ( hasData((*theBusContents)[MemoryMessageTag]->type())) {
              ++theCycles_Cache_Data;
            } else {
              ++theCycles_Cache_Control;
            }
            break;
          case kSource_Index:
            if (hasData((*theBusContents)[MemoryMessageTag]->type())) {
              ++theCycles_Index_Data;
            } else {
              ++theCycles_Index_Control;
            }
            break;
          default:
            DBG_Assert( (*theBusContents)[BusTag]->source >= kSource_CMOB);
            if (hasData((*theBusContents)[MemoryMessageTag]->type())) {
              ++theCycles_CMOB_Data;
            } else {
              ++theCycles_CMOB_Control;
            }
            break;
        }
        break;
    }

    //By message type
    if (theBusContents && (*theBusContents)[MemoryMessageTag]) {
      switch(  (*theBusContents)[MemoryMessageTag]->type() ) {
        case MemoryMessage::LoadReq:               ++theCycles_byType_LoadReq;                 break;
        case MemoryMessage::StoreReq:              ++theCycles_byType_StoreReq;                break;
        case MemoryMessage::StorePrefetchReq:      ++theCycles_byType_StorePrefetchReq;        break;
        case MemoryMessage::NonAllocatingStoreReq: ++theCycles_byType_NonAllocatingStoreReq;   break;
        case MemoryMessage::FetchReq:              ++theCycles_byType_FetchReq;                break;
        case MemoryMessage::StreamFetch:           ++theCycles_byType_StreamFetch;             break;
        case MemoryMessage::RMWReq:                ++theCycles_byType_RMWReq;                  break;
        case MemoryMessage::CmpxReq:               ++theCycles_byType_CmpxReq;                 break;
        case MemoryMessage::AtomicPreloadReq:      ++theCycles_byType_AtomicPreloadReq;        break;
        case MemoryMessage::FlushReq:              ++theCycles_byType_FlushReq;                break;
        case MemoryMessage::ReadReq:               ++theCycles_byType_ReadReq;                 break;
        case MemoryMessage::WriteReq:              ++theCycles_byType_WriteReq;                break;
        case MemoryMessage::WriteAllocate:         ++theCycles_byType_WriteAllocate;           break;
        case MemoryMessage::UpgradeReq:            ++theCycles_byType_UpgradeReq;              break;
        case MemoryMessage::UpgradeAllocate:       ++theCycles_byType_UpgradeAllocate;         break;
        case MemoryMessage::Flush:                 ++theCycles_byType_Flush;                   break;
        case MemoryMessage::EvictDirty:            ++theCycles_byType_EvictDirty;              break;
        case MemoryMessage::EvictWritable:         ++theCycles_byType_EvictWritable;           break;
        case MemoryMessage::EvictClean:            ++theCycles_byType_EvictClean;              break;

        case MemoryMessage::SVBClean:              ++theCycles_byType_SVBClean;                break;
        case MemoryMessage::LoadReply:             ++theCycles_byType_LoadReply;               break;
        case MemoryMessage::StoreReply:            ++theCycles_byType_StoreReply;              break;
        case MemoryMessage::StorePrefetchReply:    ++theCycles_byType_StorePrefetchReply;      break;
        case MemoryMessage::FetchReply:            ++theCycles_byType_FetchReply;              break;
        case MemoryMessage::NonAllocatingStoreReply:++theCycles_byType_NonAllocatingStoreReply;break;
        case MemoryMessage::RMWReply:              ++theCycles_byType_RMWReply;                break;
        case MemoryMessage::CmpxReply:             ++theCycles_byType_CmpxReply;               break;
        case MemoryMessage::AtomicPreloadReply:    ++theCycles_byType_AtomicPreloadReply;      break;
        case MemoryMessage::MissReply:             ++theCycles_byType_MissReply;               break;
        case MemoryMessage::MissReplyWritable:     ++theCycles_byType_MissReplyWritable;       break;
        case MemoryMessage::MissReplyDirty:        ++theCycles_byType_MissReplyDirty;          break;
        case MemoryMessage::UpgradeReply:          ++theCycles_byType_UpgradeReply;            break;
        case MemoryMessage::PrefetchReadNoAllocReq:++theCycles_byType_PrefetchReadNoAllocReq;  break;
        case MemoryMessage::PrefetchReadAllocReq:  ++theCycles_byType_PrefetchReadAllocReq;    break;
        case MemoryMessage::PrefetchReadReply:     ++theCycles_byType_PrefetchReadReply;       break;
        case MemoryMessage::PrefetchWritableReply: ++theCycles_byType_PrefetchWritableReply;   break;
        case MemoryMessage::PrefetchDirtyReply:    ++theCycles_byType_PrefetchDirtyReply;      break;
        case MemoryMessage::StreamFetchWritableReply: ++theCycles_byType_StreamFetchWritableReply;   break;

        case MemoryMessage::PrefetchInsert:
        case MemoryMessage::PrefetchInsertWritable:
        case MemoryMessage::ProbedNotPresent:
        case MemoryMessage::ProbedClean:
        case MemoryMessage::ProbedWritable:
        case MemoryMessage::ProbedDirty:
        case MemoryMessage::DownProbePresent:
        case MemoryMessage::DownProbeNotPresent:
        case MemoryMessage::ReturnReply:
        case MemoryMessage::Invalidate:
        case MemoryMessage::Downgrade:
        case MemoryMessage::Probe:
        case MemoryMessage::DownProbe:
        case MemoryMessage::ReturnReq:
        case MemoryMessage::InvalidateAck:
        case MemoryMessage::InvUpdateAck:
        case MemoryMessage::DowngradeAck:
        case MemoryMessage::DownUpdateAck:
        case MemoryMessage::PrefetchReadRedundant:
		case MemoryMessage::StreamFetchRejected:
          //Should never go over bus.
          DBG_Assert(false, ( << "Unexpected message type on bus" ) );
          break;
      }
    }
  }

  void busCycle() {
    //If a bus transfer is in process, continue it
    if (theBusCountdown > 0) {
      DBG_Assert( theBusDirection != kIdle );
      --theBusCountdown;

    } else {
      DBG_Assert( theBusDirection == kIdle );

      //The bus is available, initiate a transfer, if there are any to start
      //Memory gets priority
      if ( ! theFromMemory_ToCacheBuffer.empty() && ! theToCacheBuffer) {
         theBusDirection = kToCache;
         theBusContents = theFromMemory_ToCacheBuffer.dequeue();
         theBusCountdown = transferTime( *theBusContents );
      }
      
      //Next is Cache Requests
      if (theBusDirection == kIdle && theFromCache_RequestBuffer) {
        if (! theToMemoryBuffer) {
          theBusDirection = kToMemory;
          theBusContents = theFromCache_RequestBuffer;
          theFromCache_RequestBuffer = boost::none;
          theBusCountdown = transferTime( *theBusContents );
        }
      }

      //Next is Cache Prefetches
      if (theBusDirection == kIdle && theFromCache_PrefetchBuffer) {
        if (! theToMemoryBuffer) {
          theBusDirection = kToMemory;
          theBusContents = theFromCache_PrefetchBuffer;
          theFromCache_PrefetchBuffer = boost::none;
          theBusCountdown = transferTime( *theBusContents );
        }
      }

      //Next is Cache Snoops
      if (theBusDirection == kIdle && theFromCache_SnoopBuffer) {
        if (! theToMemoryBuffer) {
          theBusDirection = kToMemory;
          theBusContents = theFromCache_SnoopBuffer;
          theFromCache_SnoopBuffer = boost::none;
          theBusCountdown = transferTime( *theBusContents );
        }
      }

      if (theBusDirection == kIdle && ! theFromMemory_ToIndexBuffer.empty() && ! theToIndexBuffer) {
        theBusDirection = kToIndex;
        theBusContents = theFromMemory_ToIndexBuffer.dequeue();
        theBusCountdown = transferTime( *theBusContents );
      }

      if (theBusDirection == kIdle && ! theFromMemory_ToCMOBBuffer.empty() ) {
        DBG_Assert(theFromMemory_ToCMOBBuffer.peek()[BusTag]->source >= kSource_CMOB);
        int cmob = theFromMemory_ToCMOBBuffer.peek()[BusTag]->source - kSource_CMOB;
		DBG_Assert(cmob >= 0);
        DBG_Assert( cmob <  (static_cast<int> (theToCMOBBuffer.size())));
        if (! theToCMOBBuffer[cmob]) {
          theBusDirection = kToCMOB;
          theBusContents = theFromMemory_ToCMOBBuffer.dequeue();
          theBusCountdown = transferTime( *theBusContents );
        }
      }

      //Next is CMOB Requests
      if (theBusDirection == kIdle && ! theToMemoryBuffer) {
        for (unsigned int i = 0; i < theFromCMOBBuffer.size(); ++i) {
          if ( theFromCMOBBuffer[i] ) {
            theBusDirection = kToMemory;
            theBusContents = theFromCMOBBuffer[i];
            theFromCMOBBuffer[i] = boost::none;
            theBusCountdown = transferTime( *theBusContents );
            break;
          }
        }
      }

      //Next is Index Requests
      if (theBusDirection == kIdle && theFromIndexBuffer ) {
        if (! theToMemoryBuffer) {
          theBusDirection = kToMemory;
          theBusContents = theFromIndexBuffer;
          theFromIndexBuffer = boost::none;
          theBusCountdown = transferTime( *theBusContents );
        }
      }

    }

    busUtilization();

    if ( theBusCountdown == 0 ) {
      switch (theBusDirection) {
        case kIdle:
          break; //Nothing to do
        case kToMemory:
          DBG_Assert( ! theToMemoryBuffer );
          theToMemoryBuffer = theBusContents;
          theBusContents = boost::none;
          break;
        case kToCache:
          DBG_Assert( ! theToCacheBuffer );
          theToCacheBuffer = theBusContents;
          theBusContents = boost::none;
          break;
        case kToIndex:
          DBG_Assert( ! theToIndexBuffer );
          theToIndexBuffer = theBusContents;
          theBusContents = boost::none;
          break;
        case kToCMOB: {
          int cmob = (*theBusContents)[BusTag]->source - kSource_CMOB;
		  DBG_Assert( cmob >= 0);
          DBG_Assert( cmob < ( static_cast<int> (theToCMOBBuffer.size())));
          DBG_Assert( ! theToCMOBBuffer[cmob] );
          theToCMOBBuffer[cmob] = theBusContents;
          theBusContents = boost::none;
          break;
        }
      }

      theBusDirection = kIdle;
    }
  }

  void toDMA(boost::intrusive_ptr<MemoryMessage> aMessage) { 
    DBG_( Dev, ( << "DMA transaction: " << *aMessage ) );
    //theDMAMessages.push_back(aMessage);
  }

    void createDMATracer() {
      API::conf_object_t * dma_map_object = API::SIM_get_object( "dma_mem" );
      API::SIM_clear_exception();
      if (! dma_map_object) {
        bool client_server = false;
        DBG_( Dev, ( << "Creating DMA map object" ) );
        API::conf_object_t * cpu0_mem = API::SIM_get_object("cpu0_mem");
        API::conf_object_t * cpu0 = API::SIM_get_object("cpu0");
        if ( ! cpu0_mem ) {
          client_server = true;
          cpu0_mem = API::SIM_get_object("server_server_cpu0_mem");
          cpu0 = API::SIM_get_object("server_cpu0");
          if (! cpu0_mem) {
            DBG_( Crit, ( << "Unable to connect DMA because there is no cpu0_mem." ) );
            return;
          }
        }

        API::attr_value_t map_key = API::SIM_make_attr_string("map");
        API::attr_value_t map_value = API::SIM_get_attribute(cpu0_mem, "map");
        API::attr_value_t map_pair = API::SIM_make_attr_list(2, map_key, map_value);
        API::attr_value_t map_list = API::SIM_make_attr_list(1, map_pair);
        API::conf_class_t * memory_space = API::SIM_get_class( "memory-space" );
        dma_map_object = SIM_create_object( memory_space, "dma_mem", map_list );
        DBG_Assert( dma_map_object );

        API::conf_class_t * schizo = API::SIM_get_class( "serengeti-schizo" );
        API::attr_value_t dma_attr;
        dma_attr.kind = API::Sim_Val_Object;
        dma_attr.u.object = dma_map_object;
		
	#if SIM_VERSION > 1300
        API::attr_value_t all_objects = Simics::API::SIM_get_all_objects();
		DBG_Assert(all_objects.kind == Simics::API::Sim_Val_List);
        for (int i = 0; i < all_objects.u.list.size; ++i) {
          if (all_objects.u.list.vector[i].u.object->class_data == schizo && API::SIM_get_attribute( all_objects.u.list.vector[i].u.object, "queue").u.object == cpu0 ) {
            API::SIM_set_attribute(all_objects.u.list.vector[i].u.object, "memory_space", &dma_attr );
          }
        }
	#else
        API::object_vector_t all_objects = Simics::API::SIM_all_objects();
        for (int i = 0; i < all_objects.size; ++i) {
          if (all_objects.vec[i]->class_data == schizo && API::SIM_get_attribute( all_objects.vec[i], "queue").u.object == cpu0 ) {
            API::SIM_set_attribute(all_objects.vec[i], "memory_space", &dma_attr );
          }
        }
	#endif
      }

      //Create SimicsTracer Factory
      Simics::Factory<DMATracer> tracer_factory;
      API::conf_class_t * trace_class = tracer_factory.getSimicsClass();
      registerDMAInterface(trace_class);

      std::string tracer_name("dma-tracer");
      theDMATracer = tracer_factory.create(tracer_name);
      DBG_( Crit, ( << "Connecting to DMA memory map" ) );
      theDMATracer->init(dma_map_object, ll::bind( &nCmpBus::CmpBusComponent::toDMA, this, ll::_1) );
    }

    void registerDMAInterface(Simics::API::conf_class_t * trace_class) {
      API::timing_model_interface_t *timing_interface;
      timing_interface = new API::timing_model_interface_t(); //LEAKS - Need to fix
      timing_interface->operate = &Simics::make_signature_from_addin_fn2<API::operate_func_t>::with<DMATracer,DMATracerImpl,&DMATracerImpl::dma_mem_hier_operate>::trampoline;
      API::SIM_register_interface(trace_class, "timing-model", timing_interface);
    }

};

} //End Namespace nCmpCache

FLEXUS_COMPONENT_INSTANTIATOR( CmpBus, nCmpBus );
FLEXUS_PORT_ARRAY_WIDTH( CmpBus, FromCMOB) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH( CmpBus, ToCMOB) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT CmpBus

  #define DBG_Reset
  #include DBG_Control()

