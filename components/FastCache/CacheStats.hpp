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

#ifndef FLEXUS_FASTCACHE_CACHESTATS_HPP_INCLUDED
#define FLEXUS_FASTCACHE_CACHESTATS_HPP_INCLUDED

#include <core/stats.hpp>

namespace nFastCache {

namespace Stat = Flexus::Stat;

struct CacheStats {
  Stat::StatCounter theHits_Read_stat;
  Stat::StatCounter theHits_Fetch_stat;
  Stat::StatCounter theHits_Write_stat;
  Stat::StatCounter theHits_Atomic_stat;
  Stat::StatCounter theHits_Upgrade_stat;
  Stat::StatCounter theHits_Evict_stat;
  Stat::StatCounter theHits_Prefetch_stat;

  Stat::StatCounter theMisses_Read_stat;
  Stat::StatCounter theMisses_Fetch_stat;
  Stat::StatCounter theMisses_Write_stat;
  Stat::StatCounter theMisses_Upgrade_stat;
  Stat::StatCounter theMisses_Prefetch_stat;

  Stat::StatCounter theSnoops_Invalidate_stat;
  Stat::StatCounter theSnoops_InvalidateValid_stat;
  Stat::StatCounter theSnoops_InvalidateDirty_stat;
  Stat::StatCounter theSnoops_Downgrade_stat;
  Stat::StatCounter theSnoops_DowngradeDirty_stat;
  Stat::StatCounter theSnoops_ReturnReq_stat;

  Stat::StatCounter theTagMatches_Invalid_stat;

  Stat::StatCounter thePrefetchHits_Read_stat;
  Stat::StatCounter thePrefetchHits_Write_stat;
  Stat::StatCounter thePrefetchHits_Evict_stat;
  Stat::StatCounter thePrefetchHits_ButUpgrade_stat;

  long theHits_Read;
  long theHits_Fetch;
  long theHits_Write;
  long theHits_Atomic;
  long theHits_Upgrade;
  long theHits_Evict;
  long theHits_Prefetch;

  long theMisses_Read;
  long theMisses_Fetch;
  long theMisses_Write;
  long theMisses_Upgrade;
  long theMisses_Prefetch;

  long theSnoops_Invalidate;
  long theSnoops_InvalidateValid;
  long theSnoops_InvalidateDirty;
  long theSnoops_Downgrade;
  long theSnoops_DowngradeDirty;
  long theSnoops_ReturnReq;

  long theTagMatches_Invalid;

  long thePrefetchHits_Read;
  long thePrefetchHits_Write;
  long thePrefetchHits_Evict;
  long thePrefetchHits_ButUpgrade;

  CacheStats(std::string const & theName)
    : theHits_Read_stat(theName + "-Hits:Read")
    , theHits_Fetch_stat(theName + "-Hits:Fetch")
    , theHits_Write_stat(theName + "-Hits:Write")
    , theHits_Atomic_stat(theName + "-Hits:Atomic")
    , theHits_Upgrade_stat(theName + "-Hits:Upgrade")
    , theHits_Evict_stat(theName + "-Hits:Evict")
    , theHits_Prefetch_stat(theName + "-Hits:Prefetch")

    , theMisses_Read_stat(theName + "-Misses:Read")
    , theMisses_Fetch_stat(theName + "-Misses:Fetch")
    , theMisses_Write_stat(theName + "-Misses:Write")
    , theMisses_Upgrade_stat(theName + "-Misses:Upgrade")
    , theMisses_Prefetch_stat(theName + "-Misses:Prefetch")

    , theSnoops_Invalidate_stat(theName + "-Snoops:Invalidates")
    , theSnoops_InvalidateValid_stat(theName + "-Snoops:Invalidates:Valid")
    , theSnoops_InvalidateDirty_stat(theName + "-Snoops:Invalidates:Dirty")
    , theSnoops_Downgrade_stat(theName + "-Snoops:Downgrades")
    , theSnoops_DowngradeDirty_stat(theName + "-Snoops:Downgrades:Dirty")
    , theSnoops_ReturnReq_stat(theName + "-Snoops:ReturnReq")
    , theTagMatches_Invalid_stat(theName + "-TagMatchesInvalid")

    , thePrefetchHits_Read_stat(theName + "-PrefetchHits:Read")
    , thePrefetchHits_Write_stat(theName + "-PrefetchHits:Write")
    , thePrefetchHits_Evict_stat(theName + "-PrefetchHits:Evict")
    , thePrefetchHits_ButUpgrade_stat(theName + "-PrefetchHits:ButUpgrade")
  {
    theHits_Read           = 0;
    theHits_Fetch          = 0;
    theHits_Write          = 0;
    theHits_Atomic         = 0;
    theHits_Upgrade        = 0;
    theHits_Evict          = 0;
    theHits_Prefetch       = 0;
    theMisses_Read         = 0;
    theMisses_Fetch        = 0;
    theMisses_Write        = 0;
    theMisses_Upgrade      = 0;
    theMisses_Prefetch     = 0;
    theSnoops_Invalidate   = 0;
    theSnoops_InvalidateValid = 0;
    theSnoops_InvalidateDirty = 0;
    theSnoops_Downgrade    = 0;
    theSnoops_DowngradeDirty = 0;
    theSnoops_ReturnReq    = 0;
    theTagMatches_Invalid  = 0;
    thePrefetchHits_Read   = 0;
    thePrefetchHits_Write  = 0;
    thePrefetchHits_Evict  = 0;
    thePrefetchHits_ButUpgrade = 0;
  }

  void update() {
    theHits_Read_stat           += theHits_Read           ;
    theHits_Fetch_stat          += theHits_Fetch          ;
    theHits_Write_stat          += theHits_Write          ;
    theHits_Atomic_stat         += theHits_Atomic         ;
    theHits_Upgrade_stat        += theHits_Upgrade        ;
    theHits_Evict_stat          += theHits_Evict          ;
    theHits_Prefetch_stat       += theHits_Prefetch       ;
    theMisses_Read_stat         += theMisses_Read         ;
    theMisses_Fetch_stat        += theMisses_Fetch        ;
    theMisses_Write_stat        += theMisses_Write        ;
    theMisses_Upgrade_stat      += theMisses_Upgrade      ;
    theMisses_Prefetch_stat     += theMisses_Prefetch     ;
    theSnoops_Invalidate_stat   += theSnoops_Invalidate   ;
    theSnoops_InvalidateValid_stat   += theSnoops_InvalidateValid   ;
    theSnoops_InvalidateDirty_stat   += theSnoops_InvalidateDirty   ;
    theSnoops_Downgrade_stat    += theSnoops_Downgrade    ;
    theSnoops_DowngradeDirty_stat    += theSnoops_DowngradeDirty    ;
    theSnoops_ReturnReq_stat    += theSnoops_ReturnReq;
    theTagMatches_Invalid_stat  += theTagMatches_Invalid  ;
    thePrefetchHits_Read_stat   += thePrefetchHits_Read   ;
    thePrefetchHits_Write_stat  += thePrefetchHits_Write  ;
    thePrefetchHits_Evict_stat  += thePrefetchHits_Evict  ;
    thePrefetchHits_ButUpgrade_stat += thePrefetchHits_ButUpgrade;

    theHits_Read           = 0;
    theHits_Fetch          = 0;
    theHits_Write          = 0;
    theHits_Atomic         = 0;
    theHits_Upgrade        = 0;
    theHits_Evict          = 0;
    theHits_Prefetch       = 0;
    theMisses_Read         = 0;
    theMisses_Fetch        = 0;
    theMisses_Write        = 0;
    theMisses_Upgrade      = 0;
    theMisses_Prefetch     = 0;
    theSnoops_Invalidate   = 0;
    theSnoops_InvalidateValid = 0;
    theSnoops_InvalidateDirty = 0;
    theSnoops_Downgrade    = 0;
    theSnoops_DowngradeDirty = 0;
    theSnoops_ReturnReq    = 0;
    theTagMatches_Invalid  = 0;
    thePrefetchHits_Read   = 0;
    thePrefetchHits_Write  = 0;
    thePrefetchHits_Evict  = 0;
    thePrefetchHits_ButUpgrade = 0;
  }
};


}  // namespace nFastCache

#endif /* FLEXUS_FASTCACHE_CACHESTATS_HPP_INCLUDED */

