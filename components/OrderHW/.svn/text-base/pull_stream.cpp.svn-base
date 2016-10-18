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
#include "pull_stream.hpp"
#include "ordermgr.hpp"

#ifdef ENABLE_DEPRECATED

#include <boost/utility.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace l = boost::lambda;

#include <sstream>
#include <fstream>

StatCounter overallHitsExcludingFirst("overall.HitsExcluding1");
StatCounter totalForwardedBlocks("overall.StreamForwards");
StatCounter totalHitBlocks("overall.StreamHits");
StatCounter totalStreamsCreated("overall.StreamsCreated");
StatCounter totalStreamsCreatedOneOrder("overall.StreamsCreatedOneOrder");
StatCounter totalStreamsCreatedTwoOrder("overall.StreamsCreatedTwoOrder");
StatCounter totalStreamsTerminated("overall.StreamsTerminated");
StatLog2Histogram totalStreamHits("overall.HitsPerStream");
StatLog2Histogram totalStreamForwards("overall.ForwardsPerStream");
StatWeightedLog2Histogram totalWeightedStreamHits("overall.WeightedHitsPerStream");
StatWeightedLog2Histogram totalWeightedStreamForwards("overall.WeightedForwardsPerStream");

#ifdef ENABLE_OLD_STATS
  StatLog2Histogram totalHitDistToHeadAtHit("overall.HitDistToHeadAtHit");
  StatLog2Histogram totalHitDistToHeadAtFwd("overall.HitDistToHeadAtFwd");
  StatCounter totalStride2Hit("overall.Stride2Hit");
  StatCounter totalStride3Hit("overall.Stride3Hit");
#endif //ENABLE_OLD_STATS

extern StatInstanceCounter<long long> totalStreamLength;

unsigned long kPullStreamBlockSize = 0;

std::ostream & operator << ( std::ostream & anOstream, PullStream & aStream) {
  DBG_Assert(aStream.theNumOrders > 0);
  anOstream << "Stream: from " << aStream.theOrders[0]->name();
  for(int ii = 1; ii < aStream.theNumOrders; ii++) {
    anOstream << "," << aStream.theOrders[ii]->name();
  }
  anOstream << " to " << aStream.theReceiver->name();
  return anOstream;
}

PullStream::PullStream( PrefetchReceiver * aReceiver, OrderMgr * order, long long sequenceNo, tAddress aCreatePC, tAddress aMissAddress )
 : theReceiver(aReceiver)
 , theFinalized(false)
 , theDead(false)
 , theInitBackwardSize(0)
 , theInitForwardSize(0)
 , theBodyForwardSize(0)
 , theStreamMaxBlocks(0)
 , theWindow(0)
 , theAggressive(false)
 , theUseDelta(false)
 , theNumOrders(0)
 , theGoodOrder(-1)
 , theCreatePC(aCreatePC)
 , theMissAddress(aMissAddress)
 , theFirstHitAddress(0)
 , theDeltaAddress(aMissAddress)
 , thePrev3Address(0)
 , thePrev2Address(0)
 , thePrev1Address(0)
 , theForwardedBlocks(0)
 , theHitBlocks(0)
{

  ++totalStreamsCreated;

  add(order, sequenceNo);
}

void PullStream::add(OrderMgr * order, long long sequenceNo) {
  theOrders.push_back(order);
  theOrigSequenceNos.push_back(sequenceNo);
  theCurrSequenceNos.push_back(sequenceNo);
  theNumOrders++;
}

PullStream::~PullStream() {
  finalize();
}

void PullStream::finalize() {
  if(!theFinalized) {
    DBG_(Iface, ( << "Terminated Pull Stream" ) );
    ++totalStreamsTerminated;

    totalStreamLength << std::make_pair(theHitBlocks, 1LL);
  }
  theFinalized = true;
}

void PullStream::forwardBlock(tAddress anAddress, long aStreamIndex, long long aSequenceNo, long long aHeadSequenceNo) {
  DBG_(Iface, ( << *this << " forwarding off order " << aStreamIndex << " at " << aSequenceNo ) );

  bool accepted = theReceiver->forward(anAddress, shared_from_this(), aStreamIndex, aSequenceNo, aHeadSequenceNo);
  if(accepted) {
    totalStreamForwards << -1 * static_cast<long long>(theForwardedBlocks);
    totalWeightedStreamForwards >> std::make_pair( static_cast<long long>(theForwardedBlocks), static_cast<long long>(theForwardedBlocks) );
    ++theForwardedBlocks; ++totalForwardedBlocks;
    totalStreamForwards << static_cast<long long>(theForwardedBlocks);
    totalWeightedStreamForwards << std::make_pair( static_cast<long long>(theForwardedBlocks), static_cast<long long>(theForwardedBlocks) );
  }
}

void PullStream::forwardGoodStream(long aCurrBlocks, long aForwardSize) {
  // if the consumer reported negative blocks currently in the cache, we should
  // not forward more blocks (i.e. this stream should die)
  if(aCurrBlocks >= 0) {
    if(theCurrSequenceNos[theGoodOrder] != 0) {
      //Either forward an entire chunk, or just enough to reach the consumer's limit
      long blocks = aForwardSize;
      if( blocks > (theStreamMaxBlocks - aCurrBlocks) ) {
        blocks = theStreamMaxBlocks - aCurrBlocks;
      }

      if(blocks > 0) {
        bool killStream;
        std::set< std::pair<tAddress,long long> > forwards;

        if(theUseDelta) {
          boost::tie( theDeltaAddress, theCurrSequenceNos[theGoodOrder], forwards, killStream) =
            theOrders[theGoodOrder]->nextDeltas(theDeltaAddress, theCurrSequenceNos[theGoodOrder], blocks);
        }
        else {
          boost::tie( theCurrSequenceNos[theGoodOrder], forwards, killStream) =
            theOrders[theGoodOrder]->nextChunk(theReceiver->id(), theCurrSequenceNos[theGoodOrder], blocks, 0);
        }

        while (! forwards.empty()) {
          tAddress addr = (*forwards.begin()).first;
          long long seqNo = (*forwards.begin()).second;
          forwardBlock(addr, theGoodOrder, seqNo, theOrders[theGoodOrder]->nextSequenceNo());
          forwards.erase(forwards.begin());
        }
      }

    }
  }
}

void PullStream::internalHit(tAddress anAddress, long aStreamIndex, long long aSequenceNo, long long aHeadSequenceNo) {
  totalStreamHits << -1 * static_cast<long long>(theHitBlocks);
  totalWeightedStreamHits >> std::make_pair( static_cast<long long>(theHitBlocks), static_cast<long long>(theHitBlocks) );

  ++theHitBlocks; ++totalHitBlocks;
  if(theHitBlocks == 1) {
    theFirstHitAddress = anAddress;
  }
  else {
    ++overallHitsExcludingFirst;
  }

  totalStreamHits << static_cast<long long>(theHitBlocks);
  totalWeightedStreamHits << std::make_pair( static_cast<long long>(theHitBlocks), static_cast<long long>(theHitBlocks) );
  OLD_STAT( totalHitDistToHeadAtHit << (theOrders[aStreamIndex]->nextSequenceNo() - aSequenceNo); )
  OLD_STAT( totalHitDistToHeadAtFwd << (aHeadSequenceNo - aSequenceNo); )

  #ifdef ENABLE_OLD_STATS
    long long diff23 = thePrev2Address - thePrev3Address;
    long long diff12 = thePrev1Address - thePrev2Address;
    long long diffCurr = anAddress - thePrev1Address;

    if( (thePrev1Address != 0) && (thePrev2Address != 0) ) {
      if(diff12 == diffCurr) {
        totalStride2Hit++;
        if(thePrev3Address != 0) {
          if(diff23 == diff12) {
            totalStride3Hit++;
          }
        }
      }
    }
  #endif //ENABLE_OLD_STATS

  thePrev3Address = thePrev2Address;
  thePrev2Address = thePrev1Address;
  thePrev1Address = anAddress;
}

void PullStream::notifyHit(tAddress anAddress, tVAddress aPC, long aStreamIndex, long long aSequenceNo, long long aHeadSequenceNo, long aCurrBlocks) {
  DBG_(Iface, ( << *this << " hit on order " << aStreamIndex << " at " << aSequenceNo ) );

  internalHit(anAddress, aStreamIndex, aSequenceNo, aHeadSequenceNo);
  if(theWindow == 0) {
    DBG_Assert(aStreamIndex == theGoodOrder);
    forwardGoodStream(aCurrBlocks, theBodyForwardSize);
  } else {
    if(theGoodOrder >= 0) {
      forwardGoodStream(aCurrBlocks, theBodyForwardSize);
    } else {
      forwardWindow(aCurrBlocks);
    }
  }
}

bool PullStream::notifyMiss(tAddress anAddress, tVAddress aPC) {
  tAddress block_addr = anAddress & ~(kPullStreamBlockSize - 1);
  long foundGoodOrder = -1;
  long long foundGoodSeqNo = 0;
  bool handled = false;

  if(theGoodOrder < 0) {
    for(int ii = 0; ii < theNumOrders; ii++) {
      std::vector<WindowEntry>::iterator iter = theWindows[ii].begin();
      while(iter != theWindows[ii].end()) {
        if(iter->address == block_addr) {
          if(!iter->common) {
            if(foundGoodOrder >= 0) {
              foundGoodOrder = theNumOrders;
            } else {
              foundGoodOrder = ii;
              foundGoodSeqNo = iter->sequenceNo;
            }
          } else {
            //theWindows[ii].erase(theWindows[ii].begin(), ++iter);
            theWindows[ii].erase(iter);
          }
          handled = true;
          break;
        }
        ++iter;
      }
    }

    if( (foundGoodOrder >= 0) && (foundGoodOrder < theNumOrders) ) {
      theGoodOrder = foundGoodOrder;
      theCurrSequenceNos[theGoodOrder] = foundGoodSeqNo;
      forwardGoodStream(theForwardedBlocks-theHitBlocks, theBodyForwardSize);
    } else {
      fillWindow(false);
      forwardWindow(theForwardedBlocks-theHitBlocks);
    }
  }

  return handled;
}

void PullStream::beginForward() {
  DBG_(Iface, ( << *this << " beginning to forward, window = " << theWindow ) );

  if(theWindow == 0) {
    theGoodOrder = 0;
    forwardGoodStream(0, theInitForwardSize);
  } else {
    fillWindow(true);
    forwardWindow(0);
  }
}

void PullStream::fillWindow(bool init) {
  DBG_Assert(theWindow > 0);
  int ii;

  if(init) {
    theWindows.resize(theNumOrders);
  }

  for(ii = 0; ii < theNumOrders; ii++) {
    if(theWindows[ii].size() < theWindow) {
      std::list<tAddress> newAddrs;
      int numNew = theWindow - theWindows[ii].size();
      numNew = theOrders[ii]->extract(theCurrSequenceNos[ii]+1, numNew, newAddrs);

      while(numNew > 0) {
        theCurrSequenceNos[ii]++;

        tAddress block_addr = newAddrs.front() & ~(kPullStreamBlockSize - 1);
        theWindows[ii].push_back( WindowEntry( block_addr,theCurrSequenceNos[ii]) );
        newAddrs.pop_front();
        numNew--;
      }
    }
  }

  // done flling the window - update all common flags
  compareWindows();
  if(!theIntersection.empty()) {
    for(int ii = 0; ii < theNumOrders; ii++) {
      bool markCommon = false;
      std::vector<WindowEntry>::iterator iter = theWindows[ii].end();
      while(iter != theWindows[ii].begin()) {
        --iter;
        if(markCommon) {
          iter->common = true;
        } else {
          if(theIntersection.find(iter->address) != theIntersection.end()) {
            iter->common = true;
            markCommon = true;
          }
        }
      }
    }
  }

}

void PullStream::compareWindows() {
  int ii;
  unsigned int jj;
  //theUnion.clear();
  theIntersection.clear();

  std::vector< std::set<tAddress> > windowAddresses;
  windowAddresses.resize(theNumOrders);
  for(ii = 0; ii < theNumOrders; ii++) {
    for(jj = 0; jj < theWindows[ii].size(); jj++) {
      windowAddresses[ii].insert(theWindows[ii][jj].address);
    }
  }

  if(theNumOrders == 1) {
    //theUnion = windowAddresses[0];
  }
  else if(theNumOrders == 2) {
    //set_union( windowAddresses[0].begin(), windowAddresses[0].end(),
    //           windowAddresses[1].begin(), windowAddresses[1].end(),
    //           std::inserter(theUnion, theUnion.begin()) );
    set_intersection( windowAddresses[0].begin(), windowAddresses[0].end(),
                      windowAddresses[1].begin(), windowAddresses[1].end(),
                      std::inserter(theIntersection, theIntersection.begin()) );
  }
  else {
    std::set<tAddress> temp;
    set_intersection( windowAddresses[0].begin(), windowAddresses[0].end(),
                      windowAddresses[1].begin(), windowAddresses[1].end(),
                      std::inserter(temp, temp.begin()) );
    for(ii = 2; ii < theNumOrders; ii++) {
      if(ii != 2) {
        temp = theIntersection;
        theIntersection.clear();
      }
      set_intersection( temp.begin(), temp.end(),
                        windowAddresses[ii].begin(), windowAddresses[ii].end(),
                        std::inserter(theIntersection, theIntersection.begin()) );
    }
  }
}

void PullStream::advanceWindowPtr(long & order, long & depth, long initial) {
  order++;
  if(order == theNumOrders) {
    order = 0;
  }
  if(order == initial) {
    depth++;
  }
}

void PullStream::forwardWindow(long aCurrBlocks) {
  DBG_Assert(theWindow > 0);

  // walk through the windows
  int blocks = theStreamMaxBlocks - aCurrBlocks;

  long initialOrder = 0;
  while(blocks > 0) {
    long order = 0;  // start with order #0
    long depth = 0;  // start at the beginning

    // generate fresh union and intersection data
    compareWindows();

    // fast exit condition
    if(theIntersection.empty()) {
      break;
    }

    // now walk through all window entries
    bool forwarded = false;
    while( (depth<theWindow) && !forwarded ) {
      if(depth >= theWindows[order].size()) {
        advanceWindowPtr(order, depth, initialOrder);
        continue;
      }

      tAddress addr = theWindows[order][depth].address;
      if(theAggressive) {
        // aggressive
        DBG_Assert(!theIntersection.empty());
        long whichOrder = order;
        if(theIntersection.find(addr) != theIntersection.end()) {
          whichOrder = theNumOrders;
        }
        forwardAddressFromWindow(addr, whichOrder);
        forwarded = true;
      }
      else {
        // conservative
        if(theIntersection.find(addr) != theIntersection.end()) {
          forwardAddressFromWindow(addr, theNumOrders);
          forwarded = true;
        }
      }

      advanceWindowPtr(order, depth, initialOrder);
    }  // end while (depth < window)

    // if anything comes out of a window, refill
    if(forwarded) {
      --blocks;
      fillWindow(false);
    } else {
      // otherwise we're done
      break;
    }

    // rotate the initial order for fairness
    initialOrder++;
    if(initialOrder == theNumOrders) {
      initialOrder = 0;
    }
  }

}

void PullStream::forwardAddressFromWindow(tAddress anAddress, long window) {
  bool forwarded = false;

  // look through all windows
  for(int ii = 0; ii < theNumOrders; ii++) {
    std::vector<WindowEntry>::iterator iter = theWindows[ii].begin();
    while(iter != theWindows[ii].end()) {
      if(iter->address == anAddress) {
        if(!forwarded) {
          forwardBlock(anAddress, ii, iter->sequenceNo, theOrders[ii]->nextSequenceNo());
          forwarded = true;
        }
        theWindows[ii].erase(iter);
        break;
      }
      ++iter;
    }
  }

  DBG_Assert(forwarded);
}

#else //!ENABLE_DEPRECATED

std::ostream & operator << ( std::ostream & anOstream, PullStream & aStream) {
  return anOstream;
}

PullStream::PullStream( PrefetchReceiver * aReceiver, OrderMgr * order, long long sequenceNo, tAddress aCreatePC, tAddress aMissAddress )
 : theReceiver(aReceiver)
 , theFinalized(false)
 , theDead(false)
 , theInitBackwardSize(0)
 , theInitForwardSize(0)
 , theBodyForwardSize(0)
 , theStreamMaxBlocks(0)
 , theWindow(0)
 , theAggressive(false)
 , theUseDelta(false)
 , theNumOrders(0)
 , theGoodOrder(-1)
 , theCreatePC(aCreatePC)
 , theMissAddress(aMissAddress)
 , theFirstHitAddress(0)
 , theDeltaAddress(aMissAddress)
 , thePrev3Address(0)
 , thePrev2Address(0)
 , thePrev1Address(0)
 , theForwardedBlocks(0)
 , theHitBlocks(0)
{
}

void PullStream::add(OrderMgr * order, long long sequenceNo) {
}

PullStream::~PullStream() {
}

void PullStream::finalize() {
}

void PullStream::forwardBlock(tAddress anAddress, long aStreamIndex, long long aSequenceNo, long long aHeadSequenceNo) {
}

void PullStream::forwardGoodStream(long aCurrBlocks, long aForwardSize) {
}

void PullStream::internalHit(tAddress anAddress, long aStreamIndex, long long aSequenceNo, long long aHeadSequenceNo) {
}

void PullStream::notifyHit(tAddress anAddress, tVAddress aPC, long aStreamIndex, long long aSequenceNo, long long aHeadSequenceNo, long aCurrBlocks) {
}

bool PullStream::notifyMiss(tAddress anAddress, tVAddress aPC) {
  return false;
}

void PullStream::beginForward() {
}

void PullStream::fillWindow(bool init) {
}

void PullStream::compareWindows() {
}

void PullStream::advanceWindowPtr(long & order, long & depth, long initial) {
}

void PullStream::forwardWindow(long aCurrBlocks) {
}

void PullStream::forwardAddressFromWindow(tAddress anAddress, long window) {
}

#endif //ENABLE_DEPRECATED
