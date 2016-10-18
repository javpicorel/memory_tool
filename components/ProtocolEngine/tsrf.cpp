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

#include <vector>
#include <list>

#include <core/debug/debug.hpp>

#include "tsrf.hpp"


namespace nProtocolEngine {

tTsrf::tTsrf(std::string const & anEngineName, unsigned int aSize)
  : theEngineName(anEngineName)
  , theReseveredForWBEntry_isAvail(true)
  , theReseveredForLocalEntry_isAvail(true)
{
  DBG_(VVerb, ( << theEngineName << " initializing Tsrf"));
  DBG_Assert(aSize > 0);
  theTSRF.resize(aSize+2);

  for (size_t i = 0; i < aSize; ++i) {
    DBG_(VVerb, ( << theEngineName << " pushing entry @" << & std::hex << & (theTSRF[i]) << & std::dec << " onto free list"));
    theFreeList.push_back( & (theTSRF[i]) );
  }

  theReseveredForLocalEntry = & (theTSRF[aSize + 1]);
  theReseveredForWBEntry = & (theTSRF[aSize]);
}


bool tTsrf::isEntryAvail(eEntryRequestType aType) {
  if (! theFreeList.empty()) {
    return true;
  } else {
    switch (aType) {
      case eNormalRequest:
        return false;
      case eRequestWBEntry:
        return theReseveredForWBEntry_isAvail;
      case eRequestLocalEntry:
        return theReseveredForLocalEntry_isAvail;
    }
  }
  return false;
}

tTsrfEntry * tTsrf::allocate(eEntryRequestType aType) {
  tTsrfEntry * ret_val = 0;
  if (! theFreeList.empty()) {
     ret_val = theFreeList.front();
     theFreeList.pop_front();
  } else {
    switch (aType) {
      case eNormalRequest:
        DBG_Assert( false, ( << "A tsrf entry was requested (Normal request) but no tsrf entry is free.") );
        break;
      case eRequestWBEntry:
        DBG_Assert( theReseveredForWBEntry_isAvail, ( << "A tsrf entry was requested (WB request) but no tsrf entry is free.") );
        DBG_(VVerb, ( << theEngineName << " allocating WB reserved TSRF entry.") );
        theReseveredForWBEntry_isAvail = false;
        ret_val = theReseveredForWBEntry;
        break;
      case eRequestLocalEntry:
        DBG_Assert( theReseveredForLocalEntry_isAvail, ( << "A tsrf entry was requested (Local request) but no tsrf entry is free.") );
        DBG_(VVerb, ( << theEngineName << " allocating Local reserved TSRF entry.") );
        theReseveredForLocalEntry_isAvail = false;
        ret_val = theReseveredForLocalEntry;
        break;
    }
  }
  DBG_(VVerb, ( << theEngineName << " allocating tsrf entry " << &std::hex << ret_val << & std::dec) );
  DBG_Assert(ret_val->state() == eNoThread);
  return ret_val;
}

void tTsrf::free(tTsrfEntry * anEntry) {
  DBG_Assert(anEntry->state() == eNoThread);
  DBG_(VVerb, ( << theEngineName << " freeing tsrf entry " << &std::hex << anEntry << & std::dec) );
  if (anEntry == theReseveredForWBEntry) {
    theReseveredForWBEntry_isAvail = true;
    DBG_(VVerb, ( << theEngineName << " freeing WB reserved TSRF entry.") );
  } else if  (anEntry == theReseveredForWBEntry) {
    theReseveredForLocalEntry_isAvail = true;
    DBG_(VVerb, ( << theEngineName << " freeing Local reserved TSRF entry.") );
  } else {
    theFreeList.push_back(anEntry);
  }
}




}  // namespace nProtocolEngine
