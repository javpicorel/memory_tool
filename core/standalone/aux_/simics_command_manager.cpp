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

#include <string>
#include <vector>

#include <core/debug/debug.hpp>

#include <core/target.hpp>


#include <core/simics/configuration_api.hpp>
#include <core/simics/aux_/simics_command_manager.hpp>

namespace Flexus {
namespace Simics {
namespace aux_ {


SimicsCommandManager * SimicsCommandManager::get() {
  static SimicsCommandManager theManager;
  return &theManager;
}

SimicsCommandManager::SimicsCommandManager() {
}


void SimicsCommandManager::invoke( int aCommandId ) { }


void SimicsCommandManager::addCommand(std::string const & aNamespaceName, std::string const & aFrontendCommand, std::string const & aCommandDescription, Invoker anInvoker ) { }

std::string SimicsCommandManager::arg_class_to_string(int anArgClass) {
    if ( anArgClass == SimicsCommandManager::string_class ) {
      return "str_t";
    } else if ( anArgClass == SimicsCommandManager::int_class ) {
      return "int_t";
    } else if ( anArgClass == SimicsCommandManager::flag_class ) {
      return "flag_t";
    } else if ( anArgClass == SimicsCommandManager::address_class ) {
      return "addr_t";
    } else if ( anArgClass == SimicsCommandManager::file_class ) {
      return "filename_t()";
    } else {
      return "str_t";
    }
}

void SimicsCommandManager::addCommand(std::string const & aNamespaceName, std::string const & aFrontendCommand, std::string const & aCommandDescription, Invoker anInvoker, std::string const & anArg1Name,  int anArg1Class) { }

void SimicsCommandManager::addCommand(std::string const & aNamespaceName, std::string const & aFrontendCommand, std::string const & aCommandDescription, Invoker anInvoker, std::string const & anArg1Name,  int anArg1Class, std::string const & anArg2Name,  int anArg2Class ) { }

void SimicsCommandManager::addCommand(std::string const & aNamespaceName, std::string const & aFrontendCommand, std::string const & aCommandDescription, Invoker anInvoker, std::string const & anArg1Name,  int anArg1Class, std::string const & anArg2Name,  int anArg2Class, std::string const & anArg3Name,  int anArg3Class  ) { }

} //namespace aux_
} //namespace Simics
} //namespace Flexus
