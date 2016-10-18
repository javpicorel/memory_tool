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


#include <components/uArch/uArch.hpp>

#define FLEXUS_BEGIN_COMPONENT uArch
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/Slices/ExecuteState.hpp>
#include <components/MTManager/MTManager.hpp>

#include "uArchInterfaces.hpp"

#include <core/debug/debug.hpp>
#include "microArch.hpp"

#include <core/simics/mai_api.hpp>

#define DBG_DefineCategories uArchCat, Special
#define DBG_SetDefaultOps AddCat(uArchCat)
#include DBG_Control()

namespace nuArch {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

class uArch_SimicsObject_Impl  {
    boost::shared_ptr<microArch> theMicroArch;
  public:
    uArch_SimicsObject_Impl(Flexus::Simics::API::conf_object_t * /*ignored*/ ) {}

    void setMicroArch(boost::shared_ptr<microArch> aMicroArch) {
      theMicroArch = aMicroArch;
    }

    void testCkptRestore() {
      DBG_Assert(theMicroArch);
      theMicroArch->testCkptRestore();
    }
    void printROB() {
      DBG_Assert(theMicroArch);
      theMicroArch->printROB();
    }
    void printMemQueue() {
      DBG_Assert(theMicroArch);
      theMicroArch->printMemQueue();
    }
    void printSRB() {
      DBG_Assert(theMicroArch);
      theMicroArch->printSRB();
    }
    void printMSHR() {
      DBG_Assert(theMicroArch);
      theMicroArch->printMSHR();
    }
    void pregs() {
      DBG_Assert(theMicroArch);
      theMicroArch->pregs();
    }
    void pregsAll() {
      DBG_Assert(theMicroArch);
      theMicroArch->pregsAll();
    }
    void resynchronize() {
      DBG_Assert(theMicroArch);
      theMicroArch->resynchronize();
    }
    void printRegMappings(std::string aRegSet) {
      DBG_Assert(theMicroArch);
      theMicroArch->printRegMappings(aRegSet);
    }
    void printRegFreeList(std::string aRegSet) {
      DBG_Assert(theMicroArch);
      theMicroArch->printRegFreeList(aRegSet);
    }
    void printRegReverseMappings(std::string aRegSet) {
      DBG_Assert(theMicroArch);
      theMicroArch->printRegReverseMappings(aRegSet);
    }
    void printRegAssignments(std::string aRegSet) {
      DBG_Assert(theMicroArch);
      theMicroArch->printAssignments(aRegSet);
    }

};

class uArch_SimicsObject : public Simics::AddInObject <uArch_SimicsObject_Impl> {
    typedef Simics::AddInObject<uArch_SimicsObject_Impl> base;
   public:
    static const Simics::Persistence  class_persistence = Simics::Session;
    //These constants are defined in Simics/simics.cpp
    static std::string className() { return "uArch"; }
    static std::string classDescription() { return "uArch object"; }

    uArch_SimicsObject() : base() { }
    uArch_SimicsObject(Simics::API::conf_object_t * aSimicsObject) : base(aSimicsObject) {}
    uArch_SimicsObject(uArch_SimicsObject_Impl * anImpl) : base(anImpl) {}

    template <class Class>
    static void defineClass(Class & aClass) {

      aClass.addCommand
        ( & uArch_SimicsObject_Impl::printROB
        , "print-rob"
        , "Print out ROB contents"
        );

      aClass.addCommand
        ( & uArch_SimicsObject_Impl::printSRB
        , "print-srb"
        , "Print out SRB contents"
        );

      aClass.addCommand
        ( & uArch_SimicsObject_Impl::printMemQueue
        , "print-memqueue"
        , "Print out MemQueue contents"
        );

      aClass.addCommand
        ( & uArch_SimicsObject_Impl::printMSHR
        , "print-mshr"
        , "Print out MSHR contents"
        );

      aClass.addCommand
        ( & uArch_SimicsObject_Impl::printRegMappings
        , "print-reg-mappings"
        , "Print out current register map table"
        , "regset"
        );

      aClass.addCommand
        ( & uArch_SimicsObject_Impl::printRegFreeList
        , "print-reg-freelist"
        , "Print out current register free list"
        , "regset"
        );

      aClass.addCommand
        ( & uArch_SimicsObject_Impl::printRegReverseMappings
        , "print-reg-reversemappings"
        , "Print out reverse register mappings"
        , "regset"
        );

      aClass.addCommand

        ( & uArch_SimicsObject_Impl::printRegAssignments
        , "print-reg-assignment"
        , "Print out ordered list of physical registers for each name"
        , "regset"
        );

      aClass.addCommand
        ( & uArch_SimicsObject_Impl::resynchronize
        , "resynchronize"
        , "Resynchronize with Simics"
        );

      aClass.addCommand
        ( & uArch_SimicsObject_Impl::pregs
        , "pregs"
        , "Print architectural register contents"
        );

      aClass.addCommand
        ( & uArch_SimicsObject_Impl::pregsAll
        , "pregs-all"
        , "Print architectural register contents"
        );

      aClass.addCommand
        ( & uArch_SimicsObject_Impl::testCkptRestore
        , "test-ckpt-restore"
        , "Test checkpoint/restore implementation"
        );

    }

};

Simics::Factory<uArch_SimicsObject> theuArchSimicsFactory;


class FLEXUS_COMPONENT(uArch) {
    FLEXUS_COMPONENT_IMPL(uArch);

  boost::shared_ptr<microArch> theMicroArch;
  uArch_SimicsObject theuArchObject;

  public:
   FLEXUS_COMPONENT_CONSTRUCTOR(uArch)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    {}

    bool isQuiesced() const {
      return !theMicroArch || theMicroArch->isQuiesced();
    }


    void initialize() {
      uArchOptions_t
        options;

      options.ROBSize              = cfg.ROBSize;
      options.SBSize               = cfg.SBSize;
      options.NAWBypassSB          = cfg.NAWBypassSB;
      options.NAWWaitAtSync        = cfg.NAWWaitAtSync;
      options.retireWidth          = cfg.RetireWidth;
      options.numMemoryPorts       = cfg.MemoryPorts;
      options.numSnoopPorts        = cfg.SnoopPorts;
      options.numPurgePorts        = cfg.PurgePorts; /* CMU-ONLY */
      options.numStorePrefetches   = cfg.StorePrefetches;
      options.prefetchEarly        = cfg.PrefetchEarly;
      options.spinControlEnabled   = cfg.SpinControl;
      options.consistencyModel     = (nuArch::eConsistencyModel)cfg.ConsistencyModel;
      options.coherenceUnit        = cfg.CoherenceUnit;
      options.breakOnResynchronize = cfg.BreakOnResynchronize;
   	  options.validateMMU          = cfg.ValidateMMU;
      options.speculativeOrder     = cfg.SpeculativeOrder;
      options.speculateOnAtomicValue   = cfg.SpeculateOnAtomicValue;
      options.speculateOnAtomicValuePerfect   = cfg.SpeculateOnAtomicValuePerfect;
      options.speculativeCheckpoints=cfg.SpeculativeCheckpoints;
      options.checkpointThreshold   =cfg.CheckpointThreshold;
      options.earlySGP             = cfg.EarlySGP;    /* CMU-ONLY */
      options.trackParallelAccesses= cfg.TrackParallelAccesses; /* CMU-ONLY */
      options.inOrderMemory        = cfg.InOrderMemory;
      options.inOrderExecute       = cfg.InOrderExecute;
      options.onChipLatency        = cfg.OnChipLatency;
      options.offChipLatency       = cfg.OffChipLatency;
      options.name                 = statName();
      options.node                 = flexusIndex();

      options.numIntAlu = cfg.NumIntAlu;
      options.intAluOpLatency = cfg.IntAluOpLatency;
      options.intAluOpPipelineResetTime = cfg.IntAluOpPipelineResetTime;
	    
      options.numIntMult = cfg.NumIntMult;
      options.intMultOpLatency = cfg.IntMultOpLatency;
      options.intMultOpPipelineResetTime = cfg.IntMultOpPipelineResetTime;
      options.intDivOpLatency = cfg.IntDivOpLatency;
      options.intDivOpPipelineResetTime = cfg.IntDivOpPipelineResetTime;
	    
      options.numFpAlu = cfg.NumFpAlu;
      options.fpAddOpLatency = cfg.FpAddOpLatency;
      options.fpAddOpPipelineResetTime = cfg.FpAddOpPipelineResetTime;
      options.fpCmpOpLatency = cfg.FpCmpOpLatency;
      options.fpCmpOpPipelineResetTime = cfg.FpCmpOpPipelineResetTime;
      options.fpCvtOpLatency = cfg.FpCvtOpLatency;
      options.fpCvtOpPipelineResetTime = cfg.FpCvtOpPipelineResetTime;
			        
      options.numFpMult = cfg.NumFpMult;
      options.fpMultOpLatency = cfg.FpMultOpLatency;
      options.fpMultOpPipelineResetTime = cfg.FpMultOpPipelineResetTime;
      options.fpDivOpLatency = cfg.FpDivOpLatency;
      options.fpDivOpPipelineResetTime = cfg.FpDivOpPipelineResetTime;
      options.fpSqrtOpLatency = cfg.FpSqrtOpLatency;
      options.fpSqrtOpPipelineResetTime = cfg.FpSqrtOpPipelineResetTime;

      theMicroArch = microArch::construct ( options
                                          , ll::bind( &uArchComponent::squash, this, ll::_1)
                                          , ll::bind( &uArchComponent::redirect, this, ll::_1, ll::_2)
                                          , ll::bind( &uArchComponent::changeState, this, ll::_1, ll::_2)
                                          , ll::bind( &uArchComponent::feedback, this, ll::_1)
                                          , ll::bind( &uArchComponent::notifyTMS, this, ll::_1, ll::_2, ll::_3) /* CMU-ONLY */
                                          , ll::bind( &uArchComponent::signalStoreForwardingHit, this, ll::_1)
                                          );

      theuArchObject = theuArchSimicsFactory.create( (std::string("uarch-") + boost::padded_string_cast<2,'0'>(flexusIndex())).c_str() );
      theuArchObject->setMicroArch(theMicroArch);

    }

  public:
    FLEXUS_PORT_ALWAYS_AVAILABLE(DispatchIn);
    void push( interface::DispatchIn const &, boost::intrusive_ptr< AbstractInstruction > & anInstruction ) {
        theMicroArch->dispatch(anInstruction);
    }

    FLEXUS_PORT_ALWAYS_AVAILABLE(AvailableDispatchOut);
    std::pair<int, bool> pull(AvailableDispatchOut const &) {
      return std::make_pair( theMicroArch->availableROB(), theMicroArch->isSynchronized() );
    }

    FLEXUS_PORT_ALWAYS_AVAILABLE(Stalled);
    bool pull(Stalled const &) {
      return theMicroArch->isStalled();
    }

    FLEXUS_PORT_ALWAYS_AVAILABLE(ICount);
    int pull(ICount const &) {
      return theMicroArch->iCount();
    }

    FLEXUS_PORT_ALWAYS_AVAILABLE(MemoryIn_Reply);
    void push( interface::MemoryIn_Reply const &, MemoryTransport & aTransport) {
        handleMemoryMessage(aTransport);
    }
    FLEXUS_PORT_ALWAYS_AVAILABLE(MemoryIn_Request);
    void push( interface::MemoryIn_Request const &, MemoryTransport & aTransport) {
        handleMemoryMessage(aTransport);
    }

    FLEXUS_PORT_ALWAYS_AVAILABLE( WritePermissionLost );
    void push( interface::WritePermissionLost const &, PhysicalMemoryAddress & anAddress) {
        theMicroArch->writePermissionLost(anAddress);
    }

    /* CMU-ONLY-BLOCK-BEGIN */
    // Purge implementation for R-NUCA
    FLEXUS_PORT_ALWAYS_AVAILABLE(PurgeAddrIn);
    void push(interface::PurgeAddrIn const &, MemoryTransport & aTransport) {
      DBG_ (Iface,
            Addr(aTransport[ MemoryMessageTag ]->address())
            ( << " received purge " << *aTransport[ MemoryMessageTag ] ));

      boost::intrusive_ptr< MemOp > op;
      boost::intrusive_ptr<MemoryMessage> msg (aTransport[MemoryMessageTag]);
      if (aTransport[uArchStateTag]) {
        op = aTransport[uArchStateTag];
      } else {
        op = new MemOp();
        op->thePAddr = msg->address();
        op->theIdxExtraOffsetBits = msg->idxExtraOffsetBits();
        op->thePageState = msg->pageState();
        op->dstMC = msg->dstMC();
        op->theSize = eSize(msg->reqSize());
        op->theTracker = aTransport[TransactionTrackerTag];
      }
      switch ( msg->type() ) {
        case MemoryMessage::PurgeIReq:     op->theOperation = kIPurge; break;
        case MemoryMessage::PurgeDReq:     op->theOperation = kDPurge; break;
        case MemoryMessage::FinalizePurge: op->theOperation = kFPurge; break;
        default:                           DBG_Assert( false );
      }

      theMicroArch->initializePurge( op );
    }
    /* CMU-ONLY-BLOCK-END */


  public:
    //The FetchDrive drive interface sends a commands to the Feeder and then fetches instructions,
    //passing each instruction to its FetchOut port.
    void drive(interface::uArchDrive const &) {
      doCycle();
    }

  private:
    struct ResynchronizeWithSimicsException {};

    void squash(eSquashCause aSquashReason) {
        FLEXUS_CHANNEL( SquashOut ) << aSquashReason;
    }

    void changeState(int aTL, int aPSTATE) {
      CPUState state;
      state.theTL = aTL;
      state.thePSTATE = aPSTATE;
      FLEXUS_CHANNEL( ChangeCPUState ) << state;
    }

    void redirect(VirtualMemoryAddress aPC, VirtualMemoryAddress aNextPC) {
        std::pair< VirtualMemoryAddress, VirtualMemoryAddress> redirect_addr = std::make_pair( aPC, aNextPC);
        FLEXUS_CHANNEL( RedirectOut ) << redirect_addr;
    }

    void feedback(boost::intrusive_ptr<BranchFeedback> aFeedback) {
        FLEXUS_CHANNEL( BranchFeedbackOut ) << aFeedback;
    }

    void signalStoreForwardingHit(bool garbage) {
		bool value = true;
		FLEXUS_CHANNEL( StoreForwardingHitSeen ) << value;
    }

    /* CMU-ONLY-BLOCK-BEGIN */
    void notifyTMS(PredictorMessage::tPredictorMessageType aType, PhysicalMemoryAddress anAddress, boost::intrusive_ptr<TransactionTracker> aTracker) {
      //Inform TMS that we are completing read
      boost::intrusive_ptr<PredictorMessage> pmsg (new PredictorMessage(aType, flexusIndex(), anAddress));
      PredictorTransport pt;
      pt.set(PredictorMessageTag, pmsg);
      pt.set(TransactionTrackerTag, aTracker);
      FLEXUS_CHANNEL(NotifyTMS) << pt;
    }
    /* CMU-ONLY-BLOCK-END */


    void doCycle() {
      if (cfg.Multithread) {
        if (nMTManager::MTManager::get()->runThisEX(flexusIndex())) {
          theMicroArch->cycle();
        } else {
          theMicroArch->skipCycle();
        }
      } else {
        theMicroArch->cycle();
      }
      sendMemoryMessages();
    }

    void sendMemoryMessages() {
      while (FLEXUS_CHANNEL( MemoryOut_Request ).available()) {
        boost::intrusive_ptr< MemOp > op(theMicroArch->popMemOp());
        if (! op ) break;

        DBG_( Iface, ( << "Send Request: " << *op) );

        MemoryTransport transport;
        boost::intrusive_ptr<MemoryMessage> operation;

        if (op->theNAW) {
          DBG_Assert( op->theOperation == kStore );
          operation = new MemoryMessage(MemoryMessage::NonAllocatingStoreReq, op->thePAddr, op->thePC);
          operation->idxExtraOffsetBits() = op->theIdxExtraOffsetBits;
          operation->pageState() = op->thePageState;
          operation->dstMC() = op->dstMC;
        } else {

          switch( op->theOperation ) {
          case kLoad:
            //pc = Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
            operation = MemoryMessage::newLoad(op->thePAddr, op->thePC);
            break;

          case kAtomicPreload:
            //pc = Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
            operation = MemoryMessage::newAtomicPreload(op->thePAddr, op->thePC);
            break;

          case kStorePrefetch:
            //pc = Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
            operation = MemoryMessage::newStorePrefetch(op->thePAddr, op->thePC, DataWord(op->theValue));
            break;

          case kStore:
            //pc = Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
            operation = MemoryMessage::newStore(op->thePAddr, op->thePC, DataWord(op->theValue));
            break;

          case kRMW:
            //pc = Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
            operation = MemoryMessage::newRMW(op->thePAddr, op->thePC, DataWord(op->theValue));
            break;

          case kCAS:
            //pc = Simics::Processor::getProcessor(flexusIndex())->translateInstruction(op->thePC);
            operation = MemoryMessage::newCAS(op->thePAddr, op->thePC, DataWord(op->theValue));
            break;

          default:
            DBG_Assert( false,  ( << "Unknown memory operation type: " << op->theOperation ) );
          }
        }
        operation->reqSize() = op->theSize;
        if (op->theTracker) {
          transport.set(TransactionTrackerTag, op->theTracker);
        } else {
          boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
          tracker->setAddress( op->thePAddr );
          tracker->setInitiator(flexusIndex());
          tracker->setSource("uArch");
          tracker->setOS(false); //TWENISCH - need to set this properly
          transport.set(TransactionTrackerTag, tracker);
          op->theTracker = tracker;
        }

        transport.set(MemoryMessageTag, operation);
        transport.set(uArchStateTag, op);

        if (op->theNAW && (op->thePAddr & 63) != 0) {
          //Auto-reply to the unaligned parts of NAW 
          transport[MemoryMessageTag]->type() = MemoryMessage::NonAllocatingStoreReply; 
          handleMemoryMessage( transport );
        } else {
          FLEXUS_CHANNEL( MemoryOut_Request) << transport;
        }
      }

      while (FLEXUS_CHANNEL( MemoryOut_Snoop).available()) {
        boost::intrusive_ptr< MemOp > op(theMicroArch->popSnoopOp());
        if (! op ) break;

        DBG_( Iface, ( << "Send Snoop: " << *op) );

        MemoryTransport transport;
        boost::intrusive_ptr<MemoryMessage> operation;

        PhysicalMemoryAddress pc;

        switch( op->theOperation ) {
          case kInvAck:
            DBG_( Verb, ( << "Send InvAck.") );
            operation = new MemoryMessage(MemoryMessage::InvalidateAck, op->thePAddr);
            break;

          case kDowngradeAck:
            DBG_( Verb, ( << "Send DowngradeAck.") );
            operation = new MemoryMessage(MemoryMessage::DowngradeAck, op->thePAddr);
            break;

          case kProbeAck:
            DBG_( Verb, ( << "Send ProbeAck.") );
            operation = new MemoryMessage(MemoryMessage::ProbedNotPresent, op->thePAddr);
            break;

          case kReturnReply:
            DBG_( Verb, ( << "Send ReturnReply.") );
            operation = new MemoryMessage(MemoryMessage::ReturnReply, op->thePAddr );
            break;

          default:
            DBG_Assert( false,  ( << "Unknown memory operation type: " << op->theOperation ) );
        }

        operation->reqSize() = op->theSize;
        operation->idxExtraOffsetBits() = op->theIdxExtraOffsetBits;
        operation->pageState() = op->thePageState;
        operation->dstMC() = op->dstMC;
        if (op->theTracker) {
          transport.set(TransactionTrackerTag, op->theTracker);
        } else {
          boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
          tracker->setAddress( op->thePAddr );
          tracker->setInitiator(flexusIndex());
          tracker->setSource("uArch");
          transport.set(TransactionTrackerTag, tracker);
        }

        transport.set(MemoryMessageTag, operation);

        FLEXUS_CHANNEL( MemoryOut_Snoop) << transport;
      }

      /* CMU-ONLY-BLOCK-BEGIN */
      //nikos
      while (FLEXUS_CHANNEL( MemoryOut_Purge).available()) {
        boost::intrusive_ptr< MemOp > op(theMicroArch->popPurgeOp());
        if (! op ) break;

        DBG_( Iface, ( << "Send Purge: " << *op) );

        MemoryTransport transport;
        boost::intrusive_ptr<MemoryMessage> operation;

        PhysicalMemoryAddress pc;

        switch( op->theOperation ) {

          case kIPurge:
            DBG_( Verb, ( << "Send PurgeIReq.") );
            operation = new MemoryMessage(MemoryMessage::PurgeIReq, op->thePAddr, false, flexusIndex(), flexusIndex(), op->theIdxExtraOffsetBits);
            break;

          case kDPurge:
            DBG_( Verb, ( << "Send PurgeDReq.") );
            operation = new MemoryMessage(MemoryMessage::PurgeDReq, op->thePAddr, true, flexusIndex(), flexusIndex(), op->theIdxExtraOffsetBits);
            break;

          case kFPurge:
            DBG_( Verb, ( << "Send FinalizePurge.") );
            operation = new MemoryMessage(MemoryMessage::FinalizePurge, op->thePAddr, true, flexusIndex(), flexusIndex(), op->theIdxExtraOffsetBits);
            break;

          default:
            DBG_Assert( false,  ( << "Unknown memory operation type: " << op->theOperation ) );
        }

        operation->reqSize() = op->theSize;
        operation->idxExtraOffsetBits() = op->theIdxExtraOffsetBits;
        operation->pageState() = op->thePageState;
        operation->dstMC() = op->dstMC;
        if (op->theTracker) {
          transport.set(TransactionTrackerTag, op->theTracker);
        } else {
          boost::intrusive_ptr<TransactionTracker> tracker = new TransactionTracker;
          tracker->setAddress( op->thePAddr );
          tracker->setInitiator(flexusIndex());
          tracker->setSource("uArch");
          transport.set(TransactionTrackerTag, tracker);
        }

        transport.set(MemoryMessageTag, operation);

        FLEXUS_CHANNEL( MemoryOut_Purge) << transport;
      }
      /* CMU-ONLY-BLOCK-END */

    }

    void handleMemoryMessage( MemoryTransport & aTransport) {
      boost::intrusive_ptr< MemOp > op;
      boost::intrusive_ptr<MemoryMessage> msg (aTransport[MemoryMessageTag]);

      if (aTransport[uArchStateTag]) {
        op = aTransport[uArchStateTag];
      } else {
        op = new MemOp();
        op->thePAddr = msg->address();
        op->theIdxExtraOffsetBits = msg->idxExtraOffsetBits();
        op->thePageState = msg->pageState();
        op->dstMC = msg->dstMC();
        op->theSize = eSize(msg->reqSize());
        op->theTracker = aTransport[TransactionTrackerTag];
      }

      switch (msg->type()) {
        case MemoryMessage::LoadReply:
            op->theOperation = kLoadReply;
            break;

        case MemoryMessage::AtomicPreloadReply:
            op->theOperation = kAtomicPreloadReply;
            break;

        case MemoryMessage::StoreReply:
            op->theOperation = kStoreReply;
            break;

        case MemoryMessage::NonAllocatingStoreReply:
            op->theOperation = kStoreReply;
            break;

        case MemoryMessage::StorePrefetchReply:
            op->theOperation = kStorePrefetchReply;
            break;

        case MemoryMessage::Invalidate:
            op->theOperation = kInvalidate;
            break;

        case MemoryMessage::Downgrade:
            op->theOperation = kDowngrade;
            break;

        /* CMU-ONLY-BLOCK-BEGIN */
        case MemoryMessage::PurgeAck:
          DBG_ (Iface,
                Addr(aTransport[ MemoryMessageTag ]->address())
                ( << " handling purgeAck: " << *aTransport[ MemoryMessageTag ]
               ));

          FLEXUS_CHANNEL( PurgeAckOut ) << aTransport;
          return;
        /* CMU-ONLY-BLOCK-END */

        case MemoryMessage::Probe:
            op->theOperation = kProbe;
            break;

        case MemoryMessage::RMWReply:
            op->theOperation = kRMWReply;
            break;

        case MemoryMessage::CmpxReply:
            op->theOperation = kCASReply;
            break;

        case MemoryMessage::ReturnReq:
          op->theOperation = kReturnReq;
          break;

        default:
            DBG_Assert( false,  ( << "Unhandled Memory Message type: " << msg->type() ) );
      }

      theMicroArch->pushMemOp(op);
    }
};


}//End namespace nuArch

FLEXUS_COMPONENT_INSTANTIATOR(uArch,nuArch);

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT uArch

#define DBG_Reset
#include DBG_Control()
