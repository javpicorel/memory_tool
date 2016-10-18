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


#ifndef CUCKOOINDEX_INCLUDED
#define CUCKOOINDEX_INCLUDED


#include <functional>
#include <boost/random.hpp>
#include <core/stats.hpp>

namespace nTMSIndex{

using namespace Flexus::Stat;

typedef unsigned long tAddress;

template <class T>
struct hash;

template <>
struct hash<tAddress> : public std::unary_function<tAddress, std::size_t> {
  int theSeed;
  hash()
   : theSeed(0xdeadbeef)
   {}

  hash(int aSeed)
   : theSeed(aSeed)
   {}
  
  std::size_t operator()(tAddress val) const;
};


static const int kGood = 1;
static const int kUpdated = 2;

struct CuckooEntry {
  tAddress theKey;
  long theFlags; 
  long long theValue;  
};

class CuckooIndex {
  std::string theName;
  hash<tAddress> theHash[2];
  unsigned int theLog2Buckets;
  unsigned int theBucketSize;
  unsigned int theBucketMask;
  static const int theInvalid = -1;
  unsigned int theSize;
  CuckooEntry * theArray;

  boost::mt19937 theRNG;                 
  boost::uniform_smallint<> theVictim;   
  boost::uniform_smallint<> theBucket;
  boost::variate_generator<boost::mt19937&, boost::uniform_smallint<> > getBucket;
  boost::variate_generator<boost::mt19937&, boost::uniform_smallint<> > getVictim;

  typedef long long value_t;
  typedef tAddress key_t;

  StatCounter theCKInserts;
  StatCounter theCKUpdates;
  StatCounter theCKErases;
  StatCounter theCKLookups;
  StatCounter theCKNoMatches;
  StatCounter theCKNoErase;
  StatCounter theCKCuckooSearchs;
  StatCounter theCKCuckooSearchSteps;
  StatCounter theCKFailedInserts;
  StatCounter theCKNukedVictims;
  StatCounter theCKHashCollides;
  StatCounter theCKClearProtect;
  StatCounter theCKMoveOntoUnprotected;
  
  StatCounter theLookup_GU_Both;
  StatCounter theLookup_GU_GoodOnly;
  StatCounter theLookup_GU_UpdateOnly;
  StatCounter theLookup_GU_None;
#ifdef ENABLE_CUCKOO_TRY_TAGWIDTHS 
  StatInstanceCounter<long long> theWrongMatch;
#endif //ENABLE_CUCKOO_TRY_TAGWIDTHS 

public:
  CuckooIndex( std::string const & name, int aLog2Buckets, int aBucketSize );
  ~CuckooIndex();
  std::pair<value_t, bool> lookup( key_t aKey);
  bool has( key_t aKey);
  bool try_insert( CuckooEntry & aPair, int aHash);
  bool insert( key_t aKey, value_t aValue, bool isGood);
  unsigned int bucket( key_t aKey);
  bool erase( key_t aKey, value_t aValue);
#ifdef ENABLE_CUCKOO_TRY_TAGWIDTHS 
  void tryTagWidths( key_t aKey);
#endif //ENABLE_CUCKOO_TRY_TAGWIDTHS 

  void save(std::string const & aBaseName, std::vector<unsigned long long> const & anInitialSeqNums);
  void load(std::string const & aBaseName);


};

}

#endif //CUCKOOINDEX_INCLUDED
