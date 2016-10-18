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
/*
  V9 Memory Op
*/

#include <core/types.hpp>
#include <core/flexus.hpp>

#include <components/Common/Slices/ArchitecturalInstruction.hpp>
#include "StoreBuffer.hpp"

#include <core/simics/mai_api.hpp>

namespace Flexus {
namespace Simics {
namespace API {
extern "C" {
#include FLEXUS_SIMICS_API_HEADER(types)
#define restrict
#include FLEXUS_SIMICS_API_HEADER(memory)
#undef restrict

#include FLEXUS_SIMICS_API_ARCH_HEADER

#include FLEXUS_SIMICS_API_HEADER(configuration)
#include FLEXUS_SIMICS_API_HEADER(processor)
#include FLEXUS_SIMICS_API_HEADER(front)
#include FLEXUS_SIMICS_API_HEADER(event)
#undef printf
#include FLEXUS_SIMICS_API_HEADER(callbacks)
#include FLEXUS_SIMICS_API_HEADER(breakpoints)
} //extern "C"

} //namespace API
} //namespace Simics
} //namespace Flexus

#include "DebugTrace.hpp"
#include "TraceConsumer.hpp"

using namespace Flexus::Simics::API;

namespace Flexus {
namespace SharedTypes {


    void ArchitecturalInstruction::perform() {

      thePerformed = true;

      if (!isShadow() && !isTrace()) {

        DBG_Assert(! ( isStore() && !isSync() && (theStoreBuffer == 0) && FLEXUS_TARGET_IS(v9) ) );

        if (theStoreBuffer) {

          PhysicalMemoryAddress aligned_addr(thePhysicalAddress & ~7LL);
          DBG_(VVerb, Addr( thePhysicalAddress ) ( << "Performing Store @" << &std::hex << thePhysicalAddress << " aligned: " << aligned_addr << &std::dec << " value: " << theData) );

          //Perform the store
            //The correct value is in the store buffer
            switch( theSize ) {
              case 1:
                {
                    SIM_write_phys_memory(SIM_current_processor(), thePhysicalAddress, theData.getByte(thePhysicalAddress & 7), theSize);
                    DBG_(VVerb, ( << "    Wrote: " << &std::hex <<(unsigned int) theData.getByte(thePhysicalAddress & 7) << &std::dec << " to simics memory @" << &std::hex  << thePhysicalAddress << &std::dec ) );
                }
                break;
              case 2:
                {
                    SIM_write_phys_memory(SIM_current_processor(), thePhysicalAddress, theData.getHalfWord(thePhysicalAddress & 7), theSize);
                    DBG_(VVerb, ( << "    Wrote: " << &std::hex << theData.getHalfWord(thePhysicalAddress & 7) << &std::dec << " to simics memory @" << &std::hex  << thePhysicalAddress << &std::dec ) );

                }
                break;
              case 4:
                {
                    SIM_write_phys_memory(SIM_current_processor(), thePhysicalAddress, theData.getWord(thePhysicalAddress & 7), theSize);
                    DBG_(VVerb, ( << "    Wrote: " << &std::hex << theData.getWord(thePhysicalAddress & 7) << &std::dec << " to simics memory @" << &std::hex  << thePhysicalAddress << &std::dec ) );
                }
                break;
              case 8:
                {
                    SIM_write_phys_memory(SIM_current_processor(), thePhysicalAddress, theData.getDoubleWord(thePhysicalAddress & 7), theSize);
                    DBG_(VVerb, ( << "    Wrote: " << &std::hex << theData.getDoubleWord(thePhysicalAddress & 7) << &std::dec << " to simics memory @" << &std::hex  << thePhysicalAddress << &std::dec ) );
                }
                break;
              default:
                DBG_Assert( false, ( << "Store has invalid size: " << theSize) );
            }

          StoreBuffer::iterator iter = theStoreBuffer->find(aligned_addr);
          DBG_Assert(iter != theStoreBuffer->end());

          //Decrement the outstanding store count
          if (--(iter->second) == 0) {
            DBG_(VVerb, ( << "  Last store to addr.  Freeing SB entry." ) );

            //Remove the sb_entry if the count reaches zero
            theStoreBuffer->erase(iter);
          }

        } //if (theStoreBuffer)
      } //if (!isShadow() && !isTrace())

      if (isTrace() && !isShadow() && !isNOP()) {
        DBG_(Crit, ( << "Transaction completed in "
		     << Flexus::Core::theFlexus->cycleCount() - theStartTime
		     << " cycles"));
      }

    }

  void ArchitecturalInstruction::release() {
    DBG_Assert( (theReleased == false) );
    theReleased = true;
    if (! isShadow() && theConsumer) {
      theConsumer->releaseInstruction(*this);
    }
  }

} //namespace SharedTypes
} //namespace Flexus

