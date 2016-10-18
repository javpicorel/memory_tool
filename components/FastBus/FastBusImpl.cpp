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

#include <components/FastBus/FastBus.hpp>

#include <fstream>
#include <ext/hash_map>
#include <boost/tuple/tuple.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <core/stats.hpp>

#include <core/simics/simics_interface.hpp>
#include <core/simics/mai_api.hpp>

#define FLEXUS_BEGIN_COMPONENT FastBus
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

  #define DBG_DefineCategories FastBus
  #define DBG_SetDefaultOps AddCat(FastBus)
  #include DBG_Control()

namespace nFastBus {

using namespace Flexus;
using namespace Flexus::Core;
using namespace Flexus::SharedTypes;
namespace Stat = Flexus::Stat;

typedef unsigned long block_address_t;
typedef unsigned long coherence_state_t;
coherence_state_t kInvalid = 0x00000000UL;
coherence_state_t kExclusive = 0x80000000UL;
coherence_state_t kNAW = 0x20000000UL;
coherence_state_t kDMA = 0x10000000UL;
coherence_state_t kOwner = 0x0000000FUL; //At most 16 nodes
coherence_state_t kSharers = 0x0000FFFFUL; //At most 16 nodes
const unsigned char DIR_STATE_INVALID   = 0;
const unsigned char DIR_STATE_SHARED    = 1;
const unsigned char DIR_STATE_EXCLUSIVE = 2;

unsigned long population(unsigned long x) {
    x = ((x & 0xAAAAAAAAUL) >> 1) + (x & 0x55555555UL);
    x = ((x & 0xCCCCCCCCUL) >> 2) + (x & 0x33333333UL);
    x = ((x & 0xF0F0F0F0UL) >> 4) + (x & 0x0F0F0F0FUL);
    x = ((x & 0xFF00FF00UL) >> 8) + (x & 0x00FF00FFUL);
    x = ((x & 0xFFFF0000UL) >> 16) + (x & 0x0000FFFFUL);
    return x;
}

enum SnoopSource {
  eSrcDMA,
  eSrcNAW,
  eSrcNormal,
};

class dir_entry_t {
public:
  dir_entry_t(coherence_state_t s) : state(s), theWasModified(false), thePastReaders(s & kSharers) {}
  dir_entry_t() : state(0), theWasModified(false), thePastReaders(0) {}
  coherence_state_t state;
  bool theWasModified;
  unsigned long thePastReaders;
};

bool isSharer( coherence_state_t state, index_t node) {
    return( state & (1<<node) );
}

void setSharer( coherence_state_t state, index_t node) {
    state |= (1<<node);
}

bool isExclusive( coherence_state_t state) {
    return( state & kExclusive );
}

struct IntHash {
  std::size_t operator()(unsigned long key) const {
    key = key >> 6;
    return key;
  }
};

unsigned int log_base2(unsigned int num) {
  unsigned int ii = 0;
  while(num > 1) {
    ii++;
    num >>= 1;
  }
  return ii;
}


class MemoryMap : public boost::counted_base {
public:
  MemoryMap(unsigned int page_size, unsigned num_nodes, bool round_robin)
	  : thePageSize(page_size)
		, theNumNodes(num_nodes)
		, theRoundRobin(round_robin)
		{
		  thePageSizeL2 = log_base2(thePageSize);
		}

	void loadState(std::string const & aDirName) {
    // read map from "page_map"
    std::string fname = aDirName + "/page_map.out";
    std::ifstream in(fname.c_str());
    if (in) {
      DBG_(Dev, ( << "Page map file page_map.out found.  Reading contents...") );
      int count = 0;
      while(in) {
        int node;
        long long addr;
        in >> node;
        in >> addr;
        if (in.good()) {
          DBG_(VVerb, ( << "Page " << addr << " assigned to node " << node ) );
          pagemap_t::iterator ignored;
          bool is_new;
          boost::tie(ignored, is_new) = thePageMap.insert( std::make_pair(addr, node) );
          DBG_Assert(is_new, (<< "(" << addr << "," << node << ")"));
          ++count;
        }
      }
      DBG_(Dev, (<< "Assigned " << count << " pages."));
    } else {
      DBG_(Dev, (<< "Page map file page_map.out was not found."));
    }
	}

	void saveState(std::string const & aDirName) {
	  // write page map to "page_map"
    std::string fname = aDirName + "/page_map.out";
    std::ofstream out(fname.c_str());
    if (out) {
      pagemap_t::iterator iter = thePageMap.begin();
      while (iter != thePageMap.end()) {
        out << iter->second << " " << static_cast<long long>(iter->first) << "\n";
        ++iter;
      }
      out.close();
    } else {
      DBG_(Crit, (<< "Unable to save page map to " << fname));
    }
	}

  int node(unsigned req_node, block_address_t addr) {
    pagemap_t::iterator iter;
		bool is_new;
		block_address_t page_addr = addr >> thePageSizeL2;

    // 'insert', which won't really insert if its already there
    boost::tie(iter, is_new) = thePageMap.insert( std::make_pair( page_addr, req_node ) ); //Optimized for first-touch

		// if found, return mapping
		if (is_new && theRoundRobin) {
		  // overwrite the default first-touch
			iter->second = (page_addr) & (theNumNodes-1);
		} // first-touch done by default in 'insert' call above

		return iter->second;
	}

private:
  unsigned int thePageSize;
	unsigned theNumNodes;
	bool theRoundRobin;

	unsigned int thePageSizeL2;

  typedef __gnu_cxx::hash_map<block_address_t,unsigned, IntHash > pagemap_t;
	pagemap_t thePageMap;
};


class FLEXUS_COMPONENT(FastBus) {
  FLEXUS_COMPONENT_IMPL( FastBus );

  typedef __gnu_cxx::hash_map<block_address_t,dir_entry_t, IntHash > directory_t;
  directory_t theDirectory;
  block_address_t theBlockMask;
  MemoryMessage theSnoopMessage;
	boost::intrusive_ptr<MemoryMap> theMemoryMap;

  Stat::StatCounter ** theUpgrades_S2X;
  Stat::StatCounter ** theUpgrades_X2X;
  Stat::StatCounter ** theConsumptions;
  Stat::StatCounter ** theDMABlockReads;
  Stat::StatCounter ** theDMABlockWrites;
  Stat::StatCounter ** theProductions;
  Stat::StatCounter ** theOffChipData;
  Stat::StatCounter ** theEvictions;
  Stat::StatCounter ** theFlushes;
  Stat::StatCounter ** theInvalidations;

  Stat::StatCounter theDMATransfers;
  Stat::StatCounter theDMABytes;
  Stat::StatUniqueCounter<unsigned long> theDMAAddresses;

  Stat::StatCounter theNonAllocateWrites;
  Stat::StatCounter theNonAllocateWriteBytes;
  Stat::StatUniqueCounter<unsigned long> theNonAllocateWriteAddresses;

  Stat::StatCounter theInvals;
  Stat::StatCounter theInvals_Valid;
  Stat::StatCounter theInvals_Dirty;

  Stat::StatCounter theInvals_Norm;
  Stat::StatCounter theInvals_Norm_Valid;
  Stat::StatCounter theInvals_Norm_Dirty;

  Stat::StatCounter theInvals_DMA;
  Stat::StatCounter theInvals_DMA_Valid;
  Stat::StatCounter theInvals_DMA_Dirty;

  Stat::StatCounter theInvals_NAW;
  Stat::StatCounter theInvals_NAW_Valid;
  Stat::StatCounter theInvals_NAW_Dirty;

  Stat::StatCounter theDowngrades;
  Stat::StatCounter theDowngrades_Dirty;

  Stat::StatCounter theDowngrades_Norm;
  Stat::StatCounter theDowngrades_Norm_Dirty;

  Stat::StatCounter theDowngrades_DMA;
  Stat::StatCounter theDowngrades_DMA_Dirty;

  Stat::StatCounter theOffChipReads_Fetch;
  Stat::StatCounter theOffChipReads_Prefetch;
  Stat::StatCounter theOffChipReads_Cold;
  Stat::StatCounter theOffChipReads_Replacement;
  Stat::StatCounter theOffChipReads_Coherence;
  Stat::StatCounter theOffChipReads_DMA;
  Stat::StatCounter theOffChipReads_NAW;

  Stat::StatInstanceCounter<long long> theDMAReadPCs;
  Stat::StatInstanceCounter<long long> theNAWReadPCs;

  std::vector<unsigned long long> theLastStepCount_OffChipRead;
  std::vector<unsigned long long> theLastStepCount_OffChipRead_Coherence;

  Stat::StatAverage theAvgInsPer_OffChipRead;
  Stat::StatStdDev  theStdDevInsPer_OffChipRead;
  Stat::StatAverage theAvgInsPer_OffChipRead_Coherence;
  Stat::StatStdDev  theStdDevInsPer_OffChipRead_Coherence;


public:
  FLEXUS_COMPONENT_CONSTRUCTOR(FastBus)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theSnoopMessage(MemoryMessage::Invalidate)
    , theDMATransfers("sys-bus-DMA:Transfers")
    , theDMABytes("sys-bus-DMA:Bytes")
    , theDMAAddresses("sys-bus-DMA:UniqueAddresses")
    , theNonAllocateWrites("sys-bus-NAW:Writes")
    , theNonAllocateWriteBytes("sys-bus-NAW:Bytes")
    , theNonAllocateWriteAddresses("sys-bus-NAW:UniqueAddresses")
    , theInvals("sys-bus-Invalidations:All")
    , theInvals_Valid("sys-bus-Invalidations:All:Valid")
    , theInvals_Dirty("sys-bus-Invalidations:All:Dirty")
    , theInvals_Norm("sys-bus-Invalidations:All:Normal")
    , theInvals_Norm_Valid("sys-bus-Invalidations:Normal:Valid")
    , theInvals_Norm_Dirty("sys-bus-Invalidations:Normal:Dirty")
    , theInvals_DMA("sys-bus-Invalidations:DMA")
    , theInvals_DMA_Valid("sys-bus-Invalidations:DMA:Valid")
    , theInvals_DMA_Dirty("sys-bus-Invalidations:DMA:Dirty")
    , theInvals_NAW("sys-bus-Invalidations:NAW")
    , theInvals_NAW_Valid("sys-bus-Invalidations:NAW:Valid")
    , theInvals_NAW_Dirty("sys-bus-Invalidations:NAW:Dirty")
    , theDowngrades("sys-bus-Downgrades:All")
    , theDowngrades_Dirty("sys-bus-Downgrades:All:Dirty")
    , theDowngrades_Norm("sys-bus-Downgrades:Normal")
    , theDowngrades_Norm_Dirty("sys-bus-Downgrades:Normal:Dirty")
    , theDowngrades_DMA("sys-bus-Downgrades:DMA")
    , theDowngrades_DMA_Dirty("sys-bus-Downgrades:DMA:Dirty")
    , theOffChipReads_Fetch("sys-bus-OffChipRead:Fetch")
    , theOffChipReads_Prefetch("sys-bus-OffChipRead:Prefetch")
    , theOffChipReads_Cold("sys-bus-OffChipRead:Cold")
    , theOffChipReads_Replacement("sys-bus-OffChipRead:Replacement")
    , theOffChipReads_Coherence("sys-bus-OffChipRead:Coherence")
    , theOffChipReads_DMA("sys-bus-OffChipRead:DMA")
    , theOffChipReads_NAW("sys-bus-OffChipRead:NAW")

    , theDMAReadPCs( "sys-bus-DMAReadPCs" )
    , theNAWReadPCs( "sys-bus-NAWReadPCs" )

    , theAvgInsPer_OffChipRead("sys-bus-PerIns:Avg:OffChipRead")
    , theStdDevInsPer_OffChipRead("sys-bus-PerIns:StdDev:OffChipRead")
    , theAvgInsPer_OffChipRead_Coherence("sys-bus-PerIns:Avg:OffChipRead:Coh")
    , theStdDevInsPer_OffChipRead_Coherence("sys-bus-PerIns:StdDev:OffChipRead:Coh")

  {}


  inline block_address_t blockAddress(PhysicalMemoryAddress addr) {
    return addr & theBlockMask;
  }

  bool isQuiesced() const {
    return true;
  }

  void saveState(std::string const & aDirName) {
		  unsigned width = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
		  std::ofstream oa[width];
      
      
      for (unsigned i=0; i<width; i++) {
		    std::stringstream fname;
		    fname << aDirName << "/" << std::setw(2) << std::setfill('0') << i << "-directory-tflex";
		  	oa[i].open(fname.str().c_str(), std::ofstream::out);
		  	DBG_Assert(oa[i].good(), (<< "Couldn't open " << fname.str() << " for writing"));
      }
      
		  directory_t::iterator iter = theDirectory.begin();
		  while (iter != theDirectory.end()) {
		  	unsigned node = theMemoryMap->node(0,iter->first);
              DBG_Assert(node >= 0 && node < width, (<< "node outside bounds: " << node));
      
		  	oa[node] << std::hex
		  	         << "[" << iter->first << "] "
		  	         << iter->second.state << " "
		  					 << iter->second.theWasModified << " "
		  					 << iter->second.thePastReaders << std::endl;
      
		  	++iter;
		  }
      
      for (unsigned i=0; i<width; i++) {
		  	oa[i].close();
      }

		// save memory map
		if (cfg.SavePageMap) {
		  theMemoryMap->saveState(aDirName);
    }
  }

  void loadState_TFlex(std::string const & aDirName) {
		unsigned width = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
		std::ifstream ia[width];

    for (unsigned i=0; i<width; i++) {
		  std::stringstream fname;
		  fname << aDirName << "/" << std::setw(2) << std::setfill('0') << i << "-directory-tflex";
			ia[i].open(fname.str().c_str());
			if (!ia[i].good()) {
			  DBG_(Crit, (<< "Couldn't open " << fname.str() << " for reading -- resetting to empty directory"));
				return;
			}
    }

		for (unsigned int i=0; i<width; i++) {
      do {
			  block_address_t addr; dir_entry_t entry; char paren;
				ia[i] >> std::hex
				      >> paren >> addr >> paren
				      >> entry.state
              >> entry.theWasModified
              >> entry.thePastReaders;
				if (!ia[i].eof()) {
          theDirectory.insert(std::make_pair(addr, entry));
				}
			} while (!ia[i].eof());

      ia[i].close();
		}

  }

  //Legacy load function for migratory coherence checkpoints
  void loadState_TFlex_Mig(std::string const & aDirName) {
		unsigned width = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
		std::ifstream ia[width];

    for (unsigned i=0; i<width; i++) {
		  std::stringstream fname;
		  fname << aDirName << "/" << std::setw(2) << std::setfill('0') << i << "-directory-tflex-mig";
			ia[i].open(fname.str().c_str());
			if (!ia[i].good()) {
			  DBG_(Crit, (<< "Couldn't open " << fname.str() << " for reading -- resetting to empty directory"));
				return;
			}
    }
		for (unsigned int i=0; i<width; i++) {
  		std::stringstream fname;
	  	fname << aDirName << "/" << std::setw(2) << std::setfill('0') << i << "-directory-tflex-mig";
      DBG_( Dev, ( << "Loading from " << fname.str() ) );
      unsigned long long last_addr = 1;
      do {
			  unsigned long long addr = 0; dir_entry_t entry; char paren; long ignored;
				ia[i] >> std::hex;
				ia[i] >> paren >> addr >> paren;
				ia[i] >> entry.state;
        ia[i] >> entry.theWasModified;
        ia[i] >> ignored;
        ia[i] >> std::dec >> ignored;
        ia[i] >> std::hex >> entry.thePastReaders;
				if (!ia[i].eof()) {
          theDirectory.insert(std::make_pair(block_address_t(addr), entry));
				}
        DBG_Assert( addr != last_addr, ( << " Broken page_map line at " << fname << " addr=" << std::hex << addr ) );
        last_addr = addr;
      } while (!ia[i].eof());

      ia[i].close();
		}
    DBG_( Dev, ( << "Done Loading." ) );
  }

  void loadState(std::string const & aDirName) {
    // load memory map
		theMemoryMap->loadState(aDirName);

		std::ifstream check;
		std::stringstream fname;
		fname << aDirName << "/" << std::setw(2) << std::setfill('0') << 0 << "-directory-tflex-mig";
		check.open(fname.str().c_str());
		if (check.good()) {
      check.close();
      loadState_TFlex_Mig(aDirName);
    } else {
      //Load old directory state
      loadState_TFlex(aDirName);
    }
  }


  // Initialization
  void initialize() {
    theBlockMask = ~(cfg.BlockSize-1);
    DBG_Assert ( Flexus::Core::ComponentManager::getComponentManager().systemWidth()  <= 16, ( << "This implementation only supports 16 nodes" ) );
    theMemoryMap = new MemoryMap(cfg.PageSize, Flexus::Core::ComponentManager::getComponentManager().systemWidth(), cfg.RoundRobin);

    if (cfg.TrackWrites) {
      theUpgrades_S2X = new Stat::StatCounter * [Flexus::Core::ComponentManager::getComponentManager().systemWidth()];
      theUpgrades_X2X = new Stat::StatCounter * [Flexus::Core::ComponentManager::getComponentManager().systemWidth()];
      for (unsigned int i = 0; i < Flexus::Core::ComponentManager::getComponentManager().systemWidth(); ++i) {
        theUpgrades_S2X[i] = new Stat::StatCounter( boost::padded_string_cast<2,'0'>(i) + "-bus-Upgrades:S2X" );
        theUpgrades_X2X[i] = new Stat::StatCounter( boost::padded_string_cast<2,'0'>(i) + "-bus-Upgrades:X2X" );
      }
    }
    if (cfg.TrackReads) {
      theConsumptions = new Stat::StatCounter * [Flexus::Core::ComponentManager::getComponentManager().systemWidth()];
      for (unsigned int i = 0; i < Flexus::Core::ComponentManager::getComponentManager().systemWidth(); ++i) {
        theConsumptions[i] = new Stat::StatCounter( boost::padded_string_cast<2,'0'>(i) + "-bus-Consumptions" );
      }
    }
    if (cfg.TrackDMA) {
      theDMABlockReads = new Stat::StatCounter * [Flexus::Core::ComponentManager::getComponentManager().systemWidth()];
      theDMABlockWrites = new Stat::StatCounter * [Flexus::Core::ComponentManager::getComponentManager().systemWidth()];
      for (unsigned int i = 0; i < Flexus::Core::ComponentManager::getComponentManager().systemWidth(); ++i) {
        theDMABlockReads[i] = new Stat::StatCounter( boost::padded_string_cast<2,'0'>(i) + "-bus-DMABlockReads" );
        theDMABlockWrites[i] = new Stat::StatCounter( boost::padded_string_cast<2,'0'>(i) + "-bus-DMABlockWrites" );
      }
    }

    if (cfg.TrackProductions) {
      theProductions = new Stat::StatCounter * [Flexus::Core::ComponentManager::getComponentManager().systemWidth()];
      for (unsigned int i = 0; i < Flexus::Core::ComponentManager::getComponentManager().systemWidth(); ++i) {
        theProductions[i] = new Stat::StatCounter( boost::padded_string_cast<2,'0'>(i) + "-bus-Productions" );
      }
    }
    if (cfg.TrackEvictions) {
      theEvictions = new Stat::StatCounter * [Flexus::Core::ComponentManager::getComponentManager().systemWidth()];
      for (unsigned int i = 0; i < Flexus::Core::ComponentManager::getComponentManager().systemWidth(); ++i) {
        theEvictions[i] = new Stat::StatCounter( boost::padded_string_cast<2,'0'>(i) + "-bus-Evictions" );
      }
    }
    if (cfg.TrackFlushes) {
      theFlushes = new Stat::StatCounter * [Flexus::Core::ComponentManager::getComponentManager().systemWidth()];
      for (unsigned int i = 0; i < Flexus::Core::ComponentManager::getComponentManager().systemWidth(); ++i) {
        theFlushes[i] = new Stat::StatCounter( boost::padded_string_cast<2,'0'>(i) + "-bus-Flushes" );
      }
    }
    if (cfg.TrackInvalidations) {
      theInvalidations = new Stat::StatCounter * [Flexus::Core::ComponentManager::getComponentManager().systemWidth()];
      for (unsigned int i = 0; i < Flexus::Core::ComponentManager::getComponentManager().systemWidth(); ++i) {
        theInvalidations[i] = new Stat::StatCounter( boost::padded_string_cast<2,'0'>(i) + "-bus-Invalidations" );
      }
    }

    for (unsigned int i = 0; i < Flexus::Core::ComponentManager::getComponentManager().systemWidth(); ++i) {
      theLastStepCount_OffChipRead.push_back( Simics::Processor::getProcessor(i)->stepCount() );
      theLastStepCount_OffChipRead_Coherence.push_back( Simics::Processor::getProcessor(i)->stepCount() );
    }
  }

  // Ports

  void production(MemoryMessage & aMessage, index_t aNode) { //not precise unless cache is infinite - EvictDirty may mask productions
    if (! cfg.TrackProductions) {
      return;
    }
    ++(*theProductions[aNode]);
    if (aNode > 0) {
      aMessage.coreIdx() = aNode; //Not filled in by the heirarchy
    }
  }

  void upgrade_S2X(MemoryMessage & aMessage, index_t aNode) {
    if (! cfg.TrackWrites) {
      return;
    }
    ++(*theUpgrades_S2X[aNode]);
    if (aNode > 0) {
      aMessage.coreIdx() = aNode; //Not filled in by the heirarchy
    }
    FLEXUS_CHANNEL(Writes) << aMessage;
  }

  void upgrade_X2X(MemoryMessage & aMessage, index_t aNode) {
    if (! cfg.TrackWrites) {
      return;
    }
    ++(*theUpgrades_X2X[aNode]);
    if (aNode > 0) {
      aMessage.coreIdx() = aNode; //Not filled in by the heirarchy
    }
    FLEXUS_CHANNEL(Writes) << aMessage;
  }

  void eviction(MemoryMessage & aMessage, index_t aNode) {
    if (! cfg.TrackEvictions) {
      return;
    }
    ++(*theEvictions[aNode]);
    if (aNode > 0) {
      aMessage.coreIdx() = aNode; //Not filled in by the heirarchy
    }
    FLEXUS_CHANNEL(Evictions) << aMessage;
  }

  void flush(MemoryMessage & aMessage, index_t aNode) {
    if (! cfg.TrackFlushes) {
      return;
    }
    ++(*theFlushes[aNode]);
    if (aNode > 0) {
      aMessage.coreIdx() = aNode;
    }
    FLEXUS_CHANNEL(Flushes) << aMessage;
  }

  void invalidation(MemoryMessage & aMessage, index_t aNode) {
    if (! cfg.TrackInvalidations) {
      return;
    }
    ++(*theInvalidations[aNode]);
    if (aNode > 0) {
      aMessage.coreIdx() = aNode; //Not filled in by the heirarchy
    }
    FLEXUS_CHANNEL(Invalidations) << aMessage;
  }


  void offChipRead(MemoryMessage & aMessage, index_t aNode) {
    if (! cfg.TrackReads) {
      return;
    }

    unsigned long long step_count = Simics::Processor::getProcessor(aNode)->stepCount();
    unsigned long long cycles_since_last = step_count - theLastStepCount_OffChipRead[aNode];
    theLastStepCount_OffChipRead[aNode] = step_count;
    theAvgInsPer_OffChipRead << cycles_since_last;
    theStdDevInsPer_OffChipRead << cycles_since_last;

    aMessage.address() = PhysicalMemoryAddress(aMessage.address() & ~63);
    switch (aMessage.fillType()) {
      case eCold:
        DBG_(Verb, ( << "MISS CPU[" << aMessage.coreIdx() << "] " << aMessage.address() << " Cold" )); 
        ++theOffChipReads_Cold;
        break;
      case eReplacement:
        DBG_(Verb, ( << "MISS CPU[" << aMessage.coreIdx() << "] " << aMessage.address() << " Repl" )); 
        ++theOffChipReads_Replacement;
        break;
      case eCoherence:
        DBG_(Verb, ( << "MISS CPU[" << aMessage.coreIdx() << "] " << aMessage.address() << " Coh" )); 
        cycles_since_last = step_count - theLastStepCount_OffChipRead_Coherence[aNode];
        theLastStepCount_OffChipRead_Coherence[aNode] = step_count;
        theAvgInsPer_OffChipRead_Coherence << cycles_since_last;
        theStdDevInsPer_OffChipRead_Coherence << cycles_since_last;
        ++theOffChipReads_Coherence;
        break;
      case eDMA:
        DBG_(Verb, ( << "MISS CPU[" << aMessage.coreIdx() << "] " << aMessage.address() << " DMA" )); 
        ++theOffChipReads_DMA;
        break;
      case eNAW:
        DBG_(Verb, ( << "MISS CPU[" << aMessage.coreIdx() << "] " << aMessage.address() << " NAW" )); 
        ++theOffChipReads_NAW;
        break;
      case ePrefetch:
        DBG_(Verb, ( << "MISS CPU[" << aMessage.coreIdx() << "] " << aMessage.address() << " Pref" )); 
        ++theOffChipReads_Prefetch;
        break;
      case eFetch:
        DBG_(Verb, ( << "MISS CPU[" << aMessage.coreIdx() << "] " << aMessage.address() << " Fetch" )); 
        ++theOffChipReads_Fetch;
        break;
      default:
        DBG_Assert(false);
    }
    if (aNode > 0) {
      aMessage.coreIdx() = aNode; //Not filled in by the heirarchy
    }
    if (aMessage.fillType() == eFetch) {
      FLEXUS_CHANNEL(Fetches) << aMessage;      
    } else {
      FLEXUS_CHANNEL(Reads) << aMessage;
    }
  }

  void dmaRead(MemoryMessage & aMessage, index_t aNode) {
    if (! cfg.TrackDMA) {
      return;
    }
    ++(*theDMABlockReads[aNode]);
  }
  void dmaWrite(MemoryMessage & aMessage, index_t aNode) {
    if (! cfg.TrackDMA) {
      return;
    }
    ++(*theDMABlockWrites[aNode]);
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(FromCaches);
  void push(interface::FromCaches const &, index_t anIndex, MemoryMessage & message) {
    DBG_(Iface, Addr(message.address()) ( << "request from node " << anIndex << " received: " << message ) );

    //Get the block address
    unsigned long blockAddr = blockAddress(message.address());

    // not used yet, but we should get the page mapping right
    theMemoryMap->node(anIndex, blockAddr);

    message.fillLevel() = eLocalMem;
    message.fillType() = eCold;

    //Lookup or insert the new block in the directory
    directory_t::iterator iter; bool is_new;
    boost::tie(iter, is_new) = theDirectory.insert( std::make_pair( blockAddr, dir_entry_t(kExclusive | anIndex) ) ); //Optimized for exclusive case - avoids shift operation on writes.
    if (is_new) {
      //New block
      switch (message.type()) {
        case MemoryMessage::ReadReq:                  message.fillType() = eCold;       goto label_new_read_cases;
        case MemoryMessage::LoadReq:                  message.fillType() = eCold;       goto label_new_read_cases;
        case MemoryMessage::FetchReq:                 message.fillType() = eFetch;      goto label_new_read_cases;
        case MemoryMessage::PrefetchReadNoAllocReq:   message.fillType() = ePrefetch;   goto label_new_read_cases;
        case MemoryMessage::PrefetchReadAllocReq:     message.fillType() = ePrefetch;   goto label_new_read_cases;
        label_new_read_cases:
          offChipRead(message, anIndex);
          if(message.type() == MemoryMessage::PrefetchReadNoAllocReq) {
            message.type() = MemoryMessage::PrefetchReadReply;
          } else {
            message.type() = MemoryMessage::MissReply;
          }
          message.fillLevel() = eLocalMem;
          iter->second.state = (1UL << anIndex); //Set block state to shared with this node as sharer;
          iter->second.thePastReaders|= (1UL << anIndex);
          DBG_(VVerb, Addr(message.address()) ( << "Read new " << std::hex << blockAddr << " state: " << iter->second.state << std::dec ) );
          break;

        case MemoryMessage::StoreReq:
        case MemoryMessage::StorePrefetchReq:
        case MemoryMessage::RMWReq:
        case MemoryMessage::CmpxReq:
        case MemoryMessage::WriteReq:
        case MemoryMessage::WriteAllocate:
        case MemoryMessage::UpgradeReq:
        case MemoryMessage::UpgradeAllocate:
          message.type() = MemoryMessage::MissReplyWritable;
          message.fillLevel() = eLocalMem;
          //Block state initialized to exclusive with the correct owner in insert above
          iter->second.theWasModified = true;
          DBG_(VVerb, Addr(message.address()) ( << "Write new " << std::hex << blockAddr << " state: " << iter->second.state << std::dec ) );
          upgrade_S2X(message, anIndex);
          break;
        case MemoryMessage::EvictDirty:
        case MemoryMessage::EvictWritable:
        case MemoryMessage::EvictClean:
        case MemoryMessage::FlushReq:
        default:
          DBG_Assert(false, (<< "request from node " << anIndex << " received: " << message));
      }

    } else {
      //Existing block
      unsigned long & state = iter->second.state;
      bool counted = false;
      switch (message.type()) {
        case MemoryMessage::ReadReq:                                                                   goto label_existing_read_cases;
        case MemoryMessage::LoadReq:                                                                   goto label_existing_read_cases;
        case MemoryMessage::FetchReq:                 message.fillType() = eFetch;    counted = true; goto label_existing_read_cases;
        case MemoryMessage::PrefetchReadNoAllocReq:   message.fillType() = ePrefetch; counted = true; goto label_existing_read_cases;
        case MemoryMessage::PrefetchReadAllocReq:     message.fillType() = ePrefetch; counted = true; goto label_existing_read_cases;
        label_existing_read_cases:
          if (state & kExclusive) {
            if ( (state & kOwner) == anIndex) {
              //Read request by current owner -- how did we get here?
              //DBG_Assert(message.type() != MemoryMessage::ReadReq, ( << "Read from owner? " << message << " Addr: " << std::hex << blockAddr << " state: " << iter->second.state << std::dec ) );
              if(message.type() == MemoryMessage::PrefetchReadNoAllocReq) {
                message.type() = MemoryMessage::PrefetchWritableReply;
              } else {
                message.type() = MemoryMessage::MissReplyWritable;
              }
              message.fillLevel() = eLocalMem;
              if (!counted) {
                message.fillType() = eReplacement;
              }
            } else {
              //Read request by new node.
              if (!counted) {
                message.fillType() = eCoherence;
              }
              downgrade(state & kOwner, blockAddr);
              state = (1 << (state & kOwner)) | (1 << (anIndex));
              iter->second.thePastReaders |= (1 << anIndex);
              if(message.type() == MemoryMessage::PrefetchReadNoAllocReq) {
                message.type() = MemoryMessage::PrefetchReadReply;
              } else {
                message.type() = MemoryMessage::MissReply;
              }
              message.fillLevel() = eRemoteMem;
              DBG_(VVerb, Addr(message.address()) ( << "Read downgrade " << std::hex << blockAddr << " state: " << iter->second.state << std::dec ) );
              production(message, anIndex); //not precise
            }
          } else {
            if (state & kDMA) {
              //Don't clear DMA bit on read
              dmaRead(message, anIndex);
              message.fillType() = eDMA;
              counted = true;
            } else if (state & kNAW) {
              //Don't clear NAW bit on read
              message.fillType() = eNAW;
              counted = true;
            }
            message.fillLevel() = eLocalMem;

            //Read in shared state
            if(message.type() == MemoryMessage::PrefetchReadNoAllocReq) {
              message.type() = MemoryMessage::PrefetchReadReply;
            } else {
              message.type() = MemoryMessage::MissReply;
            }
            if (! (state & (1 << anIndex)) ) {
              if (!counted) {
                message.fillType() = eCoherence;
              }
            } else if (!counted) {
              message.fillType() = eReplacement;
            }
            state |= (1 << (anIndex));
            iter->second.thePastReaders |= (1 << (anIndex));
            DBG_(VVerb, Addr(message.address()) ( << "Read shared " << std::hex << blockAddr << " state: " << iter->second.state << std::dec ) );
          }
          offChipRead(message, anIndex);
          // DBG_Assert(message.type() == MemoryMessage::MissReply, (<< "message.type() not set to MissReply: " << message));
          break;
        case MemoryMessage::StoreReq:
        case MemoryMessage::StorePrefetchReq:
        case MemoryMessage::RMWReq:
        case MemoryMessage::CmpxReq:
        case MemoryMessage::WriteReq:
        case MemoryMessage::WriteAllocate:
        case MemoryMessage::UpgradeReq:
        case MemoryMessage::UpgradeAllocate:
          if (state & kExclusive) {
            if ( (state & kOwner) != anIndex) {
              //Write request by new node.
              if(cfg.InvalAll) {
                invalidate(kSharers & (~(1 << anIndex)), blockAddr);
              } else {
                invalidateOne(state & kOwner, blockAddr);
              }
              //Note - implicitly clears DMA/NAW bits
              state = kExclusive | anIndex;
              iter->second.theWasModified = true;
              iter->second.thePastReaders |= (1 << anIndex);
              upgrade_X2X(message, anIndex);
              message.fillLevel() = eRemoteMem;
            }
            else {
              message.fillLevel() = eLocalMem;
            }
            message.type() = MemoryMessage::MissReplyDirty;
          } else {
            if (state & kDMA) {
              state &= (~kDMA); //clear DMA bit
              message.fillType() = eDMA;
              dmaWrite(message, anIndex);
            } else if (state & kNAW) {
              state &= (~kNAW); //clear NAW bit
              message.fillType() = eNAW;
              counted = true;
            }
            message.fillLevel() = eRemoteMem;

            //Write in shared state
            bool anyInvs = false;

            if(cfg.InvalAll) {
              anyInvs = invalidate(kSharers & (~(1 << anIndex)), blockAddr);
            } else {
              anyInvs = invalidate(state & kSharers & (~(1 << anIndex)), blockAddr); //Do not invalidate the writer
            }
            if (anyInvs) message.setInvs();
            message.type() = MemoryMessage::MissReplyDirty; // why was this set to MissReply?
            state = kExclusive | anIndex;
            iter->second.theWasModified = true;
            iter->second.thePastReaders |= (1 << anIndex);
            upgrade_S2X(message, anIndex);
          }
          break;
        case MemoryMessage::EvictDirty:
          //Better be exclusive, better be owned by anIndex
          DBG_Assert (  (state & (kExclusive | kSharers)) == (kExclusive | anIndex), ( << "Block: " << std::hex << blockAddr << " state: " << state << std::dec ));
          state = (1 << anIndex ); //Shared, spurious sharer optimization
					// no break -- continue to next case
        case MemoryMessage::EvictWritable: // invariant: $ hierarchy still has dirty token
        case MemoryMessage::EvictClean:
          eviction(message, anIndex);
          break;
        case MemoryMessage::FlushReq:
          state = (1 << anIndex );
          flush(message, anIndex);
          break;
        default:
          DBG_Assert(false);
      }

    }
    
    DBG_(Iface, Addr(message.address()) ( << "done request, replying: " << message ) );
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(NonAllocateWrite);
  void push(interface::NonAllocateWrite const &, index_t anIndex,  MemoryMessage & message) {
    DBG_(Verb, Addr(message.address()) ( << "Non-allocating Write from node: " << anIndex << " message: " << message ) );

    //Get the block address
    unsigned long blockAddr = blockAddress(message.address());

    ++theNonAllocateWrites;
    theNonAllocateWriteBytes += message.reqSize();
    theNonAllocateWriteAddresses << blockAddr;

    //Lookup or insert the new block in the directory
    directory_t::iterator iter; bool is_new;
    boost::tie(iter, is_new) = theDirectory.insert( std::make_pair( blockAddr, dir_entry_t(kNAW) ) );


    if (! is_new) {
      unsigned long & state = iter->second.state;

      if (state & kExclusive) {
        if ( (state & kOwner) != anIndex) {
          //Non-allocating write by non-owner
          if(cfg.InvalAll) {
            invalidate(kSharers, blockAddr, eSrcNAW);
          } else {
            invalidateOne(state & kOwner, blockAddr, eSrcNAW);
          }
          state = 0;  // don't even set anIndex as a sharer
          iter->second.theWasModified = true;
          iter->second.thePastReaders |= (1 << anIndex);
        } else {
          //No state transaction action if the current owner uses a non-a
        }
        state |= kNAW;
      } else {
        //Write in shared state
        if(cfg.InvalAll) {
          invalidate(kSharers, blockAddr, eSrcNAW);
        } else {
          invalidate(state & kSharers, blockAddr, eSrcNAW);
        }
        state = kNAW;
      }
    }
    
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(DMA);
  void push(interface::DMA const &, MemoryMessage & message) {

    //Get the block address
    unsigned long blockAddr = blockAddress(message.address());

    ++theDMATransfers;
    theDMABytes += message.reqSize();
    theDMAAddresses << blockAddr;

    if (message.type() == MemoryMessage::WriteReq) {
      //Lookup or insert the new block in the directory
      directory_t::iterator iter; bool is_new;
      boost::tie(iter, is_new) = theDirectory.insert( std::make_pair( blockAddr, dir_entry_t(kDMA) ) );
      if (! is_new) {
        unsigned long & state = iter->second.state;

        if (state & kExclusive) {
          if(cfg.InvalAll) {
            invalidate(kSharers, blockAddr, eSrcDMA);
          } else {
            invalidateOne(state & kOwner, blockAddr, eSrcDMA);
          }
          state = kDMA;
        } else {
          //Write in shared state
          if(cfg.InvalAll) {
            invalidate(kSharers, blockAddr, eSrcDMA);
          } else {
            invalidate(state & kSharers, blockAddr, eSrcDMA); //Do not invalidate the writer
          }
          state = kDMA;
        }

      }

    } else {
      DBG_Assert( message.type() == MemoryMessage::ReadReq );

      //Lookup or insert the new block in the directory
      directory_t::iterator iter = theDirectory.find( blockAddr );
      if (iter != theDirectory.end()) {
        unsigned long & state = iter->second.state;

        if (state & kExclusive) {
          downgrade(state, blockAddr, eSrcDMA);
          state = (1 << (state & kSharers) ); //Shared, spurious sharer optimization
        } else {
          //Take no action
        }

      }

    }
  }

private:


  void downgrade(coherence_state_t state, block_address_t address, SnoopSource src = eSrcNormal) {
    theSnoopMessage.type() = MemoryMessage::Downgrade;
    theSnoopMessage.address() = PhysicalMemoryAddress(address);

    FLEXUS_CHANNEL_ARRAY( ToSnoops, state & kOwner) << theSnoopMessage;
    countDowngrade( theSnoopMessage.type(), src);
  }


  bool invalidate(coherence_state_t state, block_address_t address, SnoopSource src = eSrcNormal) {
    bool anyInvs = false;
    theSnoopMessage.address() = PhysicalMemoryAddress(address);
    if (!state) { return anyInvs; }
    int width = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
    coherence_state_t bit = 1;
    for (int i = 0; i < width; ++i) {
      if (state & bit) {
        theSnoopMessage.type() = MemoryMessage::Invalidate;
        invalidation(theSnoopMessage, i);
        FLEXUS_CHANNEL_ARRAY( ToSnoops, i) << theSnoopMessage;
        countInvalidation(theSnoopMessage.type(), src);
        state &= ~bit;
        if (!state) { return anyInvs; }
      }
      bit <<= 1;
    }
    return anyInvs;
  }

  void invalidateOne(index_t aNode, block_address_t address, SnoopSource src = eSrcNormal) {
    theSnoopMessage.type() = MemoryMessage::Invalidate;
    theSnoopMessage.address() = PhysicalMemoryAddress(address);
    invalidation(theSnoopMessage, aNode);
    FLEXUS_CHANNEL_ARRAY( ToSnoops, aNode) << theSnoopMessage;
    countInvalidation(theSnoopMessage.type(), src);
  }

  void countDowngrade( MemoryMessage::MemoryMessageType type, SnoopSource src) {
        ++theDowngrades;
        if (theSnoopMessage.type() == MemoryMessage::DownUpdateAck) {
          ++theDowngrades_Dirty;
          switch (src) {
              case eSrcNormal:
                ++theDowngrades_Norm;
                ++theDowngrades_Norm_Dirty;
                break;
              case eSrcDMA:
                ++theDowngrades_DMA;
                ++theDowngrades_DMA_Dirty;
                break;
              case eSrcNAW:
                DBG_Assert(false);
                break;
          };
        } else {
          switch (src) {
              case eSrcNormal:
                ++theDowngrades_Norm;
                break;
              case eSrcDMA:
                ++theDowngrades_DMA;
                break;
              case eSrcNAW:
                DBG_Assert(false);
                break;
          };
        }
  }

  void countInvalidation( MemoryMessage::MemoryMessageType type, SnoopSource src) {
        ++theInvals;
        if (theSnoopMessage.type() == MemoryMessage::InvalidateAck) {
          ++theInvals_Valid;
          switch (src) {
              case eSrcNormal:
                ++theInvals_Norm;
                ++theInvals_Norm_Valid;
                break;
              case eSrcDMA:
                ++theInvals_DMA;
                ++theInvals_DMA_Valid;
                break;
              case eSrcNAW:
                ++theInvals_NAW;
                ++theInvals_NAW_Valid;
                break;
          };
        } else if (theSnoopMessage.type() == MemoryMessage::InvUpdateAck) {
          ++theInvals_Dirty;
          switch (src) {
              case eSrcNormal:
                ++theInvals_Norm;
                ++theInvals_Norm_Dirty;
                break;
              case eSrcDMA:
                ++theInvals_DMA;
                ++theInvals_DMA_Dirty;
                break;
              case eSrcNAW:
                ++theInvals_NAW;
                ++theInvals_NAW_Dirty;
                break;
          };
        } else {
          switch (src) {
              case eSrcNormal:
                ++theInvals_Norm;
                break;
              case eSrcDMA:
                ++theInvals_DMA;
                break;
              case eSrcNAW:
                ++theInvals_NAW;
                break;
          };
        }
  }

};

}//End namespace nFastBus

FLEXUS_COMPONENT_INSTANTIATOR( FastBus, nFastBus);
FLEXUS_PORT_ARRAY_WIDTH( FastBus, ToSnoops ) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH( FastBus, FromCaches ) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH( FastBus, NonAllocateWrite ) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT FastBus

  #define DBG_Reset
  #include DBG_Control()
