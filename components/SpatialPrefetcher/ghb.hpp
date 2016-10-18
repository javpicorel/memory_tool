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

#include <memory>
#include <fstream>
#include <sstream>

#include <ext/hash_map>

#include "seq_map.hpp"

using namespace boost::multi_index;

  #define DBG_SetDefaultOps AddCat(SpatialPrefetch)
  #include DBG_Control()

namespace nGHBPrefetcher {

typedef unsigned long MemoryAddress;

struct ghb_it_ent_t {
  MemoryAddress tag;
  int ptr;
  ghb_it_ent_t():tag(0),ptr(0) { }
};
struct ghb_ent_t {
  MemoryAddress addr;
  int prev;
  ghb_ent_t():addr(0),prev(-1) { }
};

class GHBPrefetcher {
	//LRUarray<ghb_it_ent_t,ghb_it_log_nsets,1> ghb_it;
  flexus_boost_set_assoc<MemoryAddress,ghb_it_ent_t> theIndexTable;
  typedef flexus_boost_set_assoc<MemoryAddress,ghb_it_ent_t>::iterator IndexTableIter;
	ghb_ent_t * ghb;
	int ghb_head;

	long long ghb_req_id;

  std::string theName;
  int theNodeId;
  unsigned long theBlockSize;
  unsigned long theBlockOffsetMask;
  std::deque<MemoryAddress> thePrefetches;

	unsigned int ghb_size;
	unsigned int ghb_max_idx;
	unsigned int ghb_depth;

public:
  GHBPrefetcher(std::string statName, int aNode)
    : theName(statName)
    , theNodeId(aNode)
    , ghb_depth(4)
  {}

  void init(int blockSize, int ghbSize) {
    theBlockSize = blockSize;
    theBlockOffsetMask = theBlockSize - 1;
    ghb_size = ghbSize;
    ghb_max_idx = ghb_size << 4;
    theIndexTable.init(ghb_size,1,0);
    ghb = new ghb_ent_t[ghb_size];
  }

	void ghb_prefetch(long long req_id,const MemoryAddress addr) {
		if (req_id != ghb_req_id) return;
    thePrefetches.push_back(addr);
	}

  bool prefetchReady() {
    return( !thePrefetches.empty() );
  }

  MemoryAddress getPrefetch() {
    MemoryAddress addr = thePrefetches.front();
    thePrefetches.pop_front();
    return addr;
  }

	void access(const MemoryAddress PC, const MemoryAddress addr)
	{
		int ptr = -1;
		IndexTableIter it_iter = theIndexTable.find(PC);
		if (it_iter != theIndexTable.end()) {
			ptr = it_iter->second.ptr;
		} else {
      if(theIndexTable.size() >= 1) {
        theIndexTable.pop_front();
      }
			it_iter = theIndexTable.insert( std::make_pair(PC,ghb_it_ent_t()) ).first;
			it_iter->second.tag = PC;
		}
		it_iter->second.ptr = ghb_head;
		ghb_ent_t* head_ent = &ghb[ghb_head%ghb_size];
		MemoryAddress baddr = makeBlockAddress(addr);
		head_ent->addr = baddr;
		head_ent->prev = ptr;
		ghb_head = (ghb_head+1)%ghb_max_idx;

		int ghb_deltas[ghb_size-1];
		int dc = 0;
		for(ptr = it_iter->second.ptr; unsigned(dc)<(ghb_size-1); ptr = ghb[ptr%ghb_size].prev) {
			int prev_ptr = ghb[ptr%ghb_size].prev;
			if (((ghb_head-1 - prev_ptr + ghb_max_idx)%ghb_max_idx) > ghb_size) break;
			ghb_deltas[dc++] = ghb[ptr%ghb_size].addr - ghb[prev_ptr%ghb_size].addr;
		}

		int d0,d1;
		d0 = ghb_deltas[0];
		d1 = ghb_deltas[1];
		int walk_len = 2;
		for(dc -= 2; dc>0 && ghb_deltas[dc]!=d0 && ghb_deltas[dc+1]!=d1; --dc) ++walk_len;
		if (dc<=0) return;

		long long new_req_id = ++ghb_req_id;

		for(unsigned int depth=1; (depth <= ghb_depth) && dc--; ++depth) {
			baddr += ghb_deltas[dc];
			//call(walk_len + depth,this,&self_t::ghb_prefetch,new_req_id,baddr);
			ghb_prefetch(new_req_id,baddr);
		}
	}

  MemoryAddress makeBlockAddress(MemoryAddress addr) {
    MemoryAddress res = (addr & ~(theBlockOffsetMask));
    //DBG_(Dev, ( << "makeBlockAddress: addr=" << std::hex << addr << "res=" << res ) );
    return res;
  }

}; // end class GHBPrefetcher

} // end namespace nGHBPrefetcher

  #define DBG_Reset
  #include DBG_Control()
