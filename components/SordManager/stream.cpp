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

#include <iostream>
#include <sstream>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace l = boost::lambda;

#include "sord.hpp"
#include "stream.hpp"

namespace nSordManager {


std::ostream & operator << ( std::ostream & anOstream, Stream const & aStream) {
  anOstream << "< P["<< aStream.theManager.producer() <<"]->C[" << aStream.theManager.consumer() << "] !" << aStream.theID;

  if (aStream.theStreamDead) {
    anOstream << " @dead";
  } else if (aStream.theStreamDying) {
    anOstream << " @dying";
  } else if (aStream.theIsSORDTail) {
    anOstream << " @tail";
  } else {
    anOstream << ' ' << *aStream.theFwdPtr;
  }
  anOstream << " |" << aStream.theLength << "| ";
  anOstream << ">";
  return anOstream;
}

std::string Stream::toString() const {
  std::ostringstream s;
  s << *this;
  return s.str();
}

Stream::Stream (unsigned short aTemplate, long anID, SORDLocation aLocation, char aHeadChunkSize, char aBodyChunkSize, BaseStreamManager & aManager, StreamType aStreamType)
  : theFwdPtr(aLocation)
  , theTemplate(aTemplate)
  , theBodyChunkSize(aBodyChunkSize)
  , theCurrentChunkRemaining(aHeadChunkSize)
  , theID(anID)
  , theCurrentChunk(0)
  , theManager(aManager)
  , theStreamType(aStreamType)
  , theLength( (aStreamType == StrMRP) ? 0 : 1)
  , theForwardedBlocks(0)
  , theStreamDead(false)
  , theStreamDying(false)
  , theIsSORDTail( aLocation == aManager.end(aTemplate) )
  , theNextChunkRequested(false)
  {
  //This is a non-MRP predicted
  if (! theIsSORDTail) {
    theManager.setFwdPtr(this, boost::optional<tAddress>(), boost::optional<tAddress>(theFwdPtr->address()));
  }
}

void Stream::terminate(StreamDeathReason aReason) {
  DBG_(Iface, ( << "C[" << theManager.consumer() << "] Stream.Terminate " << *this ) );
  if ( ! theStreamDying ) {

    if (! theIsSORDTail) {
      theManager.setFwdPtr(this,boost::optional<tAddress>(theFwdPtr->address()),boost::optional<tAddress>());
    }

    theStreamDying = true;

    theNodeStats[theManager.consumer()].theKilledStreams++;
    theOverallStats.theKilledStreams++;
    switch (aReason) {
      case eKilledByEndOfStream:
        DBG_(VVerb, ( << "C[" << theManager.consumer() << "] Stream.Kill " << *this << " by end of stream") );
        theNodeStats[theManager.consumer()].theKilledByEndOfStream++;
        theOverallStats.theKilledByEndOfStream++;
        break;
      case eKilledByEndOfInput:
        DBG_(VVerb, ( << "C[" << theManager.consumer() << "] Stream.Kill " << *this << " by end of input") );
        theNodeStats[theManager.consumer()].theKilledByEndOfInput++;
        theOverallStats.theKilledByEndOfInput++;
        break;
      case eKilledByReDemand:
        DBG_(VVerb, ( << "C[" << theManager.consumer() << "] Stream.Kill " << *this << " by ReDemand") );
        theNodeStats[theManager.consumer()].theKilledByReDemand++;
        theOverallStats.theKilledByReDemand++;
        break;
      default:
        DBG_Assert( false, ( << "Bad kill reason: " << aReason ));
    };

    theManager.removeLiveStream(this);

  }
}

void Stream::invalidate(tAddress anAddress) {
  DBG_(VVerb, ( << *this << " Invalidate @" << &std::hex << anAddress<< &std::dec ) );
  DBG_Assert(! theIsSORDTail );
  DBG_Assert(anAddress == theFwdPtr->address());
  terminate( eKilledByEndOfStream );
}

Stream::~Stream() {
  DBG_(VVerb, ( << "Stream.kill " << *this) );

  DBG_Assert(theStreamDying);
  DBG_Assert(!theStreamDead);
  theStreamDead = true;


  theNodeStats[theManager.consumer()].theMaxLength << theLength;
  theOverallStats.theMaxLength << theLength;
  theNodeStats[theManager.consumer()].theAvgLength << theLength;
  theOverallStats.theAvgLength << theLength;

  theOverallStats.theLengthHistogram << theLength;

  theNodeStats[theManager.consumer()].theAvgFwdBlocks << theForwardedBlocks;
  theOverallStats.theAvgFwdBlocks << theForwardedBlocks;

  long hits = theLength - 1;

  theOverallStats.theWaste += theForwardedBlocks - hits;

  if (theStreamType == StrPerConsumer) {
    theOverallStats.thePerConsumerWaste += theForwardedBlocks - hits;
  } else {
    theOverallStats.theGlobalWaste += theForwardedBlocks - hits;
  }

}

bool Stream::mayForward() {
  if ((theCurrentChunkRemaining == 0) && (theNextChunkRequested)) {
    theNextChunkRequested = false;
    theCurrentChunk++;
    theCurrentChunkRemaining = theBodyChunkSize;
  }
  return (theCurrentChunkRemaining > 0);
}

void Stream::decrementChunkRemaining() {
  theCurrentChunkRemaining--;
  DBG_Assert(theCurrentChunkRemaining >= 0);
}

void Stream::produce(SORDLocation aLocation) {
  if (! tail() ) {
    return;
  }
  DBG_( VVerb, ( << "Stream " << *this << " Produce " << *aLocation ) );

  DBG_Assert(! theStreamDying );



  if (! mayForward() ) {
    //We have no window for forwarding, so we just hang on to the new block
    //for later.  Transition out of SORD tail state.
    theIsSORDTail = false;
    theFwdPtr = aLocation;
    theManager.setFwdPtr(this, boost::optional<tAddress>(), boost::optional<tAddress>(theFwdPtr->address()));
    if ( theTemplate == 16) {
      theManager.setGlobalTailStream(0);
    } else {
      theManager.setPerConsumerTailStream(0);
    }
  } else {
    ++theForwardedBlocks;
    ++theNodeStats[theManager.producer()].theTotFwdBlocks;
    ++theOverallStats.theTotFwdBlocks;
    theManager.forward(aLocation, this, theCurrentChunk);
    decrementChunkRemaining();
  }
}

void Stream::advance( bool anInitializeFwdPtr) {
  bool done = false;
  tAddress old_addr;
  if (! anInitializeFwdPtr) {
    old_addr = theFwdPtr->address();
  }
  while (! done ) {
    ++theFwdPtr;
    if (theFwdPtr == theManager.end(theTemplate)) {
      //We are at the tail, so there is nothing to forward.
      theIsSORDTail = true;
      if (! anInitializeFwdPtr) {
        theManager.setFwdPtr(this, boost::optional<tAddress>(old_addr), boost::optional<tAddress>());
      }
      if ( theTemplate == 16) {
        theManager.setGlobalTailStream(this);
      } else {
        theManager.setPerConsumerTailStream(this);
      }
      done = true;
    } else {
      if (theFwdPtr->mispredicted()) {
        //Skip over mispredicted entries
        ++theNodeStats[theManager.consumer()].theMispredictAdvances;
        ++theOverallStats.theMispredictAdvances;
        continue;
      }
      //Check to see if we have hit a stream killing condition
      if (    (! theFwdPtr->valid())
           || ( theFwdPtr->hasConsumed(theManager.consumer()) )
           || ( theFwdPtr->mrpVector() != theTemplate )
         ) {
        //We have hit an invalid block, a block that has already been
        //read by this consumer, or a block with a new MRP vector.
        //This terminates the stream.
        theManager.setFwdPtr(this, boost::optional<tAddress>(old_addr), boost::optional<tAddress>());
        theIsSORDTail = true; //Suppress the setFwdPtr call in terminate
        terminate(eKilledByEndOfStream);
      } else {

        if (anInitializeFwdPtr) {
          theManager.setFwdPtr(this, boost::optional<tAddress>(), boost::optional<tAddress>(theFwdPtr->address()));
        } else {
          theManager.setFwdPtr(this, boost::optional<tAddress>(old_addr), boost::optional<tAddress>(theFwdPtr->address()));
        }
      }
      done = true;
    }
  }
}

void Stream::mispredict() {
  DBG_Assert( ! theStreamDying );
  DBG_Assert( ! theIsSORDTail );
  DBG_Assert( theFwdPtr->mispredicted() );

  ++theNodeStats[theManager.consumer()].theMispredictAdvances;
  ++theOverallStats.theMispredictAdvances;

  advance(false);
}

void Stream::forward() {
  if (theStreamDying || theIsSORDTail) {
    return;
  }
  while (mayForward()) {
    //Forward blocks starting at the forward pointer

    if (theFwdPtr == theManager.end(theTemplate)) {
      //We are creating this stream as a tail stream, as it was created as
      //a result of a demand consume of the last element of the SORD.
      DBG_Assert(tail());
      return;
    }

    //Check to see if we have hit a stream killing condition
    if (    (! theFwdPtr->valid())
         || ( theFwdPtr->hasConsumed(theManager.consumer()) )
         || ( theFwdPtr->mrpVector() != theTemplate )
       ) {
      //We have hit an invalid block, a block that has already been
      //read by this consumer, or a block with a new MRP vector.
      //This terminates the stream.
      terminate(eKilledByEndOfStream);
      break;
    }

    //The block is good.  Forward it.
    ++theForwardedBlocks;
    ++theNodeStats[theManager.producer()].theTotFwdBlocks;
    ++theOverallStats.theTotFwdBlocks;
    theManager.forward(theFwdPtr, this, theCurrentChunk);
    decrementChunkRemaining();

    advance(false);
  }
}

void Stream::notifyHit(long aChunkId ) {
  DBG_( Iface, ( << "Stream " << *this << " NotifyHit for " << aChunkId) );
  //We have a hit on this stream!
  //Update statistics
    ++theLength;
    theNodeStats[theManager.consumer()].theSORDHits++;
    theOverallStats.theSORDHits++;
    switch (theStreamType) {
      case StrDemandInMask:
        theNodeStats[theManager.consumer()].theSORDHitsDemandInMask++;
        theOverallStats.theSORDHitsDemandInMask++;
        break;
      case StrDemandNotInMask:
        theNodeStats[theManager.consumer()].theSORDHitsDemandNotInMask++;
        theOverallStats.theSORDHitsDemandNotInMask++;
        break;
      case StrDemandNoMask:
        theNodeStats[theManager.consumer()].theSORDHitsDemandNoMask++;
        theOverallStats.theSORDHitsDemandNoMask++;
        break;
      case StrMRP:
        if (theLength == 1) {
          theNodeStats[theManager.consumer()].theSORDHitsMRPHead++;
          theOverallStats.theSORDHitsMRPHead++;
        }
        theNodeStats[theManager.consumer()].theSORDHitsMRP++;
        theOverallStats.theSORDHitsMRP++;
        break;
      case StrGlobal:
        theNodeStats[theManager.consumer()].theSORDHitsGlobal++;
        theOverallStats.theSORDHitsGlobal++;
        break;
      case StrPerConsumer:
        theNodeStats[theManager.consumer()].theSORDHitsPerConsumer++;
        theOverallStats.theSORDHitsPerConsumer++;
        break;
      default:
        DBG_Assert(false);
    }

  if (! theStreamDying) {

    //Increase our window by the shift amount
    if (aChunkId == theCurrentChunk) {
      theNextChunkRequested = true;
    }

    //Now, we call forward to send blocks to the queue, however much space
    //is now available
      forward();
  }
}

BaseStreamManager::BaseStreamManager(tID aConsumer, tID aProducer, char aHeadChunkSize, char aBodyChunkSize, ForwardQueue & aFwdQueue, BaseSORD & aSORD)
  : theConsumer(aConsumer)
  , theProducer(aProducer)
  , theNextStreamID(0)
  , theHeadChunkSize(aHeadChunkSize)
  , theBodyChunkSize(aBodyChunkSize)
  , theForwardQueue(aFwdQueue)
  , theSORD(aSORD)
  , theLiveStreams(0)
  {
    Flexus::Stat::getStatManager()->addFinalizer( l::bind(&BaseStreamManager::finalizeStreams, this));
}

BaseStreamManager::~BaseStreamManager() {}

GlobalStreamManager::GlobalStreamManager(tID aConsumer, tID aProducer, char aHeadChunkSize, char aBodyChunkSize, ForwardQueue & aFwdQueue, BaseSORD & aSORD)
  : BaseStreamManager(aConsumer, aProducer, aHeadChunkSize, aBodyChunkSize, aFwdQueue, aSORD)
{
    Flexus::Stat::getStatManager()->addFinalizer( l::bind(&GlobalStreamManager::finalizeStreams, this));
}

PerConsStreamManager::PerConsStreamManager(tID aConsumer, tID aProducer, char aHeadChunkSize, char aBodyChunkSize, ForwardQueue & aFwdQueue, BaseSORD & aSORD)
  : BaseStreamManager(aConsumer, aProducer, aHeadChunkSize, aBodyChunkSize, aFwdQueue, aSORD)
  {}

void BaseStreamManager::setFwdPtr( Stream * aStream, boost::optional<tAddress> aPrevious, boost::optional<tAddress> aNew) {
  if (aPrevious) {
    DBG_(VVerb, ( << "Clearing old FwdPtr entry for " << *aStream  << " at address @" << &std::hex << *aPrevious << & std::dec) );
    //Remove the old head
    theFwdPtrMap.erase(*aPrevious);
  }
  if (aNew) {
    DBG_(VVerb, ( << "Setting FwdPtr entry for " << *aStream << " to " << &std::hex << *aNew << &std::dec ) );
    std::map< tAddress, Stream * >::iterator iter;
    iter = theFwdPtrMap.find( *aNew );
    if (iter == theFwdPtrMap.end()) {
      theFwdPtrMap.insert( std::make_pair( *aNew, aStream) );
    } else {
      bool is_new;
      iter->second->terminate( eKilledByReDemand ); //Recursively calls setFwdPtr clearing the old ptr.
      boost::tie(iter, is_new) = theFwdPtrMap.insert( std::make_pair( *aNew, aStream) );
      DBG_Assert(is_new);
    }
  }
}

SORDLocation BaseStreamManager::end(tMRPVector aVector) { return theSORD.end(aVector); }

void BaseStreamManager::finalizeStreams() {
  DBG_(Iface, ( <<"Finalizing streams for " << theConsumer) );

  theNodeStats[theConsumer].theMaxLiveStreams << theLiveStreams;
  theOverallStats.theMaxLiveStreams << theLiveStreams;

  while (! theStreams.empty()) {
    theStreams.front()->terminate( eKilledByEndOfInput );
  }
}

void GlobalStreamManager::finalizeStreams() {
  theTailStreams.clear();
}

void GlobalStreamManager::removeLiveStream(boost::intrusive_ptr<Stream> aStream) {
  --theLiveStreams;
  DBG_(VVerb, ( << "P[" << theSORD.producer() << "]->C[" << theConsumer << "] StreamManager decrements theLiveStreams to " << theLiveStreams ) );

  //If the dying stream is the tail stream, deal with it
  std::map<tMRPVector,boost::intrusive_ptr<Stream> >::iterator tail_stream = theTailStreams.find(aStream->mrpVector());
  if (tail_stream != theTailStreams.end()) {
    if (tail_stream->second == aStream) {
      theTailStreams.erase(tail_stream);
    }
  }

  theStreams.remove(aStream);
  DBG_Assert( theLiveStreams >= 0);
}

void PerConsStreamManager::removeLiveStream(boost::intrusive_ptr<Stream> aStream) {
  --theLiveStreams;
  DBG_(VVerb, ( << "P[" << theSORD.producer() << "]->C[" << theConsumer << "] StreamManager decrements theLiveStreams to " << theLiveStreams ) );

  //If the dying stream is the tail stream, deal with it
  if(aStream == theGlobalTailStream) {
    theGlobalTailStream = 0;
  }
  if(aStream == thePerConsumerTailStream) {
    thePerConsumerTailStream = 0;
  }

  theStreams.remove(aStream);
  DBG_Assert( theLiveStreams >= 0);
}


void BaseStreamManager::forward( SORDLocation aLocation, boost::intrusive_ptr<Stream> aStream, long aChunkId) {
  DBG_( Iface, ( << "Cache.Forward " << *aLocation << " on " << *aStream) );

  aLocation->markConsumed(consumer());
  theForwardQueue.forward(aLocation->address(), aStream, aChunkId);
}

void GlobalStreamManager::invalidate(SORDLocation aLocation) {
  DBG_(VVerb, ( << "C[" << theConsumer << "] StreamManager.Invalidate @" << *aLocation ) );

  //Invalidation of a mispredicted SORDLocation does not affect streams
  if (aLocation->mispredicted()) {
    //Ensure that there are in fact no FwdPtr's pointing to this location (there
    //shouldn't be)
    std::map< tAddress, Stream * >::iterator iter = theFwdPtrMap.find(aLocation->address());
    DBG_Assert( iter == theFwdPtrMap.end() ) ;
  } else {
    //See if this affects our tail stream
      std::map<tMRPVector,boost::intrusive_ptr<Stream> >::iterator tail_stream = theTailStreams.find(aLocation->mrpVector());
      if (tail_stream != theTailStreams.end()) {
        if (! tail_stream->second->tail()) {
          if (aLocation->index() > tail_stream->second->fwdPtr()->index()) {
            theTailStreams.erase(tail_stream);

            //The Invalidate has caused our current tail stream to no longer be the tail.
            DBG_(VVerb, ( << "C[" << theConsumer << "] StreamManager.Invalidate.AffectsTail @" << *aLocation ) );

            DBG_(VVerb, ( << "C[" << theConsumer << "] Create on Invalidate may have done something.") );
          }
        }
      }

    //Pass the invalidate to all streams, to see if it kill them
    std::map< tAddress, Stream * >::iterator iter = theFwdPtrMap.find(aLocation->address());
    if (iter != theFwdPtrMap.end()) {
      iter->second->invalidate(aLocation->address());
    }
  }
}

void PerConsStreamManager::invalidate(SORDLocation aLocation) {
  DBG_(VVerb, ( << "C[" << theConsumer << "] StreamManager.Invalidate @" << *aLocation ) );

  //Invalidation of a mispredicted SORDLocation does not affect streams
  if (aLocation->mispredicted()) {
    //Ensure that there are in fact no FwdPtr's pointing to this location (there
    //shouldn't be)
    std::map< tAddress, Stream * >::iterator iter = theFwdPtrMap.find(aLocation->address());
    DBG_Assert( iter == theFwdPtrMap.end() ) ;
  } else {
    //Pass the invalidate to all streams, to see if it kill them
    std::map< tAddress, Stream * >::iterator iter = theFwdPtrMap.find(aLocation->address());
    if (iter != theFwdPtrMap.end()) {
      iter->second->invalidate(aLocation->address());
    }
  }
}


void BaseStreamManager::mispredict(SORDLocation aLocation) {
  DBG_(VVerb, ( << "C[" << theConsumer << "] StreamManager.Mispredict @" << *aLocation ) );

  //On a mispredict, if a stream is poiting at the mispredicted block,
  //we move the head
  std::map< tAddress, Stream * >::iterator iter = theFwdPtrMap.find(aLocation->address());
  if (iter != theFwdPtrMap.end()) {
    iter->second->mispredict();
  }
}

void GlobalStreamManager::produce(SORDLocation aLocation, unsigned short aVector) {
  std::map<tMRPVector,boost::intrusive_ptr<Stream> >::iterator tail_stream = theTailStreams.find(aVector);
  if (tail_stream != theTailStreams.end()) {
    DBG_Assert( ! tail_stream->second->dead() );

    DBG_(VVerb, ( << "C[" << theConsumer << "] StreamManager.Produce.TailStream "  << *aLocation << " on " << *tail_stream->second ) );
    tail_stream->second->produce( aLocation );
  }
}

void PerConsStreamManager::produce(SORDLocation aLocation, unsigned short aTemplate) {
  if(aTemplate == 16) {
    if(theGlobalTailStream) {
      DBG_Assert( ! theGlobalTailStream->dead() );

      DBG_(Iface, ( << "C[" << theConsumer << "] StreamManager.Produce.GlobalTailStream "  << *aLocation << " on " << *theGlobalTailStream ) );
      theGlobalTailStream->produce( aLocation );
    }
  }
  if(aTemplate == theConsumer) {
    if(thePerConsumerTailStream) {
      DBG_Assert( ! thePerConsumerTailStream->dead() );

      DBG_(Iface, ( << "C[" << theConsumer << "] StreamManager.Produce.GlobalTailStream "  << *aLocation << " on " << *thePerConsumerTailStream ) );
      thePerConsumerTailStream->produce( aLocation );
    }
  }
}

void GlobalStreamManager::demandStream(SORDLocation aLocation) {
  DBG_(VVerb, ( << "C[" << theConsumer << "] DemandStream "  << *aLocation ) );

  //Demand stream at aLocation

  //Find the block that would be the head of this stream.
  SORDLocation head = aLocation;
  ++head;
  while ( (head != theSORD.end(aLocation->mrpVector())) && ( head->mispredicted() ) ) {
    ++head;
  }
  if (head == theSORD.end(aLocation->mrpVector())) {
    //Demand for tail
    newStream(head, aLocation->mrpVector(), theHeadChunkSize);
  } else {
    if (head->valid() && ! head->hasConsumed(theConsumer)) {
      //Demand for a stream
      newStream(head, aLocation->mrpVector(), theHeadChunkSize);
    } else {
      //Demand for a single block - can't start a stream
      DBG_(Iface, ( << "C[" << theConsumer << "] Demand.Stillborn "  << *aLocation ) );
    }
  }
}

void PerConsStreamManager::demandStream(SORDLocation aLocation) {
    DBG_(Iface, ( << "C[" << theConsumer << "] DemandStream "  << *aLocation ) );

    //Find the block that would be the head of this stream.
    SORDLocation head = aLocation;
    ++head;
    while ( (head != theSORD.end(aLocation->mrpVector())) && ( head->mispredicted() ) ) {
      ++head;
    }
    if (head == theSORD.end(aLocation->streamTemplate())) {
      //See if we already have a tail
      if (  (aLocation->streamTemplate() == 16 && theGlobalTailStream)
         || (aLocation->streamTemplate() == theConsumer && thePerConsumerTailStream)
         ) {
        //Don't do anything - already have a tail stream
      } else {
        //Demand for tail
        newStream(head, aLocation->streamTemplate(), true);

      }
    } else {
      if (head->valid() && ! head->hasConsumed(theConsumer)) {
        //Demand for a stream
        newStream(head, aLocation->streamTemplate(), false);
      } else {
        //Demand for a single block - can't start a stream
        DBG_(Iface, ( << "C[" << theConsumer << "] Demand.Stillborn "  << *aLocation ) );
      }
    }
}

void GlobalStreamManager::newStream(SORDLocation aLocation, tMRPVector aVector, char aWindow) {
  //Need to create a new stream pointing to this location.

  //Allocate a Stream
    boost::intrusive_ptr<Stream> new_stream;
      if (aVector & (1 << theConsumer)) {
        new_stream = new Stream(aVector, theNextStreamID++, aLocation, aWindow, theBodyChunkSize, *this, StrDemandInMask);
        if (aLocation == theSORD.end(aVector) ) {
          DBG_( Iface, ( << "C[" << theConsumer << "]  Demand (in MRP) @tail - New Demand Stream: " << *new_stream ));
        } else {
          DBG_( Iface, ( << "C[" << theConsumer << "]  Demand (in MRP) " << *aLocation <<  " - New Demand Stream: " << *new_stream ));
        }
        theNodeStats[theConsumer].theDemandInMaskStreams++;
        theOverallStats.theDemandInMaskStreams++;
      } else {
        if (aVector == 0) {
          new_stream = new Stream(aVector, theNextStreamID++, aLocation, aWindow, theBodyChunkSize, *this, StrDemandNoMask);
          if (aLocation == theSORD.end(aVector) ) {
            DBG_( Iface, ( << "C[" << theConsumer << "]  Demand (no Mask) @tail - New Demand Stream: " << *new_stream ));
          } else {
            DBG_( Iface, ( << "C[" << theConsumer << "]  Demand (no Mask) " << *aLocation <<  " - New Demand Stream: " << *new_stream ));
          }

          theNodeStats[theConsumer].theDemandNoMaskStreams++;
          theOverallStats.theDemandNoMaskStreams++;
        } else {
          new_stream = new Stream(aVector, theNextStreamID++, aLocation, aWindow, theBodyChunkSize, *this, StrDemandNotInMask);
          DBG_( Iface, ( << "C[" << theConsumer << "]  Demand (not in MRP) " << *aLocation <<  " - New Demand Stream: " << *new_stream ));
          theNodeStats[theConsumer].theDemandNotInMaskStreams++;
          theOverallStats.theDemandNotInMaskStreams++;
        }
      }

    theStreams.push_back( new_stream );
    ++theLiveStreams;
    DBG_(VVerb, ( << "P[" << theSORD.producer() << "]->C[" << theConsumer << "] StreamManager increments theLiveStreams to " << theLiveStreams ) );


  //See if the new stream should be the tail stream
    std::map<tMRPVector,boost::intrusive_ptr<Stream> >::iterator tail_stream = theTailStreams.find(aVector);
    if (tail_stream == theTailStreams.end()) {
      //No previous tail stream, so this is the one.
      theTailStreams.insert( std::make_pair( aVector, new_stream) );
    } else {
      if (! tail_stream->second->tail()) {
        if ( (aLocation == theSORD.end(aVector)) ||  (aLocation->index() > tail_stream->second->fwdPtr()->index()) ) {
          tail_stream->second = new_stream;
        }
      }
    }

  //Perform the initial forwarding from the stream to the queue
    new_stream->forward();

  //Update stats
    theNodeStats[theConsumer].theCreatedStreams++;
    theOverallStats.theCreatedStreams++;


  theNodeStats[theConsumer].theMaxLiveStreams << theLiveStreams;
  theOverallStats.theMaxLiveStreams << theLiveStreams;
}

void PerConsStreamManager::newStream(SORDLocation aLocation, unsigned char aTemplate, bool isTail ) {
  //Need to create a new stream pointing to this location.

  //Allocate a Stream
    boost::intrusive_ptr<Stream> new_stream;
    StreamType type = ( aTemplate == 16 ? StrGlobal : StrPerConsumer) ;
    if (aTemplate == 16) {
      new_stream = new Stream(aTemplate, theNextStreamID++, aLocation, theHeadChunkSize, theBodyChunkSize, *this, StrGlobal);
      DBG_( Iface, ( << "C[" << theConsumer << "]  Demand " << *aLocation <<  " - New Global Stream: " << *new_stream ));
      theNodeStats[theConsumer].theGlobalStreams++;
      theOverallStats.theGlobalStreams++;
      if (isTail) {
        DBG_Assert(!theGlobalTailStream);
        theGlobalTailStream = new_stream;
      }
    } else {
      DBG_Assert(aTemplate == theConsumer);
      new_stream = new Stream(aTemplate, theNextStreamID++, aLocation, theHeadChunkSize, theBodyChunkSize, *this, StrPerConsumer);
      DBG_( Iface, ( << "C[" << theConsumer << "]  Demand " << *aLocation <<  " - New PerConsumer Stream: " << *new_stream ));
      theNodeStats[theConsumer].thePerConsumerStreams++;
      theOverallStats.thePerConsumerStreams++;
      if (isTail) {
        DBG_Assert(!thePerConsumerTailStream);
        thePerConsumerTailStream = new_stream;
      }
    }

    theStreams.push_back( new_stream );
    ++theLiveStreams;
    DBG_(VVerb, ( << "P[" << theSORD.producer() << "]->C[" << theConsumer << "] StreamManager increments theLiveStreams to " << theLiveStreams ) );


  //Perform the initial forwarding from the stream to the queue
    new_stream->forward();

  //Update stats
    theNodeStats[theConsumer].theCreatedStreams++;
    theOverallStats.theCreatedStreams++;


  theNodeStats[theConsumer].theMaxLiveStreams << theLiveStreams;
  theOverallStats.theMaxLiveStreams << theLiveStreams;
}


void PerConsStreamManager::setGlobalTailStream( boost::intrusive_ptr<Stream> aStream) {
  if (!aStream) {
    DBG_Assert( theGlobalTailStream );
    theGlobalTailStream = 0;
  } else {
    if (theGlobalTailStream) {
      //Already have a tail stream
      aStream->terminate(eKilledByReDemand);
    } else {
      theGlobalTailStream = aStream;
    }
  }
}

void PerConsStreamManager::setPerConsumerTailStream( boost::intrusive_ptr<Stream> aStream) {
  if (!aStream) {
    DBG_Assert( thePerConsumerTailStream );
    thePerConsumerTailStream = 0;
  } else {
    if (thePerConsumerTailStream) {
      //Already have a tail stream
      aStream->terminate(eKilledByReDemand);
    } else {
      thePerConsumerTailStream = aStream;
    }
  }
}

} //end namespace nSordManager

