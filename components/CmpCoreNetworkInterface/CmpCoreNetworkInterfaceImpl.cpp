// DO-NOT-REMOVE begin-copyright-block 
//                                     
// Redistributions of any form whatsoever must retain and/or include the     
// following acknowledgment, notices and disclaimer:                         
//                                                                           
// This product includes software developed by Carnegie Mellon University.   
//                                                                           
// Copyright 2006 by Eric Chung, Brian Gold, Nikos Hardavellas, Jangwoo Kim, 
// Ippokratis Pandis, Minglong Shao, Jared Smolens, Stephen Somogyi,         
// Tom Wenisch, Anastassia Ailamaki, Babak Falsafi and James C. Hoe for      
// the SimFlex Project, Computer Architecture Lab at Carnegie Mellon,        
// Carnegie Mellon University.                                               
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

#include <components/CmpCoreNetworkInterface/CmpCoreNetworkInterface.hpp>

#include <components/Common/Slices/NetworkMessage.hpp>
#include <components/Common/Slices/MemoryMessage.hpp>

#include <core/stats.hpp>
#include <core/flexus.hpp>

#include <vector>
#include <list>

#define FLEXUS_BEGIN_COMPONENT CmpCoreNetworkInterface
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

  #define DBG_DefineCategories CmpCoreNetworkInterface
  #define DBG_SetDefaultOps AddCat(CmpCoreNetworkInterface)
  #include DBG_Control()

namespace nCmpCoreNetworkInterface {

using namespace Flexus;
using namespace Core;

class FLEXUS_COMPONENT(CmpCoreNetworkInterface) {
  FLEXUS_COMPONENT_IMPL(CmpCoreNetworkInterface);

 private:
 	std::vector< std::list< MemoryTransport > > recvQueue; // One per VC
	std::list< MemoryTransport > sendQueue; // One per port, only one port

	// The number of messages in the queues
	unsigned int currRecvCount;
	unsigned int currSendCount;

 public:
   FLEXUS_COMPONENT_CONSTRUCTOR(CmpCoreNetworkInterface)
     : base( FLEXUS_PASS_CONSTRUCTOR_ARGS ) { }

  bool isQuiesced() const {
    return (!currRecvCount && !currSendCount);
  }

  // Initialization
  void initialize() {
	recvQueue.resize(cfg.VChannels);
	currRecvCount = currSendCount = 0;
  }

  // Ports
  bool available(interface::DRequestFromCore const &) {
  	return sendQueue.size() < cfg.SendCapacity;
  }
  void push(interface::DRequestFromCore const &, MemoryTransport & transport) {
	boost::intrusive_ptr<NetworkMessage> netMsg = new NetworkMessage ();
  	transport.set ( NetworkMessageTag, netMsg );

  	transport[NetworkMessageTag]->vc = cfg.RequestVc;
  	transport[MemoryMessageTag]->coreNumber() = flexusIndex();
  	transport[MemoryMessageTag]->dstream() = true;
  	
	sendQueue.push_back( transport );
	++currSendCount;
	
	DBG_(Iface,
         Addr(transport[MemoryMessageTag]->address())
         (<< statName()
         << " accepted data request " << *transport[MemoryMessageTag]
         << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec
         <<" from cpu[" << flexusIndex() << "]"));
  }

  bool available(interface::DSnoopFromCore const &) {
  	return sendQueue.size() < cfg.SendCapacity;
  }
  void push(interface::DSnoopFromCore const &, MemoryTransport & transport) {
  	boost::intrusive_ptr<NetworkMessage> netMsg = new NetworkMessage ();
  	transport.set ( NetworkMessageTag, netMsg );
  	
  	transport[NetworkMessageTag]->vc = cfg.SnoopVc;
    if (!transport[MemoryMessageTag]->isPurge())  /* CMU-ONLY */
    {
      transport[MemoryMessageTag]->coreNumber() = flexusIndex();
      transport[MemoryMessageTag]->dstream() = true;
    }
  	
  	sendQueue.push_back( transport );
	++currSendCount;
	
	DBG_(Iface,
         Addr(transport[MemoryMessageTag]->address())
         (<< statName()
         << " accepted data snoop " << *transport[MemoryMessageTag]
         << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec 
         <<" from cpu[" << flexusIndex() << "]"));
  }
  
    bool available(interface::IRequestFromCore const &) {
  	return sendQueue.size() < cfg.SendCapacity;
  }
  void push(interface::IRequestFromCore const &, MemoryTransport & transport) {
	boost::intrusive_ptr<NetworkMessage> netMsg = new NetworkMessage ();
  	transport.set ( NetworkMessageTag, netMsg );

  	transport[NetworkMessageTag]->vc = cfg.RequestVc;
  	transport[MemoryMessageTag]->coreNumber() = flexusIndex();
  	transport[MemoryMessageTag]->dstream() = false;
  	
	sendQueue.push_back( transport );
	++currSendCount;
	
	DBG_(Iface,
         Addr(transport[MemoryMessageTag]->address())
         (<< statName()
         << " accepted instruction request " << *transport[MemoryMessageTag]
         << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec 
         <<" from cpu[" << flexusIndex() << "]"));
  }

  bool available(interface::ISnoopFromCore const &) {
  	return sendQueue.size() < cfg.SendCapacity;
  }
  void push(interface::ISnoopFromCore const &, MemoryTransport & transport) {
  	boost::intrusive_ptr<NetworkMessage> netMsg = new NetworkMessage ();
  	transport.set ( NetworkMessageTag, netMsg );
  	
  	transport[NetworkMessageTag]->vc = cfg.SnoopVc;
    if (!transport[MemoryMessageTag]->isPurge())  /* CMU-ONLY */
    {
      transport[MemoryMessageTag]->coreNumber() = flexusIndex();
      transport[MemoryMessageTag]->dstream() = false;
    }
  	
  	sendQueue.push_back( transport );
	++currSendCount;
	
	DBG_(Iface,
         Addr(transport[MemoryMessageTag]->address())
         (<< statName()
         << " accepted instruction snoop " << *transport[MemoryMessageTag]
         << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec 
         <<" from cpu[" << flexusIndex() << "]"));
  }
  
  bool available(interface::FromNetwork const &, index_t aVC) {
  	return (recvQueue[aVC].size() < cfg.RecvCapacity);
  }
  void push(interface::FromNetwork const &, index_t aVC, MemoryTransport & transport) {
	DBG_Assert(aVC == (index_t)transport[NetworkMessageTag]->vc, Comp(*this) (<< "Mismatched port/vc assignment"));
	
	recvQueue[aVC].push_back( transport );
    ++currRecvCount;
    DBG_(Iface,
         Addr(transport[MemoryMessageTag]->address())
         (<< statName()
         << " accepted data reply " << *transport[MemoryMessageTag]
         << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec 
         << " on vc[" << aVC << "]"
         << " for cpu[" << flexusIndex() << "]"));
  }
  
  //Drive Interfaces
  void drive( interface::CmpCoreNetworkInterfaceDrive const &) {
      // Handle any pending messages in the queues
      if(currSendCount > 0) {
      	msgFromNode();
      }
      if(currRecvCount > 0) {
      	msgToNode();
      }
  }
  
 private: 
	void msgFromNode() {
		// try to give a message to the network
		if(!sendQueue.empty()) {
			if (FLEXUS_CHANNEL_ARRAY(ToNetwork, sendQueue.front()[NetworkMessageTag]->vc).available()) {
				MemoryTransport transport = sendQueue.front();
				sendQueue.pop_front();
				
				--currSendCount;
				FLEXUS_CHANNEL_ARRAY(ToNetwork, transport[NetworkMessageTag]->vc) << transport;
    		}
		}
    }
  

	void msgToNode() {
      int i;
    
      // try to give a message to the node, beginning with the highest priority virtual channelz
      for(i = cfg.VChannels - 1; i >= 0; --i) {
        if(!recvQueue[i].empty()) {
          MemoryTransport transport = recvQueue[i].front();

          // Request to L1 from network (snoops and ReturnReq)
          if (transport[MemoryMessageTag]->isExtReqType()) {
            // Route to L1D
            if (transport[MemoryMessageTag]->isDstream()) {
              if (FLEXUS_CHANNEL(DRequestToCore).available()) {
                recvQueue[i].pop_front();
						
                --currRecvCount;
                DBG_(Iface,
                     Addr(transport[MemoryMessageTag]->address())
                     (<< statName()
                     << " sending data request " << *transport[MemoryMessageTag]
                     << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec 
                     <<" into cpu[" << flexusIndex() << "]"));
                FLEXUS_CHANNEL(DRequestToCore) << transport;
              }
            }
            // Route to L1I
            else {
              if (FLEXUS_CHANNEL(IRequestToCore).available()) {
                recvQueue[i].pop_front();
						
                --currRecvCount;
                DBG_(Iface,
                     Addr(transport[MemoryMessageTag]->address())
                     (<< statName()
                     << " sending instruction request " << *transport[MemoryMessageTag]
                     << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec 
                     <<" into cpu[" << flexusIndex() << "]"));
                FLEXUS_CHANNEL(IRequestToCore) << transport;
              }
            }
          }

          // reply to L1 from network
          else {
            // Route to L1D
            if (transport[MemoryMessageTag]->isDstream()) {
              if (FLEXUS_CHANNEL(DReplyToCore).available()) {
                recvQueue[i].pop_front();
						
                --currRecvCount;
                DBG_(Iface,
                     Addr(transport[MemoryMessageTag]->address())
                     (<< statName()
                     << " sending data reply " << *transport[MemoryMessageTag]
                     << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec 
                     <<" into cpu[" << flexusIndex() << "]"));
                FLEXUS_CHANNEL(DReplyToCore) << transport;
              }
            }
            // Route to L1I
            else {
              if (FLEXUS_CHANNEL(IReplyToCore).available()) {
                recvQueue[i].pop_front();
						
                --currRecvCount;
                DBG_(Iface,
                     Addr(transport[MemoryMessageTag]->address())
                     (<< statName()
                     << " sending instruction reply " << *transport[MemoryMessageTag]
                     << " for address 0x" << std::hex << transport[MemoryMessageTag]->address() << std::dec 
                     <<" into cpu[" << flexusIndex() << "]"));
                FLEXUS_CHANNEL(IReplyToCore) << transport;
              }
            }
          }
        }
      }
    }
};

} //End Namespace nCmpCoreNetworkInterface

FLEXUS_COMPONENT_INSTANTIATOR( CmpCoreNetworkInterface, nCmpCoreNetworkInterface );

FLEXUS_PORT_ARRAY_WIDTH( CmpCoreNetworkInterface, ToNetwork ) { return cfg.VChannels; }
FLEXUS_PORT_ARRAY_WIDTH( CmpCoreNetworkInterface, FromNetwork ) { return cfg.VChannels; }

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT CmpCoreNetworkInterface

  #define DBG_Reset
  #include DBG_Control()
