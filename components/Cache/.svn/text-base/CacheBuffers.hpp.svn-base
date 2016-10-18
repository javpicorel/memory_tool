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
 */

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/export.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>

#ifndef _CACHEBUFFERS_HPP
#define _CACHEBUFFERS_HPP

namespace nCache {

  using boost::counted_base;
  using boost::intrusive_ptr;
  using Flexus::SharedTypes::MemoryMessage;

  // Translator class for the access type of a memory message
  template <class MemoryMessage, class AccessType>
  class MemoryAccessTranslator;

  // specialization for UniAccessType
  template <class MemoryMessage>
  class MemoryAccessTranslator<MemoryMessage, UniAccessType> {
    typedef UniAccessType AccessType;

  public:
    static AccessType getAccessType(MemoryMessage & aMessage) {
      return getAccessType( aMessage.type() );
    }

    static AccessType getAccessType(typename MemoryMessage::MemoryMessageType aType) {
      switch(aType) {
      case MemoryMessage::LoadReq:
        return LOAD_REQ;
      case MemoryMessage::StoreReq:
        return STORE_REQ;
      case MemoryMessage::StorePrefetchReq:
        return STORE_PREFETCH_REQ;
      case MemoryMessage::FetchReq:
        return FETCH_REQ;
      case MemoryMessage::NonAllocatingStoreReq:
        return NON_ALLOCATING_STORE_REQ;
      case MemoryMessage::RMWReq:
        return RMW_REQ;
      case MemoryMessage::CmpxReq:
        return CMP_SWAP_REQ;
      case MemoryMessage::AtomicPreloadReq:
        return ATOMIC_PRELOAD_REQ;
      case MemoryMessage::FlushReq:
        return FLUSH_REQ;
        /* CMU-ONLY-BLOCK-BEGIN */
      case MemoryMessage::PurgeIReq:
      case MemoryMessage::PurgeDReq:
        return PURGE_REQ;
        /* CMU-ONLY-BLOCK-END */
      case MemoryMessage::ReadReq:
        return READ_REQ;
      case MemoryMessage::WriteReq:
        return WRITE_REQ;
      case MemoryMessage::WriteAllocate:
        return WRITE_ALLOC;
      case MemoryMessage::UpgradeReq:
        return UPGRADE_REQ;
      case MemoryMessage::UpgradeAllocate:
        return UPGRADE_ALLOC;
      case MemoryMessage::Flush:
        return FLUSH;
      case MemoryMessage::EvictDirty:
        return EVICT_DIRTY;
      case MemoryMessage::EvictWritable:
        return EVICT_WRITABLE;
      case MemoryMessage::EvictClean:
        return EVICT_CLEAN;
      case MemoryMessage::MissReply:
        return MISS_REPLY;
      case MemoryMessage::MissReplyWritable:
        return MISS_REPLY_WRITABLE;
      case MemoryMessage::MissReplyDirty:
        return MISS_REPLY_DIRTY;
      case MemoryMessage::FetchReply:
        return FETCH_REPLY;
      case MemoryMessage::UpgradeReply:
        return UPGRADE_REPLY;
      case MemoryMessage::Invalidate:
        return INVALIDATE;
      case MemoryMessage::Downgrade:
        return DOWNGRADE;
      case MemoryMessage::Probe:
        return PROBE;
      case MemoryMessage::PrefetchReadNoAllocReq:
        return PREFETCH_READ_NOALLOC_REQ;
      case MemoryMessage::PrefetchReadAllocReq:
        return PREFETCH_READ_ALLOC_REQ;
      case MemoryMessage::PrefetchInsert:
        return PREFETCH_INSERT;
      case MemoryMessage::PrefetchInsertWritable:
        return PREFETCH_INSERT_WRITABLE;
      case MemoryMessage::ReturnReq:
        return RETURN_REQ;
      case MemoryMessage::ReturnReply:
        return RETURN_REPLY;
      default:
        DBG_Assert(false);
        return LOAD_REQ;
      }
    }

  };  // end MemoryAccessTranslator


  struct EvictEntry {
    MemoryAddress theBlockAddress;
    mutable MemoryMessage::MemoryMessageType theType;
    mutable bool theEvictable;
    //Note - evict buffer entries should also contain data
    EvictEntry( MemoryAddress anAddress, MemoryMessage::MemoryMessageType aType, bool anEvictable=true)
      : theBlockAddress( anAddress )
      , theType(aType)
        , theEvictable(anEvictable)
      {}
    private:
      EvictEntry() {} //For serialization
    public:
    MemoryAddress address() const { return theBlockAddress; }
    MemoryMessage::MemoryMessageType & type() const { return theType; }
    bool & evictable() { return theEvictable; }
    const bool evictable() const { return theEvictable; }
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
      // Version 0 of the EvictEntry does not contain theEvictable.
      // It is always considered to be true in older checkpoints.
      // Version 1 contains this boolean flag.
        ar & theBlockAddress;
        ar & theType;
        if ( version > 0 ) {
          ar & theEvictable;
        } else {
          theEvictable = true;
        }
    }
    friend std::ostream & operator << ( std::ostream & anOstream, EvictEntry const & anEntry) {
      anOstream << "Evict(" << anEntry.theType << " @" << anEntry.theBlockAddress << ")";
      return anOstream;
    }
  };

  // the evict buffer contains uninitiated evictions
  class EvictBuffer {
    struct by_address {};
    struct by_order {};
    typedef multi_index_container
      < EvictEntry
      , indexed_by
        < sequenced < tag<by_order> >
        , ordered_unique
            < tag<by_address>
            , member< EvictEntry, MemoryAddress, &EvictEntry::theBlockAddress >
            >
        >
      >
      evict_buf_t;


    evict_buf_t theEvictions;
    unsigned int theSize;
    unsigned int theReserve;

  public:
    typedef evict_buf_t::index<by_address>::type::iterator iterator;
    void saveState( std::ostream & anOstream ) {
      boost::archive::binary_oarchive oa(anOstream);
      std::list<EvictEntry> evictions;
      std::copy( theEvictions.begin(), theEvictions.end(), std::back_inserter( evictions ));
      oa << (const std::list<EvictEntry>)evictions;
    }

    void loadState( std::istream & anIstream ) {
      boost::archive::binary_iarchive ia(anIstream);
      theEvictions.clear();
      std::list<EvictEntry> evictions;
      ia >> evictions;
      std::copy( evictions.begin(), evictions.end(), std::back_inserter(theEvictions));
    }

    EvictBuffer( unsigned int aSize )
      : theSize(aSize)
      , theReserve(0)
      {}

    bool empty() const {
      return theEvictions.empty();
    }

    bool full() const {
      return theEvictions.size() + theReserve >= theSize;
    }

    void reserve() {
      ++theReserve;
      DBG_Assert( theEvictions.size() + theReserve <= theSize );
    }

    void unreserve() {
      --theReserve;
      DBG_Assert( theReserve >= 0 );
    }

    unsigned int freeEntries() const {
      return theSize - theEvictions.size() - theReserve;
    }
    bool headEvictable(int anOffset) const {
      if ( empty() ) return false;
      evict_buf_t::iterator iter = theEvictions.begin();
      evict_buf_t::iterator end = theEvictions.end();

      while (anOffset > 0) {
        if ( iter == end ) { return false; }
        ++ iter; -- anOffset;
      }
      if ( iter == end ) { return false; }
      return (iter->evictable());
    }

    void setEvictable ( MemoryAddress anAddress,
                        const bool    val )
    {
      iterator entry = find ( anAddress );

      if ( entry == end() )
        return;

      entry->theEvictable = val;
    }

    void allocEntry(MemoryAddress anAddress, MemoryMessage::MemoryMessageType aType, const bool evictable = true) {
      iterator existing = find( anAddress);
      if (existing != end()) {
        DBG_( Iface, ( << "When trying to allocate an evict buffer entry for " << anAddress << " an existing entry with type " << existing->theType << " was found" ) );
        existing->theType = aType;
      } else {
        theEvictions.push_back( EvictEntry( anAddress, aType, evictable) );
      }
    }

    boost::intrusive_ptr<MemoryMessage> pop() {
      DBG_Assert( ! theEvictions.empty() );
      boost::intrusive_ptr<MemoryMessage> retval = new MemoryMessage(theEvictions.front().theType, theEvictions.front().theBlockAddress);
      DBG_( Iface, ( << "Evict buffer popping entry for " << theEvictions.front().theBlockAddress << " msg: " << *retval ) );
      DBG_Assert ( theEvictions.front().evictable() );
      theEvictions.pop_front();
      return retval;
    }

    // exact address checking should be fine here, since the original writeback
    // request should have been aligned on a block boundary
    iterator find(MemoryAddress const & anAddress) {
      return theEvictions.get<by_address>().find(anAddress);
    }

    void remove(iterator iter) {
      if (iter != end()) {
        DBG_( Iface, ( << "Evict buffer removing entry for " << iter->theBlockAddress ) );
        theEvictions.get<by_address>().erase(iter);
      }
    }

    iterator end() {
      return  theEvictions.get<by_address>().end();
    }

  };  // end class EvictBuffer


  struct SnoopEntry : boost::counted_base {
    boost::intrusive_ptr<MemoryMessage> message;
    enum MemoryMessage::MemoryMessageType state;
    explicit SnoopEntry( boost::intrusive_ptr<MemoryMessage> msg)
      : message(msg)
      , state( MemoryMessage::ProbedNotPresent )
      {}
    SnoopEntry()
      : message(0)
      , state( MemoryMessage::ProbedNotPresent )
      {}
  };
  typedef boost::intrusive_ptr<SnoopEntry> SnoopEntry_p;

  // the snoop buffer contains uninitiated evictions
  class SnoopBuffer {
    typedef std::map<MemoryAddress, SnoopEntry_p > SnoopMap;
    SnoopMap theSnoops;

  public:
    bool empty() const { return theSnoops.empty(); }
    // exact address checking should be fine here, since the original snoop
    // request should have been aligned on a block boundary
    SnoopEntry_p findEntry(MemoryAddress const & anAddress, bool removeEntry = false) {
      SnoopEntry_p ret(0);
      SnoopMap::iterator iter = theSnoops.find(anAddress);
      if(iter != theSnoops.end()) {
        ret = iter->second;
        if(removeEntry) {
          theSnoops.erase(iter);
        }
      }
      return ret;
    }

    SnoopEntry_p findAndRemoveEntry(const MemoryAddress & anAddress) {
      return findEntry(anAddress, true);
    }

    SnoopEntry_p allocEntry(boost::intrusive_ptr<MemoryMessage> newMessage) {
      // make a new MemoryMessage so that changes to the original object
      // don't change the state of this buffer entry, as the intrusive_ptr
      // is passed around
      bool was_inserted;
      SnoopMap::iterator iter;
      boost::tie( iter, was_inserted) = theSnoops.insert( std::make_pair( newMessage->address(), boost::intrusive_ptr<SnoopEntry>(new SnoopEntry(newMessage))) );
      DBG_Assert(was_inserted);
      return iter->second;
    }

    void removeEntry(SnoopEntry_p toRemove) {
      // remove from the map - it's guaranteed to be there
      int erase_count = theSnoops.erase(toRemove->message->address());
      DBG_Assert( erase_count > 0);
    }
  };  // end class SnoopBuffer



}  // end namespace nCache

BOOST_CLASS_VERSION(nCache::EvictEntry, 1)


#endif // _CACHEBUFFERS_HPP
