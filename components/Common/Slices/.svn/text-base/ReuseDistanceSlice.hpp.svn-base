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


#ifndef FLEXUS_SLICES__REUSEDISTANCESLICE_HPP_INCLUDED
#define FLEXUS_SLICES__REUSEDISTANCESLICE_HPP_INCLUDED

#ifdef FLEXUS_ReuseDistanceSlice_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::ReuseDistanceSlice data type"
#endif
#define FLEXUS_ReuseDistanceSlice_TYPE_PROVIDED

#include <core/boost_extensions/intrusive_ptr.hpp>

#include <boost/optional.hpp>
#include <boost/none.hpp>

#include <core/types.hpp>
#include <core/exception.hpp>

#include <components/Common/Slices/MemoryMessage.hpp>

namespace Flexus {
namespace SharedTypes {

  using namespace Flexus::Core;
  using boost::intrusive_ptr;

  static MemoryMessage theDummyMemMsg(MemoryMessage::LoadReq);

  struct ReuseDistanceSlice : public boost::counted_base/*, public FastAlloc */
  {
    typedef PhysicalMemoryAddress MemoryAddress;

    // enumerated message type
    enum ReuseDistanceSliceType {
      ProcessMemMsg,
      GetMeanReuseDist_Data
    };

    explicit ReuseDistanceSlice(ReuseDistanceSliceType aType)
      : theType(aType)
      , theAddress(0)
      , theMemoryMessage(theDummyMemMsg)
      , theReuseDist(0LL)
    {}
    explicit ReuseDistanceSlice(ReuseDistanceSliceType aType, MemoryAddress anAddress)
      : theType(aType)
      , theAddress(anAddress)
      , theMemoryMessage(theDummyMemMsg)
      , theReuseDist(0LL)
    {}
    explicit ReuseDistanceSlice(ReuseDistanceSliceType aType, MemoryAddress anAddress, MemoryMessage & aMemMsg)
      : theType(aType)
      , theAddress(anAddress)
      , theMemoryMessage(aMemMsg)
      , theReuseDist(0LL)
    {}

    static intrusive_ptr<ReuseDistanceSlice> newProcessMemMsg(MemoryMessage & aMemMsg) {
      intrusive_ptr<ReuseDistanceSlice> slice = new ReuseDistanceSlice(ProcessMemMsg, aMemMsg.address(), aMemMsg);
      return slice;
    }
    static intrusive_ptr<ReuseDistanceSlice> newMeanReuseDistData(MemoryAddress anAddress) {
      intrusive_ptr<ReuseDistanceSlice> slice = new ReuseDistanceSlice(GetMeanReuseDist_Data, anAddress);
      return slice;
    }

    const ReuseDistanceSliceType type() const {
      return theType;
    }
    const MemoryAddress address() const {
      return theAddress;
    }
    const long long meanReuseDist() const {
      return theReuseDist;
    }

    ReuseDistanceSliceType & type() {
      return theType;
    }
    MemoryAddress & address() {
      return theAddress;
    }
    MemoryMessage & memMsg() {
      return theMemoryMessage;
    }
    long long & meanReuseDist() {
      return theReuseDist;
    }

  private:
    ReuseDistanceSliceType theType;
    MemoryAddress theAddress;
    MemoryMessage & theMemoryMessage;
    long long theReuseDist;
  };

  std::ostream & operator << (std::ostream & s, ReuseDistanceSlice const & aReuseDistSlice);

} //End SharedTypes
} //End Flexus

#endif //FLEXUS_SLICES__REUSEDISTANCESLICE_HPP_INCLUDED
