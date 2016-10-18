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

#include "ordermgr.hpp"

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace l = boost::lambda;

#define __STDC_CONSTANT_MACROS
#include <boost/date_time/posix_time/posix_time.hpp>

#include <core/boost_extensions/padded_string_cast.hpp>
using boost::padded_string_cast;

using Flexus::SharedTypes::PhysicalMemoryAddress;


#ifdef ENABLE_PC_TRACKING
 std::ostream & operator << ( std::ostream & anOstream, OrderEntry const & anOrderEntry) {
     anOstream <<  '@' << &std::hex << anOrderEntry.theAddress << "%" << anOrderEntry.thePC << &std::dec << "[" << anOrderEntry.theSequenceNo << "]";
     return anOstream;
 }
#else //!ENABLE_PC_TRACKING
 std::ostream & operator << ( std::ostream & anOstream, OrderEntry const & anOrderEntry) {
     anOstream <<  '@' << &std::hex << anOrderEntry.theAddress << &std::dec << "[" << anOrderEntry.theSequenceNo << "]";
     return anOstream;
 }
#endif //ENABLE_PC_TRACKING


OrderMgr::OrderMgr( std::string aName, int anId, long aCapacity, bool anAllowListeners, bool aKillOnConsumed, bool aKillOnInvalidate)
 : theName(aName)
 , theId(anId)
 , theCapacity(aCapacity)
 , theAllowListeners(anAllowListeners)
 , theKillOnConsumed(aKillOnConsumed)
 , theKillOnInvalidate(aKillOnInvalidate)
 , theNextSequenceNo(1)
 , theTotalAdditions( aName + "." + "Appends" )
 {}



long long OrderMgr::append( tAddress anAddress, bool wasHit, unsigned long aPC ) {
  if (theOrdering.size() >= theCapacity) {
    theOrdering.pop_front();
  }
  long long seq_no = theNextSequenceNo;
  ++theNextSequenceNo;
#ifdef ENABLE_PC_TRACKING
  theOrdering.push_back( OrderEntry(anAddress, seq_no, aPC));
#else
  theOrdering.push_back( OrderEntry(anAddress, seq_no, wasHit));
#endif //ENABLE_PC_TRACKING
  theTotalAdditions++;
  if (theAllowListeners && !theAppendListeners.empty()) {
    //Must work on a copy in case a listner removes themselves in call to appended
    typedef std::map< int, boost::shared_ptr<AppendListener> > listen_map;
    listen_map copy(theAppendListeners);
    listen_map::iterator iter = copy.begin();
    listen_map::iterator end = copy.end();
    while (iter != end) {
      iter->second->appended(anAddress, seq_no, theNextSequenceNo);
      ++iter;
    }
  }
  return seq_no;
}


OrderMgr::iterator OrderMgr::getIterator(OrderMgr::address_iterator anIterator) {
  return theOrdering.project<by_sequence_no>(anIterator);
}

OrderMgr::iterator OrderMgr::getIterator(long long aSequenceNo) {
  typedef order_table::index<by_sequence_no>::type sequence_index_t;
  sequence_index_t const & sequence_index = theOrdering.get<by_sequence_no>();

  return sequence_index.find( aSequenceNo );
}

OrderMgr::iterator OrderMgr::getEnd() {
  typedef order_table::index<by_sequence_no>::type sequence_index_t;
  sequence_index_t const & sequence_index = theOrdering.get<by_sequence_no>();

  return sequence_index.end();
}

std::pair
  < OrderMgr::address_iterator
  , OrderMgr::address_iterator
  > OrderMgr::equal_range(tAddress anAddress) {
  typedef order_table::index<by_address>::type address_index_t;
  address_index_t const & address_index = theOrdering.get<by_address>();

  return address_index.equal_range( boost::make_tuple(anAddress) );
}

#ifdef ENABLE_PC_LOOKUP
  std::pair
    < OrderMgr::address_iterator
    , OrderMgr::address_iterator
    > OrderMgr::equal_range(tAddress anAddress, tAddress aPC) {
    typedef order_table::index<by_address>::type address_index_t;
    address_index_t const & address_index = theOrdering.get<by_address>();

    return address_index.equal_range( boost::make_tuple(anAddress, aPC) );
  }
#endif //ENABLE_PC_LOOKUP

long long OrderMgr::nextSequenceNo() {
  return theNextSequenceNo;
}

tAddress OrderMgr::mostRecentAppend() {
  if(!theOrdering.empty()) {
    return theOrdering.back().address();
  }
  return 0;
}

OrderMgr::address_iterator OrderMgr::find(tAddress anAddress, tAddress aPC) {
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
  } else {
    addr_iter = address_index.end();
  }

  if (addr_iter != address_index.end() && addr_iter->address() == anAddress) {
    return addr_iter;
  } else {
    return address_index.end();
  }
}


