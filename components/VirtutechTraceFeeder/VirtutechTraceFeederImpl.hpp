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

#define FLEXUS_BEGIN_COMPONENT VirtutechTraceFeederComponent
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

  #define DBG_DefineCategories VirtutechTraceFeeder, Feeder
  #define DBG_SetDefaultOps AddCat(VirtutechTraceFeeder | Feeder)
  #include DBG_Control()

#include "VirtutechTraceReader.hpp"

namespace nVirtutechTraceFeeder {


using namespace Flexus;
using namespace Core;
using namespace SharedTypes;


using boost::intrusive_ptr;

typedef nVirtutechTraceReader::TraceEntry TraceEntry;
typedef SharedTypes::PhysicalMemoryAddress MemoryAddress;


template <class Cfg>
class VirtutechTraceFeederComponent : public FlexusComponentBase<VirtutechTraceFeederComponent, Cfg> {
  FLEXUS_COMPONENT_IMPL(VirtutechTraceFeederComponent, Cfg);

public:
  VirtutechTraceFeederComponent( FLEXUS_COMP_CONSTRUCTOR_ARGS )
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  { }

  ~VirtutechTraceFeederComponent() {
  }

  // Initialization
  void initialize() {
    myTraceReader.init();
  }

  //Ports
  struct ToMemory : public PushOutputPortArray<MemoryTransport, NUM_PROCS> { };

  struct FromMemory : public PushInputPortArray<MemoryTransport, NUM_PROCS>, AlwaysAvailable {
    typedef FLEXUS_IO_LIST_EMPTY Inputs;
    typedef FLEXUS_IO_LIST_EMPTY Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void push(self& aFeeder, index_t anIndex, MemoryTransport transport) {
      // do absolutely nothing!
    }
  };

  //Drive Interfaces
  struct VirtutechTraceFeederDrive {
    typedef FLEXUS_IO_LIST( 1, Availability<ToMemory> ) Inputs;
    typedef FLEXUS_IO_LIST( 1, Value<ToMemory> ) Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void doCycle(self & aFeeder) {
      TraceEntry nextEntry;
      aFeeder.myTraceReader.next(nextEntry);
      DBG_(VVerb, ( << "got: " << nextEntry ) );

      // create the transport to pass to the memory
      MemoryTransport trans;

      intrusive_ptr<MemoryMessage> msg;
      if(nextEntry.write) {
        msg = new MemoryMessage(MemoryMessage::WriteReq, MemoryAddress(nextEntry.address));
      } else {
        msg = new MemoryMessage(MemoryMessage::ReadReq, MemoryAddress(nextEntry.address));
      }
      trans.set(MemoryMessageTag, msg);

      intrusive_ptr<ExecuteState> exec( new ExecuteState(MemoryAddress(nextEntry.pc), nextEntry.priv) );
      trans.set(ExecuteStateTag, exec);

      FLEXUS_CHANNEL_ARRAY(aFeeder, ToMemory, nextEntry.node) << trans;
    }

  };  // end VirtutechTraceFeederDrive

  // Declare the list of Drive interfaces
  typedef FLEXUS_DRIVE_LIST(1, VirtutechTraceFeederDrive) DriveInterfaces;

private:

  // the trace reader
  nVirtutechTraceReader::VirtutechTraceReader myTraceReader;

};

FLEXUS_COMPONENT_EMPTY_CONFIGURATION_TEMPLATE(VirtutechTraceFeederConfiguration);

} //End Namespace nVirtutechTraceFeeder

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT VirtutechTraceFeederComponent

  #define DBG_Reset
  #include DBG_Control()

