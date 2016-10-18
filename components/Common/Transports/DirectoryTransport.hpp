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

#ifndef FLEXUS_TRANSPORTS__DIRECTORY_TRANSPORT_HPP_INCLUDED
#define FLEXUS_TRANSPORTS__DIRECTORY_TRANSPORT_HPP_INCLUDED

#include <core/transport.hpp>

namespace Flexus {
namespace SharedTypes {

  #ifndef FLEXUS_TAG_DirectoryMessageTag
  #define FLEXUS_TAG_DirectoryMessageTag
    struct DirectoryMessageTag_t {};
    struct DirectoryMessage;
    namespace {
      DirectoryMessageTag_t DirectoryMessageTag;
    }
  #endif //FLEXUS_TAG_DirectoryMessageTag

  #ifndef FLEXUS_TAG_DirectoryEntryTag
  #define FLEXUS_TAG_DirectoryEntryTag
    struct DirectoryEntryTag_t {};
    struct DirectoryEntry;
    namespace {
      DirectoryEntryTag_t DirectoryEntryTag;
    }
  #endif //FLEXUS_TAG_DirectoryEntryTag

  #ifndef FLEXUS_TAG_DirMux2ArbTag
  #define FLEXUS_TAG_DirMux2ArbTag
    struct DirMux2ArbTag_t {};
    struct Mux;
    namespace {
      DirMux2ArbTag_t DirMux2ArbTag;
    }
  #endif //FLEXUS_TAG_DirMux2ArbTag

  typedef Transport
      < mpl::vector
        < transport_entry< DirectoryMessageTag_t, DirectoryMessage >
        , transport_entry< DirectoryEntryTag_t, DirectoryEntry >
        , transport_entry< DirMux2ArbTag_t, Mux >
        >
      > DirectoryTransport;

} //namespace SharedTypes
} //namespace Flexus


#endif //FLEXUS_TRANSPORTS__DIRECTORY_TRANSPORT_HPP_INCLUDED



