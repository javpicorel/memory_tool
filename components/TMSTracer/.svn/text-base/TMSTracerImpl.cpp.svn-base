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

#include <components/TMSTracer/TMSTracer.hpp>

#include <boost/bind.hpp>

#define FLEXUS_BEGIN_COMPONENT TMSTracer
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include "common.hpp"
#include "coordinator.hpp"
#include <sys/stat.h>

namespace nTMSTracer {


class FLEXUS_COMPONENT(TMSTracer) {
  FLEXUS_COMPONENT_IMPL( TMSTracer );

  TraceCoordinator * theCoordinator;
public:
  FLEXUS_COMPONENT_CONSTRUCTOR(TMSTracer)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {}

  bool isQuiesced() const {
    return true;
  }

  // Initialization
  void initialize() {
    mkdir("flexus_state_out", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    theCoordinator = new tCoordinator("flexus_state_out");
    theFlexus->onTerminate( boost::bind( &TraceCoordinator::finalize, theCoordinator) );
  }

  // Ports
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(L2Reads);
  void push(interface::L2Reads const &, index_t anIndex, MemoryMessage & message) {
    if (message.address() != 0) {
      theCoordinator->accessL2( theFlexus->cycleCount(), eRead, message.coreIdx(), message.address(), message.pc(), message.fillType(), message.fillLevel(), (message.pc() & 0x10000000ULL ) );      
    }
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(L2Writes);
  void push(interface::L2Writes const &, index_t anIndex, MemoryMessage & message) {
    if (message.address() != 0) {
      theCoordinator->accessL2( theFlexus->cycleCount(), eWrite, message.coreIdx(), message.address(), message.pc(), message.fillType(), message.fillLevel(), (message.pc() & 0x10000000ULL ) );      
    }
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(L2Fetches);
  void push(interface::L2Fetches const &, index_t anIndex, MemoryMessage & message) {
    if (message.address() != 0) {
      theCoordinator->accessL2( theFlexus->cycleCount(), ::eFetch, message.coreIdx(), message.address(), message.pc(), message.fillType(), message.fillLevel(), (message.pc() & 0x10000000ULL ) );      
    }
  }


  FLEXUS_PORT_ALWAYS_AVAILABLE(Reads);
  void push(interface::Reads const &, MemoryMessage & message ) {
    if (message.address() != 0) {
      DBG_Assert( message.fillType() <= Flexus::SharedTypes::eNAW, ( << message << " ft: " << message.fillType()));
      theCoordinator->accessOC( theFlexus->cycleCount(), eRead, message.coreIdx(), message.address(), message.pc(), message.fillType(), message.fillLevel(), (message.pc() & 0x10000000ULL ) );      
    }
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(Writes);
  void push(interface::Writes const &, MemoryMessage & message) {
    if (message.address() != 0 ) {
      DBG_Assert( message.fillType() <= Flexus::SharedTypes::eNAW, ( << message << " ft: " << message.fillType()));
      theCoordinator->accessOC( theFlexus->cycleCount(), eWrite, message.coreIdx(), message.address(), message.pc(), message.fillType(), message.fillLevel(), (message.pc() & 0x10000000ULL ) );      
    }
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(Fetches);
  void push(interface::Fetches const &, MemoryMessage & message) {
    if (message.address() != 0 ) {
      DBG_Assert( message.fillType() <= Flexus::SharedTypes::eNAW, ( << message << " ft: " << message.fillType()));
      theCoordinator->accessOC( theFlexus->cycleCount(), ::eFetch, message.coreIdx(), message.address(), message.pc(), message.fillType(), message.fillLevel(), (message.pc() & 0x10000000ULL ) );      
    }
  }

};

}//End namespace nFastBus

FLEXUS_COMPONENT_INSTANTIATOR( TMSTracer, nTMSTracer);
FLEXUS_PORT_ARRAY_WIDTH( TMSTracer, L2Reads ) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH( TMSTracer, L2Writes ) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH( TMSTracer, L2Fetches) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT TMSTracer

  #define DBG_Reset
  #include DBG_Control()
