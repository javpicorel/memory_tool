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

#ifndef FLEXUS_PROTOCOL_ENGINE_THREAD_SCHEDULER_HPP_INCLUDED
#define FLEXUS_PROTOCOL_ENGINE_THREAD_SCHEDULER_HPP_INCLUDED

#include "thread.hpp"
#include <core/stats.hpp>

namespace nProtocolEngine {

namespace Stat = Flexus::Stat;

class tInputQueueController;
class tSrvcProvider;
class tMicrocodeEmulator;

class tThreadScheduler {
    std::string theEngineName;
    tTsrf & theTsrf;
    tInputQueueController & theInputQCntl;
    tSrvcProvider & theSrvcProv;
    tMicrocodeEmulator & theExecEngine;

    std::list<tThread> theRunnableThreads;
    boost::optional<tThread> theActiveThread;

    typedef std::multimap<tAddress, tThread> tThreadMap;
    typedef std::pair< tThreadMap::iterator, tThreadMap::iterator>  tMatchedThreads;
    tThreadMap theLiveThreads;

    typedef std::map<tAddress, std::list<tThread> > tBlockedThreads;
    tBlockedThreads theBlockedThreads;

    typedef std::multimap<tThread, tVC > tThreadToStalledQueueMap;
    tThreadToStalledQueueMap theThreadToStalledQueueMap;
    std::set<tVC> theStalledQueues;

    int theNextThreadID;

    Stat::StatAverage statAvgThreadLifetime;
    Stat::StatLog2Histogram statThreadLifetimeDistribution;
    Stat::StatAverage statAvgThreadRuntime;
    Stat::StatLog2Histogram statThreadRuntimeDistribution;
    Stat::StatCounter statLocalQueueAddressConflictStallCycles;
    Stat::StatCounter statLocalQueueTSRFExhaustedStallCycles;
    Stat::StatCounter statNetworkQueueAddressConflictStallCycles;
    Stat::StatCounter statNetworkQueueTSRFExhaustedStallCycles;
    Stat::StatInstanceCounter<long long> statThreadUops;

  public:
    bool isQuiesced() const {
      return theLiveThreads.empty() ;
    }

    //Construction
      tThreadScheduler(std::string const & anEngineName, tTsrf & aTsrf, tInputQueueController & anInputQCntl, tSrvcProvider & aSrvcProv, tMicrocodeEmulator & anExecEngine);

  public:
    //Interface to ProtocolEngine
      //Instruct ThreadDriver to process all input queues and deliver packets
      void processQueues();
      //Returns true if any thread is runnable
      bool ready();
      //Returns a runnable thread selected by the thread scheduler
      tThread activate();
      //Reschedules a thread for further execution, cleanup, etc.
      void reschedule(tThread aThread);

      bool handleDowngrade(tAddress anAddress);
      bool handleInvalidate(tAddress anAddress);

  public:
    //Interface to ExecEngine
      //Implementation of the REFUSE instruction.
      void refusePacket(tThread aThread);
      //Implementation of the RECEIVE instruction.
      void waitForPacket(tThread aThread);

  private:
    //Input Queue Stalling management
      //Stall a virtual channel until a particular thread completes.
      //A channel may only be stalled on one thread (or assertions will fire)
      void stallQueue(tThread aThread, tVC aQueue);
      //Returns true if a virtual channel is stalled
      bool isQueueStalled(tVC aQueue);
      //Releases all virtual channels stalled on a thread
      void releaseAllQueues(tThread aThread);

      void deleteBlockedThread(tThread aThread);
      void unblockThreads(tThread aThread);
      void blockThread(tThread aThread, tThread aBlockOn);

    //Live Thread map management
      //Add a thread to theLiveThreads
      void registerThread( tThread aThread );
      //Reclaim all the resources associated with a thread (TSRF, stalled queues)
      void reclaimThread(tThread aThread);
      //Check if there is at least one live thread for an address
      bool hasLiveThread(tAddress anAddress);
      //Returns the unique thread in aMatchedThreadRange that is in aState, if it exists
      //ASSERTs if there are more than one such thread
      boost::optional<tThread> findUniqueThread(tThreadState aState, tMatchedThreads aMatchedThreadRange);
      //Returns true if there is at least one thread in aMatchedThreadRange that is in aState
      bool anyThreadsAre(tThreadState aState, tMatchedThreads aMatchedThreadRange);

      void launchThread(tThread aThread);

      void createBlockedThread(tVC aVC, tThread aBlockOn);



    //Main processing functions for input queue entries
      //Processes packets which do not have the same address as any existing thread
      void processUnmatchedPacket(tVC aVC);
      //Processes packets which have the same address as an existing thread
      void processMatchedPacket(tVC aVC);
      //Processes lock acquisition messages
      void processLockAcquired(tAddress anAddress);
      //Processes replies from the directory
      void processDirectoryReply(tAddress anAddress, boost::intrusive_ptr<tDirEntry const> aDirEntry);

    //Utility functions for managing thread state
      //Fills in the threads TSRF upon thread creation
      tThread createThread(tVC aVC, tTsrfEntry * anEntry);
        tThread createThread(boost::intrusive_ptr<tPacket> packet, tVC aSourceVC, tTsrfEntry * anEntry); //Used in implementation of createThread(tVC, tTsrfEntry *)
      //Transitions thread to eSuspendedForLock state
      void requestDirectoryLock(tThread aThread);
      //Transitions thread to eSuspendedForDir state
      void requestDirectoryEntry(tThread aThread);
      //Transitions a thread to eRunnable at the specified entry point
      void start(tThread aThread, unsigned anEntryPoint);
      //Transitions a thread to eRunnable and delivers a reply message
      void deliverReply(tThread aThread, tVC aVC);

      //Returns true if a packet needs to obtain a directory lock
      bool requiresDirectoryLock(tPacket const & packet);


};

}  // namespace nProtocolEngine

#endif //FLEXUS_PROTOCOL_ENGINE_THREAD_SCHEDULER_HPP_INCLUDED
