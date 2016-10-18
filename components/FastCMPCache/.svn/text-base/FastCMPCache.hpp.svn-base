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

#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>
#include <components/Common/Slices/ReuseDistanceSlice.hpp>  /* CMU-ONLY */
#include <components/Common/Slices/PerfectPlacementSlice.hpp> /* CMU-ONLY */

#define FLEXUS_BEGIN_COMPONENT FastCMPCache
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
    PARAMETER( CMPWidth, int, "Number of cores per CMP chip (0 = sys width)", "CMPwidth", 16 )

    PARAMETER( Size, int, "Cache size in bytes", "size", 16777216 )
    PARAMETER( Associativity, int, "Set associativity", "assoc", 8 )
    PARAMETER( BlockSize, int, "Block size", "bsize", 64 )

    PARAMETER( CleanEvictions, bool, "Issue clean evictions", "clean_evict", false )
    PARAMETER( CacheLevel, Flexus::SharedTypes::tFillLevel, "CacheLevel", "level", Flexus::SharedTypes::eUnknown )

    PARAMETER( InvalidateFriendIndices, bool, "Invalidate all indices for an address in R-NUCA wo/ I-/D-stream decoupling", "inval_friend_idx", false ) /* CMU-ONLY */
    PARAMETER( DirtyRingWritebacks, bool, "Turn on dirty ring writebacks under Private scheme", "dirty_ring_writebacks", false ) /* CMU-ONLY */

    PARAMETER( ReplPolicy, std::string, "Replacement policy [LRU, ReuseDistRepl]", "repl_policy", "LRU" ) /* CMU-ONLY */
    PARAMETER( ReuseDistStats, bool, "Enable reuse distance stats", "reuse_dist_stats", false )   /* CMU-ONLY */
    PARAMETER( PerfectPlacement, bool, "Simulate perfect placement", "perfect_placement", false ) /* CMU-ONLY */

    PARAMETER( NotifyReads, bool, "Notify on L2 reads (does not notify on fast-hit)", "notify_reads", false )
    PARAMETER( NotifyWrites, bool, "Notify on L2 writes", "notify_writes", false )
    PARAMETER( NotifyFetches, bool, "Notify on L2 fetches (does not notify on fast-hit)", "notify_fetches", false )
    PARAMETER( NotifyL1CleanEvicts, bool, "Notify on L1 clean evicts", "notify_L1CleanEvicts", false )
    PARAMETER( NotifyL1DirtyEvicts, bool, "Notify on L1 dirty evicts", "notify_L1DirtyEvicts", false )
    PARAMETER( NotifyL1IEvicts, bool, "Notify on L1-I evicts", "notify_L1IEvicts", false )

);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, RequestIn )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, FetchRequestIn )
  PORT( PushInput, MemoryMessage, SnoopIn )
  PORT( PushOutput, MemoryMessage, RequestOut )
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, SnoopOutD )
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, SnoopOutI )

  PORT( PushOutput, ReuseDistanceSlice, ReuseDist )  /* CMU-ONLY */

  PORT( PushOutput, MemoryMessage, MissClassifier )  /* CMU-ONLY */

  PORT( PushOutput, PerfectPlacementSlice, PerfPlc ) /* CMU-ONLY */

  PORT( PushOutput, MemoryMessage, Reads )
  PORT( PushOutput, MemoryMessage, Writes )
  PORT( PushOutput, MemoryMessage, Fetches )
  PORT( PushOutput, MemoryMessage, L1CleanEvicts )
  PORT( PushOutput, MemoryMessage, L1DirtyEvicts )
  PORT( PushOutput, MemoryMessage, L1IEvicts )

  DRIVE( UpdateStatsDrive )
);


#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT FastCMPCache
