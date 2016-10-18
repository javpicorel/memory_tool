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

#include <components/OrderHW/OrderHW.hpp>

#include <boost/bind.hpp>

#define FLEXUS_BEGIN_COMPONENT OrderHW
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include "common.hpp"


namespace nOrderHW {


class FLEXUS_COMPONENT(OrderHW) {
  FLEXUS_COMPONENT_IMPL( OrderHW );

  TraceCoordinator * theCoordinator;
public:
  FLEXUS_COMPONENT_CONSTRUCTOR(OrderHW)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {}

  bool isQuiesced() const {
    return true;
  }

  // Initialization
  void initialize() {
    theCoordinator = initialize_OrderHW(cfg.Configuration);
    theFlexus->onTerminate( boost::bind( &TraceCoordinator::finalize, theCoordinator) );
    DBG_Assert( cfg.TargetEvent != kAllData );
  }

  // Ports
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(L2Reads);
  void push(interface::L2Reads const &, index_t anIndex, MemoryMessage & message) {
    if (message.address() != 0) {
      if (cfg.TargetEvent == kOnChipReads) {
        DBG_Assert( message.fillType() <= Flexus::SharedTypes::eNAW, ( << message) );
        theCoordinator->consumption( message.coreIdx(), message.address(), message.pc(), message.fillType(), message.fillLevel(), message.priv() );
      } else {
        theCoordinator->access( anIndex, message.address(), message.pc(), message.priv()  );
      }
    }
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(L2Writes);
  void push(interface::L2Writes const &, index_t anIndex, MemoryMessage & message) {
    if (message.address() != 0 && cfg.TargetEvent == kOnChipReads) {
        theCoordinator->upgrade( message.coreIdx(), message.address(), message.pc(), message.priv()  );      
    }
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(Evictions);
  void push(interface::Evictions const &, MemoryMessage & message) {
    if (message.address() != 0 && cfg.TargetEvent != kOnChipReads ) {
      theCoordinator->eviction( message.coreIdx(), message.address());
    }
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(Invalidations);
  void push(interface::Invalidations const &, MemoryMessage & message) {
    if (message.address() != 0 && cfg.TargetEvent != kOnChipReads ) {
      theCoordinator->eviction( message.coreIdx(), message.address() );
    }
  }


  FLEXUS_PORT_ALWAYS_AVAILABLE(Writes);
  void push(interface::Writes const &, MemoryMessage & message) {
    if (message.address() != 0 && cfg.TargetEvent != kOnChipReads) {
        theCoordinator->upgrade( message.coreIdx(), message.address(), message.pc(), message.priv()  );
    }
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(Reads);
  void push(interface::Reads const &, MemoryMessage & message ) {
    if (message.address() != 0) {
      if (cfg.TargetEvent == kAllReads || (cfg.TargetEvent == kConsumptions && message.fillType() == eCoherence) ) {
        theCoordinator->consumption( message.coreIdx(), message.address(), message.pc(), message.fillType(), message.fillLevel(), message.priv()  );
      }
    }
  }

};

}//End namespace nFastBus

FLEXUS_COMPONENT_INSTANTIATOR( OrderHW, nOrderHW);
FLEXUS_PORT_ARRAY_WIDTH( OrderHW, L2Reads ) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH( OrderHW, L2Writes ) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT OrderHW

  #define DBG_Reset
  #include DBG_Control()
