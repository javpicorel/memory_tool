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
  

#include <functional>
#include <boost/random.hpp>
#include <fstream>

#include "index.hpp"

namespace nTMSIndex{

#define ENABLE_CUCKOO_NUKE_VICTIM
#define ENABLE_CUCKOO_NOMOVE
#define ENABLE_CUCKOO_ONEHASH
#define ENABLE_CUCKOO_BIASED_VICTIM
#define ENABLE_CUCKOO_NODELETE


#define HASHSIZE(n) ((uint32_t)1<<(n))
#define HASHMASK(n) (HASHSIZE(n)-1)
#define ROT(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

/*
-------------------------------------------------------------------------------
mix -- mix 3 32-bit values reversibly.

This is reversible, so any information in (a,b,c) before mix() is
still in (a,b,c) after mix().

If four pairs of (a,b,c) inputs are run through mix(), or through
mix() in reverse, there are at least 32 bits of the output that
are sometimes the same for one pair and different for another pair.
This was tested for:
* pairs that differed by one bit, by two bits, in any combination
  of top bits of (a,b,c), or in any combination of bottom bits of
  (a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
  the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
  is commonly produced by subtraction) look like a single 1-bit
  difference.
* the base values were pseudorandom, all zero but one bit set, or 
  all zero plus a counter that starts at zero.

Some k values for my "a-=c; a^=rot(c,k); c+=b;" arrangement that
satisfy this are
    4  6  8 16 19  4
    9 15  3 18 27 15
   14  9  3  7 17  3
Well, "9 15 3 18 27 15" didn't quite get 32 bits diffing
for "differ" defined as + with a one-bit base and a two-bit delta.  I
used http://burtleburtle.net/bob/hash/avalanche.html to choose 
the operations, constants, and arrangements of the variables.

This does not achieve avalanche.  There are input bits of (a,b,c)
that fail to affect some output bits of (a,b,c), especially of a.  The
most thoroughly mixed value is c, but it doesn't really even achieve
avalanche in c.

This allows some parallelism.  Read-after-writes are good at doubling
the number of bits affected, so the goal of mixing pulls in the opposite
direction as the goal of parallelism.  I did what I could.  Rotates
seem to cost as much as shifts on every machine I could lay my hands
on, and rotates are much kinder to the top and bottom bits, so I used
rotates.
-------------------------------------------------------------------------------
*/
#define MIX(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}

/*
-------------------------------------------------------------------------------
final -- final mixing of 3 32-bit values (a,b,c) into c

Pairs of (a,b,c) values differing in only a few bits will usually
produce values of c that look totally different.  This was tested for
* pairs that differed by one bit, by two bits, in any combination
  of top bits of (a,b,c), or in any combination of bottom bits of
  (a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
  the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
  is commonly produced by subtraction) look like a single 1-bit
  difference.
* the base values were pseudorandom, all zero but one bit set, or 
  all zero plus a counter that starts at zero.

These constants passed:
 14 11 25 16 4 14 24
 12 14 25 16 4 14 24
and these came close:
  4  8 15 26 3 22 24
 10  8 15 26 3 22 24
 11  8 15 26 3 22 24
-------------------------------------------------------------------------------
*/
#define FINAL(a,b,c) \
{ \
  c ^= b; c -= ROT(b,14); \
  a ^= c; a -= ROT(c,11); \
  b ^= a; b -= ROT(a,25); \
  c ^= b; c -= ROT(b,16); \
  a ^= c; a -= ROT(c,4);  \
  b ^= a; b -= ROT(a,14); \
  c ^= b; c -= ROT(b,24); \
}




std::size_t hash<tAddress>::operator()(tAddress val) const {
    register unsigned int a, b, c;
    a = b = c = theSeed;
    a += val;
    FINAL(a,b,c);
    return c;
}



CuckooIndex::CuckooIndex( std::string const & name, int aLog2Buckets, int aBucketSize ) 
   : theName(name)
   , theLog2Buckets(aLog2Buckets)
   , theBucketSize(aBucketSize)
   , theBucketMask(HASHMASK(aLog2Buckets))
   , theSize(0)
   , theVictim(0,aBucketSize-1)
   , theBucket(0,1)
   , getBucket(theRNG, theBucket)
   , getVictim(theRNG, theVictim)
   , theCKInserts(name + ".inserts")
   , theCKUpdates(name + ".updates")
   , theCKErases(name + ".erases")
   , theCKLookups(name + ".lookups")
   , theCKNoMatches(name + ".no_match")
   , theCKNoErase(name + ".no_erase")
   , theCKCuckooSearchs(name + ".cuckoo_searches")
   , theCKCuckooSearchSteps(name + ".cuckoo_search_steps")
   , theCKFailedInserts(name + ".failed_inserts")
   , theCKNukedVictims(name + ".nuked_victims")
   , theCKHashCollides(name + ".collides")
   , theCKClearProtect(name + ".clear_protect")
   , theCKMoveOntoUnprotected(name + ".move_to_unprotected")
   , theLookup_GU_Both(name + ".lookup_gu_both")
   , theLookup_GU_GoodOnly(name + ".lookup_gu_good")
   , theLookup_GU_UpdateOnly(name + ".lookup_gu_update")
   , theLookup_GU_None(name + ".lookup_gu_none")
#ifdef ENABLE_CUCKOO_TRY_TAGWIDTHS 
   , theWrongMatch( name + ".tagwidth_wrong" )
#endif //ENABLE_CUCKOO_TRY_TAGWIDTHS 
{
    theHash[0] = hash<key_t>(0xdeadbeef);
    theHash[1] = hash<key_t>(0xfeed3337);    
    theArray = new CuckooEntry [ HASHSIZE(aLog2Buckets) * aBucketSize]; 

    for (unsigned int i = 0; i <HASHSIZE(aLog2Buckets) * aBucketSize;  ++i) {
      theArray[i].theKey = 0; 
      theArray[i].theValue = theInvalid; 
      theArray[i].theFlags = 0; 
    }

}

CuckooIndex::~CuckooIndex() { delete [] theArray; }

std::pair<CuckooIndex::value_t, bool> CuckooIndex::lookup( key_t aKey) {
#ifdef ENABLE_CUCKOO_TRY_TAGWIDTHS 
  tryTagWidths( aKey );
#endif //ENABLE_CUCKOO_TRY_TAGWIDTHS 
  ++theCKLookups;
  unsigned int hash[2] = { theHash[0](aKey), theHash[1](aKey) };
  unsigned int idx[2] = { hash[0] & theBucketMask * theBucketSize, hash[1] & theBucketMask * theBucketSize };
  for (unsigned int i = 0; i < theBucketSize; ++i) {
    bool cmp[2] = {(theArray[idx[0] + i].theKey == aKey), theArray[idx[1] + i].theKey  == aKey};
    if (cmp[0]) {
      if (theArray[idx[0] + i].theFlags & kGood && theArray[idx[0] + i].theFlags & kUpdated) {        
        theLookup_GU_Both++;
      } else  if (theArray[idx[0] + i].theFlags & kUpdated) {
        theLookup_GU_UpdateOnly++;
      } else  if (theArray[idx[0] + i].theFlags & kGood) {
        theLookup_GU_GoodOnly++;
      } else {
        theLookup_GU_None++;
      }
      return std::make_pair(theArray[idx[0] + i].theValue, !! (theArray[idx[0] + i].theFlags & kGood));
    }
#ifndef ENABLE_CUCKOO_ONEHASH
    if (cmp[1]) {
      if (theArray[idx[1] + i].theFlags & kUpdated) {        
        theLookup_GU_Both++;
      } else  if (theArray[idx[1] + i].theFlags & kUpdated) {
        theLookup_GU_UpdateOnly++;
      } else  if (theArray[idx[1] + i].theFlags & kGood ) {
        theLookup_GU_LookOnly++;
      } else {
        theLookup_GU_None++;
      }
      return std::make_pair(theArray[idx[1] + i].theValue, !! (theArray[idx[1] + i].theFlags & kGood));
    }
#endif //!ENABLE_CUCKOO_ONEHASH
  }
  ++theCKNoMatches;
  return std::make_pair(theInvalid, false);
}


bool CuckooIndex::has( key_t aKey) {
  unsigned int hash = theHash[0](aKey);
  unsigned int idx = hash & theBucketMask * theBucketSize;
  for (unsigned int i = 0; i < theBucketSize; ++i) {
    bool cmp = (theArray[idx + i].theKey == aKey);
    if (cmp) {
      return true;
    }
  }
  return false;
}

bool CuckooIndex::try_insert( CuckooEntry & anEntry, int aHash) {
#ifdef ENABLE_CUCKOO_ONEHASH
  DBG_Assert( false ) ; //Not used with ONEHASH
#endif 
  DBG_Assert( aHash == 0 || aHash == 1);
  unsigned int hash = theHash[aHash](anEntry.theKey);
  unsigned int idx = hash & theBucketMask * theBucketSize;
  int unprotected = -1;
  for (unsigned int i = 0; i < theBucketSize; ++i) {
    if (theArray[idx + i].theValue == theInvalid) {
      theArray[idx + i].theKey = anEntry.theKey;
      theArray[idx + i].theValue = anEntry.theValue;
      theArray[idx + i].theFlags = anEntry.theFlags;
      //std::cerr << "CK move: " << aPair.first << "->" << aPair.second << " at " << idx << ":" << i << " (h" << aHash << ")" << std::endl;
      return true;        
    } else if (! (theArray[idx + i].theFlags & kGood ) ) {
      unprotected = i; 
    } 
  }
  #ifdef ENABLE_CUCKOO_BIASED_VICTIM
    if (unprotected != -1) { 
      ++theCKMoveOntoUnprotected;
      theArray[idx + unprotected].theKey = anEntry.theKey;
      theArray[idx + unprotected].theValue = anEntry.theValue;
      theArray[idx + unprotected].theFlags = anEntry.theFlags;       
      return true;
    }
  #endif
  
  return false;    
}

#ifdef ENABLE_CUCKOO_TRY_TAGWIDTHS 
void CuckooIndex::tryTagWidths( key_t aKey) {
  std::vector< unsigned int > tags;
  unsigned int hash = theHash[0](aKey);
  unsigned int idx = hash & theBucketMask * theBucketSize;
  int correct = -1;
  for (unsigned int i = 0; i < theBucketSize; ++i) {
    tags.push_back(theHash[0](theArray[ idx +i].theKey) >> theLog2Buckets); 
    if (aKey == theArray[ idx +i].theKey) {
      correct = i;
    }
  }
  unsigned int mask = 1;
  for (int bits = 1; bits < 32 - theLog2Buckets; ++bits) {
    bool bad = false;
    for (int i = 0; i < 8; ++i) {
      if (((hash & mask) == (tags[i] & mask)) && (correct != i)) {
        theWrongMatch << std::make_pair( bits, 1);
        bad = true;
        break;
      }
    }
    if (! bad ) break;
    mask <<= 1;
    mask |= 1;
  }
}  
#endif //ENABLE_CUCKOO_TRY_TAGWIDTHS 

unsigned int CuckooIndex::bucket( key_t aKey) {
  unsigned int hash = theHash[0](aKey);
  return hash & theBucketMask;  
}

bool CuckooIndex::insert( key_t aKey, value_t aValue, bool isGood) {
  ++theCKInserts;
  //Search for a match or empty slot
  unsigned int hash[2] = { theHash[0](aKey), theHash[1](aKey) };
  unsigned int idx[2] = { hash[0] & theBucketMask * theBucketSize, hash[1] & theBucketMask * theBucketSize };
  if (idx[0] == idx[1]) { ++theCKHashCollides;}
  int empty_i = -1;
  int empty_hash = -1;
 
  //Check hash zero 
  for (unsigned int i = 0; i < theBucketSize; ++i) {
    if (theArray[idx[0] + i].theKey == aKey) {
      theArray[idx[0] + i].theValue = aValue;                           
      theArray[idx[0] + i].theFlags |= kUpdated;
      if (isGood) {
        theArray[idx[0] + i].theFlags |= kGood;        
      }                           
      ++theCKUpdates;
      
      //std::cerr << "CK update: " << aKey << "->" << (aValue & 15) << " " << (aValue >> 4) << std::endl;
      return true;
    }
    if (empty_i == -1 && theArray[idx[0] + i].theValue == theInvalid) {
      empty_i = i;
      empty_hash = 0;
    }
  }
  //Check hash one
#ifndef ENABLE_CUCKOO_ONEHASH
  for (unsigned int i = 0; i < theBucketSize; ++i) {
    if (theArray[idx[1] + i].theKey == aKey) {
      theArray[idx[1] + i].theValue = aValue;
      theArray[idx[1] + i].theFlags |= kUpdated;                           
      ++theCKUpdates;
      //std::cerr << "CK update: " << aKey << "->" << (aValue & 15) << " " << (aValue >> 4) << std::endl;
      return true;
    }
    if (empty_i == -1 && theArray[idx[1] + i].theValue == theInvalid) {
      empty_i = i;
      empty_hash = 1;
    }
  }
#endif //!ENABLE_CUCKOO_ONEHASH

  //No existing match.  Fill in empty
  if (empty_i != -1) {
    theArray[idx[empty_hash] + empty_i].theKey = aKey;                        
    theArray[idx[empty_hash] + empty_i].theValue = aValue;                     
    if (isGood) {
      theArray[idx[empty_hash] + empty_i].theFlags = kGood;                     
    } else {
      theArray[idx[empty_hash] + empty_i].theFlags = 0;                     
    }
    ++theSize;
    //std::cerr << "CK insert: " << aKey << "->" << (aValue & 15) << " " << (aValue >> 4) << " at h" << empty_hash << " " << idx[empty_hash] << ":" << empty_i << std::endl;
    return true;    
  }    

  //std::cerr << "CK insert-collide: " << aKey << "->" << (aValue & 15) << " " << (aValue >> 4) << std::endl;

  //Both buckets full.  Need to cuckoo.     
  //Choose victim to start the cuckoo process
  ++theCKCuckooSearchs;

#ifdef ENABLE_CUCKOO_BIASED_VICTIM
  int victim_bucket = -1;
  int victim = -1;
  std::vector< std::pair<int,int> > victims;
  victims.reserve(theBucketSize*2);
  
  for (unsigned int i = 0; i < theBucketSize; ++i) {
    if (! (theArray[idx[0] + i].theFlags & kGood ) )  {
      victims.push_back( std::make_pair(0,i) );
    }     
    #ifndef ENABLE_CUCKOO_ONEHASH
      if (! (theArray[idx[1] + i].theFlags & kGood ) )  {
        victims.push_back( std::make_pair(1,i) );
      }     
    #endif //!ENABLE_CUCKOO_ONEHASH
  }
  
  if (victims.size() > 1) {
    int victim_index = boost::variate_generator<boost::mt19937&, boost::uniform_smallint<> >(theRNG,boost::uniform_smallint<>(0,victims.size() - 1)) ();
    victim_bucket = victims[victim_index].first;
    victim = victims[victim_index].second;
  } else {
    //Clear protect bits
    ++theCKClearProtect;
    for (unsigned int i = 0; i < theBucketSize; ++i) {        
      theArray[idx[0] + i].theFlags &= ~kGood;
      #ifndef ENABLE_CUCKOO_ONEHASH
        theArray[idx[1] + i].theFlags &= ~kGood;
      #endif //!ENABLE_CUCKOO_ONEHASH
    }    
    #ifdef ENABLE_CUCKOO_ONEHASH
      victim_bucket = 0;
    #else //!ENABLE_CUCKOO_ONEHASH
      victim_bucket = getBucket();    
    #endif
    victim = getVictim();
  }
  DBG_Assert( ! (theArray[idx[victim_bucket] + victim].theFlags & kGood));
#else
  #ifdef ENABLE_CUCKOO_ONEHASH
    int victim_bucket = 0;
  #else //!ENABLE_CUCKOO_ONEHASH
    int victim_bucket = getBucket();
  #endif //ENABLE_CUCKOO_ONEHASH
  int victim = getVictim();
#endif
  DBG_Assert( victim >= 0 && victim < theBucketSize);
  DBG_Assert(victim_bucket == 0 || victim_bucket == 1);

 
  #ifdef ENABLE_CUCKOO_SEARCH
    #ifdef ENABLE_CUCKOO_ONEHASH
      DBG_Assert(false); //Search doesn't make sense with one hash
    #endif //ENABLE_CUCKOO_ONEHASH

    //Try all possible victims in the selected bucket
    #define TRY_BUCKET(bucket)                                    \
      for (unsigned int i = 0; i < theBucketSize; ++i) {          \
        ++theCKCuckooSearchSteps;                                 \
        if (try_insert( theArray[idx[bucket] + victim], ! bucket)) {\
          theArray[idx[bucket] + victim].theKey = aKey;           \
          theArray[idx[bucket] + victim].theValue = aValue;       \
          if (isGood) {                                           \
            theArray[idx[bucket] + victim].theFlags = kGood;      \
          } else {                                                \
            theArray[idx[bucket] + victim].theFlags = 0;          \
          }                                                       \
          ++theSize;                                              \
          return true;                                            \
        }                                                         \
        victim++;                                                 \
        if (victim >= theBucketSize) { victim = 0; }              \
      }                                                           /**/
    TRY_BUCKET(victim_bucket)
    TRY_BUCKET(! victim_bucket)
    #undef TRY_BUCKET
  #else //!ENABLE_CUCKOO_SEARCH
    #ifdef ENABLE_CUCKOO_NOMOVE
      //Don't even try to move the victim.  Just nuke or fail.
    #else //!ENABLE_CUCKOO_NOMOVE
      #ifdef ENABLE_CUCKOO_ONEHASH
        DBG_Assert(false); // Must specify NOMOVE with ONEHASH
      #endif //ENABLE_CUCKOO_ONEHASH
    //Just try moving one victim - give up if that fails
      DBG_Assert( 1-victim_bucket == 0 || 1 - victim_bucket == 1);            
      ++theCKCuckooSearchSteps;
      if ( try_insert( theArray[idx[victim_bucket] + victim], 1 - victim_bucket) ) {
        theArray[idx[victim_bucket] + victim].theKey  = aKey;            
        theArray[idx[victim_bucket] + victim].theValue = aValue;         
        if (isGood) {          
          theArray[idx[victim_bucket] + victim].theFlags = kGood;            
        } else {
          theArray[idx[victim_bucket] + victim].theFlags = 0;            
        }
        ++theSize;
        return true; 
      }    
    #endif    
  #endif //ENABLE_CUCKOO_SEARCH    

  //Search failed.  Now what?
  #ifdef ENABLE_CUCKOO_NUKE_VICTIM
     //Nuke the victim, insert succeeds
     //std::cerr << "CK nuke: " << theArray[idx[victim_bucket] + victim].first << "->" << theArray[idx[victim_bucket] + victim].second << " at " << idx[victim_bucket] << ":" << victim << " (h" << victim_bucket << ")" << std::endl;
     theArray[idx[victim_bucket] + victim].theKey = aKey;
     theArray[idx[victim_bucket] + victim].theValue = aValue;
     if (isGood) {          
       theArray[idx[victim_bucket] + victim].theFlags = kGood;
     } else {
       theArray[idx[victim_bucket] + victim].theFlags = 0;
     }
     ++theCKNukedVictims;
     return true;
  #else //!ENABLE_CUCKOO_NUKE_VICTIM        
     //Insert fails
      //std::cerr << "CK fail: " << aKey << "->" << aValue << std::endl;
     ++theCKFailedInserts;
     return false;
  #endif //ENABLE_CUCKOO_NUKE_VICTIM       
}

bool CuckooIndex::erase( key_t aKey, value_t aValue) {
  ++theCKErases;

  unsigned int hash[2] = { theHash[0](aKey), theHash[1](aKey) };
  unsigned int idx[2] = { hash[0] & theBucketMask * theBucketSize, hash[1] & theBucketMask * theBucketSize };
  for (int i = 0; i < theBucketSize; ++i) {
    bool cmp[2] = {(theArray[idx[0] + i].theKey == aKey && theArray[idx[0] + i].theValue == aValue), theArray[idx[1] + i].theKey == aKey && theArray[idx[1] + i].theValue == aValue};
    if (cmp[0]) {
      //std::cerr << "CK erase : " << aKey << "->" << (aValue & 15) << " " << (aValue >> 4) << std::endl;
      theArray[idx[0] + i].theKey = 0;
      theArray[idx[0] + i].theValue = theInvalid;
      theArray[idx[0] + i].theFlags = 0;
      --theSize;
      return true;              
    }
    #ifndef ENABLE_CUCKOO_ONEHASH
    if (cmp[1]) {
      //std::cerr << "CK erase : " << aKey << "->" << (aValue & 15) << " " << (aValue >> 4) << std::endl;
      theArray[idx[1] + i].theKey = 0;
      theArray[idx[1] + i].theValue = theInvalid;
      theArray[idx[1] + i].theFlags = 0;
      --theSize;
      return true;              
    }
    #endif //!ENABLE_CUCKOO_ONEHASH
  }
  //std::cerr << "CK no-erase : " << aKey << "->" << (aValue & 15) << " " << (aValue >> 4) << std::endl;
  ++theCKNoErase;
  return false;      
}

void CuckooIndex::save(std::string const & aBaseName, std::vector<unsigned long long> const & anInitialSeqNums) {
  std::ofstream out( (aBaseName + theName).c_str());
  
  out << theLog2Buckets << ' ' << theBucketSize << std::endl;

  for (unsigned int i = 0; i <HASHSIZE(theLog2Buckets) * theBucketSize;  ++i) {
    if (theArray[i].theValue == -1) {
      out << "0 -1 0 0" << std::endl;       
    } else {
      int node = theArray[i].theValue & 15;
      long long seq = theArray[i].theValue >> 4 - anInitialSeqNums[node] + 1;
      if (seq < 0) {
        seq = 0; 
      }      
      out << theArray[i].theKey << " " << node << " " << seq << " " << theArray[i].theFlags << std::endl;
    }
  }
 
}

void CuckooIndex::load(std::string const & aBaseName) {
  std::ifstream in( (aBaseName + theName).c_str());
  
  if (in.fail()) {
    DBG_(Dev, ( << "No index state for " << theName ) );
    return;
  }
  
  DBG_(Dev, ( << "Loading " << theName ) );
  
  unsigned int aLog2Buckets, aBucketSize;
  in >> aLog2Buckets >> aBucketSize;
  DBG_Assert( aLog2Buckets == theLog2Buckets, ( << aLog2Buckets << " " << theLog2Buckets ) );
  DBG_Assert( aBucketSize == theBucketSize, ( << aBucketSize << " " << theBucketSize ) );

  for (unsigned int i = 0; i <HASHSIZE(theLog2Buckets) * theBucketSize ;  ++i) {
    int node;
    tAddress key;
    long long seq;
    int flags;
    in >> key >> node >> seq >> flags;
    DBG_Assert( ! in.fail() );
    
    if (node == -1) {
      theArray[i].theKey = 0;
      theArray[i].theValue = -1;
      theArray[i].theFlags = 0;      
    } else {
      DBG_Assert( node < 16);
      DBG_Assert( seq >= 0);   
      theArray[i].theKey = key;
      long long val = seq << 4 | node;
      theArray[i].theValue = val;
      theArray[i].theFlags = flags;
    }
  }
   
}

} //End namespace nTMSIndex

