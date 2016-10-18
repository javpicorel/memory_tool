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
#ifndef MULTI_CACHE_INCLUDED
#define MULTI_CACHE_INCLUDED

#include "common.hpp"
#include "multi_stream.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
using namespace boost::multi_index;

#include <ext/hash_map>


struct MultiCacheEntry {
  tAddress theAddress;
  unsigned long thePC;
  mutable boost::shared_ptr<MultiStream> theStream;
  mutable eForwardType theFwdType;

  MultiCacheEntry(tAddress anAddress, tVAddress aPC, boost::shared_ptr<MultiStream> aStream, eForwardType aFwdType)
    : theAddress(anAddress)
    , thePC(aPC)
    , theStream(aStream)
    , theFwdType(aFwdType)
  {}

  void invalidate() const {
    theStream->notifyInvalidate(theAddress, theFwdType);
  }

  void replace() const {
    theStream->notifyReplace(theAddress, theFwdType);
  }
};


enum eSGPType
  { eNormal
  , eHead
  , eSparse
  };

struct SGPCacheEntry {
  tAddress theAddress;
  mutable eSGPType theType;
  mutable unsigned long long theAccessCount;
  mutable unsigned long theId;

  SGPCacheEntry(tAddress anAddress, eSGPType aType, unsigned long long anAccessCount, unsigned long anId )
    : theAddress(anAddress)
    , theType(aType)
    , theAccessCount(anAccessCount)
    , theId(anId)
  {}
};

typedef __gnu_cxx::hash_map<unsigned long, bool> alt_cache_t;


class MultiCache : public TraceProcessor, public MultiReceiver {

  typedef multi_index_container
    < MultiCacheEntry
    , indexed_by
        < sequenced< tag<by_LRU> >
        , ordered_unique
            < tag<by_address>
            , member<MultiCacheEntry,tAddress,&MultiCacheEntry::theAddress>
            >
        >
    > cache_table;

  cache_table theCache;

  typedef multi_index_container
    < SGPCacheEntry
    , indexed_by
        < ordered_unique
            < tag<by_address>
            , member<SGPCacheEntry,tAddress,&SGPCacheEntry::theAddress>
            >
        , sequenced< tag<by_LRU> >
        >
    > sgp_cache_table;

  sgp_cache_table theSGPCache;


  alt_cache_t theAlternateCache;

  std::string theName;
  int theId;
  long long theNextStreamId;


  unsigned long theCapacity;
  unsigned long theSGPCapacity;
  unsigned long theSGPCacheSize_Normal;
  unsigned long theBlockSize;
  unsigned long theBlockAddressMask;
  Predicate theInvalidatePredicate;
  Predicate theConsumePredicate;
  MultiStreamRequestFn theStreamRequestFn;
  boost::shared_ptr<Directory> theDirectory;
  bool theNoStreamOnSGPHit;

  StatCounter theTSEForwards;
  StatCounter theTSEDuplicateForwards;

  StatCounter theSGPForwards;
  StatCounter theSGPDuplicateForwards;

  StatCounter theConsumptions;
  StatCounter theConsumptions_User;
  StatCounter theConsumptions_System;

  StatCounter theHits;
  StatCounter theHits_User;
  StatCounter theHits_System;

  StatCounter theTSEHits;
  StatCounter theTSEHits_User;
  StatCounter theTSEHits_System;

  StatCounter theTSEHits_SamePC;
  StatCounter theTSEHits_DiffPC;

  StatCounter theSGPHits;
  StatCounter theSGPHits_User;
  StatCounter theSGPHits_System;
  StatCounter theSGPHeads;
  StatCounter theSGPSparse;

#ifdef ENABLE_ALTERNATIVE_SEARCH
  StatCounter theAltCacheHits;
#endif //ENABLE_ALTERNATIVE_SEARCH

  StatCounter theMisses_CreateStream;
  StatCounter theMisses_NoStream;
  StatCounter theMisses_NoStreamSGPHit;

  unsigned long long theAccessCount;

public:
  MultiCache(std::string aName, int anId, int aNumNodes, unsigned long aCapacity, unsigned long anSGPCapacity, unsigned long aBlockSize, Predicate anInvalidatePredicate, Predicate aConsumePredicate, MultiStreamRequestFn aStreamRequestFn, Directory* aDirectory, bool aNoStreamOnSGPHit);
  virtual ~MultiCache();
  virtual void finalize();

  virtual void event( TraceData & );
  virtual std::string const & name( ) const { return theName; }
  virtual int id( ) const { return theId; }

  bool forwardTSE( tAddress anAddress, tVAddress aPC, boost::shared_ptr<MultiStream> aStream, eForwardType aFwdType);
  void forwardSGP( tAddress anAddress, eSGPType aType, unsigned long anId);

private:
  void clearAlt( );
  void forwardAlt( tAddress anAddress, bool aHeadPCMatch );
  int altSize();
  void invalidate( tAddress anAddress );
  void consume( tAddress anAddress, tVAddress aPC, tFillType aFillType, tFillLevel aFillLevel, bool anOS );
  std::pair<eSGPOutcome, long> SGP_consume( tAddress anAddress, tVAddress aPC, bool anOS );
  std::pair<eTSEOutcome, long> TSE_consume( tAddress anAddress, tVAddress aPC, bool anOS, eSGPOutcome anOutcome );
  void stats( eTSEOutcome tse, eSGPOutcome sgp );

};

#endif //MULTI_CACHE_INCLUDED
