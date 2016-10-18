// DO-NOT-REMOVE begin-copyright-block 
//                                     
// Redistributions of any form whatsoever must retain and/or include the     
// following acknowledgment, notices and disclaimer:                         
//                                                                           
// This product includes software developed by Carnegie Mellon University.   
//                                                                           
// Copyright 2006 by Eric Chung, Brian Gold, Nikos Hardavellas, Jangwoo Kim, 
// Ippokratis Pandis, Minglong Shao, Jared Smolens, Stephen Somogyi,         
// Tom Wenisch, Anastassia Ailamaki, Babak Falsafi and James C. Hoe for      
// the SimFlex Project, Computer Architecture Lab at Carnegie Mellon,        
// Carnegie Mellon University.                                               
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

#define FLEXUS_BEGIN_COMPONENT CmpNetworkControl
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/NetworkTransport.hpp>
#include <components/Common/Transports/MemoryTransport.hpp>


COMPONENT_PARAMETERS(
  FLEXUS_PARAMETER( NumCores, unsigned int, "Number of cores (0 = sys width)", "numCores", 0 )
  FLEXUS_PARAMETER( NumL2Tiles, unsigned int, "Number of L2 tiles in the CMP network", "numL2Tiles", 1)
  FLEXUS_PARAMETER( NumMemControllers, unsigned int, "Number of memory controllers in the CMP", "numMemControllers", 1)
  FLEXUS_PARAMETER( VChannels, int, "Virtual channels", "vc", 3 )

  FLEXUS_PARAMETER( PageSize, int, "Page size in bytes", "page_size", 8192 )
  FLEXUS_PARAMETER( CacheLineSize, int, "Cahe line size in bytes", "cache_line_size", 64 )
  FLEXUS_PARAMETER( L2InterleavingGranularity, unsigned int, "Granularity in bytes at which the L2 tiles are interleaved", "l2InterleavingGranularity", 64)

  FLEXUS_PARAMETER( Floorplan, std::string, "Floorplan; determines network connectivity [woven-torus, tiled-torus, mesh, 1-1, 4-4-line, 4-4-block, cores-top-mesh]", "floorplan", "woven-torus")
  FLEXUS_PARAMETER( Placement, std::string, "Placement policy [shared, private, R-NUCA]", "placement_policy", "shared" )
  FLEXUS_PARAMETER( NumCoresPerTorusRow, unsigned int, "Number of cores per torus row", "torus_row_size", 4 )

  FLEXUS_PARAMETER( SizeOfInstrCluster, unsigned int, "Number of cores in instruction interleaving cluster", "instr_cluster_size", 4 )
  FLEXUS_PARAMETER( SizeOfPrivCluster, unsigned int, "Number of cores in private data interleaving cluster", "priv_cluster_size", 4 )
);

COMPONENT_INTERFACE(
  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, FromNetwork)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ToL2)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ToCore)
  
  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, FromL2)
  DYNAMIC_PORT_ARRAY(PushInput,  MemoryTransport, FromCore)
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, ToNetwork)

  /* CMU-ONLY-BLOCK-BEGIN */
  //nikos: R-NUCA purge
  DYNAMIC_PORT_ARRAY(PushOutput, MemoryTransport, PurgeAddrOut)
  DYNAMIC_PORT_ARRAY(PushInput, MemoryTransport, PurgeAckIn)
  /* CMU-ONLY-BLOCK-END */
  
  DRIVE(CmpNetworkControlDrive)

);



#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT CmpNetworkControl
