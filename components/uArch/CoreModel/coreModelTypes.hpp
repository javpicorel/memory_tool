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


#include <list>
#include <iostream>
#include <iomanip>
#include <algorithm>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/shared_ptr.hpp>
#include <core/metaprogram.hpp>
#include <boost/variant/get.hpp>
#include <boost/optional.hpp>
#include <boost/none.hpp>
#include <boost/iterator/reverse_iterator.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
using namespace boost::multi_index;
#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>
#include <core/flexus.hpp>

#include <core/stats.hpp>
namespace Stat = Flexus::Stat;

#include "../coreModel.hpp"
#include "../MapTable.hpp"
#include "../RegisterFile.hpp"
#include "../BypassNetwork.hpp"



namespace nuArch {

using Flexus::Core::theFlexus;

static const int krRegs_Global = kGlobalRegCount ;
static const int krRegs_Windowed = kWindowRegCount;
static const int kfRegs = 64;
static const int kSpecialRegs = kSpecialRegCount;
static const int krRegs_Total = kTotalRegs;

struct by_insn {};
typedef multi_index_container
  < boost::intrusive_ptr< Instruction >
  , indexed_by
    < sequenced<>
    , ordered_non_unique
        < tag<by_insn>
        , identity
            < boost::intrusive_ptr< Instruction >
            >
        >
    >
  >
  rob_t;

typedef std::multimap< PhysicalMemoryAddress, boost::intrusive_ptr< Instruction > >  SpeculativeLoadAddressTracker;



struct MSHR;
typedef std::map< PhysicalMemoryAddress, MSHR > MSHRs_t;

enum eStatus {
    kComplete
  , kAnnulled
  , kIssuedToMemory
  , kAwaitingIssue
  , kAwaitingPort
  , kAwaitingAddress
  , kAwaitingValue
};
std::ostream & operator <<( std::ostream & anOstream, eStatus aCode);

enum eQueue {
    kLSQ
  , kSSB
  , kSB
  , kLastQueueType
};
std::ostream & operator <<( std::ostream & anOstream, eQueue aCode);

bool mmuASI(int asi);
bool interruptASI(int asi);


struct MemQueueEntry {
  boost::intrusive_ptr<Instruction> theInstruction;
  PhysicalMemoryAddress thePaddr_aligned;
  unsigned long long theSequenceNum;
  eQueue theQueue;
  mutable VirtualMemoryAddress theVaddr;
  mutable int theASI;
  mutable bool theSideEffect;
  mutable bool theInverseEndian;
  mutable bool theMMU;
  mutable bool theNonCacheable;
  mutable int theException;
  mutable PhysicalMemoryAddress thePaddr;
  mutable eOperation theOperation;
  mutable eSize theSize;
  mutable boost::optional<InstructionDependance> theDependance;
  mutable bool theAnnulled;
  mutable bool thePartialSnoop;
  mutable bool theIssued;
  mutable bool theStoreComplete;
  mutable bool theSpeculatedValue;
  mutable bool theBypassSB;
  mutable boost::optional< unsigned long long > theValue;
  mutable boost::optional< unsigned long long > theExtendedValue;
  mutable boost::optional< unsigned long long > theCompareValue;
  mutable boost::optional< MSHRs_t::iterator > theMSHR;
  mutable std::set< unsigned long long > theParallelAddresses;
  mutable boost::optional< unsigned long long > theExtraLatencyTimeout;
  MemQueueEntry( boost::intrusive_ptr<Instruction> anInstruction, unsigned long long aSequenceNum, eOperation anOperation, eSize aSize, bool aBypassSB, boost::optional<InstructionDependance> aDependance = boost::none)
    : theInstruction(anInstruction)
    , thePaddr_aligned(kUnresolved)
    , theSequenceNum(aSequenceNum)
    , theQueue(kLSQ)
    , theVaddr(kUnresolved)
    , theASI(0x80)
    , theSideEffect(false)
    , theInverseEndian(false)
    , theMMU(false)
    , theNonCacheable(false)
    , theException(0)
    , thePaddr(kUnresolved)
    , theOperation(anOperation)
    , theSize(aSize)
    , theDependance(aDependance)
    , theAnnulled(false)
    , thePartialSnoop(false)
    , theIssued(false)
    , theStoreComplete(false)
    , theSpeculatedValue(false)
    , theBypassSB(aBypassSB)
    {}
  void describe(std::ostream & anOstream) const;
  friend std::ostream & operator << (std::ostream & anOstream, MemQueueEntry const & anEntry);
  bool isStore() const { return theOperation == kStore || theOperation == kRMW || theOperation == kCAS ; }
  bool isLoad() const { return theOperation == kLoad || theOperation == kRMW || theOperation == kCAS ; }
  bool isMarker() const { return theOperation == kMEMBARMarker ; }
  bool isAtomic() const { return theOperation == kRMW || theOperation == kCAS ; }
  bool isAbnormalAccess() const { return theSideEffect || theMMU || theException != 0 || interruptASI(theASI); }
  bool isNAW() const { return theBypassSB; }
  boost::optional< unsigned long long> & loadValue( ) const {
    if (isAtomic()) {
      return theExtendedValue;
    } else {
      return theValue;
    }
  }
  eStatus status() const {
    if (theAnnulled) {
      return kAnnulled ;
    }
    if (theOperation == kMEMBARMarker) {
      return kIssuedToMemory; //Unless annulled, MEMBAR Markers are categorized as "issued to memory"
    }
    if (thePaddr == kUnresolved) {
      return kAwaitingAddress;
    }
    switch (theOperation) {
      case kLoad:
        if (theValue) {
          return kComplete;
        } else if (theMSHR) {
          return kIssuedToMemory;
        } else if (theIssued ) {
          return kAwaitingPort;
        } else {
          return kAwaitingIssue;
        }
        break;
      case kStore:
        if (! theValue) {
          return kAwaitingValue;
        } else if (theMSHR) {
          return kIssuedToMemory;
        } else if (theIssued) {
          return kAwaitingPort;
        } else {
          return kAwaitingIssue;
        }
        break;
      case kRMW:
      case kCAS:
        if ( theValue && theExtendedValue ) {
          //Load is complete, store has obtained value
          return kComplete;
        } else if (theMSHR) {
          return kIssuedToMemory; //Preload is issued to memoty
        } else if (theIssued) {
          return kAwaitingPort;  //Preload is awaiting port
        } else if ( theExtendedValue && ! theValue ) {
          //Store is awaiting value
          return kAwaitingValue;
        } else {
          return kAwaitingIssue; //Preload is awaiting issue
        }
        break;
      default:
        DBG_Assert( false, ( << " Unknown operation: " << *this ) );
    }
    return kComplete; //Suppress warning
  }
};

struct by_paddr {};
struct by_seq {};
struct by_queue {};
struct by_prefetch {};
typedef multi_index_container
  < MemQueueEntry
  , indexed_by
    < sequenced < tag<by_seq> >
    , ordered_unique
        < tag<by_paddr>
        , composite_key
          < MemQueueEntry
          , member< MemQueueEntry, PhysicalMemoryAddress, &MemQueueEntry::thePaddr_aligned>
          , member< MemQueueEntry, unsigned long long, &MemQueueEntry::theSequenceNum >
          >
        >
    , ordered_unique
        < tag<by_insn>
        , member< MemQueueEntry, boost::intrusive_ptr<Instruction>, &MemQueueEntry::theInstruction >
        >
    , ordered_unique
        < tag<by_queue>
        , composite_key
          < MemQueueEntry
          , member< MemQueueEntry, eQueue, &MemQueueEntry::theQueue>
          , member< MemQueueEntry, unsigned long long, &MemQueueEntry::theSequenceNum >
          >
        >
    >
  >
  memq_t;

struct ActionOrder {
  bool operator()(boost::intrusive_ptr< SemanticAction > const & l, boost::intrusive_ptr< SemanticAction > const & r) const;
};

typedef std::priority_queue< boost::intrusive_ptr< SemanticAction >, std::vector< boost::intrusive_ptr< SemanticAction > >, ActionOrder > action_list_t;

struct MSHR {
  PhysicalMemoryAddress thePaddr;
  eOperation theOperation;
  eSize theSize;
  std::list< memq_t::index<by_insn>::type::iterator > theWaitingLSQs;
  std::list< boost::intrusive_ptr<Instruction>  > theBlockedOps;
  std::list< boost::intrusive_ptr<Instruction>  > theBlockedPrefetches;
  boost::intrusive_ptr<TransactionTracker> theTracker;
  MSHR()
   : thePaddr(0)
   , theOperation(kLoad)
   , theSize(kWord)
   {}
};

std::ostream & operator << ( std::ostream & anOstream, MSHR const & anMSHR);

class CoreImpl;

struct PortRequest {
  unsigned long long theAge;
  boost::intrusive_ptr<Instruction> theEntry;
  PortRequest( unsigned long long age, boost::intrusive_ptr<Instruction> anInstruction)
    : theAge(age)
    , theEntry(anInstruction)
  { }
  friend bool operator < (PortRequest const & left,PortRequest const & right ) {
    return left.theAge >  right.theAge; //higher age implies lower priority
  }
};

struct StorePrefetchRequest {
  unsigned long long theAge;
  boost::intrusive_ptr< Instruction > theInstruction;
  StorePrefetchRequest( unsigned long long anAge, boost::intrusive_ptr< Instruction > anInstruction)
   : theAge(anAge)
   , theInstruction(anInstruction)
   {}
};

typedef multi_index_container
  < StorePrefetchRequest
  , indexed_by
    < ordered_unique
        < tag<by_seq>
        , member< StorePrefetchRequest, unsigned long long, &StorePrefetchRequest::theAge >
        >
    , ordered_unique
        < tag<by_insn>
        , member< StorePrefetchRequest, boost::intrusive_ptr<Instruction>, &StorePrefetchRequest::theInstruction >
        >
    >
  >
  prefetch_queue_t;

struct MemoryPortArbiter {
  unsigned int theNumPorts;

  std::priority_queue<PortRequest> theReadRequests;
  std::priority_queue<PortRequest> thePriorityRequests;
  CoreImpl & theCore;

  //Store Prefetching
    unsigned int theMaxStorePrefetches;
    prefetch_queue_t theStorePrefetchRequests;


  MemoryPortArbiter( CoreImpl & aCore, int aNumPorts, int aMaxStorePrefetches);
  void inOrderArbitrate();
  void arbitrate();
  void request( eOperation anOperation, unsigned long long anAge, boost::intrusive_ptr<Instruction> anInstruction);
  void requestStorePrefetch( memq_t::index< by_insn >::type::iterator lsq_entry);
  bool empty() const;
};

struct MemOpCounter {
  MemOpCounter(std::string aName) 
    : theCount( aName + "-Count")
    , theRetireStalls( aName + "-RetireStalls")
    , theRetireStalls_Histogram( aName + "-RetireStalls_Hist")
    , thePrefetchCount( aName + "-PrefetchCount")
    , thePrefetchLatency( aName + "-PrefetchLatency")
    , thePrefetchLatency_Histogram( aName + "-PrefetchLatency_Hist")
    , theRequestCount( aName + "-RequestCount")
    , theRequestLatency( aName + "-RequestLatency")
    , theRequestLatency_Histogram( aName + "-RequestLatency_Hist")
    {}
  Stat::StatCounter theCount;
  Stat::StatCounter theRetireStalls;
  Stat::StatInstanceCounter<long long> theRetireStalls_Histogram;
  Stat::StatCounter thePrefetchCount;
  Stat::StatCounter thePrefetchLatency;
  Stat::StatInstanceCounter<long long> thePrefetchLatency_Histogram;
  Stat::StatCounter theRequestCount;
  Stat::StatCounter theRequestLatency;    
  Stat::StatInstanceCounter<long long> theRequestLatency_Histogram;   
};

struct Checkpoint {
  Checkpoint()
   : theLostPermissionCount(0)
   {}
  v9State theState;
  int theLostPermissionCount;
  std::map< PhysicalMemoryAddress, boost::intrusive_ptr< Instruction > > theRequiredPermissions;
  std::set<PhysicalMemoryAddress> theHeldPermissions;
};  

} //nuArch
