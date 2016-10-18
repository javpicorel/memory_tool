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
#include "stride_stream.hpp"

#ifdef ENABLE_STRIDE

#include <boost/utility.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace l = boost::lambda;

#include <sstream>
#include <fstream>



extern StatCounter totalForwardedBlocks;
extern StatCounter totalHitBlocks;
extern StatCounter totalStreamsCreated;
extern StatCounter totalStreamsTerminated;
extern StatLog2Histogram totalStreamHits;
extern StatLog2Histogram totalStreamForwards;
extern StatWeightedLog2Histogram totalWeightedStreamHits;
extern StatWeightedLog2Histogram totalWeightedStreamForwards;
StatCounter totalStrideStreams("overall.StrideStreams");
StatCounter totalStrideHitOnStream("overall.StrideHitOnStream");
StatCounter totalStrideMatchOnHit("overall.StrideMatchOnHit");
StatCounter totalStrideMatchOnMiss("overall.StrideMatchOnMiss");
StatCounter totalStrideNoMatchOnHit("overall.StrideNoMatchOnHit");
StatCounter totalStrideNoMatchOnMiss("overall.StrideNoMatchOnMiss");
StatCounter totalStrideOrphanedHits("overall.StrideOrphanedHits");
StatCounter totalStridePasses("overall.StridePasses");

StatCounter totalStrideConsumptions("overall.StrideConsumptions");
StatCounter totalStrideCacheHit("overall.StrideCacheHits");
StatCounter totalStrideCacheMiss("overall.StrideCacheMisses");



std::ostream & operator << ( std::ostream & anOstream, StrideStream & aStream) {
  anOstream << "Stride Stream: to " << aStream.theReceiver->name();
  for(unsigned int ii = 0; ii < aStream.theStreams.size(); ii++) {
    anOstream << "  " << ii << ":" << aStream.theStreams[ii];
  }
  return anOstream;
}

StrideStream::StrideStream( PrefetchReceiver * aReceiver, tAddress aMissAddress, tAddress aMissPC, long aMaxStreams, long aHistory, long aStreamLength )
 : theReceiver(aReceiver)
 , theFinalized(false)
 , theDead(false)
 , theMaxStreams(aMaxStreams)
 , theHistoryDepth(aHistory)
 , theStreamLength(aStreamLength)
 , theNextStrideIndex(0)
{
  DBG_(Iface, ( << "Stride Stream: to " << aReceiver->name() ) );

  ++totalStreamsCreated;

  theLastAddresses = new tAddress[theHistoryDepth];
  theMruList = new int[theMaxStreams];
  for(int ii = 0; ii < theMaxStreams; ii++) {
    theMruList[ii] = ii;
    theStreams.push_back( new Strider(theNextStrideIndex++, theStreamLength) );
  }

  addHistory(aMissAddress, aMissPC);
}

StrideStream::~StrideStream() {
  finalize();

  delete [] theLastAddresses;
  delete [] theMruList;
  for(int ii = 0; ii < theMaxStreams; ii++) {
    delete theStreams[ii];
  }
  theStreams.clear();
}

void StrideStream::finalize() {
  if(!theFinalized) {
    DBG_(Iface, ( << "Terminated Stride Stream" ) );
    ++totalStreamsTerminated;
  }
  theFinalized = true;
}

bool StrideStream::forwardBlock(tAddress anAddress, long aStrideIndex) {
  DBG_(Iface, ( << *this << " forwarding off stride " << aStrideIndex ) );

  return theReceiver->forward(anAddress, shared_from_this(), aStrideIndex, 0, 0);
}

void StrideStream::beginForward() {
  // do nothing
}

void StrideStream::addHistory(tAddress anAddress, tVAddress aPC) {
  ++totalStrideConsumptions;
  for(unsigned int ii = theHistoryDepth-1; ii > 0 ; ii--) {
    theLastAddresses[ii] = theLastAddresses[ii-1];
  }
  theLastAddresses[0] = anAddress;
}

void StrideStream::notifyHit(tAddress anAddress, tVAddress aPC, long aStrideIndex, long long aSequenceNo, long long aHeadSequenceNo, long aCurrBlocks) {
  addHistory(anAddress, aPC);
  ++totalStrideCacheHit;

  bool found = false;
  for(int ii = 0; ii < theMaxStreams; ii++) {
    if(theStreams[ii]->index() == aStrideIndex) {
      ++totalStrideHitOnStream;
      theStreams[ii]->hit();
      found = true;
      accessed(ii);

      if(theStreams[ii]->locate(anAddress)) {
        theStreams[ii]->advance(anAddress);
        theStreams[ii]->predict(this, anAddress);
      }
    }
  }

  if(!found) {
    ++totalStrideOrphanedHits;
    consume(anAddress, aPC, true);
  }
}

bool StrideStream::notifyMiss(tAddress anAddress, tVAddress aPC) {
  addHistory(anAddress, aPC);
  ++totalStrideCacheMiss;

  consume(anAddress, aPC, false);
  return true;
}

int StrideStream::victim() {
  return theMruList[theMaxStreams-1];
}

void StrideStream::accessed(int aStream) {
  int ii = 0;
  // find the list entry for the specified index
  while(theMruList[ii] != aStream) {
    ii++;
  }
  // move appropriate entries down the MRU chain
  while(ii > 0) {
    theMruList[ii] = theMruList[ii-1];
    ii--;
  }
  theMruList[0] = aStream;
}

void StrideStream::consume(tAddress anAddress, tVAddress aPC, bool wasHit) {
  // calculate stride from the history
  int currStride = -1;
  if( (theLastAddresses[0] - theLastAddresses[1]) == (theLastAddresses[1] - theLastAddresses[2]) ) {
    currStride = theLastAddresses[0] - theLastAddresses[1];
  }
  bool predictionMade = false;

  // first try to match this to a stream prediction
  bool matched = false;
  for(int ii = 0; ii < theMaxStreams; ii++) {
    if(theStreams[ii]->locate(anAddress)) {
      // match!
      matched = true;
      if(wasHit) {
        ++totalStrideMatchOnHit;
      } else {
        ++totalStrideMatchOnMiss;
      }
      theStreams[ii]->advance(anAddress);

      // if there is a current stride, only predict on this stream
      // if the strides match; if no current stride, always predict
      bool predict = false;
      if(currStride < 0) {
        predict = true;
      }
      else {
        if(currStride == theStreams[ii]->stride()) {
          predict = true;
        }
      }

      // make the prediction if we said we would
      if(predict) {
        predictionMade = true;
        theStreams[ii]->predict(this, anAddress);
        accessed(ii);  // mark this stream as MRU
      }
    }
  }

  if(!matched) {
    if(wasHit) {
      ++totalStrideNoMatchOnHit;
    } else {
      ++totalStrideNoMatchOnMiss;
    }
  }

  // check if a new stream should be created
  if((currStride >= 0) && (!predictionMade)) {
    ++totalStrideStreams;
    int idx = victim();
    theStreams[idx]->reset(theNextStrideIndex++, currStride);
    theStreams[idx]->predict(this, anAddress);
    accessed(idx);
  }
}

void StrideStream::invalidate(tAddress anAddress) {
  // invalidate any predictions for this address
  for(int ii = 0; ii < theMaxStreams; ii++) {
    if(theStreams[ii]->locate(anAddress)) {
      theStreams[ii]->invalidate(anAddress);
    }
  }
}



Strider::Strider(long anIndex, unsigned int aStreamLength)
  : myIndex(anIndex)
  , theStreamLength(aStreamLength)
  , myStride(0)
  , theForwardedBlocks(0)
  , theHitBlocks(0)
{}

bool Strider::locate(tAddress anAddress) {
  for(unsigned int ii = 0; ii < thePredictions.size(); ii++) {
    if(thePredictions[ii] == anAddress) {
      return true;
    }
  }
  return false;
}

void Strider::advance(tAddress anAddress) {
  // remove everything before the address we care about
  while(thePredictions.front() != anAddress) {
    ++totalStridePasses;
    thePredictions.pop_front();
  }
    // take this one off too
  thePredictions.pop_front();
}

void Strider::forward(StrideStream * aForwarder, tAddress anAddress) {
  bool accepted = aForwarder->forwardBlock(anAddress, myIndex);
  if(accepted) {
    totalStreamForwards << -1 * static_cast<long long>(theForwardedBlocks);
    totalWeightedStreamForwards >> std::make_pair( static_cast<long long>(theForwardedBlocks),
                                                   static_cast<long long>(theForwardedBlocks) );

    ++theForwardedBlocks; ++totalForwardedBlocks;

    totalStreamForwards << static_cast<long long>(theForwardedBlocks);
    totalWeightedStreamForwards << std::make_pair( static_cast<long long>(theForwardedBlocks),
                                                   static_cast<long long>(theForwardedBlocks) );
  }
}

void Strider::hit() {
  totalStreamHits << -1 * static_cast<long long>(theHitBlocks);
  totalWeightedStreamHits >> std::make_pair( static_cast<long long>(theHitBlocks),
                                             static_cast<long long>(theHitBlocks) );

  ++theHitBlocks; ++totalHitBlocks;

  totalStreamHits << static_cast<long long>(theHitBlocks);
  totalWeightedStreamHits << std::make_pair( static_cast<long long>(theHitBlocks),
                                             static_cast<long long>(theHitBlocks) );
}

void Strider::predict(StrideStream * aForwarder, tAddress anAddress) {
  // base prediction off the last address in the stream (or the
  // current address if the stream is empty)
  tAddress predAddress = anAddress;
  if(!thePredictions.empty()) {
    predAddress = thePredictions.back();
  }
  predAddress += myStride;

  // predict until the stream is full
  while(thePredictions.size() < theStreamLength) {
    forward(aForwarder, predAddress);
    thePredictions.push_back(predAddress);
    predAddress += myStride;
  }
}

void Strider::reset(long aNewIndex, int aStride) {
  myIndex = aNewIndex;
  myStride = aStride;
  theForwardedBlocks = 0;
  theHitBlocks = 0;
  while(!thePredictions.empty()) {
    thePredictions.pop_front();
  }
}

void Strider::invalidate(tAddress anAddress) {
  // remove everything before the address we care about
  while(thePredictions.front() != anAddress) {
    thePredictions.pop_front();
  }
  // take this one off too
  thePredictions.pop_front();
}

#endif //ENABLE_STRIDE
