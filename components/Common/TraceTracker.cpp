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
#include "TraceTracker.hpp"
#include <core/boost_extensions/padded_string_cast.hpp>
#include <components/WhiteBox/WhiteBoxIface.hpp>
#include <core/simics/mai_api.hpp>
#include <core/simics/api_wrappers.hpp>
#undef fwrite
#include <stdlib.h>

  #define DBG_DefineCategories TraceTrack
  #define DBG_SetDefaultOps AddCat(TraceTrack)
  #include DBG_Control()

using namespace Flexus;
using namespace Core;

#define MyLevel Iface
#define MyLevel2 Tmp 

#define LOG2(x)         \
  ((x)==1 ? 0 :         \
  ((x)==2 ? 1 :         \
  ((x)==4 ? 2 :         \
  ((x)==8 ? 3 :         \
  ((x)==16 ? 4 :        \
  ((x)==32 ? 5 :        \
  ((x)==64 ? 6 :        \
  ((x)==128 ? 7 :       \
  ((x)==256 ? 8 :       \
  ((x)==512 ? 9 :       \
  ((x)==1024 ? 10 :     \
  ((x)==2048 ? 11 :     \
  ((x)==4096 ? 12 :     \
  ((x)==8192 ? 13 :     \
  ((x)==16384 ? 14 :    \
  ((x)==32768 ? 15 :    \
  ((x)==65536 ? 16 :    \
  ((x)==131072 ? 17 :   \
  ((x)==262144 ? 18 :   \
  ((x)==524288 ? 19 :   \
  ((x)==1048576 ? 20 :  \
  ((x)==2097152 ? 21 :  \
  ((x)==4194304 ? 22 :  \
  ((x)==8388608 ? 23 :  \
  ((x)==16777216 ? 24 : \
  ((x)==33554432 ? 25 : \
  ((x)==67108864 ? 26 : -0xffff)))))))))))))))))))))))))))

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define HI32(x) (x >> 32)
#define LO32(x) (x & ((1ULL<<32)-1))



namespace nTraceTracker {

int count;

Tracer::Tracer(bool ascii, const char *fname) 
{
  for (int i=0; i<16; i++) prev_time[i] = 0ULL;

  if (ascii) {
    is_ascii = true;
    fd = fopen(fname, "w");
  } else {
	count = 0;

    is_ascii = false;
    fd = popen("./tcgen", "w");
    //fd = popen("bzip2 -c -z -9 > trace.bz", "w");
    DBG_Assert(fd != NULL, (<< "Couldn't open pipe to tcgen"));

    // TODO: write header (256-bits)
    unsigned int word = 0;
    fwrite(&word, sizeof(unsigned int), 8, fd);
  }  
}

void Tracer::output(unsigned long long time, 
                    unsigned long long addr, 
                    unsigned long long pc,
                    unsigned long inst_word,
                    unsigned char size, 
                    EventType event, 
                    unsigned char node, 
                    bool priv) {
  if (is_ascii) {
    char evs[4];
    switch(event) {
    case READ:
      strcpy(evs, "RD");
      break;
    case WRITE:
      strcpy(evs, "WR");
      break;
    case EVICT:
      strcpy(evs, "EV");
      break;
    case FLUSH:
      strcpy(evs, "FL");
      break;
    case INVALIDATION:
      strcpy(evs, "IN");
      break;
    case DOWNGRADE:
      strcpy(evs, "DN");
      break;
    default:
      break;
    }

    fprintf(fd, "%10llu  addr: 0x%016llx  pc: 0x%016llx size: %1d  %s [%02d] %hhu\n",
                time, addr, pc, (int)size, evs, (int)node, (int)priv);
  } else {
    unsigned long delta = time - prev_time[node];
    prev_time[node] = time;

    // Get Thread ID from WhiteBox
    unsigned long long tid;

    nWhiteBox::WhiteBox::getWhiteBox()->getThread(node, tid);



    Record rec;
    rec.delta = delta;
    rec.addr_hi = HI32(addr);
    rec.addr_lo = LO32(addr);
    rec.pc_hi = HI32(pc);
    rec.pc_lo = LO32(pc);
    rec.tid_hi = HI32(tid);
    rec.tid_lo = LO32(tid);


    rec.inst_word = inst_word;
    rec.size = size;
    rec.event = event;
    rec.node = node;
    rec.priv = priv;
    rec.dummy = 0;
	count++;

//	if (count % 1000 == 0)
//		std::cout << "Delta = " << rec.delta << "Address = " << rec.addr_hi << " " << rec.addr_lo << "PC = " << rec.pc_hi << " " << rec.pc_lo << "TID = " << rec.tid_hi << " " << rec.tid_lo << "Inst = " << rec.inst_word << "Size = " << (int) rec.size << " Event = " << (int) rec.event << " Node = " << (int) rec.node << "Priv = " << (int) rec.priv << "\n";

    fwrite(&rec, sizeof(Record), 1, fd);
  }
}                         

void Tracer::finish(unsigned long long time) {
  if (is_ascii) {
    // nothing to do
  } else {
    fclose(fd);
  }
}

void TraceTracker::accessFetch(int aNode, SharedTypes::tFillLevel cache, address_t block, unsigned long offset, int size) {
  DBG_(MyLevel, (<< "[" << aNode << ":" << cache << "] accessLoad 0x" << std::hex << block << "," << offset));

  if (cache == SharedTypes::eL1) {
    // use this to count instructions (time) per node
    //theInstCount[aNode]++;
  }
}

#define TIME_KEEPING 0

void TraceTracker::accessLoad(int aNode, SharedTypes::tFillLevel cache, address_t block, address_t pc, unsigned long offset, int size, bool priv) {
  DBG_(MyLevel, (<< "[" << aNode << ":" << cache << "] accessLoad 0x" << std::hex << block << "," << offset));
  if (!enable) return;
  unsigned long inst_word = 0;
  #if TIME_KEEPING
  unsigned long long time = theInstCount[aNode];
  #else
  unsigned long long time = API::SIM_step_count(theCPUs[aNode]) - theStartInst[aNode];
  #endif
  theTracer->output(time, block+offset, pc, inst_word, size, READ, aNode, priv);
  theInstCount[aNode]++;
}

void TraceTracker::accessStore(int aNode, SharedTypes::tFillLevel cache, address_t block, address_t pc, unsigned long offset, int size, bool priv) {
  DBG_(MyLevel, (<< "[" << aNode << ":" << cache << "] accessStore 0x" << std::hex << block << "," << offset));

  if (!enable) return;
  unsigned long inst_word = 0;
  #if TIME_KEEPING
  unsigned long long time = theInstCount[aNode];
  #else
  unsigned long long time = API::SIM_step_count(theCPUs[aNode]) - theStartInst[aNode];
  #endif
  theTracer->output(time, block+offset, pc, inst_word, size, WRITE, aNode, priv);
  theInstCount[aNode]++;
}

void TraceTracker::accessAtomic(int aNode, SharedTypes::tFillLevel cache, address_t block, address_t pc, unsigned long offset, int size, bool priv){
  DBG_(MyLevel, (<< "[" << aNode << ":" << cache << "] accessAtomic 0x" << std::hex << block << "," << offset));

  if (!enable) return;
  unsigned long inst_word = 0;
  #if TIME_KEEPING
  unsigned long long time = theInstCount[aNode];
  #else
  unsigned long long time = API::SIM_step_count(theCPUs[aNode]) - theStartInst[aNode];
  #endif
  theTracer->output(time, block+offset, pc, inst_word, size, ATOMIC, aNode, priv);
  theInstCount[aNode]++;
}

void TraceTracker::eviction(int aNode, SharedTypes::tFillLevel cache, address_t block, bool drop) {
  DBG_(MyLevel, (<< "[" << aNode << ":" << cache << "] evict 0x" << std::hex << block));

  if (!enable) return;
  if (cache == SharedTypes::eL1) {
    #if TIME_KEEPING
    unsigned long long time = theInstCount[aNode];
    #else
    unsigned long long time = API::SIM_step_count(theCPUs[aNode]) - theStartInst[aNode];
    #endif
    theTracer->output(time, block, 0ULL, 0, 0, EVICT, aNode, false);
  }
}

void TraceTracker::invalidation(int aNode, SharedTypes::tFillLevel cache, address_t block) {
  //DBG_(Tmp, (<< "[" << aNode << ":" << cache << "] invalidate 0x" << std::hex << block));

  if (!enable) return;
  if (cache == SharedTypes::eL1) {
    #if TIME_KEEPING
    unsigned long long time = theInstCount[aNode];
    #else
    unsigned long long time = API::SIM_step_count(theCPUs[aNode]) - theStartInst[aNode];
    #endif
    theTracer->output(time, block, 0ULL, 0, 0, INVALIDATION, aNode, false);
  }
}

void TraceTracker::downgrade(int aNode, SharedTypes::tFillLevel cache, address_t block) {
  //DBG_(Tmp, (<< "[" << aNode << ":" << cache << "] downgrade 0x" << std::hex << block));

  if (!enable) return;
  if (cache == SharedTypes::eL1) {
    #if TIME_KEEPING
    unsigned long long time = theInstCount[aNode];
    #else
    unsigned long long time = API::SIM_step_count(theCPUs[aNode]) - theStartInst[aNode];
    #endif
    theTracer->output(time, block, 0ULL, 0, 0, DOWNGRADE, aNode, false);
  }
}



void TraceTracker::finalize() {
  DBG_(Iface, (<< "finalizing TraceTracker"));

  if (!enable) return;
  theTracer->finish(theInstCount[0]);
}

/********************************************************************************************************/

void TraceTracker::access(int aNode, SharedTypes::tFillLevel cache, address_t addr, address_t pc,
                          bool prefetched, bool write, bool miss, bool priv, unsigned long long ltime) {
  DBG_(MyLevel, (<< "[" << aNode << ":" << cache << "] access 0x" << std::hex << addr));
  //DBG_(Dev, (<< "[" << aNode << ":" << cache << "] access 0x" << std::hex << addr << " (ts:" << ltime <<")"));
}

void TraceTracker::commit(int aNode, SharedTypes::tFillLevel cache, address_t addr, address_t pc, unsigned long long aLogicalTime) {
  DBG_(MyLevel, (<< "[" << aNode << ":" << cache << "] commit 0x" << std::hex << addr));
}

void TraceTracker::store(int aNode, SharedTypes::tFillLevel cache, address_t addr, address_t pc,
                         bool miss, bool priv, unsigned long long aLogicalTime) {
  DBG_(MyLevel, (<< "[" << aNode << ":" << cache << "] store 0x" << std::hex << addr));
}

void TraceTracker::prefetch(int aNode, SharedTypes::tFillLevel cache, address_t block) {
  DBG_(MyLevel, (<< "[" << aNode << ":" << cache << "] prefetch 0x" << std::hex << block));
}

void TraceTracker::fill(int aNode, SharedTypes::tFillLevel cache, address_t block, SharedTypes::tFillLevel fillLevel, bool isFetch, bool isWrite) {
  DBG_(MyLevel, (<< "[" << aNode << ":" << cache << "] fill 0x" << std::hex << block));
  DBG_Assert(fillLevel != SharedTypes::eUnknown);
}

void TraceTracker::prefetchFill(int aNode, SharedTypes::tFillLevel cache, address_t block, SharedTypes::tFillLevel fillLevel) {
  DBG_(MyLevel, (<< "[" << aNode << ":" << cache << "] prefetch fill 0x" << std::hex << block));
  DBG_Assert(fillLevel != SharedTypes::eUnknown);
}

void TraceTracker::prefetchHit(int aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, bool isWrite) {
  DBG_(MyLevel, (<< "[" << aNode << ":" << cache << "] prefetch hit 0x" << std::hex << block));
}

void TraceTracker::prefetchRedundant(int aNode, Flexus::SharedTypes::tFillLevel cache, address_t block) {
  DBG_(MyLevel, (<< "[" << aNode << ":" << cache << "] prefetch redundant 0x" << std::hex << block));
}

void TraceTracker::insert(int aNode, SharedTypes::tFillLevel cache, address_t block) {
  DBG_(MyLevel, (<< "[" << aNode << ":" << cache << "] insert 0x" << std::hex << block));
}

void TraceTracker::invalidAck(int aNode, SharedTypes::tFillLevel cache, address_t block) {
  DBG_(MyLevel, (<< "[" << aNode << ":" << cache << "] invAck 0x" << std::hex << block));
}

void TraceTracker::invalidTagCreate(int aNode, SharedTypes::tFillLevel cache, address_t block) {
  DBG_(MyLevel, (<< "[" << aNode << ":" << cache << "] invTagCreate 0x" << std::hex << block));
}

void TraceTracker::invalidTagRefill(int aNode, SharedTypes::tFillLevel cache, address_t block) {
  DBG_(MyLevel, (<< "[" << aNode << ":" << cache << "] invTagRefill 0x" << std::hex << block));
}

void TraceTracker::invalidTagReplace(int aNode, SharedTypes::tFillLevel cache, address_t block) {
  DBG_(MyLevel, (<< "[" << aNode << ":" << cache << "] invTagReplace 0x" << std::hex << block));
}

TraceTracker::TraceTracker()
 : theTracer(NULL)
 , theInstCount(NULL)
 , theStartInst(NULL)
 , enable(0)
{}

void TraceTracker::initialize() {
  DBG_(Iface, (<< "initializing TraceTracker"));
  enable = true;
  Flexus::Stat::getStatManager()->addFinalizer( ll::bind(&TraceTracker::finalize, this) );

  theTracer = new Tracer(false, "trace.raw");
  int width = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
  DBG_(Tmp, (<< "width = " << width));
  theInstCount = new unsigned long long[width];
  for (int i=0; i<width; i++) theInstCount[i] = 0;

  theStartInst = new unsigned long long[width];
  for (int i=0; i<width; i++) {
		theCPUs[i] = Simics::APIFwd::SIM_get_processor(i);
		theStartInst[i] = API::SIM_step_count(theCPUs[i]);
  }  
}

} // namespace nTraceTracker

#ifndef _TRACETRACKER_OBJECT_DEFINED_
#define _TRACETRACKER_OBJECT_DEFINED_
nTraceTracker::TraceTracker theTraceTracker;
#endif
