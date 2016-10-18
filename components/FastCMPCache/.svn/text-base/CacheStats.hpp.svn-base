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

#ifndef FLEXUS_FASTCMPCACHE_CACHESTATS_HPP_INCLUDED
#define FLEXUS_FASTCMPCACHE_CACHESTATS_HPP_INCLUDED

#include <core/stats.hpp>

namespace nFastCMPCache {

namespace Stat = Flexus::Stat;

struct CacheStats {
  Stat::StatCounter theHits_Fetch_stat;
  Stat::StatCounter theHits_Read_stat;
  Stat::StatCounter theHits_Write_stat;
  Stat::StatCounter theHits_Atomic_stat;
  Stat::StatCounter theHits_Upgrade_stat;
  Stat::StatCounter theHits_Evict_stat;
  Stat::StatCounter theHits_OS_Fetch_stat;
  Stat::StatCounter theHits_OS_Read_stat;
  Stat::StatCounter theHits_OS_Write_stat;
  Stat::StatCounter theHits_OS_Atomic_stat;
  Stat::StatCounter theHits_OS_Upgrade_stat;
  Stat::StatCounter theHits_OS_Evict_stat;

  Stat::StatCounter theMisses_Fetch_stat;
  Stat::StatCounter theMisses_Read_stat;
  Stat::StatCounter theMisses_Write_stat;
  Stat::StatCounter theMisses_Upgrade_stat;
  Stat::StatCounter theMisses_OS_Fetch_stat;
  Stat::StatCounter theMisses_OS_Read_stat;
  Stat::StatCounter theMisses_OS_Write_stat;
  Stat::StatCounter theMisses_OS_Upgrade_stat;

  Stat::StatCounter theMisses_CleanEvictNonAllocating_stat;
  Stat::StatCounter theMisses_OS_CleanEvictNonAllocating_stat;
  Stat::StatCounter theRemoteAllocs_stat;

  Stat::StatCounter theSnoops_Invalidate_stat;
  Stat::StatCounter theSnoops_Downgrade_stat;
  Stat::StatCounter theSnoops_ReturnReq_stat;
  Stat::StatCounter theSnoops_Purge_stat;           /* CMU-ONLY */
  Stat::StatCounter theSnoops_PurgeWriteBack_stat;           /* CMU-ONLY */

  Stat::StatCounter theHits_Read_HierarchyDowngrade_stat;
  Stat::StatCounter theHits_Read_HierarchyReturnReq_stat;
  Stat::StatCounter theHits_Write_HierarchyExclusive_stat;
  Stat::StatCounter theMisses_Upgrade_HierarchyWrMiss_stat;
  Stat::StatCounter theHits_OS_Read_HierarchyDowngrade_stat;
  Stat::StatCounter theHits_OS_Read_HierarchyReturnReq_stat;
  Stat::StatCounter theHits_OS_Write_HierarchyExclusive_stat;
  Stat::StatCounter theMisses_OS_Upgrade_HierarchyWrMiss_stat;

  Stat::StatCounter theHits_Fetch_HierarchyDowngrade_stat;
  Stat::StatCounter theHits_Fetch_HierarchyReturnReq_stat;
  Stat::StatCounter theHits_OS_Fetch_HierarchyDowngrade_stat;
  Stat::StatCounter theHits_OS_Fetch_HierarchyReturnReq_stat;

  Stat::StatCounter theEvicts_Dirty_stat;
  Stat::StatCounter theEvicts_Dirty_ExclusiveState_stat;
  Stat::StatCounter theEvicts_Dirty_SharedState_stat;
  Stat::StatCounter theEvicts_Writable_stat;
  Stat::StatCounter theEvicts_Writable_ExclusiveState_stat;
  Stat::StatCounter theEvicts_Writable_SharedState_stat;
  Stat::StatCounter theEvicts_Valid_stat;
  Stat::StatCounter theEvicts_Valid_SharedState_stat;

  long theHits_Fetch;
  long theHits_Read;
  long theHits_Write;
  long theHits_Atomic;
  long theHits_Upgrade;
  long theHits_Evict;
  long theHits_OS_Fetch;
  long theHits_OS_Read;
  long theHits_OS_Write;
  long theHits_OS_Atomic;
  long theHits_OS_Upgrade;
  long theHits_OS_Evict;

  long theMisses_Fetch;
  long theMisses_Read;
  long theMisses_Write;
  long theMisses_Upgrade;
  long theMisses_OS_Fetch;
  long theMisses_OS_Read;
  long theMisses_OS_Write;
  long theMisses_OS_Upgrade;

  long theMisses_CleanEvictNonAllocating;
  long theMisses_OS_CleanEvictNonAllocating;
  long theRemoteAllocs;

  long theSnoops_Invalidate;
  long theSnoops_Downgrade;
  long theSnoops_ReturnReq;
  long theSnoops_Purge;           /* CMU-ONLY */
  long theSnoops_PurgeWriteBack;           /* CMU-ONLY */

  long theHits_Read_HierarchyDowngrade;
  long theHits_Read_HierarchyReturnReq;
  long theHits_Write_HierarchyExclusive;
  long theMisses_Upgrade_HierarchyWrMiss;
  long theHits_OS_Read_HierarchyDowngrade;
  long theHits_OS_Read_HierarchyReturnReq;
  long theHits_OS_Write_HierarchyExclusive;
  long theMisses_OS_Upgrade_HierarchyWrMiss;

  long theHits_Fetch_HierarchyDowngrade;
  long theHits_Fetch_HierarchyReturnReq;
  long theHits_OS_Fetch_HierarchyDowngrade;
  long theHits_OS_Fetch_HierarchyReturnReq;

  long theEvicts_Dirty;
  long theEvicts_Dirty_ExclusiveState;
  long theEvicts_Dirty_SharedState;
  long theEvicts_Writable;
  long theEvicts_Writable_ExclusiveState;
  long theEvicts_Writable_SharedState;
  long theEvicts_Valid;
  long theEvicts_Valid_SharedState;

  CacheStats(std::string const & theName)
    : theHits_Fetch_stat(theName + "-Hits:Fetch")
    , theHits_Read_stat(theName + "-Hits:Read")
    , theHits_Write_stat(theName + "-Hits:Write")
    , theHits_Atomic_stat(theName + "-Hits:Atomic")
    , theHits_Upgrade_stat(theName + "-Hits:Upgrade")
    , theHits_Evict_stat(theName + "-Hits:Evict")
    , theHits_OS_Fetch_stat(theName + "-Hits:OS:Fetch")
    , theHits_OS_Read_stat(theName + "-Hits:OS:Read")
    , theHits_OS_Write_stat(theName + "-Hits:OS:Write")
    , theHits_OS_Atomic_stat(theName + "-Hits:OS:Atomic")
    , theHits_OS_Upgrade_stat(theName + "-Hits:OS:Upgrade")
    , theHits_OS_Evict_stat(theName + "-Hits:OS:Evict")

    , theMisses_Fetch_stat(theName + "-Misses:Fetch")
    , theMisses_Read_stat(theName + "-Misses:Read")
    , theMisses_Write_stat(theName + "-Misses:Write")
    , theMisses_Upgrade_stat(theName + "-Misses:Upgrade")
    , theMisses_OS_Fetch_stat(theName + "-Misses:OS:Fetch")
    , theMisses_OS_Read_stat(theName + "-Misses:OS:Read")
    , theMisses_OS_Write_stat(theName + "-Misses:OS:Write")
    , theMisses_OS_Upgrade_stat(theName + "-Misses:OS:Upgrade")

    , theMisses_CleanEvictNonAllocating_stat(theName + "-Misses:EvictNonAllocating")
    , theMisses_OS_CleanEvictNonAllocating_stat(theName + "-Misses:OS:EvictNonAllocating")
    , theRemoteAllocs_stat(theName + "-RemoteAllocations")

    , theSnoops_Invalidate_stat(theName + "-Snoops:Invalidates")
    , theSnoops_Downgrade_stat(theName + "-Snoops:Downgrades")
    , theSnoops_ReturnReq_stat(theName + "-Snoops:ReturnReq")
    , theSnoops_Purge_stat(theName + "-Snoops:Purges")           /* CMU-ONLY */
    , theSnoops_PurgeWriteBack_stat(theName + "-Snoops:PurgeWriteBacks")           /* CMU-ONLY */

    , theHits_Read_HierarchyDowngrade_stat(theName + "-Hits:Read:HierarchyDowngrade")
    , theHits_Read_HierarchyReturnReq_stat(theName + "-Hits:Read:HierarchyReturnReq")
    , theHits_Write_HierarchyExclusive_stat(theName + "-Hits:Write:HierarchyExclusive")
    , theMisses_Upgrade_HierarchyWrMiss_stat(theName + "-Misses:Upgrade:HierarchyWrMiss")
    , theHits_OS_Read_HierarchyDowngrade_stat(theName + "-Hits:OS:Read:HierarchyDowngrade")
    , theHits_OS_Read_HierarchyReturnReq_stat(theName + "-Hits:OS:Read:HierarchyReturnReq")
    , theHits_OS_Write_HierarchyExclusive_stat(theName + "-Hits:OS:Write:HierarchyExclusive")
    , theMisses_OS_Upgrade_HierarchyWrMiss_stat(theName + "-Misses:OS:Upgrade:HierarchyWrMiss")

    , theHits_Fetch_HierarchyDowngrade_stat(theName + "-Hits:Fetch:HierarchyDowngrade")
    , theHits_Fetch_HierarchyReturnReq_stat(theName + "-Hits:Fetch:HierarchyReturnReq")
    , theHits_OS_Fetch_HierarchyDowngrade_stat(theName + "-Hits:OS:Fetch:HierarchyDowngrade")
    , theHits_OS_Fetch_HierarchyReturnReq_stat(theName + "-Hits:OS:Fetch:HierarchyReturnReq")

    , theEvicts_Dirty_stat(theName + "-Evicts:Dirty")
    , theEvicts_Dirty_ExclusiveState_stat(theName + "Evicts:Dirty:ExclusiveState")
    , theEvicts_Dirty_SharedState_stat(theName + "Evicts:Dirty:SharedState")
    , theEvicts_Writable_stat(theName + "Evicts:Writable")
    , theEvicts_Writable_ExclusiveState_stat(theName + "Evicts:Writable:ExclusiveState")
    , theEvicts_Writable_SharedState_stat(theName + "Evicts:Writable:SharedState")
    , theEvicts_Valid_stat(theName + "Evicts:Valid")
    , theEvicts_Valid_SharedState_stat(theName + "Evicts:Valid:SharedState")

  {
    theHits_Fetch          = 0;
    theHits_Read           = 0;
    theHits_Write          = 0;
    theHits_Atomic         = 0;
    theHits_Upgrade        = 0;
    theHits_Evict          = 0;
    theHits_OS_Fetch       = 0;
    theHits_OS_Read        = 0;
    theHits_OS_Write       = 0;
    theHits_OS_Atomic      = 0;
    theHits_OS_Upgrade     = 0;
    theHits_OS_Evict       = 0;

    theMisses_Fetch        = 0;
    theMisses_Read         = 0;
    theMisses_Write        = 0;
    theMisses_Upgrade      = 0;
    theMisses_OS_Fetch     = 0;
    theMisses_OS_Read      = 0;
    theMisses_OS_Write     = 0;
    theMisses_OS_Upgrade   = 0;

    theMisses_CleanEvictNonAllocating    = 0;
    theMisses_OS_CleanEvictNonAllocating = 0;
    theRemoteAllocs                      = 0;

    theSnoops_Invalidate     = 0;
    theSnoops_Downgrade      = 0;
    theSnoops_ReturnReq      = 0;
    theSnoops_Purge          = 0;           /* CMU-ONLY */
    theSnoops_PurgeWriteBack = 0;           /* CMU-ONLY */

    theHits_Read_HierarchyDowngrade      = 0;
    theHits_Read_HierarchyReturnReq      = 0;
    theHits_Write_HierarchyExclusive     = 0;
    theMisses_Upgrade_HierarchyWrMiss    = 0;
    theHits_OS_Read_HierarchyDowngrade   = 0;
    theHits_OS_Read_HierarchyReturnReq   = 0;
    theHits_OS_Write_HierarchyExclusive  = 0;
    theMisses_OS_Upgrade_HierarchyWrMiss = 0;

    theHits_Fetch_HierarchyDowngrade     = 0;
    theHits_Fetch_HierarchyReturnReq     = 0;
    theHits_OS_Fetch_HierarchyDowngrade  = 0;
    theHits_OS_Fetch_HierarchyReturnReq  = 0;

    theEvicts_Dirty                   = 0;
    theEvicts_Dirty_ExclusiveState    = 0;
    theEvicts_Dirty_SharedState       = 0;
    theEvicts_Writable                = 0;
    theEvicts_Writable_ExclusiveState = 0;
    theEvicts_Writable_SharedState    = 0;
    theEvicts_Valid                   = 0;
    theEvicts_Valid_SharedState       = 0;

  }

  void update() {
    theHits_Fetch_stat          += theHits_Fetch;
    theHits_Read_stat           += theHits_Read;
    theHits_Write_stat          += theHits_Write;
    theHits_Atomic_stat         += theHits_Atomic;
    theHits_Upgrade_stat        += theHits_Upgrade;
    theHits_Evict_stat          += theHits_Evict;
    theHits_OS_Fetch_stat       += theHits_OS_Fetch;
    theHits_OS_Read_stat        += theHits_OS_Read;
    theHits_OS_Write_stat       += theHits_OS_Write;
    theHits_OS_Atomic_stat      += theHits_OS_Atomic;
    theHits_OS_Upgrade_stat     += theHits_OS_Upgrade;
    theHits_OS_Evict_stat       += theHits_OS_Evict;

    theMisses_Fetch_stat        += theMisses_Fetch;
    theMisses_Read_stat         += theMisses_Read;
    theMisses_Write_stat        += theMisses_Write;
    theMisses_Upgrade_stat      += theMisses_Upgrade;
    theMisses_OS_Fetch_stat     += theMisses_OS_Fetch;
    theMisses_OS_Read_stat      += theMisses_OS_Read;
    theMisses_OS_Write_stat     += theMisses_OS_Write;
    theMisses_OS_Upgrade_stat   += theMisses_OS_Upgrade;

    theMisses_CleanEvictNonAllocating_stat    += theMisses_CleanEvictNonAllocating;
    theMisses_OS_CleanEvictNonAllocating_stat += theMisses_OS_CleanEvictNonAllocating;
    theRemoteAllocs_stat                      += theRemoteAllocs;

    theSnoops_Invalidate_stat     += theSnoops_Invalidate;
    theSnoops_Downgrade_stat      += theSnoops_Downgrade;
    theSnoops_ReturnReq_stat      += theSnoops_ReturnReq;
    theSnoops_Purge_stat          += theSnoops_Purge;           /* CMU-ONLY */
    theSnoops_PurgeWriteBack_stat += theSnoops_PurgeWriteBack;           /* CMU-ONLY */

    theHits_Read_HierarchyDowngrade_stat      += theHits_Read_HierarchyDowngrade;
    theHits_Read_HierarchyReturnReq_stat      += theHits_Read_HierarchyReturnReq;
    theHits_Write_HierarchyExclusive_stat     += theHits_Write_HierarchyExclusive;
    theMisses_Upgrade_HierarchyWrMiss_stat    += theMisses_Upgrade_HierarchyWrMiss;
    theHits_OS_Read_HierarchyDowngrade_stat   += theHits_OS_Read_HierarchyDowngrade;
    theHits_OS_Read_HierarchyReturnReq_stat   += theHits_OS_Read_HierarchyReturnReq;
    theHits_OS_Write_HierarchyExclusive_stat  += theHits_OS_Write_HierarchyExclusive;
    theMisses_OS_Upgrade_HierarchyWrMiss_stat += theMisses_OS_Upgrade_HierarchyWrMiss;

    theHits_Fetch_HierarchyDowngrade_stat     += theHits_Fetch_HierarchyDowngrade;
    theHits_Fetch_HierarchyReturnReq_stat     += theHits_Fetch_HierarchyReturnReq;
    theHits_OS_Fetch_HierarchyDowngrade_stat  += theHits_OS_Fetch_HierarchyDowngrade;
    theHits_OS_Fetch_HierarchyReturnReq_stat  += theHits_OS_Fetch_HierarchyReturnReq;

    theEvicts_Dirty_stat                   += theEvicts_Dirty                   ; 
    theEvicts_Dirty_ExclusiveState_stat    += theEvicts_Dirty_ExclusiveState    ; 
    theEvicts_Dirty_SharedState_stat       += theEvicts_Dirty_SharedState       ; 
    theEvicts_Writable_stat                += theEvicts_Writable                ; 
    theEvicts_Writable_ExclusiveState_stat += theEvicts_Writable_ExclusiveState ; 
    theEvicts_Writable_SharedState_stat    += theEvicts_Writable_SharedState    ; 
    theEvicts_Valid_stat                   += theEvicts_Valid                   ; 
    theEvicts_Valid_SharedState_stat       += theEvicts_Valid_SharedState       ; 

    theHits_Fetch          = 0;
    theHits_Read           = 0;
    theHits_Write          = 0;
    theHits_Atomic         = 0;
    theHits_Upgrade        = 0;
    theHits_Evict          = 0;
    theHits_OS_Fetch       = 0;
    theHits_OS_Read        = 0;
    theHits_OS_Write       = 0;
    theHits_OS_Atomic      = 0;
    theHits_OS_Upgrade     = 0;
    theHits_OS_Evict       = 0;

    theMisses_Fetch        = 0;
    theMisses_Read         = 0;
    theMisses_Write        = 0;
    theMisses_Upgrade      = 0;
    theMisses_OS_Fetch     = 0;
    theMisses_OS_Read      = 0;
    theMisses_OS_Write     = 0;
    theMisses_OS_Upgrade   = 0;

    theMisses_CleanEvictNonAllocating    = 0;
    theMisses_OS_CleanEvictNonAllocating = 0;
    theRemoteAllocs                      = 0;

    theSnoops_Invalidate     = 0;
    theSnoops_Downgrade      = 0;
    theSnoops_ReturnReq      = 0;
    theSnoops_Purge          = 0;           /* CMU-ONLY */
    theSnoops_PurgeWriteBack = 0;           /* CMU-ONLY */

    theHits_Read_HierarchyDowngrade      = 0;
    theHits_Read_HierarchyReturnReq      = 0;
    theHits_Write_HierarchyExclusive     = 0;
    theMisses_Upgrade_HierarchyWrMiss    = 0;
    theHits_OS_Read_HierarchyDowngrade   = 0;
    theHits_OS_Read_HierarchyReturnReq   = 0;
    theHits_OS_Write_HierarchyExclusive  = 0;
    theMisses_OS_Upgrade_HierarchyWrMiss = 0;

    theHits_Fetch_HierarchyDowngrade     = 0;
    theHits_Fetch_HierarchyReturnReq     = 0;
    theHits_OS_Fetch_HierarchyDowngrade  = 0;
    theHits_OS_Fetch_HierarchyReturnReq  = 0;

    theEvicts_Dirty                   = 0;
    theEvicts_Dirty_ExclusiveState    = 0;
    theEvicts_Dirty_SharedState       = 0;
    theEvicts_Writable                = 0;
    theEvicts_Writable_ExclusiveState = 0;
    theEvicts_Writable_SharedState    = 0;
    theEvicts_Valid                   = 0;
    theEvicts_Valid_SharedState       = 0;

  }
};


}  // namespace nFastCMPCache

#endif /* FLEXUS_FASTCMPCACHE_CACHESTATS_HPP_INCLUDED */

