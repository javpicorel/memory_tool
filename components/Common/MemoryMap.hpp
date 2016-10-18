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
#ifndef FLEXUS_COMPONENTS_MEMORYMAP_HPP__INCLUDED
#define FLEXUS_COMPONENTS_MEMORYMAP_HPP__INCLUDED

#include <core/types.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>

namespace Flexus {
namespace SharedTypes {

  typedef Flexus::Core::index_t node_id_t;

  struct MemoryMap : public boost::counted_base {
    static boost::intrusive_ptr<MemoryMap> getMemoryMap();
    static boost::intrusive_ptr<MemoryMap> getMemoryMap(Flexus::Core::index_t aRequestingNode);

    enum AccessType {
        Read
      , Write
      , NumAccessTypes /* Must be last */
    };

    static std::string const & AccessType_toString(AccessType anAccessType) {
      static std::string access_type_names[] =
        { "Read "
        , "Write"
        };
        //xyzzy
      DBG_Assert((anAccessType < NumAccessTypes));
      return access_type_names[anAccessType];
    }


    virtual bool isCacheable(Flexus::SharedTypes::PhysicalMemoryAddress const &) = 0;
    virtual bool isMemory(Flexus::SharedTypes::PhysicalMemoryAddress const &) = 0;
    virtual bool isIO(Flexus::SharedTypes::PhysicalMemoryAddress const &) = 0;
    virtual node_id_t node(Flexus::SharedTypes::PhysicalMemoryAddress const &) = 0;
		virtual void loadState(std::string const &) = 0;
    virtual void recordAccess(Flexus::SharedTypes::PhysicalMemoryAddress const &, AccessType anAccessType) = 0;

  };


} //SharedTypes
} //Flexus

#endif //FLEXUS_COMPONENTS_MEMORYMAP_HPP__INCLUDED
