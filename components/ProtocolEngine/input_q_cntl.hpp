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

#ifndef _INPUT_Q_CNTL_H_
#define _INPUT_Q_CNTL_H_

#include <list>
#include <core/boost_extensions/intrusive_ptr.hpp>

#include "ProtSharedTypes.hpp"
#include "util.hpp"
#include "thread.hpp"
#include <core/stats.hpp>


namespace nProtocolEngine {

namespace Stat = Flexus::Stat;

struct tSrvcProvider;
struct tThreadScheduler;

class tInputQueueController {

 private:
  std::string theEngineName;
  tSrvcProvider & theSrvcProvider;
  tThreadScheduler & theThreadScheduler;

  struct tInputQueue {
    typedef std::list< boost::intrusive_ptr<tPacket> >::iterator iterator;
    std::list< boost::intrusive_ptr<tPacket> > theQueue;
  };

  tInputQueue theQueues[kMaxVC + 1];

  Stat::StatAverage statAvgInputQWaitTime;
  Stat::StatLog2Histogram statInputQWaitTimeDistribution;

 public:
  bool isQuiesced() const {
    bool quiesced = true;
    for (int i = 0; i < kMaxVC + 1 && quiesced; ++i) {
      quiesced = theQueues[i].theQueue.empty();
    }
    return quiesced;
  }

  tInputQueueController(std::string const & anEngineName, tSrvcProvider & aSrvcProvider, tThreadScheduler & aThreadScheduler);

  void dequeue(const tVC VC, tThread aThread = tThread()); //Remove a delivered packet from the head of the queue - it has been processed
  bool available(tVC aVC) const ;    //Returns true if there is an undelivered packet at the head of the queue
  void refuse(tVC aVC, boost::intrusive_ptr<tPacket> packet);   // Reject a packet from a thread and return it to the head of the input queue.
  void refuseAndRotate(tVC aVC, boost::intrusive_ptr<tPacket> packet);   // Reject a packet from a thread and return it to the tail of the input queue.
  void enqueue(boost::intrusive_ptr<tPacket> packet);   // enqueue (receive a message from the network)

  boost::intrusive_ptr<tPacket> getQueueHead(const tVC VC) {
    DBG_Assert( VC >= 0 && VC <= kMaxVC );
    DBG_Assert( ! theQueues[VC].theQueue.empty());
    DBG_Assert( theQueues[VC].theQueue.front());
    return theQueues[VC].theQueue.front();
  }

  unsigned int size(const tVC VC) const {
    DBG_Assert( VC >= 0 && VC <= kMaxVC );
    //The +1 is used to reserve one queue entry for packet rotation when there is a refuse.
    return theQueues[VC].theQueue.size() + 1;
  }

  private:
    void handleInvalidate(boost::intrusive_ptr<tPacket>);
    void handleDowngrade(boost::intrusive_ptr<tPacket>);

};


}  // namespace nProtocolEngine

#endif // _INPUT_Q_CNTL_H_
