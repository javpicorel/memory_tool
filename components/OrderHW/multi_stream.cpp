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
#include "multi_stream.hpp"
#include "ordermgr.hpp"

#include "stream_tracker.hpp"

#include <boost/utility.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace l = boost::lambda;

#include <sstream>
#include <fstream>

int theGlobalId = 1;


#ifdef ENABLE_COUNT_ALTERNATIVES
  StatInstanceCounter<long long> overallAlternativeLookups("overall.AlternativeLookups");
  StatInstanceCounter<long long> overallAlternativeBlocks("overall.AlternativeBlocks");
#endif //ENABLE_COUNT_ALTERNATIVES

StatInstanceCounter<long long> totalStreamLength("overall.StreamLength");


StatCounter overallStreamDiscards_NZHot("overall.StreamDiscards_NZ-HOT");
StatCounter overallStreamDiscards_NZOne("overall.StreamDiscards_NZ-ONE");
StatCounter overallStreamDiscards_NZMatch("overall.StreamDiscards_NZ-MATCH");
StatCounter overallStreamDiscards_NZMatch2Hot("overall.StreamDiscards_NZ-MATCH2HOT");
StatCounter overallStreamDiscards_ZOne("overall.StreamDiscards_Z-ONE");
StatCounter overallStreamDiscards_ZMatch("overall.StreamDiscards_Z-MATCH");
StatCounter overallStreamDiscards_ZMismatch("overall.StreamDiscards_Z-MISMATCH");

StatCounter overallStreamHits_NZHot1("overall.StreamHits_NZ-HOT1");
StatCounter overallStreamHits_NZHot2("overall.StreamHits_NZ-HOT2");
StatCounter overallStreamHits_NZOne("overall.StreamHits_NZ-ONE");
StatCounter overallStreamHits_NZMatch("overall.StreamHits_NZ-MATCH");

StatCounter overallStreams_SomeStream("overall.Streams_SomeStream");
StatCounter overallStreams_NZHot1("overall.Streams_NZ-HOT1");
StatCounter overallStreams_NZHot2("overall.Streams_NZ-HOT2");
StatCounter overallStreams_NZOne("overall.Streams_NZ-ONE");
StatCounter overallStreams_NZMatch("overall.Streams_NZ-MATCH");
StatCounter overallStreams_NZMatch_to_NZHot1("overall.Streams_NZ-MATCH_to_NZ-HOT1");
StatCounter overallStreams_NZMatch_to_NZHot2("overall.Streams_NZ-MATCH_to_NZ-HOT2");
StatCounter overallStreams_ZOne("overall.Streams_Z-ONE");
StatCounter overallStreams_ZMatch("overall.Streams_Z-MATCH");
StatCounter overallStreams_ZMismatch("overall.Streams_Z-MISMATCH");
StatCounter overallStreams_NoStream("overall.Streams_NoStream");

StatCounter overallWatchMatches_HOT1("overall.WatchMatches_HOT1");
StatCounter overallWatchMatches_HOT2("overall.WatchMatches_HOT2");
StatCounter overallWatchMatches_ONE("overall.WatchMatches_ONE");
StatCounter overallWatchDiscards_HOT1("overall.WatchDiscards_HOT1");
StatCounter overallWatchDiscards_HOT2("overall.WatchDiscards_HOT2");
StatCounter overallWatchDiscards_ONE("overall.WatchDiscards_ONE");


std::ostream & operator << ( std::ostream & anOstream, MultiStream & aStream) {
  anOstream << "MultiStream[" << aStream.id() <<"] to " << aStream.theReceiver->name();
  return anOstream;
}





MultiStream::MultiStream( MultiReceiver * aReceiver, tAddress aMissAddress, unsigned long aMissPC )
 : theReceiver(aReceiver)
 , theId(theGlobalId)
 , theTargetBlocks(0)
 , theCachedBlocks(0)
 , theMissAddress(aMissAddress)
#ifdef ENABLE_PC_TRACKING
 , theMissPC(aMissPC)
#endif //ENABLE_PC_TRACKING
 , theMRCOrder(0)
#ifdef ENABLE_2MRC
 , the2MRCOrder(0)
#endif //ENABLE_2MRC
 , theStreamType(eNonStream)
 , theMismatchPending(false)
 , theHits(0)
 , theDiscards(0)
{
  ++theGlobalId;
}

void MultiStream::setMRC(OrderMgr * order, long long sequenceNo) {
  theMRCOrder = order;
  theMRCForward = sequenceNo;

#ifdef ENABLE_PC_TRACKING
  theMRCPC = order->getIterator(sequenceNo)->pc();
#endif //ENABLE_PC_TRACKING
}

#ifdef ENABLE_2MRC
  void MultiStream::set2MRC(OrderMgr * order, long long sequenceNo) {
    the2MRCOrder = order;
    the2MRCForward = sequenceNo;
    DBG_Assert ( ! ( ( the2MRCOrder == theMRCOrder ) && (the2MRCForward == theMRCForward) ) );
  }
#endif //ENABLE_2MRC

void MultiStream::addAltLocation( std::pair< OrderMgr::iterator, OrderMgr::iterator > range ) {
  #ifdef ENABLE_ALTERNATIVE_SEARCH
    theAltLocations.push_back( range );
  #endif //ENABLE_ALTERNATIVE_SEARCH
}

MultiStream::~MultiStream() {
  finalize();
}

void MultiStream::finalize() {
  switch (theStreamType) {
    case eNonStream:
      ++overallStreams_NoStream;
      break;
    case eZOne:
      ++overallStreams_ZOne;
      ++overallStreams_SomeStream;
      DBG_Assert( theHits == 0 );
      overallStreamDiscards_ZOne += theDiscards;
      break;
    case eZMatch:
      ++overallStreams_ZMatch;
      ++overallStreams_SomeStream;
      DBG_Assert( theHits == 0 );
      overallStreamDiscards_ZMatch += theDiscards;
      break;
    case eZMismatch:
      ++overallStreams_ZMismatch;
      ++overallStreams_SomeStream;
      DBG_Assert( theHits == 0 );
      overallStreamDiscards_ZMismatch += theDiscards;
      break;
    case eNZHot1:
      ++overallStreams_NZHot1;
      ++overallStreams_SomeStream;
      overallStreamDiscards_NZHot += theDiscards;
      break;
    case eNZHot2:
      ++overallStreams_NZHot2;
      ++overallStreams_SomeStream;
      overallStreamDiscards_NZHot += theDiscards;
      break;
    case eNZOne:
      ++overallStreams_NZOne;
      ++overallStreams_SomeStream;
      overallStreamDiscards_NZOne += theDiscards;
      break;
    case eNZMatch:
      ++overallStreams_NZMatch;
      ++overallStreams_SomeStream;
      overallStreamDiscards_NZMatch += theDiscards;
      break;
    case eNZMatch_to_Hot1:
      ++overallStreams_NZMatch_to_NZHot1;
      ++overallStreams_SomeStream;
      overallStreamDiscards_NZMatch2Hot += theDiscards;
      break;
    case eNZMatch_to_Hot2:
      ++overallStreams_NZMatch_to_NZHot2;
      ++overallStreams_SomeStream;
      overallStreamDiscards_NZMatch2Hot += theDiscards;
      break;
  }

  if (theHits > 0) {
    //theTracker.addStream(theMissAddress, theHits);
    #ifdef ENABLE_STREAM_TRACKING
      theTracker.countStream(theStream,theReceiver->id());
    #endif //ENABLE_STREAM_TRACKING
    totalStreamLength << std::make_pair(theHits, 1LL);
  }

}

bool MultiStream::forwardBlock(tAddress anAddress, unsigned long aPC, eForwardType aFwdType) {
  DBG_(Trace, ( << *this << " forward @" << std::hex << anAddress << " %" << aPC << std::dec ) );

  return theReceiver->forwardTSE(anAddress, aPC, shared_from_this(), aFwdType);
}

#ifdef ENABLE_ALTERNATIVE_SEARCH
  void MultiStream::doAltForward() {
    #ifdef ENABLE_COUNT_ALTERNATIVES
      overallAlternativeLookups << std::make_pair(theAltLocations.size(), 1LL);
      DBG_(Trace, ( << *this << " " << theAltLocations.size() << " alternatives" ) );
    #endif //ENABLE_COUNT_ALTERNATIVES
    theReceiver->clearAlt();
    std::list< std::pair< OrderMgr::iterator, OrderMgr::iterator > >::iterator iter;
    for (iter = theAltLocations.begin(); iter != theAltLocations.end(); ++iter) {
      OrderMgr::iterator block_iter = iter->first;
      OrderMgr::iterator block_end = iter->second;

      if (block_iter != block_end) {
        #ifdef ENABLE_PC_TRACKING
          unsigned long head_pc = block_iter->pc();
        #endif //ENABLE_PC_TRACKING
        ++ block_iter; //Advance past the head block
        for (int i = 0; i < theTargetBlocks && block_iter != block_end; ++i, ++block_iter) {
          #ifdef ENABLE_PC_TRACKING
            theReceiver->forwardAlt(block_iter->address(), (head_pc == theMissPC) && (theMissPC != theMRCPC) );
            DBG_(Iface, ( << *this << " forward-alt @" << std::hex << block_iter->address() << " %" << block_iter->pc() << std::dec ) );
          #else //!ENABLE_PC_TRACKING
            theReceiver->forwardAlt(block_iter->address(), 0 );
            DBG_(Iface, ( << *this << " forward-alt @" << std::hex << block_iter->address() << std::dec ) );
          #endif //ENABLE_PC_TRACKING
        }
      }
    }
    #ifdef ENABLE_COUNT_ALTERNATIVES
      int alternative_blocks = theReceiver->altSize();
      overallAlternativeBlocks << std::make_pair(alternative_blocks, 1LL);
      DBG_(Trace, ( << *this << " " << alternative_blocks << " alternative blocks" ) );
    #endif //ENABLE_COUNT_ALTERNATIVES
  }
#else //!ENABLE_ALTERNATIVE_SEARCH
  void MultiStream::doAltForward() { }
#endif //ENABLE_ALTERNATIVE_SEARCH


void MultiStream::doMRCForward() {
  if (! theMRCOrder) { return; }
  OrderMgr::iterator iter = theMRCOrder->getIterator(theMRCForward + 1);
  while (iter != theMRCOrder->getEnd() && theCachedBlocks < theTargetBlocks) {
    theMRCForward = iter->sequenceNo();
    #ifdef ENABLE_SPATIAL_GROUPS
      for (tAddress addr = iter->address() & SPATIAL_GROUP_MASK; addr < (iter->address() & SPATIAL_GROUP_MASK) + SPATIAL_GROUP_SIZE; addr += theGlobalBlockSize ) {
        if (theStreamType == eNZOne ) {
          forwardBlock(addr, iter->pc(), eFwdOne);
        } else if (theStreamType == eNonStream ) {
          forwardBlock(addr, iter->pc(), eFwdWatchOne) );
          theStreamType = eZOne;
          break;
        } else {
          forwardBlock(addr, iter->pc(), eFwdHot));
        }
      }
      ++theCachedBlocks;
    #else //!ENABLE_SPATIAL_GROUPS
      if (theStreamType == eNZOne ) {
        forwardBlock(iter->address(), iter->pc(), eFwdOne);
        ++theCachedBlocks;
      } else if (theStreamType == eNonStream ) {
        forwardBlock(iter->address(), iter->pc(), eFwdWatch_One);
        ++theCachedBlocks;
        theStreamType = eZOne;
        break;
      } else {
        forwardBlock(iter->address(), iter->pc(), eFwdHot);
        ++theCachedBlocks;
      }
    #endif //ENABLE_SPATIAL_GROUPS
    ++iter;
  }
}


#ifdef ENABLE_2MRC
  void MultiStream::doForward() {
    if (the2MRCOrder == 0) {
      return doMRCForward();
    }

    if (theMismatchPending) { return; } //No forwards on mismatch until resolved

    OrderMgr::iterator mrc = theMRCOrder->getIterator(theMRCForward + 1);
    OrderMgr::iterator nmrc = the2MRCOrder->getIterator(the2MRCForward + 1);
    while (mrc != theMRCOrder->getEnd() && nmrc != the2MRCOrder->getEnd() && theCachedBlocks < theTargetBlocks) {
      if ( (mrc->address() ) == (nmrc->address() )) {
        if (theStreamType == eNonStream) {
          theStreamType = eZMatch;
        }
        theMRCForward = mrc->sequenceNo();
        the2MRCForward = nmrc->sequenceNo();
        forwardBlock(mrc->address(), mrc->pc(), eFwdMatch);
        ++theCachedBlocks;
        ++mrc;
        ++nmrc;
      } else {
        //Mismatch
        if (theStreamType == eNonStream) {
          theStreamType = eZMismatch;
        }
        theMismatchPending = true;

        //Forward Watch blocks
        theMRCForward = mrc->sequenceNo();
        the2MRCForward = nmrc->sequenceNo();
        forwardBlock(mrc->address(), mrc->pc(), eFwdWatch_Hot_1);
        forwardBlock(nmrc->address(), mrc->pc(), eFwdWatch_Hot_2);
        ++theCachedBlocks;
        break;
      }
    }

    if (mrc == theMRCOrder->getEnd() || nmrc == the2MRCOrder->getEnd() ) {
      theMRCOrder = 0;
      the2MRCOrder = 0;
      return;
    }

    //Handle bottom-of-stream conditions somewhat gracefully
    /*
    if (mrc == theMRCOrder->getEnd() && nmrc != the2MRCOrder->getEnd() ) {
      the2MRCOrder = 0;
      theMRCOrder = 0;
      return;
    }
    if (mrc != theMRCOrder->getEnd() && nmrc == the2MRCOrder->getEnd() ) {
      theMRCOrder = 0;
      theMRCOrder = the2MRCOrder;
      theMRCForward = the2MRCForward;
      the2MRCOrder = 0;
      return;
    }
    */
  }
#else
  void MultiStream::doForward() {
    doMRCForward();
  }
#endif //ENABLE_2MRC

void MultiStream::notifyInvalidate(tAddress anAddress, eForwardType aType) {
  switch (aType) {
    case eFwdWatch_Hot_1:
      ++overallWatchDiscards_HOT1;
      break;
    case eFwdWatch_Hot_2:
      ++overallWatchDiscards_HOT2;
      break;
    case eFwdWatch_One:
      ++overallWatchDiscards_ONE;
      break;
    case eFwdMatch:
    case eFwdHot:
    case eFwdOne:
      ++theDiscards;
      //--theCachedBlocks;
      break;
  }
}

void MultiStream::notifyReplace(tAddress anAddress, eForwardType aType) {
  notifyInvalidate(anAddress, aType);
}


void MultiStream::notifyHit(tAddress anAddress, eForwardType aType) {

#ifdef ENABLE_STREAM_TRACKING
  theStream.push_back(anAddress);
#endif

  switch (aType) {
    case eFwdWatch_Hot_1:
      //Select MRC
      if (theStreamType == eNZMatch) {
        theStreamType = eNZMatch_to_Hot1;
      } else {
        theStreamType = eNZHot1;
      }
      #ifdef ENABLE_2MRC
        the2MRCOrder = 0;
      #endif //ENABLE_2MRC
      ++overallWatchMatches_HOT1;
      theMismatchPending = false;
      break;
    case eFwdWatch_Hot_2:
      //Select MMRC
      #ifdef ENABLE_2MRC
        if (the2MRCOrder) {
          theMRCOrder = the2MRCOrder;
          theMRCForward = the2MRCForward;
          the2MRCOrder = 0;
        }
      #endif //ENABLE_2MRC
      if (theStreamType == eNZMatch) {
        theStreamType = eNZMatch_to_Hot2;
      } else {
        theStreamType = eNZHot2;
      }
      ++overallWatchMatches_HOT2;
      theMismatchPending = false;
      break;
    case eFwdWatch_One:
      ++overallWatchMatches_ONE;
      theStreamType = eNZOne;
      theMismatchPending = false;
      break;
    case eFwdMatch:
      if (theStreamType < eNZHot1) {
        theStreamType = eNZMatch;
      }
      ++overallStreamHits_NZMatch;
      ++theHits;
      break;
    case eFwdHot:
      if ( (theStreamType == eNZHot2) || (theStreamType == eNZMatch_to_Hot2)) {
        ++overallStreamHits_NZHot2;
      } else {
        ++overallStreamHits_NZHot1;
      }
      ++theHits;
      break;
    case eFwdOne:
      ++overallStreamHits_NZOne;
      ++theHits;
      break;
  }
  --theCachedBlocks;
  doForward();
}

void MultiStream::beginForward() {
  doAltForward();
  doForward();
}


