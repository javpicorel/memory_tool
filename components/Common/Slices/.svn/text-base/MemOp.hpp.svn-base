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

#ifndef FLEXUS_SLICES__MEMOP_HPP_INCLUDED
#define FLEXUS_SLICES__MEMOP_HPP_INCLUDED

#include <boost/function.hpp>
#include <boost/dynamic_bitset.hpp>

#include <components/Common/Slices/AbstractInstruction.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>

namespace Flexus {
namespace SharedTypes {

  enum eOperation   //Sorted by priority for requesting memory ports
    { kLoad
    , kAtomicPreload
    , kRMW
    , kCAS
    , kStorePrefetch
    , kStore
    , kInvalidate
    , kDowngrade
    , kProbe
    , kReturnReq
    , kLoadReply
    , kAtomicPreloadReply
    , kStoreReply
    , kStorePrefetchReply
    , kRMWReply
    , kCASReply
    , kDowngradeAck
    , kInvAck
    , kProbeAck
    , kReturnReply
    , kMEMBARMarker
    , kINVALID_OPERATION
    , kLastOperation
   };
  std::ostream & operator << ( std::ostream & anOstream, eOperation op);

  enum eSize
    { kByte = 1
    , kHalfWord =2
    , kWord = 4
    , kDoubleWord = 8
   };

  struct MemOp : boost::counted_base {
    eOperation theOperation;
    eSize theSize;
    VirtualMemoryAddress theVAddr;
    int theASI;
    PhysicalMemoryAddress thePAddr;
    VirtualMemoryAddress thePC;
    unsigned long long theValue;
    unsigned long long theExtendedValue;
    bool theReverseEndian;
    bool theNonCacheable;
    bool theSideEffect;
    bool theAtomic;
    bool theNAW;
    boost::intrusive_ptr< TransactionTracker > theTracker;
    MemOp( )
      : theOperation( kINVALID_OPERATION )
      , theSize( kWord )
      , theVAddr( VirtualMemoryAddress(0) )
      , theASI( 0x80 )
      , thePAddr( PhysicalMemoryAddress(0) )
      , thePC (VirtualMemoryAddress(0) )
      , theValue( 0 )
      , theExtendedValue ( 0 )
      , theReverseEndian(false)
      , theNonCacheable(false)
      , theSideEffect(false)
      , theAtomic(false)
      , theNAW(false)
      {}
    MemOp( MemOp const & anOther)
     : theOperation( anOther.theOperation )
     , theSize( anOther.theSize)
     , theVAddr( anOther.theVAddr)
     , theASI( anOther.theASI )
     , thePAddr( anOther.thePAddr)
     , thePC( anOther.thePC)
     , theValue( anOther.theValue)
     , theExtendedValue( anOther.theExtendedValue)
     , theReverseEndian(anOther.theReverseEndian)
     , theNonCacheable(anOther.theNonCacheable)
     , theSideEffect(anOther.theSideEffect)
     , theAtomic(anOther.theAtomic)
     , theNAW(anOther.theNAW)
     , theTracker( anOther.theTracker )
      {}

  };

  std::ostream & operator << ( std::ostream & anOstream, MemOp const & aMemOp);


} //SharedTypes
} //Flexus

#endif //FLEXUS_SLICES__MEMOP_HPP_INCLUDED

