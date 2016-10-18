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

#define FLEXUS_BEGIN_COMPONENT TrussManager
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#define TrussManager_IMPLEMENTATION (<components/TrussManager/TrussManagerImpl.hpp>)

#ifdef FLEXUS_TrussManager_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::TrussManager data type"
#endif

#define FLEXUS_TrussManager_TYPE_PROVIDED

namespace nTrussManager {

  typedef Flexus::Core::index_t node_id_t;
  typedef unsigned long long CycleCount;

  struct TrussManager : public boost::counted_base {
    static boost::intrusive_ptr<TrussManager> getTrussManager();
    static boost::intrusive_ptr<TrussManager> getTrussManager(node_id_t aRequestingNode);

    virtual bool isMasterNode(node_id_t aNode) = 0;
    virtual bool isSlaveNode(node_id_t aNode) = 0;
    virtual int getSlaveIndex(node_id_t aNode) = 0;
    virtual node_id_t getMasterIndex(node_id_t aNode) = 0;
    virtual node_id_t getProcIndex(node_id_t aNode) = 0;
    virtual node_id_t getSlaveNode(node_id_t aNode, int slave_idx) = 0;
    virtual int getFixedDelay() = 0;

  };

  struct TrussMessage : public boost::counted_base, public FastAlloc {
    TrussMessage() : theTimestamp(0) {}
    TrussMessage(CycleCount cycle, boost::intrusive_ptr<Flexus::SharedTypes::ProtocolMessage> msg) 
      : theTimestamp(cycle), theProtocolMessage(msg) {}
    TrussMessage(CycleCount cycle, boost::intrusive_ptr<Flexus::SharedTypes::MemoryMessage> msg) 
      : theTimestamp(cycle), theMemoryMessage(msg) {}

    CycleCount theTimestamp;
    boost::intrusive_ptr<Flexus::SharedTypes::ProtocolMessage> theProtocolMessage;
    boost::intrusive_ptr<Flexus::SharedTypes::MemoryMessage> theMemoryMessage;
  };


} //nTrussManager

namespace Flexus {
namespace SharedTypes {

using nTrussManager::TrussManager;
using nTrussManager::node_id_t;

struct TrussMessageTag_t {} TrussMessageTag;
#define FLEXUS_TrussMessage_TYPE_PROVIDED
typedef nTrussManager::TrussMessage TrussMessage;

} //End SharedTypes
} //End Flexus

namespace nTrussManager {

  //Add TrussMessage to NetworkTransport
  //=================================

  typedef boost::mpl::push_front<
    FLEXUS_PREVIOUS_NetworkTransport_Typemap,
      std::pair<
          Flexus::SharedTypes::TrussMessageTag_t
        , Flexus::SharedTypes::TrussMessage
        >
      >::type NetworkTransport_Typemap;

  #undef FLEXUS_PREVIOUS_NetworkTransport_Typemap
  #define FLEXUS_PREVIOUS_NetworkTransport_Typemap nTrussManager::NetworkTransport_Typemap

} //nTrussManager

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT TrussManager
