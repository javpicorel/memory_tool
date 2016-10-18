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
#ifndef SORD_INCLUDED
#define SORD_INCLUDED

#include <boost/shared_ptr.hpp>

#include "common.hpp"

namespace nMrpTable {
  class MrpMain;
}

namespace nSordManager {


struct StreamBase : public boost::counted_base {
  virtual ~StreamBase() {}
  virtual void notifyHit(long aChunkID) = 0;
  virtual std::string toString() const = 0;
};

struct ForwardQueueEntry {
  tAddress theAddress;
  boost::intrusive_ptr<StreamBase> theStream;
  long theChunk;
};


struct ForwardQueue {
  std::list< ForwardQueueEntry> theQueue;
  void forward(tAddress anAddress, boost::intrusive_ptr<StreamBase> aStream, long aChunk) {
    theQueue.push_back(ForwardQueueEntry());
    theQueue.back().theAddress = anAddress;
    theQueue.back().theChunk = aChunk;
    theQueue.back().theStream = aStream;
  }
};



class SORD;
class BaseSORDManager;
class GlobalSORDManager;
class PerConsSORDManager;
class Stream;
class BaseStreamManager;
class GlobalStreamManager;
class PerConsStreamManager;

class BaseSORD {
  protected:
  int theNode;
  int theNumNodes;

  std::vector<BaseStreamManager *> theStreamManagers; //Stream manager per consumer
  std::vector<ForwardQueue> & theForwardQueues; //Queue manager per consumer

  long theTotalTemplateLimit;
  long theTotalTemplateInUse;

  int theEntryIndex;

  BaseSORD(int aNode, int aNumNodes, std::vector<ForwardQueue> & aForwardQueues, long aStorageLimit);
  virtual ~BaseSORD();

  public:
  int producer() { return theNode; }
  virtual SORDLocation end(unsigned short aTemplate) = 0;
  virtual SORDLocation begin(unsigned short aTemplate) = 0;

  bool frontInvalid(SORDList & aSORD);

};

class GlobalSORD : public BaseSORD {
  std::map< tMRPVector, SORDList> theSORDs;
  std::list< tMRPVector > theVictimVectors;

  GlobalSORDManager & theManager;


private:
  SORDLocation append( tAddress anAddress, unsigned int anMRPVector);

public:
  GlobalSORD(int aNode, int aNumNodes, std::vector<ForwardQueue> & aForwardQueues, char aHeadChunkSize, char aBodyChunkSize, long aStorageLimit, GlobalSORDManager & aManager);
  ~GlobalSORD();

  SORDLocation end(unsigned short anMRPVector);
  SORDLocation begin(unsigned short anMRPVector);

  bool recycle(tMRPVector aVector);
  void invalidate( SORDLocation aLocation );
  void mispredict( SORDLocation aLocation );
  SORDLocation produce( tAddress anAddress, unsigned int anMRPVector );
  void consume( tID aConsumer, SORDLocation aLocation );
};


class PerConsSORD : public BaseSORD {
  PerConsSORDLocatorMap theSORDMap;
  std::vector< SORDList> theSORDs;
  std::vector<int> theTemplateLengths;
  std::vector<int> theMaxLengths;
  std::list<int> theVictims;

  PerConsSORDManager & theManager;

private:
  SORDLocation append( unsigned int aTemplate, tAddress anAddress );

public:
  PerConsSORD(int aNode, int aNumNodes, std::vector<ForwardQueue> & aForwardQueues, char aHeadChunkSize, char aBodyChunkSize, long aStorageLimit, PerConsSORDManager & aManager);
  ~PerConsSORD();

  SORDLocation end(unsigned short aTemplate);
  SORDLocation begin(unsigned short aTemplate);

  void cleanVictims();
  void recycle(int aTemplate);
  void invalidate( tAddress anAddress);
  void mispredict( tAddress anAddress);
  void forcedRecycle( tAddress anAddress);
  void produce( tAddress anAddress , unsigned int anMRPVector);
  void consume( tID aConsumer, tAddress anAddress);
  void doProd( tID aTemplate, tAddress anAddress);
  void doCons( tID aConsumer, SORDLocation aLocation);
};


class BaseSORDManager {
protected:
  boost::scoped_ptr< nMrpTable::MrpMain > theMRP;
  std::vector<ForwardQueue> & theForwardQueues;
  bool theUseEagerForwarding;
  int theNumNodes;
  std::map<tAddress, int> theMissedConsumptions;

public:
  void readMRP(tID aProducer, tAddress anAddress, bool isOS);
  void writeMRP(tID aProducer, tAddress anAddress, bool isOS);
  void dirProduction(tID aNode, tAddress anAddress);
  void dirConsumption(tID aNode, tAddress anAddress);
  void dirUpgrade(tID aNode, tAddress anAddress);
  void dirFlush(tID aNode, tAddress anAddress);
  BaseSORDManager(tID aNodeId, int aNumNodes, std::vector<ForwardQueue> & aForwardQueues, bool aUseEagerForwarding, bool aUseMRP, int aBlockAddrBits, int anL2BlockSize, int aNumSets, int anAssoc);
  virtual ~BaseSORDManager();

  virtual void produce(tID aProducer, tAddress anAddress) = 0;
  virtual void invalidate(tID aProducer, tAddress anAddress) = 0;
  virtual void mispredict(tID aProducer, tAddress anAddress) = 0;
  virtual void consume(tID aConsumer, tAddress anAddress) = 0;

  virtual void forcedRecycle(tAddress anAddress) = 0;

};

class GlobalSORDManager : public BaseSORDManager {
  std::vector<GlobalSORD *> theSORDS; //SORD per producer
  GlobalSORDLocatorMap theSORDMap;


public:
  ~GlobalSORDManager();
  GlobalSORDManager(tID aNodeId, int aNumNodes, std::vector<ForwardQueue> & aForwardQueues, char aDemandHeadChunkSize, char aBodyChunkSize, long aTotalTemplateLimit, bool aUseEagerForwarding, bool aUseMRP, int aBlockAddrBits, int anL2BlockSize, int aNumSets, int anAssoc);


  void produce(tID aProducer, tAddress anAddress);
  void invalidate(tID aProducer, tAddress anAddress);
  void mispredict(tID aProducer, tAddress anAddress);
  void consume(tID aConsumer, tAddress anAddress);
  void forcedRecycle(tAddress anAddress);

};

class PerConsSORDManager : public BaseSORDManager {
  std::vector<PerConsSORD *> theSORDS; //SORD per producer
  ProducerLocatorMap theProducerMap;

public:
  ~PerConsSORDManager();
  PerConsSORDManager(tID aNodeId, int aNumNodes, std::vector<ForwardQueue> & aForwardQueues, char aDemandHeadChunkSize, char aBodyChunkSize, long aTotalTemplateLimit, bool aUseEagerForwarding, bool aUseMRP, int aBlockAddrBits, int anL2BlockSize, int aNumSets, int anAssoc);

  void produce(tID aProducer, tAddress anAddress);
  void invalidate(tID aProducer, tAddress anAddress);
  void mispredict(tID aProducer, tAddress anAddress);
  void consume(tID aConsumer, tAddress anAddress);
  void forcedRecycle(tAddress anAddress);

};



} //namespace nSordManager

#endif //SORD_INCLUDED
