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
/*! \file CacheController.hpp
 * \brief
 *
 *  This file contains the implementation of the CacheController.  Alternate
 *  or extended definitions can be provided here as well.  This component
 *  is a main Flexus entity that is created in the wiring, and provides
 *  a full cache model.
 *
 * Revision History:
 *     ssomogyi    17 Feb 03 - Initial Revision
 *     twenisch    23 Feb 03 - Integrated with CacheImpl.hpp
 *     twenisch    03 Sep 04 - Split implementation out to compile separately
 */

#ifndef FLEXUS_CACHE_CONTROLLER_HPP_INCLUDED
#define FLEXUS_CACHE_CONTROLLER_HPP_INCLUDED

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace ll = boost::lambda;
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/composite_key.hpp>
using namespace boost::multi_index;
#include <boost/none.hpp>

#include <components/Common/MessageQueues.hpp>
#include <components/Common/Slices/ExecuteState.hpp>


#include "BaseCacheControllerImpl.hpp"

  #define DBG_DeclareCategories Cache
  #define DBG_SetDefaultOps AddCat(Cache)
  #include DBG_Control()

namespace nCache {

  using namespace nMessageQueues;
  typedef Flexus::SharedTypes::MemoryTransport Transport;

  // the states that a MAF entry may be in
  enum MafStates
    { kWaitResponse
        //This indicates that a request of some sort has been sent on behalf
        //of this MAF entry, and a response will eventually arrive, fulfilling
        //this miss.  There can be at most one MAF entry in kWaitResponse per
        //block address
    , kWaitAddress
        //This indicates that a MAF entry has not yet been examined because its
        //address conflicts with an entry in kWaitResponse state.  There should
        //always be one entry in kWaitResponse if there are any kWaitAddress,
        //except when the cache is in the process of waking kWaitAddress entries.
    , kWaking
        //This MAF entry is in the process of waking up
    , kWaitProbe
        //This indicates that the request in this MAF entry is waiting for a probe
        // to return.  Currently, this is only used for Ifetch misses.
    , kProbeHit
        //This indicates that the request in this MAF entry is waiting for a probe
        // to return.  Currently, this is only used for Ifetch misses.
    , kProbeMiss
        //This indicates that the request in this MAF entry is waiting for a probe
        // to return.  Currently, this is only used for Ifetch misses.
    , kCompleted
        //This indicates that the request has completed and created all of its
        // response messages, however the messages have not been sent yet and
        // further transactions on the block are not cleared to proceed.
  };

  enum eEntryType
    { kRead
    , kWrite
    , kAtomic
    , kFetch
    , kReadPrefetch
    , kWritePrefetch
    , kExtInvDown // downgrade or invalidate from outside
    , kPurge /* CMU-ONLY */

    , kTotal
    , kLastType
    };

  struct MafEntry {
      MemoryAddress theBlockAddress;  //The block address of the entry, for index by_addr
      MafStates state;                //State of the MAF entry
      mutable Transport transport;    //the transport object associated with this miss
      mutable eEntryType type;
      bool isPurge; /* CMU-ONLY */

      MafEntry(MemoryAddress aBlockAddress
               , Transport & aTransport
               , MafStates aState
               , eEntryType aType
               , bool aPurgeFlag=false /* CMU-ONLY */
              )
        : theBlockAddress(aBlockAddress)
        , state(aState)
        , transport(aTransport)
        , type(aType)
        , isPurge(aPurgeFlag) /* CMU-ONLY */
        {}
  };

  class MissAddressFile {
    //MAF entries are indexed by their block address and state
    typedef multi_index_container
      < MafEntry
      , indexed_by
        < ordered_non_unique
            < composite_key
                < MafEntry
                , member< MafEntry, MemoryAddress, &MafEntry::theBlockAddress >
                , member< MafEntry, MafStates, &MafEntry::state>
                >
            >
        >
      >
      maf_t;
    maf_t theMshrs;
    unsigned int theSize;
    unsigned int theMaxTargetsPerRequest;
    unsigned int theWaitResponseEntries;

    unsigned int theEntries[kLastType];
    Stat::StatInstanceCounter<long long> * theCyclesWith[kLastType];
    Stat::StatAverage * theAverage[kLastType];
    unsigned long long theLastAccounting;
    Stat::StatMax theMaxMAFTargets;
    Stat::StatMax theMaxMAFMisses;
    std::string theName;
    int theReserve;

    int theBlockSize;
    unsigned long long thePageSize;


  public:
    typedef maf_t::iterator maf_iter;

    MissAddressFile(std::string const & aStatName, unsigned int aSize, unsigned int aMaxTargetsPerRequest, int aBlockSize, unsigned long long aPageSize)
      : theSize(aSize)
      , theMaxTargetsPerRequest(aMaxTargetsPerRequest)
      , theWaitResponseEntries(0)
      , theLastAccounting(0)
      , theMaxMAFTargets( aStatName + "-MaxMAFTargets")
      , theMaxMAFMisses( aStatName + "-MaxMAFMisses")
      , theName(aStatName)
      , theReserve(0)
      , theBlockSize( aBlockSize )
      , thePageSize( aPageSize )
    {
        DBG_Assert(theMaxTargetsPerRequest == 0); //Not supported

        for (int i = 0; i < kLastType; ++i) {
          theEntries[i] = 0;
        }

        theCyclesWith[kRead] = new Stat::StatInstanceCounter<long long>(aStatName + "-MAFEntries:Read");
        theAverage[kRead] = new Stat::StatAverage(aStatName + "-MAFAvg:Read");
        theCyclesWith[kWrite] = new Stat::StatInstanceCounter<long long>(aStatName + "-MAFEntries:Write");
        theAverage[kWrite] = new Stat::StatAverage(aStatName + "-MAFAvg:Write");
        theCyclesWith[kAtomic] = new Stat::StatInstanceCounter<long long>(aStatName + "-MAFEntries:Atomic");
        theAverage[kAtomic] = new Stat::StatAverage(aStatName + "-MAFAvg:Atomic");
        theCyclesWith[kFetch] = new Stat::StatInstanceCounter<long long>(aStatName + "-MAFEntries:Fetch");
        theAverage[kFetch] = new Stat::StatAverage(aStatName + "-MAFAvg:Fetch");
        theCyclesWith[kReadPrefetch] = new Stat::StatInstanceCounter<long long>(aStatName + "-MAFEntries:ReadPrefetch");
        theAverage[kReadPrefetch] = new Stat::StatAverage(aStatName + "-MAFAvg:ReadPrefetch");
        theCyclesWith[kWritePrefetch] = new Stat::StatInstanceCounter<long long>(aStatName + "-MAFEntries:WritePrefetch");
        theAverage[kWritePrefetch] = new Stat::StatAverage(aStatName + "-MAFAvg:WritePrefetch");
        theCyclesWith[kPurge] = new Stat::StatInstanceCounter<long long>(aStatName + "-MAFEntries:Purge"); /* CMU-ONLY */
        theAverage[kPurge] = new Stat::StatAverage(aStatName + "-MAFAvg:Purge"); /* CMU-ONLY */
        theCyclesWith[kExtInvDown] = new Stat::StatInstanceCounter<long long>(aStatName + "-MAFEntries:ExtInvDown");
        theAverage[kExtInvDown] = new Stat::StatAverage(aStatName + "-MAFAvg:ExtInvDown");
        theCyclesWith[kTotal] = new Stat::StatInstanceCounter<long long>(aStatName + "-MAFEntries:Total");
        theAverage[kTotal] = new Stat::StatAverage(aStatName + "-MAFAvg:Total");
    }

    eEntryType getType(MemoryMessage::MemoryMessageType aMessageType) {
        switch(aMessageType) {
          case MemoryMessage::AtomicPreloadReq:  //twenisch - is this correct?
          case MemoryMessage::LoadReq:
          case MemoryMessage::ReadReq:
          case MemoryMessage::StreamFetch:
            return kRead;
          case MemoryMessage::StoreReq:
          case MemoryMessage::WriteReq:
          case MemoryMessage::WriteAllocate:
          case MemoryMessage::UpgradeReq:
          case MemoryMessage::UpgradeAllocate:
          case MemoryMessage::NonAllocatingStoreReq:
            return kWrite;
          case MemoryMessage::RMWReq:
          case MemoryMessage::CmpxReq:
            return kAtomic;
          case MemoryMessage::FetchReq:
            return kFetch;
          case MemoryMessage::PrefetchReadNoAllocReq:
          case MemoryMessage::PrefetchReadAllocReq:
            return kReadPrefetch;
          case MemoryMessage::StorePrefetchReq:
            return kWritePrefetch;
          case MemoryMessage::Invalidate:
          case MemoryMessage::Downgrade:
          case MemoryMessage::ReturnReq:
            return kExtInvDown;
          case MemoryMessage::PurgeIReq: /* CMU-ONLY */
          case MemoryMessage::PurgeDReq: /* CMU-ONLY */
            return kPurge; /* CMU-ONLY */
          default:
            DBG_Assert(false, ( << "Illegal message type entered into MAF: " << aMessageType) );
            return kRead;
        }
    }

    void account(eEntryType anEntry, int aDelta) {
      //Accumulate counts since the last accounting
      long long time = Flexus::Core::theFlexus->cycleCount() - theLastAccounting;
      if (time > 0) {
        for (int i = 0; i < kLastType; ++i) {
          *(theCyclesWith[i]) << std::make_pair( static_cast<long long>(theEntries[i]), time );
          *(theAverage[i]) << std::make_pair( static_cast<long long>(theEntries[i]), time );
        }
      }

      if (aDelta != 0) {
        // Modify counts
        theEntries[anEntry] += aDelta;
        theEntries[kTotal] += aDelta;
      }

      //Assert that counts add up to MSHR contends
      int sum = 0;
      for (int i = 0; i < kTotal; ++i) {
        sum += theEntries[i];
      }
      DBG_Assert( sum == static_cast<int>(theEntries[kTotal]));
      DBG_Assert( sum == static_cast<int>(theMshrs.size()));
    }

    void allocEntry(MemoryAddress aBlockAddress
                    , Transport & aTransport
                    , MafStates aState
                    , bool aPurgeFlag=false /* CMU-ONLY */
                   )
    {
      DBG_( Trace,
            Addr(aBlockAddress)
            ( << theName
              << " Allocating for " << std::hex << aBlockAddress << std::dec 
              << " msg: " << *aTransport[MemoryMessageTag]
              << " MafState: " << aState
              << " MafPurge: " << aPurgeFlag /* CMU-ONLY */
          ));
      DBG_Assert(! full() );
      eEntryType type = getType(aTransport[MemoryMessageTag]->type());
      theMshrs.insert( MafEntry( aBlockAddress
                                 , aTransport
                                 , aState
                                 , type
                                 , aPurgeFlag /* CMU-ONLY */
                     ) );
      if (aState == kWaitResponse) {
        ++theWaitResponseEntries;
        theMaxMAFMisses << theWaitResponseEntries;
      }
      theMaxMAFTargets << theMshrs.size();
      account(type, 1);
    }

    maf_iter allocPlaceholderEntry ( MemoryAddress   aBlockAddress,
                                     Transport     & aTransport )
    {
      DBG_Assert ( ! full() );
      theMshrs.insert ( MafEntry ( aBlockAddress, aTransport, kWaitResponse, kRead ) );
      ++theWaitResponseEntries;
      theMaxMAFMisses << theWaitResponseEntries;
      theMaxMAFTargets << theMshrs.size();
      account(kRead, 1);

      maf_iter iter  = theMshrs.find( boost::make_tuple( aBlockAddress, kWaitResponse ) );
      modifyState ( iter, kCompleted );

      return iter;
    }

    bool empty() const {
      return theMshrs.empty();
    }

    void reserve() {
      ++theReserve;
      DBG_Assert( theReserve + theMshrs.size() <= theSize);
    }

    void unreserve() {
      --theReserve;
      DBG_Assert( theReserve >= 0);
    }

    bool full() const {
      if (theMaxTargetsPerRequest == 0) {
        return theMshrs.size() + theReserve >= theSize;
      } else {
        return theWaitResponseEntries >= theSize || theMshrs.size() > theSize * theMaxTargetsPerRequest;
      }
    }

    maf_iter end() const {
      return theMshrs.end();
    }

    void remove(maf_iter iter) {
      eEntryType type = iter->type;
      if (iter->state == kWaitResponse) {
        --theWaitResponseEntries;
      }
      theMshrs.erase(iter);
      account(type, -1);
    }

    void modifyState( maf_iter iter, MafStates aState) {
      if (iter->state == kWaitResponse) {
        --theWaitResponseEntries;
      }
      theMshrs.modify(iter, ll::bind( &MafEntry::state, ll::_1 ) = aState);
      if (aState == kWaitResponse) {
        ++theWaitResponseEntries;
        theMaxMAFMisses << theWaitResponseEntries;
      }
    }

    /* CMU-ONLY-BLOCK-BEGIN */
    void modifyPurgeFlag( maf_iter iter, bool aPurgeFlag) {
      theMshrs.modify(iter, ll::bind( &MafEntry::isPurge, ll::_1 ) = aPurgeFlag);
    }
    /* CMU-ONLY-BLOCK-END */

    bool contains ( const MemoryAddress  & aBlockAddress ) {
      return theMshrs.count( boost::make_tuple( aBlockAddress ) ) > 0;
    }

    bool contains ( const MemoryAddress  & aBlockAddress , MafStates aState) {
      return theMshrs.count( boost::make_tuple( aBlockAddress, aState ) ) > 0;
    }

    /* CMU-ONLY-BLOCK-BEGIN */
    // find lines or pages with purges in maf entries
    bool containsLinePurge ( const MemoryAddress  & aBlockAddress ) const {
      maf_iter iter_lower = theMshrs.lower_bound( boost::make_tuple(MemoryAddress(static_cast<unsigned long long>(aBlockAddress) & ~(theBlockSize-1))) );
      if (iter_lower == theMshrs.end()) {
        return false;
      }
      maf_iter iter_upper = theMshrs.upper_bound( boost::make_tuple(MemoryAddress(static_cast<unsigned long long>(aBlockAddress) | (theBlockSize-1))) );
      for (maf_iter iter = iter_lower; iter != iter_upper; iter++) {
        if (iter->isPurge) {
          return true;
        }
      }

      return false;
    }

    bool containsPagePurge ( const MemoryAddress  & aBlockAddress ) const {
      maf_iter iter_lower = theMshrs.lower_bound( boost::make_tuple(MemoryAddress(static_cast<unsigned long long>(aBlockAddress) & ~(thePageSize-1))) );
      if (iter_lower == theMshrs.end()) {
        return false;
      }
      maf_iter iter_upper = theMshrs.upper_bound( boost::make_tuple(MemoryAddress(static_cast<unsigned long long>(aBlockAddress) | (thePageSize-1))) );
      for (maf_iter iter = iter_lower; iter != iter_upper; iter++) {
        if (iter->isPurge) {
          return true;
        }
      }

      return false;
    }

    bool containsLinePurge ( const MemoryAddress  & aBlockAddress , MafStates aState ) const {
      maf_iter iter_lower = theMshrs.lower_bound( boost::make_tuple(MemoryAddress(static_cast<unsigned long long>(aBlockAddress) & ~(theBlockSize-1))) );
      if (iter_lower == theMshrs.end()) {
        return false;
      }
      maf_iter iter_upper = theMshrs.upper_bound( boost::make_tuple(MemoryAddress(static_cast<unsigned long long>(aBlockAddress) | (theBlockSize-1))) );
      for (maf_iter iter = iter_lower; iter != iter_upper; iter++) {
        if (iter->isPurge && iter->state == aState) {
          return true;
        }
      }

      return false;
    }

    MemoryAddress getPurgeWaitResponseAddr ( const MemoryAddress  & aBlockAddress ) const {
      maf_iter iter_lower = theMshrs.lower_bound( boost::make_tuple(MemoryAddress(static_cast<unsigned long long>(aBlockAddress) & ~(thePageSize-1))) );
      if (iter_lower == theMshrs.end()) {
        return MemoryAddress(0);
      }
      maf_iter iter_upper = theMshrs.upper_bound( boost::make_tuple(MemoryAddress(static_cast<unsigned long long>(aBlockAddress) | (thePageSize-1))) );
      for (maf_iter iter = iter_lower; iter != iter_upper; iter++) {
        if (iter->isPurge && (iter->state == kWaitResponse)) {
          return iter->theBlockAddress;
        }
      }

      return MemoryAddress(0);
    }

    maf_t::iterator getBlockedPurgeMafEntry( const MemoryAddress  & aBlockAddress ) {
      maf_iter iter_lower = theMshrs.lower_bound( boost::make_tuple(MemoryAddress(static_cast<unsigned long long>(aBlockAddress) & ~(theBlockSize-1))) );
      maf_iter iter_upper = theMshrs.upper_bound( boost::make_tuple(MemoryAddress(static_cast<unsigned long long>(aBlockAddress) |  (theBlockSize-1))) );
      for (maf_iter iter = iter_lower; iter != iter_upper; iter++) {
        if (iter->isPurge && (iter->state == kWaitAddress)) {
          return iter;
        }
      }

      return theMshrs.end();
    }
    /* CMU-ONLY-BLOCK-END */

    std::pair
      < boost::intrusive_ptr<MemoryMessage>
      , boost::intrusive_ptr<TransactionTracker>
      >
    getWaitingMAFEntry( const MemoryAddress  & aBlockAddress) {
      maf_t::iterator iter = theMshrs.find( boost::make_tuple( aBlockAddress, kWaitResponse ) );
      if ( iter == theMshrs.end() ) {
        DBG_( Crit, Addr(aBlockAddress) ( << theName << " Expected to find MAF entry for " << aBlockAddress << " but found none.") );
        return std::make_pair( boost::intrusive_ptr<MemoryMessage>(0), boost::intrusive_ptr<TransactionTracker>(0) );
      }

      return std::make_pair( iter->transport[MemoryMessageTag], iter->transport[TransactionTrackerTag]);
    }

    maf_iter getWaitingMAFEntryIter ( const MemoryAddress & aBlockAddress )
    {
      return theMshrs.find( boost::make_tuple( aBlockAddress, kWaitResponse ) );
    }

    maf_iter getProbingMAFEntry( const MemoryAddress & aBlockAddress) {
      return theMshrs.find( boost::make_tuple( aBlockAddress, kWaitProbe ) );
    }

    Transport getWaitingMAFEntryTransport ( const MemoryAddress & aBlockAddress ) {
      maf_t::iterator iter = theMshrs.find( boost::make_tuple( aBlockAddress, kWaitResponse ) );
      DBG_Assert( iter != theMshrs.end() );
      return iter->transport;
    }

    Transport removeWaitingMafEntry( const MemoryAddress  & aBlockAddress ) {
      Transport ret_val;
      maf_t::iterator iter = theMshrs.find( boost::make_tuple( aBlockAddress, kWaitResponse ) );
      DBG_Assert( iter != theMshrs.end() );
      --theWaitResponseEntries;
      ret_val = iter->transport;
      eEntryType type = iter->type;
      theMshrs.erase( iter );
      account(type, -1);
      return ret_val;
    }

    maf_t::iterator getBlockedMafEntry( const MemoryAddress  & aBlockAddress ) {
      maf_t::iterator iter = theMshrs.find( boost::make_tuple( aBlockAddress, kWaitAddress ) );
      return iter;
    }

    std::list<boost::intrusive_ptr<MemoryMessage> > getAllMessages( const MemoryAddress  & aBlockAddress) {
      std::list< boost::intrusive_ptr<MemoryMessage> > ret_val;
      maf_t::iterator iter, end;
      boost::tie(iter, end) = theMshrs.equal_range( boost::make_tuple( aBlockAddress ) );
      while (iter != end) {
        ret_val.push_back( iter->transport[MemoryMessageTag] );
        ++iter;
      }
      return ret_val;
    }

    std::list<boost::intrusive_ptr<MemoryMessage> > getAllUncompletedMessages( const MemoryAddress  & aBlockAddress) {
      std::list< boost::intrusive_ptr<MemoryMessage> > ret_val;
      maf_t::iterator iter, end;
      boost::tie(iter, end) = theMshrs.equal_range( boost::make_tuple( aBlockAddress ) );
      while (iter != end) {
        if ( iter->state != kCompleted ) {
          ret_val.push_back( iter->transport[MemoryMessageTag] );
        }
        ++iter;
      }
      return ret_val;
    }

    void dump ( void )
    {
      maf_t::iterator
        iter = theMshrs.begin();

      DBG_ ( Trace, ( << theName << " MAF content dump (" << theMshrs.size() << "/" << theSize ) );

      while ( iter != theMshrs.end() ) {
        DBG_ ( Trace,
               Addr(iter->transport[MemoryMessageTag]->address())
               ( << theName 
                 << " entry[" << iter->state << ":" << iter->type << "]: "
                 <<  *iter->transport[MemoryMessageTag] ) );
        iter++;
      }

      DBG_ ( Trace, ( << theName << " MAF content finished" ) );
    }

  };  // end class MissAddressFile

  enum ProcessType
   { eProcRequest
   , eProcPrefetch
   , eProcPurge /* CMU-ONLY */
   , eProcSnoop
   , eProcBack
   , eProcMAFWakeup
   , eProcIProbe
   , eProcEviction
   , eProcNoMoreWork
   };

  // STOPPED HERE -- need FrontSideOut_Request and FrontSideOut_Reply 
  //   should be able to conservatively reserve both, then just differentiate on reply
  // Also need LocalEngine ports

  // Resources needed by a ProcessEntry.  These can be combined a bitmask
  // for group reservation/unreservation.
  enum ProcessResourceType
    {
      kResFrontSideOut         = 0x01,
      kResBackSideOut_Request  = 0x02,
      kResBackSideOut_Snoop    = 0x04,
      kResBackSideOut_Prefetch = 0x08,
      kResEvictBuffer          = 0x10,
      kResScheduledEvict       = 0x20,
      kResMaf                  = 0x40
    };

  static int theProcessSerial = 0;

  // the process queue contains entries of this type
  class ProcessEntry : public boost::counted_base {
    Transport theOrig;              // The transport that caused this process to start
    std::list<Transport> theOutputTransports; // Transports that are generated by this process (may include a morphed starting transport)
    MissAddressFile::maf_iter theMafEntry;
    ProcessType theType;
    boost::intrusive_ptr<TransactionTracker> theWakeTransaction;
    int  theRequiresData;            // Number of times to access the data array
    bool theTransmitAfterTag;        // May send messages after the tag check (before data array)
    bool theTagOutstanding;          // Must access the local tags
    bool theDuplicateTagOutstanding; // Must access the duplicate tags
    int  theReservations;            // Reservations held by this process (for sanity check)
    bool theRemoveMafEntry;          // Should the MAF entry be removed?
    int  theSerial;

  public:
    ProcessEntry(ProcessType aType)
      : theType(aType)
      , theRequiresData(0)
      , theTransmitAfterTag(false)
      , theTagOutstanding(false)
      , theDuplicateTagOutstanding(false)
      , theReservations ( 0 )
      , theRemoveMafEntry ( false )
      , theSerial ( theProcessSerial++ )
    {}

    ProcessEntry(const Transport & request, ProcessType aType)
      : theOrig(request)
      , theType(aType)
      , theRequiresData(0)
      , theTransmitAfterTag(false)
      , theTagOutstanding(false)
      , theDuplicateTagOutstanding(false)
      , theReservations(0)
      , theRemoveMafEntry ( false )
      , theSerial ( theProcessSerial++ )
    {}

    ProcessEntry(MissAddressFile::maf_iter iter, boost::intrusive_ptr<TransactionTracker> wake, ProcessType aType)
      : theOrig(iter->transport)
      , theMafEntry(iter)
      , theType(aType)
      , theWakeTransaction(wake)
      , theRequiresData(0)
      , theTransmitAfterTag(false)
      , theTagOutstanding(false)
      , theDuplicateTagOutstanding(false)
      , theReservations(0)
      , theRemoveMafEntry ( false )
      , theSerial ( theProcessSerial++ )
    {}

    ~ProcessEntry()
    {
      // Check whether we leaked any resources or not
      // Unfortunately, debugging macros don't work in this scope (for now)
      if ( theReservations != 0 ) {
        DBG_(Crit,
             Addr(theOrig[MemoryMessageTag]->address())
             ( << " Process ending with resources reserved: " << std::hex << theReservations
               << ": " << *theOrig[MemoryMessageTag]  ));
      }
    }

    Transport & transport() {
      return theOrig;
    }

    void reserve ( const int res )
    {
      if ( ( theReservations & res ) != 0 ) {
        DBG_(Crit,
             Addr(theOrig[MemoryMessageTag]->address())
             ( << " Process serial: " << serial()
               << " WARNING: cache resource leak of type: 0x" << std::hex << (theReservations & res)
               << " for " << *theOrig[MemoryMessageTag] ));
      }
      theReservations = theReservations | res;
    }

    void unreserve ( const int res )
    {
      DBG_(Iface,
           ( << " Process serial: " << serial()
             << " unreserve: " << std::hex << res
             << " have: " << theReservations
          ));
      if ( (theReservations & res) != res ) {
        DBG_(Crit,
             ( << " Process serial: " << serial()
               << " MISSING RESOURCE ON UNRESERVE: " << std::hex << res
               << " have: " << theReservations
               << " for " << *theOrig[MemoryMessageTag]
            ));
      }
      theReservations = theReservations & (~res);
    }

    int getReservations ( void )
    {
      return theReservations;
    }

    // Methods to add output messages to the process
    void enqueueOutputMessages ( Action action )
    {
      while ( !action.theOutputMessage.empty() ) {
        MemoryTransport
          trans;

        trans.set ( MemoryMessageTag, action.theOutputMessage.front() );
        trans.set ( TransactionTrackerTag, action.theOutputTracker );

        enqueueOutputTransport ( trans );

        DBG_(Trace,
             Addr(trans[MemoryMessageTag]->address())
             ( << " Enqueued message " << *trans[MemoryMessageTag] ) );

        action.theOutputMessage.pop_front();
      }
    }

    void enqueueOutputTransport ( Transport trans )
    {
      theOutputTransports.push_back ( trans );
    }

    // The original transport that generated the process can become our output message
    void enqueueOrigTransport ( void )
    {
      enqueueOutputTransport ( theOrig );
    }

    bool hasOutputTransports ( void ) {
      return !theOutputTransports.empty();
    }

    // Consumes an output message.  Must check for messages first!
    Transport getOutputTransport ( void ) {
      Transport trans = theOutputTransports.front();
      theOutputTransports.pop_front();
      return trans;
    }

    // Does the output message go towards the backside?
    // ASSUMES ALL OUTPUT MESSAGES GO IN THE SAME DIRECTION!!!
    bool outputDirectionBack()
    {
      return theOutputTransports.front()[MemoryMessageTag]->directionToBack();
    }

    ProcessType & type() {
      return theType;
    }
    boost::intrusive_ptr<TransactionTracker> & wakeTrans() {
      return theWakeTransaction;
    }
    int & requiresData() {
      return theRequiresData;
    }
    bool & transmitAfterTag() {
      return theTransmitAfterTag;
    }
    MissAddressFile::maf_iter & mafEntry() {
      return theMafEntry;
    }
    bool & removeMafEntry() {
      return theRemoveMafEntry;
    }
    bool & tagOutstanding() {
      return theTagOutstanding;
    }
    bool & duplicateTagOutstanding() {
      return theDuplicateTagOutstanding;
    }

    void consumeAction ( Action & action )
    {
      theRequiresData = action.theRequiresData;
      theTagOutstanding = theDuplicateTagOutstanding = false;
      if ( action.theRequiresTag )          theTagOutstanding = true;
      if ( action.theRequiresDuplicateTag ) theDuplicateTagOutstanding = true;
    }

    int serial ( void ) {
      return theSerial;
    }

  };  // end struct ProcessEntry

  //Since the MessageQueues and the MissAddressFile contain
  //transports, they must live in CacheController, which is the only thing
  //that knows what the MemoryTransport contains.

  class CacheController : public BaseCacheController {

    CacheInitInfo theCacheInitInfo;
    boost::scoped_ptr<BaseCacheControllerImpl> theCacheControllerImpl;

    int theBanks;
    unsigned int thePorts;

    // types and declarations for the miss address file (MAF)
    typedef boost::intrusive_ptr<MafEntry> MafEntry_p;
    MissAddressFile theMaf;

    // types and declarations for the process queue
    typedef boost::intrusive_ptr<ProcessEntry> ProcessEntry_p;
    typedef PipelineFifo<ProcessEntry_p> Pipeline;
    std::vector<Pipeline> theMAFPipeline;
    std::vector<Pipeline> theTagPipeline;
    std::vector<Pipeline> theDuplicateTagPipeline;
    std::vector<Pipeline> theDataPipeline;

    int theScheduledEvicts;
    int theFrontSideOutReserve;

    Stat::StatInstanceCounter<long long> theMafUtilization;
    Stat::StatInstanceCounter<long long> theTagUtilization;
    Stat::StatInstanceCounter<long long> theDuplicateTagUtilization;
    Stat::StatInstanceCounter<long long> theDataUtilization;

    std::vector< std::list< std::pair < MemoryAddress, boost::intrusive_ptr<TransactionTracker> > > > theWakeMAFList;
    std::vector< std::list< MissAddressFile::maf_iter > > theIProbeList;

  public:
    //These are directly manipulated by CacheImpl

    // The FrontSideIn_* queues are small queues that take messages
    // and then arbitrate across queus for each bank controller
    std::vector< MessageQueue<MemoryTransport> > FrontSideIn_Snoop;
    std::vector< MessageQueue<MemoryTransport> > FrontSideIn_Request;
    std::vector< MessageQueue<MemoryTransport> > FrontSideIn_Prefetch;
    std::vector< MessageQueue<MemoryTransport> > FrontSideIn_Purge; /* CMU-ONLY */

    std::vector< MessageQueue<MemoryTransport> > BackSideIn_Reply;
    std::vector< MessageQueue<MemoryTransport> > BackSideIn_Request;
    std::vector< MessageQueue<MemoryTransport> > FrontSideOut;
    MessageQueue<MemoryTransport> BackSideOut_Snoop;
    MessageQueue<MemoryTransport> BackSideOut_Request;
    MessageQueue<MemoryTransport> BackSideOut_Prefetch;

    std::vector< MessageQueue<MemoryTransport> > BankFrontSideIn_Snoop;
    std::vector< MessageQueue<MemoryTransport> > BankFrontSideIn_Request;
    std::vector< MessageQueue<MemoryTransport> > BankFrontSideIn_Prefetch;
    std::vector< MessageQueue<MemoryTransport> > BankFrontSideIn_Purge; /* CMU-ONLY */
    std::vector< MessageQueue<MemoryTransport> > BankBackSideIn_Reply;
    std::vector< MessageQueue<MemoryTransport> > BankBackSideIn_Request;

    // Round-robin counters to keep fairness across the
    // n input queues
    int lastSnoopQueue;
    int lastRequestQueue;
    int lastPrefetchQueue;
    int lastPurgeQueue; /* CMU-ONLY */

    int theLastTagPipeline, theLastDuplicateTagPipeline, theLastDataPipeline, theLastScheduledBank;

    long long theTraceAddress;
    unsigned long long theTraceTimeout;

    // Now that there are an array of FrontSideOut ports
    // we need to look at each one to determine whether
    // the empty()/full() conditions are met globally.
    const bool isFrontSideOutEmpty ( void ) const
    {
      for ( int i = 0; i < theCacheInitInfo.theCores; i++ )
        if ( !FrontSideOut[i].empty() )
          return false;
      return true;
    }

    const bool isFrontSideOutFull ( void ) const
    {
      for ( int i = 0; i < theCacheInitInfo.theCores; i++ )
        if ( FrontSideOut[i].full(theFrontSideOutReserve) )
          return true;
      return false;
    }

    const bool isQueueSetEmpty ( const std::vector< MessageQueue<MemoryTransport> > & queues ) const
    {
      for ( int i = 0; i < theCacheInitInfo.theCores; i++ ) {
        if ( !queues[i].empty() )
          return false;
      }

      return true;
    }

    const bool isPipelineSetEmpty ( const std::vector<Pipeline> & pipelines ) const
    {
      for ( int i = 0; i < theBanks; i++ ) {
        if ( !pipelines[i].empty() )
          return false;
      }
      return true;
    }

    const bool isQueueSetFull ( const MessageQueue<MemoryTransport> * const & queues ) const
    {

      for ( int i = 0; i < theCacheInitInfo.theCores; i++ ) {
        if ( !queues[i].full() )
          return false;
      }

      return true;
    }

    const bool isWakeMAFListEmpty() const
    {
      for ( int i = 0; i < theBanks; i++ ) {
        if ( !theWakeMAFList[i].empty() )
          return false;
      }
      return true;
    }

    const bool isIProbeListEmpty() const
    {
      for ( int i = 0; i < theBanks; i++ ) {
        if ( !theIProbeList[i].empty() )
          return false;
      }
      return true;

    }

    const unsigned int getBank ( const ProcessEntry_p entry ) const
    {
      return getBank ( addressOf ( entry ) );
    }

    const unsigned int getBank ( const MemoryTransport trans ) const
    {
      return getBank ( trans[MemoryMessageTag]->address() );
    }

    const unsigned int getBank ( const boost::intrusive_ptr<MemoryMessage> msg ) const
    {
      return getBank ( msg->address() );
    }

    const unsigned int getBank ( const MemoryAddress address ) const
    {
      return ( ( ((unsigned long long)address) / theCacheInitInfo.theBlockSize / theCacheInitInfo.theSlices) % theBanks );
    }

    unsigned int totalPipelineSize ( std::vector<Pipeline> & pipe ) const
    {
      unsigned int
        size = 0;

      for ( unsigned int i = 0; i < pipe.size(); i++ ) {
        size += pipe[i].size();
      }

      return size;
    }

    bool isQuiesced() const {
      return    theMaf.empty()
        &&      isPipelineSetEmpty ( theMAFPipeline )
        &&      isPipelineSetEmpty ( theTagPipeline )
        &&      isPipelineSetEmpty ( theDuplicateTagPipeline )
        &&      isPipelineSetEmpty ( theDataPipeline )
        &&      isWakeMAFListEmpty()
        &&      isIProbeListEmpty()
        &&      isQueueSetEmpty ( FrontSideIn_Snoop )
        &&      isQueueSetEmpty ( FrontSideIn_Request )
        &&      isQueueSetEmpty ( FrontSideIn_Prefetch )
        &&      isQueueSetEmpty ( FrontSideIn_Purge ) /* CMU-ONLY */
        &&      BackSideIn_Reply[0].empty()
        &&      BackSideIn_Request[0].empty()
        &&      isFrontSideOutEmpty()
        &&      BackSideOut_Snoop.empty()
        &&      BackSideOut_Request.empty()
        &&      BackSideOut_Prefetch.empty()
        &&      theCacheControllerImpl->isQuiesced()
        ;
    }

    void saveState(std::string const & aDirName) {
      theCacheControllerImpl->saveState( aDirName );
    }

    void loadState(std::string const & aDirName) {
      theCacheControllerImpl->loadState( aDirName );
    }

    CacheController
      ( std::string const & aName
      , int aCores
      , int aCacheSize
      , int anAssociativity
      , int aBlockSize
      , int aPageSize
      , unsigned int aBanks
      , unsigned int aPorts
      , unsigned int aTagLatency
      , unsigned int aTagIssueLatency
      , unsigned int aDuplicateTagLatency
      , unsigned int aDuplicateTagIssueLatency
      , unsigned int aDataLatency
      , unsigned int aDataIssueLatency
      , int nodeId
      , tFillLevel aCacheLevel
      , unsigned int aQueueSize
      , unsigned int aPreQueueSize
      , unsigned int aMAFSize
      , unsigned int aMAFTargetsPerRequest
      , unsigned int anEBSize
      , bool aProbeOnIfetchMiss
      , bool aDoCleanEvictions
      , eCacheType aCacheType
      , unsigned int aTraceAddress
      , bool aIsPiranhaCache
      , int aNumOfSlices
      , int aSliceNumber
      , tPlacement aPlacement 
      , bool aPrivateWithASR /* CMU-ONLY */
      , bool anAllowOffChipStreamFetch
    ) : theCacheInitInfo ( aName,
                           aCores,
                           aCacheSize,
                           anAssociativity,
                           aBlockSize,
                           aPageSize,
                           nodeId,
                           aCacheLevel,
                           anEBSize,
                           aProbeOnIfetchMiss,
                           aNumOfSlices,
                           aSliceNumber,
                           aDoCleanEvictions,
                           REPLACEMENT_LRU,
                           aIsPiranhaCache,
                           aPlacement,
                           aPrivateWithASR, /* CMU-ONLY */
                           anAllowOffChipStreamFetch
                           )
      , theCacheControllerImpl(
        BaseCacheControllerImpl::construct ( this,
                                             &theCacheInitInfo,
                                             aCacheType ))
      , theBanks ( aBanks )
      , thePorts ( aPorts )
      , theMaf(aName, aMAFSize, aMAFTargetsPerRequest, aBlockSize, aPageSize)
      , theScheduledEvicts(0)
      , theFrontSideOutReserve(0)
      , theMafUtilization( aName + "-MafUtilization")
      , theTagUtilization( aName + "-TagUtilization")
      , theDuplicateTagUtilization( aName + "-DuplicateTagUtilization")
      , theDataUtilization( aName + "-DataUtilization")
      , BackSideOut_Snoop(aQueueSize)
      , BackSideOut_Request(aQueueSize)
      , BackSideOut_Prefetch(aQueueSize)
    {
      BackSideIn_Reply.push_back ( MessageQueue<MemoryTransport> (aQueueSize) );
      BackSideIn_Request.push_back ( MessageQueue<MemoryTransport> (aQueueSize) );

      for ( int i = 0; i < theCacheInitInfo.theCores; i++ ) {
        FrontSideOut.push_back ( MessageQueue<MemoryTransport>(aQueueSize) );

        // The input queues per processor must be a minimum size for
        // compatibility with cores that must insert more than one message
        // per cycle.
        // In the CMT we only use FrontSideIn_*[0].
        FrontSideIn_Snoop.push_back    ( MessageQueue<MemoryTransport>(aPreQueueSize) );
        FrontSideIn_Request.push_back  ( MessageQueue<MemoryTransport>(aPreQueueSize) );
        FrontSideIn_Prefetch.push_back ( MessageQueue<MemoryTransport>(aPreQueueSize) );
        FrontSideIn_Purge.push_back    ( MessageQueue<MemoryTransport>(aPreQueueSize) ); /* CMU-ONLY */
      }

      boost::intrusive_ptr<Stat::StatLog2Histogram>
        mafHist          = new Stat::StatLog2Histogram ( theCacheInitInfo.theName + "-MafServer"    +  "-InterArrivalTimes" ),
        tagHist          = new Stat::StatLog2Histogram ( theCacheInitInfo.theName + "-TagServer"    +  "-InterArrivalTimes" ),
        duplicateTagHist = new Stat::StatLog2Histogram ( theCacheInitInfo.theName + "-DupTagServer" +  "-InterArrivalTimes" ),
        dataHist         = new Stat::StatLog2Histogram ( theCacheInitInfo.theName + "-DataServer"   +  "-InterArrivalTimes" );

      // Allocate per-bank resources
      for ( int i = 0; i < theBanks; i++ ) {
        theMAFPipeline.push_back ( Pipeline ( theCacheInitInfo.theName + "-MafServer",
                                              aPorts, 1, 0, mafHist ) );

        theTagPipeline.push_back ( Pipeline ( theCacheInitInfo.theName + "-TagServer",
                                              aPorts, aTagIssueLatency, aTagLatency, tagHist ) );

        theDuplicateTagPipeline.push_back ( Pipeline ( theCacheInitInfo.theName + "-DuplicateTagServer",
                                                       aPorts, aDuplicateTagIssueLatency, aDuplicateTagLatency, duplicateTagHist ) );

        theDataPipeline.push_back ( Pipeline ( theCacheInitInfo.theName + "-DataServer",
                                               aPorts, aDataIssueLatency, aDataLatency, dataHist ) );

        theWakeMAFList.push_back ( std::list< std::pair < MemoryAddress, boost::intrusive_ptr<TransactionTracker> > >() );
        theIProbeList.push_back ( std::list< MissAddressFile::maf_iter >() );

        BankFrontSideIn_Snoop.push_back    ( MessageQueue<MemoryTransport>(aQueueSize) );
        BankFrontSideIn_Request.push_back  ( MessageQueue<MemoryTransport>(aQueueSize) );
        BankFrontSideIn_Prefetch.push_back ( MessageQueue<MemoryTransport>(aQueueSize) );
        BankFrontSideIn_Purge.push_back    ( MessageQueue<MemoryTransport>(aQueueSize) ); /* CMU-ONLY */
        BankBackSideIn_Reply.push_back     ( MessageQueue<MemoryTransport>(aQueueSize) );
        BankBackSideIn_Request.push_back   ( MessageQueue<MemoryTransport>(aQueueSize) );
      }

      theTraceAddress  = (long long)aTraceAddress & ~((long long) theCacheInitInfo.theBlockSize-1);
      theTraceTimeout  = 0;

      lastSnoopQueue    = 0;
      lastRequestQueue  = 0;
      lastPrefetchQueue = 0;
      lastPurgeQueue    = 0; /* CMU-ONLY */
      theLastScheduledBank =
        theLastTagPipeline =
        theLastDuplicateTagPipeline =
        theLastDataPipeline = 0;
    }


    //These methods are used by the Impl to manipulate the MAF

    // Warning: this may return completed MAF entries with reply messages.  You may want to
    // use the function below which excludes completed messages.
    std::list<boost::intrusive_ptr<MemoryMessage> > getAllMessages( const MemoryAddress  & aBlockAddress) {
      return theMaf.getAllMessages( aBlockAddress );
    }

    std::list<boost::intrusive_ptr<MemoryMessage> > getAllUncompletedMessages( const MemoryAddress  & aBlockAddress) {
      return theMaf.getAllUncompletedMessages( aBlockAddress );
    }

    std::pair
      < boost::intrusive_ptr<MemoryMessage>
      , boost::intrusive_ptr<TransactionTracker>
      > getWaitingMAFEntry( const MemoryAddress  & aBlockAddress) {
        return theMaf.getWaitingMAFEntry( aBlockAddress );
    }

    // Reserves queue and structured entries for a process
    // in a centralized bitmask "stew"
    void reserve ( const int      aResourceStew,
                   ProcessEntry_p aProcess )
    {
      if ( aResourceStew & kResFrontSideOut ) {
        aProcess->reserve ( kResFrontSideOut );
        theFrontSideOutReserve++;
      }

      if ( aResourceStew & kResBackSideOut_Request ) {
        aProcess->reserve ( kResBackSideOut_Request );
        BackSideOut_Request.reserve();
      }

      if ( aResourceStew & kResBackSideOut_Snoop ) {
        aProcess->reserve ( kResBackSideOut_Snoop );
        BackSideOut_Snoop.reserve();
      }

      if ( aResourceStew & kResBackSideOut_Prefetch ) {
        aProcess->reserve ( kResBackSideOut_Prefetch );
        BackSideOut_Prefetch.reserve();
      }

      if ( aResourceStew & kResEvictBuffer ) {
        aProcess->reserve ( kResEvictBuffer );
        theCacheControllerImpl->reserveEvictBuffer();
      }

      if ( aResourceStew & kResScheduledEvict ) {
        aProcess->reserve ( kResScheduledEvict );
        theScheduledEvicts++;
      }

      if ( aResourceStew & kResMaf ) {
        aProcess->reserve ( kResMaf );
        theMaf.reserve();
      }
    }


    // Unreserves queue and structured entries for a process
    // in a centralized bitmask "stew"
    void unreserve ( const int      aResourceStew,
                     ProcessEntry_p aProcess )
    {
      if ( aResourceStew & kResFrontSideOut ) {
        aProcess->unreserve ( kResFrontSideOut );
        theFrontSideOutReserve--;
      }

      if ( aResourceStew & kResBackSideOut_Request ) {
        aProcess->unreserve ( kResBackSideOut_Request );
        BackSideOut_Request.unreserve();
      }

      if ( aResourceStew & kResBackSideOut_Snoop ) {
        aProcess->unreserve ( kResBackSideOut_Snoop );
        BackSideOut_Snoop.unreserve();
      }

      if ( aResourceStew & kResBackSideOut_Prefetch ) {
        aProcess->unreserve ( kResBackSideOut_Prefetch );
        BackSideOut_Prefetch.unreserve();
      }

      if ( aResourceStew & kResEvictBuffer ) {
        aProcess->unreserve ( kResEvictBuffer );
        theCacheControllerImpl->unreserveEvictBuffer();
      }

      if ( aResourceStew & kResScheduledEvict ) {
        aProcess->unreserve ( kResScheduledEvict );
        theScheduledEvicts--;
      }

      if ( aResourceStew & kResMaf ) {
        aProcess->unreserve ( kResMaf );
        theMaf.unreserve();
      }
    }

    // Unreserve that also removes the correct backsideout channel resource
    // automatically.  The process entry must encode the proper type (beware:
    // this can change after executing the process!)
    void unreserveBSO ( const int      aResourceStew,
                        ProcessEntry_p aProcess )
    {
      switch ( aProcess->type() ) {
      case eProcPrefetch:
        unreserve ( aResourceStew | kResBackSideOut_Prefetch, aProcess );
        break;
      case eProcSnoop:
      case eProcBack:
      case eProcEviction:
        unreserve ( aResourceStew | kResBackSideOut_Snoop, aProcess );
        break;
      case eProcRequest:
      case eProcPurge: /* CMU-ONLY */
      case eProcIProbe:
      case eProcMAFWakeup:
        unreserve ( aResourceStew | kResBackSideOut_Request, aProcess );
        break;
      default:
        DBG_Assert ( false,
                     ( << theCacheInitInfo.theName
                       << " Cannot unreserve resources for processes of type: "
                       << aProcess->type() ) );
      }
    }

    // Enqueue requests in the appropriate tag pipelines
    void enqueueTagPipeline ( Action         action,
                              ProcessEntry_p aProcess )
    {
      DBG_Assert ( action.theRequiresTag > 0 || action.theRequiresDuplicateTag > 0 );

      if ( action.theRequiresTag ) {
        theTagPipeline[getBank(aProcess)].enqueue ( aProcess,
                                                    action.theRequiresTag );
      }

      if ( action.theRequiresDuplicateTag ) {
        theDuplicateTagPipeline[getBank(aProcess)].enqueue ( aProcess,
                                                             action.theRequiresDuplicateTag );
      }
    }

    //Process all pending stuff in the cache.  This is called once each cycle.
    //It iterates over all the message and process queues, moving things along
    //if they are ready to go and there is no back pressure.  The order of
    //operations in this function is important - it is designed to make sure
    //a request can be received and processed in a single cycle for L1 cache
    //hits
    void processMessages() {
      FLEXUS_PROFILE();
      DBG_(VVerb, ( << theCacheInitInfo.theName << " Process messages" ) );

      {
        // Give a dummy variable to the backside in, which has one queue
        int i = 0;
        doNewRequests ( BackSideIn_Reply, BankBackSideIn_Reply, 1, i );
        doNewRequests ( BackSideIn_Request, BankBackSideIn_Request, 1, i );
      }
      doNewRequests ( FrontSideIn_Snoop,    BankFrontSideIn_Snoop,    theCacheInitInfo.theCores, lastSnoopQueue    );
      doNewRequests ( FrontSideIn_Request,  BankFrontSideIn_Request,  theCacheInitInfo.theCores, lastRequestQueue  );
      doNewRequests ( FrontSideIn_Prefetch, BankFrontSideIn_Prefetch, theCacheInitInfo.theCores, lastPrefetchQueue );
      doNewRequests ( FrontSideIn_Purge,    BankFrontSideIn_Purge,    theCacheInitInfo.theCores, lastPurgeQueue ); /* CMU-ONLY */

      theMafUtilization << std::make_pair( totalPipelineSize ( theMAFPipeline ), 1 );
      theTagUtilization << std::make_pair( totalPipelineSize ( theTagPipeline ), 1 );
      theDuplicateTagUtilization << std::make_pair( totalPipelineSize ( theDuplicateTagPipeline ), 1);
      theDataUtilization << std::make_pair( totalPipelineSize ( theDataPipeline ), 1 );

      // End address tracing?
      if ( theTraceTimeout > 0 &&
           theTraceTimeout < theFlexus->cycleCount() ) {
        theTraceTimeout = 0;
        theFlexus->setDebug ( "dev" );
      }

      //Insert new processes into maf pipeline.
      scheduleNewProcesses();

      //Drain maf pipeline
      for ( int i = 0; i < theBanks; i++ ) {
        while ( theMAFPipeline[i].ready() && theTagPipeline[i].serverAvail() && theDuplicateTagPipeline[i].serverAvail()) {
          ProcessEntry_p process = theMAFPipeline[i].dequeue();
          switch ( process->type() ) {
          case eProcRequest:
          case eProcPrefetch:
          case eProcPurge: /* CMU-ONLY */
            runRequestProcess( process );
            break;
          case eProcSnoop:
            runSnoopProcess( process );
            break;
          case eProcBack:
            runBackProcess( process );
            break;
          case eProcMAFWakeup:
            runWakeMafProcess( process );
            break;
          case eProcIProbe:
            runIProbeProcess( process );
            break;
          case eProcEviction:
            runEvictProcess( process );
            break;
          default:
            DBG_Assert(false);
          }
        }
      }

      // Drain the remaining pipelines (CMP or non-CMP)
      if ( !theCacheInitInfo.theIsPiranhaCache )
        advancePipelines();
      else
        advancePiranhaPipelines();
    }

    // Schedule the Tag and Data pipelines for a non-CMP cache
    void advancePipelines ( void )
    {
      int
        i,
        bankCount;

      i = theLastTagPipeline;
      theLastTagPipeline = ( theLastTagPipeline + 1 ) % theBanks;
      for ( bankCount = 0; bankCount < theBanks; bankCount++ ) {
        //Drain tag pipeline
        while ( theTagPipeline[i].ready() ) {
          ProcessEntry_p process = theTagPipeline[i].peek();
          if (process->requiresData()) {
            if (theDataPipeline[i].serverAvail()) {
              if (process->transmitAfterTag()) {
                doTransmitProcess( process );
              }
              theTagPipeline[i].dequeue();
              theDataPipeline[i].enqueue( process );
            } else {
              theTagPipeline[i].stall();
              break;
            }
          } else {
            theTagPipeline[i].dequeue();
            doTransmitProcess( process );
          }
        }
        i = ( i + 1 ) % theBanks;
      }


      //Drain data pipeline
      i = theLastDataPipeline;
      theLastDataPipeline = ( theLastDataPipeline + 1 ) % theBanks;
      for ( bankCount = 0; bankCount < theBanks; bankCount++ ) {
        while ( theDataPipeline[i].ready() ) {
          ProcessEntry_p process = theDataPipeline[i].dequeue();
          doTransmitProcess( process );
        }
        i = ( i + 1 ) % theBanks;
      }

    }

    // Schedule the local and duplicate tag pipelines and data pipelines
    // for the CMP cache
    void advancePiranhaPipelines ( void )
    {
      // The CMP pipelines drain with L1 and L2 tags in order, as necessary
      // then data, if any, and finally an access to the MAF.
      //
      // The tag arrays must both be completed before continuing.

      int
        i,
        bankCount;


      // Drain the main tag pipeline
      i = theLastTagPipeline;
      theLastTagPipeline = ( theLastTagPipeline + 1 ) % theBanks;
      for ( bankCount = 0; bankCount < theBanks; bankCount++ ) {
        while ( theTagPipeline[i].ready() ) {
          ProcessEntry_p process = theTagPipeline[i].peek();

          process->tagOutstanding() = false;
          if ( process->requiresData() && !process->duplicateTagOutstanding() ) {
            if ( theDataPipeline[i].serverAvail() ) {
              if ( process->transmitAfterTag() ) {
                doTransmitProcess ( process );
              }
              theTagPipeline[i].dequeue();
              theDataPipeline[i].enqueue( process, process->requiresData() );
            } else {
              theTagPipeline[i].stall();
              break;
            }
          } else {
            theTagPipeline[i].dequeue();
            if ( !process->duplicateTagOutstanding() )
              doTransmitProcess ( process );
          }
        }
        i = ( i + 1 ) % theBanks;
      }

      // Drain the duplicate tag pipeline
      i = theLastDuplicateTagPipeline;
      theLastDuplicateTagPipeline = ( theLastDuplicateTagPipeline + 1 ) % theBanks;
      for ( bankCount = 0; bankCount < theBanks; bankCount++ ) {
        while ( theDuplicateTagPipeline[i].ready() ) {
          ProcessEntry_p process = theDuplicateTagPipeline[i].peek();
          process->duplicateTagOutstanding() = false;
          if ( process->requiresData() && !process->tagOutstanding() ) {

            if ( theDataPipeline[i].serverAvail() ) {
              if ( process->transmitAfterTag() ) {
                doTransmitProcess ( process );
              }
              theDuplicateTagPipeline[i].dequeue();
              theDataPipeline[i].enqueue( process, process->requiresData() );
            } else {
              theDuplicateTagPipeline[i].stall();
              break;
            }
          } else {
            theDuplicateTagPipeline[i].dequeue();
            if ( !process->tagOutstanding() )
              doTransmitProcess ( process );
          }
        }
        i = ( i + 1 ) % theBanks;
      }

      // Drain the data pipeline
      i = theLastDataPipeline;
      theLastDataPipeline = ( theLastDataPipeline + 1 ) % theBanks;
      for ( bankCount = 0; bankCount < theBanks; bankCount++ ) {
        while ( theDataPipeline[i].ready() ) {
          ProcessEntry_p process = theDataPipeline[i].dequeue();
          doTransmitProcess( process );
        }
        i = ( i + 1 ) % theBanks;
      }

    }

    //Insert new processes into maf pipeline.  We can return once we are out
    //of ready MAF servers
    void scheduleNewProcesses() {

      int
        i,
        bankCount;

      for ( i = theLastScheduledBank, bankCount = 0;
            bankCount < theBanks;
            i = ( i + 1 ) % theBanks, bankCount++ ) {

        // for resource debugging
        DBG_(VVerb,
             ( << theCacheInitInfo.theName
               << "-b[" << i << "]"

               << " MafSrv="  << theMAFPipeline[i].serverAvail()
               << " Maf.f="   << theMaf.full()

               << " FSI_S.e=" << BankFrontSideIn_Snoop[i].empty()
               << " FSI_R.e=" << BankFrontSideIn_Request[i].empty()
               << " FSI_P.e=" << BankFrontSideIn_Prefetch[i].empty()
               << " FSI_G.e=" << BankFrontSideIn_Purge[i].empty() /* CMU-ONLY */
               << " BSI_L.e=" << BankBackSideIn_Reply[i].empty()
               << " BSI_R.e=" << BankBackSideIn_Request[i].empty()

               << " FSO.f="   << isFrontSideOutFull()
               << " BSO_R.f=" << BackSideOut_Request.full()
               << " BSO_S.f=" << BackSideOut_Snoop.full()
               << " BSO_P.f=" << BackSideOut_Prefetch.full()

               << " Wake.e="  << theWakeMAFList[i].empty()
               << " IPL.e="   << theIProbeList[i].empty()
               << " evBlk="   << theCacheControllerImpl->evictableBlockExists(theScheduledEvicts)
               << " EvBuf.f=" << theCacheControllerImpl->fullEvictBuffer()

               << " FSI_S.ts>FSI_G.ts="
               << (!BankFrontSideIn_Snoop[i].empty() ? BankFrontSideIn_Snoop[i].headTimestamp() : -1)
               << "<"
               << (!BankFrontSideIn_Purge[i].empty() ? BankFrontSideIn_Purge[i].headTimestamp() : -1) /* CMU-ONLY */

               << " FSI_S.ts>FSI_R.ts="
               << (!BankFrontSideIn_Snoop[i].empty() ? BankFrontSideIn_Snoop[i].headTimestamp() : -1)
               << "<"
               << (!BankFrontSideIn_Request[i].empty() ? BankFrontSideIn_Request[i].headTimestamp() : -1)

               << " FSI_S.ts>FSI_P.ts="
               << (!BankFrontSideIn_Snoop[i].empty() ? BankFrontSideIn_Snoop[i].headTimestamp() : -1)
               << "<"
               << (!BankFrontSideIn_Prefetch[i].empty() ? BankFrontSideIn_Prefetch[i].headTimestamp() : -1)
            ));

        //First,  MAF entries that woke up are allocated into the MAF pipeline
        //A woken MAF process must reserve the same things as a request process
        //except that it does not need a MAF entry.
        while (      theMAFPipeline[i].serverAvail()
                && ! theWakeMAFList[i].empty()
                && ! BackSideOut_Request.full()
                && ! isFrontSideOutFull()
              )
        {
          //get an address to wake up
          std::pair < MemoryAddress, boost::intrusive_ptr<TransactionTracker> > wake_entry = theWakeMAFList[i].front();
          theWakeMAFList[i].pop_front();

          MissAddressFile::maf_iter iter;

          //first, wake up a blocked purge                                     /* CMU-ONLY */
          iter  = theMaf.getBlockedPurgeMafEntry(wake_entry.first);            /* CMU-ONLY */
          if (iter == theMaf.end()) {                                          /* CMU-ONLY */
            //if there is no blocked purge, wake up a blocked request          /* CMU-ONLY */
            iter = theMaf.getBlockedMafEntry(wake_entry.first);
          }                                                                    /* CMU-ONLY */
          //Purges wake up processes waiting on any address within the page    /* CMU-ONLY */
          if (iter == theMaf.end()) {
            continue;
          }

          // (allegro scherzando ma non toppo) Wake up, little Suzy, wake up!
          theMaf.modifyState(iter, kWaking);

          // Should this really be here?  Livelock is possible!
          //if (theMaf.contains( wake_entry.first, kWaitAddress ) ) {
          //More MAF entries to wake
          //theWakeMAFList.push_back(wake_entry);
          //}

          DBG_(Trace, Addr(iter->transport[MemoryMessageTag]->address()) ( << theCacheInitInfo.theName << " schedule WakeMAF " << *iter->transport[MemoryMessageTag] ) );

          ProcessEntry_p aProcess = new ProcessEntry ( iter, wake_entry.second, eProcMAFWakeup );
          reserve ( kResFrontSideOut | kResBackSideOut_Request, aProcess );
          theMAFPipeline[i].enqueue ( aProcess );
        }

        //Next, wake up IProbe MAF entries
        //A woken IProbe process must reserve a FrontSideOut and a BackSideOut_Request.
        while (       theMAFPipeline[i].serverAvail()
                && ! theIProbeList[i].empty()
                && ! BackSideOut_Request.full()
                && ! isFrontSideOutFull()
              )
       {
          MissAddressFile::maf_iter iter = theIProbeList[i].front();
          theIProbeList[i].pop_front();

          DBG_(Trace, Addr(iter->transport[MemoryMessageTag]->address()) ( << theCacheInitInfo.theName << " schedule IProbe " << *iter->transport[MemoryMessageTag] ) );

          ProcessEntry_p aProcess = new ProcessEntry ( iter, iter->transport[TransactionTrackerTag], eProcIProbe );
          reserve ( kResFrontSideOut | kResBackSideOut_Request, aProcess );
          theMAFPipeline[i].enqueue( aProcess );
        }

        //Next, allocate eviction processes if the evict buffer could become
        //fully reserved (i.e., < # ports entries free).  Evict buffer entries
        //reserve a BackSideOut_Snoop buffer, and increment theScheduledEvicts.

        // FIXME: allocates resources for any bank to process the eviction, instead of the
        // specific bank pipe which owns the victim. On average, this still produces
        // fair resource consumption.
        while ( theMAFPipeline[i].serverAvail()
                && theCacheControllerImpl->evictableBlockExists(theScheduledEvicts)
                && (theCacheControllerImpl->freeEvictBuffer() + theScheduledEvicts < thePorts)
                && !BackSideOut_Snoop.full()
                ) {
          DBG_(Trace, ( << theCacheInitInfo.theName << " schedule Evict" ) );

          ProcessEntry_p aProcess = new ProcessEntry ( eProcEviction );
          reserve ( kResBackSideOut_Snoop | kResScheduledEvict, aProcess );
          theMAFPipeline[i].enqueue( aProcess );
        }

        // STOPPED HERE: the code below will deadlock if the Maf is full, b/c there's
        //   no way to get replies up to the Maf entries... so we split the back channel
        //   into replies and requests?

        //Next, allocate back reply processes.  Back processes reserve a FrontSideOut
        //buffer and an evict buffer entry.
        while (      theMAFPipeline[i].serverAvail()
                && ! BankBackSideIn_Reply[i].empty()
                && ! theCacheControllerImpl->fullEvictBuffer()
                && ! isFrontSideOutFull()
                && ! BackSideOut_Snoop.full()
                     //fixme
                && ( ! theCacheInitInfo.theIsPiranhaCache
                     || ( theCacheInitInfo.theIsPiranhaCache
                          && (   ! BankBackSideIn_Reply[i].peek()[ MemoryMessageTag ]->isExtReqType()
                                 || (BankBackSideIn_Reply[i].peek()[ MemoryMessageTag ]->isExtReqType() && ! theMaf.full()))))
              )
        {
          MemoryTransport transport(BankBackSideIn_Reply[i].dequeue());
          DBG_(Trace, Addr(transport[MemoryMessageTag]->address()) ( << theCacheInitInfo.theName << " schedule reply Back " << *transport[MemoryMessageTag] ) );

          ProcessEntry_p aProcess = new ProcessEntry ( transport, eProcBack );
          reserve ( kResBackSideOut_Snoop | kResFrontSideOut | kResEvictBuffer , aProcess );
          if ( theCacheInitInfo.theIsPiranhaCache && transport[ MemoryMessageTag ]->isExtReqType() ) {
            reserve ( kResMaf, aProcess );
          }
          theMAFPipeline[i].enqueue( aProcess );
        }

        /* CMU-ONLY-BLOCK-BEGIN */
        //Next, allocate purge processes
        //Purge processes reserve a MAF entry, a BackSideOut_Request buffer, and a FrontSideOut buffer.
        while (      theMAFPipeline[i].serverAvail()
                && ! BankFrontSideIn_Purge[i].empty()
                && ! BackSideOut_Request.full()
                && ! isFrontSideOutFull()
                && ! theMaf.full()
                && ( BankFrontSideIn_Snoop[i].empty() ||
                     BankFrontSideIn_Snoop[i].headTimestamp() > BankFrontSideIn_Purge[i].headTimestamp() )
              )
        {
          MemoryTransport transport(BankFrontSideIn_Purge[i].dequeue());
          DBG_(Trace, Addr(transport[MemoryMessageTag]->address()) ( << theCacheInitInfo.theName << " schedule Purge " << *transport[MemoryMessageTag] ) );

          ProcessEntry_p aProcess = new ProcessEntry ( transport, eProcPurge );
          reserve ( kResFrontSideOut | kResBackSideOut_Request | kResMaf, aProcess );
          theMAFPipeline[i].enqueue( aProcess );
        }
        /* CMU-ONLY-BLOCK-END */

        //Next, allocate back request processes.  Back request processes reserve a FrontSideOut
        //buffer and an evict buffer entry AND A MAF.
        while (      theMAFPipeline[i].serverAvail()
                && ! BankBackSideIn_Request[i].empty()
                && ! theCacheControllerImpl->fullEvictBuffer()
                && ! isFrontSideOutFull()
                && ! BackSideOut_Snoop.full()
                && ! theMaf.full()
              )
        {
          MemoryTransport transport(BankBackSideIn_Request[i].dequeue());
          DBG_(Trace, Addr(transport[MemoryMessageTag]->address()) ( << theCacheInitInfo.theName << " schedule request Back " << *transport[MemoryMessageTag] ) );

          ProcessEntry_p aProcess = new ProcessEntry ( transport, eProcBack );
          reserve ( kResBackSideOut_Snoop | kResFrontSideOut | kResEvictBuffer | kResMaf, aProcess );
          theMAFPipeline[i].enqueue( aProcess );
        }

        //Next, allocate request processes if they have a newer timestamp than
        //the newest waiting snoop process.  Request processes reserve a MAF
        //entry, a BackSideOut_Request buffer, and a FrontSideOut buffer.
        while (      theMAFPipeline[i].serverAvail()
                && ! BankFrontSideIn_Request[i].empty()
                && ! BackSideOut_Request.full()
                && ! isFrontSideOutFull()
                && ! theMaf.full()
                && ( BankFrontSideIn_Snoop[i].empty() ||
                     BankFrontSideIn_Snoop[i].headTimestamp() > BankFrontSideIn_Request[i].headTimestamp() )
              )
        {
          MemoryTransport transport(BankFrontSideIn_Request[i].dequeue());
          DBG_(Trace, Addr(transport[MemoryMessageTag]->address()) ( << theCacheInitInfo.theName << " schedule Request " << *transport[MemoryMessageTag] ) );

          ProcessEntry_p aProcess = new ProcessEntry ( transport, eProcRequest );
          reserve ( kResFrontSideOut | kResBackSideOut_Request | kResMaf, aProcess );
          theMAFPipeline[i].enqueue( aProcess );
        }

        //Next, allocate prefetch processes if there are no waiting request
        //processes and the prefetches have a newer timestamp than
        //the newest waiting snoop process.  Prefetch processes reserve a MAF
        //entry, a BackSideOut_Prefetch buffer, and a FrontSideOut buffer.
        while (      theMAFPipeline[i].serverAvail()
                && ! BankFrontSideIn_Prefetch[i].empty()
                && ! BackSideOut_Prefetch.full()
                && ! isFrontSideOutFull()
                && ! theMaf.full()
                && ( BankFrontSideIn_Snoop[i].empty() ||
                     BankFrontSideIn_Snoop[i].headTimestamp() > BankFrontSideIn_Prefetch[i].headTimestamp() )
              )
        {
          MemoryTransport transport(BankFrontSideIn_Prefetch[i].dequeue());
          DBG_(Trace, Addr(transport[MemoryMessageTag]->address()) ( << theCacheInitInfo.theName << " schedule Prefetch " << *transport[MemoryMessageTag] ) );

          ProcessEntry_p aProcess = new ProcessEntry ( transport, eProcPrefetch );
          reserve ( kResFrontSideOut | kResBackSideOut_Prefetch | kResMaf, aProcess );
          theMAFPipeline[i].enqueue( aProcess );
        }

        //Next, allocate any snoop processes.
        while (       theMAFPipeline[i].serverAvail()
                 && ! BankFrontSideIn_Snoop[i].empty()
                 && ! BackSideOut_Snoop.full()
                 && ! theCacheControllerImpl->fullEvictBuffer()
                 && ! isFrontSideOutFull()
               )
        {
          MemoryTransport transport(BankFrontSideIn_Snoop[i].dequeue());
          DBG_(Trace, Addr(transport[MemoryMessageTag]->address()) ( << theCacheInitInfo.theName << " schedule Snoop " << *transport[MemoryMessageTag] ) );

          // JCS - In the CMP, snoop reuests may also require the frontsideout
          ProcessEntry_p aProcess = new ProcessEntry ( transport, eProcSnoop );
          reserve ( kResBackSideOut_Snoop | kResEvictBuffer | kResFrontSideOut, aProcess );
          theMAFPipeline[i].enqueue( aProcess );
        }
      }

      // Make the last lowest-priority bank the highest for the next time around
      theLastScheduledBank = ( theLastScheduledBank + theBanks - 1 ) % theBanks;
    }

    //For convenience.  Record the wakeup address, and do wakeup.
    void enqueueWakeMaf(MemoryAddress const & anAddress, boost::intrusive_ptr<TransactionTracker> aWakeTransaction) {
      DBG_(Trace, Addr(anAddress) ( << theCacheInitInfo.theName << " enqueueWakeMaf " << anAddress ) );
      theWakeMAFList[getBank(anAddress)].push_back( std::make_pair(anAddress, aWakeTransaction) );
    }

    void doNewRequests(std::vector< MessageQueue<MemoryTransport> > & aMessageQueue,
                       std::vector< MessageQueue<MemoryTransport> > & aBankQueue,
                       const int numMsgQueues,
                       int & lastQueue )
    {
      int
        i,
        queueCount;
      bool
        sentMessages = true;

      while ( sentMessages ) {

        sentMessages = false;

        for ( i = lastQueue, queueCount = 0;
              queueCount < numMsgQueues;
              queueCount++, i=(i+1)%numMsgQueues ) {

          if ( !aMessageQueue[i].empty() ) {

            unsigned int bank = getBank ( aMessageQueue[i].peek() );

            if (  ( theTraceAddress > 0 ) && ( aMessageQueue[i].peek()[MemoryMessageTag]->address() & ~((long long) theCacheInitInfo.theBlockSize-1)) == theTraceAddress ) {
              if ( theTraceTimeout <= 0 ) {
                theTraceTimeout = theFlexus->cycleCount() + 1000;
                theFlexus->setDebug ( "iface" );
              }
            }

            if ( !aBankQueue[bank].full() ) {
              MemoryTransport
                trans ( aMessageQueue[i].dequeue() );

              DBG_ ( Trace,
                     Addr(trans[MemoryMessageTag]->address())
                     ( << theCacheInitInfo.theName << " scheduling request to bank " << bank << ": " << *trans[MemoryMessageTag] << " " << theCacheInitInfo.theCacheLevel ) );
              aBankQueue[bank].enqueue ( trans );
              sentMessages = true;
            } else {
              DBG_ ( Trace,
                     Addr(aMessageQueue[i].peek()[MemoryMessageTag]->address())
                     ( << theCacheInitInfo.theName << " bank[" << bank << "] conflict for[" << i << "]: "
                              << *aMessageQueue[i].peek()[MemoryMessageTag] ) );
            }
          }
        }
      }

      lastQueue = ( lastQueue + numMsgQueues - 1 ) % numMsgQueues;
    }

    void runEvictProcess(ProcessEntry_p aProcess ) {
      FLEXUS_PROFILE();
      DBG_(Trace, ( << theCacheInitInfo.theName << " runEvictProcess" ) );

      MemoryTransport transport;
      std::pair<boost::intrusive_ptr<MemoryMessage>, boost::intrusive_ptr<TransactionTracker> > msg_pair;
      Action action = theCacheControllerImpl->doEviction();
      DBG_Assert ( !action.theOutputMessage.empty() , ( << theCacheInitInfo.theName << " action has no output messages" ) );
      transport.set(MemoryMessageTag, action.theOutputMessage.front() );
      transport.set(TransactionTrackerTag, action.theOutputTracker);
      action.theOutputMessage.pop_front();

      unreserve ( kResBackSideOut_Snoop | kResScheduledEvict, aProcess );

      // JCS - Apparently evictions are free?!?  No pipelines used?!
      sendBack_Snoop( transport );
    }

    void runRequestProcess(ProcessEntry_p aProcess) {
      FLEXUS_PROFILE();
      DBG_Assert(   aProcess->type() == eProcRequest
                 || aProcess->type() == eProcPrefetch 
                 || aProcess->type() == eProcPurge      /* CMU-ONLY */
                );
      DBG_(Trace, Addr(aProcess->transport()[MemoryMessageTag]->address()) ( << theCacheInitInfo.theName << " runRequestProcess " << *aProcess->transport()[MemoryMessageTag] ) );

      bool has_maf_entry = theMaf.contains( addressOf(aProcess) );

      //We call handleRequestMessage even if there is a maf entry outstanding,
      //because PrefetchReadRequests can be handled even if there is an address
      //conflict (they produce PrefetchReadRedudant responses)


      Action action ( kNoAction, aProcess->transport()[TransactionTrackerTag] );

      /* CMU-ONLY-BLOCK-BEGIN */
      if (aProcess->transport()[MemoryMessageTag]->type() == MemoryMessage::FinalizePurge) {
        const unsigned long long aPageAddr = addressOf(aProcess) & ~(theCacheInitInfo.thePageSize-1);
        DBG_ ( VVerb,
               Addr(aPageAddr)
               ( << theCacheInitInfo.theName << " Finalize purge " << *aProcess->transport()[MemoryMessageTag] ) );
        for (unsigned long long addr = aPageAddr; addr < aPageAddr + theCacheInitInfo.thePageSize; addr += theCacheInitInfo.theBlockSize) {
          enqueueWakeMaf ( MemoryAddress(addr), trackerOf ( aProcess ) );
        }
      }
      else
      /* CMU-ONLY-BLOCK-END */
      {
        // The CMP can get into incoherent state if multiple outstanding requests are allowed for
        // general requests.  We're being conservative and ignoring redundant prefetches for now.
        // If there is a purge for this page, wait                                                         /* CMU-ONLY */
        if ( (theCacheInitInfo.theIsPiranhaCache && has_maf_entry)
             || (aProcess->type() == eProcPurge && theMaf.contains( addressOf(aProcess), kWaitResponse ))  /* CMU-ONLY */
             || (aProcess->type() != eProcPurge && theMaf.containsPagePurge( addressOf(aProcess) ))        /* CMU-ONLY */
           )
        {
          DBG_ ( Trace,
                 Addr(aProcess->transport()[MemoryMessageTag]->address())
                 ( << theCacheInitInfo.theName << " CMP-Specific request must wait for outstanding requests to complete: "
                   << *aProcess->transport()[MemoryMessageTag]
               ) );
          theMaf.dump();
          action.theAction = kInsertMAF_WaitAddress;
        } else {
          action = theCacheControllerImpl->handleRequestMessage( aProcess->transport()[MemoryMessageTag]
                                                                 , aProcess->transport()[TransactionTrackerTag]
                                                                 , has_maf_entry
                                                               );
        }
      }

      aProcess->consumeAction ( action );

      DBG_ ( Trace,
             Addr(aProcess->transport()[MemoryMessageTag]->address())
             ( << theCacheInitInfo.theName << " Action for " << *aProcess->transport()[MemoryMessageTag] << " is: " << action.theAction ) );

      // reserve ( kResFrontSideOut | kResBackSideOut_Request | kResMaf, aProcess );
      switch(action.theAction) {
        case kSend:
          DBG_Assert( aProcess->type() == eProcRequest 
                      || aProcess->type() == eProcPrefetch  
                      || aProcess->type() == eProcPurge     /* CMU-ONLY */
                    );
          //This is used for PrefetchReadRedundant and other cases which should
          //not wake MAF entries

          aProcess->enqueueOrigTransport();

          if ( aProcess->transport()[MemoryMessageTag]->directionToFront() ) {
            unreserveBSO ( kResMaf , aProcess );  //FrontSideOut still reserved
          } else {
            unreserve ( kResMaf | kResFrontSideOut , aProcess ); //BackSideOut_* still reserved
          }

          enqueueTagPipeline ( action, aProcess );
          break;

        case kInsertMAF_WaitAddress:
          DBG_Assert( aProcess->type() == eProcRequest 
                      || aProcess->type() == eProcPrefetch 
                      || aProcess->type() == eProcPurge     /* CMU-ONLY */
                    );

          //Blocked due to address conflict.
          unreserveBSO ( kResMaf | kResFrontSideOut , aProcess );
          theMaf.allocEntry( addressOf(aProcess)
                             , aProcess->transport()
                             , kWaitAddress
                             , aProcess->transport()[MemoryMessageTag]->isPurge()  /* CMU-ONLY */
                           );
          theMaf.dump();
          break;

        case kInsertMAF_WaitResponse: {
          aProcess->enqueueOutputMessages ( action );

          if ( aProcess->outputDirectionBack() ) {
            //Cache needs help from the outside to fill this response
            unreserve ( kResMaf | kResFrontSideOut , aProcess );            //BackSideOut_* still reserved
          } else {
            // CMP needs coherence to fill this response
            unreserveBSO ( kResMaf, aProcess );            // FrontSideOut still reserved
          }
          theMaf.allocEntry( addressOf(aProcess)
                             , aProcess->transport()
                             , kWaitResponse
                             , aProcess->transport()[MemoryMessageTag]->isPurge()   /* CMU-ONLY */
                           );

          enqueueTagPipeline ( action, aProcess );
          break;
        }
        case kInsertMAF_WaitProbe: {
          //Cache needs to probe hierarchy above to determine what to do
          unreserveBSO ( kResMaf , aProcess );          //FrontSideOut still reserved

          theMaf.allocEntry(addressOf(aProcess), aProcess->transport(), kWaitProbe);
          aProcess->enqueueOutputMessages ( action );
          enqueueTagPipeline ( action, aProcess );
          break;
        }
        case kReplyAndRemoveMAF:
          {
            //Request was satisfied.  It never got a MAF entry, so no need to
            //remove.

            aProcess->enqueueOrigTransport();
            unreserveBSO ( kResMaf , aProcess );            //FrontSideOut still reserved

            if ( theCacheInitInfo.theIsPiranhaCache ) {
              // In the CMP, we need to block other requests from accessing this address
              // until we send the reply, so allocate a complete MAF entry and set it
              // to be complete
              aProcess->removeMafEntry() = true;
              aProcess->mafEntry() = theMaf.allocPlaceholderEntry ( addressOf ( aProcess ),
                                                                    aProcess->transport() );
            }

            enqueueTagPipeline ( action, aProcess );
            break;
          }
        case kNoAction:
        /* CMU-ONLY-BLOCK-BEGIN */
        {
          DBG_Assert (aProcess->transport()[MemoryMessageTag]->type() == MemoryMessage::FinalizePurge);
          unreserveBSO ( kResMaf | kResFrontSideOut , aProcess );
          aProcess->type() = eProcNoMoreWork;
          enqueueTagPipeline ( action, aProcess );
          break;
        }
        /* CMU-ONLY-BLOCK-END */
        case kReplyAndRemoveResponseMAF:
          //Not possible for requests
          DBG_Assert( false, ( << theCacheInitInfo.theName
                               << " Unexpected result from runRequestProcess: " << action
                               << " for msg: " <<  *aProcess->transport()[MemoryMessageTag]) );
      }
    }

    //If there is a MAF wakeup in progress, continue it.
    void runWakeMafProcess(ProcessEntry_p aProcess) {
      FLEXUS_PROFILE();
      DBG_Assert(aProcess->type() == eProcMAFWakeup );
      DBG_(Trace,
           Addr(aProcess->mafEntry()->transport[MemoryMessageTag]->address())
           ( << theCacheInitInfo.theName << " runWakeMafProcess " << *aProcess->mafEntry()->transport[MemoryMessageTag] ) );

      // If this is a request and there is a purge for this page, wait          /* CMU-ONLY */
      // If this is a purge and there is a request waiting for a response, wait /* CMU-ONLY */
      Action action ( kInsertMAF_WaitAddress, aProcess->mafEntry()->transport[TransactionTrackerTag] );
      /* CMU-ONLY-BLOCK-BEGIN */
      if (    (    aProcess->mafEntry()->transport[MemoryMessageTag]->isPurge()
                && theMaf.contains( aProcess->mafEntry()->transport[MemoryMessageTag]->address(), kWaitResponse ))
           || (    !aProcess->mafEntry()->transport[MemoryMessageTag]->isPurge()
                && theMaf.containsPagePurge( aProcess->mafEntry()->transport[MemoryMessageTag]->address() ))
         )
      {
        DBG_ ( Trace,
               Addr( aProcess->mafEntry()->transport[MemoryMessageTag]->address() )
               ( << theCacheInitInfo.theName << " Request must wait for outstanding requests to complete: "
                 << *aProcess->mafEntry()->transport[MemoryMessageTag]
             ));
        theMaf.dump();
      }
      else
      /* CMU-ONLY-BLOCK-END */
      {
        action = theCacheControllerImpl->wakeMaf( aProcess->mafEntry()->transport[MemoryMessageTag],
                                                  aProcess->mafEntry()->transport[TransactionTrackerTag],
                                                  aProcess->wakeTrans()
                                                );
      }

      aProcess->consumeAction ( action );
      switch(action.theAction) {

        case kInsertMAF_WaitResponse: {
          aProcess->enqueueOutputMessages ( action );

          if ( aProcess->outputDirectionBack() ) {
            //Cache needs help from the outside to fill this response
            unreserve ( kResFrontSideOut, aProcess );            //BackSideOut_* still reserved
          } else {
            unreserveBSO ( 0, aProcess );            // FrontSideOut still reserved
          }
          theMaf.modifyState(aProcess->mafEntry(), kWaitResponse);
          enqueueTagPipeline ( action, aProcess );
          break;
        }
        case kInsertMAF_WaitProbe: {
          //Cache needs to probe hierarchy above to determine what to do
          unreserveBSO ( 0, aProcess );          //FrontSideOut still reserved

          theMaf.modifyState(aProcess->mafEntry(), kWaitProbe);
          aProcess->enqueueOutputMessages ( action );
          enqueueTagPipeline ( action, aProcess );
          break;
        }
        case kReplyAndRemoveMAF:
          {
            //Request was satisfied. We send the response, and, if there are
            //other MAF entries, wake them.

            // We're waking a MAF entry, so unconditionally remove the MAF entry when we're done
            // A normal search will find the entry waiting for an address at this point...
            aProcess->removeMafEntry() = true;
            theMaf.modifyState ( aProcess->mafEntry(), kCompleted );
            aProcess->enqueueOutputTransport ( aProcess->mafEntry()->transport );
            DBG_ ( Trace, 
                   Addr(aProcess->mafEntry()->transport[MemoryMessageTag]->address())
                   ( << theCacheInitInfo.theName <<  " Selecting maf entry for removal (serial: "
                     << aProcess->serial() << "): "
                     << *aProcess->mafEntry()->transport[MemoryMessageTag] ) );

            if (aProcess->mafEntry()->transport[MemoryMessageTag]->isExtReqType()) {
              unreserve ( kResFrontSideOut, aProcess );//BackSideOut still reserved
            }
            else {
              unreserveBSO ( 0, aProcess );            //FrontSideOut still reserved
            }
            enqueueTagPipeline ( action, aProcess );
            break;
          }
        case kInsertMAF_WaitAddress:
          // Queuing at the MAF allows requests to be scheduled for wakeup before
          // we know that the address is busy. Free resources
          unreserveBSO ( kResFrontSideOut, aProcess );
          theMaf.modifyState(aProcess->mafEntry(), kWaitAddress);
          theMaf.dump();
          break;

        case kNoAction:
        case kReplyAndRemoveResponseMAF:
        case kSend:
          //Not possible for requests
          DBG_Assert( false, ( << theCacheInitInfo.theName 
                               << " Unexpected result from runWakeMafProcess: " << action
                               << " for msg: " <<  *aProcess->transport()[MemoryMessageTag]) );
      }
    }


    void runSnoopProcess(ProcessEntry_p aProcess ) {
      FLEXUS_PROFILE();
      DBG_(Trace, Addr(aProcess->transport()[MemoryMessageTag]->address()) ( << theCacheInitInfo.theName << " runSnoopProcess " << *aProcess->transport()[MemoryMessageTag] ) );

      //Need to deal with I-cache probes
      MissAddressFile::maf_iter temp = theMaf.getProbingMAFEntry( addressOf(aProcess) );
      if (  temp != theMaf.end()
            && aProcess->transport()[MemoryMessageTag]->isProbeType()
         )
      {
          //Mark the MAF entry as to whether the IProbe was a hit or miss,
          //and stick it on the queue to be handled
          switch (aProcess->transport()[MemoryMessageTag]->type()) {
            case MemoryMessage::ProbedNotPresent:
              theMaf.modifyState( temp, kProbeMiss );
              break;
            case MemoryMessage::ProbedClean:
            case MemoryMessage::ProbedWritable:
            case MemoryMessage::ProbedDirty:
              theMaf.modifyState( temp, kProbeHit );
              break;
            default:
              DBG_Assert(false);
          }

          unreserveBSO ( kResFrontSideOut | kResEvictBuffer, aProcess );
          theIProbeList[getBank(aProcess)].push_back( temp );

      }
      else {
        Action action = theCacheControllerImpl->handleSnoopMessage( aProcess->transport()[MemoryMessageTag], aProcess->transport()[TransactionTrackerTag] );
        aProcess->consumeAction ( action );

        //The only legal actions for runSnoopProcesses are Send and NoAction
        //Note that a snoop operation will never wake up MAF entries, since it
        //can never remove an entry from the MAF.
        switch (action.theAction) {
        case kNoAction:
          //Message was handled in this cache, ie PrefetchInsert
          unreserveBSO ( kResFrontSideOut | kResEvictBuffer, aProcess );
          aProcess->type() = eProcNoMoreWork;
          enqueueTagPipeline ( action, aProcess );
          break;

        case kSend:
          DBG_ ( VVerb,
                 Addr(aProcess->transport()[MemoryMessageTag]->address())
                 ( << theCacheInitInfo.theName << " kSend "
                   << *aProcess->transport()[MemoryMessageTag] ) );

          //Forward on to next cache level
          unreserve ( kResFrontSideOut | kResEvictBuffer, aProcess );          //BackSideOut_Snoop still reserved

          //Even if a snoop message requires data, we can pass it on
          //to the next hierarchy level without waiting for the data access
          aProcess->enqueueOrigTransport();
          aProcess->transmitAfterTag() = true;
          enqueueTagPipeline ( action, aProcess );
          break;

        case kReplyAndRemoveResponseMAF:
          {
            aProcess->removeMafEntry() = true;
            aProcess->mafEntry() = theMaf.getWaitingMAFEntryIter ( addressOf ( aProcess ) );
            DBG_Assert ( aProcess->mafEntry() != theMaf.end() );
            theMaf.modifyState ( aProcess->mafEntry(), kCompleted );

            Transport trans = aProcess->mafEntry()->transport;
            aProcess->enqueueOutputTransport ( trans );

            //if (aProcess->transport()[MemoryMessageTag]->directionToBack()) {  BTG commented out
            if (trans[MemoryMessageTag]->directionToBack()) {
              unreserve( kResEvictBuffer | kResFrontSideOut, aProcess);
            } else {  
              unreserveBSO ( kResEvictBuffer, aProcess );          //FrontSideOut still reserved
            }   

            DBG_ ( Trace,
                   Addr(aProcess->transport()[MemoryMessageTag]->address())
                   ( << theCacheInitInfo.theName << " just enqueued response of: "
                     << *trans[MemoryMessageTag] << " for response to: "
                     << *aProcess->transport()[MemoryMessageTag] ) );

            aProcess->transmitAfterTag() = true;
            enqueueTagPipeline ( action, aProcess );
          }
          break;

        case kInsertMAF_WaitAddress:
          {
            unreserveBSO ( kResEvictBuffer | kResFrontSideOut , aProcess );
            theMaf.allocEntry(addressOf(aProcess), aProcess->transport(), kWaitAddress);
            theMaf.dump();
            break;
          }
        case kInsertMAF_WaitProbe:
        default:
          //Not allowed
          DBG_Assert( false, ( << theCacheInitInfo.theName 
                               << " Unexpected result from runSnoopProcess: " << action
                               << " for msg: " << *aProcess->transport()[MemoryMessageTag]) );
          break;
        }
      }
    }

    void runIProbeProcess(ProcessEntry_p aProcess ) {
      FLEXUS_PROFILE();
      DBG_(Trace,
           Addr(aProcess->mafEntry()->transport[MemoryMessageTag]->address())
           ( << theCacheInitInfo.theName << " runIProbeProcess" << *aProcess->mafEntry()->transport[MemoryMessageTag] ) );

      Action action = theCacheControllerImpl->handleIprobe( aProcess->mafEntry()->state == kProbeHit, aProcess->mafEntry()->transport[MemoryMessageTag], aProcess->mafEntry()->transport[TransactionTrackerTag]);
      aProcess->consumeAction ( action );
      switch(action.theAction) {
        case kInsertMAF_WaitResponse: {
          aProcess->enqueueOutputMessages ( action );
          if ( aProcess->outputDirectionBack() ) {
            //Cache needs help from the outside to fill this response
            unreserve ( kResFrontSideOut, aProcess );            //BackSideOut_* still reserved
          } else {
            unreserveBSO ( 0, aProcess );            // FrontSideOut still reserved
          }
          theMaf.modifyState(aProcess->mafEntry(), kWaitResponse);
          enqueueTagPipeline ( action, aProcess );
          break;
        }

        case kReplyAndRemoveMAF:
          {
            aProcess->removeMafEntry() = true;
            theMaf.modifyState(aProcess->mafEntry(), kCompleted);
            
            aProcess->enqueueOrigTransport();

            unreserveBSO ( 0, aProcess );            //FrontSideOut still reserved
            aProcess->transmitAfterTag() = true; //IProbe responses can be sent without waiting for the data array
            enqueueTagPipeline ( action, aProcess );
            break;
          }
        default:
          //Not possible for requests
          DBG_Assert( false, ( << theCacheInitInfo.theName
                               << " Unexpected result from runIProbeProcess: " << action
                               << " for msg: " <<  *aProcess->transport()[MemoryMessageTag]) );
      }

    }

    void runBackProcess(ProcessEntry_p aProcess ) {
      FLEXUS_PROFILE();

      DBG_(Trace, Addr(aProcess->transport()[MemoryMessageTag]->address()) ( << theCacheInitInfo.theName << " runBackProcess " << *aProcess->transport()[MemoryMessageTag] ) );
      Action action ( kNoAction, aProcess->transport()[TransactionTrackerTag] );

      bool isMafReserved( theCacheInitInfo.theIsPiranhaCache && aProcess->transport()[ MemoryMessageTag ]->isExtReqType() );

      action = theCacheControllerImpl->handleBackMessage( aProcess->transport()[MemoryMessageTag], aProcess->transport()[TransactionTrackerTag] );

      aProcess->consumeAction ( action );
      switch( action.theAction) {
        case kSend:
          DBG_ ( Trace,
                 Addr(aProcess->transport()[MemoryMessageTag]->address())
                 ( << theCacheInitInfo.theName << " kSend "
                   << *aProcess->transport()[MemoryMessageTag] ) );

          // To support NUCA or a DSM of CMPs, back message requests have to
          // allocate MAF entries (invalidations can create a transaction context)
          //
          if (aProcess->transport()[MemoryMessageTag]->directionToBack()) {
            unreserve( kResEvictBuffer | kResFrontSideOut, aProcess);
          } else {  
            unreserveBSO ( kResEvictBuffer, aProcess );          //FrontSideOut still reserved
          }  
          if ( isMafReserved ) {
            unreserve( kResMaf, aProcess );
          }

          aProcess->enqueueOrigTransport();
          enqueueTagPipeline ( action, aProcess );
          break;

        case kReplyAndRemoveResponseMAF:
          {
            aProcess->removeMafEntry() = true;
            aProcess->mafEntry() = theMaf.getWaitingMAFEntryIter ( addressOf ( aProcess ) );
            DBG_Assert ( aProcess->mafEntry() != theMaf.end(), ( << theCacheInitInfo.theName << " msg: " << *aProcess->transport()[MemoryMessageTag]) );
            theMaf.modifyState ( aProcess->mafEntry(), kCompleted );

            Transport trans = aProcess->mafEntry()->transport;
            aProcess->enqueueOutputTransport ( trans );

            DBG_ ( Trace,
                   Addr(aProcess->transport()[MemoryMessageTag]->address())
                   ( << theCacheInitInfo.theName << " just enqueued response of: "
                     << *trans[MemoryMessageTag] << " for response to: "
                     << *aProcess->transport()[MemoryMessageTag] ) );

            if ( aProcess->outputDirectionBack() ) {
              unreserve ( kResEvictBuffer | kResFrontSideOut, aProcess );              // BackSideOut_Snoop still reserved
            } else {
              unreserveBSO ( kResEvictBuffer, aProcess );              // FrontSideOut still reserved
            }
            if ( isMafReserved ) {
              unreserve( kResMaf, aProcess );
            }

            enqueueTagPipeline ( action, aProcess );
          }
          break;

        case kReplyAndRemoveMAF:
          {
            // This type of message can resolve a waiting MAF entry.  Check
            // for waiting, matching entries and schedule them for removal.
            DBG_Assert ( aProcess->type() == eProcBack );
            MissAddressFile::maf_iter iter = theMaf.getWaitingMAFEntryIter ( addressOf ( aProcess ) );
            if ( iter != theMaf.end() ) {
              aProcess->removeMafEntry() = true;
              aProcess->mafEntry() = iter;
              theMaf.modifyState ( aProcess->mafEntry(), kCompleted );
              aProcess->enqueueOutputTransport ( aProcess->mafEntry()->transport );
              DBG_ ( Trace,
                     Addr(aProcess->mafEntry()->transport[MemoryMessageTag]->address())
                     ( << theCacheInitInfo.theName <<  " Selecting maf entry for removal (serial: "
                       << aProcess->serial() << "): "
                       << *aProcess->mafEntry()->transport[MemoryMessageTag] ) );
            } else {
              aProcess->enqueueOrigTransport();
            }

            aProcess->transmitAfterTag() = true; //Back responses can be sent without waiting for the data array

            if ( aProcess->outputDirectionBack() ) {
              unreserve ( kResEvictBuffer | kResFrontSideOut, aProcess );              // BackSideOut_Snoop still reserved
            } else {
              unreserveBSO ( kResEvictBuffer, aProcess );              // FrontSideOut still reserved
            }
            if ( isMafReserved ) {
              unreserve( kResMaf, aProcess );
            }

            enqueueTagPipeline ( action, aProcess );
            break;
          }

        case kNoAction:

          // May have CMP-internal broadcast output transports, in which case we
          // send them through the FSO
          if ( !action.theOutputMessage.empty() ) {
            aProcess->enqueueOutputMessages ( action );
            unreserveBSO ( kResEvictBuffer, aProcess );
            DBG_Assert ( !aProcess->outputDirectionBack() );
            DBG_ ( Trace,
                   Addr(aProcess->transport()[MemoryMessageTag]->address())
                   ( << theCacheInitInfo.theName
                     << " broadcast messages await for: " << *aProcess->transport()[MemoryMessageTag] ) );
          } else {
            unreserveBSO ( kResFrontSideOut | kResEvictBuffer, aProcess );
            // JCS - It's possible now
            aProcess->type() = eProcNoMoreWork;
            DBG_ ( Trace,
                   Addr(aProcess->transport()[MemoryMessageTag]->address())
                   ( << theCacheInitInfo.theName
                     << " no more work required for: " << *aProcess->transport()[MemoryMessageTag] ) );
          }
          if ( isMafReserved ) {
            unreserve( kResMaf, aProcess );
          }

          enqueueTagPipeline ( action, aProcess );
          break;

        case kInsertMAF_WaitResponse: 
          aProcess->enqueueOutputMessages ( action );
          if ( aProcess->outputDirectionBack() ) {
            //Cache needs help from the outside to fill this response
            unreserve ( kResEvictBuffer | kResFrontSideOut, aProcess );            //BackSideOut_* still reserved
          } else {
            // CMP needs coherence to fill this response
            unreserveBSO ( kResEvictBuffer , aProcess );            // FrontSideOut still reserved
          }
          if ( isMafReserved ) {
            unreserve( kResMaf, aProcess );
          }

          theMaf.allocEntry(addressOf(aProcess), aProcess->transport(), kWaitResponse);
          enqueueTagPipeline ( action, aProcess );
          break;

        case kInsertMAF_WaitAddress:
          // Queuing at the MAF allows requests to be scheduled for wakeup before
          // we know that the address is busy. Free resources
          unreserveBSO ( kResEvictBuffer | kResFrontSideOut, aProcess );
          if ( isMafReserved ) {
            unreserve( kResMaf, aProcess );
          }

          theMaf.allocEntry(addressOf(aProcess), aProcess->transport(), kWaitAddress);
          break;

        case kInsertMAF_WaitProbe:
          DBG_Assert( false, ( << theCacheInitInfo.theName 
                               << " Unexpected result from runBackProcess: " << action
                               << " for msg: " <<  *aProcess->transport()[MemoryMessageTag]) );

      }
    }

    //Get the block address of a process
    MemoryAddress const addressOf( ProcessEntry_p aProcess ) const {
      return theCacheControllerImpl->getBlockAddress( (aProcess->transport()[MemoryMessageTag] )->address() );
    }

    boost::intrusive_ptr<TransactionTracker> trackerOf( ProcessEntry_p aProcess ) const {
      return aProcess->transport()[TransactionTrackerTag];
    }

    // Send all enqueued transports to their destinations
    void doTransmitProcess( ProcessEntry_p aProcess ) {

      bool
        unreservedFSO  = false,
        unreservedBSO_P = false,
        unreservedBSO_S = false,
        unreservedBSO_R = false;

      DBG_ ( Trace, ( << theCacheInitInfo.theName << " starting transmit for process serial: " << aProcess->serial() ) );

      while ( aProcess->hasOutputTransports() ) {
        Transport
          trans = aProcess->getOutputTransport();

        DBG_(Trace,
             Addr(trans[MemoryMessageTag]->address())
             ( << theCacheInitInfo.theName
               << " transmit process process serial " << aProcess->serial()
               << " type:" << aProcess->type()
               << " with reservations:" << aProcess->getReservations()
               << " msg:" << *trans[MemoryMessageTag]
               << " directionToBack:" << trans[MemoryMessageTag]->directionToBack()
            ));

        if ( trans[MemoryMessageTag]->directionToBack() ) {

          switch ( aProcess->type() ) {

          case eProcPrefetch:

            if ( !unreservedBSO_P ) {
              unreserve ( kResBackSideOut_Prefetch, aProcess );
              unreservedBSO_P = true;
            }

            sendBack_Prefetch ( trans );
            break;

          case eProcSnoop:
          case eProcBack:
          case eProcEviction:

            if ( !unreservedBSO_S ) {
              unreserve ( kResBackSideOut_Snoop, aProcess );
              unreservedBSO_S = true;
            }

            sendBack_Snoop ( trans );
            break;

          case eProcRequest:
          case eProcPurge:      /* CMU-ONLY */
          case eProcIProbe:
          case eProcMAFWakeup:

            if ( !unreservedBSO_R ) {
              unreserve ( kResBackSideOut_Request, aProcess );
              unreservedBSO_R = true;
            }
            sendBack_Request ( trans );
            break;

          default:
            DBG_Assert ( false,
                         ( << theCacheInitInfo.theName << " For transmit to back, invalid process type: "
                           << aProcess->type() ) );
          }

        } else {          // FrontSideOut

          if ( !unreservedFSO ) {
            unreserve ( kResFrontSideOut, aProcess );
            unreservedFSO = true;
          }
          sendFront ( trans );
        }

      }


      // Finally, deallocate and unlock the MAF, if necessary and
      // wakeup any processes waiting on the address
      if ( aProcess->removeMafEntry() ) {
        aProcess->removeMafEntry() = false;

        DBG_Assert ( aProcess->mafEntry() != theMaf.end() );
        DBG_Assert ( aProcess->mafEntry()->state == kCompleted );
        DBG_ ( Trace,
               Addr(aProcess->mafEntry()->transport[MemoryMessageTag]->address())
               ( << theCacheInitInfo.theName
                 << " removing MAF entry: " << *aProcess->mafEntry()->transport[MemoryMessageTag] ) );

        if ( 
             aProcess->type() != eProcPurge &&                             /* CMU-ONLY */
             theMaf.contains ( addressOf ( aProcess ), kWaitAddress ) 
           )
        {
          enqueueWakeMaf ( addressOf ( aProcess ),
                           trackerOf ( aProcess ) );
        }

        theMaf.remove ( aProcess->mafEntry() );
      }

      // we are done with this process
      aProcess->type() = eProcNoMoreWork;
    }

    // Message -> Front
    void sendFront(MemoryTransport &  transport) {
      DBG_(Trace, Addr(transport[MemoryMessageTag]->address()) ( << theCacheInitInfo.theName << " sendFront " << *transport[MemoryMessageTag] ) );
      if (transport[TransactionTrackerTag]) {
        transport[TransactionTrackerTag]->setDelayCause(theCacheInitInfo.theName, "Front Tx");
      }

      DBG_Assert ( transport[MemoryMessageTag]->coreIdx() >= 0 &&
                   transport[MemoryMessageTag]->coreIdx() < theCacheInitInfo.theCores );

      FrontSideOut[transport[MemoryMessageTag]->coreIdx()].enqueue(transport);
    }

    // Message -> Back(Request)
    void sendBack_Request(MemoryTransport & transport) {
      DBG_(Trace, Addr(transport[MemoryMessageTag]->address()) ( << theCacheInitInfo.theName << " sendBack_Request " << *transport[MemoryMessageTag] ) );
      if (transport[TransactionTrackerTag]) {
        transport[TransactionTrackerTag]->setDelayCause(theCacheInitInfo.theName, "Back Tx");
      }
      BackSideOut_Request.enqueue(transport);
    }

    // Message -> Back(Prefetch)
    void sendBack_Prefetch(MemoryTransport & transport) {
      DBG_(Trace, Addr(transport[MemoryMessageTag]->address()) ( << theCacheInitInfo.theName << " sendBack_Prefetch " << *transport[MemoryMessageTag] ) );
      if (transport[TransactionTrackerTag]) {
        transport[TransactionTrackerTag]->setDelayCause(theCacheInitInfo.theName, "Back Tx");
      }
      BackSideOut_Prefetch.enqueue(transport);
    }

    // Message -> Back(Snoop)
    void sendBack_Snoop(MemoryTransport & transport) {
      DBG_(Trace, Addr(transport[MemoryMessageTag]->address()) ( << theCacheInitInfo.theName << " sendBack_Snoop " << *transport[MemoryMessageTag] ) );
      if (transport[TransactionTrackerTag]) {
        transport[TransactionTrackerTag]->setDelayCause(theCacheInitInfo.theName, "Back Tx");
      }
      BackSideOut_Snoop.enqueue(transport);
    }

  };  // end class CacheController

}  // end namespace nCache

  #define DBG_Reset
  #include DBG_Control()


#endif  // FLEXUS_CACHE_CONTROLLER_HPP_INCLUDED
