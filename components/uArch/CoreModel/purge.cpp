// DO-NOT-REMOVE begin-copyright-block
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2006 by Eric Chung, Brian Gold, Nikos Hardavellas, Jangwoo Kim,
// Ippokratis Pandis, Minglong Shao, Jared Smolens, Stephen Somogyi,
// Tom Wenisch, Anastassia Ailamaki, Babak Falsafi and James C. Hoe for
// the SimFlex Project, Computer Architecture Lab at Carnegie Mellon,
// Carnegie Mellon University.
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


#include "coreModelImpl.hpp"
#include <core/debug/severity.hpp>

#include <components/Common/Slices/FillLevel.hpp>
#include <components/Common/TraceTracker.hpp>

#include <iostream>
#include <core/flexus.hpp>

  #define DBG_DeclareCategories uArchCat
  #define DBG_SetDefaultOps AddCat(uArchCat)
  #include DBG_Control()

//nikos
namespace nuArch {

  bool CoreImpl::isPurgePending( void ) {
    return (thePurgeInProgress.thePurgeDelayCycles != 0);
  }

  void CoreImpl::decrementPurgeDelayCounter( void )
  {
	if (thePurgeInProgress.thePurgeDelayCycles > 0) {
      --thePurgeInProgress.thePurgeDelayCycles;	
      if (thePurgeInProgress.thePurgeDelayCycles == 0) {
        DBG_( VVerb,
              Addr(thePurgeInProgress.thePurgeAddr)
              ( << theName
                << " satisfied purge for address 0x" << thePurgeInProgress.thePurgeAddr
            ));
        thePurgeInProgress.thePurgeAddr = PhysicalMemoryAddress( 0 );
      }
	}	
  }

  void CoreImpl::initializePurge( boost::intrusive_ptr< MemOp > aPurgeOp )
  {
    if (aPurgeOp->theOperation != kFPurge) {
      DBG_Assert (aPurgeOp->theOperation == kIPurge || aPurgeOp->theOperation == kDPurge);
      PhysicalMemoryAddress purgeAddr = aPurgeOp->thePAddr;
      DBG_( VVerb,
            Addr(purgeAddr)
            ( << theName
              << " initialize purge address 0x" << std::hex << purgeAddr
              << " op: " << *aPurgeOp
          ));

      theTraceTracker.invalidation(theNode, eCore, purgeAddr);
      loseWritePermission( eLosePerm_Invalidate, purgeAddr ); 
      invalidate( purgeAddr );
    }
    DBG_( VVerb, ( << "Sending Purge to cache hierarchy: " << *aPurgeOp) );
    thePurgePorts.push_back( aPurgeOp );
  }

  void CoreImpl::ackPurge( boost::intrusive_ptr< MemOp > aPurgeAckOp )
  {
    PhysicalMemoryAddress purgeAckAddr = aPurgeAckOp->thePAddr;
    DBG_( VVerb,
          Addr(purgeAckAddr)
          ( << theName
            << " purge ack address 0x" << std::hex << purgeAckAddr
            << " op: " << *aPurgeAckOp
        ));

  }

  void CoreImpl::finalizePurge( boost::intrusive_ptr< MemOp > aPurgeOp ) {
    PhysicalMemoryAddress purgeAddr = aPurgeOp->thePAddr;
    DBG_( VVerb,
          Addr(purgeAddr)
          ( << theName
            << " finalize purge address 0x" << std::hex << purgeAddr
            << " op: " << *aPurgeOp
        ));
  }

} //nuArch
