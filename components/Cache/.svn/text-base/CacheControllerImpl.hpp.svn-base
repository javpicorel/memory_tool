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
/*! \file CacheControllerImpl.hpp
 * \brief
 *
 * Defines the interfaces that the CacheController and the CacheControllerImpl
 * use to talk to one another
 *
 * Revision History:
 *     twenisch    03 Sep 04 - Split implementation out to compile separately
 */

#ifndef _CACHECONTROLLERIMPL_HPP
#define _CACHECONTROLLERIMPL_HPP

#include "BaseCacheControllerImpl.hpp"

namespace nCache {




  class CacheControllerImpl : public BaseCacheControllerImpl
  {

  public:
    CacheControllerImpl ( BaseCacheController * aController,
                          CacheInitInfo       * aInit );

  protected:
    virtual Action performOperation ( MemoryMessage_p        msg,
                                      TransactionTracker_p   tracker,
                                      LookupResult         & lookup,
                                      bool                   wasHit,
                                      bool                   anyInvs,
                                      bool                   blockAddressOnMiss );

    virtual Action handleMiss ( MemoryMessage_p        msg,
                                TransactionTracker_p   tracker,
                                LookupResult         & lookup,
                                bool                   address_conflict_on_miss,
                                bool                   probeIfetchMiss );


    virtual Action handleBackMessage ( MemoryMessage_p      msg,
                                       TransactionTracker_p tracker );

    virtual Action handleSnoopMessage ( MemoryMessage_p      msg,
                                        TransactionTracker_p tracker );

    virtual Action handleIprobe ( bool                 aHit,
                                  MemoryMessage_p      fetchReq,
                                  TransactionTracker_p tracker );

    Action handleDownProbe ( MemoryMessage_p        msg,
                             TransactionTracker_p   tracker,
                             LookupResult         & lookup );

    Action handleAlloc ( MemoryMessage_p        msg,
                         TransactionTracker_p   tracker,
                         LookupResult         & lookup );

    Action handleFlush ( MemoryMessage_p        msg,
                         TransactionTracker_p   tracker,
                         LookupResult         & lookup );

    /* CMU-ONLY-BLOCK-BEGIN */
    Action handlePurge ( MemoryMessage_p        msg,
                         TransactionTracker_p   tracker,
                         LookupResult         & lookup );
    /* CMU-ONLY-BLOCK-END */

    Action handleAlloc_Miss ( MemoryMessage_p        msg,
                              TransactionTracker_p   tracker,
                              LookupResult         & lookup );

  }; // class CacheControllerImpl


}  // end namespace nCache

#endif // _CACHECONTROLLERIMPL_HPP
