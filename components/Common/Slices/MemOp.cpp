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


#include <iostream>

#include <core/types.hpp>

#include <components/Common/Slices/MemOp.hpp>

namespace Flexus {
namespace SharedTypes {

std::ostream & operator <<( std::ostream & anOstream, eOperation op) {
  char * map_tables[] =
    { "Load"
    , "AtomicPreload"
    , "RMW"
    , "CAS"
    , "StorePrefetch"
    , "Store"
    , "Invalidate"
    , "Downgrade"
    , "Probe"
    , "Return"
    , "LoadReply"
    , "AtomicPreloadReply"
    , "StoreReply"
    , "StorePrefetchReply"
    , "RMWReply"
    , "CASReply"
    , "DowngradeAck"
    , "InvAck"
    , "ProbeAck"
    , "ReturnReply"
    , "MEMBARMarker"
    , "LastOperation"
    };
  if (op >= kLastOperation) {
    anOstream << "Invalid Operation(" << static_cast<int>(op) << ")";
  } else {
    anOstream << map_tables[op];
  }
  return anOstream;
}

std::ostream & operator << ( std::ostream & anOstream, MemOp const & aMemOp) {
  anOstream << aMemOp.theOperation << "(" << aMemOp.theSize << ") " << aMemOp.theVAddr << "[" << aMemOp.theASI << "] " << aMemOp.thePAddr << " pc:" << aMemOp.thePC;
  if (aMemOp.theReverseEndian) {
    anOstream << " {reverse-endian}";
  }
  if (aMemOp.theNonCacheable) {
    anOstream << " {non-cacheable}";
  }
  if (aMemOp.theSideEffect) {
    anOstream << " {side-effect}";
  }
  if (aMemOp.theNAW) {
    anOstream << " {non-allocate-write}";
  }
  anOstream << " =" << aMemOp.theValue;
  return anOstream;
}

} //SharedTypes
} //Flexus
