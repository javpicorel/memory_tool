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

#include <core/debug/debug.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>

#include <boost/optional.hpp>
#include <boost/none.hpp>

#include <components/TMSController/TMSPolicies.hpp>
#include <components/Common/Slices/CMOBMessage.hpp>

#include <core/flexus.hpp>


#define TMSDetails Trace
#define TMSHitMiss Dev

  #define DBG_DeclareCategories TMSController, TMSTrace
  #define DBG_SetDefaultOps AddCat(TMSController)
  #include DBG_Control()

namespace nTMSController {

using namespace boost::multi_index;

struct by_order {};
struct by_address {};

struct AddressWindow {
  multi_index_container
    < std::pair< MemoryAddress, bool>
    , indexed_by
      < sequenced < tag<by_order> >
      , ordered_unique 
        < tag< by_address >
        , member< std::pair< MemoryAddress, bool>, MemoryAddress, &std::pair< MemoryAddress, bool>::first >
        >
      >
    > theWindow;
  unsigned int theCapacity;
  
  AddressWindow(unsigned int aCapacity)
   : theCapacity(aCapacity)
   {}
  
  void push_back( MemoryAddress anAddress, bool aFlag = false) {
    anAddress = MemoryAddress( anAddress & ~63);
    theWindow.push_back( std::make_pair(anAddress, aFlag) );
    while (theWindow.size() > theCapacity) {
      theWindow.pop_front(); 
    }
  }
  void erase( MemoryAddress anAddress) {
    anAddress = MemoryAddress( anAddress & ~63);
    theWindow.get<by_address>().erase(anAddress); 
  }
  bool has( MemoryAddress anAddress) {
    anAddress = MemoryAddress( anAddress & ~63);
    return (theWindow.get<by_address>().count(anAddress) > 0);
  }
  std::pair<MemoryAddress, bool> front() {
    return theWindow.front(); 
  }
  void pop_front() {
    theWindow.pop_front(); 
  }
  bool full() const {
     return theWindow.size() >= theCapacity;
  }
  bool empty() const {
     return theWindow.size() == 0;
  }
  int size() const { 
     return theWindow.size();
  }
    
};


namespace WindowState {
  enum eWindowState 
    { Standby
    , VerifyHead    
    , VerifyHeadPending    
    , AddressesRequested
    , Killed
    };
  std::ostream & operator << ( std::ostream & s, eWindowState wnd) {
      char* msg[] = {
      "WndStandby",
      "WndVerifyHead",
      "WndVerifyHeadPending",
      "WndAddressesRequested",
      "WndKilled"
    };
    s << msg[wnd];
    return s;
  }
}

namespace PrefetchState {
  enum ePrefetchState 
    { Filling
    , Standby
    , Watching
    , WatchNext
    , Killed
    };
  std::ostream & operator << ( std::ostream & s, ePrefetchState p) {
      char* msg[] = {
      "PrfFilling",
      "PrfStandby",
      "PrfWatching",
      "PrfWatchNext",
      "PrfKilled"      
    };
    s << msg[p];
    return s;
  }
}


class TMSStreamQueue : public StreamQueueBase {

  unsigned long theUniqueId;
  MemoryAddress theHeadAddress;
  
  int theSourceCMOB;
  long theStartLocation;    //Refers to the location of the head address
  long theRequestLocation;  //Where to ask for more addresses
  
  int theBufferedBlocks;
  int theMaxBufferedBlocks;
  
  int theCredit;

  int theMinAddressQueue;
  
  int theWindowRequestsPending;
  bool theStopAtMiss;
  bool theKilled;
  boost::optional<MemoryAddress> theWatchAddress;

  unsigned long long theBirthTime;
  unsigned long long theLastHitTime;
  unsigned long long theLastPokeTime;
  unsigned long long theLastAddressListTime;
  
  int theHits;  
  int thePrefetches;
  int theRejects;
  int thePokes;
  
  int theLineSize;
  AddressWindow theAddressQueue;  

public:
  TMSStreamQueue(unsigned long aUniqueId, unsigned long long aStartTime, MemoryAddress aHead, int aSourceCMOB, long aStartLocation, long aRequestLocation, int anInitBufferCap, int aFetchWindowAt, int aLineSize)
    : theUniqueId(aUniqueId)
    , theHeadAddress(aHead)
    , theSourceCMOB(aSourceCMOB)
    , theStartLocation(aStartLocation)
    , theRequestLocation(aRequestLocation)
    , theBufferedBlocks(0)
    , theMaxBufferedBlocks(anInitBufferCap)
    , theCredit(0)
    , theMinAddressQueue(aFetchWindowAt)
    , theWindowRequestsPending(0)
    , theStopAtMiss(true)
    , theKilled(false)
    , theBirthTime(aStartTime) 
    , theLastHitTime(0)
    , theLastPokeTime(0)
    , theLastAddressListTime(0)
    , theHits(0)
    , thePrefetches(0)
    , theRejects(0)
    , thePokes(0)
    , theLineSize( aLineSize)
    , theAddressQueue( 4 * aLineSize )
    {}

  //Score for this stream
  long score() const {
    if (theKilled) {
      return 0; 
    }
    //Hits count for 2, - 1 per 100 cycles since last hit    
    long hit_contrib = theHits * 2 - (theFlexus->cycleCount() - theLastHitTime) / 100;
    if (hit_contrib < 0) { 
      hit_contrib = 0;
    }
    
    long buffer_contrib = theBufferedBlocks;
    long pending_list_contrib = theWindowRequestsPending * 6;
    long non_watch_contrib = (theWatchAddress ? 0 : 3);
    return hit_contrib + buffer_contrib + pending_list_contrib + non_watch_contrib;
  }

  bool wantsAddresses() const {
    if (theKilled) {
      return false; 
    }
    //Need more addresses if the queue is small and we have had a useful address since last address list
    if ( (theAddressQueue.size() + theWindowRequestsPending * theLineSize < theMinAddressQueue) && theCredit > 0) {
      return true; 
    }
    return false;
  }

  bool wantsPrefetch() const {
    if (theKilled) {
      return false; 
    }
    if ( theAddressQueue.empty() ) {
      return false; 
    } 
    if ( theWatchAddress ) {
      return false; 
    } 
    return theCredit > 0 && theBufferedBlocks < theMaxBufferedBlocks;
  }
  
  void kill() {
    theKilled = true;
  }
  
  bool isDead() const {
    return ( theKilled || (theBufferedBlocks == 0 && theWindowRequestsPending == 0 && ! wantsAddresses() && ! wantsPrefetch()) );
  }


  MemoryAddress headAddress() const { return theHeadAddress; }
  int sourceCMOB() const { return theSourceCMOB; }
  long startLocation() const { return theStartLocation; }
  unsigned long long birthTime() const { return theBirthTime; }
  unsigned long long lastHitTime() const { return theLastHitTime; }

  int bufferCap() const { return theMaxBufferedBlocks;}
  void setBufferCap(int aCap) { theMaxBufferedBlocks = aCap;}
  int bufferedBlocks() const { return theBufferedBlocks;}
  int credit() const { return theCredit;}
  void setCredit(int aCredit) { theCredit = aCredit;}

  int minAddressQueue() const { return theMinAddressQueue; }
  void setMinAddressQueue(int aMinAddressQueue) { theMinAddressQueue = aMinAddressQueue; }

  int hits() const { return theHits; }
  int prefetches() const { return thePrefetches; }
  int rejects() const { return theRejects; }
  int pokes() const { return thePokes; }
  long id() const { return theUniqueId; }
  

  bool stopAtMiss() const { return theStopAtMiss; }

  void addressListReceived() {   
    DBG_Assert(theWindowRequestsPending > 0);
    --theWindowRequestsPending;
    DBG_Assert(theWindowRequestsPending >= 0);    
    theLastAddressListTime = theFlexus->cycleCount();
  }

  template < class InputIterator >
  void insertAddresses( InputIterator head, InputIterator tail) {
    while (head != tail) {
      theAddressQueue.push_back( head->first, head->second);
      ++head; 
    }
  }

  
  std::pair< int, long> takeCMOBRequest() {
    long location = theRequestLocation;
    DBG_Assert( wantsAddresses() );
    ++theWindowRequestsPending;
    theRequestLocation += theLineSize;
    DBG_Assert( theRequestLocation % theLineSize == 0 );
    return std::make_pair( theSourceCMOB, location );
  }

  std::pair<MemoryAddress, bool> takeAddress() {      
    DBG_Assert(wantsPrefetch() );
    std::pair<MemoryAddress, bool> ret_val;
    ret_val = theAddressQueue.front();
    theAddressQueue.pop_front();
    return ret_val;
  }

  void prefetch() {          
    ++theBufferedBlocks;
    ++thePrefetches;
    --theCredit;
 }

  void watch(MemoryAddress anAddress) {
    theWatchAddress = anAddress;
  }      

  void unbufferBlock() {
    --theBufferedBlocks;
    DBG_Assert( theBufferedBlocks >= 0 );    
  }

  void hit(MemoryAddress anAddress) {   
    ++theHits; 
    if (theWatchAddress && anAddress == *theWatchAddress) {
      theWatchAddress = boost::none;
      theStopAtMiss = false;      
    }
    theLastHitTime = theFlexus->cycleCount();
    theLastPokeTime = theFlexus->cycleCount();
  }

  void poke(MemoryAddress anAddress){ 
    ++thePokes;
    if (theWatchAddress && anAddress == *theWatchAddress) {
      theWatchAddress = boost::none;
      theStopAtMiss = false;      
    }
    theLastPokeTime = theFlexus->cycleCount();
    theCredit = theMaxBufferedBlocks - theBufferedBlocks;
  }

 
  void reject(MemoryAddress anAddress) {    
    unbufferBlock();
    ++theRejects;
    if (theWatchAddress && anAddress == *theWatchAddress ) {
      theWatchAddress = boost::none;
    }
  }
  
  bool near( int aSource, long aLocation ) const {
    if (aSource == theSourceCMOB ) {
      int diff =  aLocation - theStartLocation;
      return ( abs(diff) < 2 * theLineSize );
    }
    return false;
  }  

  bool hasAddress( MemoryAddress anAddress) {
    return theAddressQueue.has(anAddress); 
  }

  friend std::ostream & operator << (std::ostream & aStream, TMSStreamQueue const & aQueue);
};

std::ostream & operator << (std::ostream & aStream, TMSStreamQueue const & q) {
  aStream << "S#" << q.theUniqueId<< "{" << q.theHeadAddress << "} " 
          << q.theSourceCMOB << ":" << q.theStartLocation << "-" << q.theRequestLocation << " " 
          << q.theBufferedBlocks << "/" << q.theMaxBufferedBlocks  << "{+" << q.theCredit << "} "
          << " h=" << q.theHits << "/" << q.thePrefetches 
          << " s=" << q.score() << " ";
  if ( q.theRejects > 0) {
    aStream << "r=" <<  q.theRejects << " ";
  }
  if ( q.thePokes > 0) {
    aStream << "poke=" <<  q.thePokes << " ";
  }
          
  aStream << "q=" << q.theAddressQueue.size() << "+(" <<q.theWindowRequestsPending * q.theLineSize<< ") ";
  if (! q.theStopAtMiss) {
    aStream << "[run-past-miss] ";
  }
  if (q.isDead()) {
    aStream << "[dead] ";
  }
  if (q.theKilled) {
    aStream << "[kill] ";
  }
  if (q.theWatchAddress) {
    aStream << "[watching@"<< *q.theWatchAddress << "] ";
  }
  if (q.wantsAddresses()) {
    aStream << "[wants-addresses] ";
  }
  if (q.wantsPrefetch()) {
    aStream << "[wants-prefetch] ";
  }
  return aStream;  
}


bool operator < (boost::intrusive_ptr<TMSStreamQueue> l, boost::intrusive_ptr<TMSStreamQueue> r) {
  return l->headAddress() < r->headAddress();  
}


struct ByScore {
  bool operator() (boost::intrusive_ptr<TMSStreamQueue> l, boost::intrusive_ptr<TMSStreamQueue> r) {
    return  ( l->score() ) > (r->score() );
  }  
};

class TMSPolicyBase: public PrefetchPolicy {
  protected:
    int theIndex;
    TMSStats & theStats;
    Controller & theController;

    unsigned int theStreamId;

    AddressWindow theRecentObservedReads;
    AddressWindow theRecentPrefetchHits;

    AddressWindow theRecentPrefetches;
    AddressWindow theRecentPBRequests;
    
    std::map< long, boost::intrusive_ptr<TMSStreamQueue>  > theStreamQueues;
        
    std::list< boost::tuple< MemoryAddress, int, long, unsigned long long > > theNewQueues_NeedRequest;
    std::map< long, boost::tuple< MemoryAddress, int, long, unsigned long long > > theNewQueues_InFlight;
    long theNextQueueRequest;
        
    int theOutstandingLookups;
    int theLineSize;

    TMSPolicyBase(int anIndex, TMSStats & aStats, Controller & aController, int aLineSize)
     : theIndex(anIndex)
     , theStats(aStats)
     , theController(aController)
     , theStreamId(1)
     , theRecentObservedReads(128)
     , theRecentPrefetchHits(128)
     , theRecentPrefetches(32)
     , theRecentPBRequests(32)
     , theNextQueueRequest( -1 )
     , theOutstandingLookups(0)
     , theLineSize( aLineSize) 
     {}

    virtual bool isOnChip() const { return false; }

    void recentPBRequest( MemoryAddress const & anAddress) {
      theRecentPBRequests.push_back( anAddress );  
    }
  
    void notifyRead( MemoryAddress const & anAddress, tFillLevel aFillLevel, bool wasPredicted) {
      MemoryAddress addr = MemoryAddress(anAddress & ~63);
      bool is_hit = false;
      bool is_observed = false;
      if (theRecentObservedReads.has( addr ) ) {
        theRecentObservedReads.erase( addr );
        is_observed = true;
      }
      if (theRecentPrefetchHits.has( addr ) ) {
        theRecentPrefetchHits.erase( addr );
        is_hit = true;
      }
      
      if (is_observed || is_hit) {
        append( anAddress, is_hit ); 
      }        
      
      if (is_hit) {
        ++theStats.statCommit_SVBHit;
        DBG_( Iface, ( << " CPU[" << theIndex << "] Commit SVB HIT @" << addr ) );              
      } else if (is_observed ) {
        ++theStats.statCommit_ObservedRead;
        
        DBG_( TMSHitMiss, ( << "MISS" << ( isOnChip() ? "-ON" : "-OFF" ) << " CPU[" << theIndex << "] @" << addr << " " << aFillLevel) );      
      }
    }
  
    void considerLookup(MemoryAddress const & anAddress) {
      //We have an address that may create a stream.  Do not look it up if the address is already prefetched
      if (theController.addressPending(anAddress)) {
        DBG_( TMSDetails, ( << " CPU[" << theIndex << "] No stream lookup for " << anAddress << " : address is pending") );              
        ++theStats.statNoLookup_Pending;
        return; //No stream
      }

      if (theRecentPrefetches.has(anAddress)) {
        DBG_( TMSDetails, ( << " CPU[" << theIndex << "] No stream lookup for " << anAddress << " : address recently prefetched ") );              
        ++theStats.statNoLookup_RecentPrefetch;
        return; //No stream
      }

      //See if the address is in any queue.  If so, poke the queue and do not prefetch      
      std::map< long, boost::intrusive_ptr<TMSStreamQueue> >::iterator iter = theStreamQueues.begin();
      std::map< long, boost::intrusive_ptr<TMSStreamQueue> >::iterator end = theStreamQueues.end();
  
      while (iter != end) {
        if ( iter->second->hasAddress( anAddress ) ) {
           //Poke the queue that already has the address
           DBG_( TMSDetails, ( << " CPU[" << theIndex << "] No stream lookup for " << anAddress << " : address is in an existing queue ") );                        
           ++theStats.statNoLookup_InQueue;
           DBG_( TMSDetails, ( << " CPU[" << theIndex << "] " << anAddress << " POKE " << *(iter->second)) );              
           iter->second->poke(anAddress);
           ++theStats.statPokes;
           return; //No stream
        }
        ++iter;
      }      

      if (theOutstandingLookups > 8) {
        DBG_( TMSDetails, ( << " CPU[" << theIndex << "] No stream lookup for " << anAddress << " : too many outstanding ") );              
        ++theStats.statNoLookup_TooMany;
        return; //No stream         
      }      
      
      ++theOutstandingLookups;
      DBG_( TMSDetails, ( << " CPU[" << theIndex << "] Request lookup (" << theOutstandingLookups <<") " << anAddress ) );                        
      ++theStats.statLookups;
      requestLookup(anAddress);      
    }
  
    void lookupNoMatch() {
      --theOutstandingLookups;
      DBG_Assert( theOutstandingLookups >= 0);
    }

    //Index lookup returned
    void lookupMatch( MemoryAddress const & anAddress, int aCMOB, long aLocation, unsigned long long aStartTime) {
      //See if we actually want to allocate a queue for this address
      --theOutstandingLookups;
      DBG_Assert( theOutstandingLookups >= 0);
      
      //We have an address that may create a stream.  Do not look it up if the address is already prefetched
      if (theController.addressPending(anAddress)) {
        DBG_( TMSDetails, ( << " CPU[" << theIndex << "] Lookup match " << anAddress << " discarded : address is pending" ) );                        
        ++theStats.statMatchDiscarded_Pending;
        return; //No stream
      }

      if (theRecentPrefetches.has(anAddress)) {
        DBG_( TMSDetails, ( << " CPU[" << theIndex << "] Lookup match " << anAddress << " discarded : address recently prefetched " ) );                        
        ++theStats.statMatchDiscarded_RecentPrefetch;
        return; //No stream
      }
      
      //Don't create new queues within 2 CMOBLine of existing queues
      if (nearExistingQueue( aCMOB, aLocation) ) {
        DBG_( TMSDetails, ( << " CPU[" << theIndex << "] Lookup match " << anAddress << " discarded : near existing queue" ) );                        
        ++theStats.statMatchDiscarded_NearExisting;
        return; //No stream
      }

      ++theStats.statNewStreamRequests;
      DBG_( TMSDetails, ( << " CPU[" << theIndex << "] Lookup match " << anAddress << " enqueued for stream request " ) );                        
      theNewQueues_NeedRequest.push_front( make_tuple(anAddress, aCMOB, aLocation, aStartTime) );
     
    }
  
    void sortQueues( std::vector< boost::intrusive_ptr<TMSStreamQueue> > & aVec) {
      aVec.clear();
      std::map< long, boost::intrusive_ptr<TMSStreamQueue> >::iterator it, end;
      it = theStreamQueues.begin();
      end = theStreamQueues.end();
                  
      while (it != end) {
        aVec.push_back( it->second );
        ++it; 
      }
            
      std::sort( aVec.begin(), aVec.end(), ByScore() );
    }

    //Choose to replace a Queue
    void victimizeQueue() {

      std::vector< boost::intrusive_ptr<TMSStreamQueue> > queues;
      sortQueues( queues );      
      DBG_Assert( !queues.empty() );      
      boost::intrusive_ptr<TMSStreamQueue> q = queues.back();
      
      DBG_( TMSDetails, ( << " CPU[" << theIndex << "] Victim-queue " << *q ) );                        

      //Clean up dead queues
      ++theStats.statStreamQueuesKilled;
      unsigned long long birth = q->birthTime();
      unsigned long long lifetime = theFlexus->cycleCount() - birth;
      theStats.statStreamTimeToDeath << lifetime;
      theStats.statStreamLength << std::make_pair( q->hits(), 1);
      theStreamQueues.erase(q->id());            
    }

    void filterAddressLists(std::vector<MemoryAddress> & anAddressList, std::vector<bool> & aWasHit, std::vector< std::pair<MemoryAddress, bool> > & output) {
      for (int i = 0; i< anAddressList.size(); ++i) {        
        if (theController.addressPending(anAddressList[i]) || theRecentPrefetches.has(anAddressList[i]) ||  theRecentPBRequests.has(anAddressList[i]) ) {
          //Skip this address - in flight or recently prefetched
        } else {
          output.push_back( std::make_pair(anAddressList[i], aWasHit[i]));
        }
      }      
    }

    void handleCMOBReply( int aCMOB, long aLocation, long anId, std::vector<MemoryAddress> & anAddressList, std::vector<bool> & aWasHit ) {
      if (anId > 0) {
        handleExtendQueue(anId, aCMOB, aLocation, anAddressList, aWasHit);
      } else {
        handleNewQueue(anId, aCMOB, aLocation, anAddressList, aWasHit);        
      }
    }

    void handleNewQueue( long anId, int aCMOB, long aLocation, std::vector<MemoryAddress> & anAddressList, std::vector<bool> & aWasHit ) {
      DBG_Assert( anId < 0);
      DBG_Assert( theNewQueues_InFlight.count( anId) == 1);
      
      boost::tuple< MemoryAddress, int, long, unsigned long long > request = theNewQueues_InFlight[anId];
      theNewQueues_InFlight.erase( anId );

      //Does the head match what it should be?
      if (anAddressList.front() != request.get<0>()) {
        DBG_( TMSDetails, ( << " CPU[" << theIndex << "] No-new-queue " << request.get<0>() << " : stale " ) );                                
        ++theStats.statListDiscarded_Stale;
        return; //Stale Queue - head doesn't match 
      }

      if (nearExistingQueue( aCMOB, aLocation) ) {
        DBG_( TMSDetails, ( << " CPU[" << theIndex << "] Non-new-queue " << request.get<0>()  << " : near existing queue" ) );                        
        ++theStats.statListDiscarded_NearExisting;
        return; //No stream
      }
              
      //Figure out next request location      
      long aNextLocation = aLocation + anAddressList.size();
            
      //Filter the address list down to what is interesting
      std::vector< std::pair<MemoryAddress, bool> > good_addresses;
      filterAddressLists( anAddressList, aWasHit, good_addresses);
      
      if (good_addresses.empty() ) {
        DBG_( TMSDetails, ( << " CPU[" << theIndex << "] No-new-queue " << request.get<0>() << " : no good addresses" ) );                                
        ++theStats.statListDiscarded_NoAddresses;
        return; //No useful addresses in this address list - don't bother allocating 
      }

      //We want to allocate a stream queue!!!!      
      while (theStreamQueues.size() >= theController.maxStreamQueues()) {
        victimizeQueue(); 
      }
      
      ++theStats.statStreamQueuesAllocated;
      boost::intrusive_ptr<TMSStreamQueue> q(new TMSStreamQueue(theStreamId++, request.get<3>(), request.get<0>(), aCMOB, aLocation, aNextLocation, theController.initBufferCap(), theController.fetchCMOBAt(), theLineSize ) );
      theStreamQueues.insert(std::make_pair(q->id(), q));
      
      q->insertAddresses( good_addresses.begin(), good_addresses.end() );
      q->setCredit( theController.initBufferCap() );

      DBG_( TMSDetails, ( << " CPU[" << theIndex << "] New-queue " << *q ) );                                

    }


    //Address list arrived
    void handleExtendQueue( long aQueueId, int aCMOB, long aLocation, std::vector<MemoryAddress> & anAddressList, std::vector<bool> & aWasHit ) {
      if (theStreamQueues.count( aQueueId) != 1) {
        //Drop the address list on the floor - its queue has been victimized        
        DBG_( TMSDetails, ( << " CPU[" << theIndex << "] Drop-queue-extend " << aQueueId << " : queue is gone" ) );                                
        ++theStats.statExtendDiscarded_NoQueue;
      } else {
        boost::intrusive_ptr<TMSStreamQueue> q = theStreamQueues[aQueueId];        
    
        q->addressListReceived();
        
        std::vector< std::pair<MemoryAddress, bool> > good_addresses;
        filterAddressLists( anAddressList, aWasHit, good_addresses);
        
        if (good_addresses.empty()) {
          DBG_( TMSDetails, ( << " CPU[" << theIndex << "] Failed-Extend-Queue " << *q ) );
          
        } else {        
          q->insertAddresses( good_addresses.begin(), good_addresses.end() );
          ++theStats.statExtendQueue;
          DBG_( TMSDetails, ( << " CPU[" << theIndex << "] Extend-Queue " << *q ) );
        }                                
      }   
    }

    void process() {

      std::vector< boost::intrusive_ptr<TMSStreamQueue> > queues;
      sortQueues( queues );      
      
      std::vector< boost::intrusive_ptr<TMSStreamQueue> >::iterator iter = queues.begin();
      std::vector< boost::intrusive_ptr<TMSStreamQueue> >::iterator end = queues.end();

      while (iter != end) {
        boost::intrusive_ptr<TMSStreamQueue> q(*iter);
        ++iter; 

        //See if the queue wants to prefetch
        while (q->wantsPrefetch() && theController.mayPrefetch() ) {
          doPrefetch(q);
        }
      
        //See if the queue wants to request addresses
        if (q->wantsAddresses() && theController.mayRequestAddresses()) {
          //Get more addresses
          int aCMOB;
          long aLocation;
          boost::tie( aCMOB, aLocation ) = q->takeCMOBRequest();
          DBG_(TMSDetails, ( << "TMSc[" << theIndex << "] CMOB Request @" << aLocation << ": " << *q ) );
          requestCMOBRange( aCMOB, aLocation, q->id() ); 
        }
        

        if (q->isDead()) {
          //Clean up dead queues
          ++theStats.statStreamQueuesDied;
          unsigned long long birth = q->birthTime();
          unsigned long long lifetime = theFlexus->cycleCount() - birth;
          theStats.statStreamTimeToDeath << lifetime;
          theStats.statStreamLength << std::make_pair( q->hits(), 1);
          theStreamQueues.erase(q->id());            
        }        
      }
      
      if (! theNewQueues_NeedRequest.empty() && theController.mayRequestAddresses()) {
        boost::tuple< MemoryAddress, int, long, unsigned long long > request = theNewQueues_NeedRequest.front();
        theNewQueues_NeedRequest.pop_front();
        theNewQueues_InFlight.insert( std::make_pair( theNextQueueRequest, request) );
        DBG_( TMSDetails, ( << " CPU[" << theIndex << "] Issued New Stream request " << request.get<0>() << " " << request.get<1>() << ":" << request.get<2>()) );                        
        requestCMOBRange( request.get<1>(), request.get<2>(), theNextQueueRequest); 
        --theNextQueueRequest;
      }

    }

    //Prefetch one block
    void doPrefetch( boost::intrusive_ptr<TMSStreamQueue> q) {
      MemoryAddress address;
      bool is_hit;
      boost::tie(address, is_hit) = q->takeAddress();    

      //Is this address worth prefetching?
      if (theController.addressPending(address)) {
        //Don't prefetch - already pending
        DBG_( TMSDetails, ( << " CPU[" << theIndex << "] No prefetch " << address << " : address pending " << *q ) );     
        ++theStats.statPrefetchDropped_Pending;
        return;
      }

      if (theRecentPrefetches.has(address)) {
        //Don't prefetch - recently prefetched
        DBG_( TMSDetails, ( << " CPU[" << theIndex << "] No prefetch " << address << " : recently prefetched " << *q ) );                                
        ++theStats.statPrefetchDropped_RecentPrefetch;
        return;         
      }

      if (theRecentPBRequests.has(address)) {
        //Don't prefetch - recently explicitly requested
        DBG_( TMSDetails, ( << " CPU[" << theIndex << "] No prefetch " << address << " : recently PB-seen " << *q ) ); 
        ++theStats.statPrefetchDropped_RecentRequest;
        DBG_( TMSDetails, ( << " CPU[" << theIndex << "] " << address << " POKE " << *q) );              
        q->poke( address );                               
        ++theStats.statPokes;
        return;                  
      }

      //Prefetch the block
      if (! is_hit && q->stopAtMiss()) {
        q->watch( address );
      }       

      if (q->prefetches() == 0) {
        unsigned long long birth = q->birthTime();
        unsigned long long lifetime = theFlexus->cycleCount() - birth;
        theStats.statStreamTimeToFirstPrefetch << lifetime;             
      }

      theController.prefetch( address, q );
      q->prefetch();
      DBG_( TMSDetails, ( << " CPU[" << theIndex << "] PREFETCH " << address << " " << *q ) );                                
      theRecentPrefetches.push_back(address);            
    }


    void cancelPrefetch(MemoryAddress const & anAddress, boost::intrusive_ptr<StreamQueueBase> aQ ) {
      boost::intrusive_ptr< TMSStreamQueue > q (boost::dynamic_pointer_cast<TMSStreamQueue >( aQ ));
      q->reject(anAddress);
      DBG_( TMSDetails, ( << " CPU[" << theIndex << "] Reject " << anAddress << " " << *q ) );                                
    }
    void removeLine(MemoryAddress const & anAddress, bool isReplacement, boost::intrusive_ptr<StreamQueueBase> aQ  ) {
      boost::intrusive_ptr< TMSStreamQueue > q (boost::dynamic_pointer_cast<TMSStreamQueue >( aQ ));
      q->unbufferBlock();
      if (isReplacement) {
        DBG_( TMSDetails, ( << " CPU[" << theIndex << "] Replace " << anAddress << " " << *q ) );                                
      } else {
        DBG_( TMSDetails, ( << " CPU[" << theIndex << "] Inval " << anAddress << " " << *q ) );                                        
      }
    }  

    void hit(MemoryAddress const & anAddress, boost::intrusive_ptr<StreamQueueBase> aQ , bool wasPartial ) { 
      boost::intrusive_ptr< TMSStreamQueue > q (boost::dynamic_pointer_cast<TMSStreamQueue >( aQ ));      
      q->unbufferBlock();
      if (q->hits() == 0) {
        unsigned long long birth = q->birthTime();
        unsigned long long lifetime = theFlexus->cycleCount() - birth;
        theStats.statStreamTimeToFirstHit << lifetime;        

        if (theController.usePIndex()) {
          ++theStats.statPersistentIndexInserts;
          theController.recordMapping( kPersistentIndex_Insert, q->headAddress(), q->sourceCMOB(), q->startLocation()); 
        }  
      }

      unsigned long long last_hit = q->lastHitTime();
      q->hit(anAddress);

      //Adjust credit
      int buffer_cap = q->bufferCap();
      if (wasPartial) {
        buffer_cap += 2;
        buffer_cap += 2;
        if (buffer_cap > theController.maxBufferCap()) {
          buffer_cap = theController.maxBufferCap();
        }
        int min_address_queue = q->minAddressQueue();
        min_address_queue += 2;
        if (min_address_queue > 3 * theLineSize - 1) {
          min_address_queue = 3 * theLineSize - 1;          
        }
        q->setMinAddressQueue( min_address_queue );
      } else {
        if (theFlexus->cycleCount() - last_hit > 1000) {
          -- buffer_cap;
          if (buffer_cap < theController.minBufferCap()) {
            buffer_cap = theController.minBufferCap();
          }         
        }
      }
      q->setBufferCap( buffer_cap );
      q->setCredit( buffer_cap - q->bufferedBlocks() );

      theRecentPrefetchHits.push_back( anAddress );  
      DBG_(TMSHitMiss, ( << "HIT" << ( isOnChip() ? "-ON" : "-OFF" ) << " CPU[" << theIndex << "] " << anAddress << " " << *q ) );
    }

    bool nearExistingQueue( int aSource, long aLocation) {
      std::map< long, boost::intrusive_ptr<TMSStreamQueue>  >::iterator iter = theStreamQueues.begin();
      std::map< long, boost::intrusive_ptr<TMSStreamQueue>  >::iterator end = theStreamQueues.end();
      while (iter != end) {
        boost::intrusive_ptr<TMSStreamQueue> q(iter->second);
        ++iter;
        if (q->near( aSource, aLocation)) {
          return true; 
        }
      }
      return false;
    }

    void requestLookup( MemoryAddress anAddress) {
      if (theController.usePIndex()) {
        ++theStats.statTrainingIndexLookups;
        theController.requestMapping( kPersistentIndex, anAddress); 
        
      } else {
        ++theStats.statTrainingIndexLookups;
        theController.requestMapping( kTrainingIndex, anAddress); 
      }
    }
  
    void append( MemoryAddress anAddress, bool wasHit) {
      long cmob_index = theController.getNextCMOBIndex();
      ++theStats.statCMOBAppends;
      theController.appendToCMOB(anAddress, wasHit);
      
      //Also need to add to indices
      if (theController.usePIndex()) {
        ++theStats.statPersistentIndexUpdates;
        theController.recordMapping( kPersistentIndex_Update, anAddress,  theIndex, cmob_index);              
      } else{
        ++theStats.statTrainingIndexInserts;
        theController.recordMapping( kTrainingIndex, anAddress,  theIndex, cmob_index);      
      }
    }

    void requestCMOBRange( int aCMOB, int aStartOffset, long anId) {
      DBG_( Verb, ( << "Request Addresses from CMOB[" << aCMOB <<"] at " << aStartOffset ));
      ++theStats.statCMOBRequests;
      theController.requestCMOBRange(aCMOB, aStartOffset, anId);
    }
  
};

class TMSPrefetchPolicy : public TMSPolicyBase {

        
  public:
    TMSPrefetchPolicy(int anIndex, TMSStats & aStats, Controller & aController)
     : TMSPolicyBase(anIndex, aStats, aController, 12)
     {}

    virtual bool isOnChip() const { return false; }
     
  //Policy functions  
    void observe( MemoryMessage const & aMessage ) {    
      if (aMessage.type() == MemoryMessage::ReadReq || aMessage.type() == MemoryMessage::PrefetchReadNoAllocReq ) { 
        theRecentObservedReads.push_back( aMessage.address() );      
        if (aMessage.type() == MemoryMessage::ReadReq) {
          DBG_( Iface, ( << " CPU[" << theIndex << "] Off chip Read " << aMessage) );      
          ++theStats.statPins_Read;
          //Trigger a TMS lookup
          considerLookup( aMessage.address() );
        } else {
          ++theStats.statPins_Prefetch;
          DBG_( Iface, ( << " CPU[" << theIndex << "] TMS-Prefetch " << aMessage) );                
        }
      }
    }
        
    

};

PrefetchPolicy * PrefetchPolicy::createTMSPolicy(int anIndex, TMSStats & aStats, Controller & aController) {
  return new TMSPrefetchPolicy(anIndex, aStats, aController);
}
  

class TMSOnchipPolicy : public TMSPolicyBase {
  public:
    TMSOnchipPolicy(int anIndex, TMSStats & aStats, Controller & aController)
     : TMSPolicyBase(anIndex, aStats, aController, 8)
     {}

    virtual bool isOnChip() const { return true; }
    
  //Policy functions  

    void observe( MemoryMessage const & aMessage ) {          
      switch ( aMessage.type() ) {
        case MemoryMessage::MissReply:
        case MemoryMessage::MissReplyWritable:
        case MemoryMessage::MissReplyDirty:
          DBG_( TMSDetails, ( << " CPU[" << theIndex << "] Reply Message " << aMessage << " fill: " << aMessage.fillLevel()) );      
          if (aMessage.fillLevel() == eL2 || aMessage.fillLevel() == ePeerL1Cache) {
            theRecentObservedReads.push_back( aMessage.address() );      
            ++theStats.statOnChip_Read;
            //Trigger a TMS lookup
            considerLookup( aMessage.address() );
            // 
          }
          break;
        default:
          //Do nothing 
          break;
      }
    }
    

};

PrefetchPolicy * PrefetchPolicy::createOnChipTMSPolicy(int anIndex, TMSStats & aStats, Controller & aController) {
  return new TMSOnchipPolicy(anIndex, aStats, aController);
}

} //End Namespace nTMSController


  #define DBG_Reset
  #include DBG_Control()

