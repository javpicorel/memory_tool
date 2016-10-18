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

#include "multi_cache.hpp"
#include "sequitur.hpp"
#include "trace.hpp"

#ifdef ENABLE_WHITE_BOX
  #include <components/WhiteBox/WhiteBoxIface.hpp>
#endif //ENABLE_WHITE_BOX

#include <core/flexus.hpp>
#include <sstream>
#include <fstream>

#define __STDC_CONSTANT_MACROS
#include <boost/date_time/posix_time/posix_time.hpp>

StatUniqueCounter<unsigned long> overallUniqueAddress("overall.UniqueAddresses");

StatCounter overallTSEForwards("overall.TSEForwards");
StatCounter overallTSEDuplicateForwards("overall.TSEDupForwards");
StatCounter overallTSEWatchConvertForwards("overall.TSEWatchConvertForwards");

StatCounter overallSGPForwards("overall.SGPForwards");
StatCounter overallSGPDuplicateForwards("overall.SGPDupForwards");

StatCounter overallConsumptions("overall.Consumptions");
StatCounter overallConsumptions_User("overall.ConsumptionsUser");
StatCounter overallConsumptions_System("overall.ConsumptionsSystem");
StatCounter overallHits("overall.Hits");
StatCounter overallHits_User("overall.HitsUser");
StatCounter overallHits_System("overall.HitsSystem");
StatCounter overallTSEHits_SamePC("overall.TSEHits_SamePC");
StatCounter overallTSEHits_DiffPC("overall.TSEHits_DiffPC");

StatCounter overallTSEHits("overall.TSEHits");
StatCounter overallTSEHits_User("overall.TSEHits_User");
StatCounter overallTSEHits_System("overall.TSEHits_System");
StatCounter overallTSEWatchMatches("overall.TSEWatchMatches");

StatCounter overallTSEDiscards_Replace("overall.TSEDiscards_Replace");
StatCounter overallTSEDiscards_Invalidate("overall.TSEDiscards_Invalidate");

StatCounter overallSGPHits("overall.SGPHits");
StatCounter overallSGPHits_User("overall.SGPHits_User");
StatCounter overallSGPHits_System("overall.SGPHits_System");

StatCounter overallSGPHeads("overall.SGPHeads");
StatCounter overallSGPSparse("overall.SGPSparse");

StatCounter overallSGPDiscards_Invalidate("overall.SGPDiscards_Invalidate");
StatCounter overallSGPDiscards_Replace("overall.SGPDiscards_Replace");

StatMax overallSGP_PeakBufferSize("overall.SGP_PeakBufferSize");

#ifdef ENABLE_ALTERNATIVE_SEARCH
  StatCounter overallAltCacheHits("overall.AltCacheHits");
  StatCounter overallAltCacheHits_PCMatch("overall.AltCacheHits_PCMatch");
  StatCounter overallAltCacheHits_PCMismatch("overall.AltCacheHits_PCMismatch");
#endif //ENABLE_ALTERNATIVE_SEARCH


StatCounter overallMissNoStream("overall.MissNoStream");
StatCounter overallMissNoStreamSGPHit("overall.MissNoStreamSGPHit");
StatCounter overallMissCreateStream("overall.MissCreateStream");

StatCounter overall_Joint_SH_TH("overall.Joint_SH_TH");
StatCounter overall_Joint_SE_TH("overall.Joint_SE_TH");
StatCounter overall_Joint_SS_TH("overall.Joint_SS_TH");
StatCounter overall_Joint_SM_TH("overall.Joint_SM_TH");

StatCounter overall_Joint_SH_TW("overall.Joint_SH_TW");
StatCounter overall_Joint_SE_TW("overall.Joint_SE_TW");
StatCounter overall_Joint_SS_TW("overall.Joint_SS_TW");
StatCounter overall_Joint_SM_TW("overall.Joint_SM_TW");

StatCounter overall_Joint_SH_TM("overall.Joint_SH_TM");
StatCounter overall_Joint_SE_TM("overall.Joint_SE_TM");
StatCounter overall_Joint_SS_TM("overall.Joint_SS_TM");
StatCounter overall_Joint_SM_TM("overall.Joint_SM_TM");

StatCounter overall_Joint_SH_TU("overall.Joint_SH_TU");
StatCounter overall_Joint_SE_TU("overall.Joint_SE_TU");
StatCounter overall_Joint_SS_TU("overall.Joint_SS_TU");
StatCounter overall_Joint_SM_TU("overall.Joint_SM_TU");

StatCounter overall_Joint_sum("overall.Joint_sum");

StatCounter * overall_Joint[4][4]  =
  { { &overall_Joint_SH_TH, &overall_Joint_SH_TW, &overall_Joint_SH_TM, &overall_Joint_SH_TU}
  , { &overall_Joint_SE_TH, &overall_Joint_SE_TW, &overall_Joint_SE_TM, &overall_Joint_SE_TU}
  , { &overall_Joint_SS_TH, &overall_Joint_SS_TW, &overall_Joint_SS_TM, &overall_Joint_SS_TU}
  , { &overall_Joint_SM_TH, &overall_Joint_SM_TW, &overall_Joint_SM_TM, &overall_Joint_SM_TU}
  };

StatLog2Histogram overallSGPHitDistance("overall.SGPHitDistance");

MultiCache::MultiCache(std::string aName, int anId, int aNumNodes, unsigned long aCapacity, unsigned long anSGPCapacity, unsigned long aBlockSize, Predicate anInvalidatePredicate, Predicate aConsumePredicate, MultiStreamRequestFn aStreamRequestFn, Directory* aDirectory, bool aNoStreamOnSGPHit)
 : theName(aName)
 , theId(anId)
 , theNextStreamId(0)
#ifdef ENABLE_SPATIAL_GROUPS
 , theCapacity(aCapacity * (SPATIAL_GROUP_SIZE / aBlockSize) )
#else //!ENABLE_SPATIAL_GROUPS
 , theCapacity(aCapacity)
#endif //ENABLE_SPATIAL_GROUPS
 , theSGPCapacity(anSGPCapacity)
 , theSGPCacheSize_Normal(0)
 , theBlockSize(aBlockSize)
 , theBlockAddressMask( ~(theBlockSize - 1UL) )
 , theInvalidatePredicate(anInvalidatePredicate)
 , theConsumePredicate(aConsumePredicate)
 , theStreamRequestFn(aStreamRequestFn)
 , theDirectory(aDirectory)
 , theNoStreamOnSGPHit(aNoStreamOnSGPHit)
 , theTSEForwards( aName + ".TSEForwards")
 , theTSEDuplicateForwards( aName + ".TSEDupForwards")
 , theSGPForwards( aName + ".SGPForwards")
 , theSGPDuplicateForwards( aName + ".SGPDupForwards")
 , theConsumptions( aName + ".Consumptions")
 , theConsumptions_User( aName + ".ConsumptionsUser")
 , theConsumptions_System( aName + ".ConsumptionsSystem")
 , theHits( aName + ".Hits")
 , theHits_User( aName + ".HitsUser")
 , theHits_System( aName + ".HitsSystem")
 , theTSEHits( aName + ".TSEHits")
 , theTSEHits_User( aName + ".TSEHitsUser")
 , theTSEHits_System( aName + ".TSEHitsSystem")
 , theTSEHits_SamePC( aName + ".TSEHitsSamePC")
 , theTSEHits_DiffPC( aName + ".TSEHitsDiffPC")
 , theSGPHits( aName + ".SGPHits")
 , theSGPHits_User( aName + ".SGPHitsUser")
 , theSGPHits_System( aName + ".SGPHitsSystem")
 , theSGPHeads( aName + ".SGPHeads")
 , theSGPSparse( aName + ".SGPSparse")
#ifdef ENABLE_ALTERNATIVE_SEARCH
 , theAltCacheHits( aName + ".AltCacheHits")
#endif //ENABLE_ALTERNATIVE_SEARCH
 , theMisses_CreateStream( aName + ".MissesCreateStream")
 , theMisses_NoStream( aName + ".MissesNoStream")
 , theMisses_NoStreamSGPHit( aName + ".MissesNoStreamSGPHit")
 {
   // nothing at the moment
 }

MultiCache::~MultiCache() {
  boost::posix_time::ptime now(boost::posix_time::second_clock::local_time());
  DBG_(Dev, ( << "Destructing multi cache " << theName << " at " << now ) );
}

void MultiCache::finalize() {
  DBG_(Dev, ( << "Finalizing list cache " << theName ) );
  theCache.clear();
  theSGPCache.clear();
  theAlternateCache.clear();

}

void MultiCache::clearAlt( ) {
  DBG_(Verb, ( << name() << " clearing alt cache." ) );
  theAlternateCache.clear();
}
void MultiCache::forwardAlt( tAddress anAddress, bool aHeadPCMatch ) {
  theAlternateCache[anAddress] |= aHeadPCMatch;
}
int MultiCache::altSize() {
  return theAlternateCache.size();
}


bool MultiCache::forwardTSE( tAddress anAddress, tVAddress aPC, boost::shared_ptr<MultiStream> aStream, eForwardType aFwdType) {
  anAddress &= theBlockAddressMask;

  ++theTSEForwards; ++overallTSEForwards;

  cache_table::index<by_address>::type::iterator addr_iter = theCache.get<by_address>().find(anAddress);
  if (addr_iter != theCache.get<by_address>().end()) {
    #ifdef ENABLE_SPATIAL_GROUPS
      theCache.get<by_LRU>().relocate(theCache.get<by_LRU>().end(), theCache.project<by_LRU>(addr_iter) );
    #endif //ENABLE_SPATIAL_GROUPS
    if (aFwdType < eFwdWatch_Hot_1 && addr_iter->theFwdType >= eFwdWatch_Hot_1) {
      addr_iter->theFwdType = aFwdType;
      addr_iter->theStream = aStream;
      //theCache.get<by_LRU>().relocate(theCache.get<by_LRU>().end(), theCache.project<by_LRU>(addr_iter) );
      ++overallTSEWatchConvertForwards;
      return true;
    } else {
      ++theTSEDuplicateForwards; ++overallTSEDuplicateForwards;
      return false;
    }
  }


  if (theCache.size() >= theCapacity) {
    theCache.get<by_LRU>().front().replace();
    theCache.get<by_LRU>().pop_front();

    ++overallTSEDiscards_Replace;
  }

  theCache.get<by_LRU>().push_back(MultiCacheEntry(anAddress, aPC, aStream, aFwdType));

  return (aFwdType < eFwdWatch_Hot_1);
}

void MultiCache::forwardSGP( tAddress anAddress, eSGPType aType, unsigned long anId) {
  anAddress &= theBlockAddressMask;

  sgp_cache_table::iterator iter;
  bool is_new;

    ++theSGPForwards; ++overallSGPForwards;

  //SGP Capacity
  while (!theSGPCache.empty() && aType == eNormal && theSGPCapacity > 0 && theSGPCacheSize_Normal >= theSGPCapacity ) {
    if ( theSGPCache.get<by_LRU>().front().theType == eNormal) {
      ++overallSGPDiscards_Replace;
      --theSGPCacheSize_Normal;
    }
    theSGPCache.get<by_LRU>().pop_front();
  }

  boost::tie(iter, is_new) = theSGPCache.insert( SGPCacheEntry(anAddress, aType, theAccessCount, anId) );
  if (! is_new && aType == eNormal) {
    if (iter->theType != eNormal) {
      iter->theType = eNormal;
      iter->theId = anId;

      ++theSGPCacheSize_Normal;
      iter->theAccessCount = theAccessCount;
    } else {
      ++theSGPDuplicateForwards; ++overallSGPDuplicateForwards;
      iter->theAccessCount = theAccessCount;
    }
  } else if (aType == eNormal) {
    ++theSGPCacheSize_Normal;
  }

  overallSGP_PeakBufferSize << theSGPCache.size();

  DBG_(Iface, ( << theName << " SGP-forward: addr=" << std::hex << anAddress << std::dec ) );
}

void MultiCache::event( TraceData & anEvent) {
  if (theInvalidatePredicate(anEvent)) {
    invalidate(anEvent.theAddress);
  }
  if (theConsumePredicate(anEvent)) {
    consume(anEvent.theAddress, anEvent.thePC, anEvent.theFillType, anEvent.theFillLevel, anEvent.theOS);
  }
}

void MultiCache::invalidate( tAddress anAddress) {
  anAddress &= theBlockAddressMask;
  cache_table::index<by_address>::type::iterator iter = theCache.get<by_address>().find(anAddress);
  if (iter != theCache.get<by_address>().end()) {
    DBG_(Trace, ( << theName << ": Invalidate-TSE @" << std::hex << anAddress << std::dec ) );

    iter->invalidate();
    ++overallTSEDiscards_Invalidate;
    theCache.get<by_address>().erase(iter);
  }

  sgp_cache_table::iterator sgp_iter = theSGPCache.find(anAddress);
  if (sgp_iter != theSGPCache.end()) {
    DBG_(Trace, ( << theName << ": Invalidate-SGP @" << std::hex << anAddress << std::dec ) );
    if (sgp_iter->theType == eNormal) {
      ++overallSGPDiscards_Invalidate;
      --theSGPCacheSize_Normal;
    }
    theSGPCache.erase(sgp_iter);
  }

  #ifdef ENABLE_ALTERNATIVE_SEARCH
    theAlternateCache.erase(anAddress);
  #endif //ENABLE_ALTERNATIVE_SEARCH
}

std::pair<eSGPOutcome, long> MultiCache::SGP_consume( tAddress anAddress, tVAddress aPC, bool anOS) {
  eSGPOutcome outcome = eSGP_Miss;
  long sgp_stream = 0;
  tAddress blockAddr = anAddress & theBlockAddressMask;
  overallUniqueAddress << blockAddr;

  sgp_cache_table::index<by_address>::type::iterator  iter = theSGPCache.find(blockAddr);


  if (iter != theSGPCache.end()) {
    if (iter->theType == eNormal) {
      DBG_( Trace, ( << theName << ": Consume @" << std::hex << blockAddr << std::dec << " SGP-Hit"));

      //Count the Hits
      ++theSGPHits; ++overallSGPHits;
      if (anOS) {
        ++theSGPHits_System; ++overallSGPHits_System;
      } else {
        ++theSGPHits_User; ++overallSGPHits_User;
      }

      overallSGPHitDistance << (theAccessCount - iter->theAccessCount);

      outcome = eSGP_Hit;

      --theSGPCacheSize_Normal;
    } else if (iter->theType == eHead) {
      DBG_( Trace, ( << theName << ": Consume @" << std::hex << blockAddr << std::dec << " SGP-Head"));
      ++theSGPHeads; ++overallSGPHeads;
      outcome = eSGP_Head;
    } else if (iter->theType == eSparse) {
      DBG_( Trace, ( << theName << ": Consume @" << std::hex << blockAddr << std::dec << " SGP-Sparse"));
      ++theSGPSparse; ++overallSGPSparse;
      outcome = eSGP_Sparse;
    }
    sgp_stream = iter->theId;

    theSGPCache.erase(iter);
  } else {
    DBG_( Trace, ( << theName << ": Consume @" << std::hex << blockAddr << std::dec << " SGP-Miss"));
    outcome = eSGP_Miss;
  }

  ++theAccessCount;

  return std::make_pair(outcome, sgp_stream);
}

std::pair<eTSEOutcome, long> MultiCache::TSE_consume( tAddress anAddress, tVAddress aPC, bool anOS, eSGPOutcome anSGPOutcome ) {
  eTSEOutcome outcome = eTSE_Miss;
  long stream_id = 0;
  tAddress blockAddr = anAddress & theBlockAddressMask;

  cache_table::index<by_address>::type::iterator  iter = theCache.get<by_address>().find(blockAddr);
  if (iter != theCache.get<by_address>().end()) {
    DBG_( Trace, ( << theName << ": Consume @" << std::hex << anAddress << std::dec << " TSE-Hit"));

    //Extract stream info from the cache
    boost::shared_ptr<MultiStream> stream = iter->theStream;
    stream_id = iter->theStream->id();
    eForwardType fwd_type = iter->theFwdType;

    //Remove the entry from the cache
    theCache.get<by_address>().erase(iter);
    stream->notifyHit(anAddress, fwd_type);

    if (fwd_type >= eFwdWatch_Hot_1 ) {
      outcome = eTSE_Watch;

      ++overallTSEWatchMatches;
    } else {
      outcome = eTSE_Hit;

      if (iter->thePC == aPC) {
        ++theTSEHits_SamePC; ++overallTSEHits_SamePC;
      } else {
        ++theTSEHits_DiffPC; ++overallTSEHits_DiffPC;
      }

      //Count the Hits
      ++theTSEHits; ++overallTSEHits;
      if (anOS) {
        ++theTSEHits_System; ++overallTSEHits_System;
      } else {
        ++theTSEHits_User; ++overallTSEHits_User;
      }

    }


  } else {
    DBG_( Trace, ( << theName << ": Consume @" << std::hex << anAddress << std::dec << " TSE-Miss"));
    boost::shared_ptr<MultiStream> newStream;

    #ifdef ENABLE_ALTERNATIVE_SEARCH
      alt_cache_t::iterator iter = theAlternateCache.find(blockAddr);
      if (iter != theAlternateCache.end()) {
        ++theAltCacheHits; ++overallAltCacheHits;
        if (iter->second) {
          DBG_( Trace, ( << "Miss @" << std::hex << blockAddr << std::dec << " alt-hit PC match" ) );
          ++overallAltCacheHits_PCMatch;
        } else {
          DBG_( Trace, ( << "Miss @" << std::hex << blockAddr << std::dec << " alt-hit PC mismatch" ) );
          ++overallAltCacheHits_PCMismatch;
        }
      }
    #endif //ENABLE_ALTERNATIVE_SEARCH

    newStream = theStreamRequestFn(this, anAddress, aPC);
    if(newStream) {

        //DBG_(Dev, ( << "stream created on miss address " << std::hex << anAddress << " from node " << std::dec << newStream->id(0) ) );
        if (theNoStreamOnSGPHit && anSGPOutcome == eSGP_Hit ) {
          ++theMisses_NoStreamSGPHit; ++overallMissNoStreamSGPHit;
        } else {
          ++theMisses_CreateStream; ++overallMissCreateStream;
          newStream->beginForward();
        }
        outcome = eTSE_Miss;
        stream_id = newStream->id();
    } else {

        ++theMisses_NoStream; ++overallMissNoStream;
        outcome = eTSE_Unavoidable;
    }
  }

  #ifdef ENABLE_ALTERNATIVE_SEARCH
    theAlternateCache.erase(blockAddr);
  #endif //ENABLE_ALTERNATIVE_SEARCH

  return std::make_pair(outcome, stream_id);
}

void MultiCache::stats( eTSEOutcome tse, eSGPOutcome sgp ) {
  ++(*overall_Joint[sgp][tse]);
  ++overall_Joint_sum;
}

void MultiCache::consume( tAddress anAddress, tVAddress aPC, tFillType aFillType, tFillLevel aFillLevel, bool anOS) {
  eTSEOutcome tse_outcome = eTSE_Miss;
  eSGPOutcome sgp_outcome = eSGP_Miss;
  long tse_stream = 0, sgp_stream = 0;

  //Count consumption
  ++theConsumptions; ++overallConsumptions;
  if (anOS) {
    ++theConsumptions_System; ++overallConsumptions_System;
  } else {
    ++theConsumptions_User; ++overallConsumptions_User;
  }

  boost::tie(sgp_outcome, sgp_stream) = SGP_consume( anAddress, aPC, anOS );
  boost::tie(tse_outcome, tse_stream) = TSE_consume( anAddress, aPC, anOS, sgp_outcome );

  stats(tse_outcome, sgp_outcome);

  if ( tse_outcome == eTSE_Hit || sgp_outcome == eSGP_Hit) {
    //Count the Hits
    ++theHits; ++overallHits;
    if (anOS) {
      ++theHits_System; ++overallHits_System;
    } else {
      ++theHits_User; ++overallHits_User;
    }
  }

  #ifdef ENABLE_TRACE
    trace::MissRecord aRecord;

    aRecord.aCycle = Flexus::Core::theFlexus->cycleCount();
    aRecord.aNode = theId;
    aRecord.anAddress = anAddress;
    aRecord.aFillType = aFillType;
    aRecord.aFillLevel = aFillLevel;
    aRecord.aPC = aPC;
    aRecord.aTSE = tse_outcome;
    aRecord.aTSEStream = tse_stream;
    aRecord.anSGP = sgp_outcome;
    aRecord.anSGPStream = sgp_stream;
    aRecord.aBits = 0;
    if (anOS) {
      aRecord.aBits |= kBITS_OS;
    }
    #ifdef ENABLE_WHITE_BOX
      nWhiteBox::CPUState state;
      nWhiteBox::WhiteBox::getWhiteBox()->getState(theId, state);
      aRecord.aThread = state.theThread;
      aRecord.aCurrentTrap = state.theTrap;
      std::list<unsigned long long>::iterator bt_iter = state.theBackTrace.begin();
      std::list<unsigned long long>::iterator bt_end = state.theBackTrace.end();
      for (int i = 0; i< 16; ++i) {
        if (bt_iter == bt_end) {
          aRecord.aStackTrace[i] = 0;
        } else {
          aRecord.aStackTrace[i] = *bt_iter;
          ++bt_iter;
        }
      }
    #endif //ENABLE_WHITE_BOX
    trace::addMiss(aRecord);
  #endif //ENABLE_TRACE

  #ifdef ENABLE_SEQUITUR
    sequitur::addSymbol(anAddress, id() );
  #endif //ENABLE_SEQUITUR
}
