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

#ifndef FLEXUS_CORE_SIMICS_INTERFACE_HPP_INCLUDED
#define FLEXUS_CORE_SIMICS_INTERFACE_HPP_INCLUDED

#include <string>
#include <cstring>

#include <core/debug/debug.hpp>

#include <core/flexus.hpp>

#include <core/metaprogram.hpp>
#include <core/simics/configuration_api.hpp>
#include <core/simics/event_api.hpp>
#include <core/simics/hap_api.hpp>
#include <core/simics/control_api.hpp>
#include <core/flexus.hpp>


namespace Flexus {
namespace Simics {




class SimicsInterfaceImpl {
   public:
    void doCycleEvent(API::conf_object_t * anObj) {
        DBG_(VVerb, Core() ( << "Entering Flexus." ) );
        try {
          Core::theFlexus->doCycle();
          EventQueue(anObj).post(theEveryCycleEvent);
        } catch (std::exception & e) {
          DBG_(Crit, Core() ( << "Exception in Flexus: " << e.what() ) );
          BreakSimulation(e.what());
        }
        DBG_(VVerb, Core() ( << "Returing to Simics " ) );
    }

    void enableCycleHook() {
        if (! theIsEnabled) {
          DBG_(VVerb, Core() ( << "Enabling cycle hook" ) );
          theIsEnabled = true;
          EventQueue queue;
          queue.post(theEveryCycleEvent);
        }
    }

    void disableCycleHook() {
        DBG_(VVerb, Core() ( << "Disabling cycle hook" ) );
        if (theIsEnabled) {
          theIsEnabled = false;
          EventQueue queue;
          while(queue) {
            queue.clean(theInitialCycleEvent);
            queue.clean(theEveryCycleEvent);
            queue = queue.nextQueue();
          }
        }
    }

    void continueSimulation(API::conf_object_t * ignored) {
      if (! Flexus::Core::theFlexus->initialized()) {
        Flexus::Core::theFlexus->initializeComponents();
      }
    }

    void stopSimulation(API::conf_object_t *, long long, char *) {
    }

    Flexus::Core::index_t getSystemWidth() {
      int cpu_count = 0;
      int client_cpu_count = 0;
      API::conf_object_t * queue = API::SIM_next_queue(NULL);
      while (queue != NULL) {
        if (std::strncmp(queue->name,"client",6) == 0) {
          ++client_cpu_count;
        } else {
          ++cpu_count;
        }
        queue = API::SIM_next_queue(queue);
      }
      return cpu_count;
    }

   private:
    //These must follow the definition of doCycle();
    typedef MemberFnEvent<SimicsInterfaceImpl, &SimicsInterfaceImpl::doCycleEvent> do_cycle_event_t;
    do_cycle_event_t theInitialCycleEvent;
    do_cycle_event_t theEveryCycleEvent;

    HapToMemFnBinding<HAPs::Core_Continuation, SimicsInterfaceImpl, &SimicsInterfaceImpl::continueSimulation> theContinueSimulationHAP;
    HapToMemFnBinding<HAPs::Core_Simulation_Stopped, SimicsInterfaceImpl, &SimicsInterfaceImpl::stopSimulation> theStopSimulationHAP;

    bool theIsEnabled;

    Simics::API::conf_object_t * thisObject;

   public:
    SimicsInterfaceImpl(Simics::API::conf_object_t * anObject )
      : theInitialCycleEvent(do_cycle_event_t::timing_cycles, 0, this)
      , theEveryCycleEvent(do_cycle_event_t::timing_cycles, 1, this)
      , theContinueSimulationHAP(this)
      , theStopSimulationHAP(this)
      , theIsEnabled(true)
      {
        EventQueue queue;
        queue.post(theInitialCycleEvent);
    }
};

class SimicsInterface_Obj : public AddInObject<SimicsInterfaceImpl> {
    typedef AddInObject<SimicsInterfaceImpl> base;
   public:
    static const Persistence  class_persistence = Session;
    //These constants are defined in Simics/simics.cpp
    static std::string className() { return "SimicsInterface"; }
    static std::string classDescription() { return "Simics Interface class"; }

    SimicsInterface_Obj() : base() {}
    SimicsInterface_Obj(Simics::API::conf_object_t * anObject) : base(anObject) {}
    SimicsInterface_Obj(SimicsInterfaceImpl * anImpl) : base(anImpl) {}

};

typedef Factory< SimicsInterface_Obj > SimicsInterfaceFactory;

extern SimicsInterface_Obj theSimicsInterface;

}  //End Namespace Simics
} //namespace Flexus

#endif //FLEXUS_CORE_SIMICS_INTERFACE_HPP_INCLUDED
