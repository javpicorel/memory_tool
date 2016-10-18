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

#include <components/Common/Transports/MemoryTransport.hpp>

#define FLEXUS_BEGIN_COMPONENT FastBus
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( BlockSize, int, "Size of coherence unit", "bsize", 64 )
  PARAMETER( TrackDMA, bool, "Track DMA blocks", "dma", true )
  PARAMETER( TrackWrites, bool, "Track/Notify on upgrades", "upgrades", false )
  PARAMETER( TrackProductions, bool, "Track/Notify on productions", "productions", false )
  PARAMETER( TrackReads, bool, "Track/Notify on off-chip reads", "offchipreads", false )
  PARAMETER( InvalAll, bool, "Invalidate all nodes on exclusive", "invalall", false )
  PARAMETER( TrackEvictions, bool, "Track/Notify on evictions", "evictions", false )
  PARAMETER( TrackFlushes, bool, "Track/Notify on flushes", "flushes", false )
  PARAMETER( TrackInvalidations, bool, "Track/Notify on invalidations", "invalidations", false )
	PARAMETER( PageSize, unsigned int, "Page size in bytes", "pagesize", 8192)
	PARAMETER( RoundRobin, bool, "Use static round-robin page allocation", "round_robin", true)
	PARAMETER( SavePageMap, bool, "Save page_map.out in checkpoints", "save_page_map", true)
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY( PushOutput, MemoryMessage, ToSnoops)
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, FromCaches)

  PORT( PushOutput, MemoryMessage, Writes )
  PORT( PushOutput, MemoryMessage, Reads )
  PORT( PushOutput, MemoryMessage, Fetches )
  PORT( PushOutput, MemoryMessage, Evictions )
  PORT( PushOutput, MemoryMessage, Flushes )
  PORT( PushOutput, MemoryMessage, Invalidations )

  PORT( PushInput, MemoryMessage, DMA )
  DYNAMIC_PORT_ARRAY( PushInput, MemoryMessage, NonAllocateWrite )

);


#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT FastBus
