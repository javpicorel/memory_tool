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
#ifndef SET_ASSOC_SET_DEFN_HPP_
#define SET_ASSOC_SET_DEFN_HPP_

namespace nSetAssoc {


template
  < class Types
  , class TBlock
  >
class BaseSetDefinition {
  //Types must provide
    // a Tag typedef
    // a BlockNumber typedef
    // an Index typedef
  //TBlock should be the block type.
    //It must supply at least these functions:
    //Default-constructor
    //

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
      ar & myAssoc;
      ar & myIndex;
      for (unsigned int i = 0; i < myAssoc; ++i) {
        ar & theBlocks[i];
      }
    }

public:

  // convenience typedefs
  typedef typename Types::Tag Tag;
  typedef typename Types::BlockNumber BlockNumber;
  typedef typename Types::Index Index;

  // allow others to extract the block type
  typedef TBlock Block;

  // CONSTRUCTION
  BaseSetDefinition(const unsigned int anAssoc,
                    const Index anIndex)
    : myIndex(anIndex)
    , myAssoc(anAssoc)
  {
    unsigned int ii;
    theBlocks = (Block*)( new char[myAssoc * sizeof(Block)] );
    // this uses placement new, so on delete, make sure to manually call
    // the destructor for each Block, then delete[] the array
    for(ii=0; ii < myAssoc; ii++) {
      new(theBlocks+ii) Block();    //No corresponding delete yet
    }
  }

  // BLOCK LOOKUP
  Block & operator[] (const BlockNumber & aBlockNumber) {
    // TODO: bounds checking/assertion?
    return theBlocks[aBlockNumber];
  }
  const Block & operator[] (const BlockNumber & aBlockNumber) const {
    // TODO: bounds checking/assertion?
    return theBlocks[aBlockNumber];
  }

  // INDEXING
  BlockNumber indexOf(const Block & aBlock) const {
    int index = &aBlock - theBlocks;
    DBG_Assert((index >= 0) && (index < (int)myAssoc), ( << "index=" << index ) );
    return BlockNumber(index);
  }
  Index index() const {
    return myIndex;
  }

  // ITERATION
  typedef Block * BlockIterator;

  BlockIterator blocks_begin() {
    return theBlocks;
  }
  const BlockIterator blocks_begin() const {
    return theBlocks;
  }
  BlockIterator blocks_end() {
    return theBlocks + myAssoc;
  }
  const BlockIterator blocks_end() const {
    return theBlocks + myAssoc;
  }

  friend std::ostream & operator << (std::ostream & anOstream, const BaseSetDefinition<Types,TBlock> & aSet) {
    anOstream << "set " << aSet.myIndex << ": ";
    for(BlockIterator iter = aSet.blocks_begin(); iter != aSet.blocks_end(); ++iter) {
      anOstream << *iter;
    }
    return anOstream;
  }


protected:
  // The index of this set
  Index myIndex;

  // The associativity of this set
  unsigned int myAssoc;

  // The actual blocks that belong to this set
  Block * theBlocks;

};  // end BaseSetDefinition


}  // end namespace nSetAssoc

#endif
