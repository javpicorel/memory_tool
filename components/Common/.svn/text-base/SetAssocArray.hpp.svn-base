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
#ifndef SET_ASSOC_ARRAY_HPP_
#define SET_ASSOC_ARRAY_HPP_

#include "SetAssocLookupResult.hpp"

namespace nSetAssoc {


struct Dynamic {};
struct HighAssociative {};
struct LowAssociative {};
struct Infinite {};


template < class UserDefinedSet, class Implementation = LowAssociative>
class SetAssocArray;
  //UserDefinedSet must be:
    //derived from BaseSetDefinition
    //
  //Implementation may be:
    //Dynamic
    //HighAssociative
    //LowAssociative
    //Infinite

template < class UserDefinedSet >
class SetAssocArray<UserDefinedSet, LowAssociative> {

  public:
    // allow others to extract the block and set types
    typedef UserDefinedSet Set;
    typedef typename Set::Block Block;

    // convenience typedefs for basic types
    typedef typename Set::Tag Tag;
    typedef typename Set::BlockNumber BlockNumber;
    typedef typename Set::Index Index;

    typedef aux_::LowAssociativityLookupResult<Set> BaseLookupResult;

  private:
    // The number of sets
    unsigned int myNumSets;

    // The associativity of each set
    unsigned int myAssoc;

    // The actual sets (which contain blocks and hence data)
    Set *theSets;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
      ar & myNumSets;
      for (unsigned int i = 0; i < myNumSets; ++i) {
        ar & theSets[i];
      }
    }

  public:
    // CONSTRUCTION
    SetAssocArray(unsigned int aNumSets, unsigned int anAssoc)
      : myNumSets(aNumSets)
      , myAssoc(anAssoc)
    {
      theSets = (Set*)( new char[myNumSets * sizeof(Set)] );
      // this uses placement new, so on delete, make sure to manually call
      // the destructor for each Set, then delete[] the array
      for(unsigned int ii = 0; ii < myNumSets; ii++) {
        new(theSets+ii) Set(myAssoc, Index(ii));
      }

      //DBG_( VVerb, ( << "Constructed a dynamic set-associative array" ) );
    }

    // SET LOOKUP
    Set & operator[](const Index & anIndex) {
      // TODO: bounds checking/assertion?
      return theSets[anIndex];
    }
    const Set & operator[](const Index & anIndex) const {
      // TODO: bounds checking/assertion?
      return theSets[anIndex];
    }

};  // end class SetAssocArray



}  // end namespace nSetAssoc

#endif
