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
  V9 Memory Op
*/

namespace nInorderSimicsFeeder {
using namespace Flexus;

using namespace Core;
using namespace SharedTypes;

  // when debugging, every call from Simics to trace_mem_hier_operate() will
  // populate one of these entries
  struct DebugTraceEntry {
    PhysicalMemoryAddress addr;
    bool control;
    bool data;
    bool instruction;
    bool prefetch;
    bool read;
    bool write;

    bool atomic;
    bool may_stall;
    int ret_value;
    char assem[32];
  };

  // a class for debugging through traces that Simics feeds us.  It is
  // implemented as a circular queue that tracks the last N calls to our
  // callback function.  It dumps this backtrace when prompted.
  const int TraceLen = 5;
  class SimicsTraceDebugger {
    int theIndex;  // flexusIdx for debugging purposes
    DebugTraceEntry entries[TraceLen];
    int nextToWrite;
    bool dumped;  // did we dump the trace this cycle?

  public:
    SimicsTraceDebugger()
      : theIndex(-1)
      , nextToWrite(0)
      , dumped(false)
    {
      for(int ii = 0; ii < TraceLen; ii++) {
        entries[ii].addr = PhysicalMemoryAddress(0xfedcba99);
      }
    }

    void init(int index) {
      theIndex = index;
    }

    void nextCallback(Simics::APIFwd::memory_transaction_t* mem_trans) {
      entries[nextToWrite].control = Simics::API::SIM_mem_op_is_control(&mem_trans->s);
      entries[nextToWrite].data = Simics::API::SIM_mem_op_is_data(&mem_trans->s);
      entries[nextToWrite].instruction = Simics::API::SIM_mem_op_is_instruction(&mem_trans->s);
      entries[nextToWrite].prefetch = Simics::API::SIM_mem_op_is_prefetch(&mem_trans->s);
      entries[nextToWrite].read = Simics::API::SIM_mem_op_is_read(&mem_trans->s);
      entries[nextToWrite].write = Simics::API::SIM_mem_op_is_write(&mem_trans->s);

      entries[nextToWrite].addr = PhysicalMemoryAddress(mem_trans->s.physical_address);
      entries[nextToWrite].atomic = mem_trans->s.atomic;
      entries[nextToWrite].may_stall = mem_trans->s.may_stall;
      entries[nextToWrite].ret_value = -1;

      if(entries[nextToWrite].instruction) {
        Simics::API::tuple_int_string_t * retval = Simics::API::SIM_disassemble(Simics::API::SIM_current_processor(),
                                                      mem_trans->s.physical_address,
                                                      0);
        if(retval) {
          strncpy(entries[nextToWrite].assem, retval->string, 32);
        } else {
          strncpy(entries[nextToWrite].assem, "non-disassemblable", 32);
        }
      }
      else {
        strncpy(entries[nextToWrite].assem, "", 32);
      }
      dumped = false;
    }

    void ret(int returnedToSimics) {
      DBG_Assert(entries[nextToWrite].ret_value == -1);
      entries[nextToWrite].ret_value = returnedToSimics;
      nextToWrite = (nextToWrite+1) % TraceLen;

      // if we dumped this cycle, the most current entry could not have
      // had a return value, since that value was only known now - so
      // dump the return value so the user has complete informatino
      if(dumped) {
        DBG_(Dev, SetNumeric( (FlexusIdx) theIndex )
                  ( << " returned this cycle: " << returnedToSimics ) );
      }
    }

    void dump() {
      // if ret value has not been set, the nextToWrite entry is not yet
      // complete and should be dumped last; otherwise the current entry
      // is complete, and the nextToWrite entry is the oldest (dumped first)
      int start;
      if(entries[nextToWrite].ret_value == -1) {
        start = (nextToWrite+1) % TraceLen;
      } else {
        start = nextToWrite;
      }
      int idx = start;

      DBG_(Dev, SetNumeric( (FlexusIdx) theIndex )
                ( << "dumping trace debug info from oldest to current:" ) );
      do {
        DBG_(Dev, SetNumeric( (FlexusIdx) theIndex )
                  ( << " " << std::hex << entries[idx].addr
                    << " ctrl=" << entries[idx].control
                    << " data=" << entries[idx].data
                    << " inst=" << entries[idx].instruction
                    << " prefetch=" << entries[idx].prefetch
                    << " read=" << entries[idx].read
                    << " write=" << entries[idx].write
                    << " atomic=" << entries[idx].atomic
                    << " may_stall=" << entries[idx].may_stall
                    << " ret=" << entries[idx].ret_value
                    << (entries[idx].instruction ? " op: " : "")
                    << entries[idx].assem
                  ) );
        idx = (idx+1) % TraceLen;
      }
      while(idx != start);

      dumped = true;
    }

  };  // class SimicsTraceDebugger


}
