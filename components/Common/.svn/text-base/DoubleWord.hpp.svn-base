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
#ifndef FLEXUS_COMMON__DOUBLE_WORD_HPP_INCLUDED
#define FLEXUS_COMMON__DOUBLE_WORD_HPP_INCLUDED

#include <core/debug/debug.hpp>


namespace Flexus {
namespace SharedTypes {

struct DoubleWord {
    unsigned long long theDoubleWord;
    unsigned char theValidBits;

    DoubleWord()
      : theDoubleWord(0xEAEAEAEA)
      , theValidBits(0)
      {}

    DoubleWord(unsigned long long aULL)
      : theDoubleWord(aULL)
      , theValidBits(0xFF)
      {}

    DoubleWord(unsigned long long aBase, DoubleWord const & anApplyVal)
      : theDoubleWord(aBase)
      , theValidBits(0xFF)
    {
      apply(anApplyVal);
    }

    void set(unsigned long long aValue, int aSize, unsigned int anOffset) {
      switch(aSize) {
        case 1:
          getByte( anOffset ) = aValue;
          theValidBits |= (0x1 << (7 - anOffset));
          break;
        case 2:
          getHalfWord( anOffset ) = aValue;
          theValidBits |= (0x3 << (6 - anOffset));
          break;
        case 4:
          getWord( anOffset ) = aValue;
          theValidBits |= (0xF << (4 - anOffset));
          break;
        case 8:
          getDoubleWord( anOffset ) = aValue;
          theValidBits = 0xFF;
          break;
        default:
          DBG_Assert(false, (<< "Unsupported size for DoubleWord::set") );
      }
    }

    bool isValid(int aSize, unsigned int anOffset) const {
      unsigned char bits = 0;
      switch(aSize) {
        case 1:
          bits = 0x1 << (7 - anOffset);
          break;
        case 2:
          bits = 0x3 << (6 - anOffset);
          break;
        case 4:
          bits = 0xF << (4 - anOffset);
          break;
        case 8:
          bits = 0xFF;
          break;
        default:
          return false;
      }
      bool ret_val = (theValidBits & bits) == bits;
      return ret_val;
    }

    bool isEqual(unsigned long long aValue, int aSize, unsigned int anOffset) {
      if (! isValid(aSize, anOffset)) {
        return false;
      }
      switch(aSize) {
        case 1:
          return getByte( anOffset ) == static_cast<unsigned char>(aValue);
        case 2:
          return getHalfWord( anOffset ) == static_cast<unsigned short>(aValue);
        case 4:
          return getWord( anOffset ) == static_cast<unsigned long>(aValue);
        case 8:
          return getDoubleWord( anOffset ) == aValue;
        default:
          DBG_Assert(false, (<< "Unsupported size for DoubleWord::set") );
          return false;
      }
    }

    unsigned char & getByte( unsigned int anOffset) {
      return *( reinterpret_cast< unsigned char *>(&theDoubleWord) + (7 - anOffset) );
    }
    unsigned short & getHalfWord( unsigned int anOffset) {
      return *( reinterpret_cast< unsigned short *>(&theDoubleWord) + (3 - (anOffset>>1)) );
    }
    unsigned long & getWord( unsigned int anOffset) {
      return *( reinterpret_cast< unsigned long *>(&theDoubleWord) + (1 - (anOffset>>2) ) );
    }
    unsigned long long & getDoubleWord( unsigned int anOffset /* ignored */) {
      DBG_Assert(anOffset == 0);
      return theDoubleWord;
    }

    void apply( DoubleWord const & aMask) {
      if (aMask.theValidBits != 0) {
       unsigned long long masked_val = 0;
        for( int i = 7; i >= 0; --i) {
          masked_val <<= 8;
          if (aMask.theValidBits & (1<<i)) {
            theValidBits |= (1<<i);
            masked_val |= ((aMask.theDoubleWord >>(8*i )) & 0xFF);
          } else {
            masked_val |= ((theDoubleWord >>(8*i )) & 0xFF);
          }
        }
        theDoubleWord = masked_val;
      }
    }

};

bool operator== (unsigned long long compare, DoubleWord const & aDoubleWord);
bool operator== (DoubleWord const & entry, unsigned long long compare);
std::ostream & operator <<(std::ostream & anOstream, DoubleWord const & aDoubleWord);

} //SharedTypes
} //Flexus

#endif //FLEXUS_COMMON__DOUBLE_WORD_HPP_INCLUDED
