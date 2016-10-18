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
#ifndef SET_ASSOC_LOOKUP_RESULT_HPP_
#define SET_ASSOC_LOOKUP_RESULT_HPP_

#include <core/exception.hpp>

namespace nSetAssoc {
namespace aux_ {

template <class UserDefinedSet>
struct BaseLookupResult {
    typedef UserDefinedSet Set;
    typedef typename Set::Block Block;
    typedef typename Set::Tag Tag;

  public:
    //! HIT/MISS check
    bool hit() const {
      return isHit;
    }
    bool miss() const {
      return !isHit;
    }

    //! SET access
    Set & set() {
      return theSet;
    }
    const Set & set() const {
      return theSet;
    }

    //! BLOCK access
    Block & block() {
      if(theBlock) {
        return *theBlock;
      }
      throw Flexus::Core::FlexusException("no block");
    }
    const Block & block() const {
      if(theBlock) {
        return *theBlock;
      }
      throw Flexus::Core::FlexusException("no block");
    }

  protected:
    //! CONSTRUCTORS
    BaseLookupResult(Set & aSet,
                     Block & aBlock,
                     bool aHit)
      : theSet(aSet)
      , theBlock(&aBlock)
      , isHit(aHit)
    {}
    BaseLookupResult(Set & aSet,
                     bool aHit)
      : theSet(aSet)
      , theBlock(0)
      , isHit(aHit)
    {}

    //! MEMBERS
    Set & theSet;
    Block * theBlock;
    bool isHit;

};

template <class UserDefinedSet>
struct LowAssociativityLookupResult : public BaseLookupResult<UserDefinedSet> {

    //! CREATION
    static typename LowAssociativityLookupResult::Block * find(typename LowAssociativityLookupResult::Set & aSet, const typename LowAssociativityLookupResult::Tag & aTag) {
     for(typename LowAssociativityLookupResult::Set::BlockIterator iter = aSet.blocks_begin(); iter != aSet.blocks_end(); ++iter) {
       if(iter->tag() == aTag) {
         if(iter->valid()) {
           return iter;
         }
       }
     }
     return 0;
    }

  protected:
    //! CONSTRUCTORS
    LowAssociativityLookupResult(typename LowAssociativityLookupResult::Set & aSet,
                     typename LowAssociativityLookupResult::Block & aBlock,
                     bool aHit)
      : BaseLookupResult<UserDefinedSet>(aSet, aBlock, aHit)
      {}
    LowAssociativityLookupResult(typename LowAssociativityLookupResult::Set & aSet,
                     bool aHit)
      : BaseLookupResult<UserDefinedSet>(aSet, aHit)
      {}

};

} //namepspace aux_
} //namepspace nSetAssoc

#endif //SET_ASSOC_LOOKUP_RESULT_HPP_
