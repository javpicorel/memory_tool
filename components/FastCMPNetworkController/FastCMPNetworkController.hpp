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

#include <core/simulator_layout.hpp>

#include <components/Common/Slices/MemoryMessage.hpp>

#define FLEXUS_BEGIN_COMPONENT FastCMPNetworkController
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
    PARAMETER( NumCores, int, "Number of cores (0 = sys width)", "num_cores", 0 )
    PARAMETER( NumL2Slices, int, "Number of L2 Slices (0 = sys width)", "num_slices", 0 )
    PARAMETER( PageSize, int, "Page size in bytes", "page_size", 8192 )
    PARAMETER( CacheLineSize, int, "Cahe line size in bytes", "cache_line_size", 64 )

    PARAMETER( Placement, std::string, "Placement policy [shared, private, R-NUCA]", "placement_policy", "shared" )
    PARAMETER( PrivateWithASR, bool, "Turn on ASR under Private scheme", "private_asr_enable", false ) /* CMU-ONLY */

    PARAMETER( DecoupleInstrDataSpaces, bool, "Decouple instruction from data address spaces", "decouple_addr_spaces", false ) /* CMU-ONLY */

    PARAMETER( WhiteBoxDebug, bool, "White box debugging on/off switch", "whitebox_debug", false )

    /* CMU-ONLY-BLOCK-BEGIN */
    // XJ: Parameters for a bounded directory
    PARAMETER( DirEntries, int, "Number of entries in each bounded sub-directory.", "dentries", 0 )
    PARAMETER( DirWays,    int, "Number of ways in each bounded sub-directory.", "dways", 4 )

    // ASR parameters
    PARAMETER( NLHBSize, int, "NLHB size", "nlhb_size", 1<<14 )
    PARAMETER( VTBSize, int, "VTB size", "vtb_size", 1<<10 )
    PARAMETER( MonitorSize, int, "Monitor size", "monitor_size", 1<<10 )
    PARAMETER( FramesPerL2Slice, int, "Number of frames per L2 slice", "frames_per_l2_slice", 64*1024 )
    PARAMETER( ASRLocalHitLatency, int, "Average local L2 hit latency", "asr_local_hit_latency", 1 )
    PARAMETER( ASRRemoteHitLatency, int, "Average remote L2 hit latency", "asr_remote_hit_latency", 4 )
    PARAMETER( ASROffChipLatency, int, "Off-Chip latency", "asr_off_chip_latency", 18 )
    PARAMETER( CleanSingletonRingWritebacks, bool, "Turn on ring writebacks for L2 clean singleton evicts under Private scheme", "clean_singleton_ring_writebacks", false )
    /* CMU-ONLY-BLOCK-END */

    PARAMETER( SizeOfInstrCluster, unsigned int, "Number of cores in instruction interleaving cluster", "instr_cluster_size", 4 )
    PARAMETER( SizeOfPrivCluster, unsigned int, "Number of cores in private data interleaving cluster", "priv_cluster_size", 1 )
    PARAMETER( NumCoresPerTorusRow, unsigned int, "Number of cores each torus row", "torus_row_size", 4 )
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, RequestInD )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, RequestInI )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, RequestFromL2 )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, SnoopInD )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, SnoopInI )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, BusSnoopIn )

  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, RequestOutD )
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, RequestOutI )
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, RequestOutMem )
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, SnoopOutD )
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, SnoopOutI )
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, ToL2Snoops )

  DRIVE( UpdateStatsDrive )
);


#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT FastCMPNetworkController
