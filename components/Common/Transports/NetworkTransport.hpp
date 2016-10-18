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

#ifndef FLEXUS_TRANSPORTS__NETWORK_TRANSPORT_HPP_INCLUDED
#define FLEXUS_TRANSPORTS__NETWORK_TRANSPORT_HPP_INCLUDED

#include <core/transport.hpp>

#include <components/Common/Slices/MRCMessage.hpp> /* CMU-ONLY */
#include <components/Common/Slices/NetworkMessage.hpp>
#include <components/Common/Slices/PrefetchCommand.hpp> /* CMU-ONLY */
#include <components/Common/Slices/ProtocolMessage.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>

namespace Flexus {
namespace SharedTypes {

  #ifndef FLEXUS_TAG_NetworkMessageTag
  #define FLEXUS_TAG_NetworkMessageTag
    struct NetworkMessageTag_t {};
    struct NetworkMessage;
    namespace {
      NetworkMessageTag_t NetworkMessageTag;
    }
  #endif //FLEXUS_TAG_NetworkMessageTag

  #ifndef FLEXUS_TAG_ProtocolMessageTag
  #define FLEXUS_TAG_ProtocolMessageTag
    struct ProtocolMessageTag_t {};
    struct ProtocolMessage;
    namespace {
      ProtocolMessageTag_t ProtocolMessageTag;
    }
  #endif //FLEXUS_TAG_NetworkMessageTag

/* CMU-ONLY-BLOCK-BEGIN */
  #ifndef FLEXUS_TAG_PrefetchCommandTag
  #define FLEXUS_TAG_PrefetchCommandTag
    struct PrefetchCommandTag_t {};
    struct PrefetchCommand;
    namespace {
      PrefetchCommandTag_t PrefetchCommandTag;
    }
  #endif //FLEXUS_TAG_PrefetchCommandTag

  #ifndef FLEXUS_TAG_MRCMessageTag
  #define FLEXUS_TAG_MRCMessageTag
    struct MRCMessageTag_t {};
    struct MRCMessage;
    namespace {
      MRCMessageTag_t MRCMessageTag;
    }
  #endif //FLEXUS_TAG_PrefetchCommandTag
/* CMU-ONLY-BLOCK-END */

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
        < transport_entry< NetworkMessageTag_t, NetworkMessage >
        , transport_entry< ProtocolMessageTag_t, ProtocolMessage >
/* CMU-ONLY-BLOCK-BEGIN */
        , transport_entry< PrefetchCommandTag_t, PrefetchCommand >
        , transport_entry< MRCMessageTag_t, MRCMessage >
/* CMU-ONLY-BLOCK-END */
        , transport_entry< TransactionTrackerTag_t, TransactionTracker >
        >
      > NetworkTransport;

} //namespace SharedTypes
} //namespace Flexus


#endif //FLEXUS_TRANSPORTS__NETWORK_TRANSPORT_HPP_INCLUDED



