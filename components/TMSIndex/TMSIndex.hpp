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

#define FLEXUS_BEGIN_COMPONENT TMSIndex
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Slices/IndexMessage.hpp>
#include <components/Common/Transports/MemoryTransport.hpp>

COMPONENT_PARAMETERS(
    PARAMETER( BucketsLog2, unsigned int, "Log 2 of number of buckets in index", "buckets_log2", 17  ) 
    PARAMETER( BucketSize, unsigned int, "Number of entries in each Bucket", "bucket_size", 8  ) 
    PARAMETER( UseMemory, bool, "Model memory traffic/latency for CMOB", "use_memory", true ) 
    PARAMETER( QueueSizes, unsigned int, "Size of input and output queues", "queue_size", 32 )
    PARAMETER( IndexName, std::string, "Index name", "name", "TMS_p1_tindex" )
    PARAMETER( FillPrefix, int, "Fill in prefixes in IndexMessages", "fill_prefix", 0 )
);

typedef std::pair<int, long> prefix_read_t;
typedef std::list< PhysicalMemoryAddress > prefix_read_out_t;

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY(PushInput,  IndexMessage, TMSc_Request )
  DYNAMIC_PORT_ARRAY(PushOutput, IndexMessage, TMSc_Reply   )  

  PORT(PushOutput, MemoryTransport, ToMemory )
  PORT(PushInput,  MemoryTransport, FromMemory )

  DYNAMIC_PORT_ARRAY(PushOutput,  prefix_read_t, PrefixRead )
  DYNAMIC_PORT_ARRAY(PushInput,  prefix_read_out_t, PrefixReadIn )

  DRIVE( IndexDrive )
);



#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT TMSIndex
