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

#include <components/Common/DoubleWord.hpp>

namespace Flexus {
namespace SharedTypes {

bool operator== (unsigned long long compare, DoubleWord const & aDoubleWord) {
  bool ret_val(true);
  for( int i = 7; i >= 0 && ret_val; --i) {
    if (aDoubleWord.theValidBits & (1<<i)) {
      ret_val = ((aDoubleWord.theDoubleWord >>(8*i )) & 0xFF) == ((compare >>(8*i)) & 0xFF);
    }
  }
  return ret_val;
};

bool operator== (DoubleWord const & entry, unsigned long long compare) {
  return compare == entry;
}

std::ostream & operator <<(std::ostream & anOstream, DoubleWord const & aDoubleWord) {

  anOstream << "DW<<" << &std::hex << (unsigned int) aDoubleWord.theValidBits << ',' << aDoubleWord.theDoubleWord << ">> ";
  anOstream << "0x" << &std::hex;
  unsigned long long value(aDoubleWord.theDoubleWord);
  for( int i = 7; i >= 0; --i) {
    if (aDoubleWord.theValidBits & (1<<i)) {
      anOstream << (unsigned int) ((value>>(8*i + 4)) & 0xF);
      anOstream << (unsigned int) ((value>>(8*i)) & 0xF);
    } else {
      anOstream << "--";
    }
  }
  anOstream << & std::dec;
  return anOstream;
}

} //SharedTypes
} //Flexus
