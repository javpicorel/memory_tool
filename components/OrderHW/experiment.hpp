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


#include <boost/shared_ptr.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace l = boost::lambda;

#include <boost/none.hpp>

#include <boost/utility.hpp>

#include <core/boost_extensions/padded_string_cast.hpp>


#include "ordermgr.hpp"
#include "ordergroupmgr.hpp"
#include "pull_stream.hpp"
#include "multi_stream.hpp"
#include "stride_stream.hpp"
#include "list_cache.hpp"
#include "multi_cache.hpp"
#include "coordinator.hpp"
#include "spatialpredictor.hpp"

#include "predicates.hpp"


struct ExperimentBase : public tCoordinator {
  bool theWatchBlock;
  long theInitialForwardChunk;
  long theInitialBackwardChunk;
  long theBodyForwardChunk;
  long theMaxStreams;
  long theStreamMaxBlocks;
  long theStreamWindow;
  bool theAggressive;

  virtual std::string description() = 0;

  ExperimentBase(long aNumNodes, int aNumExperiments, long aPageSize, bool aWatchBlock, long anInitialForwardChunk, long anInitialBackwardChunk, long aBodyForwardChunk, long aMaxStreams, long aStreamMaxBlocks, long aStreamWindow, bool anAggressive, unsigned long long aStopCycle = 0)
    : tCoordinator(aStopCycle, aNumNodes, aNumExperiments, aPageSize)
    , theWatchBlock(aWatchBlock)
    , theInitialForwardChunk(anInitialForwardChunk)
    , theInitialBackwardChunk(anInitialBackwardChunk)
    , theBodyForwardChunk(aBodyForwardChunk)
    , theMaxStreams(aMaxStreams)
    , theStreamMaxBlocks(aStreamMaxBlocks)
    , theStreamWindow(aStreamWindow)
    , theAggressive(anAggressive)
  {}

  void createListCaches(int kNumNodes, long aCacheCapacity, long aBlockSize, long aMaxStreams, long anIntersectDist, PullStreamRequestFn aStreamRequestFn, int anExperiment = 0 ) {
    //Create caches
    for (int i = 0;  i < kNumNodes; ++i) {
      std::string name( std::string("C[") + fill<2>(i) + "]" );
      if(anExperiment > 0) {
        name = std::string("C" + fill<1>(anExperiment) + "[") + fill<2>(i) + "]";
      }

      addCache
        ( new ListCache
          ( name
          , i
          , kNumNodes
          , aCacheCapacity
          , aBlockSize
          , aMaxStreams
          , anIntersectDist
          , l::bind( &isType, eUpgrade, l::_1)
          , l::bind( &isType, eConsumption, l::_1) && l::bind( &isNode, i, l::_1)
          , aStreamRequestFn
          , this->getDirectory()
          )
        , anExperiment
        );
    }
  }

  void createListCaches(int kNumNodes, long aCacheCapacity, long aBlockSize, long aMaxStreams, long anIntersectDist, std::vector<PullStreamRequestFn> & aStreamRequestFns, int anExperiment = 0 ) {
    //Create caches
    for (int i = 0;  i < kNumNodes; ++i) {
      std::string name( std::string("C[") + fill<2>(i) + "]" );
      if(anExperiment > 0) {
        name = std::string("C" + fill<1>(anExperiment) + "[") + fill<2>(i) + "]";
      }

      addCache
        ( new ListCache
          ( name
          , i
          , kNumNodes
          , aCacheCapacity
          , aBlockSize
          , aMaxStreams
          , anIntersectDist
          , l::bind( &isType, eUpgrade, l::_1)
          , l::bind( &isType, eConsumption, l::_1) && l::bind( &isNode, i, l::_1)
          , aStreamRequestFns[i]
          , this->getDirectory()
          )
        , anExperiment
        );
    }
  }

  void createMultiCaches(int kNumNodes, long aCacheCapacity, long anSGPCacheCapacity, long aBlockSize, MultiStreamRequestFn aStreamRequestFn, bool aNoStreamOnSGPHit ) {
    //Create caches
    for (int i = 0;  i < kNumNodes; ++i) {
      std::string name( std::string("C[") + fill<2>(i) + "]" );

      MultiCache * cache = new MultiCache
          ( name
          , i
          , kNumNodes
          , aCacheCapacity
          , anSGPCacheCapacity
          , aBlockSize
          , l::bind( &isType, eUpgrade, l::_1)
          , l::bind( &isType, eConsumption, l::_1) && l::bind( &isNode, i, l::_1)
          , aStreamRequestFn
          , this->getDirectory()
          , aNoStreamOnSGPHit
          );

      addCache(cache, 0);
      if ( theSpatialPredictors.size() > i) {
        theSpatialPredictors[i]->setCache(cache);
      }

    }
  }


  boost::shared_ptr<SuperStream> startPullStream( boost::shared_ptr<PullStream> aStream) {
    if (aStream) {
      aStream->setInitBackwardSize(theInitialBackwardChunk);
      aStream->setInitForwardSize(theInitialForwardChunk);
      aStream->setBodyForwardSize(theBodyForwardChunk);
      aStream->setMaxBlocks(theStreamMaxBlocks);
      aStream->setWindow(theStreamWindow);
      aStream->setAggressive(theAggressive);
    }
    return aStream;
  }

  boost::shared_ptr<MultiStream> startMultiStream( boost::shared_ptr<MultiStream> aStream) {
    if (aStream) {
      aStream->setTargetBlocks(theBodyForwardChunk);
    }
    return aStream;
  }

  boost::shared_ptr<SuperStream> startStrideStream(PrefetchReceiver * aReceiver, tAddress aRequestedAddress, tAddress aRequestPC) {
    boost::shared_ptr<StrideStream> newStream( new StrideStream(aReceiver,
                                                                aRequestedAddress,
                                                                aRequestPC,
                                                                theMaxStreams,
                                                                theStreamWindow,
                                                                theStreamMaxBlocks)
                                             );
    return newStream;
  }

};







struct TwoMRC: public ExperimentBase {

  std::string description() { return "Most and Second Most Recent Consumption"; }

  //Ordering Group
  //=========
  OrderGroupMgr * orderMostRecentConsumption;
  PullStreamRequestFn theStreamRequestFunction;
  boost::optional<std::string> theSaveState;

  TwoMRC(int aNumNodes, long aPageSize, long anOrderCapacity, long aCacheCapacity, unsigned long aBlockSize, long aMaxStreams, long anIntersectDist, long aStreamWindow, long aNumRecent, bool anAggressive, long anInitialForwardChunk, long anInitialBackwardChunk, long aBodyForwardChunk, long aStreamMaxBlocks, boost::optional<std::string> aLoadState, boost::optional<std::string> aSaveState, unsigned long long aStopCycle )
    : ExperimentBase( aNumNodes, 1, aPageSize, false /*aWatchBlock*/, anInitialForwardChunk, anInitialBackwardChunk, aBodyForwardChunk, aMaxStreams, aStreamMaxBlocks, aStreamWindow, anAggressive, aStopCycle )
   , theSaveState(aSaveState)
  {

    //Create the order manager
    //=========================
      orderMostRecentConsumption = new OrderGroupMgr
        ( "C"
        , anOrderCapacity
        , aBlockSize
        , false  /*anAllowLiveStreams*/
        , false  /*aKillOnConsumed*/
        , false  /*aKillOnInvalidate*/
        , aNumNodes
        , -1 /* group mgr will determine id's */
        , aNumRecent
        , 0  /* aDeltas */
        , l::bind( &isType, eConsumption, l::_1 )
        , l::bind( &isFalse, l::_1 )
        , l::bind( &node, l::_1 )
        , aLoadState
        , boost::none
        , getDirectory()
        );

      addOrderGroup(orderMostRecentConsumption, 0);
      theStreamRequestFunction =
        l::bind
          ( &ExperimentBase::startPullStream
          , this
          , l::bind
              ( &OrderGroupMgr::pullRequest
              , orderMostRecentConsumption
              , l::_1
              , l::_2
              , l::_3
              )
          );

    //Create Caches
    //=============
    createListCaches( aNumNodes, aCacheCapacity, aBlockSize, aMaxStreams, anIntersectDist, theStreamRequestFunction);

  }

  virtual void finalize() {
    #ifndef ENABLE_PC_LOOKUP
      if ( theSaveState ) {
        //Save the state of the consort, mrc-reflector.
        orderMostRecentConsumption->save( *theSaveState, this->getDirectory() );
      }
    #endif //!ENABLE_PC_LOOKUP

    //Delegate to base implementation
    tCoordinator::finalize();
  }

};

struct TwoLORDS: public ExperimentBase {

  std::string description() { return "Two Most Recent Consumptions on Own Order"; }

  //Ordering Group
  //=========
  std::vector<OrderGroupMgr *> orderConsumptions;
  std::vector<PullStreamRequestFn> theStreamRequestFunctions;

  TwoLORDS(int aNumNodes, long aPageSize, long anOrderCapacity, long aCacheCapacity, unsigned long aBlockSize, long aMaxStreams, long anIntersectDist, long aStreamWindow, long aNumRecent, bool anAggressive, long anInitialForwardChunk, long anInitialBackwardChunk, long aBodyForwardChunk, long aStreamMaxBlocks, boost::optional<std::string> aLoadState )
    : ExperimentBase( aNumNodes, 1, aPageSize, false /*aWatchBlock*/, anInitialForwardChunk, anInitialBackwardChunk, aBodyForwardChunk, aMaxStreams, aStreamMaxBlocks, aStreamWindow, anAggressive )
  {

    //Create the order manager
    //=========================
    for (int i = 0; i < aNumNodes; ++i) {
      OrderGroupMgr * mgr = new OrderGroupMgr
        ( "self"
        , anOrderCapacity
        , aBlockSize
        , false  /*anAllowLiveStreams*/
        , false  /*aKillOnConsumed*/
        , false  /*aKillOnInvalidate*/
        , 1
        , i
        , aNumRecent
        , 0  /* aDeltas */
        , l::bind( &isType, eConsumption, l::_1 ) && l::bind( &isNode, i, l::_1 )
        , l::bind( &isFalse, l::_1 )
        , l::constant( 0 )
        , boost::none
        , boost::none
        , getDirectory()
        );

      orderConsumptions.push_back(mgr);
      addOrderGroup(mgr, 0);
      theStreamRequestFunctions.push_back
        ( l::bind( &ExperimentBase::startPullStream, this, l::bind( &OrderGroupMgr::pullRequest, mgr, l::_1, l::_2, l::_3) ) );

    }

    //Create Caches
    //=============
    createListCaches( aNumNodes, aCacheCapacity, aBlockSize, aMaxStreams, anIntersectDist, theStreamRequestFunctions);

  }

};


#ifdef ENABLE_STRIDE
struct Stride: public ExperimentBase {

  std::string description() { return "Stride"; }

  PullStreamRequestFn theStreamRequestFunction;

  Stride(int aNumNodes, long aPageSize, long anOrderCapacity, long aCacheCapacity, unsigned long aBlockSize,long aMaxStreams, long aStreamWindow, long aStreamMaxBlocks )
   : ExperimentBase( aNumNodes, 1, aPageSize, false, 0, 0, 0, aMaxStreams, aStreamMaxBlocks, aStreamWindow, false )
  {

      theStreamRequestFunction =
        l::bind
          ( &ExperimentBase::startStrideStream
          , this
          , l::_1
          , l::_2
          , l::_3
          );

    //Create Caches
    //=============
    createListCaches( aNumNodes, aCacheCapacity, aBlockSize, aMaxStreams, 0, theStreamRequestFunction);

  }

};
#endif //ENABLE_STRIDE


struct GHB: public ExperimentBase {

  std::string description() { return "Global History Buffer G/DC"; }

  //Ordering Group
  //=========
  std::vector<OrderGroupMgr *> orderConsumptions;
  std::vector<PullStreamRequestFn> theStreamRequestFunctions;

  GHB(int aNumNodes, long aPageSize, long anOrderCapacity, long aCacheCapacity, unsigned long aBlockSize, long aMaxStreams, long anIntersectDist, long aStreamWindow, long aNumRecent, bool anAggressive, long anInitialForwardChunk, long anInitialBackwardChunk, long aBodyForwardChunk, long aStreamMaxBlocks, long aDeltas, boost::optional<std::string> aLoadState )
    : ExperimentBase( aNumNodes, 1, aPageSize, false /*aWatchBlock*/, anInitialForwardChunk, anInitialBackwardChunk, aBodyForwardChunk, aMaxStreams, aStreamMaxBlocks, aStreamWindow, anAggressive )
  {

    //Create the order manager
    //=========================
    for (int i = 0; i < aNumNodes; ++i) {
      OrderGroupMgr * mgr = new OrderGroupMgr
        ( "self"
        , anOrderCapacity
        , aBlockSize
        , false  /*anAllowLiveStreams*/
        , false  /*aKillOnConsumed*/
        , false  /*aKillOnInvalidate*/
        , 1
        , i
        , aNumRecent
        , aDeltas
        , l::bind( &isType, eConsumption, l::_1 ) && l::bind( &isNode, i, l::_1 )
        , l::bind( &isFalse, l::_1 )
        , l::constant( 0 )
        , boost::none
        , boost::none
        , getDirectory()
        );

      orderConsumptions.push_back(mgr);
      addOrderGroup(mgr, 0);
      theStreamRequestFunctions.push_back
        ( l::bind( &ExperimentBase::startPullStream, this, l::bind( &OrderGroupMgr::pullRequest, mgr, l::_1, l::_2, l::_3) ) );

    }

    //Create Caches
    //=============
    createListCaches( aNumNodes, aCacheCapacity, aBlockSize, aMaxStreams, anIntersectDist, theStreamRequestFunctions);

  }

};

struct ExhaustiveLookup: public ExperimentBase {

  std::string description() { return "Exhaustive lookup of consumption sequences"; }

  //Ordering Group
  //=========
  OrderGroupMgr * orderMostRecentConsumption;
  MultiStreamRequestFn theStreamRequestFunction;

  ExhaustiveLookup(int aNumNodes, long aPageSize, long anOrderCapacity, long aCacheCapacity, long anSGPCacheCapacity, long aBlockSize, long aBodyForwardChunk )
    : ExperimentBase( aNumNodes, 1, aPageSize, false /*aWatchBlock*/, 0, 0, aBodyForwardChunk, 0, 0, 0, false )
  {

    //MRC
    //=========================
    orderMostRecentConsumption = new OrderGroupMgr
      ( "C"
      , anOrderCapacity
      , aBlockSize
      , false
      , false
      , false
      , aNumNodes
      , -1 /* group mgr will determine id's */
#ifdef ENABLE_2MRC
      , 2  /* aNumRecent */
#else //!ENABLE_2MRC
      , 1  /* aNumRecent */
#endif //ENABLE_2MRC
      , 0  /* aDeltas */
      , l::bind( &isType, eConsumption, l::_1 )
      , l::bind( &isFalse, l::_1 )
      , l::bind( &node, l::_1 )
      , boost::none
      , boost::none
      , getDirectory()
      );
    addOrderGroup(orderMostRecentConsumption, 0);

    theStreamRequestFunction =
      l::bind( &ExperimentBase::startMultiStream, this, l::bind( &OrderGroupMgr::multiRequest, orderMostRecentConsumption, l::_1, l::_2, l::_3) ) ;

    //Create Caches
    //=============
    createMultiCaches( aNumNodes, aCacheCapacity, anSGPCacheCapacity, aBlockSize, theStreamRequestFunction, false);

  }

};

struct SpatioTemporal : public ExperimentBase {

  std::string description() { return "Spatial and Temporal Prediction"; }

  //Ordering Group
  //=========
  OrderGroupMgr * orderMostRecentConsumption;
  MultiStreamRequestFn theStreamRequestFunction;

  SpatioTemporal(int aNumNodes, long aPageSize, long anOrderCapacity, long aCacheCapacity, long anSGPCacheCapacity, long aBlockSize, long aSpatialGroupSize, long aBodyForwardChunk, bool aNoStreamOnSGPHit )
    : ExperimentBase( aNumNodes, 1, aPageSize, false /*aWatchBlock*/, 0, 0, aBodyForwardChunk, 0, 0, 0, false )
  {

    //MRC
    //=========================
    orderMostRecentConsumption = new OrderGroupMgr
      ( "C"
      , anOrderCapacity
      , aBlockSize
      , false
      , false
      , false
      , aNumNodes
      , -1 /* group mgr will determine id's */
#ifdef ENABLE_2MRC
      , 2  /* aNumRecent */
#else //!ENABLE_2MRC
      , 1  /* aNumRecent */
#endif //ENABLE_2MRC
      , 0  /* aDeltas */
      , l::bind( &isType, eConsumption, l::_1 )
      , l::bind( &isFalse, l::_1 )
      , l::bind( &node, l::_1 )
      , boost::none
      , boost::none
      , getDirectory()
      );
    addOrderGroup(orderMostRecentConsumption, 0);

    theStreamRequestFunction =
      l::bind( &ExperimentBase::startMultiStream, this, l::bind( &OrderGroupMgr::multiRequest, orderMostRecentConsumption, l::_1, l::_2, l::_3) ) ;

    for (int i = 0; i < aNumNodes; ++i) {
      SpatialPredictor * pred = new SpatialPredictor
        ( std::string("SGP") + boost::padded_string_cast<2,'0'>(i)
        , aBlockSize
        , aSpatialGroupSize
        );
      addSpatialPredictor(pred);
    }

    //Create Caches
    //=============
    createMultiCaches( aNumNodes, aCacheCapacity, anSGPCacheCapacity, aBlockSize, theStreamRequestFunction, aNoStreamOnSGPHit);

  }

};
