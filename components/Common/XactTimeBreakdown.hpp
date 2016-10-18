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



#ifndef FLEXUS_XACT_TIME_BREAKDOWN_INCLUDED
#define FLEXUS_XACT_TIME_BREAKDOWN_INCLUDED


#include <core/target.hpp>
#include <core/types.hpp>
#include <core/flexus.hpp>
#include <core/stats.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include <algorithm>

namespace nXactTimeBreakdown {
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
    , "SideEffect:Speculating"

    , "MMUAccess"

    , "MEMBAR"

    , "EmptyROB:Unknown"
    , "EmptyROB:IStall"
    , "EmptyROB:Mispredict"
    , "EmptyROB:Sync"
    , "EmptyROB:Resync"
    , "EmptyROB:FailedSpeculation"
    , "EmptyROB:Exception"
    , "EmptyROB:Interrupt"

    , "Branch"

    , "SyncPipe"

    , "FailedSpeculation"
    , "SyncWhileSpeculating"
    , "TSOBReplay"

    };
}

namespace Stat = Flexus::Stat;
using Flexus::Core::theFlexus;

  enum eCycleClass
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
    , kSideEffect_Speculating

    , kMMUAccess

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
    , kEmptyROB_Interrupt


    //Branch
    , kBranch

    //SyncPipe
    , kSyncPipe

    //FailedSpeculation
    , kFailedSpeculation
    , kSyncWhileSpeculating
    , kTSOBReplay

    , kLastStallClass
    };

    enum eDiscardCause {
      ePreserve,
      eDiscard_Resync,
      eDiscard_FailedSpeculation
    };


  struct TimeXact {
    unsigned int theTimeClass;
    enum Type {
      eSpinRetire,
      eBusyRetire,
      eStall
    } theType;
    unsigned int theCount;
    eCycleClass theCycleClass;
    unsigned long long theSequence;
  };

  struct TimeBreakdown {
    std::string theSource;
    unsigned long long theLastAccountedCycle;
    unsigned long long theLastEnqueueSequence;
    int theLastTimeClass;
    Stat::StatCounter theSkippedCycles;


    struct TimeClass {
      std::string theClassName;

      Stat::StatCounter theCycles;
      Stat::StatCounter theCommits;
      Stat::StatCounter theCommits_Busy;
      Stat::StatCounter theCommits_Spin;
      Stat::StatCounter theDiscards_Resync;
      Stat::StatCounter theDiscards_Resync_Busy;
      Stat::StatCounter theDiscards_Resync_Spin;
      Stat::StatCounter theDiscards_FailedSpec;
      Stat::StatCounter theDiscards_FailedSpec_Busy;
      Stat::StatCounter theDiscards_FailedSpec_Spin;
      std::vector<Stat::StatCounter *> theStallAccumulators;

      TimeClass(std::string const & aSource, std::string const & aClass)
       : theClassName(aClass)
       , theCycles(aSource + ":" + aClass + ":AccountedCycles")
       , theCommits(aSource + ":" + aClass + ":Commits")
       , theCommits_Busy(aSource + ":" + aClass + ":Commits:Busy")
       , theCommits_Spin(aSource + ":" + aClass + ":Commits:Spin")
       , theDiscards_Resync(aSource + ":" + aClass + ":Discards:Resync")
       , theDiscards_Resync_Busy(aSource + ":" + aClass + ":Discards:Resync:Busy")
       , theDiscards_Resync_Spin(aSource + ":" + aClass + ":Discards:Resync:Spin")
       , theDiscards_FailedSpec(aSource + ":" + aClass + ":Discards:FailedSpec")
       , theDiscards_FailedSpec_Busy(aSource + ":" + aClass + ":Discards:FailedSpec:Busy")
       , theDiscards_FailedSpec_Spin(aSource + ":" + aClass + ":Discards:FailedSpec:Spin")
       {
          theStallAccumulators.resize(kLastStallClass);
          for(int i = 0; i < kLastStallClass; ++i) {
             theStallAccumulators[i] = new Stat::StatCounter(aSource + ":" + aClass + ":Bkd:" + kStallNames[i]);
          }
       }

      void applyTransaction( TimeXact const & aTransaction ) {
        switch (aTransaction.theType) {
          case TimeXact::eStall:
            theCycles += aTransaction.theCount;
            (*theStallAccumulators[aTransaction.theCycleClass]) += aTransaction.theCount;
            break;
          case TimeXact::eSpinRetire:
            theCommits  += aTransaction.theCount;
            theCommits_Spin += aTransaction.theCount;
            break;
          case TimeXact::eBusyRetire:
            theCommits  += aTransaction.theCount;
            theCommits_Busy += aTransaction.theCount;
            break;
        }
      }

      void applyTransaction( TimeXact const & aTransaction, eCycleClass anAlternateClass, eDiscardCause aDiscardCommits = ePreserve ) {
        switch (aTransaction.theType) {
          case TimeXact::eStall:
            theCycles += aTransaction.theCount;
            (*theStallAccumulators[anAlternateClass]) += aTransaction.theCount;
            break;
          case TimeXact::eSpinRetire:
            switch (aDiscardCommits) {
              case ePreserve:
                theCommits  += aTransaction.theCount;
                theCommits_Spin += aTransaction.theCount;
                break;
              case eDiscard_Resync:
                theDiscards_Resync  += aTransaction.theCount;
                theDiscards_Resync_Spin += aTransaction.theCount;
                break;
              case eDiscard_FailedSpeculation:
                theDiscards_FailedSpec  += aTransaction.theCount;
                theDiscards_FailedSpec_Spin += aTransaction.theCount;
                break;
            }
            break;
          case TimeXact::eBusyRetire:
            switch (aDiscardCommits) {
              case ePreserve:
                theCommits  += aTransaction.theCount;
                theCommits_Busy += aTransaction.theCount;
                break;
              case eDiscard_Resync:
                theDiscards_Resync  += aTransaction.theCount;
                theDiscards_Resync_Busy += aTransaction.theCount;
                break;
              case eDiscard_FailedSpeculation:
                theDiscards_FailedSpec  += aTransaction.theCount;
                theDiscards_FailedSpec_Busy += aTransaction.theCount;
                break;
            }
            break;
        }
      }

    };

    std::vector<TimeClass *> theClasses;
    std::list<TimeXact> theTransactionQueue;

    TimeBreakdown(std::string const & aSource)
     : theSource(aSource)
     , theSkippedCycles( aSource + ":SkippedCycles" )
    {
      theLastAccountedCycle = 0;
      theLastEnqueueSequence = 0;
      theLastTimeClass = 0;
    }

    void skipCycle() {
      //Advance theLastAccountedCycle by one to "skip" a cycle from the next
      //accounting - do not create a transaction for the cycle
      theLastAccountedCycle++;
      theSkippedCycles++;
    }

    unsigned int addClass(std::string const & aClass) {
      theClasses.push_back( new TimeClass(theSource, aClass) );
      return theClasses.size() - 1;
    }

    void enqueueTransaction( TimeXact const & aTransaction ) {
      if (! theTransactionQueue.empty()) {
        DBG_Assert( aTransaction.theSequence >= theTransactionQueue.back().theSequence );
      }
      theTransactionQueue.push_back( aTransaction );
    }

    void applyTransactions( unsigned long long aStopSequenceNumber) {
      while (! theTransactionQueue.empty() && theTransactionQueue.front().theSequence <= aStopSequenceNumber ) {
         theClasses[theTransactionQueue.front().theTimeClass]->applyTransaction(theTransactionQueue.front());
         theTransactionQueue.pop_front();
      }
    }

    void modifyAndApplyTransactions( unsigned long long aStopSequenceNumber, eCycleClass anAlternateClass, eDiscardCause aDiscardCommits = ePreserve ) {
      while (! theTransactionQueue.empty() && theTransactionQueue.front().theSequence <= aStopSequenceNumber ) {
         theClasses[theTransactionQueue.front().theTimeClass]->applyTransaction(theTransactionQueue.front(), anAlternateClass, aDiscardCommits );
         theTransactionQueue.pop_front();
      }
    }
    void modifyAndApplyTransactionsBackwards( unsigned long long aStopSequenceNumber, eCycleClass anAlternateClass, eDiscardCause aDiscardCommits = ePreserve ) {
      while (! theTransactionQueue.empty() && theTransactionQueue.back().theSequence >= aStopSequenceNumber ) {
         theClasses[theTransactionQueue.back().theTimeClass]->applyTransaction(theTransactionQueue.back(), anAlternateClass, aDiscardCommits );
         theTransactionQueue.pop_back();
      }
    }
    void modifyAndApplyAllTransactions( eCycleClass anAlternateClass, eDiscardCause aDiscardCommits = ePreserve ) {
      while (! theTransactionQueue.empty() ) {
         theClasses[theTransactionQueue.front().theTimeClass]->applyTransaction(theTransactionQueue.front(), anAlternateClass, aDiscardCommits );
         theTransactionQueue.pop_front();
      }
    }

    //Enqueue stall cycles
    int stall(eCycleClass aCycleClass) {
      unsigned long long cycles = theFlexus->cycleCount() - theLastAccountedCycle;
      theLastAccountedCycle = theFlexus->cycleCount();
      stall(aCycleClass, cycles);
      return cycles;
    }
    int stall( eCycleClass aCycleClass, unsigned long long aNumCycles) {
      TimeXact xact;
      xact.theType = TimeXact::eStall;
      xact.theTimeClass = theLastTimeClass;
      xact.theCount = aNumCycles;
      xact.theCycleClass = aCycleClass;
      xact.theSequence = theLastEnqueueSequence;
      enqueueTransaction(xact);
      return aNumCycles;
    }

    int retire(eCycleClass aPrecedingStallClass, unsigned long long anInsnSequence, int aTimeClass, bool isSpin) {
      bool count_retire_cycle = false;
      int stall_cycles = 0;
      theLastTimeClass = aTimeClass;
      if (theLastAccountedCycle != theFlexus->cycleCount()) {
        count_retire_cycle = true;
        stall_cycles = theFlexus->cycleCount() - theLastAccountedCycle - 1;
        if (stall_cycles > 0) {
          stall( aPrecedingStallClass, stall_cycles );
        }
      }
      instruction( anInsnSequence, isSpin);
      if (count_retire_cycle) {
        if (isSpin) {
          stall(kSpin_Busy, 1);
        } else {
          stall(kBusy, 1);
        }
      }
      theLastAccountedCycle = theFlexus->cycleCount();
      return stall_cycles;
    }

    void instruction( unsigned long long anInsnSequence, bool isSpin) {
      TimeXact xact;
      if (isSpin) {
        xact.theType = TimeXact::eSpinRetire;
      } else {
        xact.theType = TimeXact::eBusyRetire;
      }
      xact.theTimeClass = theLastTimeClass;
      xact.theCount = 1;
      xact.theCycleClass = kUnknown;
      xact.theSequence = anInsnSequence;
      theLastEnqueueSequence = anInsnSequence;
      enqueueTransaction(xact);
    }

  };

} //End nXactTimeBreakdown

#endif //FLEXUS_XACT_TIME_BREAKDOWN_INCLUDED
