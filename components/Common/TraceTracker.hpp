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
#ifndef _TRACKER_HPP_
#define _TRACKER_HPP_

#include <map>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>
#include <core/component.hpp>
#include <core/flexus.hpp>
#include <core/target.hpp>
#include <core/types.hpp>
#include <core/stats.hpp>

#include <components/Common/Slices/TransactionTracker.hpp>
#include <core/simics/configuration_api.hpp>
#include <core/simics/mai_api.hpp>

#include DBG_Control()

namespace nTraceTracker {

  namespace Simics = Flexus::Simics;
  namespace API = Simics::API;

  typedef unsigned long long address_t;
  
  // there is a corresponding enum in the AVFtool... make sure they change together
  typedef enum {FILL, READ, WRITE, ATOMIC, EVICT, FLUSH, INVALIDATION, DOWNGRADE, END} EventType;

  typedef struct {
    unsigned long delta;   
    unsigned long addr_hi;      
    unsigned long addr_lo;     
    unsigned long pc_hi;       
    unsigned long pc_lo;       
    unsigned long tid_hi;
    unsigned long tid_lo;
    unsigned long inst_word;
    unsigned char size;  
    unsigned char event;
    unsigned char node;
    unsigned char priv;
    int dummy;
  } Record; 
 
  class Tracer {
  public:
    Tracer() : is_ascii(true), fd(stdout) {} 
    Tracer(bool ascii, const char *fname) ;

    void output(unsigned long long time, 
                unsigned long long addr, 
                unsigned long long pc,
                unsigned long inst_word,
                unsigned char size, 
                EventType event, 
                unsigned char node, 
                bool priv); 
    void finish(unsigned long long time);

  private:
    bool is_ascii;
    FILE *fd;
    unsigned long long prev_time[16];
  };  
 
  class TraceTracker {

    Tracer *theTracer;
    unsigned long long *theInstCount;
    unsigned long long *theStartInst;
    API::conf_object_t *theCPUs[16];

    bool enable;

	public:
		void access      (int aNode, Flexus::SharedTypes::tFillLevel cache, address_t addr, address_t pc, bool prefetched, bool write, bool miss, bool priv, unsigned long long ltime);
    void commit      (int aNode, Flexus::SharedTypes::tFillLevel cache, address_t addr, address_t pc, unsigned long long ltime);
    void store       (int aNode, Flexus::SharedTypes::tFillLevel cache, address_t addr, address_t pc, bool miss, bool priv, unsigned long long ltime);
    void prefetch    (int aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
    void fill        (int aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, Flexus::SharedTypes::tFillLevel fillLevel, bool isFetch, bool isWrite);
    void prefetchFill(int aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, Flexus::SharedTypes::tFillLevel fillLevel);
    void prefetchHit (int aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, bool isWrite);
    void prefetchRedundant(int aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
    void insert      (int aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
    void eviction    (int aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, bool drop);
    void invalidation(int aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
    void downgrade   (int aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
    void invalidAck  (int aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
    void invalidTagCreate (int aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
    void invalidTagRefill (int aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);
    void invalidTagReplace(int aNode, Flexus::SharedTypes::tFillLevel cache, address_t block);

    void accessLoad  (int aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, address_t pc, unsigned long offset, int size, bool priv);
    void accessStore (int aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, address_t pc, unsigned long offset, int size, bool priv);
    void accessFetch (int aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, unsigned long offset, int size);
    void accessAtomic(int aNode, Flexus::SharedTypes::tFillLevel cache, address_t block, address_t pc, unsigned long offset, int size, bool priv);

    TraceTracker();
    void initialize();
    void finalize();

	};

} // namespace nTraceTracker

extern nTraceTracker::TraceTracker theTraceTracker;

#endif
