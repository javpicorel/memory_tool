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

#include <core/target.hpp>
#include <core/simics/hap_api.hpp>
#include <core/simics/event_api.hpp>


namespace Flexus {
namespace Simics {

const char * HAPs::Core_Initial_Configuration::hap_name = "Core_Initial_Configuration";
const char * HAPs::Core_Asynchronous_Trap::hap_name = "Core_Asynchronous_Trap";
const char * HAPs::Core_Exception_Return::hap_name = "Core_Exception_Return";
const char * HAPs::Core_Continuation::hap_name = "Core_Continuation";
const char * HAPs::Core_Simulation_Stopped::hap_name = "Core_Simulation_Stopped";
const char * HAPs::Core_Magic_Instruction::hap_name = "Core_Magic_Instruction";
const char * HAPs::Core_Periodic_Event::hap_name = "Core_Periodic_Event";
const char * HAPs::Ethernet_Network_Frame::hap_name = "Ethernet_Network_Frame";
const char * HAPs::Ethernet_Frame::hap_name = "Ethernet_Frame";
const char * HAPs::Xterm_Break_String::hap_name = "Xterm_Break_String";
const char * HAPs::Gfx_Break_String::hap_name = "Gfx_Break_String";

InitialConfigHapHandler * InitialConfigHapHandler::theStaticInitialConfigHapHandler = 0;

namespace Detail {
	EventImpl::timing_seconds_tag EventImpl::timing_seconds;
	EventImpl::timing_cycles_tag EventImpl::timing_cycles;
	EventImpl::timing_steps_tag EventImpl::timing_steps;
	EventImpl::timing_stacked_tag EventImpl::timing_stacked;
}


}  //End Namespace Simics
} //namespace Flexus


