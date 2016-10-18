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


#define FLEXUS_BEGIN_COMPONENT DirTracker
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

  #define DBG_DefineCategories DirTracker
  #include DBG_Control()

#include <boost/shared_array.hpp>
#include <iomanip>
#include <deque>


#include "dir_stats.hpp"
#include "directory.hpp"

namespace nDirTracker {

using namespace Flexus::SharedTypes;
using namespace Flexus::Core;
typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

using boost::intrusive_ptr;


template <class Cfg>
class DirTrackerComponent : public FlexusComponentBase<DirTrackerComponent, Cfg> {
  FLEXUS_COMPONENT_IMPL(nDirTracker::DirTrackerComponent, Cfg);

private:
  std::list<PredictorTransport> thePredictorMessagesIn;

  boost::scoped_ptr<DirStats> theDirStats;
  boost::scoped_ptr<Directory> theDirectory;

  Flexus::Stat::StatCounter FlushNotifications;
  Flexus::Stat::StatCounter ReadPredictedNotifications;
  Flexus::Stat::StatCounter ReadNonPredictedNotifications;
  Flexus::Stat::StatCounter WriteNotifications;

  Flexus::Stat::StatCounter PrefetchHitNotifications;
  Flexus::Stat::StatCounter PrefetchHitWithoutStream;
  Flexus::Stat::StatCounter PrefetchAckLate;

public:
  DirTrackerComponent( FLEXUS_COMP_CONSTRUCTOR_ARGS )
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , FlushNotifications(name() + ".Flush Notifications")
    , ReadPredictedNotifications(name() + ".Read Predicted Notifications")
    , ReadNonPredictedNotifications(name() + ".Read Non-Predicted Notifications")
    , WriteNotifications(name() + ".Write Notification")
    , PrefetchHitNotifications(name() + ".Prefetch Hit Notifications")
    , PrefetchHitWithoutStream(name() + ".Prefetch Hit Without Stream")
    , PrefetchAckLate(name() + ".Prefetch Ack Too Late")
  {}


  // Initialization
  void initialize() {
    theDirStats.reset( new DirStats(cfg.NumNodes.value) );
    theDirectory.reset( new Directory(*theDirStats, cfg.UseOS.value) );
  }

  struct FromEngine : public PushInputPort< PredictorTransport >, AlwaysAvailable {
    FLEXUS_WIRING_TEMPLATE
    static void push(self& aMgr, PredictorTransport aMessage) {
      DBG_(Iface, Comp(aMgr) ( << "Received FromEngine: "  << *(aMessage[PredictorMessageTag]) ) );
      aMgr.enqueue(aMessage);
    }
  };

  struct FromLocal : public PushInputPort< PredictorTransport >, AlwaysAvailable {
    FLEXUS_WIRING_TEMPLATE
    static void push(self& aMgr, PredictorTransport aMessage) {
      DBG_(Iface, Comp(aMgr) ( << "Received FromLocal: "  << *(aMessage[PredictorMessageTag]) ) );
      aMgr.enqueue(aMessage);
    }
  };

  // Drive Interfaces
  struct DirStatsDrive {
    FLEXUS_DRIVE( PredictorDrive ) ;
    typedef FLEXUS_IO_LIST_EMPTY Inputs;
    typedef FLEXUS_IO_LIST_EMPTY Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void doCycle(self &aMgr) {
      DBG_(VVerb, Comp(aMgr) (<< "DirStatsDrive" ) ) ;
      aMgr.process();
    }
  };

  typedef FLEXUS_DRIVE_LIST (1, DirStatsDrive ) DriveInterfaces;

private:
  void enqueue(PredictorTransport & aMessage) {
    thePredictorMessagesIn.push_back(aMessage);
  }

  void process() {
    //Process incoming messages from the Protocol engine - creates new predictions
    processPredictorMessages();
  }

  void processPredictorMessages() {

    while (! thePredictorMessagesIn.empty() ) {
      boost::intrusive_ptr<PredictorMessage> msg = thePredictorMessagesIn.front()[PredictorMessageTag];
      boost::intrusive_ptr<TransactionTracker> tracker = thePredictorMessagesIn.front()[TransactionTrackerTag];
      DBG_Assert(msg);
      DBG_(VVerb, Comp(*this) ( << "Processing predictor message: "  << *msg) );

      switch (msg->type()) {
        case PredictorMessage::eFlush:
          DBG_Assert(tracker);
          DBG_Assert(tracker->OS(), ( << "no OS state in DirStats for: " << *msg ) );
          ++FlushNotifications;
          theDirectory->downgrade(msg->node(), msg->address(), *tracker->OS());
          break;
        case PredictorMessage::eReadPredicted:
          ++ReadPredictedNotifications;
          theDirectory->consume(msg->node(), msg->address(), true, false);
          break;
        case PredictorMessage::eReadNonPredicted:
          DBG_Assert(tracker);
          DBG_Assert(tracker->OS(), ( << "no OS state in DirStats for: " << *msg ) );
          ++ReadNonPredictedNotifications;
          theDirectory->consume(msg->node(), msg->address(), false, *tracker->OS());
          break;
        case PredictorMessage::eWrite:
          DBG_Assert(tracker);
          DBG_Assert(tracker->OS(), ( << "no OS state in DirStats for: " << *msg ) );
          ++WriteNotifications;
          theDirectory->storeMiss(msg->node(), msg->address(), *tracker->OS());
          break;
        default:
          DBG_Assert(false, ( << "DirStats received illegal PredictorMessage: " << *msg ) );
      }

      thePredictorMessagesIn.pop_front();
    }


  }

};

FLEXUS_COMPONENT_CONFIGURATION_TEMPLATE(DirStatsConfiguration,
    FLEXUS_PARAMETER( NumNodes, int, "Number of Nodes", "nodes", 16 )
    FLEXUS_PARAMETER( UseOS, bool, "Use OS references", "os", true )
);

} //End Namespace nDirStats

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT DirStats

  #define DBG_Reset
  #include DBG_Control()

