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
#include "../Common/SetAssocTypes.hpp"

namespace nDgpTable {

template < class AddressLength >
class DgpMappingType;

template<>
class DgpMappingType<Flexus::Core::Address32Bit> {
public:

  // first define the size of base types used everywhere
  struct Types {
    typedef nSetAssoc::SimpleTag<unsigned int> Tag;
    typedef nSetAssoc::SimpleIndex Index;
    typedef nSetAssoc::SimpleBlockNumber BlockNumber;
  };

  // allow the address type to be extracted
  typedef SignatureMapping::Signature Address;

private:
  typedef Types::Tag Tag;
  typedef Types::Index Index;

public:
  DgpMappingType(unsigned int aNumSets,
                 unsigned int anAssoc)
    : myNumSets(aNumSets)
    , myAssoc(anAssoc)
    , myIndexBits(log_base2(aNumSets))
  {}

  const unsigned int numSets() const {
    return myNumSets;
  }
  const unsigned int assoc() const {
    return myAssoc;
  }

  Index index(const Address & anAddress) const {
    return Index( anAddress & (myNumSets-1) );
  }
  Tag tag(const Address & anAddress) const {
    return Tag( anAddress >> myIndexBits );
  }
  Address address(const Tag & aTag,
                  const Index & anIndex) const {
    return Address( ( aTag << myIndexBits) | anIndex );
  }

private:
  // Number of sets in the array
  const unsigned int myNumSets;

  // Associativity of the array
  const unsigned int myAssoc;

  // Bits required to represent the index
  const unsigned int myIndexBits;

};  // end class DgpMappingType



template < class BaseLookup >
class DgpAccessPolicyType {
public:
  static void access(BaseLookup & aLookup, const DgpAccessType & anAccess) {
    // do nothing - the access type is not sufficient to determine what
    // state the block should go into - more information is needed, and
    // this comes from outside the DGP signature table
  }
};  // end class DgpAccessPolicyType


template < class Set >
class DgpReplacementPolicyType {
  typedef typename Set::Block Block;

public:
  static Block & victim(Set & aSet) {
    // choose the LRU entry - no update of the MRU chain is required
    return aSet[aSet.listTail()];
  }

  static void perform(Set & aSet, Block & aBlock, const DgpAccessType & anAccess) {
    switch(anAccess) {
    case WriteAccess:
    case SnoopAccess:
      // move to MRU position on every access
      aSet.moveToHead(aSet.indexOf(aBlock));
      break;
    case Eviction:
      // move to LRU position
      aSet.moveToTail(aSet.indexOf(aBlock));
      break;
    case DontCare:
    default:
      break;
    }
  }

};  // end class DgpReplacementPolicyType


template < class TArrayType >
class DgpPoliciesType {
public:
  // allow others to extract the array type
  typedef TArrayType ArrayType;

  // convenience typedefs for policies
  typedef DgpReplacementPolicyType<typename ArrayType::Set> ReplacementPolicy;
  typedef DgpAccessPolicyType<typename ArrayType::BaseLookupResult> AccessPolicy;
};  // end class DgpPoliciesType


}  // end namespace nDgpTable
