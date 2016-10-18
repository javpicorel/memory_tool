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
#ifndef SET_ASSOC_REPLACEMENT_HPP_
#define SET_ASSOC_REPLACEMENT_HPP_

#include <boost/serialization/base_object.hpp>

namespace nSetAssoc {


template < class BaseSet >
class LowAssociativeMRUSet : public BaseSet {
public:

  // convenience typedefs
  typedef typename BaseSet::Tag Tag;
  typedef typename BaseSet::BlockNumber BlockNumber;
  typedef typename BaseSet::Index Index;
  typedef typename BaseSet::Block Block;

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & boost::serialization::base_object<BaseSet>(*this);
    for (unsigned int i = 0; i < this->myAssoc; ++i) {
      ar & mruList[i];
    }
  }

  LowAssociativeMRUSet(const unsigned int anAssoc,
             const Index anIndex)
    : BaseSet(anAssoc, anIndex)
    , mruList(0)
  {
    init();
  }

  void init() {
    unsigned int ii;
    mruList = (BlockNumber*)( new char[this->myAssoc * sizeof(BlockNumber)] );
    // this uses placement new, so on delete, make sure to manually call
    // the destructor for each BlockNumber, then delete[] the array
    for(ii=0; ii < this->myAssoc; ii++) {
      new(mruList+ii) BlockNumber(ii);
    }
  }

  // returns the most recently used block
  BlockNumber listHead() {
    return mruList[0];
  }

  // returns the least recently used block
  BlockNumber listTail() {
    return mruList[this->myAssoc-1];
  }

  // moves the indicated block to the MRU position
  void moveToHead(BlockNumber aNumber) {
    int ii = 0;
    // find the list entry for the specified index
    while(mruList[ii] != aNumber) {
      ii++;
    }
    // move appropriate entries down the MRU chain
    while(ii > 0) {
      mruList[ii] = mruList[ii-1];
      ii--;
    }
    mruList[0] = aNumber;
  }

  // moves the indicated block to the LRU position
  void moveToTail(BlockNumber aNumber) {
    unsigned int ii = 0;
    // find the list entry for the specified index
    while(mruList[ii] != aNumber) {
      ii++;
    }
    // move appropriate entries up the MRU chain
    while(ii < this->myAssoc-1) {
      mruList[ii] = mruList[ii+1];
      ii++;
    }
    mruList[this->myAssoc-1] = aNumber;
  }

  // The list itself.  This is an array of indices corresponding to
  // blocks in the set.  The list is arranged in order of most recently
  // used to least recently used.
  BlockNumber *mruList;

};  // end struct MruSetExtension


}  // end namespace nSetAssoc

#endif //SET_ASSOC_REPLACEMENT_HPP_
