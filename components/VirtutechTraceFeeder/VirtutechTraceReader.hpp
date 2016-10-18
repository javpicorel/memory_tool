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
#define USE_CMU_TRACE_FORMAT

namespace nVirtutechTraceReader {

typedef unsigned long Time;
typedef long long Address;

struct TraceEntry {
  TraceEntry()
  {}
  TraceEntry(bool isWrite, Address anAddr, Address aPC, int aNode, bool isPriv)
    : write(isWrite)
    , address(anAddr)
    , pc(aPC)
    , node(aNode)
    , priv(isPriv)
  {}

  bool write;         // true=>write  false=>read
  Address address;    // address of data reference
  Address pc;         // PC of instruction that caused reference
  int node;           // node of read or write
  bool priv;          // privileged instruction?

  friend std::ostream & operator << (std::ostream & anOstream, const TraceEntry & anEntry) {
    if(anEntry.write) {
      anOstream << "write";
    } else {
      anOstream << "read ";
    }
    anOstream << " address=" << std::hex << anEntry.address
              << " pc=" << anEntry.pc
              << " node=" << anEntry.node;
    if(anEntry.priv) {
      anOstream << " privileged";
    } else {
      anOstream << " non-priv";
    }
    return anOstream;
  }

};


class VirtutechTraceReader {
  FILE* myTraceFile;

public:
  VirtutechTraceReader()
  {}

  void init() {
#ifdef USE_CMU_TRACE_FORMAT
    myTraceFile = fopen("tracedata", "r");
    DBG_Assert(myTraceFile, ( << "could not open file \"tracedata\"." ) );
#endif
  }

  void next(TraceEntry & entry) {
    // read until we find an appropriate trace entry
    char buf[256];

#ifdef USE_CMU_TRACE_FORMAT
    int node;
    unsigned int pc;
    unsigned int addr;
    unsigned int data;
    int write;
    int pid;
    int priv;

    if(fgets(buf, 256, myTraceFile) == 0) {
      // EOF
      DBG_(Crit, ( << "end of trace file" ) );
      throw Flexus::Core::FlexusException();
    }
    sscanf(buf, "%d %x %x %x %d %d %d", &node, &pc, &addr, &data, &write, &pid, &priv);

    entry.node = node;
    entry.pc = pc;
    entry.address = addr;
    entry.write = (bool)write;
    entry.priv = (bool)priv;
#endif  // USE_CMU_TRACE_FORMAT
  }

};  // end class VirtutechTraceReader

}  // end namespace nVirtutechTraceReader
