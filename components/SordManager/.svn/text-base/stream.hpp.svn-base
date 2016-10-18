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
#ifndef STREAM_INCLUDED
#define STREAM_INCLUDED

#include <core/boost_extensions/intrusive_ptr.hpp>

#include "sord.hpp"

namespace nSordManager {

class Stream;
class BaseStreamManager;

typedef char StreamType;
static const char StrDemandInMask = 0;
static const char StrDemandNotInMask = 1;
static const char StrMRP = 2;
static const char StrDemandNoMask = 3;
static const char StrGlobal = 4;
static const char StrPerConsumer = 5;


class Stream : public StreamBase {
  SORDLocation theFwdPtr;
  unsigned short theTemplate;
  char theBodyChunkSize;
  char theCurrentChunkRemaining;
  long theID;
  long theCurrentChunk;

  BaseStreamManager & theManager;
  StreamType theStreamType;

  //Statistics
  int theLength;
  int theForwardedBlocks;

  char theStreamDead:1;
  char theStreamDying:1;
  char theIsSORDTail:1;
  char theNextChunkRequested:1;

public:
  Stream (unsigned short aVectorOrTemplate, long anID, SORDLocation aLocation, char aHeadChunkSize, char aBodyChunkSize, BaseStreamManager & aManager, StreamType aStreamType);
  virtual ~Stream ();

  //Accessors
    long id() { return theID; }
    bool dead() { return theStreamDead; }
    bool tail() { return theIsSORDTail; }
    SORDLocation fwdPtr() { return theFwdPtr; }
    unsigned short mrpVector() { return theTemplate; }
    unsigned char streamTemplate() { return theTemplate; }

  void terminate(StreamDeathReason aReason);
  friend std::ostream & operator << ( std::ostream & anOstream, Stream const & aStream);
  void invalidate(tAddress anAddress);
  void produce(SORDLocation aLocation);
  void forward();
  void mispredict();
  void advance(bool);
  void notifyHit(long aChunkID);
  bool mayForward();
  void decrementChunkRemaining();
  std::string toString() const;

};

class BaseStreamManager {
  protected:
    tID theConsumer;
    tID theProducer;
    int theNextStreamID;
    std::list< boost::intrusive_ptr<Stream> > theStreams;

    char theHeadChunkSize;
    char theBodyChunkSize;

    ForwardQueue & theForwardQueue;
    std::map<tAddress, Stream * > theFwdPtrMap;
    BaseSORD & theSORD;
    int theLiveStreams;

  public:
    BaseStreamManager(tID aConsumer, tID aProducer, char anHeadChunkSize, char aBodyChunkSize, ForwardQueue & aForwardQueue, BaseSORD & aSORD);
    virtual ~BaseStreamManager();

    void setFwdPtr( Stream * aStream, boost::optional<tAddress> aPrevious, boost::optional<tAddress> aNew);

    tID consumer() { return theConsumer; }
    tID producer() { return theProducer; }

    void finalizeStreams();
    SORDLocation end(tMRPVector aVector);
    virtual void removeLiveStream( boost::intrusive_ptr<Stream> aStream) = 0;
    void forward( SORDLocation aLocation, boost::intrusive_ptr<Stream> aStream, long aChunkId);
    virtual void invalidate(SORDLocation aLocation) = 0;
    void mispredict(SORDLocation aLocation);
    virtual void produce(SORDLocation aLocation, unsigned short aTemplate) = 0;
    virtual void demandStream(SORDLocation aLocation) = 0;
    void notifyHit(long aChunkID);

    virtual void setGlobalTailStream( boost::intrusive_ptr<Stream> aStream) { } //Do nothing in GlobalStreamManager
    virtual void setPerConsumerTailStream( boost::intrusive_ptr<Stream> aStream) {} //Do nothing in GlobalStreamManager

};

class PerConsStreamManager : public BaseStreamManager {
    boost::intrusive_ptr<Stream> theGlobalTailStream;
    boost::intrusive_ptr<Stream> thePerConsumerTailStream;
  public:
    PerConsStreamManager(tID aConsumer, tID aProducer, char anHeadChunkSize, char aBodyChunkSize, ForwardQueue & aForwardQueue, BaseSORD & aSORD);

    void setGlobalTailStream( boost::intrusive_ptr<Stream> aStream);
    void setPerConsumerTailStream( boost::intrusive_ptr<Stream> aStream);
    void removeLiveStream( boost::intrusive_ptr<Stream> aStream);
    void invalidate(SORDLocation aLocation);
    void produce(SORDLocation aLocation, unsigned short aTemplate);
    void demandStream(SORDLocation aLocation);
  private:
    void newStream(SORDLocation aLocation, unsigned char aTemplate, bool isTail );


};

class GlobalStreamManager : public BaseStreamManager {
    std::map<tMRPVector, boost::intrusive_ptr<Stream> > theTailStreams;

  public:
    GlobalStreamManager(tID aConsumer, tID aProducer, char anHeadChunkSize, char aBodyChunkSize, ForwardQueue & aForwardQueue, BaseSORD & aSORD);
    void removeLiveStream( boost::intrusive_ptr<Stream> aStream);
    void invalidate(SORDLocation aLocation);
    void produce(SORDLocation aLocation, unsigned short aTemplate);
    void demandStream(SORDLocation aLocation);
    void finalizeStreams();
  private:
    void newStream(SORDLocation aLocation, tMRPVector aVector, char aHeadChunkSize);

};

} //end namespace nSordManager

#endif //STREAM_INCLUDED
