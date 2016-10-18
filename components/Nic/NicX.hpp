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

#ifndef FLEXUS_NIC_NUMPORTS
#error "FLEXUS_NIC_NUMPORTS must be defined"
#endif

#include <core/simulator_layout.hpp>
#include <boost/preprocessor/repeat.hpp>

#define NicX BOOST_PP_CAT(Nic, FLEXUS_NIC_NUMPORTS)

#define FLEXUS_BEGIN_COMPONENT NicX
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/NetworkTransport.hpp>

#define NodeOutputPort(z,N,_) DYNAMIC_PORT_ARRAY( PushOutput, NetworkTransport, BOOST_PP_CAT(ToNode,N) )
#define NodeInputPort(z,N,_) PORT( PushInput, NetworkTransport, BOOST_PP_CAT(FromNode,N) )


COMPONENT_PARAMETERS(
  FLEXUS_PARAMETER( VChannels, int, "Virtual channels", "vc", 3 )
  FLEXUS_PARAMETER( RecvCapacity, unsigned int, "Recv Queue Capacity", "recv-capacity", 1)
  FLEXUS_PARAMETER( SendCapacity, unsigned int, "Send Queue Capacity", "send-capacity", 1)
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushOutput, NetworkTransport, ToNetwork )
  DYNAMIC_PORT_ARRAY( PushInput, NetworkTransport, FromNetwork )
  BOOST_PP_REPEAT(FLEXUS_NIC_NUMPORTS,NodeOutputPort, _ )
  BOOST_PP_REPEAT(FLEXUS_NIC_NUMPORTS,NodeInputPort, _ )
  DRIVE( NicDrive )
);


#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT NicX
