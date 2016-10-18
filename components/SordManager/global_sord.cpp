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

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace l = boost::lambda;

#include <fstream>

#include "sord.hpp"
#include "stream.hpp"


#include <components/MRP/MrpMain.hpp>

#define DBG_DefineCategories MRPTrace
#include DBG_Control()


namespace nSordManager {

BaseSORD::BaseSORD(int aNode, int aNumNodes, std::vector<ForwardQueue> & aFwdQueues, long aStorageLimit)
  : theNode(aNode)
  , theNumNodes(aNumNodes)
  , theForwardQueues(aFwdQueues)
  , theTotalTemplateLimit(aStorageLimit)
  , theTotalTemplateInUse(0)
  , theEntryIndex(0)
{ }

BaseSORD::~BaseSORD() {}

bool BaseSORD::frontInvalid(SORDList & aSORD) {
  if (! aSORD.empty()) {
     return ! aSORD.front().valid();
  } else {
     return false;
  }
}

GlobalSORD::GlobalSORD(int aNode, int aNumNodes, std::vector<ForwardQueue> & aForwardQueues, char aHeadChunkSize, char aBodyChunkSize, long aStorageLimit, GlobalSORDManager & aManager)
 : BaseSORD(aNode, aNumNodes, aForwardQueues, aStorageLimit)
 , theManager(aManager)
{
  for (int i = 0; i < theNumNodes; ++i) {
    theStreamManagers.push_back(new GlobalStreamManager( i, aNode, aHeadChunkSize, aBodyChunkSize, aForwardQueues[i] , *this) );
  }
}


GlobalSORD::~GlobalSORD() {}

SORDLocation GlobalSORD::begin(unsigned short aVector) {
  std::map< tMRPVector, SORDList>::iterator sord_iter = theSORDs.find(aVector);
  DBG_Assert( sord_iter != theSORDs.end());
  SORDList & sord = sord_iter->second;
  return sord.begin();
}

SORDLocation GlobalSORD::end(unsigned short aVector) {
  std::map< tMRPVector, SORDList>::iterator sord_iter = theSORDs.find(aVector);
  DBG_Assert( sord_iter != theSORDs.end());
  SORDList & sord = sord_iter->second;
  return sord.end();
}

SORDLocation GlobalSORD::append( tAddress anAddress, unsigned int anMRPVector ) {
  while (theTotalTemplateInUse >= theTotalTemplateLimit) {
    //Need to reclaim storage from a template.
    DBG_Assert(!theVictimVectors.empty());
    tMRPVector victim = theVictimVectors.front();
    theVictimVectors.pop_front();
    std::map< tMRPVector, SORDList>::iterator iter = theSORDs.find(victim);
    if (iter != theSORDs.end()) {

      if (iter->second.front().valid()) {
        theNodeStats[theNode].theOverwrittenTemplateBlocks++;
        theOverallStats.theOverwrittenTemplateBlocks++;
        //Inform the SORDManager that we are throwing this block away.
        theManager.forcedRecycle(iter->second.front().address());
        //Kill any streams pointing to the location.  Note that we do not steal
        //the block from any caches.
        for (int i = 0; i < 16; ++i) {
          theStreamManagers[i]->invalidate(iter->second.begin());
        }
        iter->second.front().invalidate();
      }
      if (! recycle(victim)) {
        theVictimVectors.push_back(victim);
      }
    } //Otherwise, we try again with the next victim
  }


  std::map< tMRPVector, SORDList>::iterator sord_iter;
  bool is_new;
  boost::tie( sord_iter, is_new) = theSORDs.insert( std::make_pair(anMRPVector, SORDList()) );
  SORDList & sord = sord_iter->second;

  theTotalTemplateInUse++;

  sord.push_back( SORDEntry(anAddress, theEntryIndex++, anMRPVector) );
  SORDLocation ret_val = --sord.end();
  if (is_new) {
    DBG_(Iface, ( << "P[" << theNode << "] SORD.Create " << *ret_val) );

    theVictimVectors.push_back(anMRPVector);

    theNodeStats[theNode].theCreatedSORDS++;
    theOverallStats.theCreatedSORDS++;
    theNodeStats[theNode].theMaxLiveSORDS << theSORDs.size();
    theOverallStats.theMaxLiveSORDS << theSORDs.size();
  } else {
    DBG_(Iface, ( << "P[" << theNode << "] SORD.Add " << *ret_val ) );
  }
  return ret_val;
}


bool GlobalSORD::recycle(tMRPVector aVector) {
  bool ret_val = false;
  DBG_( VVerb, ( << "P[" << theNode << "] Recycling memory at head of SORD.") );

  std::map< tMRPVector, SORDList>::iterator sord_iter = theSORDs.find(aVector);
  DBG_Assert( sord_iter != theSORDs.end());

  SORDList & sord = sord_iter->second;
  while (frontInvalid(sord)) {
    theTotalTemplateInUse--;
    DBG_Assert(theTotalTemplateInUse >= 0);
    sord.pop_front();
  }
  if (sord.empty()) {
    DBG_(Iface, ( << "P[" << theNode << "] SORD.Exhaust /" << & std::hex  << aVector << & std::dec <<'/') );
    theSORDs.erase(sord_iter);
    theNodeStats[theNode].theExhaustedSORDS++;
    theOverallStats.theExhaustedSORDS++;
    ret_val = true;
  }
  return ret_val;
}


void GlobalSORD::mispredict( SORDLocation aLocation) {
  DBG_(VVerb, ( << "P[" << theNode << "] SORD.Mispredict " << *aLocation ) );

  //Mark the location invalid
  aLocation->mispredict();

  //Move any streams pointing to the location
  for (int i = 0; i < theNumNodes; ++i) {
    theStreamManagers[i]->mispredict(aLocation);
  }
}


void GlobalSORD::invalidate( SORDLocation aLocation) {
  DBG_(VVerb, ( << "P[" << theNode << "] SORD.Invalidate " << *aLocation ) );

  //Mark the location invalid
  aLocation->invalidate();

  //Kill any streams pointing to the location
  for (int i = 0; i < theNumNodes; ++i) {
    theStreamManagers[i]->invalidate(aLocation);
  }
}

SORDLocation GlobalSORD::produce( tAddress anAddress , unsigned int anMRPVector) {
  SORDLocation new_location = append(anAddress, anMRPVector);

  //Give StreamManagers a chance to forward on any SORD Tail streams or
  //create new MRP-predicted streams
  for (int i = 0; i < theNumNodes; ++i) {
    theStreamManagers[i]->produce(new_location, anMRPVector );
  }
  return new_location;
}

void GlobalSORD::consume( tID aConsumer, SORDLocation aLocation) {
  //MISS
  DBG_(Iface, ( << "C[" << aConsumer << "]<-P[" << theNode <<"] SORD MISS " << *aLocation) );
  //We missed in the queue manager.
    theNodeStats[aConsumer].theSORDMisses++;
    theOverallStats.theSORDMisses++;

  //Categorize the miss
  if (aLocation->mrpVector() == 0) {
    theNodeStats[aConsumer].theSORDMissesNoMask++;
    theOverallStats.theSORDMissesNoMask++;
  } else if ( (aLocation->mrpVector() & ( 1 << aConsumer) ) == 0) {
    theNodeStats[aConsumer].theSORDMissesNotInMask++;
    theOverallStats.theSORDMissesNotInMask++;
  } else {
    theNodeStats[aConsumer].theSORDMissesInMask++;
    theOverallStats.theSORDMissesInMask++;
  }

  //Check if it makes sense to create a new stream, and create one if so.
  theStreamManagers[aConsumer]->demandStream(aLocation);

  //In any event, record in the SORD that this consumer has consumed the entry
  aLocation->markConsumed(aConsumer);
}



BaseSORDManager::~BaseSORDManager() {}
GlobalSORDManager::~GlobalSORDManager() {}


BaseSORDManager::BaseSORDManager(tID aNode, int aNumNodes, std::vector<ForwardQueue> & aFwdQueues, bool aUseEagerForwarding, bool aUseMRP, int aBlockAddrBits, int anL2BlockSize, int aNumSets, int anAssoc)
 : theForwardQueues(aFwdQueues)
 , theUseEagerForwarding(aUseEagerForwarding)
 , theNumNodes(aNumNodes)
{
  if (aUseMRP) {
    std::stringstream name;
    name << "mrp-" << std::setw(2) << std::setfill('0') << aNode;
    theMRP.reset( new nMrpTable::MrpMain( name.str(), aNode, aBlockAddrBits, anL2BlockSize, aNumSets, anAssoc));
  }
}

void BaseSORDManager::writeMRP(tID aProducer, tAddress anAddress, bool isOS) {
  //Inform MRP about the production
  if ( theMRP ) {
    theMRP->write(nMrpTable::MemoryAddress(anAddress), aProducer, isOS);
  }
}

void BaseSORDManager::readMRP(tID aConsumer, tAddress anAddress, bool isOS) {
  //Inform MRP about the consumption
  if (theMRP) {
    theMRP->read( nMrpTable::MemoryAddress(anAddress), aConsumer, isOS);
  }
}

void BaseSORDManager::dirProduction(tID aNode, tAddress anAddress) {
  theNodeStats[aNode].theDirProductions++;
  theOverallStats.theDirProductions++;
  //theOverallStats.theProductionAddresses << std::make_pair(anAddress,1);
}

void BaseSORDManager::dirConsumption(tID aNode, tAddress anAddress) {
  theNodeStats[aNode].theDirConsumptions++;
  theOverallStats.theDirConsumptions++;
}

void BaseSORDManager::dirUpgrade(tID aNode, tAddress anAddress) {
  theNodeStats[aNode].theDirUpgrades++;
  theOverallStats.theDirUpgrades++;
}

void BaseSORDManager::dirFlush(tID aNode, tAddress anAddress) {
  theNodeStats[aNode].theDirFlushes++;
  theOverallStats.theDirFlushes++;
}

GlobalSORDManager::GlobalSORDManager(tID aNode, int aNumNodes, std::vector<ForwardQueue> & aFwdQueues, char aHeadChunkSize, char aBodyChunkSize, long aStorageLimit, bool aUseEagerForwarding, bool aUseMRP, int aBlockAddrBits, int anL2BlockSize, int aNumSets, int anAssoc)
 : BaseSORDManager(aNode, aNumNodes, aFwdQueues, aUseEagerForwarding, aUseMRP, aBlockAddrBits, anL2BlockSize, aNumSets, anAssoc)
{
  for (int i = 0; i < theNumNodes; ++i) {
    theSORDS.push_back(new GlobalSORD( i, theNumNodes, aFwdQueues, aHeadChunkSize, aBodyChunkSize, aStorageLimit, *this) );
  }
}


void GlobalSORDManager::produce(tID aProducer, tAddress anAddress) {
  DBG_( Iface, ( << "P[" << aProducer << "] SORDMgr.Produce @" << & std::hex <<anAddress << &std::dec) );

  //Stats
    theNodeStats[aProducer].theProductions++;
    theOverallStats.theProductions++;

  if (theUseEagerForwarding) {
    //Get the MRP prediction
    int mrp_prediction = 0;
    if ( theMRP ) {
      mrp_prediction = theMRP->lastPrediction(nMrpTable::MemoryAddress(anAddress));
    }

    for (int i = 0; i < theNumNodes; ++i) {
      if (mrp_prediction & (1 << i)) {
        theForwardQueues[i].forward(anAddress, 0, 0);
      }
    }
  } else {

    //Invalidate an existing entry
    GlobalSORDLocatorMap::iterator iter = theSORDMap.find(anAddress);
    if (iter != theSORDMap.end()) {
      tID producer = iter->second.producer();
      tMRPVector mrp_vector = iter->second.location()->mrpVector();

      theSORDS[producer]->invalidate(iter->second.location());
      theSORDS[producer]->recycle(mrp_vector);
    }

  //Get the MRP prediction
    int mrp_prediction = 0;
    if ( theMRP ) {
      mrp_prediction = theMRP->lastPrediction(nMrpTable::MemoryAddress(anAddress));
    }

  //Add the production to the appropriate SORD
    SORDLocation location = theSORDS[aProducer]->produce(anAddress, mrp_prediction );
    theSORDMap[anAddress] = SORDMapEntry( location, aProducer) ;
  }

  std::map<tAddress, int>::iterator miss= theMissedConsumptions.find(anAddress);
  if (miss != theMissedConsumptions.end()) {
    theOverallStats.theConsumptionsBeatDowngrade += miss->second;
    theMissedConsumptions.erase( miss );
  }

}


void GlobalSORDManager::mispredict(tID aProducer, tAddress anAddress) {
  DBG_( Iface, ( << "P[" << aProducer << "] SORDMgr.Mispredict @" << & std::hex << anAddress << & std::dec ) );

  GlobalSORDLocatorMap::iterator iter = theSORDMap.find(anAddress);
  if (iter != theSORDMap.end()) {
    //There is an existing entry for this address in some SORD.
    //We must kill it.
    DBG_( VVerb, ( << "P[" << aProducer << "] SORDMgr.Invalidate.Match " << *iter->second.location()) );
    tID producer = iter->second.producer();
    DBG_Assert( producer == aProducer);

    theSORDS[producer]->mispredict(iter->second.location());
  }
}

void GlobalSORDManager::forcedRecycle(tAddress anAddress) {
    theSORDMap.erase( anAddress );
}

void GlobalSORDManager::invalidate(tID aProducer, tAddress anAddress) {
  DBG_( Iface, ( << "P[" << aProducer << "] SORDMgr.Invalidate @" << & std::hex << anAddress << & std::dec ) );

  //Stats
    theNodeStats[aProducer].theInvalidations++;
    theOverallStats.theInvalidations++;

    //Kill entries for this address in any SORD
    GlobalSORDLocatorMap::iterator iter = theSORDMap.find(anAddress);
    if (iter != theSORDMap.end()) {
      //There is an existing entry for this address in some SORD.
      //We must kill it.
      DBG_( VVerb, ( << "P[" << aProducer << "] SORDMgr.Invalidate.Match " << *iter->second.location()  ) );
      tID producer = iter->second.producer();
      tMRPVector mrp_vector = iter->second.location()->mrpVector();


      theSORDS[producer]->invalidate(iter->second.location());
      theSORDMap.erase( iter );
      theSORDS[producer]->recycle(mrp_vector);
    }

  std::map<tAddress, int>::iterator miss = theMissedConsumptions.find(anAddress);
  if (miss != theMissedConsumptions.end()) {
    theMissedConsumptions.erase( miss );
  }

}

void GlobalSORDManager::consume(tID aConsumer, tAddress anAddress) {
  DBG_( Iface, ( << "C[" << aConsumer << "] SORDMgr.Consume @" << &std::hex << anAddress << & std::dec ) );
  theNodeStats[aConsumer].theConsumptions++;
  theOverallStats.theConsumptions++;

  if (! theUseEagerForwarding) {

    //See if there is a sord that contains this address
      GlobalSORDLocatorMap::iterator iter = theSORDMap.find(anAddress);
      if (iter != theSORDMap.end()) {
        DBG_(VVerb, ( << "C[" << aConsumer << "] SORDMgr.ConsumePossible: " << *iter->second.location()) );
        //Have this consumer try to consume the address on the sord
        theSORDS[iter->second.producer()]->consume(aConsumer, iter->second.location());
      } else {
        DBG_(Iface, ( << "C[" << aConsumer << "] SORDMgr.UnSORDable @" << &std::hex << anAddress << & std::dec) );
        //Address is not present in any SORD.  It may be in some consumers cache, though.
        //Let all the consumer take a shot at it.
        theOverallStats.theConsumptionsMissingAllSORDS++;

        std::map<tAddress, int>::iterator iter = theMissedConsumptions.find(anAddress);

        if (iter == theMissedConsumptions.end()) {
          theMissedConsumptions.insert( std::make_pair(anAddress, 1));
        } else {
          iter->second++;
        }
      }
  }
}

} //end nSordManager

