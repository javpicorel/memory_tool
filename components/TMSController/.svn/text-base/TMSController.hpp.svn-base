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

#define FLEXUS_BEGIN_COMPONENT TMSController
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/PrefetchTransport.hpp>
#include <components/Common/Transports/PredictorTransport.hpp>
#include <components/Common/Transports/MemoryTransport.hpp>
#include <components/Common/Slices/IndexMessage.hpp>

#include <boost/tuple/tuple.hpp>

COMPONENT_PARAMETERS(
    PARAMETER( MaxPrefetches, unsigned int, "Maximum number of outstanding prefetches", "prefetches", 4 )
    PARAMETER( QueueSize, unsigned int, "Size of message queues", "queue_size", 8 )
    PARAMETER( StreamQueues, unsigned int, "Number of stream queues", "stream_queues", 8 )
    PARAMETER( EnableTMS, bool, "Enable TMS", "enable", false )  
    PARAMETER( OnChipTMS, bool, "Run TMS for on-chip streaming", "on_chip", false )  
    PARAMETER( MinBufferCap, int, "Minimum buffer cap", "min_buffer_cap", 4 )  
    PARAMETER( InitBufferCap, int, "Initial buffer cap", "init_buffer_cap", 8 )  
    PARAMETER( MaxBufferCap, int, "Maximum buffer cap", "max_buffer_cap", 24 )  
    PARAMETER( FetchCMOBAt, int, "Low water mark to fetch more address", "cmob_fetch", 4 )  
    PARAMETER( UsePIndex, bool, "Enable the PIndex", "pindex", false )  
    PARAMETER( TIndexInsertProb, float, "TIndex Insertion probability", "insert_prob", 1.0 )
);

typedef std::pair<Flexus::SharedTypes::PhysicalMemoryAddress, bool> append_t;
typedef boost::tuple<int, Flexus::SharedTypes::PhysicalMemoryAddress, int, long> mapping_t;
typedef boost::tuple<int/*aCMOB*/, long /* aStart */, long /*anId*/ > cmob_request_t;
typedef boost::tuple<int/*aCMOB*/, long /* aStart */, long /*anId*/, std::vector< MemoryAddress > &, std::vector<bool> & > cmob_reply_t;

COMPONENT_INTERFACE(

  PORT(PushOutput, PrefetchTransport, ToSVB)
  PORT(PushInput, PrefetchTransport, FromSVB )

  PORT( PushInput, PredictorTransport, CoreNotify )
  PORT( PushOutput, PredictorTransport, CoreNotifyOut )

  PORT(PushInput, MemoryTransport, MonitorMemRequests)

  PORT(PullInput, long, NextCMOBIndex )
  PORT(PushOutput, append_t, CMOBAppend )
  PORT(PushOutput, cmob_request_t, CMOBRequest )
  PORT(PushInput, cmob_reply_t, CMOBReply )

  PORT(PushOutput, IndexMessage, ToTIndex)
  PORT(PushInput,  IndexMessage, FromTIndex )

  PORT(PushOutput, IndexMessage, ToPIndex)
  PORT(PushInput,  IndexMessage, FromPIndex )

  PORT(PushInput, MemoryAddress, RecentRequests)

  DRIVE( TMSControllerDrive )
);



#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT TMSController
