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

#define FLEXUS_BEGIN_COMPONENT StridePrefetcher
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/MemoryTransport.hpp>

COMPONENT_PARAMETERS(
  PARAMETER( NumEntries, unsigned int, "# of entries in the stream buffer", "entries", 0 )
  PARAMETER( QueueSizes, unsigned int, "Size of memory message queues", "queues", 8 )
  PARAMETER( MAFSize, unsigned int, "Size of miss address file", "maf", 32 )
  PARAMETER( ProcessingDelay, unsigned int, "Delay to check buffer", "delay", 0 )
  PARAMETER( MaxPrefetchQueue, unsigned int, "Size of prefetch queue", "prefetch_queue", 32 )
  PARAMETER( MaxOutstandingPrefetches, int, "Number of outstanding prefetches", "outstanding", 16 )

);

COMPONENT_INTERFACE(
  PORT(PushOutput, MemoryTransport, FrontSideOut )
  PORT(PushInput, MemoryTransport, FrontSideIn_Snoop)
  PORT(PushInput, MemoryTransport, FrontSideIn_Request)
  PORT(PushInput, MemoryTransport, FrontSideIn_Prefetch)
  PORT(PushOutput, MemoryTransport, BackSideOut_Snoop)
  PORT(PushOutput, MemoryTransport, BackSideOut_Request)
  PORT(PushOutput, MemoryTransport, BackSideOut_Prefetch)
  PORT(PushInput, MemoryTransport, BackSideIn)
  DRIVE(StrideDrive)
);


#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT StridePrefetcher
