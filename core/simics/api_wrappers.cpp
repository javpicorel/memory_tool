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
#include <core/simics/api_wrappers.hpp>


namespace Flexus {
namespace Simics {
namespace API {
extern "C" {
  #include FLEXUS_SIMICS_API_HEADER(control)
  #include FLEXUS_SIMICS_API_HEADER(processor)
}
} //namespace API

namespace APIFwd {

Simics::API::conf_class_t * SIM_register_class(const char *name, Simics::API::class_data_t *class_data) {
  return Simics::API::SIM_register_class(name,class_data);
}

int SIM_register_attribute(
    Simics::API::conf_class_t *class_struct
  , const char *attr_name
  , Simics::API::get_attr_t get_attr
  , Simics::API::lang_void *get_attr_data
  , Simics::API::set_attr_t set_attr
  , Simics::API::lang_void *set_attr_data
  , Simics::API::attr_attr_t attr
  , const char *doc
  ) {
    return Simics::API::SIM_register_attribute(class_struct, attr_name, get_attr, get_attr_data, set_attr, set_attr_data, attr, doc);
}

void SIM_object_constructor(Simics::API::conf_object_t *conf_obj, Simics::API::parse_object_t *parse_obj) {
  Simics::API::SIM_object_constructor(conf_obj, parse_obj);
}

Simics::API::conf_object_t *SIM_new_object(Simics::API::conf_class_t *conf_class, const char *instance_name) {
  return Simics::API::SIM_new_object(conf_class, instance_name);
}

void SIM_break_simulation(const char * aMessage) {
  return Simics::API::SIM_break_simulation(aMessage);
}

void SIM_write_configuration_to_file(const char * aFilename) {
  Simics::API::SIM_write_configuration_to_file(aFilename);
}

Simics::API::conf_object_t * SIM_get_processor(int aCPUNum) {
  #if SIM_VERSION > 1300 
    return API::SIM_get_processor(aCPUNum);  
  #else
    return API::SIM_proc_no_2_ptr(aCPUNum);  
  #endif 
}

int SIM_get_processor_number(Simics::API::conf_object_t *  aProcessor) {
  #if SIM_VERSION > 1300 
    return API::SIM_get_processor_number(aProcessor);  
  #else
    return API::SIM_get_proc_no(aProcessor);  
  #endif   
}


void SIM_flush_all_caches() {
  #if SIM_VERSION > 1300 
    return API::SIM_flush_all_caches();  
  #else
    return API::SIM_dump_caches();  
  #endif     
}

} //namespace APIFwd
} //End Namespace Simics
} //namespace Flexus


