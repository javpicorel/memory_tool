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

namespace nSordManager {


void PerConsSORD::cleanVictims() {
  while (theTotalTemplateInUse >= theTotalTemplateLimit) {
    //Need to reclaim storage from a template.
    DBG_Assert(!theVictims.empty());
    int victim = theVictims.front();
    theVictims.pop_front();
    recycle(victim);

    while(theTemplateLengths[victim] > theMaxLengths[victim]) {
      SORDList & strTemplate = theSORDs[victim];
      if (strTemplate.empty() || !strTemplate.front().valid()) {
        break;
      }

      theNodeStats[theNode].theOverwrittenTemplateBlocks++;
      theOverallStats.theOverwrittenTemplateBlocks++;

      //Remove this template from the list for this address
      tAddress address = strTemplate.front().address();
      PerConsSORDLocatorMap::iterator iter;
      PerConsSORDLocatorMap::iterator end;
      boost::tie( iter, end ) = theSORDMap.equal_range(address);
      bool erased = false;
      bool some = false;
      while (iter != end) {
        some = true;
        if(iter->second->streamTemplate() == victim) {
          theSORDMap.erase(iter);
          erased = true;
          break;
        }
        ++iter;
      }
      DBG_Assert(some, ( << "bad address["<<theNode<<"]:" << std::hex << address << " entry:" << strTemplate.front() ) );
      DBG_Assert(erased);
      //If necessary, inform the SORDManager that we are throwing this block away.
      if(theSORDMap.count(address) == 0) {
        theManager.forcedRecycle(address);
      }

      //Kill any streams pointing to the location.  Note that we do not steal
      //the block from any caches.
      if(strTemplate.front().streamTemplate() < 16) {
        theStreamManagers[strTemplate.front().streamTemplate()]->invalidate(strTemplate.begin());
      } else {
        for (int i = 0; i < 16; ++i) {
          theStreamManagers[i]->invalidate(strTemplate.begin());
        }
      }
      strTemplate.front().invalidate();

      recycle(victim);

    } // while(length[victim] > maxLength[victim]

  } //Otherwise, we try again with the next victim

}

SORDLocation PerConsSORD::append( unsigned int aTemplate, tAddress anAddress) {
  cleanVictims();

  SORDList & sord = theSORDs[aTemplate];
  sord.push_back( SORDEntry(anAddress, theEntryIndex++, aTemplate) );
  theTotalTemplateInUse++;
  theTemplateLengths[aTemplate]++;
  if(theTemplateLengths[aTemplate] >= theMaxLengths[aTemplate]) {
    theVictims.push_back(aTemplate);
  }

  SORDLocation ret_val = --sord.end();
  theSORDMap.insert(std::make_pair(anAddress,ret_val));
  DBG_(Iface, ( << "P[" << theNode << "] SORD.Add " << *ret_val ) );
  return ret_val;
}

PerConsSORD::PerConsSORD(int aNode, int aNumNodes, std::vector<ForwardQueue> & aForwardQueues, char aHeadChunkSize, char aBodyChunkSize, long aStorageLimit, PerConsSORDManager & aManager)
 : BaseSORD(aNode, aNumNodes, aForwardQueues, aStorageLimit)
 , theManager(aManager)
{
  theSORDs.resize(17);
  theTemplateLengths.resize(17);
  theMaxLengths.resize(17);
  for (int i = 0; i < 16; ++i) {
    theMaxLengths[i] = theTotalTemplateLimit / 32;
  }
  theMaxLengths[16] = theTotalTemplateLimit / 2;

  for (int i = 0; i < 16; ++i) {
    theStreamManagers.push_back(new PerConsStreamManager( i, aNode, aHeadChunkSize, aBodyChunkSize, aForwardQueues[i] , *this) );
  }
}

PerConsSORD::~PerConsSORD() {}

SORDLocation PerConsSORD::begin(unsigned short aTemplate) {
  return theSORDs[aTemplate].begin();
}

SORDLocation PerConsSORD::end(unsigned short aTemplate) {
  return theSORDs[aTemplate].end();
}

void PerConsSORD::recycle(int aTemplate) {
  DBG_( VVerb, ( << "P[" << theNode << "] Recycling memory at head of SORD.") );

  SORDList & sord = theSORDs[aTemplate];
  while (frontInvalid(sord)) {
    theTotalTemplateInUse--;
    theTemplateLengths[aTemplate]--;
    DBG_Assert(theTotalTemplateInUse >= 0);
    sord.pop_front();
  }
}


void PerConsSORD::mispredict( tAddress anAddress) {
  DBG_(VVerb, ( << "P[" << theNode << "] SORD.Mispredict " << anAddress ) );

  //Find all occurances of this address
  PerConsSORDLocatorMap::iterator iter;
  PerConsSORDLocatorMap::iterator end;
  boost::tie( iter, end ) = theSORDMap.equal_range(anAddress);
  while (iter != end) {
    SORDLocation location = iter->second;


    //Mark the location mispredicted
    location->mispredict();

    //Kill any streams pointing to the location
    if(location->streamTemplate() < 16) {
      theStreamManagers[location->streamTemplate()]->mispredict(location);
    } else {
      for (int i = 0; i < 16; ++i) {
        theStreamManagers[i]->mispredict(location);
      }
    }

    ++iter;
  }
}


void PerConsSORD::invalidate( tAddress anAddress) {
  DBG_(VVerb, ( << "P[" << theNode << "] SORD.Invalidate " << anAddress ) );

  //Find all occurances of this address
  PerConsSORDLocatorMap::iterator begin;
  PerConsSORDLocatorMap::iterator iter;
  PerConsSORDLocatorMap::iterator end;
  boost::tie( begin, end ) = theSORDMap.equal_range(anAddress);
  iter = begin;
  while (iter != end) {
    SORDLocation location = iter->second;

    //Mark the location invalid
    location->invalidate();

    //Kill any streams pointing to the location
    if(location->streamTemplate() < 16) {
      theStreamManagers[location->streamTemplate()]->invalidate(location);
    } else {
      for (int i = 0; i < 16; ++i) {
        theStreamManagers[i]->invalidate(location);
      }
    }
    recycle(location->streamTemplate());

    ++iter;
  }

  theSORDMap.erase(begin,end);
}

void PerConsSORD::doProd( tID aTemplate, tAddress anAddress) {
  SORDLocation new_location = append(aTemplate, anAddress);

  //Give StreamManagers a chance to forward on any SORD Tail streams or
  //create new MRP-predicted streams
  if(aTemplate < 16) {
    theStreamManagers[aTemplate]->produce(new_location, aTemplate );
  } else {
    for (int i = 0; i < 16; ++i) {
      theStreamManagers[i]->produce(new_location, aTemplate );
    }
  }
}

void PerConsSORD::produce( tAddress anAddress , unsigned int anMRPVector) {
  for (int i = 0; i < 16; ++i) {
    unsigned int vector = 1 << i;
    if (anMRPVector & vector) {
      doProd(i, anAddress);
    }
  }
  if(anMRPVector == 0) {
    doProd(16, anAddress);
  }
}

void PerConsSORD::doCons( tID aConsumer, SORDLocation aLocation) {
  DBG_(VVerb, ( << "C[" << aConsumer << "]<-P[" << theNode <<"] SORD.Consume " << *aLocation) );

    //MISS
    DBG_(Iface, ( << "C[" << aConsumer << "]<-P[" << theNode <<"] SORD MISS " << *aLocation) );
    //We missed in the queue manager.
    theNodeStats[aConsumer].theSORDMisses++;
    theOverallStats.theSORDMisses++;

    //Categorize the miss
    if (aLocation->streamTemplate() == 16) {
      theNodeStats[aConsumer].theSORDMissesGlobal++;
      theOverallStats.theSORDMissesGlobal++;
    } else {
      theNodeStats[aConsumer].theSORDMissesPerConsumer++;
      theOverallStats.theSORDMissesPerConsumer++;
    }

    //Check if it makes sense to create a new stream, and create one if so.
      theStreamManagers[aConsumer]->demandStream(aLocation);

  //In any event, record in the SORD that this consumer has consumed the entry
  aLocation->markConsumed(aConsumer);
}

void PerConsSORD::consume( tID aConsumer, tAddress anAddress) {
  //Find the occurance of this address
  bool found = false;
  bool some = false;
  PerConsSORDLocatorMap::iterator iter;
  PerConsSORDLocatorMap::iterator end;
  boost::tie( iter, end ) = theSORDMap.equal_range(anAddress);
  while (iter != end) {
    some = true;
    SORDLocation location = iter->second;
    if(location->streamTemplate() == aConsumer) {
      DBG_Assert(!found, ( << "dup template[" << theNode << "]:" << *location ) );
      found = true;
      doCons(aConsumer, location);
    }
    if(location->streamTemplate() == 16) {
      DBG_Assert(!found, ( << "dup template[" << theNode << "]:" << *location ) );
      found = true;
      doCons(aConsumer, location);
    }
    ++iter;
  }
  DBG_Assert(some, ( << "bad address:" << std::hex << anAddress ) );
}


PerConsSORDManager::~PerConsSORDManager() {}

PerConsSORDManager::PerConsSORDManager(tID aNode, int aNumNodes, std::vector<ForwardQueue> & aFwdQueues, char aHeadChunkSize, char aBodyChunkSize, long aStorageLimit, bool aUseEagerForwarding, bool aUseMRP, int aBlockAddrBits, int anL2BlockSize, int aNumSets, int anAssoc)
 : BaseSORDManager(aNode, aNumNodes, aFwdQueues, aUseEagerForwarding, aUseMRP, aBlockAddrBits, anL2BlockSize, aNumSets, anAssoc)
{
  for (int i = 0; i < 16; ++i) {
    theSORDS.push_back(new PerConsSORD( i, 16, aFwdQueues, aHeadChunkSize, aBodyChunkSize, aStorageLimit, *this) );
  }
}



void PerConsSORDManager::produce(tID aProducer, tAddress anAddress) {
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

    for (int i = 0; i < 16; ++i) {
      if (mrp_prediction & (1 << i)) {
        theForwardQueues[i].forward(anAddress, 0, 0);
      }
    }
  } else {

    //Invalidate an existing entry
    ProducerLocatorMap::iterator iter = theProducerMap.find(anAddress);
    if (iter != theProducerMap.end()) {
      tID producer = iter->second;
      theSORDS[producer]->invalidate(anAddress);
    }
    theProducerMap[anAddress] = aProducer;

    //Get the MRP prediction
    int mrp_prediction = 0;
    if ( theMRP ) {
      mrp_prediction = theMRP->lastPrediction(nMrpTable::MemoryAddress(anAddress));
    }

    //Add the production to the appropriate SORD
    theSORDS[aProducer]->produce(anAddress, mrp_prediction);
  }

  std::map<tAddress, int>::iterator miss= theMissedConsumptions.find(anAddress);
  if (miss != theMissedConsumptions.end()) {
    theOverallStats.theConsumptionsBeatDowngrade += miss->second;
    theMissedConsumptions.erase( miss );
  }
}

void PerConsSORDManager::mispredict(tID aProducer, tAddress anAddress) {
  DBG_( Iface, ( << "P[" << aProducer << "] SORDMgr.Invalidate @" << & std::hex << anAddress << & std::dec ) );

  ProducerLocatorMap::iterator iter = theProducerMap.find(anAddress);
  if (iter != theProducerMap.end()) {
    //There is an existing entry for this address in some SORD.
    //We must kill it.
    DBG_( VVerb, ( << "P[" << aProducer << "] SORDMgr.Invalidate.Match " << iter->second ) );
    tID producer = iter->second;
    DBG_Assert( producer == aProducer);

    theSORDS[producer]->mispredict(anAddress);
  }
}

void PerConsSORDManager::forcedRecycle(tAddress anAddress) {
  int num = theProducerMap.erase( anAddress );
  DBG_Assert(num == 1, ( << "number erased: " << num << " address:0x" << &std::hex << anAddress ) );
}

void PerConsSORDManager::invalidate(tID aProducer, tAddress anAddress) {
  DBG_( Iface, ( << "P[" << aProducer << "] SORDMgr.Invalidate @" << & std::hex << anAddress << & std::dec ) );

  //Stats
  theNodeStats[aProducer].theInvalidations++;
  theOverallStats.theInvalidations++;

  //Kill entries for this address in any SORD
  ProducerLocatorMap::iterator iter = theProducerMap.find(anAddress);
  if (iter != theProducerMap.end()) {
    //There is an existing entry for this address in some SORD.
    //We must kill it.
    DBG_( VVerb, ( << "P[" << aProducer << "] SORDMgr.Invalidate.Match " << iter->second ) );
    theSORDS[iter->second]->invalidate(anAddress);
    theProducerMap.erase( iter );
  }

  std::map<tAddress, int>::iterator miss = theMissedConsumptions.find(anAddress);
  if (miss != theMissedConsumptions.end()) {
    theMissedConsumptions.erase( miss );
  }
}

void PerConsSORDManager::consume(tID aConsumer, tAddress anAddress) {
  DBG_( VVerb, ( << "C[" << aConsumer << "] SORDMgr.Consume @" << &std::hex << anAddress << & std::dec ) );
  theNodeStats[aConsumer].theConsumptions++;
  theOverallStats.theConsumptions++;

  if (! theUseEagerForwarding) {

    //See if there is a sord that contains this address
      ProducerLocatorMap::iterator iter = theProducerMap.find(anAddress);
      if (iter != theProducerMap.end()) {
        DBG_(VVerb, ( << "C[" << aConsumer << "] SORDMgr.ConsumePossible: " << iter->second) );
        //Have this consumer try to consume the address on the sord
        theSORDS[iter->second]->consume(aConsumer, anAddress);
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

