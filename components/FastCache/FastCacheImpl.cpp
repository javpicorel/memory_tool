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


#include <components/FastCache/FastCache.hpp>

#include <components/FastCache/CacheStats.hpp>
#include <components/FastCache/AssociativeCache.hpp>

#include <components/Common/TraceTracker.hpp>


#include <boost/bind.hpp>
#include <fstream>


  #define DBG_DefineCategories Cache
  #define DBG_SetDefaultOps AddCat(Cache) Comp(*this)
  #include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT FastCache
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()



namespace nFastCache {


using namespace Flexus;

class FLEXUS_COMPONENT(FastCache){
  FLEXUS_COMPONENT_IMPL( FastCache );

  AssociativeCache * theCache;
  CacheStats * theStats;
  unsigned long theBlockMask;

  unsigned long theLastTagset;
  bool theLastTagsetDirty;

  MemoryMessage theEvictMessage;

  int theIndex;

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(FastCache)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theEvictMessage(MemoryMessage::EvictDirty)
  {
  }

  //InstructionOutputPort
  //=====================
  bool isQuiesced() const {
    return true;
  }

  void saveState(std::string const & aDirName) {
    std::string fname( aDirName );
    fname += "/" + statName();
    std::ofstream ofs(fname.c_str());

    theCache->saveState ( ofs );

    ofs.close();
  }

  void loadState(std::string const & aDirName) {
    std::string fname( aDirName);
    fname += "/" + statName();
    std::ifstream ifs(fname.c_str());
    if (! ifs.good()) {
      DBG_( Dev, ( << " saved checkpoint state " << fname << " not found.  Resetting to empty cache. " )  );
    } else {
      ifs >> std::skipws;

      if ( ! theCache->loadState( ifs ) ) {
        DBG_ ( Dev, ( << "Error loading checkpoint state from file: " << fname <<
                       ".  Make sure your checkpoints match your current cache configuration." ) );
        DBG_Assert ( false );
      }
      ifs.close();
    }
  }

  void initialize(void) {
    static volatile bool widthPrintout = true;
    if (widthPrintout) {
      DBG_( Crit, ( << "Running with MT width " << cfg.MTWidth ) );
      widthPrintout = false;
    }

    theStats = new CacheStats(statName());
    theIndex = flexusIndex();

    //Confirm that BlockSize is a power of 2
    DBG_Assert( (cfg.BlockSize & (cfg.BlockSize - 1)) == 0);
    DBG_Assert( cfg.BlockSize  >= 4);

    int num_sets = cfg.Size / cfg.BlockSize / cfg.Associativity;

    //Confirm that num_sets is a power of 2
    DBG_Assert( (num_sets & (num_sets  - 1)) == 0);

    //Confirm that settings are consistent
    DBG_Assert( cfg.BlockSize * num_sets * cfg.Associativity == cfg.Size);

    //Calculate shifts and masks
    theBlockMask = ~(cfg.BlockSize - 1);

    theLastTagset = 1; //Can't match anything

    fillAccessTable();

    theCache = new AssociativeCache(cfg.BlockSize, num_sets, cfg.Associativity, cfg.CleanEvictions, boost::bind( &FastCacheComponent::evict, this, _1, _2), theIndex, cfg.CacheLevel, cfg.TraceTracker );
  }

  void notifyRead( MemoryMessage & aMessage) {
    if (cfg.NotifyReads) {
      FLEXUS_CHANNEL( Reads ) << aMessage;
    }
  }

  void notifyWrite( MemoryMessage & aMessage) {
    if (cfg.NotifyWrites) {
      FLEXUS_CHANNEL( Writes ) << aMessage;
    }
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(FetchRequestIn);
  void push( interface::FetchRequestIn const &,
             index_t         anIndex,
             MemoryMessage & aMessage)
  {
		//DBG_( Iface, Addr(aMessage.address()) ( << "FetchRequestIn[" << anIndex << "]: " << aMessage << " tagset: " << std::hex << (aMessage.address() & theBlockMask) << std::dec ));
    push( interface::RequestIn(), anIndex, aMessage);
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(RequestIn);
  void push( interface::RequestIn const &,
             index_t         anIndex,
             MemoryMessage & aMessage)
  {
    //Create a set and tag from the message's address
    unsigned long tagset = aMessage.address() & theBlockMask;
    unsigned long offset = aMessage.address() & (~theBlockMask);
    //DBG_( Iface, Addr(aMessage.address()) ( << "Request[" << anIndex << "]: " << aMessage << " tagset: " << std::hex << tagset << std::dec ));

    //Map the memory message type into the internal access type
    long * hit_counter = 0;
    bool read_access = false;
    unsigned long load_access = 0;
    access_t access_type = 0;
    switch (aMessage.type()) {
      case MemoryMessage::FetchReq:
        //Fast hit optimization
        if (tagset == theLastTagset ) {
          //DBG_( Iface, ( << "Fast Hit: " << aMessage ));
          ++theStats->theHits_Fetch;
          aMessage.fillLevel() = cfg.CacheLevel;
          return;
        }
        access_type = kFetchAccess;
        hit_counter = &theStats->theHits_Fetch;
        break;

      case MemoryMessage::LoadReq:
        //Fast hit optimization
        if (tagset == theLastTagset ) {
					//DBG_( Iface, ( << "Fast Hit: " << aMessage ));
					if (cfg.TraceTracker) {
						theTraceTracker.accessLoad(theIndex, cfg.CacheLevel, tagset, aMessage.pc(), offset, aMessage.reqSize(), aMessage.isPriv());
					}
					++theStats->theHits_Read;
					aMessage.fillLevel() = cfg.CacheLevel;
					return;
				}
				notifyRead( aMessage );
				access_type = kLoadAccess;
				load_access = kDirtyBit; //We use this constant in the MissReplyDirty case below
				hit_counter = &theStats->theHits_Read;
				break;

			case MemoryMessage::StoreReq:
			case MemoryMessage::StorePrefetchReq:
				//Fast hit optimization
				notifyWrite( aMessage );
				if (tagset == theLastTagset && theLastTagsetDirty) {
					//DBG_( Iface, ( << "Fast Hit: " << aMessage ));
					if (cfg.TraceTracker) {
						theTraceTracker.accessStore(theIndex, cfg.CacheLevel, tagset, aMessage.pc(), offset, aMessage.reqSize(), aMessage.isPriv());
					}
					++theStats->theHits_Write;
					aMessage.fillLevel() = cfg.CacheLevel;
					return;
				}
				access_type = kStoreAccess;
				hit_counter = &theStats->theHits_Write;
				break;

			case MemoryMessage::RMWReq:
			case MemoryMessage::CmpxReq:
				//Fast hit optimization
				notifyWrite( aMessage );
				if (tagset == theLastTagset && theLastTagsetDirty) {
					if (cfg.TraceTracker) {
						theTraceTracker.accessAtomic(theIndex, cfg.CacheLevel, tagset, aMessage.pc(), offset, aMessage.reqSize(), aMessage.isPriv());
					}
					//DBG_( Iface, ( << "Fast Hit: " << aMessage ));
          ++theStats->theHits_Atomic;
          aMessage.fillLevel() = cfg.CacheLevel;
          return;
        }
        access_type = kStoreAccess;
        hit_counter = &theStats->theHits_Atomic;
        break;

      case MemoryMessage::ReadReq:
        notifyRead( aMessage );
        access_type = kReadAccess;
				read_access = true;
        hit_counter = &theStats->theHits_Read;
        break;

      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:
        notifyRead( aMessage );
        notifyWrite( aMessage );
        access_type = kWriteAccess;
        hit_counter = &theStats->theHits_Write;
        break;

      case MemoryMessage::UpgradeReq:
      case MemoryMessage::UpgradeAllocate:
        notifyWrite( aMessage );
        access_type = kWriteAccess;
        hit_counter = &theStats->theHits_Upgrade;
        break;

      case MemoryMessage::EvictDirty:
        access_type = kDirtyEvictAccess;
        hit_counter = &theStats->theHits_Evict;
        break;

      case MemoryMessage::EvictWritable:
      case MemoryMessage::PrefetchInsertWritable:
        access_type = kWritableEvictAccess;
        hit_counter = &theStats->theHits_Evict;
        break;

      case MemoryMessage::EvictClean:
      case MemoryMessage::PrefetchInsert:
        access_type = kCleanEvictAccess;
        hit_counter = &theStats->theHits_Evict;
        break;

      case MemoryMessage::FlushReq:
        theCache->downgrade(tagset);
        if (theLastTagset == tagset) {
          theLastTagsetDirty = false;
        }
        FLEXUS_CHANNEL( RequestOut ) << aMessage;
        return; // JUMP OUT!

      case MemoryMessage::PrefetchReadAllocReq:
        access_type = kLoadAccess;
        load_access = kDirtyBit; //We use this constant in the MissReplyDirty case below
        hit_counter = &theStats->theHits_Prefetch;
        break;

      case MemoryMessage::PrefetchReadNoAllocReq:
        access_type = kPrefetchReadNoAlloc;
        read_access = true;
        hit_counter = &theStats->theHits_Prefetch;
        break;

      default:
        DBG_Assert(false, ( << aMessage )); //Unhandled message type
    }

    if(aMessage.type() == MemoryMessage::PrefetchReadAllocReq ||
       aMessage.type() == MemoryMessage::PrefetchReadNoAllocReq) {
      // don't allow a fast hit to this block on the next access, or the
      // prefetched bit will not be checked
      theLastTagset = 1;
    } else {
      theLastTagset = tagset;
    }

    //Perform the access - will update access_type to indicate what further
    //actions to take
    access_t orig_access = access_type;
    unsigned long * frame_ptr = theCache->access(tagset, access_type);

    access_t miss = access_type & kMiss;
    bool prefetched = access_type & kPrefetchHit;
    if (miss) {
      //Handle misses
      if (miss == kReadMiss) {
         //DBG_( Iface, ( << "Read Miss: " << aMessage ));
         if (cfg.TraceTracker) {
           doTraceTrackerAccess(tagset, offset, aMessage, true, prefetched);
         }
         if(aMessage.isPrefetchType()) {
           ++theStats->theMisses_Prefetch;
         } else {
           ++theStats->theMisses_Read;
         }
         if(access_type & kInvalidTagMatch) {
           ++theStats->theTagMatches_Invalid;
         }
         access_t prefetch_access = kInvalid;
         if(aMessage.type() == MemoryMessage::PrefetchReadAllocReq) {
           prefetch_access = kPrefetchedBit;
         }

         aMessage.type() = MemoryMessage::ReadReq;
         FLEXUS_CHANNEL( RequestOut ) << aMessage;

         if (cfg.TraceTracker) {
           if(prefetch_access != kInvalid) {
             theTraceTracker.prefetchFill(theIndex, cfg.CacheLevel, tagset, aMessage.fillLevel());
           } else {
             theTraceTracker.fill(theIndex, cfg.CacheLevel, tagset, aMessage.fillLevel(), false, false);
           }
         }
         switch( aMessage.type() ) {
            case MemoryMessage::MissReply:
              theCache->apply_returned_state( frame_ptr, kValid | prefetch_access );
              access_type |= kFillValid;
              theLastTagsetDirty = false;
              break;
            case MemoryMessage::MissReplyWritable:
              // DBG_Assert(false, Addr(aMessage.address()) (<< "how did we get here? statname=" << statName() << " address=0x" << std::hex << aMessage.address()));
              theCache->apply_returned_state( frame_ptr, kWritable | prefetch_access );
              access_type |= kFillWritable;
              theLastTagsetDirty = false;
              break;
            case MemoryMessage::MissReplyDirty:
              theCache->apply_returned_state( frame_ptr, kWritable | (kDirtyBit & load_access) | prefetch_access );
              if(kDirtyBit & load_access) {
                DBG_( Trace, ( << "Marking block dirty at this level: " << aMessage ) );
                access_type |= kFillWritable;  // return Writable to above hierarchy
              } else {
                access_type |= kFillDirty;  // must propagate dirtiness if not set here
              }
              theLastTagsetDirty = true;
              break;
            case MemoryMessage::ReadReq:
              //This case indicates nothing is hooked up below this cache.
              theCache->apply_returned_state( frame_ptr, kValid | prefetch_access );
              access_type |= kFillValid;
              theLastTagsetDirty = false;
              break;
            default:
              DBG_Assert(false);
         }
      } else if (miss == kFetchMiss) {
         //DBG_( Iface, ( << "Fetch Miss: " << aMessage ));
         if (cfg.TraceTracker) {
           doTraceTrackerAccess(tagset, offset, aMessage, true, prefetched);
         }
         ++theStats->theMisses_Fetch;
         aMessage.type() = MemoryMessage::FetchReq;
         FLEXUS_CHANNEL( RequestOut ) << aMessage;
         if (cfg.TraceTracker) {
           theTraceTracker.fill(theIndex, cfg.CacheLevel, tagset, aMessage.fillLevel(), true, false);
         }
         theLastTagsetDirty = false;
      } else if (miss == kWriteMiss) {
         //DBG_( Iface, ( << "Write Miss: " << aMessage ));
         if (cfg.TraceTracker) {
           doTraceTrackerAccess(tagset, offset, aMessage, true, prefetched);
         }
         ++theStats->theMisses_Write;
         if(access_type & kInvalidTagMatch) {
           ++theStats->theTagMatches_Invalid;
         }
         aMessage.type() = MemoryMessage::WriteReq;
         FLEXUS_CHANNEL( RequestOut ) << aMessage;
         if (cfg.TraceTracker) {
           theTraceTracker.fill(theIndex, cfg.CacheLevel, tagset, aMessage.fillLevel(), false, true);
         }
         theLastTagsetDirty = false;
      } else if (miss == kReadNoAllocMiss) {
         //DBG_( Iface, ( << "Read No Allocate Miss: " << aMessage ));
         if (cfg.TraceTracker) {
           doTraceTrackerAccess(tagset, offset, aMessage, true, prefetched);
         }
         ++theStats->theMisses_Prefetch;
         // don't change the message type
         FLEXUS_CHANNEL( RequestOut ) << aMessage;
         theLastTagsetDirty = false;
      } else { //Upgrade Miss
         //DBG_( Iface, ( << "Upgrade Miss: " << aMessage ));
         if (prefetched) {
           ++theStats->thePrefetchHits_ButUpgrade;
         }
         if (cfg.TraceTracker) {
           doTraceTrackerAccess(tagset, offset, aMessage, false, prefetched);
         }
         ++theStats->theMisses_Upgrade;
         aMessage.type() = MemoryMessage::UpgradeReq;
         FLEXUS_CHANNEL( RequestOut ) << aMessage;
         theLastTagsetDirty = false;
      }

    } else { //Hit
      //DBG_( Iface, ( << "Hit: " << aMessage ));
      if (prefetched) {
        if (isReadType(orig_access)) ++theStats->thePrefetchHits_Read;
        if (isWriteType(orig_access)) ++theStats->thePrefetchHits_Write;
        if (isEvictType(orig_access)) ++theStats->thePrefetchHits_Evict;
      }
      if (cfg.TraceTracker) {
        doTraceTrackerAccess(tagset, offset, aMessage, false, prefetched);
      }
      ++(*hit_counter);
      theLastTagsetDirty = (access_type & kDirtyBit);
      aMessage.fillLevel() = cfg.CacheLevel;
    }

    //Read accesses need to return a fill type
    if (read_access) {
      if ( (access_type & kFill) == kFillValid) {
        aMessage.type() = MemoryMessage::MissReply;
      } else if ( (access_type & kFill) == kFillWritable) {
        aMessage.type() = MemoryMessage::MissReplyWritable;
      } else if ( (access_type & kFill) == kFillDirty) {
        aMessage.type() = MemoryMessage::MissReplyDirty;
      } else if ( (access_type & kFill) == kPrefetchRedundant) {
        aMessage.type() = MemoryMessage::PrefetchReadRedundant;
      }
    }

    //DBG_( Iface, ( << "Done, reply: " << aMessage ));
  }

  void doTraceTrackerAccess(unsigned long tagset, unsigned long offset, MemoryMessage & aMessage, bool miss, bool prefetched) {
    switch (aMessage.type()) {
      case MemoryMessage::FetchReq:
        theTraceTracker.accessFetch(theIndex, cfg.CacheLevel, tagset, offset, 8);
				theTraceTracker.access(theIndex, cfg.CacheLevel, aMessage.address(), aMessage.pc(), prefetched, false, miss, aMessage.isPriv(), 0);
				break;

			case MemoryMessage::LoadReq:
				// case MemoryMessage::PrefetchReadReq:
				theTraceTracker.accessLoad(theIndex, cfg.CacheLevel, tagset, aMessage.pc(), offset, aMessage.reqSize(), aMessage.isPriv());
			case MemoryMessage::ReadReq:
				theTraceTracker.access(theIndex, cfg.CacheLevel, aMessage.address(), aMessage.pc(), prefetched, false, miss, aMessage.isPriv(), 0);
				break;

			case MemoryMessage::PrefetchReadAllocReq:
				theTraceTracker.prefetch(theIndex, cfg.CacheLevel, aMessage.address());
				break;

			case MemoryMessage::StoreReq:
				theTraceTracker.accessStore(theIndex, cfg.CacheLevel, tagset, aMessage.pc(), offset, aMessage.reqSize(), aMessage.isPriv());
			case MemoryMessage::StorePrefetchReq:
			case MemoryMessage::WriteReq:
			case MemoryMessage::WriteAllocate:
			case MemoryMessage::UpgradeReq:
			case MemoryMessage::UpgradeAllocate:
				theTraceTracker.access(theIndex, cfg.CacheLevel, aMessage.address(), aMessage.pc(), prefetched, true, miss, aMessage.isPriv(), 0);
				break;

			case MemoryMessage::RMWReq:
			case MemoryMessage::CmpxReq:
				theTraceTracker.accessAtomic(theIndex, cfg.CacheLevel, tagset, aMessage.pc(), offset, aMessage.reqSize(), aMessage.isPriv());
				theTraceTracker.access(theIndex, cfg.CacheLevel, aMessage.address(), aMessage.pc(), prefetched, true, miss, aMessage.isPriv(), 0);
				break;

      case MemoryMessage::EvictDirty:
      case MemoryMessage::EvictWritable:
      case MemoryMessage::EvictClean:
      case MemoryMessage::PrefetchInsert:
        theTraceTracker.insert(theIndex, cfg.CacheLevel, tagset);
        break;

      case MemoryMessage::PrefetchReadNoAllocReq:
        // do nothing
        break;

      default:
        DBG_Assert(false); //Unhandled message type
    }
  }

  void evict(unsigned long aTagset, access_t aLineState) {
    switch(aLineState) {
      case kDirty:
        theEvictMessage.type() = MemoryMessage::EvictDirty;
        break;
      case kWritable:
        theEvictMessage.type() = MemoryMessage::EvictWritable;
        break;
      case kValid:
        theEvictMessage.type() = MemoryMessage::EvictClean;
        break;
      default:
        DBG_Assert( false, ( << std::hex << "Tagset: " << aTagset << " EvictType: " << aLineState << " Msg: " << theEvictMessage << std::dec ) );
    }
    theEvictMessage.address() = PhysicalMemoryAddress(aTagset);
    //DBG_( Iface, ( << "Evict: " << theEvictMessage ));
    FLEXUS_CHANNEL( RequestOut ) << theEvictMessage;
  }

  FLEXUS_PORT_ALWAYS_AVAILABLE(SnoopIn);
  void push( interface::SnoopIn const &, MemoryMessage & aMessage) {
    unsigned long tagset = aMessage.address() & theBlockMask;
    //DBG_( Iface, ( << "Snoop: " << aMessage << " tagset: " << std::hex << tagset << std::dec ));

    bool valid_flag = false, dirty_flag = false;
    switch (aMessage.type()) {
      case MemoryMessage::ReturnReq: {
        ++theStats->theSnoops_ReturnReq;
        bool found = theCache->returnReq(tagset);
        DBG_Assert(found, ( << " Snoop: " << aMessage << " tagset: " << std::hex << tagset << std::dec ) );
        if (theLastTagset == tagset) {
          theLastTagset = 0;
          theLastTagsetDirty = false;
        }
        break;
      }
      case MemoryMessage::Invalidate: {
        ++theStats->theSnoops_Invalidate;
        boost::tie(valid_flag, dirty_flag) = theCache->invalidate(tagset);
        if ( valid_flag) {
          ++theStats->theSnoops_InvalidateValid;
          if (dirty_flag) {
            ++theStats->theSnoops_InvalidateDirty;
          }
        }
        if (cfg.TraceTracker) {
          theTraceTracker.invalidation(theIndex, cfg.CacheLevel, tagset);
        }
        if (theLastTagset == tagset) {
          theLastTagset = 0;
          theLastTagsetDirty = false;
        }
        break;
      }
      case MemoryMessage::Downgrade: {
        ++theStats->theSnoops_Downgrade;
        dirty_flag = theCache->downgrade(tagset);
        if (dirty_flag) {
          ++theStats->theSnoops_DowngradeDirty;
        }
				if (cfg.TraceTracker) {
					theTraceTracker.downgrade(theIndex, cfg.CacheLevel, tagset);
				}
				if (theLastTagset == tagset) {
          theLastTagsetDirty = false;
        }
        break;
      }
      default:
        DBG_Assert(false);
    }

    //send snoop up
    FLEXUS_CHANNEL( SnoopOut ) << aMessage;

    if(aMessage.type() == MemoryMessage::Invalidate ||
       aMessage.type() == MemoryMessage::InvalidateAck ||
       aMessage.type() == MemoryMessage::InvUpdateAck) {
      if (cfg.TraceTracker) {
        theTraceTracker.invalidAck(theIndex, cfg.CacheLevel, tagset);
      }
    }

    //check reply
    switch (aMessage.type()) {
      case MemoryMessage::ReturnReq:
        //Nothing above
        aMessage.type() = MemoryMessage::ReturnReply;
        break;
      case MemoryMessage::ReturnReply:
      case MemoryMessage::InvUpdateAck:
      case MemoryMessage::DownUpdateAck:
        break;
      case MemoryMessage::Invalidate:
        //Nothing above
      case MemoryMessage::InvalidateAck:
        if (dirty_flag) {
          aMessage.type() = MemoryMessage::InvUpdateAck;
        }
        else if (valid_flag) {
          aMessage.type() = MemoryMessage::InvalidateAck;
        }
        break;
      case MemoryMessage::Downgrade:
        //Nothing above
      case MemoryMessage::DowngradeAck:
        if (dirty_flag) {
          aMessage.type() = MemoryMessage::DownUpdateAck;
        }
        else {
          aMessage.type() = MemoryMessage::DowngradeAck;
        }
        break;
      default:
        DBG_Assert(false);
    }
  }

  void drive( interface::UpdateStatsDrive const &) {
    theStats->update();
  }

  bool isReadType(access_t access) {
    switch(access) {
      case kLoadAccess:
      case kReadAccess:
      case kFetchAccess:
      case kPrefetchReadAccess:
      case kPrefetchReadNoAlloc:
        return true;
    }
    return false;
  }
  bool isWriteType(access_t access) {
    switch(access) {
      case kStoreAccess:
      case kWriteAccess:
        return true;
    }
    return false;
  }
  bool isEvictType(access_t access) {
    switch(access) {
      case kDirtyEvictAccess:
      case kWritableEvictAccess:
      case kCleanEvictAccess:
        return true;
    }
    return false;
  }

};  // end class FastCache


}  // end Namespace nFastCache

FLEXUS_COMPONENT_INSTANTIATOR( FastCache, nFastCache);

FLEXUS_PORT_ARRAY_WIDTH( FastCache, RequestIn)      { return (cfg.MTWidth); }
FLEXUS_PORT_ARRAY_WIDTH( FastCache, FetchRequestIn) { return (cfg.MTWidth); }
FLEXUS_PORT_ARRAY_WIDTH( FastCache, SnoopOut)       { return (cfg.MTWidth); }

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT FastCache

  #define DBG_Reset
  #include DBG_Control()
