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



#ifndef FLEXUS_TIME_BREAKDOWN_INCLUDED
#define FLEXUS_TIME_BREAKDOWN_INCLUDED


#include <core/target.hpp>
#include <core/types.hpp>
#include <core/flexus.hpp>
#include <core/stats.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include <algorithm>

namespace nTimeBreakdown {
namespace ll = boost::lambda;

namespace {
  char * kStallNames[] =
    { "Unknown"
    , "Busy"

    , "Dataflow"
    , "DataflowBubble"

    , "Spin:Busy"
    , "Spin:Stall"
    , "Spin:SBNonEmpty"

    //WillRaise
    , "WillRaise:Load"
    , "WillRaise:Store"
    , "WillRaise:Atomic"
    , "WillRaise:Branch"
    , "WillRaise:MEMBAR"
    , "WillRaise:Computation"
    , "WillRaise:Synchronizing"
    , "WillRaise:Other"


    , "Load:Forwarded"
    , "Load:L1"
    , "Load:L2"
    , "Load:L2:Coherence"
    , "Load:PB"
    , "Load:Local:Cold"
    , "Load:Local:Replacement"
    , "Load:Local:DGP"
    , "Load:Local:Coherence"
    , "Load:Remote:Cold"
    , "Load:Remote:Replacement"
    , "Load:Remote:DGP"
    , "Load:Remote:Coherence:Shared"
    , "Load:Remote:Coherence:Modified"
    , "Load:Remote:Coherence:Invalid"
    , "Load:PeerL1:Coherence:Shared"
    , "Load:PeerL1:Coherence:Modified"

    , "Store:Unkown"
    , "Store:Forwarded"
    , "Store:L1"
    , "Store:L2"
    , "Store:L2:Coherence"
    , "Store:PB"
    , "Store:Local:Cold"
    , "Store:Local:Replacement"
    , "Store:Local:DGP"
    , "Store:Local:Coherence"
    , "Store:Remote:Cold"
    , "Store:Remote:Replacement"
    , "Store:Remote:DGP"
    , "Store:Remote:Coherence:Shared"
    , "Store:Remote:Coherence:Modified"
    , "Store:Remote:Coherence:Invalid"
    , "Store:PeerL1:Coherence:Shared"
    , "Store:PeerL1:Coherence:Modified"
    , "Store:SBFull"

    //Atomic
    , "Atomic:Forwarded"
    , "Atomic:L1"
    , "Atomic:L2"
    , "Atomic:L2:Coherence"
    , "Atomic:PB"
    , "Atomic:Local:Cold"
    , "Atomic:Local:Replacement"
    , "Atomic:Local:DGP"
    , "Atomic:Local:Coherence"
    , "Atomic:Remote:Cold"
    , "Atomic:Remote:Replacement"
    , "Atomic:Remote:DGP"
    , "Atomic:Remote:Coherence:Shared"
    , "Atomic:Remote:Coherence:Modified"
    , "Atomic:Remote:Coherence:Invalid"
    , "Atomic:PeerL1:Coherence:Shared"
    , "Atomic:PeerL1:Coherence:Modified"
    , "Atomic:SBDrain"

    , "SideEffect:Atomic"
    , "SideEffect:Load"
    , "SideEffect:Store"
    , "SideEffect:SBDrain"

    , "MEMBAR"

    , "EmptyROB:Unknown"
    , "EmptyROB:IStall"
    , "EmptyROB:Mispredict"
    , "EmptyROB:Sync"
    , "EmptyROB:Resync"
    , "EmptyROB:FailedSpeculation"
    , "EmptyROB:Exception"

    , "Branch"

    , "SyncPipe"

    , "FailedSpeculation"
    , "SyncWhileSpeculating"

    };
}

namespace Stat = Flexus::Stat;
using Flexus::Core::theFlexus;

  enum eCycleClasses
    { kUnknown
    , kBusy

    //Dataflow dependancy preventing commit
    , kDataflow
    , kDataflowBubble

    //Spin
    , kSpin_Busy
    , kSpin_Stall
    , kSpin_SBNonEmpty

    //WillRaise
    , kWillRaise_Load
    , kWillRaise_Store
    , kWillRaise_Atomic
    , kWillRaise_Branch
    , kWillRaise_MEMBAR
    , kWillRaise_Computation
    , kWillRaise_Synchronizing
    , kWillRaise_Other


    //Load
    , kLoad_Forwarded
    , kLoad_L1
    , kLoad_L2
    , kLoad_L2_Coherence
    , kLoad_PB
    , kLoad_Local_Cold
    , kLoad_Local_Replacement
    , kLoad_Local_DGP
    , kLoad_Local_Coherence
    , kLoad_Remote_Cold
    , kLoad_Remote_Replacement
    , kLoad_Remote_DGP
    , kLoad_Remote_Coherence_Shared
    , kLoad_Remote_Coherence_Modified
    , kLoad_Remote_Coherence_Invalid
    , kLoad_PeerL1Cache_Coherence_Shared
    , kLoad_PeerL1Cache_Coherence_Modified

    //Store
    , kStore_Unknown
    , kStore_Forwarded
    , kStore_L1
    , kStore_L2
    , kStore_L2_Coherence
    , kStore_PB
    , kStore_Local_Cold
    , kStore_Local_Replacement
    , kStore_Local_DGP
    , kStore_Local_Coherence
    , kStore_Remote_Cold
    , kStore_Remote_Replacement
    , kStore_Remote_DGP
    , kStore_Remote_Coherence_Shared
    , kStore_Remote_Coherence_Modified
    , kStore_Remote_Coherence_Invalid
    , kStore_PeerL1Cache_Coherence_Shared
    , kStore_PeerL1Cache_Coherence_Modified
    , kStore_BufferFull

    //Atomic
    , kAtomic_Forwarded
    , kAtomic_L1
    , kAtomic_L2
    , kAtomic_L2_Coherence
    , kAtomic_PB
    , kAtomic_Local_Cold
    , kAtomic_Local_Replacement
    , kAtomic_Local_DGP
    , kAtomic_Local_Coherence
    , kAtomic_Remote_Cold
    , kAtomic_Remote_Replacement
    , kAtomic_Remote_DGP
    , kAtomic_Remote_Coherence_Shared
    , kAtomic_Remote_Coherence_Modified
    , kAtomic_Remote_Coherence_Invalid
    , kAtomic_PeerL1Cache_Coherence_Shared
    , kAtomic_PeerL1Cache_Coherence_Modified
    , kAtomic_SBDrain

    //SideEffect Accesses
    , kSideEffect_Atomic
    , kSideEffect_Load
    , kSideEffect_Store
    , kSideEffect_SBDrain

    //MEMBAR
    , kMEMBAR

    //EmptyROB
    , kEmptyROB_Unknown
    , kEmptyROB_IStall
    , kEmptyROB_Mispredict
    , kEmptyROB_Sync
    , kEmptyROB_Resync
    , kEmptyROB_FailedSpeculation
    , kEmptyROB_Exception


    //Branch
    , kBranch

    //SyncPipe
    , kSyncPipe

    //FailedSpeculation
    , kFailedSpeculation
    , kSyncWhileSpeculating

    , kLastStallClass
    };


  struct TimeBreakdown {
    std::string theSource;
    unsigned long long theLastAccountedCycle;
    int theLastClass;

    struct TimeClass {
      std::string theClassName;

      Stat::StatCounter theCycles;
      int pendCycles;
      Stat::StatCounter theCommits;
      int pendCommits;
      Stat::StatCounter theCommits_Busy;
      int pendCommits_Busy;
      Stat::StatCounter theCommits_Spin;
      int pendCommits_Spin;
      std::vector<Stat::StatCounter *> theStallAccumulators;
      std::vector<int> pendStallAccumulators;
      bool theHasPending;

      TimeClass(std::string const & aSource, std::string const & aClass)
       : theClassName(aClass)
       , theCycles(aSource + ":" + aClass + ":AccountedCycles")
       , pendCycles(0)
       , theCommits(aSource + ":" + aClass + ":Commits")
       , pendCommits(0)
       , theCommits_Busy(aSource + ":" + aClass + ":Commits:Busy")
       , pendCommits_Busy(0)
       , theCommits_Spin(aSource + ":" + aClass + ":Commits:Spin")
       , pendCommits_Spin(0)
       , theHasPending(false)
       {
          theStallAccumulators.resize(kLastStallClass);
          pendStallAccumulators.resize(kLastStallClass, 0);
          for(int i = 0; i < kLastStallClass; ++i) {
             theStallAccumulators[i] = new Stat::StatCounter(aSource + ":" + aClass + ":Bkd:" + kStallNames[i]);
          }
       }

       void commitPending() {
         if (theHasPending) {
           //Accumulate pending stall cycles
           std::vector<Stat::StatCounter *>::iterator iter = theStallAccumulators.begin();
           std::vector<Stat::StatCounter *>::iterator end = theStallAccumulators.end();
           std::vector<int>::iterator pend_iter = pendStallAccumulators.begin();
           DBG_Assert( theStallAccumulators.size() == pendStallAccumulators.size());
           DBG_Assert( theStallAccumulators.size() == kLastStallClass);
           while (iter != end) {
             (**iter) += *pend_iter;
             ++iter;
             ++pend_iter;
           }
           theCommits += pendCommits;
           theCommits_Busy += pendCommits_Busy;
           theCommits_Spin += pendCommits_Spin;
           theCycles += pendCycles;
           clearPending();
         }
       }

       void clearPending() {
         std::fill(pendStallAccumulators.begin(), pendStallAccumulators.end(), 0);
         pendCommits = 0;
         pendCommits_Busy = 0;
         pendCommits_Spin = 0;
         pendCycles = 0;
         theHasPending = false;
       }

       void abortPending(eCycleClasses aStallClass) {
         (*theStallAccumulators[aStallClass]) += pendCycles;
         theCycles += pendCycles;
         clearPending();
       }

       void spinSBNonEmpty(int aNumCommits, unsigned long long aNumCycles) {
         theCycles += aNumCycles;
         if (aNumCycles > 0) {
            (*theStallAccumulators[kSpin_SBNonEmpty]) += aNumCycles;
         }
         if (aNumCommits > 0) {
            theCommits += aNumCommits;
            theCommits_Spin += aNumCommits;
         }
       }

       void spinSBNonEmpty_pend(int aNumCommits, unsigned long long aNumCycles) {
         theHasPending = true;
         pendCycles += aNumCycles;
         if (aNumCycles > 0) {
            pendStallAccumulators[kSpin_SBNonEmpty] += aNumCycles;
         }
         if (aNumCommits > 0) {
            pendCommits += aNumCommits;
            pendCommits_Spin += aNumCommits;
         }
       }

       void spin(int aNumCommits, unsigned long long  aBusyCycles, unsigned long long  aStallCycles) {
         theCycles += aBusyCycles + aStallCycles;
         if (aBusyCycles > 0) {
            (*theStallAccumulators[kSpin_Busy]) += aBusyCycles;
         }
         if (aStallCycles > 0) {
            (*theStallAccumulators[kSpin_Stall]) += aStallCycles;
         }
         if (aNumCommits > 0) {
            theCommits += aNumCommits;
            theCommits_Spin += aNumCommits;
         }
       }

       void spin_pend(int aNumCommits, unsigned long long  aBusyCycles, unsigned long long  aStallCycles) {
         theHasPending = true;
         pendCycles += aBusyCycles + aStallCycles;
         if (aBusyCycles > 0) {
            pendStallAccumulators[kSpin_Busy] += aBusyCycles;
         }
         if (aStallCycles > 0) {
            pendStallAccumulators[kSpin_Stall] += aStallCycles;
         }
         if (aNumCommits > 0) {
            pendCommits += aNumCommits;
            pendCommits_Spin += aNumCommits;
         }
       }

       void commit(int aNumCommits, unsigned long long aBusyCycles, unsigned long long aStallCycles, eCycleClasses aStallClass) {
         theCycles += aBusyCycles + aStallCycles;
         if (aBusyCycles > 0) {
            (*theStallAccumulators[kBusy]) += aBusyCycles;
         }
         if (aStallCycles > 0) {
            (*theStallAccumulators[aStallClass]) += aStallCycles;
         }
         if (aNumCommits > 0) {
            theCommits += aNumCommits;
            theCommits_Busy += aNumCommits;
         }
       }

       void commit_pend(int aNumCommits, unsigned long long aBusyCycles, unsigned long long aStallCycles, eCycleClasses aStallClass) {
         theHasPending = true;
         pendCycles += aBusyCycles + aStallCycles;
         if (aBusyCycles > 0) {
            pendStallAccumulators[kBusy] += aBusyCycles;
         }
         if (aStallCycles > 0) {
            pendStallAccumulators[aStallClass] += aStallCycles;
         }
         if (aNumCommits > 0) {
            pendCommits += aNumCommits;
            pendCommits_Busy += aNumCommits;
         }
       }

    };

    std::vector<TimeClass *> theClasses;
    bool isSpeculating;

    TimeBreakdown(std::string const & aSource)
     : theSource(aSource)
     , isSpeculating(false)
    {
      theLastAccountedCycle = 0;
      theLastClass = 0;
    }

    void skipCycle() {
      //Advance theLastAccountedCycle by one to "skip" a cycle from the next
      //accounting
      theLastAccountedCycle++;
    }

    unsigned int addClass(std::string const & aClass) {
      theClasses.push_back( new TimeClass(theSource, aClass) );
      return theClasses.size() - 1;
    }

    void spin(unsigned int aClass, int aNumCommits, bool isSBNonEmpty) {
      DBG_Assert(aClass < theClasses.size());
      if (isSBNonEmpty) {
        unsigned long long spin_sbnonempty_cycles = theFlexus->cycleCount() - theLastAccountedCycle;
        if (isSpeculating) {
          theClasses[aClass]->spinSBNonEmpty_pend(aNumCommits, spin_sbnonempty_cycles);
        } else {
          theClasses[aClass]->spinSBNonEmpty(aNumCommits, spin_sbnonempty_cycles);
        }
      }  else {
        unsigned long long busy_cycles = (aNumCommits > 0 ? 1 : 0);
        unsigned long long stall_cycles = theFlexus->cycleCount() - theLastAccountedCycle - busy_cycles;
        if (theFlexus->cycleCount() == theLastAccountedCycle ) {
          busy_cycles = 0;
          stall_cycles = 0;
        }
        if (isSpeculating) {
          theClasses[aClass]->spin_pend(aNumCommits, busy_cycles, stall_cycles);
        } else {
          theClasses[aClass]->spin(aNumCommits, busy_cycles, stall_cycles);
        }
      }
      theLastAccountedCycle = theFlexus->cycleCount();
      theLastClass = aClass;
    }

    void commit(unsigned int aClass, eCycleClasses aCycleClass, int aNumCommits) {
      DBG_Assert(aClass < theClasses.size());
      unsigned long long busy_cycles = (aNumCommits > 0 ? 1 : 0);
      unsigned long long stall_cycles = theFlexus->cycleCount() - theLastAccountedCycle - busy_cycles;
      if (theFlexus->cycleCount() == theLastAccountedCycle ) {
        busy_cycles = 0;
        stall_cycles = 0;
      }
      theLastAccountedCycle = theFlexus->cycleCount();
      if (isSpeculating) {
        theClasses[aClass]->commit_pend(aNumCommits, busy_cycles, stall_cycles, aCycleClass);
      } else {
        theClasses[aClass]->commit(aNumCommits, busy_cycles, stall_cycles, aCycleClass);
      }
      theLastClass = aClass;
    }

    void stall(eCycleClasses aCycleClass) {
      commit(theLastClass, aCycleClass, 0);
    }
    void stall(unsigned int aClass, eCycleClasses aCycleClass) {
      commit(aClass, aCycleClass, 0);
    }

    void startSpeculation() {
      DBG_Assert( ! isSpeculating );
      isSpeculating = true;
    }
    void advanceSpeculation() {
      DBG_Assert( isSpeculating );
      //Commit speculative counters and reset them to zero
      std::for_each
        ( theClasses.begin()
        , theClasses.end()
        , ll::bind( &TimeClass::commitPending, ll::var(ll::_1) )
        );
    }
    void endSpeculation() {
      advanceSpeculation();
      isSpeculating = false;
    }
    void abortSpeculation(eCycleClasses aCycleClass) {
      DBG_Assert( isSpeculating );
      //Replace speculative counters with the specified "failed speculation" category
      //Reset speculative counters
      std::for_each
        ( theClasses.begin()
        , theClasses.end()
        , ll::bind( &TimeClass::abortPending, ll::var(ll::_1), aCycleClass )
        );
      isSpeculating = false;
    }



  };

} //End nTimeBreakdown

#endif //FLEXUS_TIME_BREAKDOWN_INCLUDED
