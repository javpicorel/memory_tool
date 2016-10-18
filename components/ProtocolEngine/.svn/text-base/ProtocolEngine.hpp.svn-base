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

#define FLEXUS_BEGIN_COMPONENT ProtocolEngine
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/NetworkTransport.hpp>
#include <components/Common/Transports/MemoryTransport.hpp>
#include <components/Common/Transports/DirectoryTransport.hpp>
#include <components/Common/Transports/PredictorTransport.hpp> /* CMU-ONLY */

COMPONENT_PARAMETERS(
    PARAMETER( Remote, int, "Use Remote Engine", "useRE", 1 )
    PARAMETER( SpuriousSharerOpt, int, "Use SpuriousSharer Optimization", "useSpuriousSharerOpt", 1 )
    PARAMETER( TSRFSize, unsigned int, "TSRF Size", "tsrf", 16 )
    PARAMETER( VChannels, int, "Virtual channels", "vc", 3)
    //PARAMETER(RecvCapacity, unsigned int, "Receive Capacity", "receive-capacity", 8)
    PARAMETER(CPI, int, "Cycles per instr", "cpi", 4)
);

COMPONENT_INTERFACE(
  PORT(PushOutput, NetworkTransport, ToNic )
  DYNAMIC_PORT_ARRAY(PushInput, NetworkTransport, FromNic )
  PORT(PushOutput, DirectoryTransport, ToDirectory )
  PORT(PushInput, DirectoryTransport, FromDirectory )
  PORT(PushOutput, MemoryTransport, ToCpu )
  PORT(PushInput, MemoryTransport, FromCpu )
  PORT(PushOutput, PredictorTransport, ToPredictor ) /* CMU-ONLY */
  DRIVE(ProtocolEngineDrive)
);


#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT ProtocolEngine
