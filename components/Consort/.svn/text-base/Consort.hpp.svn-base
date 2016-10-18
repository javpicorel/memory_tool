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

#define FLEXUS_BEGIN_COMPONENT Consort
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/NetworkTransport.hpp>
#include <components/Common/Transports/PredictorTransport.hpp>

COMPONENT_PARAMETERS(
    PARAMETER( Enable, bool, "Enable Reflector", "enable", false )
    PARAMETER( OrderSize, unsigned int, "Size of order circular buffer", "size", 250000 )
    PARAMETER( AddressList, unsigned int, "Size of address list for each request", "list", 16)
    PARAMETER( CoherenceBlockSize, unsigned int, "Size of coherence unit", "block_size", 64)
    PARAMETER( OrderReadLatency, unsigned int, "Latency to fetch a block from the CMOB", "cmob_lat", 0)
    PARAMETER( OrderCacheSize, unsigned int, "Size of the order cache", "cmob_cache", 32)
    PARAMETER( VChannels, int, "Virtual channels", "vc", 3)
    PARAMETER( Trace, bool, "Create trace", "trace", false)
    PARAMETER( AllMisses, bool, "Record all off-chip misses", "all_misses", false)
);

COMPONENT_INTERFACE(
  PORT(PushOutput, NetworkTransport, ToNic )
  DYNAMIC_PORT_ARRAY(PushInput, NetworkTransport, FromNic )
  PORT(PushInput, PredictorTransport, FromCPU )
  DRIVE( ConsortDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT Consort
