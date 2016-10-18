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

#define FLEXUS_BEGIN_COMPONENT Network
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#define Network_IMPLEMENTATION (<components/Network/NetworkImpl.hpp>)

#ifdef FLEXUS_NetworkMessage_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::NetworkMessage data type"
#endif

#include "../Common/TransactionTracker.hpp"

#include <core/fast_alloc.hpp>

namespace nNetwork {

  using boost::counted_base;

  struct NetworkMessage : public boost::counted_base, FastAlloc
  {
    int src;     // source node
    int dest;    // destination node
    int vc;      // virtual channel
    int size;    // message (i.e. payload) size
    int src_port;
    int dst_port;
  };

} // End namespace nNetwork


namespace Flexus {
namespace SharedTypes {

  struct NetworkMessageTag_t {} NetworkMessageTag;

  #define FLEXUS_NetworkMessage_TYPE_PROVIDED
  typedef nNetwork :: NetworkMessage NetworkMessage;

  #ifndef FLEXUS_TransactionTracker_TYPE_PROVIDED
    struct TransactionTrackerTag_t {} TransactionTrackerTag;
    #define FLEXUS_TransactionTracker_TYPE_PROVIDED
    typedef nTransactionTracker :: TransactionTracker TransactionTracker;
  #endif //FLEXUS_TransactionTracker_TYPE_PROVIDED

} // End namespace SharedTypes
} // End namespace Flexus


namespace nNetwork {

  typedef boost::mpl::push_front<
    FLEXUS_PREVIOUS_NetworkTransport_Typemap,
      std::pair<
          Flexus::SharedTypes::NetworkMessageTag_t
        , Flexus::SharedTypes::NetworkMessage
        >
      >::type temp_Typemap;


  typedef boost::mpl::push_front<
    temp_Typemap,
      std::pair<
          Flexus::SharedTypes::TransactionTrackerTag_t
        , Flexus::SharedTypes::TransactionTracker
        >
      >::type NetworkTransport_Typemap;

  #undef FLEXUS_PREVIOUS_NetworkTransport_Typemap
  #define FLEXUS_PREVIOUS_NetworkTransport_Typemap nNetwork::NetworkTransport_Typemap

} // End namespace nNetwork




#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT Network
