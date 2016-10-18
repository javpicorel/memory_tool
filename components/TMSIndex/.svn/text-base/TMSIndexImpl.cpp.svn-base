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

#include <components/TMSIndex/TMSIndex.hpp>

#define FLEXUS_BEGIN_COMPONENT TMSIndex
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()


#include <fstream>

#include "index.hpp"

#include <components/Common/MessageQueues.hpp>

#include <core/stats.hpp>
#include <core/performance/profile.hpp>
#include <core/boost_extensions/padded_string_cast.hpp>
using boost::padded_string_cast;

  #define DBG_DefineCategories TMSIndex
  #define DBG_SetDefaultOps AddCat(TMSIndex)
  #include DBG_Control()


namespace nTMSIndex{

using namespace Flexus::Core;
namespace Stat = Flexus::Stat;
using namespace Flexus::SharedTypes;

using namespace nMessageQueues;

typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;


class FLEXUS_COMPONENT(TMSIndex) {
  FLEXUS_COMPONENT_IMPL(TMSIndex);

  std::list< IndexMessage > theMessagesIn;
  std::list< IndexMessage > theMessagesOut;
  
  MessageQueue< MemoryTransport > theMemoryMessagesIn;
  MessageQueue< MemoryTransport > theMemoryReadsOut;
  MessageQueue< MemoryTransport > theMemoryWritesOut;

  boost::scoped_ptr< CuckooIndex > theIndex;

  std::multimap< MemoryAddress, IndexMessage > thePendingReads;   

  Stat::StatCounter theDroppedWrites;
  Stat::StatCounter theDroppedReads;

  std::list<MemoryAddress> thePrefix;

 public:
   FLEXUS_COMPONENT_CONSTRUCTOR(TMSIndex)
     : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
     , theDroppedWrites( statName() + "-DroppedInserts" )
     , theDroppedReads( statName() + "-DroppedLookup" )
  { }

  bool isQuiesced() const {
    return  theMessagesIn.empty()
      &&    theMessagesOut.empty()
      &&    theMemoryReadsOut.empty()
      &&    theMemoryWritesOut.empty()
      &&    theMemoryMessagesIn.empty()
      ;
  }

  void saveState(std::string const & aDirName) { }

  void loadState(std::string const & aDirName) {  
    theIndex->load(aDirName + "/");
  }


  // Initialization
  void initialize() {
    theIndex.reset( new CuckooIndex( cfg.IndexName, cfg.BucketsLog2, cfg.BucketSize ) );
    theMemoryMessagesIn.setSize(cfg.QueueSizes); 
    theMemoryReadsOut.setSize(cfg.QueueSizes); 
    theMemoryWritesOut.setSize(cfg.QueueSizes); 
  }

  // Ports
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(TMSc_Request);
  void push(interface::TMSc_Request const &, index_t anIndex, IndexMessage & aMessage) {
    DBG_(Iface, Comp(*this) ( << aMessage) );
    theMessagesIn.push_back(aMessage);
  }

  bool available(interface::FromMemory const &) {
    return ! theMemoryMessagesIn.full(); 
  }
  void push(interface::FromMemory const &,  MemoryTransport & trans) {
    DBG_(Iface, Comp(*this) ( << *(trans[MemoryMessageTag]) ) );
    DBG_Assert( ! theMemoryMessagesIn.full() );
    theMemoryMessagesIn.enqueue(trans);
  }


  // Drive Interfaces
  void drive(interface::IndexDrive const &) {
    DBG_(VVerb, Comp(*this) (<< "IndexDrive" ) ) ;

    processMemoryMessages();

    processIndexMessages();

    sendMessages();
  }

  void processMemoryMessages() {
    while (! theMemoryMessagesIn.empty() ) {
      boost::intrusive_ptr<MemoryMessage> msg = theMemoryMessagesIn.dequeue()[MemoryMessageTag];      
      switch( msg->type() ) {
        case MemoryMessage::LoadReply:
          processReadReply(*msg);
          break;
        case MemoryMessage::RMWReply:
          processWriteReply(*msg);
          break;
        default:
          DBG_Assert(false);
      }
    }    
  }

  void processIndexMessages() {
    while (! theMessagesIn.empty() ) {
      IndexMessage msg = theMessagesIn.front();
      DBG_(Verb, Comp(*this) ( << "Processing: "  << msg ));    
      theMessagesIn.pop_front();
      switch (msg.theCommand) {
        case IndexCommand::eLookup:
          doLookup( msg );        
          break;
        case IndexCommand::eInsert:
          doInsert( msg.theAddress, msg.theCMOB, msg.theCMOBOffset );        
          break;
        case IndexCommand::eUpdate:
          doUpdate( msg );        
          break;
        default:
          DBG_Assert(false);
      }
    }    
  }

  long long makeEntry( int aCMOB, long aLocation) {
    return static_cast<long long>(aLocation) << 4 | (aCMOB & 15);
  }

  std::pair<int, long> splitEntry( long long anEntry) {
    return std::make_pair( anEntry & 15, anEntry >> 4);
  }

  void doUpdate(IndexMessage & msg) {
    if (theIndex->has( msg.theAddress)) {
      theIndex->insert( static_cast<unsigned long>(msg.theAddress), makeEntry( msg.theCMOB, msg.theCMOBOffset), false );
    } else {
      msg.theCommand = IndexCommand::eNoUpdateReply;
      DBG_(Iface, Comp(*this) ( << "Update: " << msg ) );
      theMessagesOut.push_back(msg);                 
    }       
  }

  void doInsert(MemoryAddress anAddress, int aCMOB, long aLocation) {

    theIndex->insert( static_cast<unsigned long>(anAddress), makeEntry( aCMOB, aLocation), false );

    if (! cfg.UseMemory) {
      return;
    }

    if (theMemoryWritesOut.full()) {
      ++theDroppedWrites; 
      return;
    }
    
    MemoryAddress idx_address( address(anAddress) );
        
    MemoryTransport transport;
    boost::intrusive_ptr<MemoryMessage> operation( new MemoryMessage(MemoryMessage::RMWReq, idx_address));

    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    tracker->setAddress( idx_address );
    tracker->setInitiator(flexusIndex());
    transport.set(TransactionTrackerTag, tracker);
    transport.set(MemoryMessageTag, operation);


    DBG_Assert( ! theMemoryWritesOut.full() );
    theMemoryWritesOut.enqueue(transport);    
    
  }

  MemoryAddress address( MemoryAddress anAddress) {
    unsigned int bucket = theIndex->bucket(anAddress);
    return MemoryAddress( 0x80000000UL | (bucket * 64) ); 
  }

  void doLookup(IndexMessage & aMessage) {
    if (! cfg.UseMemory) {
      finishLookup( aMessage );
      return;
    }
    DBG_(Iface, Comp(*this) ( << "startLookup: " << aMessage) );
    
    if (theMemoryReadsOut.full() ) {
      enqueueTIndexMiss( aMessage );
      ++theDroppedReads;
      return;
    }
    
    MemoryAddress idx_address( address(aMessage.theAddress) );
    
    thePendingReads.insert( std::make_pair(idx_address, aMessage) );
    
    MemoryTransport transport;
    boost::intrusive_ptr<MemoryMessage> operation( new MemoryMessage(MemoryMessage::LoadReq, idx_address));
    operation->reqSize() = 0;

    boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
    tracker->setAddress( idx_address );
    tracker->setInitiator(flexusIndex());
    transport.set(TransactionTrackerTag, tracker);
    transport.set(MemoryMessageTag, operation);


    DBG_Assert( ! theMemoryReadsOut.full() );
    theMemoryReadsOut.enqueue(transport);
  }

  void finishLookup(IndexMessage & msg) {
    long anEntry;
    bool is_good;
    boost::tie(anEntry, is_good) = theIndex->lookup( msg.theAddress);
    if (anEntry == -1) {
      enqueueTIndexMiss( msg );
    } else {
      int aCMOB;
      long aLocation;
      boost::tie( aCMOB, aLocation) = splitEntry( anEntry);
      
      enqueueTIndexHit( msg, aCMOB, aLocation);
    }        
  }

  void processReadReply(MemoryMessage & msg) {
    std::multimap< MemoryAddress, IndexMessage >::iterator begin, iter, end;
    boost::tie(begin, end) = thePendingReads.equal_range( msg.address() );
    iter = begin;
    while (iter != end) {
      finishLookup(iter->second);
      ++iter; 
    }
    thePendingReads.erase( begin, end );
    
  }

  void processWriteReply(MemoryMessage & msg) { 
    //Nothing to do  
  }

  void enqueueTIndexMiss( IndexMessage & msg ) {
    msg.theCommand = IndexCommand::eNoMatchReply;
    msg.theCMOB = -1;
    msg.theCMOBOffset = -1;
    DBG_(Iface, Comp(*this) ( << "finishLookup: " << msg ) );
    theMessagesOut.push_back(msg);    
  }

  void enqueueTIndexHit( IndexMessage & msg, int aCMOB, long aLocation) {
    msg.theCommand = IndexCommand::eMatchReply;
    msg.theCMOB = aCMOB;
    msg.theCMOBOffset = aLocation;
    if (cfg.FillPrefix > 0) {
      std::pair< int, long> prefix( cfg.FillPrefix, aLocation + 1);
      FLEXUS_CHANNEL_ARRAY( PrefixRead, aCMOB ) << prefix;
      msg.thePrefix = thePrefix; 
      DBG_( Iface, ( << "Got a prefix of length " << msg.thePrefix.size() ) );
    }

    DBG_(Iface, Comp(*this) ( << "finishLookup: " << msg ) );
    theMessagesOut.push_back(msg);    
  }

  void sendMessages() {
    while (  !theMessagesOut.empty()) {
      int TMSc = theMessagesOut.front().theTMSc;
      if (FLEXUS_CHANNEL_ARRAY(TMSc_Reply, TMSc).available()) {
        DBG_(Iface, Comp(*this) ( << "Sending TMSif_Reply: "  << (theMessagesOut.front()) ));
        FLEXUS_CHANNEL_ARRAY(TMSc_Reply, TMSc) << theMessagesOut.front();
        theMessagesOut.pop_front();        
      } else {
        break; 
      }
    }

    while ( FLEXUS_CHANNEL(ToMemory).available() && !theMemoryReadsOut.empty()) {
      DBG_(Iface, Comp(*this) ( << "Sending ToMemory: "  << *(theMemoryReadsOut.peek()[MemoryMessageTag]) ));
      MemoryTransport trans( theMemoryReadsOut.dequeue());
      FLEXUS_CHANNEL(ToMemory) << trans;
    }

    while ( FLEXUS_CHANNEL(ToMemory).available() && !theMemoryWritesOut.empty()) {
      DBG_(Iface, Comp(*this) ( << "Sending ToMemory: "  << *(theMemoryWritesOut.peek()[MemoryMessageTag]) ));
      MemoryTransport trans( theMemoryWritesOut.dequeue());
      FLEXUS_CHANNEL(ToMemory) << trans;
    }

  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(PrefixReadIn);
  void push(interface::PrefixReadIn const &, index_t anIndex, std::list<MemoryAddress> & aPrefix ) {
    thePrefix = aPrefix;
  }

};


} //End Namespace nTMSIndex

FLEXUS_COMPONENT_INSTANTIATOR( TMSIndex, nTMSIndex);
FLEXUS_PORT_ARRAY_WIDTH( TMSIndex, TMSc_Request) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH( TMSIndex, TMSc_Reply) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH( TMSIndex, PrefixRead) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH( TMSIndex, PrefixReadIn) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }


#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT TMSIndex

  #define DBG_Reset
  #include DBG_Control()

