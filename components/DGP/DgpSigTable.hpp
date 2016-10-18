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
#include "../Common/SetAssocArray.hpp"    // normal array
#include "../Common/SetAssocSetDefn.hpp"  // base set definition
#include "../Common/SetAssocReplacement.hpp"    // used for the replacement extension

#include "DgpSigTableTypes.hpp"
#include "DgpSigTablePolicies.hpp"

namespace nDgpTable {


class DgpSigTable {

  // construct the required types
  typedef DgpMappingType<Flexus::Core::Address32Bit> DgpMapping;
public:
  typedef DgpBlockType<DgpMapping::Types::Tag> DgpBlock;
private:
  // create the Set type - first the MRU extension, then our own top level
  typedef nSetAssoc::BaseSetDefinition<DgpMapping::Types, DgpBlock> DgpBaseSet;
  typedef nSetAssoc::LowAssociativeMRUSet<DgpBaseSet> DgpMruSet;
  class DgpSet
    : public DgpMruSet
  {
  public:
    DgpSet(const unsigned int anAssoc,
           const Index anIndex)
      : DgpMruSet(anAssoc, anIndex)
    {}

    friend std::ostream & operator << (std::ostream & anOstream, const DgpSet & aSet) {
      anOstream << "set " << aSet.myIndex << " (mru order): ";
      // traverse the MRU list and list blocks in this order
      for(unsigned int ii = 0; ii < aSet.myAssoc; ii++) {
        const DgpBlock & block = aSet[aSet.mruList[ii]];
        if(block.valid()) {
          anOstream << block;
        }
      }
      return anOstream;
    }

  };
  // end class DgpSet ====================

  typedef nSetAssoc::SetAssocArray<DgpSet> DgpArray;
  typedef DgpPoliciesType<DgpArray> DgpPolicies;
public:
  typedef DgpLookupResultType<DgpMapping, DgpPolicies> DgpLookupResult;

  typedef DgpMapping::Address Address;
  typedef DgpMapping::Types::Tag Tag;
  typedef DgpMapping::Types::Index Index;

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & theArray;
  }

public:
  DgpSigTable(unsigned int numSets,
              unsigned int assoc)
    : theMapping(numSets, assoc)
    , theArray(numSets, assoc)
  {}


  DgpLookupResult operator[](const Address & anAddress) {
    return DgpLookupResult::lookup(anAddress, theArray, theMapping);
  }

  Tag makeTag(const Address & anAddress) {
    return theMapping.tag(anAddress);
  }

  Address makeAddress(const Tag & aTag, const Index & anIndex) {
    return theMapping.address(aTag, anIndex);
  }

  friend std::ostream & operator << (std::ostream & anOstream, const DgpSigTable & aTable) {
    anOstream << "Entries: " << aTable.theMapping.numSets()*aTable.theMapping.assoc()
              << "  Sets: " << aTable.theMapping.numSets()
              << "  Assoc: " << aTable.theMapping.assoc();
    return anOstream;
  }

  void finalize(DgpStats * stats) {
    // nothing to do
  }

private:
  DgpMapping theMapping;
  DgpArray theArray;

};  // end class DgpSigTable


}  // end namespace nDgpTable
