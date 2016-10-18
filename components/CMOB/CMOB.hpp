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

#include <list>

#include <core/simulator_layout.hpp>


#define FLEXUS_BEGIN_COMPONENT CMOB
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Slices/CMOBMessage.hpp>
#include <components/Common/Transports/MemoryTransport.hpp>

COMPONENT_PARAMETERS(
    PARAMETER( CMOBSize, unsigned int, "CMOB Size.  Must be divisible by CMOB Line size (12)", "cmobsize", 131072*12  ) 
    PARAMETER( CMOBName, std::string, "CMOB saved file name", "cmob_name", "TMS_CMOB_p1_" )
    PARAMETER( UseMemory, bool, "Model memory traffic/latency for CMOB", "use_memory", true ) 
    PARAMETER( CMOBLineSize, int, "CMOB Line Size", "line_size", 12 ) 
);

typedef boost::intrusive_ptr<CMOBMessage> cmob_message_t;
typedef std::pair<int, long> prefix_read_t;
typedef std::list< PhysicalMemoryAddress > prefix_read_out_t;

COMPONENT_INTERFACE(
  PORT(PushInput, cmob_message_t, TMSif_Request )
  PORT(PushOutput,  cmob_message_t, TMSif_Reply   )

  PORT(PullOutput, long, TMSif_NextAppendIndex )
  PORT(PullOutput, CMOBMessage, TMSif_Initialize )

  PORT(PushOutput, MemoryTransport, ToMemory )
  PORT(PushInput,  MemoryTransport, FromMemory )

  PORT(PushInput,  prefix_read_t, PrefixRead )
  PORT(PushOutput,  prefix_read_out_t, PrefixReadOut )

  DRIVE( CMOBDrive )
);



#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT CMOB
