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

#include <iomanip>


#include <boost/optional.hpp>
#include <core/boost_extensions/padded_string_cast.hpp>
using boost::padded_string_cast;

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include "common.hpp"
#include "ordergroupmgr.hpp"
#include "ordermgr.hpp"
#include "pull_stream.hpp"
#include "multi_stream.hpp"

#define __STDC_CONSTANT_MACROS
#include <boost/date_time/posix_time/posix_time.hpp>

unsigned long theGlobalBlockSize; //used by multi_strem when ENABLE_SPATIAL_GROUPS

OrderGroupMgr::OrderGroupMgr(std::string aName, long aCapacityPerOrder, unsigned long aBlockSize, bool anAllowLiveStreams, bool aKillOnConsumed, bool aKillOnInvalidate, int aNumOrders, int anId, int aNumRecent, long aDeltas, Predicate anAppendPredicate, Predicate anInvalidatePredicate, SplitFn aSplitFn, boost::optional<std::string> aLoadState, boost::optional<std::string> aPreLoadState, Directory * aDirectory)
  : theName(aName)
  , theNumOrders(aNumOrders)
  , theNumRecent(aNumRecent)
  , theDeltas(aDeltas)
  , theAppendPredicate(anAppendPredicate)
  , theInvalidatePredicate(anInvalidatePredicate)
  , theSplitFn(aSplitFn)
  , theDirectory(aDirectory)
  , theBlockSize(aBlockSize)
  , theBlockMask( ~( theBlockSize - 1 ) )
{
  theGlobalBlockSize = theBlockSize;

  DBG_Assert(theNumRecent > 0);
  if(theDeltas > 0) {
    DBG_Assert(theNumOrders == 1);
    DBG_Assert(theNumRecent == 1);
  }

  for (int i = 0 ; i < aNumOrders; ++i) {
    int id = i;
    if(aNumOrders == 1) {
      id = anId;
    }
    theOrders.push_back(new OrderMgr( aName + "[" + fill<2>(i) + "]", id, aCapacityPerOrder, anAllowLiveStreams, aKillOnConsumed, aKillOnInvalidate) );
  }

  #ifndef ENABLE_PC_LOOKUP
    if (aPreLoadState) {
      for (int i = 0; i < aNumOrders; ++i) {
        theOrders[i]->load(*aPreLoadState);
      }
      for (int i = 0; i < aNumOrders; ++i) {
        loadReflector(*aPreLoadState, i);
      }
    }

    if (aLoadState) {
      for (int i = 0; i < aNumOrders; ++i) {
        theOrders[i]->load(*aLoadState);
      }
      for (int i = 0; i < aNumOrders; ++i) {
        loadReflector(*aLoadState, i);
      }
    }
  #endif //ENABLE_PC_LOOKUP

}

OrderGroupMgr::mrc_table_t::iterator OrderGroupMgr::findMR( tAddress anAddress, tAddress aPC) {
  #ifdef ENABLE_PC_LOOKUP
    return theMostRecentAppends.find( std::make_pair( anAddress, aPC ) );
  #else //!ENABLE_PC_LOOKUP
    return theMostRecentAppends.find( anAddress );
  #endif //ENABLE_PC_LOOKUP
}

OrderGroupMgr::mrc_table_t::iterator OrderGroupMgr::insertMR( tAddress anAddress, tAddress aPC) {
  #ifdef ENABLE_PC_LOOKUP
   return theMostRecentAppends.insert( std::make_pair( std::make_pair( anAddress, aPC), std::list<OrderMgr*>()) ).first;
  #else //!ENABLE_PC_LOOKUP
   return theMostRecentAppends.insert( std::make_pair(anAddress, std::list<OrderMgr*>()) ).first;
  #endif //ENABLE_PC_LOOKUP
}

void OrderGroupMgr::event( TraceData & anEvent) {
#ifdef ENABLE_ENTRY_STATE
  if (theInvalidatePredicate( anEvent) ) {
    std::vector< OrderMgr * >::iterator iter;
    for(iter = theOrders.begin(); iter != theOrders.end(); ++iter) {
      (*iter)->invalidate(anEvent.theAddress);
    }
  }
#endif //ENABLE_ENTRY_STATE

  if (theAppendPredicate( anEvent) ) {
    int order = theSplitFn(anEvent);
#ifdef ENABLE_ENTRY_STATE
    DBG_Assert(anEvent.theWasConsumed);
#endif //ENABLE_ENTRY_STATE
    tAddress delta = anEvent.theAddress - theOrders[order]->mostRecentAppend();

#ifdef ENABLE_SPATIAL_LOOKUP
    tAddress addr = anEvent.theAddress & SPATIAL_GROUP_MASK;
#else //!ENABLE_SPATIAL_LOOKUP
    tAddress addr = anEvent.theAddress;
#endif //ENABLE_SPATIAL_LOOKUP

#ifndef ENABLE_BO_LOOKUP
    addr = addr & theBlockMask;
#endif //ENABLE_BO_LOOKUP


#ifdef ENABLE_PC_TRACKING
    long long newSeqNo = theOrders[order]->append(addr, false, anEvent.thePC);
#elif ENABLE_ENTRY_STATE
    long long newSeqNo = theOrders[order]->append(addr, anEvent.theWasHit, anEvent.thePC);
#else //ENABLE_ENTRY_STATE
    long long newSeqNo = theOrders[order]->append(addr, false, 0);
#endif //ENABLE_ENTRY_STATE

    if(theNumOrders > 1) {
      mrc_table_t::iterator iter = findMR( addr, anEvent.thePC );
      if(iter != theMostRecentAppends.end()) {
        while(iter->second.size() >= theNumRecent) {
          iter->second.pop_back();
        }
        iter->second.push_front( theOrders[order] );
      }
      else {
        iter = insertMR( addr, anEvent.thePC );
        iter->second.push_front( theOrders[order] );
      }
    } else {

      #ifdef ENABLE_DELTAS
        if(theDeltas > 0) {
          delta_table::index<by_delta>::type::iterator iter = theDeltaIndex.get<by_delta>().find(delta);
          if(iter != theDeltaIndex.get<by_delta>().end()) {
            DeltaEntry temp(*iter);
            theDeltaIndex.get<by_delta>().erase(iter);
            temp.theSequenceNo = newSeqNo;
            theDeltaIndex.get<by_LRU>().push_front(temp);
          }
          else {
            while(theDeltaIndex.size() >= theDeltas) {
              theDeltaIndex.get<by_LRU>().pop_back();
            }
            theDeltaIndex.get<by_LRU>().push_front( DeltaEntry(delta,newSeqNo) );
          }
        }
      #endif //ENABLE_DELTAS

    }

  }
}

void OrderGroupMgr::addAllAlternatives( boost::shared_ptr<MultiStream> aStream, tAddress anAddress) {
  //For all orders
  for (int i = 0; i < theNumOrders; ++i) {
    OrderMgr::address_iterator first, end;
    boost::tie(first, end) = theOrders[i]->equal_range(anAddress);
    while (first != end) {
      DBG_( Trace, ( << "   Alternative from " << theOrders[i]->name() << " #" << first->sequenceNo() << std::dec ) );
      aStream->addAltLocation( std::make_pair( theOrders[i]->getIterator(first), theOrders[i]->getEnd() ) );
      ++first;
    }
  }
}

boost::shared_ptr<MultiStream> OrderGroupMgr::multiRequest(MultiReceiver * aReceiver, tAddress aRequestedAddress, unsigned long aRequestPC) {
  boost::shared_ptr<MultiStream> stream( new MultiStream(aReceiver, aRequestedAddress, aRequestPC) );

#ifndef ENABLE_BO_LOOKUP
    aRequestedAddress = aRequestedAddress & theBlockMask;
#endif //ENABLE_BO_LOOKUP

#ifdef ENABLE_SPATIAL_LOOKUP
    aRequestedAddress = aRequestedAddress & SPATIAL_GROUP_MASK;
#endif //ENABLE_SPATIAL_LOOKUP

  //Find MRC occerence of address
  mrc_table_t::iterator mrc_iter = findMR( aRequestedAddress, aRequestPC );
  if (mrc_iter != theMostRecentAppends.end() && ! mrc_iter->second.empty()) {
    OrderMgr * order = mrc_iter->second.front();
    OrderMgr::address_iterator iter = order->find(aRequestedAddress, aRequestPC);
    if (iter != order->end() ) {
      //Stick the possible starting locations into the multi-stream
      stream->setMRC( order, iter->sequenceNo() );

      #ifdef ENABLE_2MRC
        std::list<OrderMgr *>::iterator two_iter = ++( mrc_iter->second.begin());
        if (two_iter != mrc_iter->second.end()) {
          OrderMgr * second = *two_iter;
          if (second == order) {
            //same order - use second occurrence
            ++iter;
            if (iter != order->end() && iter->address() == aRequestedAddress) {
              stream->set2MRC( order, iter->sequenceNo() );
            }
          } else {
            //distict order - find most recent occurence
            OrderMgr::address_iterator second_iter = second->find(aRequestedAddress, aRequestPC);
            if (second_iter != second->end() ) {
              stream->set2MRC( second, second_iter->sequenceNo() );
            }
          }
        }
      #endif //ENABLE_2MRC

      DBG_(Trace, ( << " MultiRequest @" << std::hex << aRequestedAddress << " %" << aRequestPC << std::dec << " Stream: " << *stream) );

      #ifdef ENABLE_ALTERNATIVE_SEARCH
        addAllAlternatives( stream, aRequestedAddress );
      #endif //ENABLE_ALTERNATIVE_SEARCH

      return stream;
    }
  }

  //Could not create stream
  DBG_(Trace, ( << " MultiRequest @" << std::hex << aRequestedAddress << std::dec << " No-Stream") );
  return boost::shared_ptr<MultiStream>();
}

