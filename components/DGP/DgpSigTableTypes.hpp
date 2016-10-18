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
namespace nDgpTable {


enum DgpAccessType {
  WriteAccess,
  SnoopAccess,
  Eviction,
  DontCare
};


template < class Tag >
class DgpBlockType {
public:
  DgpBlockType()
    : myTag(0)
    , myValid(0)
  {}

  const Tag & tag() const {
    return myTag;
  }
  Tag & tag() {
    return myTag;
  }
  bool valid() const {
    return myValid;
  }
  void setValid(bool value) {
    myValid = value;
  }
  const Confidence & conf() const {
    return myConf;
  }
  Confidence & conf() {
    return myConf;
  }

  template<class T>
  friend std::ostream & operator << (std::ostream & anOstream, const DgpBlockType<T> & aBlock) {
    return anOstream << "[tag=0x" << std::hex << aBlock.myTag << " v="
                     << (int)aBlock.myValid << " conf=" << aBlock.myConf << "] ";
  }


private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
      ar & myTag;
      ar & myValid;
      ar & myConf;
    }

  Tag myTag;
  unsigned char myValid;
  Confidence myConf;
};  // end class DgpBlockType

template < class Mapping
         , class Policies
         >
class DgpLookupResultType
  : public Policies::ArrayType::BaseLookupResult
{
  typedef typename Mapping::Address Address;
  typedef typename Policies::ArrayType ArrayType;
  typedef typename Policies::AccessPolicy AccessPolicy;
  typedef typename Policies::ReplacementPolicy ReplacementPolicy;
  typedef typename ArrayType::BaseLookupResult BaseLookupResult;
  typedef typename ArrayType::Tag Tag;
  typedef typename ArrayType::Index Index;
  typedef typename ArrayType::Set Set;
  typedef typename ArrayType::Block Block;

public:

  // CREATION
  static DgpLookupResultType lookup(const Address & anAddress,
                                    ArrayType & anArray,
                                    Mapping & aMapping) {
    Set & set = anArray[ aMapping.index(anAddress) ];
    DBG_(VVerb, ( << "lookup for addr=" << std::hex << anAddress << " set=" << set.index() ) );
    Block * block = BaseLookupResult::find( set, aMapping.tag(anAddress) );
    if(block) {
      return DgpLookupResultType(aMapping, set, *block, true);
    }
    return DgpLookupResultType(aMapping, set, false);
  }

  // ADDRESSING
  Address address() {
    return theMapping.address(this->block().tag(), this->set().index());
  }

  //! CAUSE ARRAY STATE SIDE EFFECTS
  void access(const DgpAccessType & anAccess) {
    DBG_(VVerb, ( << "Performing set-assoc side effects for an access of type " << anAccess ) );
    AccessPolicy::access(*this, anAccess);
    ReplacementPolicy::perform(this->theSet, *(this->theBlock), anAccess);
  }

  // Find a victim block in the set, setting the Block reference in this
  // LookupResult to that victim block
  Block & victim() {
    Block & tempBlock = ReplacementPolicy::victim(this->theSet);
    this->theBlock = &tempBlock;
    return tempBlock;
  }

  // Remove a signature (mark a block as invalid)
  void remove() {
    this->theBlock->setValid(false);
    ReplacementPolicy::perform(this->theSet, *(this->theBlock), Eviction);
  }

private:
  // CONSTRUCTORS
  DgpLookupResultType(Mapping & aMapping,
                      Set & aSet,
                      Block & aBlock,
                      bool aHit)
    : BaseLookupResult(aSet, aBlock, aHit)
    , theMapping(aMapping)
  {}
  DgpLookupResultType(Mapping & aMapping,
                      Set & aSet,
                      bool aHit)
    : BaseLookupResult(aSet, aHit)
    , theMapping(aMapping)
  {}

  // save the mapping to perform addresses in the future
  Mapping & theMapping;

};  // end class DgpLookupResultType


}  // end namespace nDgpTable
