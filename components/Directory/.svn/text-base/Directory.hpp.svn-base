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

#include <components/Common/Transports/DirectoryTransport.hpp>

#define FLEXUS_BEGIN_COMPONENT Directory
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

COMPONENT_PARAMETERS(
  PARAMETER( Latency, int, "Delay for load/store", "latency", 1 )
  PARAMETER( FastLatency, int, "Delay for second consecutive access to same address in bank", "fast_latency", 1 )
  PARAMETER( NumBanks, int, "# Banks for mem&dir", "banks", 1 )
  PARAMETER( CoheBlockSize, int, "Block size of coherence unit", "bsize", 64 )
  PARAMETER( RandomLatency, int, "Random latency range to be added/removed from non-fast accesses", "random_latency", 0 )
  PARAMETER( RandomSeed, unsigned int, "Seed for random latency generator", "random_seed", 1 )
);


COMPONENT_INTERFACE(
  PORT(  PushOutput, DirectoryTransport, DirReplyOut )
  PORT(  PushInput,  DirectoryTransport, DirRequestIn )
  DRIVE( DirectoryDrive )
);

/*
struct DirectoryJumpTable;
struct DirectoryInterface : public Flexus::Core::ComponentInterface {
  struct DirReplyOut {
    typedef Flexus::SharedTypes::DirectoryTransport payload;
    typedef Flexus::Core::aux_::push port_type;
    static const bool is_array = false;
  };

  struct DirRequestIn {
    typedef Flexus::SharedTypes::DirectoryTransport payload;
    typedef Flexus::Core::aux_::push port_type;
    static const bool is_array = false;
  };
  virtual void push(DirRequestIn const &, DirRequestIn::payload &) = 0;
  virtual bool available(DirRequestIn const &) = 0;

  struct DirectoryDrive { static std::string name() { return "DirectoryComponentInterface::DirectoryDrive"; } };
  virtual void drive(DirectoryDrive const & ) = 0;

  static DirectoryInterface * instantiate(DirectoryConfiguration::cfg_struct_ & aCfg, DirectoryJumpTable & aJumpTable, Flexus::Core::index_t anIndex, Flexus::Core::index_t aWidth);
  typedef DirectoryConfiguration_struct configuration;
  typedef DirectoryJumpTable jump_table;
};

struct DirectoryJumpTable {
  typedef DirectoryInterface iface;
  bool (*wire_available_DirReplyOut)(Flexus::Core::index_t anIndex);
  void (*wire_manip_DirReplyOut)(Flexus::Core::index_t anIndex, iface::DirReplyOut::payload & aPayload);
  DirectoryJumpTable() {
    wire_available_DirReplyOut = 0;
    wire_manip_DirReplyOut = 0;
  }
  void check(std::string const & anInstance) {
    if ( wire_available_DirReplyOut == 0 ) { DBG_(Crit, (<< anInstance << " port " "DirReplyOut" " is not wired" ) ); }
  }
};
*/

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT Directory
