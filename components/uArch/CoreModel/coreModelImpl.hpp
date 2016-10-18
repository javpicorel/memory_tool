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


#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <vector>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/shared_ptr.hpp>
#include <core/metaprogram.hpp>
#include <boost/variant/get.hpp>
#include <boost/optional.hpp>
#include <boost/none.hpp>
#include <boost/iterator/reverse_iterator.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
using namespace boost::multi_index;
#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>
#include <core/performance/profile.hpp>
#include <core/flexus.hpp>

#include <core/stats.hpp>
namespace Stat = Flexus::Stat;

#include "../coreModel.hpp"
#include "../MapTable.hpp"
#include "../RegisterFile.hpp"
#include "../BypassNetwork.hpp"
#include "coreModelTypes.hpp"
#include <components/Common/XactTimeBreakdown.hpp>
#include <components/Common/Slices/PredictorMessage.hpp> /* CMU-ONLY */
#include "bbv.hpp" /* CMU-ONLY */



namespace nuArch {


using nXactTimeBreakdown::TimeBreakdown;

class CoreImpl : public CoreModel {
  //CORE STATE
  //==========================================================================
    //Simulation
      std::string theName;
      unsigned int theNode;
      boost::function< void (Flexus::Simics::Translation &, bool) > translate;
      boost::function<int(bool)> advance_fn;
      boost::function< void(eSquashCause)> squash_fn;
      boost::function< void(VirtualMemoryAddress, VirtualMemoryAddress)> redirect_fn;
      boost::function< void(int, int)> change_mode_fn;
      boost::function< void( boost::intrusive_ptr<BranchFeedback> )> feedback_fn;
      boost::function< void (PredictorMessage::tPredictorMessageType, PhysicalMemoryAddress, boost::intrusive_ptr<TransactionTracker> ) > notifyTMS_fn; /* CMU-ONLY */
      boost::function< void( bool )> signalStoreForwardingHit_fn;

    //Map Tables
      RegisterWindowMap theWindowMap;
      std::vector< boost::shared_ptr<PhysicalMap> > theMapTables;

      RegisterWindowMap theArchitecturalWindowMap;

    //Register Files
      RegisterFile theRegisters;
      int theRoundingMode;
      unsigned long long theTPC[5];      //0
      unsigned long long theTNPC[5];     //1
      unsigned long long theTSTATE[5];   //2
      unsigned int theTT[5];       //3
      //TICK                        //4
      unsigned long long theTBA;         //5
      unsigned long long thePSTATE;      //6
      unsigned int theTL;          //7
      unsigned int thePIL;         //8
      //CWP                        //9
      unsigned int theCANSAVE;     //10
      unsigned int theCANRESTORE;  //11
      unsigned int theCLEANWIN;    //12
      unsigned int theOTHERWIN;    //13
      unsigned int theWSTATE;      //14
      unsigned long long theVER;         //31

      unsigned long long theFPRS;         //6
      unsigned long long theFSR;          //37

      unsigned long long theTICK;         //39
      unsigned long long theSTICK;        //43
      long long theSTICK_interval; //Number of cycles between STICK updates
      long long theSTICK_tillIncrement; //Number of cycles remaining till next STICK increment

      unsigned long long thePC;
      boost::optional<unsigned long long> theNPC;

      unsigned int thePendingTrap;
      bool thePopTLRequested;
      boost::intrusive_ptr<Instruction> theTrapInstruction;

    //Bypass Network
      BypassNetwork theBypassNetwork;

      unsigned long long  theLastGarbageCollect;


    //Semantic Action & Effect Management
      action_list_t theActiveActions;
      action_list_t theRescheduledActions;

      std::list< boost::intrusive_ptr< Interaction > > theDispatchInteractions;
      bool thePreserveInteractions;

    //Resource arbitration
      MemoryPortArbiter theMemoryPortArbiter;

    //Reorder Buffer
      rob_t theROB;
      int theROBSize;

    //Speculative Retirement Buffer
      rob_t theSRB;

    //Retirement
      unsigned int theRetireWidth;

    //Interrupt processing
      bool theInterruptSignalled;
      int thePendingInterrupt;
      boost::intrusive_ptr< Instruction > theInterruptInstruction;

    //Branch Feedback
      std::list< boost::intrusive_ptr<BranchFeedback> > theBranchFeedback;

    //Squash and Redirect control
      bool theSquashRequested;
      eSquashCause theSquashReason;
      rob_t::iterator theSquashInstruction;
      bool theSquashInclusive;

      bool theRedirectRequested;
      VirtualMemoryAddress theRedirectPC;
      VirtualMemoryAddress theRedirectNPC;

    //Load Store Queue and associated memory control
      unsigned long long theMemorySequenceNum;
  public:
      memq_t theMemQueue;
  private:
      long theLSQCount;
      long theSBCount; //Includes SSB
      long theSBNAWCount;
      long theSBSize;
      bool theNAWBypassSB;
      bool theNAWWaitAtSync;
      MSHRs_t theMSHRs;
      eConsistencyModel theConsistencyModel;
      unsigned long long theCoherenceUnit;
      unsigned long thePartialSnoopersOutstanding;

    //Spin detection and control
      unsigned long long theSustainedIPCCycles;
      unsigned long long theKernelPanicCount;
      unsigned long long theInterruptReceived;

    //Spin detection and control
      bool theSpinning;
      std::set<PhysicalMemoryAddress> theSpinMemoryAddresses;
      std::set<VirtualMemoryAddress> theSpinPCs;
      unsigned long theSpinDetectCount;
      unsigned long theSpinRetireSinceReset;
      unsigned long long theSpinStartCycle;
      bool theSpinControlEnabled;

    //Outsanding prefetch tracking
    bool thePrefetchEarly;
    std::map<PhysicalMemoryAddress, std::set<boost::intrusive_ptr<Instruction> >  > theOutstandingStorePrefetches;
    #ifdef VALIDATE_STORE_PREFETCHING
      std::map<PhysicalMemoryAddress, std::set<boost::intrusive_ptr<Instruction> >  > theWaitingStorePrefetches;
      std::map<PhysicalMemoryAddress, std::set<boost::intrusive_ptr<Instruction> >  > theBlockedStorePrefetches;
    #endif //VALIDATE_STORE_PREFETCHING
    std::map<PhysicalMemoryAddress, std::pair<int, bool> > theSBLines_Permission;
    int theSBLines_Permission_falsecount;


    //Speculation control
      bool theSpeculativeOrder;
      bool theSpeculateOnAtomicValue;
      bool theSpeculateOnAtomicValuePerfect;
      bool theValuePredictInhibit;
      bool theIsSpeculating;
      std::map< boost::intrusive_ptr< Instruction >, Checkpoint> theCheckpoints;
      boost::intrusive_ptr< Instruction > theOpenCheckpoint;
      int theRetiresSinceCheckpoint;
      int theAllowedSpeculativeCheckpoints;
      int theCheckpointThreshold;
      bool theAbortSpeculation;
      int theTSOBReplayStalls;
      boost::intrusive_ptr< Instruction > theViolatingInstruction;
      SpeculativeLoadAddressTracker theSLAT;
      Stat::StatCounter theSLATHits_Load;
      Stat::StatCounter theSLATHits_Store;
      Stat::StatCounter theSLATHits_Atomic;
      Stat::StatCounter theSLATHits_AtomicAvoided;
      Stat::StatCounter theValuePredictions;
      Stat::StatCounter theValuePredictions_Successful;
      Stat::StatCounter theValuePredictions_Failed;


      Stat::StatCounter theDG_HitSB;
      Stat::StatCounter theInv_HitSB;
      Stat::StatCounter theRepl_HitSB;

      bool theEarlySGP; /* CMU-ONLY */
      bool theTrackParallelAccesses; /* CMU-ONLY */
  public:
      bool theInOrderMemory;
      bool theInOrderExecute;
  private:
      bool theIdleThisCycle;
      int theIdleCycleCount;
      BBVTracker* theBBVTracker; /* CMU-ONLY */
      unsigned long theOnChipLatency;
      unsigned long theOffChipLatency;
      bool theValidateMMU;

    public:
      std::list< boost::intrusive_ptr< MemOp > > theMemoryPorts;
      unsigned int theNumMemoryPorts;
      std::list< boost::intrusive_ptr< MemOp > > theSnoopPorts;
      unsigned int theNumSnoopPorts;
      std::list< boost::intrusive_ptr< MemOp > > thePurgePorts; /* CMU-ONLY */
      unsigned int theNumPurgePorts; /* CMU-ONLY */
    private:
      std::list< boost::intrusive_ptr< MemOp > > theMemoryReplies;

    //Stats
      unsigned long long theMispredictCycles;
      unsigned long long theMispredictCount;

      unsigned long long theCommitNumber;
      Stat::StatCounter theCommitCount;

      Stat::StatCounter theCommitCount_NonSpin_User;
      Stat::StatCounter theCommitCount_NonSpin_System;
      Stat::StatCounter theCommitCount_Spin_User;
      Stat::StatCounter theCommitCount_Spin_System;
      Stat::StatCounter * theCommitUSArray[4];
      std::vector< Stat::StatCounter * > theCommitsByCode[2];


      Stat::StatInstanceCounter<long long> theLSQOccupancy;
      Stat::StatInstanceCounter<long long> theSBOccupancy;
      Stat::StatInstanceCounter<long long> theSBUniqueOccupancy;
      Stat::StatInstanceCounter<long long> theSBNAWOccupancy;

      Stat::StatCounter theSpinCount;
      Stat::StatCounter theSpinCycles;

      Stat::StatCounter theStorePrefetches;
      Stat::StatCounter theAtomicPrefetches;
      Stat::StatCounter theStorePrefetchConflicts;
      Stat::StatCounter theStorePrefetchDuplicates;

      Stat::StatCounter thePartialSnoopLoads;

      Stat::StatCounter theRaces;
      Stat::StatCounter theRaces_LoadReplayed_User;
      Stat::StatCounter theRaces_LoadReplayed_System;
      Stat::StatCounter theRaces_LoadForwarded_User;
      Stat::StatCounter theRaces_LoadForwarded_System;
      Stat::StatCounter theRaces_LoadAlreadyOrdered_User;
      Stat::StatCounter theRaces_LoadAlreadyOrdered_System;

      Stat::StatCounter theSpeculations_Started;
      Stat::StatCounter theSpeculations_Successful;
      Stat::StatCounter theSpeculations_Rollback;
      Stat::StatCounter theSpeculations_PartialRollback;
      Stat::StatCounter theSpeculations_Resync;

      Stat::StatInstanceCounter<long long> theSpeculativeCheckpoints;
      Stat::StatMax theMaxSpeculativeCheckpoints;

      Stat::StatInstanceCounter<long long> theROBOccupancyTotal;
      Stat::StatInstanceCounter<long long> theROBOccupancySpin;
      Stat::StatInstanceCounter<long long> theROBOccupancyNonSpin;
      Stat::StatCounter theSideEffectOnChip;
      Stat::StatCounter theSideEffectOffChip;

      Stat::StatCounter theNonSpeculativeAtomics;
      Stat::StatCounter theRequiredDiscards;
      Stat::StatLog2Histogram theRequiredDiscards_Histogram;
      Stat::StatCounter theNearestCkptDiscards;
      Stat::StatLog2Histogram theNearestCkptDiscards_Histogram;
      Stat::StatCounter theSavedDiscards;
      Stat::StatLog2Histogram theSavedDiscards_Histogram;

      Stat::StatInstanceCounter<long long> theCheckpointsDiscarded;
      Stat::StatInstanceCounter<long long> theRollbackToCheckpoint;

      Stat::StatCounter theWP_CoherenceMiss;
      Stat::StatCounter theWP_SVBHit;

      Stat::StatCounter
        theResync_GarbageCollect,
        theResync_AbortedSpec,
        theResync_BlackBox,
        theResync_MAGIC,
        theResync_ITLBMiss,
        theResync_UnimplementedAnnul,
        theResync_RDPRUnsupported,
        theResync_WRPRUnsupported,
        theResync_MEMBARSync,
        theResync_POPCUnsupported,
        theResync_UnexpectedException,
        theResync_Interrupt,
        theResync_DeviceAccess,
        theResync_FailedValidation,
        theResync_FailedHandleTrap,
        theResync_UnsupportedLoadASI,
        theResync_UnsupportedAtomicASI,
        theResync_SideEffectLoad,
        theResync_Unknown,

        theFalseITLBMiss;

    MemOpCounter * theMemOpCounters [2][2][8];
    Stat::StatCounter * theEpochEnd[2][8];
    Stat::StatCounter theEpochs;
    Stat::StatLog2Histogram theEpochs_Instructions;
    Stat::StatAverage theEpochs_Instructions_Avg;
    Stat::StatStdDev theEpochs_Instructions_StdDev;
    Stat::StatLog2Histogram theEpochs_Loads;
    Stat::StatAverage theEpochs_Loads_Avg;
    Stat::StatStdDev theEpochs_Loads_StdDev;
    Stat::StatLog2Histogram theEpochs_Stores;
    Stat::StatAverage theEpochs_Stores_Avg;
    Stat::StatStdDev theEpochs_Stores_StdDev;
    Stat::StatLog2Histogram theEpochs_OffChipStores;
    Stat::StatAverage theEpochs_OffChipStores_Avg;
    Stat::StatStdDev theEpochs_OffChipStores_StdDev;
    Stat::StatLog2Histogram theEpochs_StoreAfterOffChipStores;
    Stat::StatAverage theEpochs_StoreAfterOffChipStores_Avg;
    Stat::StatStdDev theEpochs_StoreAfterOffChipStores_StdDev;
    Stat::StatLog2Histogram theEpochs_StoresOutstanding;
    Stat::StatAverage theEpochs_StoresOutstanding_Avg;
    Stat::StatStdDev theEpochs_StoresOutstanding_StdDev;
    Stat::StatInstanceCounter<long long> theEpochs_StoreBlocks;
    Stat::StatInstanceCounter<long long> theEpochs_LoadOrStoreBlocks;
    unsigned long long theEpoch_StartInsn;
    unsigned long long theEpoch_LoadCount;
    unsigned long long theEpoch_StoreCount;
    unsigned long long theEpoch_OffChipStoreCount;
    unsigned long long theEpoch_StoreAfterOffChipStoreCount;
    std::set<unsigned long long> theEpoch_StoreBlocks;
    std::set<unsigned long long> theEpoch_LoadOrStoreBlocks;

    //Accounting Stats
    enum eEmptyROBCause {
      kIStall_PeerL1
      , kIStall_L2
      , kIStall_Mem
      , kIStall_Other
      , kMispredict
      , kSync
      , kResync
      , kFailedSpeculation
      , kRaisedException
      , kRaisedInterrupt
    } theEmptyROBCause;

  //fixme
  /*
    std::ostream & operator << (std::ostream & anOstream, eEmptyROBCause aType) {
      const char * const name[] = { 
        "kIStall_PeerL1"
        , "kIStall_L2"
        , "kIStall_Mem"
        , "kIStall_Other"
        , "kMispredict"
        , "kSync"
        , "kResync"
        , "kFailedSpeculation"
        , "kRaisedException"
        , "kRaisedInterrupt"
      };
      anOstream << name[aType];
      return anOstream;
    }
  */

    unsigned int theLastCycleIStalls;  //Used to fix accounting error when sync instructions are dispatched

    unsigned int theRetireCount;  //Count retirements each cycle

      TimeBreakdown theTimeBreakdown;
      nXactTimeBreakdown::eCycleClass theLastStallCause;
      int theCycleCategory;
      int kTBUser;
      int kTBSystem;

    Stat::StatCounter theMix_Total;
    Stat::StatCounter theMix_Exception;
    Stat::StatCounter theMix_Load;
    Stat::StatCounter theMix_Store;
    Stat::StatCounter theMix_Atomic;
    Stat::StatCounter theMix_Branch;
    Stat::StatCounter theMix_MEMBAR;
    Stat::StatCounter theMix_Computation;
    Stat::StatCounter theMix_Synchronizing;

    Stat::StatCounter theAtomicVal_RMWs;
    Stat::StatCounter theAtomicVal_RMWs_Zero;
    Stat::StatCounter theAtomicVal_RMWs_NonZero;
    Stat::StatCounter theAtomicVal_CASs;
    Stat::StatCounter theAtomicVal_CASs_Mismatch;
    Stat::StatCounter theAtomicVal_CASs_MismatchAfterMismatch;
    Stat::StatCounter theAtomicVal_CASs_Match;
    Stat::StatCounter theAtomicVal_CASs_MatchAfterMismatch;
    Stat::StatCounter theAtomicVal_CASs_Zero;
    Stat::StatCounter theAtomicVal_CASs_NonZero;
    bool theAtomicVal_LastCASMismatch;

    Stat::StatCounter theCoalescedStores;

	unsigned int intAluOpLatency;
	unsigned int intAluOpPipelineResetTime;
	    
	unsigned int intMultOpLatency;
	unsigned int intMultOpPipelineResetTime;
	unsigned int intDivOpLatency;
	unsigned int intDivOpPipelineResetTime;
	    
	unsigned int fpAddOpLatency;
	unsigned int fpAddOpPipelineResetTime;
	unsigned int fpCmpOpLatency;
	unsigned int fpCmpOpPipelineResetTime;
	unsigned int fpCvtOpLatency;
	unsigned int fpCvtOpPipelineResetTime;
	        
	unsigned int fpMultOpLatency;
	unsigned int fpMultOpPipelineResetTime;
	unsigned int fpDivOpLatency;
	unsigned int fpDivOpPipelineResetTime;
	unsigned int fpSqrtOpLatency;
	unsigned int fpSqrtOpPipelineResetTime;

	// Cycles until each FU becomes ready to accept a new operation
	std::vector<unsigned int> intAluCyclesToReady;
	std::vector<unsigned int> intMultCyclesToReady;
	std::vector<unsigned int> fpAluCyclesToReady;
	std::vector<unsigned int> fpMultCyclesToReady;

  //CONSTRUCTION
  //==========================================================================
  public:
    CoreImpl( uArchOptions_t options
            , boost::function< void (Flexus::Simics::Translation &, bool) > xlat
            , boost::function< int(bool) > advance
            , boost::function< void(eSquashCause)> squash
            , boost::function< void(VirtualMemoryAddress, VirtualMemoryAddress) > redirect
            , boost::function< void(int, int) > change_mode
            , boost::function< void( boost::intrusive_ptr<BranchFeedback> ) > feedback
            , boost::function< void ( PredictorMessage::tPredictorMessageType, PhysicalMemoryAddress, boost::intrusive_ptr<TransactionTracker> ) > notifyTMS /* CMU-ONLY */
            , boost::function< void( bool )> signalStoreForwardingHit
            );

    virtual ~CoreImpl() {}

  //Cycle Processing
  //==========================================================================
  public:
    void skipCycle();
    void cycle(int aPendingInterrupt);

  private:
    void prepareCycle();
    void arbitrate();
    void evaluate();
    void endCycle();
    void satisfy( InstructionDependance  const & aDep);
    void squash( InstructionDependance  const & aDep);
    void satisfy( std::list<InstructionDependance > & dependances);
    void squash( std::list<InstructionDependance > & dependances);

  /* CMU-ONLY-BLOCK-BEGIN */
    // Purge implementation for R-NUCA
    //==========================================================================
  public:
    void initializePurge( boost::intrusive_ptr< MemOp > aPurgeOp );
    void ackPurge(boost::intrusive_ptr< MemOp > aPurgeAckOp);
    void finalizePurge( boost::intrusive_ptr< MemOp > aPurgeOp );

  private:
    typedef struct tPurgeInProgress {
      PhysicalMemoryAddress thePurgeAddr;
      unsigned int          thePurgeDelayCycles;
    } tPurgeInProgress;
    tPurgeInProgress thePurgeInProgress;
		
    void decrementPurgeDelayCounter( void );
    bool isPurgePending( void );
  /* CMU-ONLY-BLOCK-END */


  //Instruction completion
  //==========================================================================
  public:
    bool sbEmpty( ) const { return theSBCount == 0; }
    bool sbFull( ) const { 
      if (theSBSize == 0) {
        return false; 
      }
      if (theConsistencyModel == kRMO) {
        return theSBLines_Permission_falsecount >= theSBSize;
      } else {
        return ( theSBCount >= theSBSize || (theSpeculativeOrder ? (static_cast<int> (theSBLines_Permission.size())) >  theSBSize / 8 : false));        
      }
    }
    bool mayRetire_MEMBARStSt( ) const;
    bool mayRetire_MEMBARStLd( ) const;
    bool mayRetire_MEMBARSync( ) const;

    void spinDetect( memq_t::index<by_insn>::type::iterator );
    void retireMem( boost::intrusive_ptr<Instruction> aCorrespondingInstruction);
    void checkTranslation( boost::intrusive_ptr<Instruction> anInsn);
    void commitStore( boost::intrusive_ptr<Instruction> aCorrespondingInstruction);
    bool checkStoreRetirement( boost::intrusive_ptr<Instruction> aStore);
    void accessMem( PhysicalMemoryAddress anAddress, boost::intrusive_ptr<Instruction> anInsn);
    bool isSpinning( ) const { return theSpinning; }
    unsigned int outstandingStorePrefetches() const { return theOutstandingStorePrefetches.size(); }

  private:
    void retire();
    void commit();
    void commit( boost::intrusive_ptr< Instruction > anInstruction );
    bool acceptInterrupt();

  //Retirement accounting
  //==========================================================================
    void accountRetire( boost::intrusive_ptr<Instruction> anInst);
    nXactTimeBreakdown::eCycleClass getStoreStallType( boost::intrusive_ptr<TransactionTracker> tracker );
    void chargeStoreStall( boost::intrusive_ptr<Instruction> inst, boost::intrusive_ptr<TransactionTracker> tracker);
    void accountCommit( boost::intrusive_ptr<Instruction> anInst, bool aRaised);
    void completeAccounting();
    void accountAbortSpeculation(unsigned long long aCheckpointSequenceNumber);
    void accountStartSpeculation();
    void accountEndSpeculation();
    void accountResyncSpeculation();
    void accountResyncReason( boost::intrusive_ptr< Instruction > anInstruction);
    void accountResync();
    void prepareMemOpAccounting();
    void accountCommitMemOp( boost::intrusive_ptr< Instruction > insn);


  //Squashing & Front-end control
  //==========================================================================
  public:
    bool squashAfter( boost::intrusive_ptr< Instruction > anInsn);
    void redirectFetch( VirtualMemoryAddress anAddress );
    void branchFeedback( boost::intrusive_ptr<BranchFeedback> feedback );

    void takeTrap(boost::intrusive_ptr< Instruction > anInsn, int aTrapNum);
    void popTL();
    void handleTrap();
    void handlePopTL();


  private:
    void doSquash();
    void doAbortSpeculation();

  //Dispatch
  //==========================================================================
  public:
    int availableROB() const;
    bool isSynchronized() const { return theROB.empty(); }
    bool isStalled() const;
    int iCount() const;
    bool isQuiesced() const {
      return theROB.empty()
        &&   theBranchFeedback.empty()
        &&   theMemQueue.empty()
        &&   theMSHRs.empty()
        &&   theMemoryPortArbiter.empty()
        &&   theMemoryPorts.empty()
        &&   theSnoopPorts.empty()
        &&   thePurgePorts.empty() /* CMU-ONLY */
        &&   theMemoryReplies.empty()
        &&   theActiveActions.empty()
        &&   theRescheduledActions.empty()
        &&  !theSquashRequested
        &&  !theRedirectRequested
        ;
    }

    bool isFullyStalled() const {
      return  theIdleCycleCount > 5;
    }
    void dispatch( boost::intrusive_ptr< Instruction > anInsn);
    void applyToNext( boost::intrusive_ptr< Instruction > anInsn, boost::intrusive_ptr<Interaction> anInteraction);
    void deferInteraction( boost::intrusive_ptr< Instruction > anInsn, boost::intrusive_ptr<Interaction> anInteraction);

  //Semantic Action & Effect scheduling
  //==========================================================================
  public:
    void create( boost::intrusive_ptr< SemanticAction > anAction);
    void reschedule( boost::intrusive_ptr< SemanticAction > anAction);


  //Bypass Network Interface
  //==========================================================================
    void bypass(mapped_reg aReg, register_value aValue);
    void connectBypass(mapped_reg aReg, boost::intrusive_ptr<Instruction> inst, boost::function<bool(register_value)> fn);

  //Register File Interface
  //==========================================================================
  public:
    void mapRegister( mapped_reg aRegister );
    void unmapRegister( mapped_reg aRegister );
    eResourceStatus requestRegister( mapped_reg aRegister, InstructionDependance  const & aDependance );
    eResourceStatus requestRegister( mapped_reg aRegister );
    void squashRegister( mapped_reg aRegister);
    register_value readRegister( mapped_reg aRegister );
    register_value readArchitecturalRegister( unmapped_reg aRegister, bool aRotate );
    void writeRegister( mapped_reg aRegister, register_value aValue );
    void disconnectRegister( mapped_reg aReg, boost::intrusive_ptr< Instruction > inst);
    void initializeRegister(mapped_reg aRegister, register_value aValue);
    void copyRegValue(mapped_reg aSource, mapped_reg aDest);

  //Mapping Table Interface
  //==========================================================================
  private:
    PhysicalMap & mapTable( eRegisterType aMapTable );
  public:
    mapped_reg map( unmapped_reg aReg );
    std::pair<mapped_reg,mapped_reg> create( unmapped_reg aReg );
    void free( mapped_reg aReg );
    void restore( unmapped_reg aName, mapped_reg aReg );
    unsigned int saveWindow();
    void saveWindowPriv();
    void saved();
    unsigned int restoreWindow();
    void restoreWindowPriv();
    void restored();
    int selectedRegisterSet() const;

  //Synchronization with Simics
  //==========================================================================
  public:
    void reset();
    void resetv9();

    void getv9State( v9State & aState);
    void restorev9State( v9State & aState);
    void comparev9State( v9State & aLeft, v9State & aRight);

    bool isIdleLoop();

    unsigned long long pc() const;
    unsigned long long npc() const;

    void setAG( bool anAG) {
      if (anAG) {
        setPSTATE( getPSTATE() | 1ULL );
      } else {
        setPSTATE( getPSTATE() & ~1ULL );
      }
    }
    void setIG( bool anIG) {
      if (anIG) {
        setPSTATE (getPSTATE() | 0x800ULL);
      } else {
        setPSTATE (getPSTATE() & ~0x800ULL);
      }
    }
    void setMG( bool anMG) {
      if (anMG) {
        setPSTATE (getPSTATE() | 0x400ULL);
      } else {
        setPSTATE (getPSTATE() & ~0x400ULL);
      }
    }
    void setCWP( unsigned int aCWP ) {
      theWindowMap.setCWP( aCWP );
      theArchitecturalWindowMap.setCWP( aCWP );
    }

    unsigned long long getTPC(unsigned int aTL) { if (aTL == 0 || aTL > 5) return 0; return theTPC[aTL - 1]; }
    void setTPC( unsigned long long aTPC, unsigned int aTL) { if (aTL == 0 || aTL > 5) return; theTPC[aTL - 1] = aTPC; }
    unsigned long long getTNPC(unsigned int aTL) { if (aTL == 0 || aTL > 5) return 0; return theTNPC[aTL - 1]; }
    void setTNPC( unsigned long long aTNPC, unsigned int aTL) { if (aTL == 0 || aTL > 5) return; theTNPC[aTL - 1] = aTNPC; }
    unsigned long long getTSTATE(unsigned int aTL) { if (aTL == 0 || aTL > 5) return 0; return theTSTATE[aTL - 1]; }
    void setTSTATE( unsigned long long aTSTATE, unsigned int aTL) { if (aTL == 0 || aTL > 5) return; theTSTATE[aTL - 1] = aTSTATE; }
    unsigned int getTT(unsigned int aTL) { if (aTL == 0 || aTL > 5) return 0; return theTT[aTL - 1]; }
    void setTT( unsigned int aTT, unsigned int aTL) { if (aTL == 0 || aTL > 5) return; theTT[aTL - 1] = aTT; }

    unsigned long long getTBA() { return theTBA; }
    void setTBA( unsigned long long aTBA) { theTBA = aTBA; }
    unsigned long long getPSTATE() { return thePSTATE; }
    void setPSTATE( unsigned long long aPSTATE) {
      //Change AG
      if ((aPSTATE & 0x1ULL) != (thePSTATE & 0x1ULL)) {
        theWindowMap.setAG( (aPSTATE & 0x1ULL) );
        theArchitecturalWindowMap.setAG( (aPSTATE & 0x1ULL)  );
      }
      if ((aPSTATE & 0x800ULL) != (thePSTATE & 0x800ULL)) {
        theWindowMap.setIG( (aPSTATE & 0x800ULL));
        theArchitecturalWindowMap.setIG( (aPSTATE & 0x800ULL) );
      }
      if ((aPSTATE & 0x400ULL) != (thePSTATE & 0x400ULL)) {
        theWindowMap.setMG( (aPSTATE & 0x400ULL));
        theArchitecturalWindowMap.setMG( (aPSTATE & 0x400ULL) );
      }
      //Change priviledge mode
      if (aPSTATE != thePSTATE) {
        thePSTATE = aPSTATE;
        change_mode_fn( getTL(),  thePSTATE);
      }
    }
    unsigned int getTL() { return theTL; }
    void setTL( unsigned int aTL) {
      if (aTL != theTL) {
        theTL = aTL;
        change_mode_fn( theTL,  getPSTATE());
      }
    }
    unsigned int getPIL() { return thePIL; }
    void setPIL( unsigned int aPIL) { thePIL = aPIL; }

    unsigned int getCWP();
    void setCLEANWIN( unsigned int aCLEANWIN ) { theCLEANWIN = aCLEANWIN; }
    unsigned int getCLEANWIN() { return theCLEANWIN; }
    void setCANSAVE( unsigned int aCANSAVE ) { theCANSAVE = aCANSAVE; }
    unsigned int getCANSAVE() { return theCANSAVE; }
    void setCANRESTORE( unsigned int aCANRESTORE) { theCANRESTORE = aCANRESTORE; }
    unsigned int getCANRESTORE() { return theCANRESTORE; }
    void setOTHERWIN( unsigned int aOTHERWIN) { theOTHERWIN = aOTHERWIN; }
    unsigned int getOTHERWIN() { return theOTHERWIN; }
    void setWSTATE( unsigned int aWSTATE) { theWSTATE = aWSTATE; }
    unsigned int getWSTATE() { return theWSTATE; }

    void setVER( unsigned long long aVER) { theVER = aVER; }
    unsigned long long getVER() { return theVER; }

    void setFPRS( unsigned long long anFPRS) { theFPRS = anFPRS; }
    unsigned long long getFPRS() { return theFPRS; }

    void setFSR( unsigned long long anFSR) { theFSR = anFSR; }
    void writeFSR( unsigned long long anFSR);
    unsigned long long getFSR() {
      return theFSR;
    }
    unsigned long long readFSR();


    void setTICK( unsigned long long aTICK ) { theTICK = aTICK; }
    unsigned long long getTICK() { return theTICK; }

    void setSTICK( unsigned long long aSTICK ) { theSTICK = aSTICK; }
    unsigned long long getSTICK() { return theSTICK; }
    void setSTICKInterval( unsigned long long aSTICK_interval ) { theSTICK_interval = aSTICK_interval; theSTICK_tillIncrement = theSTICK_interval; }

    void setRoundingMode( unsigned int aRoundingMode );
    unsigned int getRoundingMode() { return theRoundingMode; }
    void writePR(unsigned int aPR, unsigned long long aVal);
    unsigned long long readPR(unsigned int aPR);
    std::string prName(unsigned int aPR);

    void setASI(unsigned long long);
    unsigned long long getASI();
    void setCCR(unsigned long);
    unsigned long getCCR();
    unsigned long getCCRArchitectural();
    void setRRegister(unsigned int aReg, unsigned long long aVal);
    unsigned long long getRRegister(unsigned int aReg);

    void setPC( unsigned long long aPC) { thePC = aPC; }
    void setNPC( unsigned long long anNPC) { theNPC = anNPC; }

  //Interface to Memory Unit
  //==========================================================================
    void breakMSHRLink( memq_t::index<by_insn>::type::iterator iter );
    eConsistencyModel consistencyModel( ) const { return theConsistencyModel; }
    bool speculativeConsistency( ) const { return theSpeculativeOrder; }
    void insertLSQ( boost::intrusive_ptr< Instruction > anInsn, eOperation anOperation, eSize aSize, bool aBypassSB);
    void insertLSQ( boost::intrusive_ptr< Instruction > anInsn, eOperation anOperation, eSize aSize, bool aBypassSB, InstructionDependance  const & aDependance  );
    void eraseLSQ( boost::intrusive_ptr< Instruction > anInsn );
    void cleanMSHRS( unsigned long long aDiscardAfterSequenceNum );
    void clearLSQ();
    void clearSSB( );
    int clearSSB( unsigned long long aStopAtInsnSeq );
    void pushMemOp(boost::intrusive_ptr< MemOp > anOp);
    bool canPushMemOp();
    boost::intrusive_ptr<MemOp> popMemOp();
    boost::intrusive_ptr<MemOp> popSnoopOp();
    boost::intrusive_ptr<MemOp> popPurgeOp(); /* CMU-ONLY */
  private:
    bool hasSnoopBuffer() const {
      return theSnoopPorts.size() < theNumSnoopPorts;
    }
    /* CMU-ONLY-BLOCK-BEGIN */
    bool hasPurgeBuffer() const {
      return thePurgePorts.size() < theNumPurgePorts;
    }
    /* CMU-ONLY-BLOCK-END */
  public:
    bool hasMemPort() const {
      return theMemoryPorts.size() < theNumMemoryPorts;
    }
    eInstructionClass getROBHeadClass() const {
      eInstructionClass rob_head = clsComputation;
      if (! theROB.empty()) {
        rob_head = theROB.front()->instClass();
      }
      return rob_head;
   }

  //Checkpoint processing
  //==========================================================================
  public:
    void createCheckpoint( boost::intrusive_ptr<Instruction> anInsn);
    void freeCheckpoint( boost::intrusive_ptr<Instruction> anInsn);
    void requireWritePermission( memq_t::index<by_insn>::type::iterator aWrite);
    void unrequireWritePermission( PhysicalMemoryAddress anAddress);
    void acquireWritePermission( PhysicalMemoryAddress anAddress);
    void loseWritePermission( eLoseWritePermission aReason, PhysicalMemoryAddress anAddress);
    bool isSpeculating() const { return theIsSpeculating; }
    void startSpeculating();
    void checkStopSpeculating();
    void resolveCheckpoint();


  //Invalidation processing
  //==========================================================================
  public:
    void invalidate( PhysicalMemoryAddress anAddress);
    void addSLATEntry( PhysicalMemoryAddress anAddress, boost::intrusive_ptr<Instruction> anInsn);
    void removeSLATEntry( PhysicalMemoryAddress anAddress, boost::intrusive_ptr<Instruction> anInsn);

  //Address and Value resolution for Loads and Stores
  //==========================================================================
  public:
    unsigned long long retrieveLoadValue( boost::intrusive_ptr<Instruction> anInsn);
    unsigned long long retrieveExtendedLoadValue( boost::intrusive_ptr<Instruction> anInsn);
    void resolveVAddr( boost::intrusive_ptr< Instruction > anInsn, VirtualMemoryAddress anAddr, int anASI );
    void updateStoreValue( boost::intrusive_ptr< Instruction > anInsn, unsigned long long aValue, boost::optional<unsigned long long> anExtendedValue = boost::none);
    void annulStoreValue( boost::intrusive_ptr< Instruction > anInsn );
    void updateCASValue( boost::intrusive_ptr< Instruction > anInsn, unsigned long long aValue, unsigned long long aCMPValue );

  //Value forwarding
  //==========================================================================
    void forwardValue( MemQueueEntry const & aStore, memq_t::index< by_insn >::type::iterator aLoad);
    template <class Iter>
    boost::optional< memq_t::index< by_insn >::type::iterator > snoopQueue ( Iter iter, Iter end, memq_t::index< by_insn >::type::iterator load);
    boost::optional< memq_t::index< by_insn>::type::iterator > snoopStores( memq_t::index< by_insn >::type::iterator aLoad, boost::optional< memq_t::index< by_insn >::type::iterator > aCachedSnoopState);
    void updateDependantLoads( memq_t::index< by_insn >::type::iterator anUpdatedStore );
    void applyStores( memq_t::index< by_paddr >::type::iterator aFirstStore, memq_t::index< by_paddr >::type::iterator aLastStore, memq_t::index< by_insn >::type::iterator aLoad );
    void applyAllStores( memq_t::index< by_insn >::type::iterator aLoad );

  //Memory request processing
  //===========================================================================
    void issueStore();
    void issuePartialSnoop();
    void issueAtomic();
    void issueAtomicSpecWrite();
    void valuePredictAtomic();
    void issueSpecial();
    void checkExtraLatencyTimeout();
    void doStore( memq_t::index< by_insn >::type::iterator lsq_entry);
    void resnoopDependantLoads( memq_t::index< by_insn >::type::iterator lsq_entry);
    boost::optional< memq_t::index< by_insn >::type::iterator >  doLoad( memq_t::index< by_insn >::type::iterator lsq_entry, boost::optional< memq_t::index< by_insn >::type::iterator > aCachedSnoopState );
    void updateVaddr( memq_t::index< by_insn >::type::iterator  lsq_entry , VirtualMemoryAddress anAddr, int anASI  );



  //MSHR Management
  //===========================================================================
    bool scanAndAttachMSHR( memq_t::index< by_insn >::type::iterator anLSQEntry );
    bool scanAndBlockMSHR( memq_t::index< by_insn >::type::iterator anLSQEntry);
    bool scanAndBlockPrefetch( memq_t::index< by_insn >::type::iterator anLSQEntry );
    bool scanMSHR( memq_t::index< by_insn >::type::iterator anLSQEntry );
    void requestPort(memq_t::index< by_insn >::type::iterator anLSQEntry);
    void requestPort(boost::intrusive_ptr<Instruction> anInstruction );
    void issue(boost::intrusive_ptr<Instruction> anInstruction );
    void issueStorePrefetch( boost::intrusive_ptr<Instruction> anInstruction );
    void requestStorePrefetch( boost::intrusive_ptr<Instruction> anInstruction);
    void requestStorePrefetch( memq_t::index< by_insn >::type::iterator lsq_entry);
    void killStorePrefetches( boost::intrusive_ptr<Instruction> anInstruction);


  //Memory reply Processing
  //==========================================================================
    void processMemoryReplies();
    void ackInvalidate(MemOp const & anInvalidate);
    void ackDowngrade (MemOp const & aDowngrade);
    void ackProbe (MemOp const & aProbe);
    void ackReturn (MemOp const & aReturn);
    bool processReply( MemOp const & anOperation );
    bool satisfies(eOperation aResponse, eOperation anOperation);
    void complete ( MemOp const & anOperation );
    void completeLSQ( memq_t::index<by_insn>::type::iterator lsq_entry, MemOp const & anOperation );

    void startMiss( boost::intrusive_ptr<TransactionTracker> tracker);
    void finishMiss( boost::intrusive_ptr<TransactionTracker> tracker, bool matched_mshr );
    void processTable();


  //Debugging
  //==========================================================================
  public:
    void printROB();
    void printSRB();
    void printMemQueue();
    void printMSHR();
    void printRegMappings(std::string);
    void printRegFreeList(std::string);
    void printRegReverseMappings(std::string);
    void printAssignments(std::string);
  private:
    void dumpROB();
    void dumpSRB();
    void dumpMemQueue();
    void dumpMSHR();
    void dumpActions();
    void dumpCheckpoints();
    void dumpSBPermissions();

};

} //nuArch
