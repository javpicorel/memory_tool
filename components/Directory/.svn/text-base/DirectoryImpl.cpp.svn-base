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

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <components/Directory/Directory.hpp>

#define FLEXUS_BEGIN_COMPONENT Directory
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <set>
#include <deque>
#include <fstream>

#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/random.hpp>

#include <core/stats.hpp>
#include <core/flexus.hpp>

#include <components/Common/Slices/DirectoryMessage.hpp>
#include <components/Common/Slices/DirectoryEntry.hpp>
#include <components/Common/Slices/Mux.hpp>
#include <components/Common/MemoryMap.hpp>


  #define DBG_DefineCategories Directory
  #define DBG_SetDefaultOps AddCat(Directory)
  #include DBG_Control()

namespace nDirectory {


unsigned int log_base2(unsigned int num) {
  unsigned int ii = 0;
  while(num > 1) {
    ii++;
    num >>= 1;
    }
  return ii;
}

using namespace Flexus;
using namespace Core;
using namespace Flexus::SharedTypes;
using boost::intrusive_ptr;
using namespace Flexus::SharedTypes::DirectoryCommand;

class DirectoryMemory {
  typedef intrusive_ptr<DirectoryEntry> DirEntry_p;
  typedef std::map<DirectoryAddress,DirEntry_p> MemMap;
  typedef MemMap::iterator Iterator;
  typedef std::pair<DirectoryAddress,DirEntry_p> MapPair;
  typedef std::pair<Iterator,bool> InsertPair;

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version)
  {
      ar & theMemory;
  }

public:
   DirEntry_p find(DirectoryAddress anAddr) {
    Iterator iter = theMemory.find(anAddr);
    if(iter != theMemory.end()) {
      return iter->second;
    }
    return DirEntry_p(new DirectoryEntry());
  }

  void store(DirectoryAddress anAddr, DirEntry_p anEntry) {
      // try to insert a new entry - if successful, we're done;
      // otherwise replace the old entry
      MapPair insert = std::make_pair(anAddr,anEntry);
      InsertPair result = theMemory.insert(insert);
      if(result.second == false) {
        // was not inserted - indicates the entry already exists
        theMemory[anAddr] = anEntry;
      }
  }

private:
  // maintain the directory entries as a map table, because the common
  // case is that a block does not have a non-default entry
  MemMap theMemory;

}; // class DirectoryMemory


class LockMemory {
  // comparison function for the set
  struct ltAddr {
    bool operator() (const DirectoryAddress & a, const DirectoryAddress & b) const {
      return(a < b);
    }
  };
  typedef std::set<DirectoryAddress, ltAddr> LockSet;
  typedef LockSet::iterator Iter;
  typedef std::pair<Iter,bool> RetType;

  // the actual set
  LockSet theSet;

public:
  bool empty() const {
    return theSet.empty();
  }

  bool isLocked(const DirectoryAddress addr) {
    return( theSet.find(addr) != theSet.end() );
  }
  void lock(const DirectoryAddress addr) {
    RetType ret = theSet.insert(addr);
    // make sure it wasn't already locked!
    DBG_Assert(ret.second);
  }
  void unlock(const DirectoryAddress addr) {
    Iter entry = theSet.find(addr);
    // make sure the lock exists!
    DBG_Assert(isLocked(addr));
    theSet.erase(entry);
  }
};  // class LockMemory


class FLEXUS_COMPONENT(Directory) {
  FLEXUS_COMPONENT_IMPL(Directory);

  Flexus::Stat::StatAverage theAverageReadLatency;
  Flexus::Stat::StatLog2Histogram theReadLatencyHistogram;
  Flexus::Stat::StatAverage theAverageWriteLatency;
  Flexus::Stat::StatLog2Histogram theWriteLatencyHistogram;
  Flexus::Stat::StatInstanceCounter<long long> theQueueNormalInstance;
  Flexus::Stat::StatInstanceCounter<long long> theQueueFastInstance;

  boost::intrusive_ptr<SharedTypes::MemoryMap> theMemoryMap;

  boost::mt19937 * theRNG;
  boost::uniform_int<> * theRNG_distribution;
  boost::variate_generator<boost::mt19937&, boost::uniform_int<> > * randomLatency;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(Directory)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theAverageReadLatency(statName() + "-AvgReadLatency")
    , theReadLatencyHistogram(statName() + "-ReadLatencyHistogram")
    , theAverageWriteLatency(statName() + "-AvgWriteLatency")
    , theWriteLatencyHistogram(statName() + "-WriteLatencyHistogram")
    , theQueueNormalInstance(statName() + "-NormalQueueUsageHistogram")
    , theQueueFastInstance(statName() + "-FastQueueUsageHistogram")
  {
  }

  bool isQuiesced() const {
    bool quiesced =   outQueue.empty()
        &&            lockRequests.empty()
        &&            theLocks.empty()
        ;
    if (quiesced && inQueues) {
      for (int i = 0; i < cfg.NumBanks - 1 && quiesced; ++i) {
        quiesced = inQueues[i].empty();
      }
    }
    return quiesced;
  }

  void saveState(std::string const & aDirName) const {
    std::string fname( aDirName);
    fname += "/" + statName();
    std::ofstream ofs(fname.c_str(), std::ios::binary);
    boost::archive::binary_oarchive oa(ofs);

    oa << theDirectory;
    oa << theLastAddress;
    // close archive
    ofs.close();
  }

  void loadState(std::string const & aDirName) {
    std::string fname( aDirName);
    fname += "/" + statName();
    std::ifstream ifs(fname.c_str(), std::ios::binary);
    if (ifs.good()) {
      boost::archive::binary_iarchive ia(ifs);

      ia >> theDirectory;
      ia >> theLastAddress;
      // close archive
      ifs.close();
    } else {
      // try sys-bus w/ migration
			fname = aDirName + "/" + statName() + "-tflex-mig";
      std::ifstream ifs3(fname.c_str());
      if (ifs3.good()) {
		    DBG_(Dev, Comp(*this) (<< statName() << " couldn't find checkpoint " << statName() << ".  Using " << fname << " instead. " ) );

        // we have to map from a centralized directory to a distrib on
        // read the entire directory and insert addresses this directory owns
        //constants from FastBus:
				unsigned long kExclusive = 0x80000000UL;
				unsigned long kOwner = 0x0000000FUL;
				unsigned long kSharers = 0x0000FFFFUL;

        unsigned long long last_addr = 1;
        do {
				  unsigned long addr;
					unsigned long state, thePastReaders;
					bool theWasModified;
					int theMigratory;
					int theLastWriter;
					char paren;
				  ifs3 >> std::hex
				       >> paren >> addr >> paren
				       >> state
               >> theWasModified
               >> theMigratory;
          ifs3 >> std::dec >> theLastWriter;
          ifs3 >> std::hex >> thePastReaders;
				  if (!ifs3.eof()) {
            DBG_Assert( addr != last_addr, ( << " Broken page_map line at " << fname << " addr=" << std::hex << addr ) );
            last_addr = addr;
            intrusive_ptr<DirectoryEntry> entry(new DirectoryEntry());
						if (state & kExclusive) {
							if ((state & kOwner) == flexusIndex()) {
							  entry->setState(DIR_STATE_INVALID);
							} else {
						    entry->setState(DIR_STATE_MODIFIED);
								entry->setOwner(state & kOwner);
							}
						} else if (state & kSharers) {
						  entry->setState(DIR_STATE_SHARED);
						  entry->setSharers(state & kSharers);
						} else {
						  entry->setState(DIR_STATE_INVALID);
						}
					  if (theWasModified) entry->markModified();
						entry->setPastReaders((unsigned long long)thePastReaders);

					  theDirectory.store((DirectoryAddress)addr, entry);
				  }
				} while (!ifs3.eof());

				ifs3.close();
        return;
      }

		  // try sys-bus (TraceFlex component)
			fname = aDirName + "/" + statName() + "-tflex";
      //std::ifstream ifs2(fname.c_str(), std::ios::binary);
      std::ifstream ifs2(fname.c_str());
			if (ifs2.good()) {
		    DBG_(Dev, Comp(*this) (<< statName() << " couldn't find checkpoint " << statName() << ".  Using " << fname << " instead. " ) );

				//boost::archive::binary_iarchive ia(ifs2);

        // we have to map from a centralized directory to a distrib on
        // read the entire directory and insert addresses this directory owns
        //constants from FastBus:
				unsigned long kExclusive = 0x80000000UL;
				unsigned long kOwner = 0x0000000FUL;
				unsigned long kSharers = 0x0000FFFFUL;

        do {
				  unsigned long addr;
					unsigned long state, thePastReaders;
					bool theWasModified;
					char paren;
				  ifs2 >> std::hex
				       >> paren >> addr >> paren
				       >> state
               >> theWasModified
               >> thePastReaders;
				  if (!ifs2.eof()) {
            intrusive_ptr<DirectoryEntry> entry(new DirectoryEntry());
						if (state & kExclusive) {
							if ((state & kOwner) == flexusIndex()) {
							  entry->setState(DIR_STATE_INVALID);
							} else {
						    entry->setState(DIR_STATE_MODIFIED);
								entry->setOwner(state & kOwner);
							}
						} else if (state & kSharers) {
						  entry->setState(DIR_STATE_SHARED);
						  entry->setSharers(state & kSharers);
						} else {
						  entry->setState(DIR_STATE_INVALID);
						}
					  if (theWasModified) entry->markModified();
						entry->setPastReaders((unsigned long long)thePastReaders);

					  theDirectory.store((DirectoryAddress)addr, entry);
				  }
				} while (!ifs2.eof());

				ifs2.close();
			} else {
        DBG_(Dev, Comp(*this) (<< statName() << " saved checkpoint state " << fname << " not found.  Resetting to empty directory. " )  );
			}
    }
  }


  // Initialization
  void initialize() {
    theMemoryMap = SharedTypes::MemoryMap::getMemoryMap(flexusIndex());
    inQueues = new BankQueueType[cfg.NumBanks];
    theLastAddress.resize(cfg.NumBanks, DirectoryAddress(0));
    myBankMask = cfg.NumBanks - 1;
    myBlockBits = log_base2(cfg.CoheBlockSize);

    if (cfg.RandomLatency !=  0) {
      theRNG = new boost::mt19937(cfg.RandomSeed);
      theRNG_distribution = new boost::uniform_int<>(-1 * cfg.RandomLatency, cfg.RandomLatency);
      randomLatency = new boost::variate_generator<boost::mt19937&, boost::uniform_int<> >(*theRNG, *theRNG_distribution);
    }

    DBG_(Iface, Comp(*this) (<< "NumBanks: " << cfg.NumBanks << " Latency: " << cfg.Latency << " Fast Latency: " << cfg.FastLatency << " Random Latency Range: " << cfg.RandomLatency << " Cohe Block Size: " << cfg.CoheBlockSize) );
  }

  // Ports
    FLEXUS_PORT_ALWAYS_AVAILABLE(DirRequestIn);

    void push(interface::DirRequestIn const &, DirectoryTransport & aDirTransport) {
      intrusive_ptr<DirectoryMessage> msg( aDirTransport[DirectoryMessageTag] );
      DBG_(Iface, Comp(*this) (<< "received " << *msg ) );
			if (msg->addr == 0x8c9b8d00) DBG_(Tmp, Comp(*this) (<< "received " << *msg ) );
      switch(msg->op) {
      case Set:
        // perform the "store entry" here, since the address will
        // likely be unlocked by the time the delay is over... however,
        // the request is still inserted into the delay queue to
        // ensure that timing remains constistent
        theDirectory.store(msg->addr, aDirTransport[DirectoryEntryTag]);
        delayRequest(aDirTransport);
        break;
      case Get:
        // only gets & sets incur latency
        delayRequest(aDirTransport);
        break;
      case Lock:
        // if the address is already locked, queue the request
        if(theLocks.isLocked(msg->addr)) {
          DBG_(VVerb, Comp(*this) ( << "queuing addr: " << &std::hex << msg->addr ) Addr( msg->addr) );
          lockRequests.push_back(aDirTransport);
        }
        else {
          // set the address as locked and inform the requester
          DBG_(VVerb, Comp(*this) ( << "locking addr: " << &std::hex << msg->addr ) Addr( msg->addr) );
          theLocks.lock(msg->addr);
          msg->op = Acquired;
          outQueue.push_back(aDirTransport);
        }
        break;
      case Unlock:
        // unlock the address - no response necessary
        DBG_(VVerb, Comp(*this) ( << "unlocking addr: " << &std::hex << msg->addr ) Addr( msg->addr) );
        theLocks.unlock(msg->addr);
        // now check if a waiting lock request can be satisfied
        wakeLockRequest(msg->addr);
        break;
      case Squash:
        // remove the pending lock request for this address
        if(!removeLockRequest(msg->addr, aDirTransport[DirMux2ArbTag])) {
          DBG_Assert(false,  ( << "no pending lock for squash"));
        }
        break;
      default:
        DBG_Assert( false, ( <<"Invalid request to directory"));
      }
    }

  //Drive Interfaces
  void drive(DirectoryDrive const &) {
      advanceRequests();

      if(!outQueue.empty()) {
        if(FLEXUS_CHANNEL( DirReplyOut ).available()) {
          DBG_(Iface, Comp(*this) (<< "sent " << *(outQueue.front()[DirectoryMessageTag]) ) );
          FLEXUS_CHANNEL( DirReplyOut) << outQueue.front();
          outQueue.erase(outQueue.begin());
        }
      }

    }


private:
  // chooses a bank for this request, and inserts it at the back of the
  // appropriate queue
  void delayRequest(DirectoryTransport trans) {
    int bank = (trans[DirectoryMessageTag]->addr >> myBlockBits) & myBankMask;

    if ( (trans[DirectoryMessageTag]->addr >> myBlockBits) == theLastAddress[bank] ) {
      //Same address as last, fast request
      DBG_(VVerb, Comp(*this) Addr(trans[DirectoryMessageTag]->addr)
               ( << "same as last request, bank= " << bank ) );
      inQueues[bank].push_back( BankQueueEntry() );
      inQueues[bank].back().theTransport = trans;
      if (theFlexus->isFastMode()) {
        inQueues[bank].back().theLatency = 1;
      } else {
        inQueues[bank].back().theLatency = cfg.FastLatency;
      }
      inQueues[bank].back().theInsertTime = theFlexus->cycleCount();
      theQueueFastInstance << std::make_pair(bank, 1);

    } else {
      DBG_(VVerb, Comp(*this) Addr(trans[DirectoryMessageTag]->addr)
               ( << "delaying request, bank= " << bank ) );
      inQueues[bank].push_back( BankQueueEntry() );
      inQueues[bank].back().theTransport = trans;
      if (theFlexus->isFastMode()) {
        inQueues[bank].back().theLatency = 1;
      } else {
        int latency = cfg.Latency;
        if (cfg.RandomLatency != 0) {
          latency += (*randomLatency)();
          DBG_( VVerb, Comp(*this)  ( << "Random mem latency: " << latency ) );
        }
        inQueues[bank].back().theLatency = latency;
      }
      inQueues[bank].back().theInsertTime = theFlexus->cycleCount();
      theLastAddress[bank] = DirectoryAddress(trans[DirectoryMessageTag]->addr >> myBlockBits);
      theQueueNormalInstance << std::make_pair(bank, 1);

    }
  }

  // decrements the current delay time for queued requests, and executes when ready
  void advanceRequests() {
    DBG_(VVerb, Comp(*this) (<< "advanceRequests") );
    int ii;
    for(ii = 0; ii < cfg.NumBanks; ii++) {
      if(!inQueues[ii].empty()) {
        inQueues[ii].front().theLatency--;
        if(inQueues[ii].front().theLatency <= 0) {
          // this one is ready to go
          DirectoryTransport transport = inQueues[ii].front().theTransport;
          long long total_time = theFlexus->cycleCount() - inQueues[ii].front().theInsertTime;
          inQueues[ii].pop_front();
          intrusive_ptr<DirectoryMessage> msg( transport[DirectoryMessageTag] );
          DBG_(VVerb, Comp(*this) Addr(msg->addr) ( << "executing request: " << *msg ) );
          switch(msg->op) {
          case Set:
            // this is just a placeholder for timing correctness/consistency
            // (the actual Set has already been performed) - do nothing
            theAverageWriteLatency << total_time;
            theWriteLatencyHistogram << total_time;
            break;
          case Get:
            msg->op = Found;
            transport.set(DirectoryEntryTag, theDirectory.find(msg->addr));
            outQueue.push_back(transport);
            theAverageReadLatency << total_time;
            theReadLatencyHistogram << total_time;
            break;
          default:
            DBG_Assert( false, ( << "Invalid delayed request in directory"));
          }
        }
      }
    }
  }

  // wakes the oldest waiting request for the given address
  // (assumes the address is not currently locked)
  void wakeLockRequest(DirectoryAddress addr) {
    DBG_(VVerb, Comp(*this) ( << "checking addr to wake: " << &std::hex << addr ) Addr( addr) );
    std::vector<DirectoryTransport>::iterator iter;
    for(iter = lockRequests.begin(); iter != lockRequests.end(); iter++) {
      if((*iter)[DirectoryMessageTag]->addr == addr) {
        // found a match!
        DBG_(VVerb, Comp(*this) ( << "woke addr: " << &std::hex << addr ) Addr( addr) );
        theLocks.lock(addr);
        (*iter)[DirectoryMessageTag]->op = Acquired;
        outQueue.push_back(*iter);
        lockRequests.erase(iter);
        break;  // don't look any further
      }
    }
  }

  // removes the oldest waiting request for the given address
  bool removeLockRequest(DirectoryAddress addr, intrusive_ptr<Mux> source) {
    DBG_(VVerb, Comp(*this) ( << "squashing addr: " << &std::hex << addr ) Addr( addr) );
    std::vector<DirectoryTransport>::iterator iter;
    for(iter = lockRequests.begin(); iter != lockRequests.end(); iter++) {
      if((*iter)[DirectoryMessageTag]->addr == addr) {
        if(source) {
          // if an arbitration slice exists for the squash request, make sure
          // to only remove a pending request from the same source
          if((*iter)[DirMux2ArbTag]->source == source->source) {
            // found a match!
            lockRequests.erase(iter);
            return true;
          }
        }
        else {
          // found a match!
          lockRequests.erase(iter);
          return true;
        }
      }
    }
    return false;
  }


  // The actual directory
  DirectoryMemory theDirectory;

  // The lock set
  LockMemory theLocks;

  // The output queue
  std::vector<DirectoryTransport> outQueue;

  // The queue of waiting lock requests
  std::vector<DirectoryTransport> lockRequests;

  struct BankQueueEntry {
    DirectoryTransport theTransport;
    int theLatency;
    long long theInsertTime;
  };

  // The request queues for each bank
  typedef std::deque< BankQueueEntry > BankQueueType;
  BankQueueType *inQueues;
  std::vector<DirectoryAddress> theLastAddress;

  // Mask for determining the bank
  unsigned int myBankMask;

  // Number of bits corresponding to the size of the coherency unit
  unsigned int myBlockBits;

};

} //namespace nDirectory

FLEXUS_COMPONENT_INSTANTIATOR( Directory, nDirectory );



#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT Directory

  #define DBG_Reset
  #include DBG_Control()
