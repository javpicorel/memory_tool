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

#include <components/Common/Slices/MemoryMessage.hpp>

#define FLEXUS_BEGIN_COMPONENT DecoupledFeeder
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
    PARAMETER( SimicsQuantum, long long, "CPU quantum size in simics", "simics_quantum", 100 )
    PARAMETER( SystemTickFrequency, double, "CPU System tick frequency. 0.0 leaves frequency unchanged", "stick", 0.0 )
    PARAMETER( HousekeepingPeriod, long long, "Simics cycles between housekeeping events", "housekeeping_period", 1000 )
    PARAMETER( TrackIFetch, bool, "Track and report instruction fetches", "ifetch", false )
    PARAMETER( CMPWidth, int, "Number of cores per CMP chip (0 = sys width)", "CMPwidth", 1 )
);

typedef std::pair< unsigned long, unsigned long> ulong_pair;

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, ToL1D )
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, ToL1I )
  DYNAMIC_PORT_ARRAY( PushOutput, ulong_pair, ToBPred )
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, ToNAW )

  PORT( PushOutput, MemoryMessage, ToDMA )
);


#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT DecoupledFeeder
