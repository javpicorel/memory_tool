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

#define FLEXUS_BEGIN_COMPONENT Network
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()


  #define DBG_DefineCategories Network
  #define DBG_SetDefaultOps AddCat(Network)
  #include DBG_Control()


namespace nNetwork {

using namespace Flexus;

using namespace Core;
using namespace SharedTypes;


template <class Configuration>
class NetworkComponent : public FlexusComponentBase< NetworkComponent, Configuration> {
  FLEXUS_COMPONENT_IMPL(nNetwork::NetworkComponent, Configuration);

 public:
   NetworkComponent( FLEXUS_COMP_CONSTRUCTOR_ARGS )
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    {}

  bool isQuiesced() const {
    bool quiesced = true;
    if (sendQueue) {
      for (int i = 0; i < cfg_t::NumNodes_t::value && quiesced; ++i) {
        quiesced = sendQueue[i].empty();
      }
    }
    return quiesced;
  }

  // Initialization
  void initialize() {
    sendQueue = new std::vector<DelayTrans> [cfg_t::NumNodes_t::value];
  }

  // Ports
  struct ToNode : public PushOutputPortArray<NetworkTransport, cfg_t::NumNodes_t::value> { };

  struct FromNode : public PushInputPortArray<NetworkTransport, cfg_t::NumNodes_t::value * cfg_t::VChannels_t::value>, AlwaysAvailable {
    typedef FLEXUS_IO_LIST_EMPTY Inputs;
    typedef FLEXUS_IO_LIST_EMPTY Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void push(self& aNetwork, index_t anIndex, NetworkTransport transport) {
      DBG_Assert( (transport[NetworkMessageTag]->src == static_cast<int>(anIndex)) ); //static_cast to suppress warning about signed/unsigned comparison
      aNetwork.newPacket(transport);
    }
  };

  //Drive Interfaces
  struct NetworkDrive {
    FLEXUS_DRIVE( NetworkDrive ) ;
    typedef FLEXUS_IO_LIST(1, Availability<ToNode> ) Inputs;
    typedef FLEXUS_IO_LIST(1, Value<ToNode> ) Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void doCycle(self & aNetwork) {
      aNetwork.sendMessages<FLEXUS_PASS_WIRING>();
    }
  };

  //Declare the list of Drive interfaces
  typedef FLEXUS_DRIVE_LIST(1, NetworkDrive) DriveInterfaces;


private:

  // Add a new transport to the network from a node
  void newPacket(NetworkTransport & transport) {
    // just place it at the end of the source node's queue
    DBG_Assert(transport[NetworkMessageTag]);
    if (transport[TransactionTrackerTag]) {
      std::string cause( filled_lexical_cast<2>(transport[NetworkMessageTag]->src) + " -> " + filled_lexical_cast<2>(transport[NetworkMessageTag]->dest) );
      transport[TransactionTrackerTag]->setDelayCause("Network", cause);
    }
    sendQueue[transport[NetworkMessageTag]->src].push_back(make_pair(transport,cfg.NetDelay.value));
  }

  // Handle all the message passing for one cycle
  FLEXUS_WIRING_TEMPLATE
  void sendMessages() {
    int ii;

    for(ii = 0; ii < cfg.NumNodes.value; ii++) {
      DelayIter iter = sendQueue[ii].begin();
      while(iter != sendQueue[ii].end()) {
        if(iter->second > 0) {
          // decrement the wait time
          (iter->second)--;
          iter++;
        }
        else {
          // this packet is ready - send it
          boost::intrusive_ptr<NetworkMessage> msg = (iter->first)[NetworkMessageTag];
	  index_t pdest = (msg->dest) * cfg.VChannels.value + msg->vc;
          if ( FLEXUS_CHANNEL_ARRAY( *this, ToNode, pdest).available() ) {
              //One of these two debug messages will be used
              DBG_( Iface, Comp(*this)
                           Condition(   ((iter->first)[ProtocolMessageTag]) )
                           ( << "  " << *((iter->first)[ProtocolMessageTag]))
                  ) ;
              DBG_( Iface, Comp(*this)
                           Condition( ! ((iter->first)[ProtocolMessageTag]) )
                           ( << "delivering packet: src=" << msg->src
                             << " dest=" << msg->dest << " vc=" << msg->vc
                           )
                   );
            FLEXUS_CHANNEL_ARRAY( *this, ToNode, pdest ) << iter->first;
            sendQueue[ii].erase(iter);
          }
          else {
            iter ++;
          }
        }
      }
    }

  } // end sendMessages()

  // the to-be-sent message queues for each node
  typedef std::pair<NetworkTransport,int> DelayTrans;
  typedef std::vector<DelayTrans>::iterator DelayIter;
  std::vector<DelayTrans> *sendQueue;

};

FLEXUS_COMPONENT_CONFIGURATION_TEMPLATE(NetworkConfiguration,
    FLEXUS_PARAMETER( NetDelay, int, "Network delay", "delay", 0 )
    FLEXUS_STATIC_PARAMETER( NumNodes, int, "Number of Nodes", "nodes", 2)
    FLEXUS_STATIC_PARAMETER( VChannels, int, "Number vcs", "vc", 3)
);

} //End Namespace nNetwork


#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT Network

  #define DBG_Reset
  #include DBG_Control()
