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
/*
   InorderSimicsFeeder.hpp

   Stephen Somogyi
   Carnegie Mellon University

   22Jul2003 - initial version (derived from in-order SimicsTraceFeederImpl.hpp)

   This is both a module for Simics and Flexus.  As a Simics module,
   it snoops all memory accesses.  These accesses are converted into
   Flexus trace instructions.  The module is also a Flexus
   fetch component, and injects these instructions into the Flexus
   pipeline.

*/


#include <components/InorderSimicsFeeder/InorderSimicsFeeder.hpp>

#include <iomanip>

#include <boost/shared_ptr.hpp>

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

#include <core/simics/configuration_api.hpp>
#include <core/simics/mai_api.hpp>
#include <core/simics/simics_interface.hpp>
#include <core/stats.hpp>

#include <components/Common/DoubleWord.hpp>
#include <components/Common/Transports/InstructionTransport.hpp>
#include <components/Common/Slices/ArchitecturalInstruction.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>

  #define DBG_DefineCategories Feeder
  #define DBG_SetDefaultOps AddCat(Feeder)
  #include DBG_Control()

namespace nInorderSimicsFeeder {

using namespace Flexus;

using namespace Core;
using namespace SharedTypes;

using namespace Simics::API;

using boost::intrusive_ptr;

typedef ArchitecturalInstruction Instruction;

}


#include <components/InorderSimicsFeeder/DebugTrace.hpp>
#include <components/InorderSimicsFeeder/StoreBuffer.hpp>
#include <components/InorderSimicsFeeder/TraceConsumer.hpp>
#include <components/InorderSimicsFeeder/CycleManager.hpp>
#include <components/InorderSimicsFeeder/SimicsTracer.hpp>
#include <components/InorderSimicsFeeder/MemoryTrace.hpp>


#define FLEXUS_BEGIN_COMPONENT InorderSimicsFeeder
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()


namespace nInorderSimicsFeeder {

using namespace Flexus;



/*****************************************************************
 * Here the "Flexus" part of the implementation begins.
 *****************************************************************/

class FLEXUS_COMPONENT(InorderSimicsFeeder){
  FLEXUS_COMPONENT_IMPL( InorderSimicsFeeder );

  //The Simics objects (one for each processor) for getting trace data
  int theNumCPUs;
  SimicsTracer * theTracers;

  std::vector< boost::shared_ptr<SimicsTraceConsumer> > theConsumers;

  boost::scoped_ptr<SimicsCycleManager> theSimicsCycleManager;

  // Memory trace (optional)
  bool traceInit;
  unsigned long long theLastTransactionStart;
  nMemoryTrace::MemoryTrace * theMemoryTrace;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(InorderSimicsFeeder)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {
    theNumCPUs = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
    theTracers = new SimicsTracer [ theNumCPUs ];
    doInitialize();
  }

  virtual ~InorderSimicsFeederComponent() {}

  //InstructionOutputPort
  //=====================
    bool available(interface::InstructionOutputPort const &, index_t anIndex) {
      if (!cfg.UseTrace) {
          return (theConsumers[anIndex]->isReady());
      } else {
          return true;
      }
    }

    InstructionTransport pull(interface::InstructionOutputPort const &, index_t anIndex) {
      InstructionTransport aTransport;
      DBG_Assert(anIndex < theConsumers.size() );

      if (!cfg.UseTrace) {
        aTransport.set(ArchitecturalInstructionTag, theConsumers[anIndex]->ready_instruction());
        theConsumers[anIndex]->begin_instruction();
      } else {
        if (!traceInit) {
          traceInit = true;
          DBG_(Crit, ( << "Initializing from Trace file " << cfg.TraceFile ));
          theMemoryTrace = new nMemoryTrace::MemoryTrace(theNumCPUs);
          theMemoryTrace->init(cfg.TraceFile.c_str());
        }
        nMemoryTrace::MemoryOp op(theMemoryTrace->getNextMemoryOp(anIndex));

        // map the MemoryOp into an Instruction
        boost::intrusive_ptr<Instruction> instr( new ArchitecturalInstruction());
        switch (op.type) {
        case nMemoryTrace::NOP:
          instr->setIsNop();
          break;
        case nMemoryTrace::LD:
          instr->setIsLoad();
          break;
        case nMemoryTrace::ST:
          instr->setIsStore();
          break;
        case nMemoryTrace::TSTART:
          // start new transaction
          theLastTransactionStart = Flexus::Core::theFlexus->cycleCount();
          break;
        case nMemoryTrace::TSTOP:
          DBG_(Crit, ( << "Transaction completed in " <<
           Flexus::Core::theFlexus->cycleCount() - theLastTransactionStart + 1
           << " cycles"));
          break;
        case nMemoryTrace::MSG:
          // do nothing, shouldn't reach here
          break;
        }
        instr->setAddress((Flexus::SharedTypes::PhysicalMemoryAddress)op.addr);
        instr->setPhysInstAddress((Flexus::SharedTypes::PhysicalMemoryAddress)0xbaadf00d);  // UNIMPLEMENTED (for now)

        instr->setTrace();
        instr->setStartTime(Flexus::Core::theFlexus->cycleCount());

        if (op.type != nMemoryTrace::NOP)
          DBG_(Iface, (<< "From trace: [" << anIndex << "] -- " << *instr));

        // stick it in transport
        aTransport.set(ArchitecturalInstructionTag, instr);

        // end simulation?
        if (theMemoryTrace->isComplete()) {
          Flexus::Core::theFlexus->terminateSimulation();
        }
      }

      return aTransport;
    }


  void registerSnoopInterface(Simics::API::conf_class_t * trace_class) {
      Simics::API::timing_model_interface_t *snoop_interface;

      snoop_interface = new Simics::API::timing_model_interface_t(); //LEAKS - Need to fix
      snoop_interface->operate = &Simics::make_signature_from_addin_fn<Simics::API::operate_func_t>::with<SimicsTracer,SimicsTracerImpl,&SimicsTracerImpl::trace_snoop_operate>::trampoline;
      Simics::API::SIM_register_interface(trace_class, "snoop-memory", snoop_interface);
  }

  void registerTimingInterface(Simics::API::conf_class_t * trace_class) {
    Simics::API::timing_model_interface_t *timing_interface;

    timing_interface = new Simics::API::timing_model_interface_t(); //LEAKS - Need to fix
    timing_interface->operate = &Simics::make_signature_from_addin_fn2<Simics::API::operate_func_t>::with<SimicsTracer,SimicsTracerImpl,&SimicsTracerImpl::trace_mem_hier_operate>::trampoline;
    Simics::API::SIM_register_interface(trace_class, "timing-model", timing_interface);
  }

  bool isQuiesced() const {
    if (theConsumers.empty()) {
      //Not yet initialized, so we must be quiesced
      return true;
    } else {
      for(int i = 0; i < theNumCPUs; ++i) {
        if (theConsumers[i]->isQuiesced()) {
          return true;
        }
      }
    }
    return true;
  }

  void
  initialize(void)
  {

    #ifndef FLEXUS_FEEDER_OLD_SCHEDULING
      DBG_( Dev, ( << "Using new scheduling mechanism."  ) Comp(*this) );
      Flexus::Simics::theSimicsInterface->disableCycleHook();
    #endif

  }

  /*
    Initialize. Install ourselves as the default. This is equivalent to
    the init_local() function in an actual Simics module.
  */
  void
  doInitialize(void)
  {
    DBG_( Dev, ( << "Initializing InorderSimicsFeeder."  ) Comp(*this) );

    Simics::APIFwd::SIM_flush_all_caches();

    for(int i = 0; i < theNumCPUs; ++i) {
      std::string name;
      if (theNumCPUs > 1) {
        name = boost::padded_string_cast<3,'0'>(i) + "-feeder" ;
      } else {
        name = "sys-feeder" ;
      }
      theConsumers.push_back( boost::shared_ptr<SimicsTraceConsumer>(new SimicsTraceConsumer(name)) );
    }

    bool client_server = false;
    if (Simics::API::SIM_get_object("cpu0") == 0) {
      if (Simics::API::SIM_get_object("server_cpu0") == 0) {
        DBG_Assert( false, ( << "InorderSimicsFeeder cannot locate cpu0 or server_cpu0 objects." ) );
      } else {
        DBG_( Crit, ( << "InorderSimicsFeeder detected client-server simulation.  Connecting to server CPUs only." ) );
        client_server = true;
      }
    }


    theSimicsCycleManager.reset(new SimicsCycleManager(theConsumers, client_server ));

    Simics::Factory<SimicsTracer> tracer_factory;

    Simics::API::conf_class_t * trace_class = tracer_factory.getSimicsClass();

    registerTimingInterface(trace_class);
    registerSnoopInterface(trace_class);

    for(int ii = 0; ii < theNumCPUs ; ++ii) {
      std::string feeder_name("flexus-feeder");
      if (theNumCPUs > 1) {
        feeder_name += '-' + lexical_cast<std::string> (ii);
      }
      theTracers[ii] = tracer_factory.create(feeder_name);
    }

    for(int ii = 0; ii < theNumCPUs; ++ii) {
      std::string name("cpu");
      if (client_server) {
        name = "server_cpu";
      }
      name += boost::lexical_cast<std::string>(ii);
      DBG_( Crit, ( << "Connecting: " << name ) );
      Simics::API::conf_object_t * cpu = Simics::API::SIM_get_object( name.c_str() );
      DBG_Assert(cpu != 0, ( << "InorderSimicsFeeder cannot locate " << name << " object. No such object in Simics" ));

      theTracers[ii]->init(cpu, ii, cfg.StallCap);
      theTracers[ii]->setTraceConsumer(theConsumers[ii]);
      theTracers[ii]->setCycleManager(*theSimicsCycleManager);
    }

    /* Enable instruction tracing */
    Simics::API::attr_value_t attr;
    attr.kind = Simics::API::Sim_Val_String;
    attr.u.string = "instruction-fetch-trace";
    Simics::API::SIM_set_attribute(Simics::API::SIM_get_object("sim"), "instruction_profile_mode", &attr);

    // set traceInit to false for now
    // at first instruction pull, we'll really initialize the trace
    traceInit = false;

    DBG_( VVerb, ( << "Done initializing InorderSimicsFeeder."  ) Comp(*this));
  }  // end initialize()

};  // end class SimicsMPTraceFeederComponent

std::string theTraceFile("theTraceFile");

}  // end Namespace nInorderSimicsFeeder

FLEXUS_COMPONENT_INSTANTIATOR( InorderSimicsFeeder, nInorderSimicsFeeder );
FLEXUS_PORT_ARRAY_WIDTH( InorderSimicsFeeder, InstructionOutputPort ) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT InorderSimicsFeeder

  #define DBG_Reset
  #include DBG_Control()
