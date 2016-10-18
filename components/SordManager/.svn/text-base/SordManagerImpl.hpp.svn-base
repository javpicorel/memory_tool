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


#define FLEXUS_BEGIN_COMPONENT SordManager
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

  #define DBG_DefineCategories SordManager
  #include DBG_Control()

#include <boost/shared_array.hpp>
#include <iomanip>
#include <deque>


#include "sord.hpp"
#include "directory.hpp"

namespace nSordManager {

using namespace Flexus::SharedTypes;
using namespace Flexus::Core;
typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

using boost::intrusive_ptr;


struct StreamReference : public StreamReferenceBase {
  long theChunk;
  boost::intrusive_ptr<StreamBase> theStream;

  StreamReference(boost::intrusive_ptr<StreamBase> aStream, long aChunk)
    : theChunk(aChunk)
    , theStream(aStream)
    {}

  ~StreamReference() {}
  std::string toString() const {
     return std::string("Stream ") + theStream->toString() + " Chunk #" + boost::lexical_cast<std::string>(theChunk);
  }
  void notifyHit() {
    DBG_Assert(theStream);
    theStream->notifyHit(theChunk);
  }
};


template <class Cfg>
class SordManagerComponent : public FlexusComponentBase<SordManagerComponent, Cfg> {
  FLEXUS_COMPONENT_IMPL(nSordManager::SordManagerComponent, Cfg);

private:
  std::list<NetworkTransport> theNetworkMessagesIn;
  std::list<NetworkTransport> theNetworkMessagesOut;
  std::list<PredictorTransport> thePredictorMessagesIn;

  boost::scoped_ptr<BaseSORDManager> thePredictor;
  boost::scoped_ptr<Directory> theDirectory;
  std::vector<ForwardQueue> theForwardQueues;

  Flexus::Stat::StatCounter FlushNotifications;
  Flexus::Stat::StatCounter ReadPredictedNotifications;
  Flexus::Stat::StatCounter ReadNonPredictedNotifications;
  Flexus::Stat::StatCounter WriteNotifications;

  Flexus::Stat::StatCounter PrefetchHitNotifications;
  Flexus::Stat::StatCounter PrefetchHitWithoutStream;
  Flexus::Stat::StatCounter PrefetchAckLate;

public:
  SordManagerComponent( FLEXUS_COMP_CONSTRUCTOR_ARGS )
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , FlushNotifications(name() + ".Flush Notifications")
    , ReadPredictedNotifications(name() + ".Read Predicted Notifications")
    , ReadNonPredictedNotifications(name() + ".Read Non-Predicted Notifications")
    , WriteNotifications(name() + ".Write Notification")
    , PrefetchHitNotifications(name() + ".Prefetch Hit Notifications")
    , PrefetchHitWithoutStream(name() + ".Prefetch Hit Without Stream")
    , PrefetchAckLate(name() + ".Prefetch Ack Too Late")
    {}


  // Initialization
  void initialize() {
    theForwardQueues.resize(cfg.NumNodes.value);

    if (cfg.PerConsumer.value) {
      thePredictor.reset(
        new PerConsSORDManager( flexusIndex()
                        , cfg.NumNodes.value
                        , theForwardQueues
                        , cfg.HeadChunkSize.value
                        , cfg.BodyChunkSize.value
                        , cfg.TemplateStorage.value
                        , cfg.EagerForwarding.value
                        , cfg.MRP.value
                        , cfg.MRPAddrBits.value
                        , cfg.CacheBlockSize.value
                        , cfg.MRPSets.value
                        , cfg.MRPAssociativity.value
                        )
      );
    } else {
      thePredictor.reset(
        new GlobalSORDManager( flexusIndex()
                        , cfg.NumNodes.value
                        , theForwardQueues
                        , cfg.HeadChunkSize.value
                        , cfg.BodyChunkSize.value
                        , cfg.TemplateStorage.value
                        , cfg.EagerForwarding.value
                        , cfg.MRP.value
                        , cfg.MRPAddrBits.value
                        , cfg.CacheBlockSize.value
                        , cfg.MRPSets.value
                        , cfg.MRPAssociativity.value
                        )
      );
    }
    theDirectory.reset( new Directory(*thePredictor, cfg.UseOS.value) );
  }

  struct ToNic: public PushOutputPort< NetworkTransport > { };

  struct FromNic : public PushInputPort< NetworkTransport >, AlwaysAvailable {
    FLEXUS_WIRING_TEMPLATE
    static void push(self& aMgr, NetworkTransport aMessage) {
      DBG_(Iface, Comp(aMgr) ( << "Received FromNic: "  << *(aMessage[PrefetchCommandTag]) ) );
      aMgr.enqueue(aMessage);
    }
  };

  struct FromEngine : public PushInputPort< PredictorTransport >, AlwaysAvailable {
    FLEXUS_WIRING_TEMPLATE
    static void push(self& aMgr, PredictorTransport aMessage) {
      DBG_(Iface, Comp(aMgr) ( << "Received FromEngine: "  << *(aMessage[PredictorMessageTag]) ) );
      aMgr.enqueue(aMessage);
    }
  };

  struct FromLocal : public PushInputPort< PredictorTransport >, AlwaysAvailable {
    FLEXUS_WIRING_TEMPLATE
    static void push(self& aMgr, PredictorTransport aMessage) {
      DBG_(Iface, Comp(aMgr) ( << "Received FromLocal: "  << *(aMessage[PredictorMessageTag]) ) );
      aMgr.enqueue(aMessage);
    }
  };

  // Drive Interfaces
  struct PredictorDrive {
    FLEXUS_DRIVE( PredictorDrive ) ;
    typedef FLEXUS_IO_LIST(1, Availability<ToNic>
                             ) Inputs;
    typedef FLEXUS_IO_LIST(1, Value<ToNic>
                             ) Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void doCycle(self &aMgr) {
      DBG_(VVerb, Comp(aMgr) (<< "SORDDrive" ) ) ;
      aMgr.process<FLEXUS_PASS_WIRING>();
    }
  };

  typedef FLEXUS_DRIVE_LIST (1, PredictorDrive ) DriveInterfaces;

private:
  void enqueue(NetworkTransport & aMessage) {
    theNetworkMessagesIn.push_back(aMessage);
  }
  void enqueue(PredictorTransport & aMessage) {
    thePredictorMessagesIn.push_back(aMessage);
  }

  FLEXUS_WIRING_TEMPLATE
  void process() {
    //Process incoming messages from the Protocol engine - creates new predictions
    processPredictorMessages();

    //Process incoming messages from the network
    processNetworkMessages();

    //Enqueue outgoing messages for the new predictions
    enqueueNewForwards();

    //Send outgoing messages to the network
    sendMessages<FLEXUS_PASS_WIRING>();
  }

  FLEXUS_WIRING_TEMPLATE
  void sendMessages() {
    //Send to Network
    if (cfg.FwdEnable.value) {
      while ( FLEXUS_CHANNEL(*this,ToNic).available() && !theNetworkMessagesOut.empty()) {
        DBG_(Iface, Comp(*this) ( << "Sending ToNetwork: "  << *(theNetworkMessagesOut.front()[PrefetchCommandTag]) ));
        FLEXUS_CHANNEL(*this,ToNic) << theNetworkMessagesOut.front();
        theNetworkMessagesOut.pop_front();
      }
    } else {
      theNetworkMessagesOut.clear();
    }
  }

  void processPredictorMessages() {

    while (! thePredictorMessagesIn.empty() ) {
      boost::intrusive_ptr<PredictorMessage> msg = thePredictorMessagesIn.front()[PredictorMessageTag];
      boost::intrusive_ptr<TransactionTracker> tracker = thePredictorMessagesIn.front()[TransactionTrackerTag];
      DBG_Assert(msg);
      DBG_(VVerb, Comp(*this) ( << "Processing predictor message: "  << *msg) );

      switch (msg->type()) {
        case PredictorMessage::eFlush:
          DBG_Assert(tracker);
          DBG_Assert(tracker->OS(), ( << "no OS state in SordMgr for: " << *msg ) );
          ++FlushNotifications;
          theDirectory->downgrade(msg->node(), msg->address(), *tracker->OS());
          break;
        case PredictorMessage::eReadPredicted:
          ++ReadPredictedNotifications;
          theDirectory->consume(msg->node(), msg->address(), true, false);
          break;
        case PredictorMessage::eReadNonPredicted:
          DBG_Assert(tracker);
          DBG_Assert(tracker->OS(), ( << "no OS state in SordMgr for: " << *msg ) );
          ++ReadNonPredictedNotifications;
          theDirectory->consume(msg->node(), msg->address(), false, *tracker->OS());
          break;
        case PredictorMessage::eWrite:
          DBG_Assert(tracker);
          DBG_Assert(tracker->OS(), ( << "no OS state in SordMgr for: " << *msg ) );
          ++WriteNotifications;
          theDirectory->storeMiss(msg->node(), msg->address(), *tracker->OS());
          break;
        default:
          DBG_Assert(false, ( << "SordManager received illegal PredictorMessage: " << *msg ) );
      }

      thePredictorMessagesIn.pop_front();
    }


  }

  void enqueueNewForwards() {
    for (int i = 0; i < cfg.NumNodes.value; ++i) {
      while (! theForwardQueues[i].theQueue.empty() ) {
        ForwardQueueEntry & entry = theForwardQueues[i].theQueue.front();

        boost::intrusive_ptr<StreamBase> stream = entry.theStream;

        boost::intrusive_ptr<StreamReferenceBase> stream_ref = new StreamReference(entry.theStream, entry.theChunk);

        boost::intrusive_ptr<PrefetchCommand> cmd (new PrefetchCommand(PrefetchCommand::ePrefetchAddressList, stream_ref));

        while (! theForwardQueues[i].theQueue.empty() && theForwardQueues[i].theQueue.front().theStream == stream) {
          cmd->addressList().push_back(MemoryAddress(theForwardQueues[i].theQueue.front().theAddress));
          theForwardQueues[i].theQueue.pop_front();
        }

        // create the network routing piece
        intrusive_ptr<NetworkMessage> net_msg (new NetworkMessage());
        net_msg->src = flexusIndex();
        net_msg->dest = i;
        net_msg->src_port = 2; //SORDManager is at port 2
        net_msg->dst_port = 1; //PrefetchListener is at port 1
        net_msg->vc = 0;

        // now make the transport and queue it
        NetworkTransport transport;
        transport.set(PrefetchCommandTag, cmd);
        transport.set(NetworkMessageTag, net_msg);
        theNetworkMessagesOut.push_back(transport);
      }
    }
  }

  void processNetworkMessages() {
    while (! theNetworkMessagesIn.empty() ) {
      boost::intrusive_ptr<NetworkMessage> net_msg = theNetworkMessagesIn.front()[NetworkMessageTag];
      boost::intrusive_ptr<PrefetchCommand> cmd = theNetworkMessagesIn.front()[PrefetchCommandTag];
      DBG_(VVerb, Comp(*this) ( << "Processing prefetch command: "  << *cmd) );
      DBG_Assert(net_msg);
      DBG_Assert(cmd);
      DBG_Assert(cmd->type() == PrefetchCommand::ePrefetchAcknowledgeHit, ( << "PrefetchListener received illegal PrefetchCommand: " << *cmd) );

      DBG_Assert(cmd->streamReference());

      ++PrefetchHitNotifications;
      boost::intrusive_ptr<StreamReference> stream( static_cast<StreamReference *>( cmd->streamReference().get()) );
      boost::intrusive_ptr<TransactionTracker> tracker = theNetworkMessagesIn.front()[TransactionTrackerTag];
      if (stream) {
        stream->notifyHit();
      }
      else {
        ++PrefetchHitWithoutStream;
      }

      DBG_Assert(tracker);
      DBG_Assert(tracker->OS(), ( << "no OS state in SordMgr for: " << *cmd ) );
      // inform the directory that we know a predicted read was correct
      tID consumer = net_msg->src;
      std::list<MemoryAddress>::iterator addrs;
      for(addrs = cmd->addressList().begin(); addrs != cmd->addressList().end(); ++addrs) {
        if(! theDirectory->predictedHit(consumer, *addrs, *tracker->OS()) ) {
          ++PrefetchAckLate;
          //DBG_(Dev, ( << "late prefetch ack for address" << *addrs ) );
        }
      }


      theNetworkMessagesIn.pop_front();
    }
  }

};

FLEXUS_COMPONENT_CONFIGURATION_TEMPLATE(SordManagerConfiguration,
    FLEXUS_PARAMETER( FwdEnable, bool, "Enable Forwarding", "enable", false )
    FLEXUS_PARAMETER( NumNodes, int, "Number of Nodes", "nodes", 16 )
    FLEXUS_PARAMETER( UseOS, bool, "Use OS references", "os", true )
    FLEXUS_PARAMETER( HeadChunkSize, int, "Head chunk size", "head_chunk", 1 )
    FLEXUS_PARAMETER( BodyChunkSize, int, "Body chunk size", "body_chunk", 1 )
    FLEXUS_PARAMETER( TemplateStorage, int, "Stream Template Storage", "storage", 100000 )
    FLEXUS_PARAMETER( EagerForwarding, bool, "Use Eager Forwarding", "eager", false )
    FLEXUS_PARAMETER( PerConsumer, bool, "Use Per-Consumer templates", "percons", false )
    FLEXUS_PARAMETER( MRP, bool, "Use MRP", "mrp", true )
    FLEXUS_PARAMETER( MRPAddrBits, int, "MRP Address Bits", "mrp_addr_bits", 8 )
    FLEXUS_PARAMETER( CacheBlockSize, int, "Cache Block Size", "blk_size", 64 )
    FLEXUS_PARAMETER( MRPSets, int, "MRP Sets", "mrp_sets", 0 )
    FLEXUS_PARAMETER( MRPAssociativity, int, "MRP Associativity", "mrp_assoc", 0 )
);

} //End Namespace nSordManager

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT SordManager

  #define DBG_Reset
  #include DBG_Control()

