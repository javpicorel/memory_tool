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
/*! \file PBController.hpp
 * \brief
 *
 *  This file contains the implementation of the CacheController.  Alternate
 *  or extended definitions can be provided here as well.  This component
 *  is a main Flexus entity that is created in the wiring, and provides
 *  a full cache model.
 *
 * Revision History:
 *     ssomogyi    17 Feb 03 - Initial Revision
 *     twenisch    23 Feb 03 - Integrated with CacheImpl.hpp
 */


#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>

#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>
#include <components/Common/Slices/PrefetchMessage.hpp>





namespace nPrefetchBuffer {

  enum eAction
    { kNoAction
    , kSendToFront
    , kSendToFrontAndClearMAF
    , kSendToBackSnoop
    , kSendToBackRequest
    , kBlockOnAddress
    , kBlockOnPrefetch
    , kBlockOnWatch
    , kRemoveAndWakeMAF
    };

  enum eProbeMAFResult
    { kNoMatch
    , kCancelled
    , kNotCancelled
    , kWatch
    };

  using namespace Flexus::SharedTypes;
  typedef PhysicalMemoryAddress MemoryAddress;

  struct PB {
    virtual void sendMaster( boost::intrusive_ptr<PrefetchMessage> msg, boost::intrusive_ptr<TransactionTracker> tracker ) = 0;
    virtual void sendBackRequest( boost::intrusive_ptr<MemoryMessage> msg, boost::intrusive_ptr<TransactionTracker> tracker ) = 0;
    virtual void sendBackPrefetch( boost::intrusive_ptr<MemoryMessage> msg, boost::intrusive_ptr<TransactionTracker> tracker ) = 0;
    virtual void sendBackSnoop( boost::intrusive_ptr<MemoryMessage> msg, boost::intrusive_ptr<TransactionTracker> tracker ) = 0;
    virtual void sendFront( boost::intrusive_ptr<MemoryMessage> msg, boost::intrusive_ptr<TransactionTracker> tracker ) = 0;
    virtual bool cancel( MemoryAddress const & anAddress ) = 0;
    virtual eProbeMAFResult probeMAF( MemoryAddress const & anAddress ) = 0;
    virtual ~PB() {}
  };

  struct PBController {
    static PBController * construct(PB * aPB, std::string const & aName, unsigned aNodeId, int aNumEntries, int aWatchEntries, bool anEvictClean, bool aUseStreamFetch) ;
    virtual eAction processRequestMessage( boost::intrusive_ptr<MemoryMessage> msg, boost::intrusive_ptr<TransactionTracker> tracker, bool hasMAFConflict, bool waited ) = 0;
      //Possible returns: kSendToFront, kSendToBackRequest, kBlockOnAddress
    virtual eAction processBackMessage( boost::intrusive_ptr<MemoryMessage>  msg, boost::intrusive_ptr<TransactionTracker>  tracker ) = 0;
      //Possible returns: kSendToFront, kRemoveAndWakeMAF
    virtual eAction processSnoopMessage( boost::intrusive_ptr<MemoryMessage>  msg, boost::intrusive_ptr<TransactionTracker>  tracker ) = 0;
      //Possible returns: kSendToBackSnoop, kRemoveAndWakeMAF, kNoAction
    virtual eAction processPrefetch( boost::intrusive_ptr<PrefetchMessage>  msg, boost::intrusive_ptr<TransactionTracker>  tracker, bool hasMAFConflict ) = 0;
      //Possible returns: kBlockOnPrefetch, kNoAction
    virtual ~PBController() {}
    virtual void saveState(std::string const & aDirName) const = 0;
    virtual void loadState(std::string const & aDirName) = 0;
  };


}  // end namespace nPrefetchBuffer

