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
#include <components/Common/Slices/TransactionTracker.hpp>

#define FLEXUS_BEGIN_COMPONENT Cache
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
    PARAMETER( Cores, int, "Number of cores", "cores", 1 )
    PARAMETER( Size, int, "Cache size in bytes", "size", 16384 )
    PARAMETER( Associativity, int, "Set associativity", "assoc", 2 )
    PARAMETER( BlockSize, int, "Block size", "bsize", 64 )
    PARAMETER( PageSize, unsigned long long, "Page size in bytes", "page_size", 8192 )
    PARAMETER( Ports, unsigned int, "Number of ports on data and tag arrays", "ports", 1 )
    PARAMETER( Banks, unsigned int, "number of banks on the data and tag arrays", "banks", 1 )
    PARAMETER( TagLatency, unsigned int, "Total latency of tag pipeline", "tag_lat", 1 )
    PARAMETER( TagIssueLatency, unsigned int, "Minimum delay between issues to tag pipeline", "dup_tag_issue_lat", 0 )
    PARAMETER( DataLatency, unsigned int, "Total latency of data pipeline", "data_lat", 1 )
    PARAMETER( DataIssueLatency, unsigned int, "Minimum delay between issues to data pipeline", "data_issue_lat", 0 )
    PARAMETER( CacheLevel, Flexus::SharedTypes::tFillLevel, "CacheLevel", "level", Flexus::SharedTypes::eUnknown )
    PARAMETER( QueueSizes, unsigned int, "Size of input and output queues", "queue_size", 8 )
    PARAMETER( PreQueueSizes, unsigned int, "Size of input arbitration queues", "pre_queue_size", 4 )
    PARAMETER( MAFSize, unsigned int, "Number of MAF entries", "maf", 32 )
    PARAMETER( MAFTargetsPerRequest, unsigned int, "Number of MAF targets per request", "maf_targets", 8)
    PARAMETER( EvictBufferSize, unsigned int, "Number of Evict Buffer entries", "eb", 40 )
    PARAMETER( ProbeFetchMiss, bool, "Probe hierarchy on Ifetch miss", "probe_fetchmiss", false )
    PARAMETER( BusTime_NoData, unsigned int, "Bus transfer time - no data", "bustime_nodata", 1 )
    PARAMETER( BusTime_Data, unsigned int, "Bus transfer time - data", "bustime_data", 2 )
    PARAMETER( EvictClean, bool, "Cause the cache to evict clean blocks", "allow_evict_clean", false )
    PARAMETER( FastEvictClean, bool, "Send clean evicts without reserving data bus", "fast_evict_clean", false )
    PARAMETER( NoBus, bool, "No bus model (i.e., infinite BW, no latency)", "no_bus", false )
    PARAMETER( TraceAddress, unsigned int, "Address to initiate tracing", "trace_address", 0 )
    PARAMETER( Placement, std::string, "Placement policy [shared, private, R-NUCA]", "placement_policy", "shared" )
    PARAMETER( PrivateWithASR, bool, "Turn on ASR under Private scheme", "private_asr_enable", false ) /* CMU-ONLY */
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, FrontSideIn_Snoop)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, FrontSideIn_Request)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, FrontSideIn_Prefetch)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, FrontSideIn_Purge) /* CMU-ONLY */
  PORT(PushOutput, MemoryTransport, BackSideOut_Snoop)
  PORT(PushOutput, MemoryTransport, BackSideOut_Request)
  PORT(PushOutput, MemoryTransport, BackSideOut_Prefetch)

  // for chip coherence
  // PORT(PushInput, MemoryTransport, BackSideIn)
  PORT(PushInput, MemoryTransport, BackSideIn_Request)
  PORT(PushInput, MemoryTransport, BackSideIn_Reply)
  // DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, FrontSideOut )
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, FrontSideOut_Request )
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, FrontSideOut_Reply )

  PORT( PushOutput, bool, RequestSeen) // Cache access from core for PowerTracker
  
  DRIVE(CacheDrive)
);


#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT Cache
