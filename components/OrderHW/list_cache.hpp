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
#ifndef LIST_CACHE_INCLUDED
#define LIST_CACHE_INCLUDED

#include "common.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>

using namespace boost::multi_index;


struct ListCacheEntry {
  tAddress theAddress;
  boost::shared_ptr<SuperStream> theStream;
  long theStreamIndex;
  long long theSequenceNo;
  long long theHeadSequenceNo;

  ListCacheEntry(tAddress anAddress, boost::shared_ptr<SuperStream> aStream, long aStreamIndex, long long aSequenceNo, long long aHeadSequenceNo)
    : theAddress(anAddress)
    , theStream(aStream)
    , theStreamIndex(aStreamIndex)
    , theSequenceNo(aSequenceNo)
    , theHeadSequenceNo(aHeadSequenceNo)
  {}
  void finalize() const {
    theStream->finalize();
  }
};


struct PullStreamStorageEntry {
  boost::shared_ptr<SuperStream> theStream;
  long thePresentBlocks;
  long long theUniqueId;
  PullStreamStorageEntry(boost::shared_ptr<SuperStream> aStream, long anUniqueId)
    : theStream(aStream)
    , thePresentBlocks(0)
    , theUniqueId(anUniqueId)
  {}
  void finalize() const {
    theStream->finalize();
  }

  static void addBlock(PullStreamStorageEntry & anEntry, long long aNewId) {
    anEntry.thePresentBlocks++;
    anEntry.theUniqueId = aNewId;
  }
  static void hitBlock(PullStreamStorageEntry & anEntry, long long aNewId) {
    anEntry.thePresentBlocks--;
    anEntry.theUniqueId = aNewId;
  }
  static void removeBlock(PullStreamStorageEntry & anEntry) {
    anEntry.thePresentBlocks--;
  }
};


struct ListCache : public TraceProcessor, public PrefetchReceiver {

  enum eDropReason
    { kInvalidation
    , kDuplicateForward
    };


  typedef multi_index_container
    < ListCacheEntry
    , indexed_by
        < sequenced< tag<by_LRU> >
        , ordered_unique
            < tag<by_address>
            , member<ListCacheEntry,tAddress,&ListCacheEntry::theAddress>
            >
        >
    > cache_table;

  typedef multi_index_container
    < PullStreamStorageEntry
    , indexed_by
      < ordered_unique< tag<by_replacement>
                      , composite_key< PullStreamStorageEntry
                                     , member<PullStreamStorageEntry,long,&PullStreamStorageEntry::thePresentBlocks>
                                     , member<PullStreamStorageEntry,long long,&PullStreamStorageEntry::theUniqueId>
                                     >
                      >
      , ordered_unique< tag<by_stream>
                      , member<PullStreamStorageEntry,boost::shared_ptr<SuperStream>,&PullStreamStorageEntry::theStream>
                      >
      >
    > stream_table;


  std::string theName;
  int theId;
  cache_table theCache;
  stream_table theStreams;
  long long theNextStreamId;

  unsigned long theCapacity;
  unsigned long theBlockSize;
  unsigned long theMaxStreams;
  unsigned long theIntersectDistance;
  Predicate theInvalidatePredicate;
  Predicate theConsumePredicate;
  PullStreamRequestFn theStreamRequestFn;
  boost::shared_ptr<Directory> theDirectory;

  long long theConsecutiveStreamHits;
  boost::shared_ptr<SuperStream> thePreviousStreamAccess;
  long thePreviousStreamIndex;

  StatCounter theForwards;
  StatCounter theDuplicateForwards;
  StatCounter theDiscards;
  StatCounter theLRUDiscards;
  StatCounter theInvalidateDiscards;
  StatCounter theCacheDiscards;
  StatCounter theConsumptions;
  StatCounter theConsumptions_User;
  StatCounter theConsumptions_System;
  StatCounter theHits;
  StatCounter theHits_User;
  StatCounter theHits_System;
  StatCounter theHitsMyOrder;
  StatCounter theHitSameStreamSameOrder;
  StatCounter theHitSameStreamDiffOrder;
  StatCounter theHitDiffStreamSameOrder;
  StatCounter theHitDiffStreamDiffOrder;
  StatCounter theHitDiffStreamNoPrevOrder;
  StatCounter theMissExistingHandled;
  StatCounter theMissCreateStream;
  StatCounter theMissNoStream;
  StatCounter theEvictedStream;
  StatCounter theOrphanedHits;

  ListCache(std::string aName, int anId, int aNumNodes, unsigned long aCapacity, unsigned long aBlockSize, unsigned long aMaxStreams, unsigned long anIntersectDistance, Predicate anInvalidatePredicate, Predicate aConsumePredicate, PullStreamRequestFn aStreamRequestFn, Directory* aDirectory);
  virtual ~ListCache();
  virtual void finalize();

  virtual void event( TraceData & );
  virtual std::string const & name( ) const { return theName; }
  virtual int id( ) const { return theId; }

private:
  bool forward( tAddress anAddress, boost::shared_ptr<SuperStream> aStream, long anIndex, long long aSequenceNo, long long aHeadSequenceNo );
  void invalidate( tAddress anAddress );
  bool consume( tAddress anAddress, tAddress aPC, bool anOS, tID * hitId, long long * hitSeqNo );
  long streamConsume(bool isHit, boost::shared_ptr<SuperStream> aSuperStream);

};

#endif //LIST_CACHE_INCLUDED
