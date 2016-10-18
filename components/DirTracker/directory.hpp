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
static const int kProducer = 0;
namespace nDirTracker {

typedef int BlockStateEnum;
static const int StateShared = 0;
static const int StateExclusive = 1;
static const int StateDowngraded = 2;

struct BlockState : boost::counted_base {
  BlockState(int node)
    : state(StateShared)
    , producer(-1)
    , sharers(1<<node)
  {}
  BlockState(int node, int ignored)
    : state(StateExclusive)
    , producer(node)
    , sharers(0)
  {}

  bool isSharer(int node) {
    return( sharers & (1<<node) );
  }
  void setSharer(int node) {
    sharers |= (1<<node);
  }
  void newShareList(int node, int oldOwner) {
    sharers = (1<<node);
    sharers |= (1<<oldOwner);
  }

  BlockStateEnum state;
  int producer;
  unsigned short sharers;
} __attribute__((packed));

typedef boost::intrusive_ptr<BlockState> BlockState_p;

class MemoryStateTable {
  typedef std::map<tAddress,BlockState_p> StateMap;
  typedef StateMap::iterator Iterator;
  typedef std::pair<tAddress,BlockState_p> BlockPair;
  typedef std::pair<Iterator,bool> InsertPair;

public:
  MemoryStateTable()
  {}

  BlockState_p find(tAddress anAddr) {
    Iterator iter = theBlockStates.find(anAddr);
    if(iter != theBlockStates.end()) {
      return iter->second;
    }
    return BlockState_p(0);
  }

  void remove(tAddress anAddr) {
    Iterator iter = theBlockStates.find(anAddr);
    if(iter != theBlockStates.end()) {
      theBlockStates.erase(iter);
    }
  }

  void addEntry(tAddress anAddr, BlockState_p anEntry) {
    // try to insert a new entry - if successful, we're done;
    // otherwise abort
    BlockPair insert = std::make_pair(anAddr, anEntry);
    InsertPair result = theBlockStates.insert(insert);
    DBG_Assert(result.second);
  }

private:
  StateMap theBlockStates;

};  // end class MemoryStateTable

class Directory {
   // state for all active blocks
   MemoryStateTable myStateTable;
   DirStats & theStats;
   bool theSkipOS;

  public:
    Directory(DirStats & aStats, bool useOS)
      : theStats( aStats )
      , theSkipOS( !useOS )
    {}

    void downgrade( tID aProducer, tAddress anAddress, bool isOS) {
      if(theSkipOS && isOS) {
        return;
      }
      DBG_(Iface, ( << "P[" << aProducer << "] Downgrade @" << & std::hex << anAddress << & std::dec ) );

      BlockState_p block = myStateTable.find(anAddress);
      if(block) {
        theStats.dirFlush(aProducer, anAddress);
        //It is possible to get a downgrade in Shared state because of a race between a Flush and a Fwd'd Read
        if (block->state == StateExclusive) {
          if (block->producer == aProducer) {
            //Standard downgrade case
            block->state = StateDowngraded;

          } else {
            //Write race.  State remains exclusive.  Do not write MRP or update the
            //current producer, because these must have already been done.
          }
        }
        else if (block->state == StateShared) {
          //State remains shared - we already know this was a good DGP prediction
          //the MRP was already informed of the write in the consume call
        }
        else {  // StateDowngraded
          if (block->producer != aProducer) {
            DBG_(Dev, ( << "downgrade on state " << block->state << " by node " << aProducer
                        << " @" << & std::hex << anAddress << " (prev: owner=" << block->producer
                        << " sharers=0x" << block->sharers << & std::dec ) );
          }
        }
      }
    }

    void storeMiss( tID aProducer, tAddress anAddress, bool isOS) {
      if(theSkipOS && isOS) {
        return;
      }
      DBG_(Iface, ( << "P[" << aProducer << "] Store Miss @" << & std::hex << anAddress << & std::dec ) );

      // grab the current state of this block
      BlockState_p block = myStateTable.find(anAddress);
      if(block) {
        DBG_(VVerb, ( << "retrieved block state" ) );

        if(block->state == StateShared) {
          DBG_(VVerb, ( << "state shared" ) );

          // transition the block to exclusive
          block->state = StateExclusive;
          block->producer = aProducer;
          theStats.dirUpgrade(aProducer, anAddress);
        }
        else if (block->state == StateExclusive) {
          DBG_(VVerb, ( << "state exclusive" ) );

          if(aProducer != block->producer) {
            DBG_(VVerb, ( << "new producer" ) );

            theStats.dirProduction(block->producer, anAddress);  // count the old producer
            theStats.dirUpgrade(aProducer, anAddress);

            // transition to new producer
            block->producer = aProducer;
          }
        }
        else { //StateDowngraded
          if (block->producer == aProducer) {
            //The last owner screwed up his prediction.  Lets inform the
            //SORDManager so it can fix the SORD.
          }
          else {
            // now we know the downgrade was a real production
            theStats.dirProduction(block->producer, anAddress);
            block->producer = aProducer;
            theStats.dirUpgrade(aProducer, anAddress);
          }
          block->state = StateExclusive;
        }
      }
      else {
        DBG_(VVerb, ( << "no existing block state" ) );

        // no current state for this block
        block = new BlockState(aProducer,kProducer);
        myStateTable.addEntry(anAddress, block);
        theStats.dirUpgrade(aProducer, anAddress);
      }
    }

    void consume( tID aConsumer, tAddress anAddress, bool wasPredicted, bool isOS) {
      if(theSkipOS && isOS) {
        return;
      }
      DBG_(Iface, ( << "C[" << aConsumer << "] Dir.Consume @" << & std::hex << anAddress << & std::dec ) );

      BlockState_p block = myStateTable.find(anAddress);
      if(block) {
        DBG_(VVerb, ( << "retrieved block state" ) );

        if(block->state == StateShared) {
          DBG_(VVerb, ( << "state shared" ) );

          if (! block->isSharer(aConsumer) ) {
            block->setSharer(aConsumer);

            if (! wasPredicted) {
              theStats.dirConsumption(aConsumer, anAddress);
            }

          }

        }
        else if (block->state == StateExclusive)  {

          // check if the reader is already the node with exclusive access
          if(block->producer == aConsumer) {
            DBG_(VVerb, ( << "state exclusive - read by owner: @" << & std::hex << anAddress << & std::dec ) );

          }
          else {
            DBG_(VVerb, ( << "state exclusive - read by another" ) );

            // the MRP gets a downgrade
            theStats.dirProduction(block->producer, anAddress);

            // transition to shared and record this first read time
            block->state = StateShared;
            block->newShareList(aConsumer, block->producer);

            if (! wasPredicted) {
              theStats.dirConsumption(aConsumer, anAddress);
            }
          }
        } else { //StateDowngraded
          if (block->producer != aConsumer) {
            //Inform the last producer that he was right
            block->state = StateShared;
            block->newShareList(aConsumer, block->producer);
            theStats.dirProduction(block->producer, anAddress);

            if (! wasPredicted) {
              theStats.dirConsumption(aConsumer, anAddress);
            }
          }
        }
      }
      else {
        DBG_(VVerb, ( << "no existing block state" ) );

        // no current state for this block
        block = new BlockState(aConsumer);
        myStateTable.addEntry(anAddress, block);

        // record this first read time
        if (! wasPredicted) {
          theStats.dirConsumption(aConsumer, anAddress);
        }
      }
    }

    bool predictedHit( tID aConsumer, tAddress anAddress, bool isOS) {
      DBG_(Iface, ( << "C[" << aConsumer << "] Dir.PredHit @" << & std::hex << anAddress << & std::dec ) );
      bool ok = false;

      BlockState_p block = myStateTable.find(anAddress);
      if(block) {
        DBG_(VVerb, ( << "retrieved block state" ) );

        if(block->state == StateShared) {
          DBG_(VVerb, ( << "state shared" ) );

          if (block->isSharer(aConsumer)) {
            ok = true;
          }
        }
      }

      // this is a real consumption, regardless of what the directory state is
      theStats.dirConsumption(aConsumer, anAddress);

      return ok;
    }

};

}  // namespace nDirTracker
