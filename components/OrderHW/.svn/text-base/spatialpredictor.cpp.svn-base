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

#include "common.hpp"
#include "spatialpredictor.hpp"
#include "multi_cache.hpp"
#include "trace.hpp"

StatCounter overallSGP_Accesses("overall.SGP_Accesses");
StatCounter overallSGP_Over("overall.SGP_Over");
StatCounter overallSGP_Under("overall.SGP_Under");
StatCounter overallSGP_First("overall.SGP_First");
StatCounter overallSGP_Train("overall.SGP_Train");
StatCounter overallSGP_Hit("overall.SGP_Hit");

StatCounter overallSGP_Forwards("overall.SGP_Forwards");
StatCounter overallSGP_HeadForwards("overall.SGP_HeadForwards");
StatCounter overallSGP_SparseForwards("overall.SGP_SparseForwards");

unsigned long theSGPGlobalId = 0;

unsigned long population(unsigned long x) {
    x = ((x & 0xAAAAAAAAUL) >> 1) + (x & 0x55555555UL);
    x = ((x & 0xCCCCCCCCUL) >> 2) + (x & 0x33333333UL);
    x = ((x & 0xF0F0F0F0UL) >> 4) + (x & 0x0F0F0F0FUL);
    x = ((x & 0xFF00FF00UL) >> 8) + (x & 0x00FF00FFUL);
    x = ((x & 0xFFFF0000UL) >> 16) + (x & 0x0000FFFFUL);
    return x;
}

SpatialVectorIterator::SpatialVectorIterator(SpatialVector & aVector, unsigned long aBaseAddress)
  : theVector(aVector.theVector)
  , theBit(1)
  , theAddress(aBaseAddress)
{ }

SpatialVectorIterator::SpatialVectorIterator()
  : theVector(0)
  , theBit(0)
  , theAddress(0)
  {}

bool SpatialVectorIterator::operator==(SpatialVectorIterator & rhs) {
   return (theBit == rhs.theBit);
}

bool SpatialVectorIterator::operator!=(SpatialVectorIterator & rhs) {
   return !(*this == rhs);
}

SpatialVectorIterator & SpatialVectorIterator::operator++() {
  while (theBit) {
    theBit <<= 1;
    theAddress += SpatialVector::kBlockSize;
    if (theVector & theBit) break;
  }
  return *this;
}

SpatialVectorIterator & SpatialVectorIterator::operator++(int) {
  return ++(*this);
}

SpatialVectorIterator & SpatialVectorIterator::operator--() {
  while (theBit) {
    theBit >>= 1;
    theAddress -= SpatialVector::kBlockSize;
    if (theVector & theBit) break;
  }
  return *this;
}

SpatialVectorIterator & SpatialVectorIterator::operator--(int) {
  return --(*this);
}

unsigned long SpatialVectorIterator::operator *() {
  return theAddress;
}

unsigned long SpatialVectorIterator::count() const {
  return population(theVector);
}

unsigned long SpatialVector::kIndexShift = 0;
unsigned long SpatialVector::kIndexMask = 0;
unsigned long SpatialVector::kBlockSize = 0;

SpatialVector::SpatialVector()
  : theTag(0)
  , theVector(0)
  {}

SpatialVector::SpatialVector(SpatialVector const & other)
  : theTag( other.theTag)
  , theVector( other.theVector )
  {}

SpatialVector & SpatialVector::operator = (SpatialVector const & other) {
  theTag = other.theTag;
  theVector = other.theVector;
  return *this;
}

bool SpatialVector::contains(tAddress anAddress) {
  unsigned long index = (anAddress & kIndexMask) >> kIndexShift;
  unsigned long bit = 1UL << index;
  DBG_Assert(bit != 0);
  return theVector & bit;
}

void SpatialVector::insert(tAddress anAddress) {
  unsigned long index = (anAddress & kIndexMask) >> kIndexShift;
  unsigned long bit = 1UL << index;
  DBG_Assert(bit != 0);
  theVector |= bit;
}

SpatialVectorIterator SpatialVector::begin(tAddress anAddress) {
  return SpatialVectorIterator(*this, anAddress);
}

SpatialVectorIterator SpatialVector::end() {
  return SpatialVectorIterator();
}

std::ostream & operator << (std::ostream & anOstream, SpatialVector const & aVector) {
  anOstream << std::hex << "<" << aVector.theTag << ">: " << std::setw(8) << std::setfill('0') << aVector.theVector << std::dec;
  return anOstream;
}

unsigned int log_base2(unsigned int num) {
  unsigned int ii = 0;
  while(num > 1) {
    ii++;
    num >>= 1;
  }
  return ii;
}


SpatialPredictor::SpatialPredictor(std::string aName, unsigned long aBlockSize, unsigned long aSpatialGroupSize)
  : theName(aName)
  , theBlockSize(aBlockSize)
  , theSpatialGroupSize(aSpatialGroupSize)
{
  kGroupMask = ~(aSpatialGroupSize - 1);
  SpatialVector::kIndexMask = aSpatialGroupSize-1 & (~(aBlockSize - 1 ));
  SpatialVector::kIndexShift = log_base2(aBlockSize);
  SpatialVector::kBlockSize = aBlockSize;
  kBlockShift = SpatialVector::kIndexShift;
  kBlockAddrMask = ~(aBlockSize - 1);
  kBlockMask = aSpatialGroupSize - 1;
  kPCShift = log_base2(aSpatialGroupSize) - kBlockShift;
}


SpatialPredictor::~SpatialPredictor () {}
void SpatialPredictor::finalize() {}

void SpatialPredictor::setCache( MultiCache * aCache) {
  theCache = aCache;
}

void SpatialPredictor::computeStats( SpatialVector const & anExisting, SpatialVector const & aNew ) {
  unsigned long accesses = population(aNew.theVector);
  unsigned long intersect = anExisting.theVector & aNew.theVector;
  unsigned long matches = population(intersect);
  overallSGP_Accesses += accesses;
  if (matches >= 1) {
    ++overallSGP_First;
    --matches;
  }
  overallSGP_Hit += matches;
  unsigned long over = population(anExisting.theVector & ~intersect);
  overallSGP_Over += over;
  unsigned long under = population(aNew.theVector & ~intersect);
  if (anExisting.theVector == 0) {
    overallSGP_Train += under;
  } else {
    overallSGP_Under += under;
  }
}

void SpatialPredictor::access( TraceData & anEvent ) {
  DBG_( Verb, ( << theName << " Access " << anEvent ) );
  unsigned long group = anEvent.theAddress & kGroupMask;
  SpatialTable::iterator iter;
  bool is_new;
  boost::tie(iter,is_new) = theCacheState.insert( std::make_pair( group, SpatialVector() ) );
  if (is_new) {
    unsigned long tag = makeSGPTag(anEvent.theAddress, anEvent.thePC);
    iter->second.setTag( tag );
    doForward( tag, anEvent.theAddress, anEvent.thePC );
  }
  iter->second.insert(anEvent.theAddress);

  DBG_( Verb, ( << theName << " Spatial Group:" << std::hex << group << std::dec << " vector: " <<   iter->second) );
}

void SpatialPredictor::eviction( TraceData & anEvent  ) {
  DBG_( Verb, ( << theName << " Evict " << anEvent ) );
  unsigned long group = anEvent.theAddress & kGroupMask;
  SpatialTable::iterator iter = theCacheState.find(group);

  if (iter != theCacheState.end() && iter->second.contains(anEvent.theAddress)) {
    SpatialTable::iterator existing = theSGP.find(iter->second.tag());
    if (existing != theSGP.end()) {
      computeStats(existing->second, iter->second);
    } else {
      computeStats(SpatialVector(), iter->second);
    }
    theSGP[iter->second.tag()] = iter->second;
    DBG_( Verb, ( << theName << " End spatial group :" << std::hex << group << std::dec << " vector: " <<   iter->second) );
    theCacheState.erase(iter);
  }
}

unsigned long SpatialPredictor::makeSGPTag( tAddress anAddress, tVAddress aPC) {
  unsigned long tag = aPC << kPCShift;
  tag |= (anAddress & kBlockMask) >> kBlockShift;
  return tag;
}

void SpatialPredictor::doForward( unsigned long aTag, tAddress anAddress, tVAddress aPC) {
  tAddress addr = anAddress & kBlockAddrMask;

  theSGPGlobalId++;
  SpatialTable::iterator iter = theSGP.find(aTag);
  if (iter != theSGP.end()) {
    DBG_( Trace, ( << theName << " Forwarding spatial group tag:" << std::hex << aTag << " head: " << addr  << std::dec << " vector: " <<   iter->second ) );
    #ifdef ENABLE_TRACE_SGPGENERATIONS
      trace::addGeneration( trace::Generation(theSGPGlobalId, theCache->id(), anAddress, aPC, iter->second.theVector) );
    #endif //ENABLE_TRACE_SGPGENERATIONS
    SpatialVector::iterator addr_iter = iter->second.begin(addr  & kGroupMask);
    SpatialVector::iterator addr_end = iter->second.end();
    while (addr_iter != addr_end) {
      DBG_( Trace, ( << theName << " Forwarding:" << std::hex << *addr_iter << std::dec << " head?: " << (*addr_iter == addr )  ) );
      ++ overallSGP_Forwards;
      if (*addr_iter == addr ) {
        if (addr_iter.count() == 1) {
          ++ overallSGP_SparseForwards;
          theCache->forwardSGP(*addr_iter, eSparse, theSGPGlobalId);
        } else {
          ++ overallSGP_HeadForwards;
          theCache->forwardSGP(*addr_iter, eHead, theSGPGlobalId);
        }
      } else {
        theCache->forwardSGP(*addr_iter, eNormal, theSGPGlobalId);
      }
      ++addr_iter;
    }
  } else {
    #ifdef ENABLE_TRACE_SGPGENERATIONS
      trace::addGeneration( trace::Generation(theSGPGlobalId, theCache->id(), anAddress, aPC, 0) );
    #endif //ENABLE_TRACE_SGPGENERATIONS
  }
}
