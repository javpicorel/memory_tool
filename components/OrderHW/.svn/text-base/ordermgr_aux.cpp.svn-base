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

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <fstream>

#include "ordermgr.hpp"

#include <boost/serialization/vector.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace l = boost::lambda;

#define __STDC_CONSTANT_MACROS
#include <boost/date_time/posix_time/posix_time.hpp>

#ifdef ENABLE_ENTRY_STATE
  StatCounter overallAttemptedUpgrades("overall.AttemptedOrderUpgrades");
  StatCounter overallPerformedUpgrades("overall.PerformedOrderUpgrades");
#endif //ENABLE_ENTRY_STATE

#include <core/boost_extensions/padded_string_cast.hpp>
using boost::padded_string_cast;

using Flexus::SharedTypes::PhysicalMemoryAddress;

OrderMgr::~OrderMgr() {
  boost::posix_time::ptime now(boost::posix_time::second_clock::local_time());
  DBG_(Dev, ( << "Destructing order manager " << theName << " at " << now ) );
  //theOrdering.clear();
  theAppendListeners.clear();
}

void OrderMgr::finalize() {
  DBG_(Dev, ( << "Finalizing order manager " << theName ) );
  std::map< int, boost::shared_ptr<AppendListener> >::iterator iter = theAppendListeners.begin();
  while(iter != theAppendListeners.end()) {
    iter->second->finalize();
    ++iter;
  }
}

long long OrderMgr::getSequenceNo(tAddress anAddress, unsigned int aLookBack) {
  //Obtain the address index
  typedef order_table::index<by_address>::type address_index_t;
  address_index_t const & address_index = theOrdering.get<by_address>();

  //Locate the entry for this address with the highest sequence number
  address_index_t::iterator addr_iter = address_index.upper_bound(boost::make_tuple(anAddress) );
  if (addr_iter != address_index.begin()) {
    //Note: need to back up one from upper_bound, since this points one past
    //the last item with anAddress.
    --addr_iter;

    while(aLookBack > 0) {
      --aLookBack;
      if (addr_iter != address_index.begin()) {
        --addr_iter;
      }
    }
  }

  if (addr_iter != address_index.end() && addr_iter->address() == anAddress) {
    return addr_iter->sequenceNo();
  } else {
    return 0;
  }
}


long long OrderMgr::getSequenceNo(tAddress anAddress, tAddress aPC, unsigned int aLookBack) {
  //Obtain the address index
  typedef order_table::index<by_address>::type address_index_t;
  address_index_t const & address_index = theOrdering.get<by_address>();

  //Locate the entry for this address with the highest sequence number
  #ifdef ENABLE_PC_LOOKUP
    address_index_t::iterator addr_iter = address_index.upper_bound(boost::make_tuple(anAddress, aPC) );
  #else //!ENABLE_PC_LOOKUP
    address_index_t::iterator addr_iter = address_index.upper_bound(boost::make_tuple(anAddress) );
  #endif //ENABLE_PC_LOOKUP
  if (addr_iter != address_index.begin()) {
    //Note: need to back up one from upper_bound, since this points one past
    //the last item with anAddress.
    --addr_iter;

    while(aLookBack > 0) {
      --aLookBack;
      if (addr_iter != address_index.begin()) {
        --addr_iter;
      }
    }
  }

  #ifdef ENABLE_PC_LOOKUP
  if (addr_iter != address_index.end() && addr_iter->address() == anAddress && addr_iter->pc() == aPC) {
  #else //!ENABLE_PC_LOOKUP
  if (addr_iter != address_index.end() && addr_iter->address() == anAddress) {
  #endif //ENABLE_PC_LOOKUP
    return addr_iter->sequenceNo();
  } else {
    return 0;
  }
}

tAddress OrderMgr::getAddress(long long aSequenceNo) {
  typedef order_table::index<by_sequence_no>::type sequence_index_t;
  sequence_index_t const & sequence_index = theOrdering.get<by_sequence_no>();
  sequence_index_t::iterator seq_iter = sequence_index.find( aSequenceNo );

  if (seq_iter != sequence_index.end()) {
    return seq_iter->theAddress;
  }

  return 0;
}

long OrderMgr::extract(long long aSequenceNo, long aMaximum, std::list<tAddress> & aList) {
  typedef order_table::index<by_sequence_no>::type sequence_index_t;
  sequence_index_t const & sequence_index = theOrdering.get<by_sequence_no>();
  sequence_index_t::iterator seq_iter = sequence_index.find( aSequenceNo );

  long count = 0;
  while( (aMaximum > 0) && (seq_iter != sequence_index.end()) ) {
    aList.push_back(seq_iter->theAddress);
    --aMaximum;
    ++count;
    ++seq_iter;
  }

  return count;
}

boost::tuple< long long, std::set< std::pair<tAddress,long long> >, bool >
OrderMgr::nextChunk(tID aNode, long long aLocation, int aForwardChunkSize, int aBackwardChunkSize) {
  bool kill_stream = false;

  //Obtain the sequence number index
  typedef order_table::index<by_sequence_no>::type sequence_index_t;
  sequence_index_t const & sequence_index = theOrdering.get<by_sequence_no>();

  //Locate the entry for this address with the highest sequence number
  sequence_index_t::iterator seq_iter = sequence_index.find( aLocation );

  if (seq_iter != sequence_index.end()) {
    //The sequence number still appears in the order

    //Record the sequence number
    long long next_sequence_no = aLocation;

    //Set of addresses and sequence numbers to be sent
    std::set< std::pair<tAddress,long long> > forward_set;

    //Project from the address index to the sequence index
    order_table::iterator order_iter = theOrdering.project<by_location>(seq_iter);
    order_table::iterator order_iter_back = order_iter;

    //If we go backwards from the head, accumulate the appropriate addresses
    while (aBackwardChunkSize > 0) {
      if (order_iter_back != theOrdering.get<by_location>().begin()) {
        --aBackwardChunkSize;
        --order_iter_back;
#ifdef ENABLE_ENTRY_STATE
        if (!theKillOnConsumed || ! order_iter_back->hasConsumed(aNode) ) {
          if (!theKillOnInvalidate || order_iter_back->isValid() ) {
            forward_set.insert( std::make_pair(order_iter_back->address(),order_iter_back->sequenceNo()) );
            order_iter_back->markConsumed(aNode);
          }
        }
#endif //ENABLE_ENTRY_STATE
      } else {
        break;
      }
    }

    //Advance one element past the previous sent element
    if (order_iter != theOrdering.get<by_location>().end()) {
#ifdef ENABLE_ENTRY_STATE
      order_iter->markConsumed(aNode);
#endif //ENABLE_ENTRY_STATE
      ++order_iter;
    }

    //Accumulate the addresses that appear forward in the order, and record
    //the sequence number for the next request
    while (aForwardChunkSize > 0) {
      if (order_iter != theOrdering.get<by_location>().end()) {
        --aForwardChunkSize;

#ifdef ENABLE_ENTRY_STATE
        if (theKillOnConsumed && order_iter->hasConsumed(aNode)) {
          kill_stream = true;
          break;
        } else if (theKillOnInvalidate && !order_iter->isValid()) {
          kill_stream = true;
          break;
        } else {
          forward_set.insert( std::make_pair(order_iter->address(),order_iter->sequenceNo()) );
          order_iter->markConsumed(aNode);
          ++next_sequence_no;
          ++order_iter;
        }
#else
        forward_set.insert( std::make_pair(order_iter->address(),order_iter->sequenceNo()) );
        ++next_sequence_no;
        ++order_iter;
#endif //ENABLE_ENTRY_STATE

      } else {
        break;
      }
    }

    /*
    DBG_(Verb, ( << "order " << theId << " verifying node " << aNode << " has consumed " << forward_set.size() << " items" ) );
    std::set< std::pair<tAddress,long long> >::iterator iter2;
    for(iter2 = forward_set.begin(); iter2 != forward_set.end(); ++iter2) {
      sequence_index_t::iterator seq_iter2 = sequence_index.find( iter2->second );
      DBG_Assert(seq_iter2->hasConsumed(aNode));
      DBG_(Verb, ( << "order " << theId << " verified node " << aNode << " has consumed seqNo " << iter2->second ) );
    }
    */

    if (forward_set.size() > 0) {
      return boost::make_tuple( next_sequence_no, forward_set, kill_stream );
    } else {
      return boost::make_tuple( 0, forward_set, kill_stream );
    }
  } else {
    return boost::make_tuple( 0, std::set< std::pair<tAddress,long long> >(), kill_stream );
  }
}

boost::tuple< tAddress, long long, std::set< std::pair<tAddress,long long> >, bool >
OrderMgr::nextDeltas(tAddress aStartAddress, long long aLocation, int aForwardChunkSize) {
  bool kill_stream = false;

  //Obtain the sequence number index
  typedef order_table::index<by_sequence_no>::type sequence_index_t;
  sequence_index_t const & sequence_index = theOrdering.get<by_sequence_no>();

  //Locate the entry for this address with the highest sequence number
  sequence_index_t::iterator seq_iter = sequence_index.find( aLocation );

  if (seq_iter != sequence_index.end()) {
    //The sequence number still appears in the order

    //Set of addresses and sequence numbers to be sent
    std::set< std::pair<tAddress,long long> > forward_set;

    //Project from the address index to the sequence index
    order_table::iterator order_iter = theOrdering.project<by_location>(seq_iter);

    //Remember the address at the current location
    tAddress prevAddress;
    if (order_iter != theOrdering.get<by_location>().end()) {
      prevAddress = order_iter->address();
      ++order_iter;
    }

    //Walk down the order, finding the delta between each pair of addresses,
    //and calculate successive forwarding addresses from the deltas
    while (aForwardChunkSize > 0) {
      if (order_iter != theOrdering.get<by_location>().end()) {
        --aForwardChunkSize;
        tAddress delta = order_iter->address() - prevAddress;
        aStartAddress += delta;
        forward_set.insert( std::make_pair(aStartAddress,order_iter->sequenceNo()) );
        prevAddress = order_iter->address();
        ++aLocation;
        ++order_iter;
      } else {
        break;
      }
    }

    if (forward_set.size() > 0) {
      return boost::make_tuple( aStartAddress, aLocation, forward_set, kill_stream );
    } else {
      return boost::make_tuple( aStartAddress, 0, forward_set, kill_stream );
    }
  } else {
    return boost::make_tuple( aStartAddress, 0, std::set< std::pair<tAddress,long long> >(), kill_stream );
  }
}


#ifdef ENABLE_ENTRY_STATE
  void OrderMgr::invalidate(tAddress anAddress) {
    //Obtain the address index
    typedef order_table::index<by_address>::type address_index_t;
    address_index_t const & address_index = theOrdering.get<by_address>();

    //Locate the entry for this address with the highest sequence number
    address_index_t::iterator addr_iter = address_index.upper_bound(boost::make_tuple(anAddress) );
    if (addr_iter != address_index.begin()) {
      //Note: need to back up one from upper_bound, since this points one past
      //the last item with anAddress.
      --addr_iter;
    }

    if (addr_iter != address_index.end() && addr_iter->address() == anAddress) {
      addr_iter->invalidate();
    }
  }

  void OrderMgr::upgrade(tAddress anAddress) {
    //Obtain the address index
    typedef order_table::index<by_address>::type address_index_t;
    address_index_t const & address_index = theOrdering.get<by_address>();

    //Locate the entry for this address with the highest sequence number
    address_index_t::iterator addr_iter = address_index.upper_bound(boost::make_tuple(anAddress) );
    if (addr_iter != address_index.begin()) {
      //Note: need to back up one from upper_bound, since this points one past
      //the last item with anAddress.
      --addr_iter;
    }

    overallAttemptedUpgrades++;
    if (addr_iter != address_index.end() && addr_iter->address() == anAddress) {
      addr_iter->produce();
      overallPerformedUpgrades++;
    }
  }
#endif //ENABLE_ENTRY_STATE



#ifdef ENABLE_ENTRY_STATE
  bool OrderMgr::hasConsumed(tID aNode, long long aLocation) {
    typedef order_table::index<by_sequence_no>::type sequence_index_t;
    sequence_index_t const & sequence_index = theOrdering.get<by_sequence_no>();
    sequence_index_t::iterator seq_iter = sequence_index.find( aLocation );
    if(seq_iter != sequence_index.end()) {
      return seq_iter->hasConsumed(aNode);
    }
    DBG_Assert(false, ( << theName << ": has node " << aNode << " consumed " << aLocation << "?" ) );
    return false;
  }

  void OrderMgr::markConsumed(tID aNode, long long aLocation) {
    typedef order_table::index<by_sequence_no>::type sequence_index_t;
    sequence_index_t const & sequence_index = theOrdering.get<by_sequence_no>();
    sequence_index_t::iterator seq_iter = sequence_index.find( aLocation );
    if(seq_iter != sequence_index.end()) {
      return seq_iter->markConsumed(aNode);
    }
    DBG_Assert(false, ( << theName << ": mark node " << aNode << " has consumed " << aLocation ) );
  }

  bool OrderMgr::wasHit(long long aSequenceNo) {
    typedef order_table::index<by_sequence_no>::type sequence_index_t;
    sequence_index_t const & sequence_index = theOrdering.get<by_sequence_no>();
    sequence_index_t::iterator seq_iter = sequence_index.find( aSequenceNo );

    DBG_Assert(seq_iter != sequence_index.end());
    return seq_iter->wasHit();
  }

  bool OrderMgr::wasProduced(long long aSequenceNo) {
    typedef order_table::index<by_sequence_no>::type sequence_index_t;
    sequence_index_t const & sequence_index = theOrdering.get<by_sequence_no>();
    sequence_index_t::iterator seq_iter = sequence_index.find( aSequenceNo );

    DBG_Assert(seq_iter != sequence_index.end());
    return seq_iter->wasProduced();
  }

  long long OrderMgr::findPrevMiss(long long aSequenceNo) {
    long long dist = 0;
    typedef order_table::index<by_sequence_no>::type sequence_index_t;
    sequence_index_t const & sequence_index = theOrdering.get<by_sequence_no>();
    sequence_index_t::iterator seq_iter = sequence_index.find( aSequenceNo );

    if(seq_iter != sequence_index.end()) {
      while(seq_iter != sequence_index.begin()) {
        --seq_iter;
        if(!seq_iter->wasHit()) {
          break;
        }
        dist++;
      }
    }
    return dist;
  }

  long long OrderMgr::findNextMiss(long long aSequenceNo) {
    long long dist = 0;
    typedef order_table::index<by_sequence_no>::type sequence_index_t;
    sequence_index_t const & sequence_index = theOrdering.get<by_sequence_no>();
    sequence_index_t::iterator seq_iter = sequence_index.find( aSequenceNo );

    if(seq_iter != sequence_index.end()) {
      ++seq_iter;
      while(seq_iter != sequence_index.end()) {
        if(!seq_iter->wasHit()) {
          break;
        }
        dist++;
        ++seq_iter;
      }
    }
    return dist;
  }
#endif //ENABLE_ENTRY_STATE

void OrderMgr::load(std::string const & aDirName) {
  std::vector< PhysicalMemoryAddress > theOrder;
  long theOrderTail;


  std::string fname(aDirName);
  fname += "/" + padded_string_cast<3,'0'>(theId) + "-consort";

  DBG_(Dev, ( << "order " << theId << " load state from " << fname) );
  std::ifstream ifs(fname.c_str(), std::ios::binary);
  boost::archive::binary_iarchive ia(ifs);

  ia >> theOrderTail;
  ia >> theOrder;
  // close archive
  ifs.close();

  long long count = 0;
  long long zeros = 0;
  static std::set<PhysicalMemoryAddress> addresses;

  DBG_(Dev, ( << "order " << theId << " order-size " << theOrder.size() << " tail " << theOrderTail ) );

  unsigned long i = theOrderTail + 1;
  DBG_Assert( static_cast<unsigned long>(theOrderTail) <= theOrder.size() );
  if (i > theOrder.size()) { i %= theOrder.size(); }
  while (i != static_cast<unsigned long>(theOrderTail)) {
    if (theOrder[i] != 0) {
      append( theOrder[i], false, 0 );
      addresses.insert(theOrder[i]);
      count++;
    } else {
      if (i < theOrderTail) {
        DBG_(Dev, ( << "zero in order" ) );
      }
      zeros++;
    }
    ++i;
    if (i > theOrder.size()) { i %= theOrder.size(); }
  }

  DBG_(Dev, ( << "order " << theId << " count " << count << " zeros " << zeros << " addresses " << addresses.size() ) );
}

void OrderMgr::save(std::string const & aDirName, boost::function< void( tAddress, int, long ) > aReflectFn) const {
  std::vector< PhysicalMemoryAddress > theOrder( theCapacity, PhysicalMemoryAddress(0) );
  long theOrderTail = 0;

  order_table::iterator iter = theOrdering.begin();
  order_table::iterator end = theOrdering.end();
  while (iter != end) {
    theOrder[theOrderTail] = PhysicalMemoryAddress(iter->theAddress);
    aReflectFn( iter->theAddress, theId, theOrderTail);
    ++theOrderTail;
    ++iter;
  }

  std::string fname(aDirName);
  fname += "/" + padded_string_cast<3,'0'>(theId) + "-consort";

  DBG_(Dev, ( << "order " << theId << " save state to " << fname) );

  std::ofstream ofs(fname.c_str(), std::ios::binary);
  boost::archive::binary_oarchive oa(ofs);

  oa << theOrderTail;
  oa << (const std::vector<PhysicalMemoryAddress>)theOrder;
  // close archive
  ofs.close();

}

void OrderMgr::listen( int id, boost::shared_ptr<AppendListener> aListener ) {
  theAppendListeners.insert( std::make_pair( id, aListener) );
}

void OrderMgr::unlisten( int id ) {
  theAppendListeners.erase( id );
}
