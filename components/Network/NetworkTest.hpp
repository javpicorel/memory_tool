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

#define FLEXUS_BEGIN_COMPONENT NetworkTest
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#define NetworkTest_IMPLEMENTATION (<components/Network/NetworkTestImpl.hpp>)

#include "../Common/TransactionTracker.hpp"


#ifdef FLEXUS_ProtocolMessage_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::ProtocolMessage data type"
#endif


namespace nNetworkTest {

  using boost::counted_base;

  struct ProtocolMessage : public boost::counted_base
  {
    unsigned int value;
  };

} // End namespace nNetworkTest


namespace Flexus {
namespace SharedTypes {

  struct ProtocolMessageTag_t {} ProtocolMessageTag;

  #define FLEXUS_ProtocolMessage_TYPE_PROVIDED
  typedef nNetworkTest :: ProtocolMessage ProtocolMessage;

} // End namespace SharedTypes
} // End namespace Flexus


namespace nNetworkTest {

  typedef boost::mpl::push_front<
  FLEXUS_PREVIOUS_NetworkTransport_Typemap,
      std::pair<
          Flexus::SharedTypes::ProtocolMessageTag_t
        , Flexus::SharedTypes::ProtocolMessage
        >
      >::type NetworkTransport_Typemap;

  #undef FLEXUS_PREVIOUS_NetworkTransport_Typemap
  #define FLEXUS_PREVIOUS_NetworkTransport_Typemap nNetworkTest::NetworkTransport_Typemap

} // End namespace NetworkTest



#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT NetworkTest
