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

#include <components/Common/OneWayMux.hpp>

#define FLEXUS_BEGIN_COMPONENT OneWayMux
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nOneWayMux {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

class FLEXUS_COMPONENT(OneWayMux) {
  FLEXUS_COMPONENT_IMPL( OneWayMux );

  public:
   FLEXUS_COMPONENT_CONSTRUCTOR(OneWayMux)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    {}

 public:
  // Initialization
  void initialize() {
  }

  bool isQuiesced() const {
    return true; //Mux is always quiesced
  }

  // Ports
  // From the instruction cache
  bool available( interface::InI const &,
                  index_t anIndex )
  {
    return true;
  }

  void push( interface::InI const &,
             index_t         anIndex,
             MemoryMessage & aMessage )
  {
    FLEXUS_CHANNEL( Out ) << aMessage;
  }


  // From the data cache
  bool available( interface::InD const &,
                  index_t anIndex )
  {
    return true;
  }

  void push( interface::InD const &,
             index_t         anIndex,
             MemoryMessage & aMessage )
  {
    FLEXUS_CHANNEL( Out ) << aMessage;
  }

};

} //End Namespace nOneWayMux

FLEXUS_COMPONENT_INSTANTIATOR( OneWayMux, nOneWayMux );

FLEXUS_PORT_ARRAY_WIDTH( OneWayMux, InI)      { return 1; }
FLEXUS_PORT_ARRAY_WIDTH( OneWayMux, InD)      { return 1; }

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT OneWayMux
