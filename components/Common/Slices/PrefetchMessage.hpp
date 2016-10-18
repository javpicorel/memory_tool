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

#ifndef FLEXUS_SLICES__PREFETCHMESSAGE_HPP_INCLUDED
#define FLEXUS_SLICES__PREFETCHMESSAGE_HPP_INCLUDED

#include <core/boost_extensions/intrusive_ptr.hpp>

#include <core/types.hpp>

namespace Flexus {
namespace SharedTypes {

  struct PrefetchMessage : public boost::counted_base {
    typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

    // enumerated message type
    enum PrefetchMessageType {
      // Requests made by Prefetch controller
      PrefetchReq,
      // Tells the prefetch buffer to perform a prefetch.
      DiscardReq,
      // Tells the prefetch buffer to discard a line, it if is present.
      WatchReq,
      // Tells the prefetch buffer to monitor a cache line, and report if
      // any requests arrive for it.

      // Responses/Notifications from Prefetch Buffer
      PrefetchComplete,
      // Indicates that a prefetch operation has been completed.
      PrefetchRedundant,
      // Indicates that a prefetch operation was rejected by the memory
      // heirarchy.

      LineHit,
      // This indicates that a hit has occurred on a prefetched line
      LineHit_Partial,
      // This indicates that a hit has occurred on a prefetched line
      LineReplaced,
      // This indicates that a line was removed by replacement
      LineRemoved,
      // This indicates that a line was removed for some reason other
      // than replacement.

      WatchPresent,
      // This indicates that a watched line was found when the cache heirarchy
      // was probed
      WatchRedundant,
      // This indicates that a watched line was found in the prefetch buffer
      WatchRequested,
      // This indicates that a watched line was found when the cache heirarchy
      // was probed
      WatchRemoved,
      // This indicates that a watched line is no longer tracked because of a
      // snoop or a write to the line
      WatchReplaced
      // This indicates that a watched line was dropped from the watch list
      // because of a replacement
    };

    const PrefetchMessageType type() const {
      return theType;
    }
    const MemoryAddress address() const {
      return theAddress;
    }

    PrefetchMessageType & type() {
      return theType;
    }
    MemoryAddress & address() {
      return theAddress;
    }

    const long streamID() const {
      return theStreamID;
    }
    long & streamID() {
      return theStreamID;
    }

    explicit PrefetchMessage(PrefetchMessageType aType, MemoryAddress anAddress, long aStreamId = 0)
      : theType(aType)
      , theAddress(anAddress)
      , theStreamID(aStreamId)
    {}

    friend std::ostream & operator << (std::ostream & s, PrefetchMessage const & aMsg);

  private:
    PrefetchMessageType theType;
    MemoryAddress theAddress;
    long theStreamID;
  };


} //SharedTypes
} //Scaffold

#endif  // FLEXUS_SLICES__PREFETCHMESSAGE_HPP_INCLUDED
