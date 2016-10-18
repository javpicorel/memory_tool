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

#define FLEXUS_BEGIN_COMPONENT DirTest
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#include <components/Common/Transports/DirectoryTransport.hpp>


COMPONENT_NO_PARAMETERS ;

/*
COMPONENT_INTERFACE(
  PORT(  PushOutput, DirectoryTransport, RequestOut )
  PORT(  PushInput,  DirectoryTransport, ResponseIn )
  DRIVE( DirTestDrive )
);
*/


struct DirTestJumpTable;
struct DirTestInterface : public Flexus::Core::ComponentInterface {
  struct RequestOut {
    typedef Flexus::SharedTypes::DirectoryTransport payload;
    typedef Flexus::Core::aux_::push port_type;
    static const bool is_array = false;
  };

  struct ResponseIn {
    typedef Flexus::SharedTypes::DirectoryTransport payload;
    typedef Flexus::Core::aux_::push port_type;
    static const bool is_array = false;
  };
  virtual void push(ResponseIn const &, ResponseIn::payload &) = 0;
  virtual bool available(ResponseIn const &) = 0;

  struct DirTestDrive { static std::string name() { return "DirTestInterface::DirTestDrive"; } };
  virtual void drive(DirTestDrive const & ) = 0;

  static DirTestInterface * instantiate(DirTestConfiguration::cfg_struct_ & aCfg, DirTestJumpTable & aJumpTable, Flexus::Core::index_t anIndex, Flexus::Core::index_t aWidth);
  typedef DirTestConfiguration_struct configuration;
  typedef DirTestJumpTable jump_table;

};

struct DirTestJumpTable {
  typedef DirTestInterface iface;
  bool (*wire_available_RequestOut)(Flexus::Core::index_t anIndex);
  void (*wire_manip_RequestOut)(Flexus::Core::index_t anIndex, iface::RequestOut::payload & aPayload);
  DirTestJumpTable() {
    wire_available_RequestOut = 0;
    wire_manip_RequestOut = 0;
  }
  void check(std::string const & anInstance) {
    if ( wire_available_RequestOut == 0 ) { DBG_(Crit, (<< anInstance << " port " "RequestOut" " is not wired" ) ); }
  }
};

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT DirTest
