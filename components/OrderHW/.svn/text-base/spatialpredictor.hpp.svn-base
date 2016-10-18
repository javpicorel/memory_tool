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

#ifndef SPATIALPREDICTOR_INCLUDED
#define SPATIALPREDICTOR_INCLUDED

#include "common.hpp"

#include <ext/hash_map>

class MultiCache;

unsigned int log_base2(unsigned int num);

class SpatialVector;

struct SpatialVectorIterator {
  unsigned long theVector;
  unsigned long theBit;
  unsigned long theAddress;

  SpatialVectorIterator(SpatialVector & aVector, unsigned long aBaseAddress);
  SpatialVectorIterator();
  bool operator==(SpatialVectorIterator & rhs);
  bool operator!=(SpatialVectorIterator & rhs);
  SpatialVectorIterator & operator++();
  SpatialVectorIterator & operator++(int);
  SpatialVectorIterator & operator--();
  SpatialVectorIterator & operator--(int);
  unsigned long operator *();
  unsigned long count() const;
};

struct SpatialVector {
  static unsigned long kIndexShift;
  static unsigned long kIndexMask;
  static unsigned long kBlockSize;

  unsigned long theTag;
  unsigned long theVector;

  typedef SpatialVectorIterator iterator;

  SpatialVector();
  SpatialVector(SpatialVector const & other);
  SpatialVector & operator = (SpatialVector const & other);
  void setTag(unsigned long aTag) { theTag = aTag; }
  unsigned long tag() const { return theTag; }
  bool contains(tAddress anAddress);
  void insert(tAddress anAddress);

  iterator begin(tAddress aBaseAddress);
  iterator end();
};

std::ostream & operator << (std::ostream & anOstream, SpatialVector const & aVector) ;

typedef __gnu_cxx::hash_map<unsigned long, SpatialVector> SpatialTable;


using Flexus::SharedTypes::PhysicalMemoryAddress;

class SpatialPredictor {
    std::string theName;

    unsigned long theBlockSize;
    unsigned long theSpatialGroupSize;
    unsigned long kGroupMask;
    unsigned long kBlockAddrMask;

    unsigned long kPCShift;
    unsigned long kBlockShift;
    unsigned long kBlockMask;

    SpatialTable theSGP;
    SpatialTable theCacheState;

    MultiCache * theCache;

  public:
    SpatialPredictor (std::string aName, unsigned long aBlockSize, unsigned long aSpatialGroupSize);
    virtual ~SpatialPredictor ();
    virtual void finalize();

    virtual void access( TraceData & anEvent  );
    virtual void eviction( TraceData & anEvent );

    void setCache(MultiCache * aCache);
    unsigned long makeSGPTag(tAddress anAddress, tVAddress aPC);
    void computeStats( SpatialVector const & anExisting, SpatialVector const & aNew );

    void doForward( unsigned long aTag, tAddress anAddress, tVAddress aPC );

};

#endif //SPATIALPREDICTOR_INCLUDED
