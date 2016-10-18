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

#include <fstream>

#include "common.hpp"
#include "event.hpp"

#define __STDC_CONSTANT_MACROS
#include <boost/date_time/posix_time/posix_time.hpp>


bool operator < (tEvent const & aLeft, tEvent const & aRight) {
  if (aLeft.theTime ==  aRight.theTime) {
    //Same Type
    if (aLeft.theObject->type() == aRight.theObject->type()) {
      //For the same type, order by node
      return aLeft.theObject->theNode > aRight.theObject->theNode;
    }

    //Different Types
    switch (aLeft.theObject->type()) {
      case eAccess:
      case eEviction:
      case eConsumption:
        //Always comes after the other two
        return true;
        break;
      case eProduction:
        if (aRight.theObject->type() == eUpgrade) {
          //Second if node is the same
          if (aLeft.theObject->theNode == aRight.theObject->theNode) {
            return true;
          } else {
            return aLeft.theObject->theNode > aRight.theObject->theNode;
          }
        } else {
          //Always precedes Consumption
          return false;
        }
        break;
      case eUpgrade:
        if (aRight.theObject->type() == eProduction) {
          //First if node is the same
          if (aLeft.theObject->theNode == aRight.theObject->theNode) {
            return false;
          } else {
            return aLeft.theObject->theNode > aRight.theObject->theNode;
          }
        } else {
          //Always precedes Consumption
          return false;
        }
        break;
    }
  }
  return aLeft.theTime > aRight.theTime; //Lower time means higher priority.
}

EventQueue::EventQueue(unsigned long long aStop)
  : theStop(aStop)
  , theTerminateFlag(false)
  {}

void EventQueue::fill(boost::optional<tEvent> const & anEvent) {
    if (anEvent) {
      theEventQueue.push( *anEvent );
    }
}

void EventQueue::go(boost::function<void ()> aPeriodicFunction) {
  tTime last_timestamp = 0;
  tTime last_stats = 0;
  long long now = 0;
  while (! theEventQueue.empty()) {
    if (theEventQueue.top().theTime  - last_timestamp > 1000000) {
      last_timestamp = theEventQueue.top().theTime;
      boost::posix_time::ptime now(boost::posix_time::second_clock::local_time());
      DBG_(Dev, ( << "Cycle: " << last_timestamp << " Timestamp: " << boost::posix_time::to_simple_string(now)));

      if ( last_timestamp - last_stats > 100000000) {
         last_stats = last_timestamp;
         DBG_(Dev, Cat(Stats) ( << "Dumping Intermediate stats up to timestamp: " << last_stats ));

         remove("stats_db.intermediate.out");
         rename("stats_db.out", "stats_db.intermediate.out");
         std::ofstream out("stats_db.out", std::ios::binary);
         getStatManager()->save(out);
         out.close();
         remove("stats_db.intermediate.out");

         aPeriodicFunction();


      }
    }

    if (theTerminateFlag || (theStop > 0 && theEventQueue.top().theTime > theStop)) {
      DBG_(Dev, ( << "Reached end of experiment @ timestamp: " << theEventQueue.top().theTime ));
      break;
    }

    tEventProcessor * object = theEventQueue.top().theObject;
    getStatManager()->tick(theEventQueue.top().theTime - now );
    now = theEventQueue.top().theTime;
    theEventQueue.pop();
    object->process();

    boost::optional<tEvent> event = object->nextEvent();
    if (event) {
      theEventQueue.push( *event  );
    }

  }
}


