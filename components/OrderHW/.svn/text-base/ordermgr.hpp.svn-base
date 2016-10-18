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

#ifndef ORDERMGR_INCLUDED
#define ORDERMGR_INCLUDED

#include "common.hpp"


#include <boost/function.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>

using namespace boost::multi_index;


class OrderMgr {
    std::string theName;
    int theId;

    typedef multi_index_container
      < OrderEntry
      , indexed_by
          < sequenced< tag<by_location> >
          , ordered_unique
              < tag<by_sequence_no>
              , member<OrderEntry,long long,&OrderEntry::theSequenceNo>
              >
#ifdef ENABLE_PC_LOOKUP
          , ordered_unique
              < tag<by_address>
              , composite_key
                  < OrderEntry
                  , member<OrderEntry,tAddress,&OrderEntry::theAddress>
                  , member<OrderEntry,tVAddress,&OrderEntry::thePC>
                  , member<OrderEntry,long long,&OrderEntry::theSequenceNo>
                  >
              >
          >
#else //!ENABLE_PC_LOOKUP
          , ordered_unique
              < tag<by_address>
              , composite_key
                  < OrderEntry
                  , member<OrderEntry,tAddress,&OrderEntry::theAddress>
                  , member<OrderEntry,long long,&OrderEntry::theSequenceNo>
                  >
              >
          >
#endif //ENABLE_PC_LOOKUP
      > order_table;

    order_table theOrdering;
    unsigned long theCapacity;
    bool theAllowListeners;
    bool theKillOnConsumed;
    bool theKillOnInvalidate;
    long long theNextSequenceNo;

    StatCounter theTotalAdditions;

    std::map< int, boost::shared_ptr<AppendListener> > theAppendListeners;

  public:
    typedef order_table::index<by_sequence_no>::type::iterator iterator;
    typedef order_table::index<by_address>::type::iterator address_iterator;

    OrderMgr( std::string aName, int anId, long aCapacity, bool anAllowListeners, bool aKillOnConsumed, bool aKillOnInvalidate);

    std::string const & name() const { return theName; }
    int id() const { return theId; }

    long long nextSequenceNo();
    tAddress mostRecentAppend();
    long long append( tAddress anAddress, bool wasHit, unsigned long aPC);

    long long getSequenceNo(tAddress anAddress, unsigned int aLookBack);
    long long getSequenceNo(tAddress anAddress, tAddress aPC, unsigned int aLookBack);

    tAddress getAddress(long long aSequenceNo);

    address_iterator find(tAddress anAddress, tAddress aPC); //PC is ignored if !ENABLE_PC_LOOKUP
    address_iterator end() { return theOrdering.get<by_address>().end(); }

    std::pair<address_iterator, address_iterator> equal_range(tAddress anAddress);
#ifdef ENABLE_PC_LOOKUP
    std::pair<address_iterator, address_iterator> equal_range(tAddress anAddress, tAddress aPC );
#endif //ENABLE_PC_LOOKUP

    iterator getIterator(long long aSequenceNo);
    iterator getIterator(address_iterator anIterator);
    iterator getEnd();

    ~OrderMgr();
    void finalize();

    long extract(long long aSequenceNo, long aMaximum, std::list<tAddress> & aList);
    boost::tuple< long long, std::set< std::pair<tAddress,long long> >, bool >
      nextChunk(tID aNode, long long aLocation, int aForwardChunkSize, int aBackwardChunkSize = 0);
    boost::tuple< tAddress, long long, std::set< std::pair<tAddress,long long> >, bool >
      nextDeltas(tAddress aStartAddress, long long aLocation, int aForwardChunkSize);

    void listen( int id, boost::shared_ptr<AppendListener> aListener );
    void unlisten( int id );

#ifdef ENABLE_ENTRY_STATE
    void invalidate( tAddress anAddress);
    void upgrade( tAddress anAddress);

    void markConsumed(tID aNode, long long aLocation);
    bool hasConsumed(tID aNode, long long aLocation);

    bool wasHit(long long aSequenceNo);
    bool wasProduced(long long aSequenceNo);
    long long findPrevMiss(long long aSequenceNo);
    long long findNextMiss(long long aSequenceNo);
#endif //ENABLE_ENTRY_STATE

    void load(std::string const & aDirName);
    void save(std::string const & aDirName, boost::function< void (tAddress, int, long)  > aReflectFn) const;

};

#endif //ORDERMGR_INCLUDED
