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

#include <components/Directory/DirMux2.hpp>

#define FLEXUS_BEGIN_COMPONENT DirMux2
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

  #define DBG_DefineCategories DirMux
  #define DBG_SetDefaultOps AddCat(DirMux)
  #include DBG_Control()

#include <components/Common/Slices/Mux.hpp>


namespace nDirMux2 {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;
using boost::intrusive_ptr;


class FLEXUS_COMPONENT(DirMux2) {
  FLEXUS_COMPONENT_IMPL(DirMux2);

 public:
   FLEXUS_COMPONENT_CONSTRUCTOR(DirMux2)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    {}

  bool isQuiesced() const {
    return true; //DirMux is always quiesced.
  }

  // Initialization
  void initialize() {
  }

  // Ports

  bool available(interface::TopIn1 const &) {
      return FLEXUS_CHANNEL(BottomOut).available();
  }
  void push(interface::TopIn1 const &, DirectoryTransport & aDirTransport) {
      // add the arbitration slice to the transport so it can be
      // directed correctly on the way back
      intrusive_ptr<Mux> arb( new Mux(1) );
      aDirTransport.set(DirMux2ArbTag, arb);
      FLEXUS_CHANNEL( BottomOut) << aDirTransport;
  }

  bool available(interface::TopIn2 const &) {
      return FLEXUS_CHANNEL(BottomOut).available();
  }
  void push(interface::TopIn2 const &, DirectoryTransport & aDirTransport) {
      // add the arbitration slice to the transport so it can be
      // directed correctly on the way back
      intrusive_ptr<Mux> arb( new Mux(2) );
      aDirTransport.set(DirMux2ArbTag, arb);
      FLEXUS_CHANNEL( BottomOut) << aDirTransport;
  }

  bool available(interface::BottomIn const &) {
      return FLEXUS_CHANNEL(TopOut1).available() && FLEXUS_CHANNEL(TopOut2).available();
  }
  void push(interface::BottomIn const &, DirectoryTransport & aDirTransport) {
      switch(aDirTransport[DirMux2ArbTag]->source) {
      case 1:
        FLEXUS_CHANNEL( TopOut1) << aDirTransport;
        break;
      case 2:
        FLEXUS_CHANNEL( TopOut2) << aDirTransport;
        break;
      default:
        DBG_Assert(false, (<< "Invalid source"));
      }
  }

};

} //End Namespace nDirMux2

FLEXUS_COMPONENT_INSTANTIATOR( DirMux2, nDirMux2 );


#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT DirMux2

  #define DBG_Reset
  #include DBG_Control()
