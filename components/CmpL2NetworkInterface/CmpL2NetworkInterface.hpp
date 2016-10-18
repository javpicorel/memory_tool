// DO-NOT-REMOVE begin-copyright-block 
//                                     
// Redistributions of any form whatsoever must retain and/or include the     
// following acknowledgment, notices and disclaimer:                         
//                                                                           
// This product includes software developed by Carnegie Mellon University.   
//                                                                           
// Copyright 2006 by Eric Chung, Brian Gold, Nikos Hardavellas, Jangwoo Kim, 
// Ippokratis Pandis, Minglong Shao, Jared Smolens, Stephen Somogyi,         
// Tom Wenisch, Anastassia Ailamaki, Babak Falsafi and James C. Hoe for      
// the SimFlex Project, Computer Architecture Lab at Carnegie Mellon,        
// Carnegie Mellon University.                                               
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

#define FLEXUS_BEGIN_COMPONENT CmpL2NetworkInterface
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/NetworkTransport.hpp>
#include <components/Common/Transports/MemoryTransport.hpp>


COMPONENT_PARAMETERS(
  FLEXUS_PARAMETER( NumL2Tiles, unsigned int, "Number of L2 tiles in the CMP network", "numL2Tiles", 0)
  FLEXUS_PARAMETER( NumMemControllers, unsigned int, "Number of memory controllers in the CMP", "numMemControllers", 4)
  FLEXUS_PARAMETER( L2InterleavingGranularity, unsigned int, "Granularity in bytes at which the L2 tiles are interleaved", "l2InterleavingGranularity", 64)
  FLEXUS_PARAMETER( Placement, std::string, "Placement policy [shared, private, R-NUCA]", "placement_policy", "shared" )

  FLEXUS_PARAMETER( VChannels, int, "Virtual channels", "vc", 3 )
  FLEXUS_PARAMETER( RecvCapacity, unsigned int, "Recv Queue Capacity", "recv-capacity", 1)
  FLEXUS_PARAMETER( SendCapacity, unsigned int, "Send Queue Capacity", "send-capacity", 1)
  FLEXUS_PARAMETER( RequestVc, int, "Virtual channel used for requests", "requestVc", 0)
  FLEXUS_PARAMETER( SnoopVc, int, "Virtual channel used for snoops", "snoopVc", 1)
  FLEXUS_PARAMETER( ReplyVc, int, "Virtual channel used for replies", "replyVc", 1)
);

COMPONENT_INTERFACE(
  // Out into the network
  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, IReplyFromL2)
  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, DReplyFromL2)
  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, IRequestFromL2)
  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, DRequestFromL2)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ToNetwork)

  // from/to backside
  PORT(PushInput,  MemoryTransport, RequestFromL2BackSide)
  PORT(PushInput,  MemoryTransport, SnoopFromL2BackSide)
  PORT(PushOutput, MemoryTransport, RequestToMem)
  PORT(PushOutput, MemoryTransport, ReplyToL2)
  PORT(PushOutput, MemoryTransport, SnoopToL2)
  PORT(PushInput,  MemoryTransport, ReplyFromMem)

  // In from the network  
  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, FromNetwork)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ISnoopToL2)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, IRequestToL2)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, DSnoopToL2)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, DRequestToL2)
  
  
  DRIVE(CmpL2NetworkInterfaceDrive)
);



#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT CmpL2NetworkInterface
