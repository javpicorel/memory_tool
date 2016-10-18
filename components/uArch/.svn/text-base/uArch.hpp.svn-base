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

#include <core/simulator_layout.hpp>

#include <core/boost_extensions/intrusive_ptr.hpp>

#include <components/Common/Transports/MemoryTransport.hpp>
#include <components/Common/Transports/PredictorTransport.hpp> /* CMU-ONLY */
#include <components/Common/Slices/AbstractInstruction.hpp>
#include <components/uFetch/uFetchTypes.hpp>

#define FLEXUS_BEGIN_COMPONENT uArch
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
    PARAMETER( ROBSize, unsigned int, "Reorder buffer size", "rob", 256 )
    PARAMETER( SBSize, unsigned int, "Store buffer size", "sb", 256 )
    PARAMETER( NAWBypassSB, bool, "Allow Non-Allocating-Writes to bypass store-buffer", "naw_bypass_sb", false )
    PARAMETER( NAWWaitAtSync, bool, "Force MEMBAR #Sync to wait for non-allocating writes to finish", "naw_wait_at_sync", false )
    PARAMETER( RetireWidth, unsigned int, "Retirement width", "retire", 8 )
    PARAMETER( MemoryPorts, unsigned int, "Memory Ports", "memports", 4 )
    PARAMETER( SnoopPorts, unsigned int, "Snoop Ports", "snoopports", 1 )
    PARAMETER( PurgePorts, unsigned int, "Purge Ports", "purgeports", 1 ) /* CMU-ONLY */
    PARAMETER( StorePrefetches, unsigned int, "Simultaneous store prefeteches", "storeprefetch", 16 )
    PARAMETER( PrefetchEarly, bool, "Issue store prefetch requests when address resolves", "prefetch_early", false )
    PARAMETER( ConsistencyModel, unsigned int, "Consistency Model", "consistency", 0 /* SC */ )
    PARAMETER( CoherenceUnit, unsigned int, "Coherence Unit", "coherence", 64 )
    PARAMETER( BreakOnResynchronize, bool, "Break on resynchronizer", "break_on_resynch", false )
    PARAMETER( SpinControl, bool, "Enable spin control", "spin_control", true )
    PARAMETER( SpeculativeOrder, bool, "Speculate on Memory Order", "spec_order", false )
    PARAMETER( SpeculateOnAtomicValue, bool, "Speculate on the Value of Atomics", "spec_atomic_val", false )
    PARAMETER( SpeculateOnAtomicValuePerfect, bool, "Use perfect atomic value prediction", "spec_atomic_val_perfect", false )
    PARAMETER( SpeculativeCheckpoints, int, "Number of checkpoints allowed.  0 for infinite", "spec_ckpts", 0)
    PARAMETER( CheckpointThreshold, int, "Number of instructions between checkpoints.  0 disables periodic checkpoints", "ckpt_threshold", 0)
    PARAMETER( EarlySGP, bool, "Notify SGP Early", "early_sgp", false )   /* CMU-ONLY */
    PARAMETER( TrackParallelAccesses, bool, "Track which memory accesses can proceed in parallel", "track_parallel", false ) /* CMU-ONLY */
    PARAMETER( InOrderMemory, bool, "Only allow ROB/SB head to issue to memory", "in_order_memory", false )
    PARAMETER( InOrderExecute, bool, "Ensure that instructions execute in order", "in_order_execute", false )
    PARAMETER( OnChipLatency, unsigned long, "On-Chip Side-Effect latency", "on-chip-se", 0)
    PARAMETER( OffChipLatency, unsigned long, "Off-Chip Side-Effect latency", "off-chip-se", 0)
    PARAMETER( Multithread, bool, "Enable multi-threaded execution", "multithread", false )

    PARAMETER( NumIntAlu, unsigned int, "Number of integer ALUs", "numIntAlu", 1)
    PARAMETER( IntAluOpLatency, unsigned int, "End-to-end latency of an integer ALU operation", "intAluOpLatency", 1)
    PARAMETER( IntAluOpPipelineResetTime, unsigned int, "Number of cycles required between subsequent integer ALU operations", "intAluOpPipelineResetTime", 1)
    
    PARAMETER( NumIntMult, unsigned int, "Number of integer MUL/DIV units", "numIntMult", 1)
    PARAMETER( IntMultOpLatency, unsigned int, "End-to-end latency of an integer MUL operation", "intMultOpLatency", 1)
    PARAMETER( IntMultOpPipelineResetTime, unsigned int, "Number of cycles required between subsequent integer MUL operations", "intMultOpPipelineResetTime", 1)
    PARAMETER( IntDivOpLatency, unsigned int, "End-to-end latency of an integer DIV operation", "intDivOpLatency", 1)
    PARAMETER( IntDivOpPipelineResetTime, unsigned int, "Number of cycles required between subsequent integer DIV operations", "intDivOpPipelineResetTime", 1)
    
    PARAMETER( NumFpAlu, unsigned int, "Number of FP ALUs", "numFpAlu", 1)
    PARAMETER( FpAddOpLatency, unsigned int, "End-to-end latency of an FP ADD/SUB operation", "fpAddOpLatency", 1)
    PARAMETER( FpAddOpPipelineResetTime, unsigned int, "Number of cycles required between subsequent FP ADD/SUB operations", "fpAddOpPipelineResetTime", 1)
    PARAMETER( FpCmpOpLatency, unsigned int, "End-to-end latency of an FP compare operation", "fpCmpOpLatency", 1)
    PARAMETER( FpCmpOpPipelineResetTime, unsigned int, "Number of cycles required between subsequent FP compare operations", "fpCmpOpPipelineResetTime", 1)
    PARAMETER( FpCvtOpLatency, unsigned int, "End-to-end latency of an FP convert operation", "fpCvtOpLatency", 1)
    PARAMETER( FpCvtOpPipelineResetTime, unsigned int, "Number of cycles required between subsequent FP convert operations", "fpCvtOpPipelineResetTime", 1)
        
    PARAMETER( NumFpMult, unsigned int, "Number of FP MUL/DIV units", "numFpMult", 1)
    PARAMETER( FpMultOpLatency, unsigned int, "End-to-end latency of an FP MUL operation", "fpMultOpLatency", 1)
    PARAMETER( FpMultOpPipelineResetTime, unsigned int, "Number of cycles required between subsequent FP MUL operations", "fpMultOpPipelineResetTime", 1)
    PARAMETER( FpDivOpLatency, unsigned int, "End-to-end latency of an FP DIV operation", "fpDivOpLatency", 1)
    PARAMETER( FpDivOpPipelineResetTime, unsigned int, "Number of cycles required between subsequent FP DIV operations", "fpDivOpPipelineResetTime", 1)
    PARAMETER( FpSqrtOpLatency, unsigned int, "End-to-end latency of an FP SQRT operation", "fpSqrtOpLatency", 1)
    PARAMETER( FpSqrtOpPipelineResetTime, unsigned int, "Number of cycles required between subsequent FP SQRT operations", "fpSqrtOpPipelineResetTime", 1)   
    PARAMETER( ValidateMMU, bool, "Validate MMU after each instruction", "validate-mmu", false )
);

typedef std::pair<int, bool> dispatch_status;
typedef std::pair<VirtualMemoryAddress, VirtualMemoryAddress> vaddr_pair;

COMPONENT_INTERFACE(
  PORT( PushInput, boost::intrusive_ptr< AbstractInstruction >, DispatchIn)
  PORT( PullOutput, dispatch_status, AvailableDispatchOut)
  PORT( PullOutput, bool, Stalled)
  PORT( PullOutput, int, ICount)
  PORT( PushOutput, eSquashCause, SquashOut )
  PORT( PushOutput, vaddr_pair, RedirectOut )
  PORT( PushOutput, CPUState, ChangeCPUState )
  PORT( PushOutput, boost::intrusive_ptr<BranchFeedback>, BranchFeedbackOut )
  PORT( PushOutput, PredictorTransport, NotifyTMS ) /* CMU-ONLY */
  PORT( PushOutput, MemoryTransport, MemoryOut_Request )
  PORT( PushOutput, MemoryTransport, MemoryOut_Snoop )
  PORT( PushInput, MemoryTransport, MemoryIn_Reply )
  PORT( PushInput, MemoryTransport, MemoryIn_Request )
  PORT( PushInput, PhysicalMemoryAddress, WritePermissionLost )
  
  PORT( PushOutput, bool, StoreForwardingHitSeen) // Signal a store forwarding hit in the LSQ to the PowerTracker
  
  /* CMU-ONLY-BLOCK-BEGIN */
  PORT( PushInput, MemoryTransport, PurgeAddrIn) // fixme nikos: address to purge as a side-effect
  PORT( PushOutput, MemoryTransport, PurgeAckOut) // fixme nikos: address to purge as a side-effect
  PORT( PushOutput, MemoryTransport, MemoryOut_Purge )
  /* CMU-ONLY-BLOCK-END */
  
  DRIVE( uArchDrive )
);


#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT uArch


