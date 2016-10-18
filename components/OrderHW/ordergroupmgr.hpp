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

#ifndef ORDERGROUPMGR_INCLUDED
#define ORDERGROUPMGR_INCLUDED

#include "common.hpp"

#include <set>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/member.hpp>

using namespace boost::multi_index;

#include <ext/hash_map>

namespace __gnu_cxx {
  template<> struct hash< std::pair<tAddress, tAddress> >
  { size_t operator()(std::pair<tAddress, tAddress> __x) const { return __x.first + __x.second; } };
}

class Stream;
class PullStream;
class MultiStream;
class OrderMgr;


using Flexus::SharedTypes::PhysicalMemoryAddress;

struct ReflectorEntry {
  unsigned int theMRC;
  long theLocation;
  ReflectorEntry()
    : theMRC(0)
    , theLocation(0)
    {}
  ReflectorEntry(int anMRC, long aLocation)
    : theMRC(anMRC)
    , theLocation(aLocation)
    {}
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & theMRC;
    ar & theLocation;
  }
};

#ifdef ENABLE_DELTAS
  struct DeltaEntry {
    long theDelta;
    long long theSequenceNo;
    DeltaEntry(tAddress aDelta, long long aSeq)
      : theDelta(aDelta)
      , theSequenceNo(aSeq)
    {}
  };

  typedef boost::multi_index_container
    < DeltaEntry
    , indexed_by
        < sequenced< tag<by_LRU> >
        , ordered_unique
            < tag<by_delta>
            , member<DeltaEntry,long,&DeltaEntry::theDelta>
            >
        >
    > delta_table;
#endif //ENABLE_DELTAS


typedef std::map< PhysicalMemoryAddress, ReflectorEntry > reflector_map;

class OrderGroupMgr : public TraceProcessor {
    std::string theName;
    int theNumOrders;
    int theNumRecent;
    long theDeltas;
#ifdef ENABLE_PC_LOOKUP
    typedef __gnu_cxx::hash_map<std::pair<tAddress, tAddress>, std::list<OrderMgr *> > mrc_table_t;
#else //!ENABLE_PC_LOOKUP
    typedef std::map< tAddress, std::list<OrderMgr*> > mrc_table_t;
#endif //ENABLE_PC_LOOKUP

    mrc_table_t theMostRecentAppends;
    std::vector< OrderMgr * > theOrders;
    Predicate theAppendPredicate;
    Predicate theInvalidatePredicate;
    SplitFn theSplitFn;

    #ifdef ENABLE_DELTAS
      delta_table theDeltaIndex;
    #endif //ENABLE_DELTAS

    reflector_map theSaveMaps[16];  //Used to save state
    reflector_map theSave2Maps[16]; //Used to save state
    Directory * theDirectory;       //Used to save state

    unsigned long theBlockSize;
    unsigned long theBlockMask;

  public:
    OrderGroupMgr(std::string aName, long aCapacityPerOrder, unsigned long aBlockSize, bool anAllowLiveStreams, bool aKillOnConsumed, bool aKillOnInvalidate, int aNumOrders, int anId, int aNumRecent, long aDeltas, Predicate anAppendPredicate, Predicate anInvalidatePredicate, SplitFn aSplitFn, boost::optional<std::string> aLoadState = boost::optional<std::string>(), boost::optional<std::string> aPreLoadState = boost::optional<std::string>(), Directory * aDirectory = 0);
    virtual ~OrderGroupMgr();
    virtual void finalize();

    virtual void event( TraceData & );
    boost::shared_ptr<PullStream> pullRequest(PrefetchReceiver *, tAddress aRequestedAddress, tAddress aRequestPC);
    boost::shared_ptr<MultiStream> multiRequest(MultiReceiver * aReceiver, tAddress aRequestedAddress, tAddress aRequestPC);

    void addAllAlternatives( boost::shared_ptr<MultiStream> aStream, tAddress anAddress);

    mrc_table_t::iterator findMR( tAddress anAddress, tAddress aPC);
    mrc_table_t::iterator insertMR( tAddress anAddress, tAddress aPC);

#ifndef ENABLE_PC_LOOKUP
    void loadReflector(std::string aDirName, int anId);
    void save( std::string const & aDirName, Directory * aDirectory );
    void saveReflector(std::string aDirName, int anId);
    void reflect( tAddress anAdrress, int anMRC, long aLocation );
    void reconstructTwo();
#endif //!ENABLE_PC_LOOKUP


};

#endif //ORDERGROUPMGR_INCLUDED
