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


#ifndef _PROTOCOL_ENGINE_H_
#define _PROTOCOL_ENGINE_H_

#include "ProtSharedTypes.hpp"
#include "tsrf.hpp"
#include "input_q_cntl.hpp"
#include "thread_scheduler.hpp"

class tSrvcProvider;

namespace nProtocolEngine {


struct tProtocolEngineBase {
  virtual std::string const & engineName() = 0;
  virtual void enqueue(boost::intrusive_ptr<tPacket> packet) = 0;
  virtual unsigned int queueSize(const tVC aVC) const = 0;
  virtual ~tProtocolEngineBase () {}
};

template <class ExecEngine>
class tProtocolEngine : public tProtocolEngineBase {

 private:
  std::string theEngineName;                          // name of the protocol engine (for debug)

  //The parts of the protocol engine
    tTsrf                         theTsrf;            // Transaction State Register File
    tInputQueueController         theInputQCntl;      // input queue
    tThreadScheduler              theThreadScheduler; // thread scheduler
    ExecEngine                    theExecEngine;      // execution engine (overloaded to home or remote)

    Stat::StatCounter statBusyCycles;
    Stat::StatCounter statIdleCycles;

 public:
  // constructor
  tProtocolEngine(std::string const & name, tSrvcProvider & srvcProv, const unsigned aTSRFSize)
   : theEngineName(name)
   , theTsrf(name, aTSRFSize)
   , theInputQCntl(name, srvcProv, theThreadScheduler)
   , theThreadScheduler(name, theTsrf, theInputQCntl, srvcProv, theExecEngine)
   , theExecEngine(name, srvcProv, theInputQCntl, theThreadScheduler)
   , statBusyCycles(name + "-BusyCycles")
   , statIdleCycles(name + "-IdleCycles")
   { }

  // destructor
  ~tProtocolEngine() {}

  bool isQuiesced() const {
    return  theTsrf.isQuiesced()
      &&    theInputQCntl.isQuiesced()
      &&    theThreadScheduler.isQuiesced()
      ;
  }

  // initialization
  void init() { theExecEngine.init(); }

  // printout stats
  void dump_mcd_stats() const { theExecEngine.dump_mcd_stats(); }

  // dump counters to xe.cnt file
  void dump_mcd_counters() const { theExecEngine.dump_mcd_counters();}

  // enqueue remote request
  void enqueue(boost::intrusive_ptr<tPacket> packet) { theInputQCntl.enqueue(packet); }

  // find if there is space available

  // get size of the queue
  unsigned int queueSize(const tVC VC) const { return theInputQCntl.size(VC); }


  ///////////////////////////////////////////////////////////
  //
  // main execution loop routine
  //
 public:
  std::string const & engineName() { return theEngineName; }

  void do_cycle() {
   theThreadScheduler.processQueues();

    if (theThreadScheduler.ready()) {
      statBusyCycles++;
      tThread thread = theThreadScheduler.activate();
      theExecEngine.runThread(thread);
      theThreadScheduler.reschedule(thread);
    } else {
      statIdleCycles++;
    }
  }


};

}  // namespace nProtocolEngine


#endif // _PROTOCOL_ENGINE_H_


