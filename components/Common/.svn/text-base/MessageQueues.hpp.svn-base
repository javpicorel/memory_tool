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

#ifndef FLEXUS_COMMON_MESSAGE_QUEUES_HPP_INCLUDED
#define FLEXUS_COMMON_MESSAGE_QUEUES_HPP_INCLUDED

#include <list>

#include <core/stats.hpp>

  #define DBG_DeclareCategories CommonQueues
  #define DBG_SetDefaultOps AddCat(CommonQueues)
  #include DBG_Control()


namespace nMessageQueues {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

template <class Transport>
class MessageQueue {
  std::list< std::pair<Transport, long long> > theQueue;
  unsigned int theSize;
  unsigned int theCurrentUsage;
  unsigned int theCurrentReserve;

public:
  MessageQueue( )
   : theSize(1)
   , theCurrentUsage(0)
   , theCurrentReserve(0)
   {}
  MessageQueue( unsigned int aSize )
    : theSize(aSize)
    , theCurrentUsage(0)
    , theCurrentReserve(0)
    {}

  void setSize( unsigned int aSize ) {
    theSize = aSize;
  }

  void enqueue(Transport const & aMessage) {
    theQueue.push_back( std::make_pair(aMessage, Flexus::Core::theFlexus->cycleCount()) );
    ++theCurrentUsage;
    ++theCurrentReserve;
    DBG_Assert( theCurrentReserve >= theCurrentUsage );
    DBG_Assert( theCurrentReserve <= theSize, ( << theCurrentReserve << "/" << theCurrentUsage << "/" << theSize ) );
  }

  Transport dequeue() {
    DBG_Assert( ! theQueue.empty() );
    Transport  ret_val(theQueue.front().first);
    theQueue.pop_front();
    --theCurrentUsage;
    --theCurrentReserve;
    DBG_Assert( theCurrentReserve >= theCurrentUsage );
    DBG_Assert( theCurrentUsage >= 0 );
    return ret_val;
  }

  const Transport & peek() {
    return theQueue.front().first;
  }

  bool hasSpace(unsigned int msgs) const {
    return theCurrentReserve + msgs <= theSize;
  }

  long long headTimestamp() const {
    DBG_Assert( ! theQueue.empty() );
    return theQueue.front().second;
  }

  bool empty() const {
    return theQueue.empty();
  }
  bool full(unsigned int anAdditionalReserve = 0) const {
    return theCurrentReserve + anAdditionalReserve >= theSize;
  }

  void reserve() {
    ++theCurrentReserve;
    DBG_Assert( theCurrentReserve >= theCurrentUsage );
    DBG_Assert( theCurrentReserve <= theSize );
  }

  void unreserve() {
    --theCurrentReserve;
    DBG_Assert( theCurrentReserve >= theCurrentUsage );
    DBG_Assert( theCurrentReserve >= 0);
  }

};  // class MessageQueue

typedef unsigned long long CycleTime;

template< class Item >
class DelayFifo {
  typedef std::pair<Item,CycleTime> DelayElement;
  std::list<DelayElement> theQueue;
  unsigned int theSize;
  unsigned int theCurrentSize;

public:
  DelayFifo( )
   : theSize(1)
   , theCurrentSize(0)
   {}
  DelayFifo( unsigned int aSize )
    : theSize(aSize)
    , theCurrentSize(0)
    {}

  void setSize( unsigned int aSize ) {
    theSize = aSize;
  }

  void enqueue(Item anItem, unsigned int delay) {
    CycleTime ready = Flexus::Core::theFlexus->cycleCount() + delay;
    theQueue.push_back( std::make_pair(anItem,ready) );
    ++theCurrentSize;
  }

  bool ready() {
    if(theCurrentSize == 0) {
      return false;
    }
    return( Flexus::Core::theFlexus->cycleCount() >= theQueue.front().second );
  }

  Item dequeue() {
    // remove and remember the head of the queue
    Item ret_val(theQueue.front().first);
    theQueue.pop_front();
    --theCurrentSize;
    return ret_val;
  }

  Item & peek() {
    // remove and remember the head of the queue
    return theQueue.front().first;
  }

  bool empty() const {
    return theCurrentSize == 0;
  }
  bool full() const {
    return theCurrentSize >= theSize;
  }
  unsigned int size() const {
    return theCurrentSize;
  }

};  // class DelayFifo

template< class Item >
class PipelineFifo {
  typedef std::pair<Item,unsigned long long> PipelineElement;
  std::list<PipelineElement> theQueue;
  std::list<unsigned long long> theServerReadyTimes;
  unsigned int theCurrentSize;
  long theIssueLatency;
  long theLatency;

  unsigned long long theLastArrival;
  boost::intrusive_ptr<Stat::StatLog2Histogram> theInterArrival;


public:
  PipelineFifo( std::string aName, 
                unsigned int aNumPipelines, 
                long anIssueLatency, 
                long aLatency, 
                boost::intrusive_ptr<Stat::StatLog2Histogram> anInterArrival = NULL )
   : theCurrentSize(0)
   , theIssueLatency(anIssueLatency)
   , theLatency(aLatency)
   , theLastArrival(0)
   , theInterArrival(anInterArrival)
  {
    DBG_Assert( aNumPipelines > 0);
    DBG_Assert( theIssueLatency >= 1);
    for (unsigned int i = 0; i < aNumPipelines; ++i) {
      theServerReadyTimes.push_back( 0 );
    }

    if ( anInterArrival == NULL ) { 
      theInterArrival = new Stat::StatLog2Histogram(aName + "-InterArrivalTimes");
    }
  }

  // Enqueue an item for one or more non-overlapping accesses to the same resource
  void enqueue(Item anItem, const unsigned int aRepeatCount = 1) {
    unsigned long long curr = Flexus::Core::theFlexus->cycleCount();
    DBG_Assert( theServerReadyTimes.front() <= curr);
    for ( unsigned int i = 0; i < aRepeatCount; i++ ) {
      theServerReadyTimes.pop_front();
      theServerReadyTimes.push_back(curr + theIssueLatency * (i+1) );
    }
    unsigned long long complete = curr + theLatency*aRepeatCount;
    theQueue.push_back( std::make_pair(anItem,complete) );
    ++theCurrentSize;
    if ( theInterArrival ) 
      *theInterArrival << (curr - theLastArrival);
    theLastArrival = curr;
  }

  bool ready() {
    if(theCurrentSize == 0) {
      return false;
    }
    return( Flexus::Core::theFlexus->cycleCount() >= theQueue.front().second );
  }

  Item dequeue() {
    --theCurrentSize;
    DBG_Assert( theCurrentSize >= 0 );
    // remove and remember the head of the queue
    Item ret_val(theQueue.front().first);
    theQueue.pop_front();
    return ret_val;
  }

  Item peek() {
    return theQueue.front().first;
  }

  void stall() {
    std::list<unsigned long long>::iterator iter = theServerReadyTimes.begin();
    std::list<unsigned long long>::iterator end = theServerReadyTimes.end();
    while (iter != end) {
      ++(*iter);
      ++iter;
    }
  }
  bool serverAvail() const {
    return theServerReadyTimes.front() <= Flexus::Core::theFlexus->cycleCount();
  }
  bool empty() const {
    return theCurrentSize == 0;
  }
  unsigned int size() const {
    return theCurrentSize;
  }

};  // class PipelineFifo

} //End Namespace nMessageQueues


  #define DBG_Reset
  #include DBG_Control()

#endif //FLEXUS_COMMON_MESSAGE_QUEUES_HPP_INCLUDED

