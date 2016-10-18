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

#include <components/CmpCache/CmpCache.hpp>


#include <boost/scoped_ptr.hpp>
#include <core/performance/profile.hpp>
#include <core/simics/configuration_api.hpp>


#include "../Cache/CacheController.hpp"

  #define DBG_DefineCategories CmpCache
  #define DBG_SetDefaultOps AddCat(CmpCache)
  #include DBG_Control()


#define FLEXUS_BEGIN_COMPONENT CmpCache
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()


namespace nCmpCache {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;
using namespace nCache;

using boost::scoped_ptr;


class FLEXUS_COMPONENT(CmpCache) {
  FLEXUS_COMPONENT_IMPL(CmpCache);

 private:

  std::auto_ptr<CacheController> theController; //Deleted automagically when the cache goes away
  unsigned int interleavingBlockShiftBits;

 public:
  FLEXUS_COMPONENT_CONSTRUCTOR(CmpCache)
    : base ( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theBusDirection(kIdle)
    {}

  enum eDirection
    { kIdle
    , kToBackSideIn_Reply
    , kToBackSideIn_Request
    , kToBackSideOut_Request
    , kToBackSideOut_Snoop
    };

  unsigned int theBusTxCountdown;
  eDirection theBusDirection;
  MemoryTransport theBusContents;
  boost::optional< MemoryTransport> theBackSideInReplyBuffer;
  boost::optional< MemoryTransport> theBackSideInRequestBuffer;

  bool isQuiesced() const {
    return theBusDirection == kIdle
      && !theBackSideInReplyBuffer
      && !theBackSideInRequestBuffer
      && (theController.get() ? theController->isQuiesced() : true )
      ;
  }

  void saveState(std::string const & aName) {
    theController->saveState( aName );
  }

  void loadState(std::string const & aName) {
    theController->loadState( aName );
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
    else {
      DBG_Assert(false, ( << "Unknown L2 design") );
      aPlacement = kSharedCache; // compiler happy
    }

    DBG_Assert(!cfg.PrivateWithASR || (aPlacement == kPrivateCache),( << "If using asr_enable, cache placement scheme must be 'private'"));  /* CMU-ONLY */
    theController.reset(new CacheController(statName(),
                                            cfg.Cores*2,
                                            cfg.Size,
                                            cfg.Associativity,
                                            cfg.BlockSize,
                                            cfg.PageSize,
                                            cfg.Banks,
                                            cfg.Ports,
                                            cfg.TagLatency,
                                            cfg.TagIssueLatency,
                                            cfg.DuplicateTagLatency,
                                            cfg.DuplicateTagIssueLatency,
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
                                            cacheTypeName ( "piranha" ),
                                            cfg.TraceAddress,
                                            true, /* cfg.IsPiranhaCache */
                                            cfg.NumL2Tiles,  // Number of slices, used to reserve bank bits without instanting a banked component here
                                            flexusIndex(),   // Slice number, so that block addresses can be created correctly
                                            aPlacement,
                                            cfg.PrivateWithASR,  /* CMU-ONLY */
                                            cfg.AllowOffChipStreamFetch
                                            ));

    DBG_Assert( cfg.BusTime_Data > 0);
    DBG_Assert( cfg.BusTime_NoData > 0);

  	interleavingBlockShiftBits = 0;
  	for (unsigned int i = 1; i < cfg.L2InterleavingGranularity; i*=2) {
  		++interleavingBlockShiftBits;
  	}
  }

#define mapIDPorts(IDX,IS_DATA)   ((IS_DATA)?(((IDX)*2)+1):((IDX)*2))

  //Ports
  //======
  //FrontSideInI_Snoop PushInput port array
  //---------------------------------------
  bool available( interface::FrontSideInI_Snoop const &, index_t anIndex) {
    return ! theController->FrontSideIn_Snoop[mapIDPorts(anIndex, false)].full();
  }

  void push( interface::FrontSideInI_Snoop const &, index_t anIndex, MemoryTransport & aMessage) {
    DBG_Assert(aMessage[MemoryMessageTag]->usesSnoopChannel());
    //// only for distributed shared cache
    //// DBG_Assert(getL2Tile(aMessage[MemoryMessageTag]->address()) == flexusIndex());
    
    // Set the message as coming from this core
    aMessage[MemoryMessageTag]->coreIdx() = mapIDPorts(anIndex,false);
    DBG_Assert(! theController->FrontSideIn_Snoop[mapIDPorts(anIndex,false)].full());
    DBG_(Trace, Comp(*this) ( << "Received on Port FrontSideIn[" << mapIDPorts(anIndex,false) << "](Snoop): " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    if (aMessage[TransactionTrackerTag]) {
      aMessage[TransactionTrackerTag]->setDelayCause(name(), "Front Rx");
    }

    theController->FrontSideIn_Snoop[mapIDPorts(anIndex,false)].enqueue(aMessage);
  }

  //FrontSideInD_Snoop PushInput port array
  //---------------------------------------
  bool available( interface::FrontSideInD_Snoop const &, index_t anIndex) {
    return ! theController->FrontSideIn_Snoop[mapIDPorts(anIndex, true)].full();
  }
  void push( interface::FrontSideInD_Snoop const &, index_t anIndex, MemoryTransport & aMessage) {
  	DBG_Assert(aMessage[MemoryMessageTag]->usesSnoopChannel());
        //// only for distributed shared cache
  	//// DBG_Assert(getL2Tile(aMessage[MemoryMessageTag]->address()) == flexusIndex());
  	
    // Set the message as coming from this core
    aMessage[MemoryMessageTag]->coreIdx() = mapIDPorts(anIndex, true);
    DBG_Assert(! theController->FrontSideIn_Snoop[mapIDPorts(anIndex, true)].full());
    DBG_(Trace, Comp(*this) ( << "Received on Port FrontSideIn[" << mapIDPorts(anIndex, true) << "](Snoop): " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    if (aMessage[TransactionTrackerTag]) {
      aMessage[TransactionTrackerTag]->setDelayCause(name(), "Front Rx");
    }
    theController->FrontSideIn_Snoop[mapIDPorts(anIndex, true)].enqueue(aMessage);
  }

  //FrontSideInI_Request PushInput port array
  //---------------------------------------
  bool available( interface::FrontSideInI_Request const &, index_t anIndex) {
    return ! theController->FrontSideIn_Request[mapIDPorts(anIndex, false)].full();
  }
  void push( interface::FrontSideInI_Request const &, index_t anIndex, MemoryTransport & aMessage) {
  	DBG_Assert(!(aMessage[MemoryMessageTag]->usesSnoopChannel()));
        //// only for distributed shared cache
  	//// DBG_Assert(getL2Tile(aMessage[MemoryMessageTag]->address()) == flexusIndex());
  	
      DBG_Assert(! theController->FrontSideIn_Request[mapIDPorts(anIndex, false)].full(), ( << statName() ) );
      // Set the message as coming from this core
      aMessage[MemoryMessageTag]->coreIdx() = mapIDPorts(anIndex, false);
      DBG_(Trace, Comp(*this) ( << "Received on Port FrontSideIn[" << mapIDPorts(anIndex, false) << "](Request): " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
      if (aMessage[TransactionTrackerTag]) {
        aMessage[TransactionTrackerTag]->setDelayCause(name(), "Front Rx");
      }

      theController->FrontSideIn_Request[mapIDPorts(anIndex, false)].enqueue(aMessage);
      
      // Notify the PowerTracker that there was a cache access
	  bool garbage = true;
      FLEXUS_CHANNEL( RequestSeen) << garbage;
  }

  //FrontSideInD_Request PushInput port array
  //---------------------------------------
  bool available( interface::FrontSideInD_Request const &, index_t anIndex) {
      return ! theController->FrontSideIn_Request[mapIDPorts(anIndex, true)].full();
  }
  void push( interface::FrontSideInD_Request const &, index_t anIndex, MemoryTransport & aMessage) {
  	DBG_Assert(!(aMessage[MemoryMessageTag]->usesSnoopChannel()));
        //// only for distributed shared cache
  	//// DBG_Assert(getL2Tile(aMessage[MemoryMessageTag]->address()) == flexusIndex());
  	
    DBG_Assert(! theController->FrontSideIn_Request[mapIDPorts(anIndex, true)].full(), ( << statName() ) );
    // Set the message as coming from this core
    aMessage[MemoryMessageTag]->coreIdx() = mapIDPorts(anIndex, true);
    DBG_(Trace, Comp(*this) ( << "Received on Port FrontSideIn[" << mapIDPorts(anIndex, true) << "](Request): " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    if (aMessage[TransactionTrackerTag]) {
      aMessage[TransactionTrackerTag]->setDelayCause(name(), "Front Rx");
    }

    theController->FrontSideIn_Request[mapIDPorts(anIndex, true)].enqueue(aMessage);
    
    // Notify the PowerTracker that there was a cache access
	bool garbage = true;
    FLEXUS_CHANNEL( RequestSeen) << garbage;
  }

  //FrontSideInD_Prefetch PushInput port array
  //---------------------------------------
  bool available( interface::FrontSideInD_Prefetch const &, index_t anIndex) {
      return ! theController->FrontSideIn_Prefetch[mapIDPorts(anIndex, true)].full();
  }
  void push( interface::FrontSideInD_Prefetch const &, index_t anIndex, MemoryTransport & aMessage) {
    DBG_Assert(! theController->FrontSideIn_Prefetch[mapIDPorts(anIndex, true)].full(), ( << statName() ) );
    // Set the message as coming from this core
    aMessage[MemoryMessageTag]->coreIdx() = mapIDPorts(anIndex, true);
    DBG_(Trace, Comp(*this) ( << "Received on Port FrontSideIn[" << mapIDPorts(anIndex, true) << "](Prefetch): " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    if (aMessage[TransactionTrackerTag]) {
      aMessage[TransactionTrackerTag]->setDelayCause(name(), "Front Rx");
    }

    theController->FrontSideIn_Prefetch[mapIDPorts(anIndex, true)].enqueue(aMessage);
    
  }

  //BackSideIn_Reply PushInput port
  //-------------------------
  bool available( interface::BackSideIn_Reply const &) {
    if (cfg.BypassBus) {
      return ! theController->BackSideIn_Reply[0].full();      
    } else {
      return ! theBackSideInReplyBuffer;
    }
  }
  void push( interface::BackSideIn_Reply const &, MemoryTransport & aMessage) {
    if (cfg.BypassBus) {
      DBG_Assert(! theController->BackSideIn_Reply[0].full(), ( << statName() ) );
      // Set the message as coming from this core
      DBG_(Trace, Comp(*this) ( << "Received on Port BackSideIn_Reply: " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
      theController->BackSideIn_Reply[0].enqueue(aMessage);
    } else {
      DBG_Assert(! theBackSideInReplyBuffer);
      theBackSideInReplyBuffer = aMessage;
    }
  }

  //BackSideIn_Request PushInput port
  //-------------------------
  bool available( interface::BackSideIn_Request const &) {
    return ! theBackSideInReplyBuffer;
  }
  void push( interface::BackSideIn_Request const &, MemoryTransport & aMessage) {
    DBG_Assert(! theBackSideInReplyBuffer);
    theBackSideInReplyBuffer = aMessage;
    
    // Notify the PowerTracker that there was a cache access
	bool garbage = true;
    FLEXUS_CHANNEL( RequestSeen) << garbage;
  }

  //CacheDrive
  //----------
  void drive(interface::CacheDrive const &) {
  	  bool garbage = true;
	  FLEXUS_CHANNEL( ClockTickSeen ) << garbage;
  	
      DBG_(Verb, Comp(*this) (<< "CacheDrive" ) ) ;
      theController->processMessages();
      if (cfg.BypassBus) {
        exchangeMessages(); 
      } else {
        busCycle();
      }
  }

  unsigned int transferTime(MemoryTransport & trans) {
    if (theFlexus->isFastMode()) {
      return 0;
    }
    return ( (trans[MemoryMessageTag]->reqSize() > 0) ? cfg.BusTime_Data : cfg.BusTime_NoData ) - 1;
  }

  void exchangeMessages() {
    for ( int i = 0; i < cfg.Cores*2; i=i+2 ) {
      //Send as much on FrontSideOutI as possible for each core
      while(! theController->FrontSideOut[i].empty() && FLEXUS_CHANNEL_ARRAY( FrontSideOutI_Reply, (i/2)).available()) {
        MemoryTransport transport = theController->FrontSideOut[i].dequeue();
        transport[MemoryMessageTag]->dstream() = false;
        DBG_(Trace,
             Comp(*this)
             Addr(transport[MemoryMessageTag]->address())
             (<< "Sent on Port FrontSideOutI_Reply[" << i << "]: " << *(transport[MemoryMessageTag]) )
             );
        FLEXUS_CHANNEL_ARRAY(FrontSideOutI_Reply, (i/2)) << transport;
      }
    }

    for ( int i = 1; i < cfg.Cores*2; i=i+2 ) {
      //Send as much on FrontSideOutD as possible for each core
      while(! theController->FrontSideOut[i].empty() && FLEXUS_CHANNEL_ARRAY( FrontSideOutD_Reply, (i/2)).available()) {
        MemoryTransport transport = theController->FrontSideOut[i].dequeue();
        transport[MemoryMessageTag]->dstream() = true;
        DBG_(Trace,
             Comp(*this) 
             Addr(transport[MemoryMessageTag]->address())
             (<< "Sent on Port FrontSideOutD_Reply[" << i << "]: " << *(transport[MemoryMessageTag]) )
             );
        FLEXUS_CHANNEL_ARRAY( FrontSideOutD_Reply, (i/2)) << transport;
      }
    }

    while (! theController->BackSideOut_Snoop.empty() && FLEXUS_CHANNEL(BackSideOut_Snoop).available() ) {
        MemoryTransport transport = theController->BackSideOut_Snoop.dequeue();
        DBG_(Trace, Comp(*this) (<< "Sent on Port BackSideOut(Snoop): " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
        FLEXUS_CHANNEL(BackSideOut_Snoop) << transport;      
    }

    while (! theController->BackSideOut_Request.empty() && FLEXUS_CHANNEL(BackSideOut_Request).available() ) {
        MemoryTransport transport = theController->BackSideOut_Request.dequeue();
        DBG_(Trace, Comp(*this) (<< "Sent on Port BackSideOut(Request): " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
        FLEXUS_CHANNEL(BackSideOut_Request) << transport;      
    }

    while (! theController->BackSideOut_Prefetch.empty() && FLEXUS_CHANNEL(BackSideOut_Prefetch).available() ) {
        MemoryTransport transport = theController->BackSideOut_Prefetch.dequeue();
        DBG_(Trace, Comp(*this) (<< "Sent on Port BackSideOut(Prefetch): " << *(transport[MemoryMessageTag]) ) Addr(transport[MemoryMessageTag]->address()) );
        FLEXUS_CHANNEL(BackSideOut_Prefetch) << transport;      
    }
  }

  void busCycle() {

    for ( int i = 0; i < cfg.Cores*2; i=i+2 ) {
      //Send as much on FrontSideOutI as possible for each core
      while( ! theController->FrontSideOut[i].empty() 
             && FLEXUS_CHANNEL_ARRAY( FrontSideOutI_Reply, (i/2)).available()
             && FLEXUS_CHANNEL_ARRAY( FrontSideOutI_Reply, (i/2)).available()
           )
      {
        MemoryTransport transport = theController->FrontSideOut[i].dequeue();
        transport[MemoryMessageTag]->dstream() = false;
        if (transport[MemoryMessageTag]->isRequest()) {
          DBG_(Trace,
               Comp(*this)
               Addr(transport[MemoryMessageTag]->address())
               (<< "Sent on Port FrontSideOutI_Reply[" << i << "]: " << *(transport[MemoryMessageTag]) )
               );
          FLEXUS_CHANNEL_ARRAY(FrontSideOutI_Reply, (i/2)) << transport;
        }
        else {
          DBG_(Trace,
               Comp(*this)
               Addr(transport[MemoryMessageTag]->address())
               (<< "Sent on Port FrontSideOutI_Reply[" << i << "]: " << *(transport[MemoryMessageTag]) )
               );
          FLEXUS_CHANNEL_ARRAY(FrontSideOutI_Reply, (i/2)) << transport;
        }  

      }
    }

    for ( int i = 1; i < cfg.Cores*2; i=i+2 ) {
      //Send as much on FrontSideOutD as possible for each core
      while( ! theController->FrontSideOut[i].empty() 
             && FLEXUS_CHANNEL_ARRAY( FrontSideOutD_Reply, (i/2)).available()
             && FLEXUS_CHANNEL_ARRAY( FrontSideOutD_Reply, (i/2)).available()
           )
      {
        MemoryTransport transport = theController->FrontSideOut[i].dequeue();
        transport[MemoryMessageTag]->dstream() = true;
        //fixme
        /*
        DBG_(Trace,
             Comp(*this)
             Addr(transport[MemoryMessageTag]->address())
             (<< "Sent on Port FrontSideOutD_Reply[" << i << "]: " << *(transport[MemoryMessageTag]) )
             );
        FLEXUS_CHANNEL_ARRAY( FrontSideOutD_Reply, (i/2)) << transport;
        */
        if (transport[MemoryMessageTag]->isRequest()) {
          DBG_(Trace,
               Comp(*this)
               Addr(transport[MemoryMessageTag]->address())
               (<< "Sent on Port FrontSideOutD_Reply[" << i << "]: " << *(transport[MemoryMessageTag]) )
               );
          FLEXUS_CHANNEL_ARRAY( FrontSideOutD_Reply, (i/2)) << transport;
        }
        else {
          DBG_(Trace,
               Comp(*this)
               Addr(transport[MemoryMessageTag]->address())
               (<< "Sent on Port FrontSideOutD_Reply[" << i << "]: " << *(transport[MemoryMessageTag]) )
               );
          FLEXUS_CHANNEL_ARRAY( FrontSideOutD_Reply, (i/2)) << transport;
        }
      }
    }

    //If a bus transfer is in process, continue it
    if (theBusTxCountdown > 0) {
      DBG_Assert( theBusDirection != kIdle );
      --theBusTxCountdown;
    } else {
      DBG_Assert( theBusDirection == kIdle );

      //The bus is available, initiate a transfer, if there are any to start
      if (theBackSideInReplyBuffer && !theController->BackSideIn_Reply[0].full()) {
        theBusContents = *theBackSideInReplyBuffer;
        theBackSideInReplyBuffer = boost::none;
        theBusDirection = kToBackSideIn_Reply;
      } else if (theBackSideInRequestBuffer && !theController->BackSideIn_Request[0].full()) {
        theBusContents = *theBackSideInRequestBuffer;
        theBackSideInRequestBuffer = boost::none;
        theBusDirection = kToBackSideIn_Request;
      } else if (!theController->BackSideOut_Snoop.empty() && FLEXUS_CHANNEL(BackSideOut_Snoop).available() ) {
        theBusContents = theController->BackSideOut_Snoop.dequeue();
        theBusDirection = kToBackSideOut_Snoop;
        //A cache will NOT send a request out if there are any snoops waiting to
        //be sent.  This is to ensure that a request message cannot overtake a
        //snoop message, which can break correctness.
      } else if (theController->BackSideOut_Snoop.empty() && !theController->BackSideOut_Request.empty() && FLEXUS_CHANNEL(BackSideOut_Request).available() ) {
        theBusContents = theController->BackSideOut_Request.dequeue();
        theBusDirection = kToBackSideOut_Request;
      }
      if (theBusDirection != kIdle) {
        theBusTxCountdown = transferTime( theBusContents );
      }
    }

    if ( theBusTxCountdown == 0 ) {
      switch (theBusDirection) {

      case kIdle:
        break; //Nothing to do

      case kToBackSideIn_Reply:
        DBG_Assert( ! theController->BackSideIn_Reply[0].full(),
                    ( << *(theBusContents[MemoryMessageTag]) ) );
        DBG_(Trace,
             Comp(*this)
             Addr(theBusContents[MemoryMessageTag]->address())
             (<< "Received on Port BackSideIn_Reply: " << *(theBusContents[MemoryMessageTag]) )
            );
        theController->BackSideIn_Reply[0].enqueue( theBusContents );
        break;

      case kToBackSideIn_Request:
        DBG_Assert( ! theController->BackSideIn_Request[0].full(),
                    ( << *(theBusContents[MemoryMessageTag]) ) );
        DBG_(Trace,
             Comp(*this)
             Addr(theBusContents[MemoryMessageTag]->address())
             (<< "Received on Port BackSideIn_Request: " << *(theBusContents[MemoryMessageTag]) )
            );
        theController->BackSideIn_Request[0].enqueue( theBusContents );
        break;

      case kToBackSideOut_Snoop:
        DBG_Assert( FLEXUS_CHANNEL(BackSideOut_Snoop).available(),
                    ( << *(theBusContents[MemoryMessageTag]) )  );
        DBG_(Trace,
             Comp(*this)
             Addr(theBusContents[MemoryMessageTag]->address())
             (<< "Sent on Port BackSideOut(Snoop): " << *(theBusContents[MemoryMessageTag]) )
            );
        FLEXUS_CHANNEL(BackSideOut_Snoop) << theBusContents;
        break;

      case kToBackSideOut_Request:
        DBG_Assert( FLEXUS_CHANNEL(BackSideOut_Request).available(),
                    ( << *(theBusContents[MemoryMessageTag]) )  );
        DBG_(Trace,
             Comp(*this)
             Addr(theBusContents[MemoryMessageTag]->address())
             (<< "Sent on Port BackSideOut(Request): " << *(theBusContents[MemoryMessageTag]) )
            );
        FLEXUS_CHANNEL(BackSideOut_Request) << theBusContents;
        break;

      }

      theBusDirection = kIdle;
    }
  }

  unsigned int getL2Tile(PhysicalMemoryAddress theAddress) {
	return (theAddress >> interleavingBlockShiftBits) % cfg.NumL2Tiles;
  }
};

} //End Namespace nCmpCache

FLEXUS_COMPONENT_INSTANTIATOR( CmpCache, nCmpCache );
FLEXUS_PORT_ARRAY_WIDTH( CmpCache, FrontSideInI_Snoop) { return (cfg.Cores ? cfg.Cores : Flexus::Core::ComponentManager::getComponentManager().systemWidth()); }
FLEXUS_PORT_ARRAY_WIDTH( CmpCache, FrontSideInD_Snoop) { return (cfg.Cores ? cfg.Cores : Flexus::Core::ComponentManager::getComponentManager().systemWidth()); }
FLEXUS_PORT_ARRAY_WIDTH( CmpCache, FrontSideInI_Request) { return (cfg.Cores ? cfg.Cores : Flexus::Core::ComponentManager::getComponentManager().systemWidth()); }
FLEXUS_PORT_ARRAY_WIDTH( CmpCache, FrontSideInD_Request) { return (cfg.Cores ? cfg.Cores : Flexus::Core::ComponentManager::getComponentManager().systemWidth()); }
FLEXUS_PORT_ARRAY_WIDTH( CmpCache, FrontSideInD_Prefetch) { return (cfg.Cores ? cfg.Cores : Flexus::Core::ComponentManager::getComponentManager().systemWidth()); }
FLEXUS_PORT_ARRAY_WIDTH( CmpCache, FrontSideOutI_Reply) { return (cfg.Cores ? cfg.Cores : Flexus::Core::ComponentManager::getComponentManager().systemWidth()); }
FLEXUS_PORT_ARRAY_WIDTH( CmpCache, FrontSideOutD_Reply) { return (cfg.Cores ? cfg.Cores : Flexus::Core::ComponentManager::getComponentManager().systemWidth()); }
FLEXUS_PORT_ARRAY_WIDTH( CmpCache, FrontSideOutI_Request) { return (cfg.Cores ? cfg.Cores : Flexus::Core::ComponentManager::getComponentManager().systemWidth()); }
FLEXUS_PORT_ARRAY_WIDTH( CmpCache, FrontSideOutD_Request) { return (cfg.Cores ? cfg.Cores : Flexus::Core::ComponentManager::getComponentManager().systemWidth()); }

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT CmpCache

  #define DBG_Reset
  #include DBG_Control()

