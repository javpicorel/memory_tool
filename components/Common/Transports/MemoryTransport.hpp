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

#ifndef FLEXUS_TRANSPORTS__MEMORY_TRANSPORT_HPP_INCLUDED
#define FLEXUS_TRANSPORTS__MEMORY_TRANSPORT_HPP_INCLUDED

#include <core/transport.hpp>

#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/Slices/ExecuteState.hpp>
#include <components/Common/Slices/MemOp.hpp>
#include <components/Common/Slices/Mux.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>
#include <components/Common/Slices/DirectoryEntry.hpp>

namespace Flexus {
namespace SharedTypes {

  #ifndef FLEXUS_TAG_MemoryMessageTag
  #define FLEXUS_TAG_MemoryMessageTag
    struct MemoryMessageTag_t {};
    namespace {
      MemoryMessageTag_t MemoryMessageTag;
    }
  #endif //FLEXUS_TAG_MemoryMessageTag

  #ifndef FLEXUS_TAG_ExecuteStateTag
  #define FLEXUS_TAG_ExecuteStateTag
    struct ExecuteStateTag_t {};
    namespace {
      ExecuteStateTag_t ExecuteStateTag;
    }
  #endif //FLEXUS_TAG_ExectueStateTag

  #ifndef FLEXUS_TAG_uArchStateTag
  #define FLEXUS_TAG_uArchStateTag
    struct uArchStateTag_t {};
    namespace {
      uArchStateTag_t uArchStateTag;
    }
  #endif //FLEXUS_TAG_uArchStateTag

  #ifndef FLEXUS_TAG_MuxTag
  #define FLEXUS_TAG_MuxTag
    struct MuxTag_t {};
    namespace {
      MuxTag_t MuxTag;
    }
  #endif //FLEXUS_TAG_MuxTag

  #ifndef FLEXUS_TAG_BusTag
  #define FLEXUS_TAG_BusTag
    struct BusTag_t {};
    namespace {
      BusTag_t BusTag;
    }
  #endif //FLEXUS_TAG_BusTag

  #ifndef FLEXUS_TAG_DirectoryEntryTag
  #define FLEXUS_TAG_DirectoryEntryTag
    struct DirectoryEntryTag_t {};
    struct DirectoryEntry;
    namespace {
      DirectoryEntryTag_t DirectoryEntryTag;
    }
  #endif //FLEXUS_TAG_DirectoryEntryTag

  #ifndef FLEXUS_TAG_TransactionTrackerTag
  #define FLEXUS_TAG_TransactionTrackerTag
    struct TransactionTrackerTag_t {};
    struct TransactionTracker;
    namespace {
      TransactionTrackerTag_t TransactionTrackerTag;
    }
  #endif //FLEXUS_TAG_TransactionTrackerTag



  typedef Transport
      < mpl::vector
        < transport_entry< MemoryMessageTag_t, MemoryMessage >
        , transport_entry< ExecuteStateTag_t, ExecuteState >
        , transport_entry< uArchStateTag_t, MemOp >
        , transport_entry< MuxTag_t, Mux >
        , transport_entry< BusTag_t, Mux >
        , transport_entry< DirectoryEntryTag_t, DirectoryEntry >
        , transport_entry< TransactionTrackerTag_t, TransactionTracker >
        >
      > MemoryTransport;

} //namespace SharedTypes
} //namespace Flexus


#endif //FLEXUS_TRANSPORTS__MEMORY_TRANSPORT_HPP_INCLUDED



