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

#include "common.hpp"
#include "sequitur.hpp"
#include "trace.hpp"


#define DBG_DefineCategories ExperimentDbg, ReaderDbg, IgnoreDbg
#define DBG_SetInitialGlobalMinSev Iface
#include DBG_Control()

#include <fstream>

#include <boost/shared_ptr.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace l = boost::lambda;

#include <boost/utility.hpp>

#define __STDC_CONSTANT_MACROS
#include <boost/date_time/posix_time/posix_time.hpp>


#include "coordinator.hpp"
#include "ordergroupmgr.hpp"
#include "spatialpredictor.hpp"


#ifdef ENABLE_COMBO
  StatCounter comboConsumptions("Combo.Consumptions");
  StatCounter comboHitAll("Combo.Hit_All");
  StatCounter comboHit0("Combo.Hit_M");
  StatCounter comboHit1("Combo.Hit_2");
  StatCounter comboHit2("Combo.Hit_L");
  StatCounter comboHit01("Combo.Hit_M2");
  StatCounter comboHit12("Combo.Hit_2L");
  StatCounter comboHit02("Combo.Hit_ML");
  StatCounter comboHitNone("Combo.Hit_None");

  StatCounter comboAllDiff("ComboBreakdown.All_diff");
  StatCounter comboAllMeqL("ComboBreakdown.All_MeqL");
  StatCounter comboAllMeq2("ComboBreakdown.All_Meq2");
  StatCounter comboAll2eqL("ComboBreakdown.All_2eqL");
  StatCounter comboAllSame("ComboBreakdown.All_same");
  StatCounter combo01Diff("ComboBreakdown.M2_diff");
  StatCounter combo12Diff("ComboBreakdown.2L_diff");
  StatCounter combo02Diff("ComboBreakdown.ML_diff");
#endif //ENABLE_COMBO

std::ostream & operator << (std::ostream & str, TraceData const & aTraceData) {
  str << "[" << fill<2>(aTraceData.theNode) << "]";
  switch (aTraceData.theEventType) {
    case eProduction:
      str << " P @";
      break;
    case eConsumption:
      str << " C @";
      break;
    case eUpgrade:
      str << " U @";
      break;
    case eAccess:
      str << " A @";
      break;
    case eEviction:
      str << " E @";
      break;
  }
  str << std::hex << aTraceData.theAddress << std::dec ;
#ifdef ENABLE_ENTRY_STATE
  str << " /" << std::hex << std::setw(4) << std::setfill('0') << aTraceData.theBitVector << "/" << std::dec;
#endif //ENABLE_ENTRY_STATE

  return str;
}

tCoordinator::tCoordinator( unsigned long long aStopAt, long aNumNodes, int aNumExperiments, long aPageSize )
 : theNumExperiments(aNumExperiments)
{
  thePageMap = new PageMap(aNumNodes, aPageSize);
  theOrderGroups.resize(theNumExperiments);
  theCaches.resize(theNumExperiments);

  #ifdef ENABLE_SEQUITUR
    sequitur::initialize(aNumNodes);
  #endif
  #ifdef ENABLE_TRACE
    trace::initialize();
  #endif
}

tCoordinator::~tCoordinator() {}


#ifdef ENABLE_COMBO
#define match(a,b) ((hitIds[a]==hitIds[b]) && (hitSeqNos[a]==hitSeqNos[b]))
#endif //ENABLE_COMBO

void tCoordinator::access( tID aNode, tAddress anAddress, tVAddress aPC, bool anOS) {
  if (theSpatialPredictors.size() > aNode) {
    TraceData evt(eAccess, aNode, anAddress, aPC, eCold, eLocalMem, anOS );
    theSpatialPredictors[aNode]->access(evt);
  }
}

void tCoordinator::eviction( tID aNode, tAddress anAddress) {
  if (theSpatialPredictors.size() > aNode) {
    TraceData evt(eEviction, aNode, anAddress, 0, eCold, eLocalMem, 0);
    theSpatialPredictors[aNode]->eviction(evt);
  }
}

void tCoordinator::consumption( tID aNode, tAddress anAddress, tVAddress aPC, tFillType aFillType, tFillLevel aFillLevel, bool anOS) {
  TraceData evt(eConsumption, aNode, anAddress, aPC, aFillType, aFillLevel, anOS );

  //Process through the appropriate cache
  theCaches[0][aNode]->event(evt);

  //Process through the order group managers
    std::for_each
      ( theOrderGroups[0].begin()
      , theOrderGroups[0].end()
      , l::bind(&TraceProcessor::event, l::_1,  l::var(evt))
      );

}

void tCoordinator::upgrade( tID aNode, tAddress anAddress, tVAddress aPC, bool anOS) {
  TraceData evt(eUpgrade, aNode, anAddress, aPC, eCold, eLocalMem, anOS );

  //Process the upgrade through all caches
    std::for_each
      ( theCaches[0].begin()
      , theCaches[0].end()
      , l::bind(&TraceProcessor::event, l::_1,  l::var(evt))
      );

}

void tCoordinator::event( tEventType anEventType, tID aNode, tAddress anAddress, tMRPVector aBitVector, tTime aTimestamp, tVAddress aPC, bool anOS) {

  //Pass the event to the instance manager to get back the instance number for this name
  long instance = 0;
  tID producer = 0;

  #ifdef ENABLE_INSTANCE
    boost::tie(instance, producer) = theInstanceManager.getInstance( anEventType, aNode, anAddress, aBitVector);
  #endif

  TraceData evt(anEventType, aNode, producer, 0, anAddress, instance, aBitVector, aTimestamp, aPC, anOS );

  //Efficiency hack for spatial predictor events

  DBG_( Iface, ( << evt ));
  //std::cout << "event: " << evt << "\n";

  int ii;
#ifdef ENABLE_COMBO
  bool hits[theNumExperiments];
  tID hitIds[theNumExperiments];
  long long hitSeqNos[theNumExperiments];
#endif //ENABLE_COMBO

  for(ii = 0; ii < theNumExperiments; ii++) {
#ifdef ENABLE_ENTRY_STATE
    evt.theWasConsumed = false;
    evt.theWasHit = false;
#endif //ENABLE_ENTRY_STATE



    //std::cout << "before caches - event: " << evt << "\n";
    //Process the event through the caches
    std::for_each
      ( theCaches[ii].begin()
      , theCaches[ii].end()
      , l::bind(&TraceProcessor::event, l::_1,  l::var(evt))
      );

    //std::cout << "between - event: " << evt << "\n";
    //Add the event to the orders
    std::for_each
      ( theOrderGroups[ii].begin()
      , theOrderGroups[ii].end()
      , l::bind(&TraceProcessor::event, l::_1,  l::var(evt))
      );


    //std::cout << "after order groups - event: " << evt << "\n";
    //Remember stats if this was a consumption
#ifdef ENABLE_COMBO
    hits[ii] = false;
    if(evt.theWasConsumed) {
      if(ii == 0) {
        ++comboConsumptions;
      }
      hits[ii] = evt.theWasHit;
      hitIds[ii] = evt.theHitId;
      hitSeqNos[ii] = evt.theHitSeqNo;
    }
#endif //ENABLE_COMBO

  }

#ifdef ENABLE_COMBO
  //Calculate combo stats
  if(evt.theWasConsumed) {
    if(theNumExperiments == 2) {

      if(hits[0] && hits[1]) {
        ++comboHitAll;
      } else if(hits[0]) {
        ++comboHit0;
      } else if(hits[1]) {
        ++comboHit2;
      } else {
        ++comboHitNone;
      }

    } else if(theNumExperiments == 3) {

      if(hits[0] && hits[1] && hits[2]) {
        ++comboHitAll;
        if( match(1,2) && match(0,2) ) {
          ++comboAllSame;
        } else if( match(0,2) ) {
          ++comboAllMeqL;
        } else if( match(1,2) ) {
          ++comboAll2eqL;
        } else if( match(0,1) ) {
          ++comboAllMeq2;
        } else {
          ++comboAllDiff;
        }
      } else if(hits[0] && hits[1]) {
        ++comboHit01;
        if( !match(0,1) ) {
          ++combo01Diff;
        }
      } else if(hits[1] && hits[2]) {
        ++comboHit12;
        if( !match(1,2) ) {
          ++combo12Diff;
        }
      } else if(hits[0] && hits[2]) {
        ++comboHit02;
        if( !match(0,2) ) {
          ++combo02Diff;
        }
      } else if(hits[0]) {
        ++comboHit0;
      } else if(hits[1]) {
        ++comboHit1;
      } else if(hits[2]) {
        ++comboHit2;
      } else {
        ++comboHitNone;
      }

    }

  }  // wasConsumed
#endif //ENABLE_COMBO

}

void tCoordinator::addOrderGroup(OrderGroupMgr * anOrdering, int anExperiment) {
  theOrderGroups[anExperiment].push_back(anOrdering);
}

void tCoordinator::addSpatialPredictor(SpatialPredictor * aSpatialPredictor) {
  theSpatialPredictors.push_back(aSpatialPredictor);
}

void tCoordinator::addCache(TraceProcessor * aCache, int anExperiment) {
  theCaches[anExperiment].push_back(aCache);
}

void tCoordinator::finalize() {
  boost::posix_time::ptime now(boost::posix_time::second_clock::local_time());
  DBG_(Dev, Cat(ExperimentDbg) ( << "Finalizing state. " << boost::posix_time::to_simple_string(now)));
  unsigned int ii, jj;
  for(jj = 0; jj < theOrderGroups.size(); jj++) {
    for(ii = 0; ii < theOrderGroups[jj].size(); ii++) {
      theOrderGroups[jj][ii]->finalize();
    }
  }
  for(jj = 0; jj < theCaches.size(); jj++) {
    for(ii = 0; ii < theCaches[jj].size(); ii++) {
      theCaches[jj][ii]->finalize();
    }
  }
  if (theStreamTracker) {
    theStreamTracker->finalize();
  }

  #ifdef ENABLE_SEQUITUR
    sequitur::printRules();
  #endif
  #ifdef ENABLE_TRACE
    trace::flushFiles();
  #endif
}
