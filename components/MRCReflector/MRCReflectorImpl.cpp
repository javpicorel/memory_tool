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

#include <components/MRCReflector/MRCReflector.hpp>

#include <list>
#include <fstream>

#include <boost/serialization/map.hpp>

#include <core/stats.hpp>
#include <core/boost_extensions/padded_string_cast.hpp>


#define FLEXUS_BEGIN_COMPONENT MRCReflector
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

  #define DBG_DefineCategories MRCReflector
  #define DBG_SetDefaultOps AddCat(MRCReflector)
  #include DBG_Control()


namespace nMRCReflector {

using namespace Flexus::SharedTypes;
using namespace Flexus::Core;
using Flexus::SharedTypes::PhysicalMemoryAddress;
namespace Stat = Flexus::Stat;

static const int kPrefetchListener = 1;
static const int kConsort = 2;
static const int kMRCReflector = 3;


class FLEXUS_COMPONENT(MRCReflector) {
  FLEXUS_COMPONENT_IMPL(MRCReflector);

private:
  std::list<NetworkTransport> theNetworkMessagesIn;
  std::list<NetworkTransport> theNetworkMessagesOut;
  std::list<PredictorTransport> thePredictorMessagesIn;

  Stat::StatMax statMaxEntries;
  Stat::StatCounter statRequests;
  Stat::StatCounter statMisses;
  Stat::StatCounter statTwoMisses;
  std::vector<Stat::StatCounter *> statRequests_ByNode;
  std::vector<Stat::StatCounter *> statMisses_ByNode;


  static const long kTransient = -1;


  struct ReflectorEntry {
    unsigned int theMRC;
    long theLocation;
    ReflectorEntry()
      : theMRC(0)
      , theLocation(kTransient)
      {}
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
      ar & theMRC;
      ar & theLocation;
    }
  };

  typedef std::map< PhysicalMemoryAddress, ReflectorEntry > reflector_map;
  reflector_map theReflectorMap;
  reflector_map theReflectorTwoMap;
  long long theNextTag;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(MRCReflector)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , statMaxEntries                     ( statName() + "-MaxEntries"             )
    , statRequests                       ( statName() + "-Requests"               )
    , statMisses                         ( statName() + "-Misses"                 )
    , statTwoMisses                      ( statName() + "-TwoMisses"              )
  {
    for (unsigned int i = 0; i < 16; ++i) {
      statRequests_ByNode.push_back(  new Stat::StatCounter( statName() + "-Requests[" + boost::padded_string_cast<2,'0'>( i ) + "]"   ) );
      statMisses_ByNode.push_back(    new Stat::StatCounter( statName() + "-Misses[" + boost::padded_string_cast<2,'0'>( i ) + "]"     ) );
    }
  }

  bool isQuiesced() const {
    return    theNetworkMessagesIn.empty()
      &&      theNetworkMessagesOut.empty()
      &&      thePredictorMessagesIn.empty()
      ;
  }

  void saveState(std::string const & aDirName) const {
    std::string fname( aDirName);
    fname += "/" + statName();
    std::ofstream ofs(fname.c_str(), std::ios::binary);
    boost::archive::binary_oarchive oa(ofs);

    oa << theReflectorMap;
    // close archive
    ofs.close();

  }

  void loadState(std::string const & aDirName) {
    if (cfg.Enable) {
      std::string fname( aDirName);
      fname += "/" + statName();
      std::ifstream ifs(fname.c_str(), std::ios::binary);
      boost::archive::binary_iarchive ia(ifs);

      ia >> theReflectorMap;
      // close archive
      ifs.close();

      fname = aDirName + "/" + statName() + "2";
      std::ifstream ifs2(fname.c_str(), std::ios::binary);
      if (ifs2) {
        DBG_(Dev, ( << "Loading MRC2 state" ) );
        boost::archive::binary_iarchive ia2(ifs2);

        ia2 >> theReflectorTwoMap;
        // close archive
        ifs2.close();
      }

    }
  }

  // Initialization
  void initialize() {
    theNextTag = flexusIndex();
  }


  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(FromNic);
  void push( interface::FromNic const &, index_t aVC, NetworkTransport & aMessage) {
      DBG_(Iface, Comp(*this) ( << "Received FromNic: "  << *(aMessage[MRCMessageTag]) ) );
      DBG_Assert(aVC == 0);
      enqueue(aMessage);
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(FromEngine);
  void push( interface::FromEngine const &, PredictorTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received FromEngine: "  << *(aMessage[PredictorMessageTag]) ) );
    enqueue(aMessage);
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(FromLocal);
  void push( interface::FromLocal const &, PredictorTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received FromLocal: "  << *(aMessage[PredictorMessageTag]) ) );
    enqueue(aMessage);
  }

  // Drive Interfaces
  void drive( interface::ReflectorDrive const &) {
    DBG_(VVerb, Comp(*this) (<< "ReflectorDrive" ) ) ;
    process();
  }

private:

  void recordMRC( unsigned int aNode, PhysicalMemoryAddress anAddress, long aLocation) {
    reflector_map::iterator iter = theReflectorMap.find(anAddress);
    if (iter != theReflectorMap.end()) {
      theReflectorTwoMap[anAddress].theMRC = iter->second.theMRC;
      theReflectorTwoMap[anAddress].theLocation = iter->second.theLocation;
      iter->second.theMRC = aNode;
      iter->second.theLocation = aLocation;
    } else {
      theReflectorMap[anAddress].theMRC = aNode;
      theReflectorMap[anAddress].theLocation = aLocation;
    }

    statMaxEntries << theReflectorMap.size();
  }

  void reflectRequest( unsigned int aNode, PhysicalMemoryAddress anAddress ) {
    ++statRequests;
    ++(* statRequests_ByNode[aNode]);
    reflector_map::iterator iter;

    //Find MRC
    iter = theReflectorMap.find(anAddress);
    if (iter == theReflectorMap.end()) {
      DBG_(Trace, ( << "Reflector miss on request for " << anAddress << " by " << aNode) );
      ++statMisses;
      ++(* statMisses_ByNode[aNode]);
      return; //No MRC info for this address
    }
    unsigned int mrc = iter->second.theMRC;
    long mrc_location = iter->second.theLocation;
    DBG_Assert( mrc_location != kTransient );

    //Find 2MRC
    int two = -1;
    long two_location = -1;
    iter = theReflectorTwoMap.find(anAddress);
    if (iter == theReflectorTwoMap.end()) {
      DBG_(Trace, ( << "Two miss on request for " << anAddress << " by " << aNode) );
      ++statTwoMisses;
    } else {
      two = iter->second.theMRC;
      two_location = iter->second.theLocation;
      DBG_Assert( two_location != kTransient );
    }

    //Send MRC Message

    long long tag = -1;
    if ( cfg.TwoEnable && two != -1 ) {
      theNextTag += 32;
      tag = theNextTag;
    }
    boost::intrusive_ptr<MRCMessage> mrc_msg (new MRCMessage(MRCMessage::kRequestStream, aNode, anAddress, mrc_location, tag));


    // create the network routing piece
    boost::intrusive_ptr<NetworkMessage> net_msg (new NetworkMessage());
    net_msg->src = flexusIndex();
    net_msg->dest = mrc;
    net_msg->src_port = kMRCReflector;
    net_msg->dst_port = kConsort;
    net_msg->vc = 0;
    net_msg->size = 0; //Control message

    // now make the transport and queue it
    NetworkTransport transport;
    transport.set(MRCMessageTag, mrc_msg);
    transport.set(NetworkMessageTag, net_msg);
    theNetworkMessagesOut.push_back(transport);

    //Send 2MRC Message
    if (cfg.TwoEnable && (two != -1)) {
      boost::intrusive_ptr<MRCMessage> two_msg (new MRCMessage(MRCMessage::kRequestStream, aNode, anAddress, two_location, theNextTag+16 ));
      // create the network routing piece
      boost::intrusive_ptr<NetworkMessage> net_msg (new NetworkMessage());
      net_msg->src = flexusIndex();
      net_msg->dest = two;
      net_msg->src_port = kMRCReflector;
      net_msg->dst_port = kConsort;
      net_msg->vc = 0;
      net_msg->size = 0; //Control message

      // now make the transport and queue it
      NetworkTransport transport;
      transport.set(MRCMessageTag, two_msg);
      transport.set(NetworkMessageTag, net_msg);
      theNetworkMessagesOut.push_back(transport);
    }

  }

  void enqueue(NetworkTransport & aMessage) {
    if (cfg.Enable) {
      theNetworkMessagesIn.push_back(aMessage);
    }
  }
  void enqueue(PredictorTransport & aMessage) {
    if (cfg.Enable) {
      thePredictorMessagesIn.push_back(aMessage);
    }
  }

  void process() {
    if (cfg.Enable) {
      //Process incoming messages from the Protocol engine - reflects stream requests to Consorts
      processPredictorMessages();

      //Process incoming messages from the network - add notifications from Consorts
      processNetworkMessages();

      //Send outgoing messages to the network
      sendMessages();
    }
  }

  void sendMessages() {
    //Send to Network
    while ( FLEXUS_CHANNEL(ToNic).available() && !theNetworkMessagesOut.empty()) {
      DBG_(Iface, Comp(*this) ( << "Sending ToNetwork: "  << *(theNetworkMessagesOut.front()[MRCMessageTag]) ));
      FLEXUS_CHANNEL(ToNic) << theNetworkMessagesOut.front();
      theNetworkMessagesOut.pop_front();
    }
  }

  void processPredictorMessages() {
    while (! thePredictorMessagesIn.empty() ) {
      boost::intrusive_ptr<PredictorMessage> msg = thePredictorMessagesIn.front()[PredictorMessageTag];
      boost::intrusive_ptr<TransactionTracker> tracker = thePredictorMessagesIn.front()[TransactionTrackerTag];
      DBG_Assert(msg);
      DBG_(VVerb, Comp(*this) ( << "Processing predictor message: "  << *msg) );

      if (msg->type() == PredictorMessage::eReadNonPredicted && ( cfg.AllMisses || (tracker->fillType() && *tracker->fillType() == eCoherence) ) ) {
        if (! tracker->isFetch()  || !(*tracker->isFetch()) ) {
          reflectRequest(msg->node(), msg->address());
        }
      }

      thePredictorMessagesIn.pop_front();
    }
  }

  void processNetworkMessages() {
    while (! theNetworkMessagesIn.empty() ) {
      boost::intrusive_ptr<NetworkMessage> net_msg = theNetworkMessagesIn.front()[NetworkMessageTag];
      boost::intrusive_ptr<MRCMessage> mrc_msg = theNetworkMessagesIn.front()[MRCMessageTag];
      DBG_(VVerb, Comp(*this) ( << "Processing MRC Message: "  << *cmd) );
      DBG_Assert(net_msg);
      DBG_Assert(mrc_msg);
      DBG_Assert(mrc_msg->node() == static_cast<unsigned>(net_msg->src));
      DBG_Assert(mrc_msg->type() == MRCMessage::kAppend, ( << name() << " received illegal MRCMessage: " << *mrc_msg) );

      recordMRC(mrc_msg->node(), mrc_msg->address(), mrc_msg->location());

      theNetworkMessagesIn.pop_front();
    }
  }

};


} //End Namespace nMRCReflector

FLEXUS_COMPONENT_INSTANTIATOR( MRCReflector, nMRCReflector );
FLEXUS_PORT_ARRAY_WIDTH( MRCReflector, FromNic) { return cfg.VChannels; }

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT MRCReflector

  #define DBG_Reset
  #include DBG_Control()

