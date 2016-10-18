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

#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT TMSOnChipIface
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/MemoryTransport.hpp>
#include <components/Common/Slices/CMOBMessage.hpp>
#include <components/Common/Slices/IndexMessage.hpp>

#include <boost/tuple/tuple.hpp>

COMPONENT_PARAMETERS(
    PARAMETER( IndexName, std::string, "Index name", "name", "TMSon_c32768_p1_mrc" )  
);

typedef std::pair<MemoryAddress, bool> append_t;
typedef boost::intrusive_ptr<CMOBMessage> cmob_message_t;
typedef boost::tuple<int, Flexus::SharedTypes::PhysicalMemoryAddress, int, long> mapping_t;
typedef boost::tuple<int/*aCMOB*/, long /* aStart */, long /*anId*/ > cmob_request_t;
typedef boost::tuple<int/*aCMOB*/, long /* aStart */, long /*anId*/, std::vector< MemoryAddress > &, std::vector<bool> & > cmob_reply_t;

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, MonitorL1Replies_In)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, MonitorL1Replies_Out)

  DYNAMIC_PORT_ARRAY(PushOutput,  MemoryTransport, TMSc_L1Replies)

  DYNAMIC_PORT_ARRAY(PushInput,  IndexMessage, TMSc_IndexRequest)
  DYNAMIC_PORT_ARRAY(PushOutput, IndexMessage, TMSc_IndexReply)

  DYNAMIC_PORT_ARRAY(PullInput,  long, CMOB_NextAppendIndex )
  DYNAMIC_PORT_ARRAY(PullInput,  CMOBMessage, CMOB_Initialize)

  DYNAMIC_PORT_ARRAY(PullOutput,  long, TMSc_NextCMOBIndex )
  DYNAMIC_PORT_ARRAY(PushInput,   append_t, TMSc_CMOBAppend )
  DYNAMIC_PORT_ARRAY(PushInput,   cmob_request_t, TMSc_CMOBRequest )
  DYNAMIC_PORT_ARRAY(PushOutput,  cmob_reply_t, TMSc_CMOBReply )

  DYNAMIC_PORT_ARRAY(PushOutput, cmob_message_t, CMOB_Request )
  DYNAMIC_PORT_ARRAY(PushInput,  cmob_message_t, CMOB_Reply   )

);



#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT TMSOnChipIface
