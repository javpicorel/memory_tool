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

#include <components/FastCMPCache/FastCMPCache.hpp>

#include <components/FastCMPCache/CacheStats.hpp>
#include <components/FastCMPCache/AssociativeCache.hpp>

#include <boost/bind.hpp>
#include <ext/hash_map>

#include <fstream>

#include <stdlib.h> // for random()

  #define DBG_DefineCategories CMPCache
  #define DBG_SetDefaultOps AddCat(CMPCache) Comp(*this)
  #include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT FastCMPCache
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()



namespace nFastCMPCache {

using namespace Flexus;
using namespace Flexus::Core;
using namespace Flexus::SharedTypes;

static unsigned int theBlockSizeLog2;

class FLEXUS_COMPONENT(FastCMPCache){
  FLEXUS_COMPONENT_IMPL( FastCMPCache );

  // some bookkeeping vars
  unsigned int    theCMPWidth;       // # cores per CMP chip
  block_address_t theBlockMask;      // mask out the byte within the block
  unsigned long long   theEvictMask;      // determines which evictions will generate a message
  int             theNumSets;        // the number of cache sets

  // coherence state masks
  typedef unsigned short coherence_state_t;

  static const coherence_state_t kLineMask     = 0x70;  //Line state for the outside world:
  static const coherence_state_t kLineValid    = 0x10;  //    the CMP node has the line in valid mode
  static const coherence_state_t kLineWritable = 0x20;  //    the CMP node has the line in writable mode
  static const coherence_state_t kLineDirty    = 0x40;  //    the CMP node has the line in dirty mode

  static const coherence_state_t kExclusive    = 0x80;  //Line state internally:
                                                        //    an L1 cache has the line in exclusive mode

                                                                   //CMP sharers masks:
  typedef unsigned long long tSharers;                             // type definition
  static const tSharers kOwnerMask       = 0x000000000000007FULL;  //    Current owner: At most 64 nodes (64 L1i + 64 L1d)
  static const tSharers kPastSharersMask = 0;   /* CMU-ONLY */
  /* CMU-ONLY-BLOCK-BEGIN */
  //TWENISCH - PAST SHARERS hack
  // - to enable correct determination of coherence vs. replacement misses, uncomment
  //   all code marked with PAST SHARERS.  Note that this will allow the CMP
  //   to support at most 15 rather than 30 nodes.
  // FIXME: does not work in this code base
  //static const coherence_state_t kSharersMask     = 0x00000000FFFFFFFFULL;  //    Current sharers: At most 14 nodes (14 L1i + 14 L1d)
  //static const coherence_state_t kPastSharersMask = 0x0FFFFFFF00000000ULL;  //    Current sharers: At most 14 nodes (14 L1i + 14 L1d)
  /* CMU-ONLY-BLOCK-END */

  // the CMP directory structure
  struct IntHash {
    std::size_t operator()(unsigned long long key) const {
      key = key >> nFastCMPCache::theBlockSizeLog2;
      return key;
    }
  };

  typedef struct tCMPDirEntry {
    coherence_state_t theState;
    tSharers theSharers[2];
  } tCMPDirEntry;

  unsigned int theSharersBitmaskComponentSizeLog2;
  unsigned int theSharersBitmaskComponents;
  tSharers theSharersBitmaskLowMask;

  tCMPDirEntry anInvalCMPDirEntry;

  typedef __gnu_cxx::hash_map< block_address_t, tCMPDirEntry, IntHash > directory_t;
  directory_t theCMPDirectory;

  // the cache and the associated statistics
  AssociativeCache * theCache;
  CacheStats *       theStats;

  // message handlers for snoops generated at this level and evictions
  MemoryMessage theEvictMessage;
  MemoryMessage theSnoopMessage;
  ReuseDistanceSlice theReuseDistanceSlice;       /* CMU-ONLY */
  PerfectPlacementSlice thePerfectPlacementSlice; /* CMU-ONLY */

public:
  FLEXUS_COMPONENT_CONSTRUCTOR(FastCMPCache)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theEvictMessage(MemoryMessage::EvictDirty)
    , theSnoopMessage(MemoryMessage::Invalidate)
    , theReuseDistanceSlice(ReuseDistanceSlice::ProcessMemMsg)      /* CMU-ONLY */
    , thePerfectPlacementSlice(PerfectPlacementSlice::ProcessMsg)   /* CMU-ONLY */
  { }

  //InstructionOutputPort
  //=====================
  bool isQuiesced() const {
    return true;
  }

  void finalize( void ) {
    theStats->update();
  }

  void initialize(void) {
    theCMPWidth = (cfg.CMPWidth ? cfg.CMPWidth : Flexus::Core::ComponentManager::getComponentManager().systemWidth());
    static volatile bool widthPrintout = true;
    if (widthPrintout) {
      DBG_( Crit, ( << "Running with CMP width " << theCMPWidth ) );
      widthPrintout = false;
    }

    DBG_Assert ( theCMPWidth <= 64, ( << "This implementation only supports up to 64 nodes" ) );

    theStats = new CacheStats(statName());

    // sharers bitmask size for up to 64 nodes
    unsigned int theSharersBitmaskComponentSize = sizeof(tSharers) * 8;
    theSharersBitmaskComponents = 64 * 2 / theSharersBitmaskComponentSize;
    theSharersBitmaskComponentSizeLog2 = log_base2(theSharersBitmaskComponentSize);
    theSharersBitmaskLowMask = (theSharersBitmaskComponentSize-1);
    DBG_Assert(theSharersBitmaskComponents <= 2, ( << "The current implementation supports at most 64 nodes."));

    // inval dir entry
    clearDirEntry(anInvalCMPDirEntry);

    //Confirm there are enough bits in the CMP directory to hold the sharers lists
    DBG_Assert(sizeof(coherence_state_t) * 8 >= 8);  // 4 bits for the cache line state internally, 4 bits externally
    DBG_Assert(sizeof(index_t) * 8 >= 7);             // At most 64 nodes
    DBG_Assert(sizeof(long) * 8 >= 7);
    DBG_Assert(sizeof(int) * 8 >= 7);

    //Confirm that BlockSize is a power of 2
    DBG_Assert( (cfg.BlockSize & (cfg.BlockSize - 1)) == 0);
    DBG_Assert( cfg.BlockSize  >= 4);

    theNumSets = cfg.Size / cfg.BlockSize / cfg.Associativity;

    //Confirm that num_sets is a power of 2
    DBG_Assert( (theNumSets & (theNumSets  - 1)) == 0);

    //Confirm that settings are consistent
    DBG_Assert( cfg.BlockSize * theNumSets * cfg.Associativity == cfg.Size);

    //Calculate shifts and masks
    theBlockMask = ~(cfg.BlockSize - 1);
    theBlockSizeLog2 = log_base2(cfg.BlockSize);

    fillAccessTable();

    if (cfg.ReplPolicy == "LRU") {    /* CMU-ONLY */
      theCache = new AssociativeCache(cfg.BlockSize,
                                      theNumSets,
                                      cfg.Associativity,
                                      boost::bind( &FastCMPCacheComponent::evict, this, _1, _2),
                                      boost::bind( &FastCMPCacheComponent::LRU_hit, this, _1, _2, _3),
                                      boost::bind( &FastCMPCacheComponent::LRU_miss, this, _1, _2, _3),
                                      boost::bind( &FastCMPCacheComponent::LRU_victim, this, _1),
                                      boost::bind( &FastCMPCacheComponent::LRU_inval, this, _1, _2, _3)
                                     );
    /* CMU-ONLY-BLOCK-BEGIN */
    }
    else if (cfg.ReplPolicy == "ReuseDistRepl") {
      theCache = new AssociativeCache(cfg.BlockSize,
                                      theNumSets,
                                      cfg.Associativity,
                                      boost::bind( &FastCMPCacheComponent::evict, this, _1, _2),
                                      boost::bind( &FastCMPCacheComponent::ReuseDistRepl_hit, this, _1, _2, _3),
                                      boost::bind( &FastCMPCacheComponent::ReuseDistRepl_miss, this, _1, _2, _3),
                                      boost::bind( &FastCMPCacheComponent::ReuseDistRepl_victim, this, _1),
                                      boost::bind( &FastCMPCacheComponent::ReuseDistRepl_inval, this, _1, _2, _3)
                                     );
    }
    else {
      DBG_Assert(false);
    }
    /* CMU-ONLY-BLOCK-END */

    if (cfg.CleanEvictions) {
      theEvictMask = kAllEvicts;
    } else {
      theEvictMask = kDirtyEvicts;
    }

    Stat::getStatManager()->addFinalizer( boost::lambda::bind( &nFastCMPCache::FastCMPCacheComponent::finalize, this ) );

    srandom(theNumSets);
  }

  ////////////////////// from ICache
  bool available( interface::FetchRequestIn const &,
                  index_t anIndex )
  {
    return true;
  }

  void push( interface::FetchRequestIn const &,
             index_t         anIndex,
             MemoryMessage & aMessage )
  {
    DBG_( Iface, Addr(aMessage.address()) ( << " Received on Port FetchRequestIn[" << anIndex << "]"
                                            << " Request: " << aMessage
                                            << " tagset: " << std::hex << blockAddress(aMessage.address()) << std::dec ));
    aMessage.dstream() = false;
    processRequest( anIndex, aMessage );
  }

  ////////////////////// from DCache
  bool available( interface::RequestIn const &,
                  index_t anIndex)
  {
    return true;
  }

  void push( interface::RequestIn const &,
             index_t         anIndex,
             MemoryMessage & aMessage )
  {
    DBG_( Iface, Addr(aMessage.address()) ( << "Received on Port RequestIn[" << anIndex << "]"
                                            << " Request: " << aMessage
                                            << " tagset: " << std::hex << blockAddress(aMessage.address()) << std::dec ));
    aMessage.dstream() = true;
    processRequest( anIndex, aMessage );
  }

  ////////////////////// snoop port
  FLEXUS_PORT_ALWAYS_AVAILABLE(SnoopIn);
  void push( interface::SnoopIn const &,
             MemoryMessage & aMessage)
  {
    const unsigned long long anAddr = aMessage.address();
    const unsigned long long tagset = blockAddress(anAddr);
    DBG_( Iface, Addr(anAddr) ( << "Received on Port SnoopIn: " << aMessage
                                << " tagset: " << std::hex << tagset << std::dec ) );

    // notify the miss classifier /* CMU-ONLY */
    FLEXUS_CHANNEL( MissClassifier ) << aMessage; /* CMU-ONLY */

    //Get the block address
    const block_address_t blockAddr = blockAddress(anAddr);
    const unsigned int idxExtraOffsetBits = aMessage.idxExtraOffsetBits();

    DBG_( Verb, Addr(anAddr) ( << " Received on Port SnoopIn: " << aMessage
                               << std::hex << " tagset: " << tagset
                             ));

    //Lookup or insert the new block in the directory
    directory_t::iterator iter;
    bool is_new;
    tCMPDirEntry * theCMPDirEntry = NULL;
    if ( aMessage.type() != MemoryMessage::Probe ) {
      boost::tie(iter, is_new) = theCMPDirectory.insert( std::make_pair( blockAddr, anInvalCMPDirEntry) );
      theCMPDirEntry = &iter->second;
      DBG_( Verb, Addr(anAddr) ( << std::hex
                                 << " tagset: " << tagset
                                 << " CMPDir: " << theCMPDirEntry->theState << "-" << theCMPDirEntry->theSharers[1] << "-" << theCMPDirEntry->theSharers[0]
                                 << " is_new: " << is_new
                               ));
    }

    //simulate perfect placement/replacement  /* CMU-ONLY */
    processPerfPlcReq(aMessage);              /* CMU-ONLY */

    //process the snoop
    bool purge_I = false;   /* CMU-ONLY */
    bool valid_flag = false;
    bool dirty_flag = false;
    switch (aMessage.type()) {
      case MemoryMessage::ReturnReq: {
        ++theStats->theSnoops_ReturnReq;
        bool found = theCache->returnReq(tagset, idxExtraOffsetBits);
        DBG_Assert( !isExclusive(*theCMPDirEntry) );
        if (found || isInvalid(*theCMPDirEntry) ) {
          //This level has the most up-to-date data.
          //Send them over and don't bother looking above.
          theSnoopMessage.address() = PhysicalMemoryAddress(anAddr);
          theSnoopMessage.type() = MemoryMessage::ReturnReply;
        }
        else {
          returnReq(*theCMPDirEntry, blockAddr); //send snoop up
        }
        DBG_( Verb, Addr(anAddr) ( << std::hex
                                   << " New CMPDir for tagset: " << tagset
                                   << " CMPDir: " << theCMPDirEntry->theState << "-" << theCMPDirEntry->theSharers[1] << "-" << theCMPDirEntry->theSharers[0]
                                   << std::dec ));
        break;
      }
      case MemoryMessage::Invalidate: {
        ++theStats->theSnoops_Invalidate;
        dirty_flag = theCache->invalidate(tagset, idxExtraOffsetBits);
        if (isExclusive(*theCMPDirEntry)) {
          invalidateOne(*theCMPDirEntry, blockAddr, getOwner(*theCMPDirEntry)); //send snoop up
        }
        else if (!isInvalid(*theCMPDirEntry)) {
          invalidate(*theCMPDirEntry, blockAddr); //send snoop up
        }
        else {
          //Only this level has the data.
          theSnoopMessage.address() = PhysicalMemoryAddress(anAddr);
          theSnoopMessage.type() = MemoryMessage::Invalidate;
        }
        clearDirEntry(*theCMPDirEntry);
        DBG_( Verb, Addr(anAddr) ( << std::hex
                                   << " New CMPDir for tagset: " << tagset
                                   << " CMPDir :" << theCMPDirEntry->theState << "-" << theCMPDirEntry->theSharers[1] << "-" << theCMPDirEntry->theSharers[0]
                                   << std::dec ));
        break;
      }
      case MemoryMessage::Downgrade: {
        ++theStats->theSnoops_Downgrade;
        dirty_flag = theCache->downgrade(tagset, idxExtraOffsetBits);
        DBG_Assert( ( ((theCMPDirEntry->theState & kLineMask) == kLineDirty) || ((theCMPDirEntry->theState & kLineMask) == kLineWritable)),
                    ( << " Received on Port SnoopIn: " << aMessage
                      << std::hex
                      << " tagset: " << tagset
                      << " CMPDir :" << theCMPDirEntry->theState << "-" << theCMPDirEntry->theSharers[1] << "-" << theCMPDirEntry->theSharers[0]
                      << std::dec ));
        if (isExclusive(*theCMPDirEntry)) {
          downgrade(*theCMPDirEntry, blockAddr); //send snoop up
          // theCMPDirState = kLineValid | sharerMask(getOwner(theCMPDirState)) | pastSharerMask(getOwner(theCMPDirState)); /* CMU-ONLY */
          theCMPDirEntry->theState = kLineValid;
          unsigned int owner = getOwner(*theCMPDirEntry);
          clearSharers(*theCMPDirEntry);
          addSharer(owner, *theCMPDirEntry);
        }
        else {
          // NIKOS: we also need to send a returnReq if L2 does not have the data
          // theCMPDirState = (theCMPDirState & ~kLineMask) | kLineValid;
          theCMPDirEntry->theState = (theCMPDirEntry->theState & ~kLineMask) | kLineValid;
          theSnoopMessage.address() = PhysicalMemoryAddress(anAddr);
          theSnoopMessage.type() = MemoryMessage::Downgrade;
        }
        DBG_( Verb, Addr(anAddr) ( << std::hex
                                   << " New CMPDir for tagset: " << tagset
                                   << " CMPDir:" << theCMPDirEntry->theState << "-" << theCMPDirEntry->theSharers[1] << "-" << theCMPDirEntry->theSharers[0]
                                   << std::dec ));
        break;
      }
      /* CMU-ONLY-BLOCK-BEGIN */
      case MemoryMessage::PurgeIReq:
        purge_I = true;
      case MemoryMessage::PurgeDReq: {
        // NOTE: Make sure purge does not invalidate iter (pointed to by theCMPDirEntry)
        ++theStats->theSnoops_Purge;
        if (isExclusive(*theCMPDirEntry)) {
          purgeOne(*theCMPDirEntry, blockAddr, getOwner(*theCMPDirEntry), purge_I); //send snoop up
        }
        else if (!isInvalid(*theCMPDirEntry)) {
          purge(*theCMPDirEntry, blockAddr, purge_I); //send snoop up
        }
        DBG_(VVerb, Addr(blockAddr) ( << std::hex
                                      << "Preparing to purge " << blockAddr
                                      << " tagset " << tagset
                                      << " CMPDir:" << theCMPDirEntry->theState << "-" << theCMPDirEntry->theSharers[1] << "-" << theCMPDirEntry->theSharers[0]
                                      << std::dec ));
        boost::tie(valid_flag, dirty_flag) = theCache->purge(tagset, idxExtraOffsetBits);
        if (valid_flag) {
          aMessage.type() = MemoryMessage::PurgeAck;
        }
        if (valid_flag) {
          ++theStats->theSnoops_PurgeWriteBack;
        }
        // the purge causes an evict, which removes the CMPDir entry.
        // clearDirEntry(theCMPDirState);
        break;
      }
      /* CMU-ONLY-BLOCK-END */
      case MemoryMessage::Probe: {
        theSnoopMessage = aMessage;
        theSnoopMessage.type() = theCache->probe(tagset, idxExtraOffsetBits);
        break;
      }

      /* CMU-ONLY-BLOCK-BEGIN */
      // Here is how we can get an EvictCleanNonAllocating request:
      // An L1 evicts a line with EvictClean. This line reaches the network control which checks
      // (using a Probe) is this line currently resides at another cache on chip (i.e., it is shared).
      // If not, the control flips a coin. If the coin says "allocate", the control sends the EvictClean down the hierarchy.
      // If the coin says "do not allocate", the control sends an EvictCleanNonAllocating down the hierarchy.
      // If the EvictCleanNonAllocating hits at the next level, it is absorbed there.
      // If it does not hit, the recipient cache sends the EvictCleanNonAllocating to the network.
      // The network control searches for a match or an empty frame on other caches.
      // If one is found, it sends the message to this cache through the snoop channel.
      // Few!
      case MemoryMessage::EvictCleanNonAllocating: {
        access_t access_type = kCleanEvictAllocOnFreeAccess;
        access_t ret_access_type = access_type;
        const unsigned int idxExtraOffsetBits = aMessage.idxExtraOffsetBits();

        //Perform the access - will update access_type to indicate what further actions to take
        unsigned long long * frame_ptr = theCache->access(tagset
                                                          , ret_access_type
                                                          , coherenceState2AccessType(*theCMPDirEntry)
                                                          , idxExtraOffsetBits
                                                         );
        DBG_Assert( (ret_access_type != kPoison),
                    ( << "Message: " << aMessage
                      << std::hex
                      << " access: " << access_type
                      << " CMPDirState: " << theCMPDirEntry->theState << "-" << theCMPDirEntry->theSharers[1] << "-" << theCMPDirEntry->theSharers[0]
                      << " (" << coherenceState2AccessType(*theCMPDirEntry) << ")"
                      << " state: " << *frame_ptr
                      << std::dec ) );

        theStats->theRemoteAllocs ++; // count stats

        // patch up the state
        if ((*frame_ptr & kState) == kAllocOnFreeFrameState) {
          *frame_ptr = (*frame_ptr & ~kState) | kValid;
        }
        // patch up the external state
        if ((theCMPDirEntry->theState & kLineMask) == 0ULL) {
          theCMPDirEntry->theState |= kLineValid;
        }
        if ((ret_access_type & kState) == kInvalid) {
          DBG_Assert( false ); // FIXME: remove probe + evict sequence. Perform all actions on evict snoop.
        }

        DBG_ (Verb, Addr(anAddr) ( << " Message: " << aMessage
                                   << std::hex
                                   << " access: " << access_type
                                   << " CMPDirState: " << theCMPDirEntry->theState << "-" << theCMPDirEntry->theSharers[1] << "-" << theCMPDirEntry->theSharers[0]
                                   << " (" << coherenceState2AccessType(*theCMPDirEntry) << ")"
                                   << " state: " << *frame_ptr
                                   << std::dec ) );
        break;
      }

      case MemoryMessage::EvictDirtyNonAllocating: {
        access_t access_type = kDirtyEvictAllocOnFreeAccess;
        access_t ret_access_type = access_type;
        const unsigned int idxExtraOffsetBits = aMessage.idxExtraOffsetBits();

        //Perform the access - will update access_type to indicate what further actions to take
        unsigned long long * frame_ptr = theCache->access(tagset
                                                          , ret_access_type
                                                          , coherenceState2AccessType(*theCMPDirEntry)
                                                          , idxExtraOffsetBits
                                                         );
        DBG_Assert( (ret_access_type != kPoison),
                    ( << "Message: " << aMessage
                      << std::hex
                      << " access: " << access_type
                      << " CMPDirState: " << theCMPDirEntry->theState << "-" << theCMPDirEntry->theSharers[1] << "-" << theCMPDirEntry->theSharers[0]
                      << " (" << coherenceState2AccessType(*theCMPDirEntry) << ")"
                      << " state: " << *frame_ptr
                      << std::dec ) );

        theStats->theRemoteAllocs ++; // count stats

        // patch up the state
        if ((*frame_ptr & kState) == kAllocOnFreeFrameState) {
          *frame_ptr = (*frame_ptr & ~kState) | kValid | kDirty;
        }

        // patch up the external state
        theCMPDirEntry->theState = kLineDirty;
        clearSharers(*theCMPDirEntry);

        if ((ret_access_type & kState) == kInvalid) {
          DBG_Assert( false ); // FIXME: remove probe + evict sequence. Perform all actions on evict snoop.
        }

        DBG_ (Verb, Addr(anAddr) ( << " Message: " << aMessage
                                   << std::hex
                                   << " access: " << access_type
                                   << " CMPDirState: " << theCMPDirEntry->theState << "-" << theCMPDirEntry->theSharers[1] << "-" << theCMPDirEntry->theSharers[0]
                                   << " (" << coherenceState2AccessType(*theCMPDirEntry) << ")"
                                   << " state: " << *frame_ptr
                                   << std::dec ) );
        break;
      }
      /* CMU-ONLY-BLOCK-END */

      default:
        DBG_Assert( (false), ( << "Message: " << aMessage ) ); //Unhandled message type
    }
    DBG_( Verb, Addr(anAddr) ( << " Reply from front: " << theSnoopMessage << " tagset: " << std::hex << tagset << std::dec ));

    //check reply
    switch (theSnoopMessage.type()) {
      case MemoryMessage::ReturnReq:
        //Nothing interesting above
        aMessage.type() = MemoryMessage::ReturnReply;
        break;
      /* CMU-ONLY-BLOCK-BEGIN */
      case MemoryMessage::PurgeIReq:
      case MemoryMessage::PurgeDReq:
        //Nothing interesting above
        // aMessage.type() = MemoryMessage::PurgeAck;
        break;
      /* CMU-ONLY-BLOCK-END */
      case MemoryMessage::ReturnReply:
      case MemoryMessage::InvUpdateAck:
      case MemoryMessage::DownUpdateAck:
    case MemoryMessage::PurgeAck:      /* CMU-ONLY */
        aMessage.type() = theSnoopMessage.type();
        break;
      case MemoryMessage::Invalidate:
        //Nothing interesting above
      case MemoryMessage::InvalidateAck:
        if (dirty_flag) {
          aMessage.type() = MemoryMessage::InvUpdateAck;
        }
        else {
          aMessage.type() = MemoryMessage::InvalidateAck;
        }
        break;
      case MemoryMessage::Downgrade:
        //Nothing interesting above
      case MemoryMessage::DowngradeAck:
        if (dirty_flag) {
          aMessage.type() = MemoryMessage::DownUpdateAck;
        }
        else {
          aMessage.type() = MemoryMessage::DowngradeAck;
        }
        break;
      case MemoryMessage::ProbedNotPresentSetNotFull:
      case MemoryMessage::ProbedNotPresent:
      case MemoryMessage::ProbedClean:
      case MemoryMessage::ProbedDirty:
      case MemoryMessage::ProbedWritable:
        aMessage.type() = theSnoopMessage.type();
        break;
      default:
        DBG_Assert( (false), ( << "Message: " << aMessage ) ); //Unhandled message type

    }
    DBG_( Verb, Addr(anAddr) ( << " Reply to back: " << aMessage << " tagset: " << std::hex << tagset << std::dec ));
  }

  void drive( interface::UpdateStatsDrive const &) {
    theStats->update();
  }

 private:
  /////////////////////////////////////////////////////////////////////////////////////
  // Helper functions

  inline block_address_t blockAddress(const unsigned long long addr) const {
    return (addr & theBlockMask);
  }

  ////// cache indexing
  inline index_t getCacheIdx(const index_t aNodeIdx,
                             const bool    isData) const
  {
    // For software coherent ICaches
    // return( aNodeIdx );
    return( (aNodeIdx << 1) | isData );
  }

  inline index_t getCoreIdxFromCacheIdx(const index_t aCacheIdx) const
  {
    // For software coherent ICaches
    // return( aCacheIdx );
    return ( aCacheIdx >> 1 );
  }

  ////// differentiate icache from dcache requests
  inline bool isDCache(const index_t aCacheIdx) const
  {
    // For software coherent ICaches
    // return true;
    return (aCacheIdx & 1);
  }

  ////// directory state maintenance
  void clearDirEntry( tCMPDirEntry & theCMPDirEntry ) {
    theCMPDirEntry.theState = 0;
    theCMPDirEntry.theSharers[0] = 0ULL;
    theCMPDirEntry.theSharers[1] = 0ULL;
  }

  void clearSharers( tCMPDirEntry & theCMPDirEntry ) {
    theCMPDirEntry.theSharers[0] = 0ULL;
    theCMPDirEntry.theSharers[1] = 0ULL;
  }

  void addSharer( const index_t aNode
                  , tCMPDirEntry & theCMPDirEntry
                ) 
  {
    unsigned int idx = aNode >> theSharersBitmaskComponentSizeLog2;
    unsigned int pos = aNode & theSharersBitmaskLowMask;
    theCMPDirEntry.theSharers[idx] |= (1ULL << pos);
  }

  void addSharer( const index_t aNode
                  , const bool  isData
                  , tCMPDirEntry & theCMPDirEntry
                )
  {
    addSharer( getCacheIdx(aNode, isData), theCMPDirEntry );
  }

  void rmSharer( const index_t aNode
                 , tCMPDirEntry & theCMPDirEntry
               ) 
  {
    unsigned int idx = aNode >> theSharersBitmaskComponentSizeLog2;
    unsigned int pos = aNode & theSharersBitmaskLowMask;
    theCMPDirEntry.theSharers[idx] &= ~(1ULL << pos);
  }

  void rmSharer( const index_t aNode
                 , const bool  isData
                 , tCMPDirEntry & theCMPDirEntry
               ) 
  {
    rmSharer( getCacheIdx(aNode, isData), theCMPDirEntry );
  }

  bool isSharer( const index_t aNode
                 , const tCMPDirEntry & theCMPDirEntry
               ) const
  {
    unsigned int idx = aNode >> theSharersBitmaskComponentSizeLog2;
    unsigned int pos = aNode & theSharersBitmaskLowMask;
    return ((theCMPDirEntry.theSharers[idx] & (1ULL << pos)) != 0ULL);
  }

  bool isSharer( const index_t aNode
                 , const bool isData
                 , const tCMPDirEntry & theCMPDirEntry
               ) const
  {
    return isSharer( getCacheIdx(aNode, isData), theCMPDirEntry );
  }

  bool someSharers( const tCMPDirEntry & theCMPDirEntry ) const
  {
    return (theCMPDirEntry.theSharers[0]!=0ULL || theCMPDirEntry.theSharers[1]!=0ULL);
  }

  unsigned int getOwner( const tCMPDirEntry & theCMPDirEntry ) const
  {
    return (theCMPDirEntry.theSharers[0]);
  }

  void setOwner(const index_t aNode
                , tCMPDirEntry & theCMPDirEntry
               ) const
  {
    theCMPDirEntry.theSharers[0] = aNode;
    theCMPDirEntry.theSharers[1] = 0ULL;
  }

  bool isExclusive( const tCMPDirEntry & theCMPDirEntry ) const
  {
    return( theCMPDirEntry.theState & kExclusive );
  }

  bool isInvalid( const tCMPDirEntry & theCMPDirEntry ) const
  {
    const bool isExcl = theCMPDirEntry.theState & kExclusive;
    bool isShr = false;
    for (unsigned int i=0; i < theSharersBitmaskComponents; i++) {
      if (theCMPDirEntry.theSharers[i] != 0ULL) {
        isShr = true;
        break;
      }
    }
    return (!isExcl && !isShr);
  }

  access_t coherenceState2AccessType( const tCMPDirEntry & theCMPDirEntry ) const
  {
    if (isInvalid(theCMPDirEntry)) {
      return kCMPDirInvalid;
    }
    else if (isExclusive(theCMPDirEntry)) {
      return kCMPDirExclusive;
    }
    else {
      return kCMPDirShared;
    }
  }

  /* CMU-ONLY-BLOCK-BEGIN */
  ////// aux channels
  inline void collectReuseDistStats( MemoryMessage & aMessage ) {
    if (cfg.ReuseDistStats) {
      theReuseDistanceSlice.type() = ReuseDistanceSlice::ProcessMemMsg;
      theReuseDistanceSlice.address() = aMessage.address();
      theReuseDistanceSlice.memMsg() = aMessage;
      FLEXUS_CHANNEL( ReuseDist ) << theReuseDistanceSlice;
    }
  }

  inline void processPerfPlcReq( MemoryMessage & aMessage ) {
    if (cfg.PerfectPlacement) {
      thePerfectPlacementSlice.type() = PerfectPlacementSlice::ProcessMsg;
      thePerfectPlacementSlice.address() = aMessage.address();
      thePerfectPlacementSlice.memMsg() = aMessage;
      FLEXUS_CHANNEL( PerfPlc ) << thePerfectPlacementSlice;
    }
  }

  inline void processPerfPlcState( MemoryMessage & aMessage, const unsigned long long * const frame_ptr ) {
    if (cfg.PerfectPlacement) {
      unsigned long long new_block_state = theCache->getState(frame_ptr);
      if (new_block_state == kWritable || new_block_state == kDirty) {
        thePerfectPlacementSlice.type() = PerfectPlacementSlice::MakeBlockWritable;
        thePerfectPlacementSlice.address() = aMessage.address();
        FLEXUS_CHANNEL( PerfPlc ) << thePerfectPlacementSlice;
      }
    }
  }
  /* CMU-ONLY-BLOCK-END */

  /////////////////////////////////////////////////////////////////////////////////////
  // Replacement functions

  ////// LRU specific
  void LRU_hit( unsigned long long * const set_base, const int hit_way, const unsigned long long hit_tag ) {
    //Slide other set contents down MRU chain
    memmove(set_base+1, set_base, sizeof(unsigned long long)*hit_way);

    //Insert this tag at head of MRU chain
    *set_base = hit_tag;
  }

  void LRU_miss( unsigned long long * const set_base, const int alloc_way, const unsigned long long new_tag ) {
    ////DBG_Assert(alloc_way == cfg.Associativity-1);

    //LRU-specific actions on hit or miss are the same (after victim is selected)
    LRU_hit( set_base, alloc_way, new_tag);
  }

  int LRU_victim( unsigned long long * const set_base ) {
    return (cfg.Associativity - 1);
  }

  void LRU_inval( unsigned long long * const set_base, const int inval_way, const unsigned long long inval_tag ) {
    unsigned long long * cache_frame = set_base + inval_way - 1;

    //Slide all sets up MRU chain
    memmove(cache_frame, cache_frame+1, sizeof(unsigned long long)*(cfg.Associativity-inval_way));

    //Set block to invalid state at MRU tail
    unsigned long long * lru_frame = set_base + (cfg.Associativity-1);
    *lru_frame = inval_tag;
  }


  /* CMU-ONLY-BLOCK-BEGIN */
  ////// Reuse-distance replacement policy specific
  //As an optimization, we still place the MRU block at the top of the set and the
  //invalidated block at the bottom, but we now select a victim based on its reuse distance
  void ReuseDistRepl_hit( unsigned long long * const set_base, const int hit_way, const unsigned long long hit_tag ) {
    //The reuse distance has already been updated

    //Optimization: place MRU block at the top of the associative set
    LRU_hit( set_base, hit_way, hit_tag );

    //Update the tag
    // unsigned long long * cache_frame = set_base + hit_way;
    // *cache_frame = hit_tag;
  }

  void ReuseDistRepl_miss( unsigned long long * const set_base, const int alloc_way, const unsigned long long new_tag ) {
    //The reuse distance has already been updated

    //ReuseDistRepl-specific actions on hit or miss are the same (after victim is selected)
    // ReuseDistRepl_hit( set_base, alloc_way, new_tag);
    LRU_miss( set_base, alloc_way, new_tag);
  }

  int ReuseDistRepl_victim( unsigned long long * const set_base ) {
    //compare the reuse distances of the blocks allocated at this set and evict the one with the largest
    long long max_reuse = 0;
    int max_reuse_idx = 0;
    theReuseDistanceSlice.type() = ReuseDistanceSlice::GetMeanReuseDist_Data;

    for (int i=0; i < cfg.Associativity; i++) {
      //invalid blocks have a reuse distance of infinity
      if ( ((*(set_base + i)) & kState ) == kInvalid ) {
        return i;
      }
      theReuseDistanceSlice.address() = static_cast<ReuseDistanceSlice::MemoryAddress>(( (*(set_base + i)) & ~kState ));
      FLEXUS_CHANNEL( ReuseDist ) << theReuseDistanceSlice;
      if (theReuseDistanceSlice.meanReuseDist() > max_reuse) {
        max_reuse = theReuseDistanceSlice.meanReuseDist();
        max_reuse_idx = i;
      }
    }

    return max_reuse_idx;
  }

  void ReuseDistRepl_inval( unsigned long long * const set_base, const int inval_way, const unsigned long long inval_tag ) {

    //Optimization: place the invalidated block at the bottom of associative set
    LRU_inval( set_base, inval_way, inval_tag );

    //Update the tag
    //*cache_frame = inval_tag;
  }
  /* CMU-ONLY-BLOCK-END */

  /////////////////////////////////////////////////////////////////////////////////////
  ////// process incoming requests
  void processRequest( const index_t   anIndex,
                       MemoryMessage & aMessage )
  {
    const index_t coreIdx    = anIndex;
    const bool    fromDCache = aMessage.isDstream(); // differentiate icache from dcache
    const index_t cacheIdx   = getCacheIdx(coreIdx, fromDCache);

    // Set the message as coming from this core
    aMessage.coreIdx() = coreIdx;

    //Create a set and tag from the message address
    unsigned long long tagset = blockAddress(aMessage.address());
    DBG_( Verb, Addr(aMessage.address()) ( << " Received on Port RequestIn[" << anIndex << "] Request: " << aMessage << " tagset: " << std::hex << tagset << std::dec ));

    //Get the block address
    block_address_t blockAddr = tagset;
    const unsigned int idxExtraOffsetBits = aMessage.idxExtraOffsetBits();

    //Map the memory message type into the internal access type
    long * hit_counter = 0;
    long * hit_os_counter = 0;
    access_t access_type = 0;
    access_t ret_access_type = 0;
    switch (aMessage.type()) {
      case MemoryMessage::FetchReq:
        access_type = kFetchAccess;
        hit_counter = &theStats->theHits_Fetch;
        hit_os_counter = &theStats->theHits_OS_Fetch;
        collectReuseDistStats(aMessage); /* CMU-ONLY */
        break;

      case MemoryMessage::LoadReq:
      //case MemoryMessage::PrefetchReadNoAllocReq:
      //case MemoryMessage::PrefetchReadAllocReq:
        access_type = kLoadAccess;
        hit_counter = &theStats->theHits_Read;
        hit_os_counter = &theStats->theHits_OS_Read;
        collectReuseDistStats(aMessage); /* CMU-ONLY */
        break;

      case MemoryMessage::StoreReq:
      case MemoryMessage::StorePrefetchReq:
        access_type = kStoreAccess;
        hit_counter = &theStats->theHits_Write;
        hit_os_counter = &theStats->theHits_OS_Write;
        collectReuseDistStats(aMessage); /* CMU-ONLY */
        break;

      case MemoryMessage::RMWReq:
      case MemoryMessage::CmpxReq:
        access_type = kStoreAccess;
        hit_counter = &theStats->theHits_Atomic;
        hit_os_counter = &theStats->theHits_OS_Atomic;
        collectReuseDistStats(aMessage); /* CMU-ONLY */
        break;

      case MemoryMessage::ReadReq:
        access_type = kReadAccess;
        hit_counter = &theStats->theHits_Read;
        hit_os_counter = &theStats->theHits_OS_Read;
        collectReuseDistStats(aMessage); /* CMU-ONLY */
        break;

      case MemoryMessage::WriteReq:
      case MemoryMessage::WriteAllocate:
        access_type = kWriteAccess;
        hit_counter = &theStats->theHits_Write;
        hit_os_counter = &theStats->theHits_OS_Write;
        collectReuseDistStats(aMessage); /* CMU-ONLY */
        break;

      case MemoryMessage::UpgradeReq:
      case MemoryMessage::UpgradeAllocate:
        access_type = kUpgradeAccess;
        hit_counter = &theStats->theHits_Upgrade;
        hit_os_counter = &theStats->theHits_OS_Upgrade;
        collectReuseDistStats(aMessage); /* CMU-ONLY */
        break;

      case MemoryMessage::EvictDirty:
        access_type = kDirtyEvictAccess;
        hit_counter = &theStats->theHits_Evict;
        if (aMessage.isPriv()) {
          hit_os_counter = &theStats->theHits_OS_Evict;
        }
        break;

      case MemoryMessage::EvictWritable:
        access_type = kWritableEvictAccess;
        hit_counter = &theStats->theHits_Evict;
        if (aMessage.isPriv()) {
          hit_os_counter = &theStats->theHits_OS_Evict;
        }
        break;

      case MemoryMessage::EvictClean:
      case MemoryMessage::PrefetchInsert:
        // For software coherent ICaches
        //
        // if (fromDCache) {
        //   access_type = kCleanEvictAccess;
        // }
        // else {
        //   access_type = kICacheEvictAccess;
        // }
        access_type = kCleanEvictAccess;
        hit_counter = &theStats->theHits_Evict;
        if (aMessage.isPriv()) {
          hit_os_counter = &theStats->theHits_OS_Evict;
        }
        break;

      /* CMU-ONLY-BLOCK-BEGIN */
      case MemoryMessage::EvictCleanNonAllocating:
        access_type = kCleanEvictNonAllocatingAccess;
        hit_counter = &theStats->theHits_Evict;
        if (aMessage.isPriv()) {
          hit_os_counter = &theStats->theHits_OS_Evict;
        }
        break;
      /* CMU-ONLY-BLOCK-END */

      default:
        DBG_Assert( (false), ( << "Message: " << aMessage ) ); //Unhandled message type
    }

    //simulate perfect placement/replacement  /* CMU-ONLY */
    processPerfPlcReq(aMessage);              /* CMU-ONLY */

    //Lookup or insert the new block in the directory
    directory_t::iterator iter;
    bool is_new;
    boost::tie(iter, is_new) = theCMPDirectory.insert( std::make_pair( blockAddr, anInvalCMPDirEntry ) ); //default is invalid, to simplify the code
    tCMPDirEntry & theCMPDirEntry = iter->second;
    tCMPDirEntry oldCMPDirEntry = theCMPDirEntry;

    //Perform the access - will update access_type to indicate what further actions to take
    ret_access_type = access_type;
    unsigned long long * frame_ptr = theCache->access(tagset
                                                 , ret_access_type
                                                 , coherenceState2AccessType(theCMPDirEntry)
                                                 , idxExtraOffsetBits
                                                );
    DBG_Assert( (ret_access_type != kPoison),
                ( << "Message: " << aMessage
                << std::hex
                << " access: " << access_type
                << " CMPDirState: " << theCMPDirEntry.theState << "-" << theCMPDirEntry.theSharers[1] << "-" << theCMPDirEntry.theSharers[0]
                << " (" << coherenceState2AccessType(theCMPDirEntry) << ")"
                << " state: " << *frame_ptr
                << std::dec ) );
    access_t snoop       = ret_access_type & kSnoop;
    access_t fill        = ret_access_type & kFill;
    access_t miss        = ret_access_type & kMiss;
    access_t next_state  = ret_access_type & kState;

    DBG_ (Verb, Addr(aMessage.address()) ( << " Message: " << aMessage
                                           << std::hex
                                           << " access: " << access_type
                                           << " CMPDirState: " << theCMPDirEntry.theState << "-" << theCMPDirEntry.theSharers[1] << "-" << theCMPDirEntry.theSharers[0]
                                           << " (" << coherenceState2AccessType(theCMPDirEntry) << ")"
                                           << " state: " << *frame_ptr
                                           << " is_new: " << is_new
                                           << std::dec ) );

    /////////// Trace hits/misses
    // std::cout << std::hex << statName() << " A= " << tagset << " H= " << (!miss) << std::dec << "\n";


    //Determine fill type
    if (! miss ) {
      if (is_new) {
        //Must be off chip.  Bus component will fill it in.  Default to eCold so that we catch any mistakes
        aMessage.fillType() = eCold;        
      } else {
        aMessage.fillType() = eCoherence;          
        /* CMU-ONLY-BLOCK-BEGIN */
        if (0 /* isPastSharer(theCMPDirEntry, cacheIdx) */ ) { //fixme
          aMessage.fillType() = eReplacement;          
        }
        /* CMU-ONLY-BLOCK-END */
      }
    }

    ////////////////send the snoop to the previous cache level
    // also fix the CMPDir state
    MemoryMessage::MemoryMessageType snoopReplyType;
    switch ( snoop ) {

      case kNoSnoop:
        //do nothing;
        if (! miss) {
          aMessage.fillLevel() = cfg.CacheLevel;
        }
        break;

      case kReturnReq: {
        returnReq(theCMPDirEntry, blockAddr);
        DBG_ ( Iface, Addr(aMessage.address()) ( << "ReturnReq sent to one of "
                                                 << std::hex
                                                 << theCMPDirEntry.theSharers[1] << "-" << theCMPDirEntry.theSharers[0]
                                                 << std::dec
                                                 << " reply: " << theSnoopMessage ) );
        DBG_Assert( !miss );
        if (fromDCache) {
          ++theStats->theHits_Read_HierarchyReturnReq;
          if (aMessage.isPriv()) {
            ++theStats->theHits_OS_Read_HierarchyReturnReq;
          }
        }
        else {
          ++theStats->theHits_Fetch_HierarchyReturnReq;
          if (aMessage.isPriv()) {
            ++theStats->theHits_OS_Fetch_HierarchyReturnReq;
          }
        }
        aMessage.fillLevel() = ePeerL1Cache;
        //new sharer in CMPDir
        // For software coherent ICaches
        // theCMPDirState |= (fromDCache ? sharerMask(cacheIdx) : 0ULL);
        // theCMPDirState |= sharerMask(cacheIdx) | pastSharerMask(cacheIdx); /* CMU-ONLY */
        addSharer(cacheIdx, theCMPDirEntry);
        break;
      }
      case kInvalidate: {
        if (isExclusive(theCMPDirEntry)) {
          //Write in exclusive state.
          unsigned int old_owner = getOwner(theCMPDirEntry);
          DBG_Assert(old_owner != cacheIdx);
          invalidateOne(theCMPDirEntry, blockAddr, old_owner);
          DBG_ ( Iface, Addr(aMessage.address()) ( << "invalidateOne sent to " << old_owner << " reply: " << theSnoopMessage ) );
          ++theStats-> theHits_Write_HierarchyExclusive;
          if (aMessage.isPriv()) {
            ++theStats-> theHits_OS_Write_HierarchyExclusive;
          }
          DBG_Assert( !miss );
        }
        else {
          //Write in shared state.
          // coherence_state_t other_sharers = theCMPDirState & ~sharerMask(cacheIdx) & kSharersMask; //Do not invalidate the writer
          tCMPDirEntry aCMPDirEntry = theCMPDirEntry;
          rmSharer(cacheIdx, aCMPDirEntry);
          DBG_ ( VVerb, Addr(aMessage.address()) ( << std::hex
                                                   << "theCMPDirState= " << aCMPDirEntry.theState << "-" << aCMPDirEntry.theSharers[1] << "-" << aCMPDirEntry.theSharers[0]
                                                   << " cacheIdx= " << cacheIdx
                                                   << std::dec ));
          if (someSharers(aCMPDirEntry)) {
            invalidate(aCMPDirEntry, blockAddr);
            DBG_ ( Iface, Addr(aMessage.address()) ( << "invalidate sent to one of "
                                                     << std::hex
                                                     << aCMPDirEntry.theSharers[1] << "-" << theCMPDirEntry.theSharers[0]
                                                     << std::dec
                                                     << " reply: " << theSnoopMessage ) );
          }
        }
        aMessage.fillLevel() = cfg.CacheLevel;
        //CMPDir in exclusive state
        // theCMPDirState = (theCMPDirState & kLineMask) | kExclusive | cacheIdx;
        theCMPDirEntry.theState = (theCMPDirEntry.theState & kLineMask) | kExclusive;
        setOwner(cacheIdx, theCMPDirEntry);
        if (theSnoopMessage.type() == MemoryMessage::InvUpdateAck) {
          theCMPDirEntry.theState = (theCMPDirEntry.theState & ~kLineMask) | kLineDirty;
        }
        break;
      }
      case kReturnInval: {
        tCMPDirEntry aCMPDirEntry = theCMPDirEntry;
        rmSharer(cacheIdx, aCMPDirEntry);
        DBG_Assert ( someSharers(theCMPDirEntry),
                     ( << "return-invalidate sent to " << std::hex << theCMPDirEntry.theState << std::dec
                       << " reply: " << theSnoopMessage ) ); //requestor can not have a copy already
        returnReq(theCMPDirEntry, blockAddr);
        invalidate(theCMPDirEntry, blockAddr);
        DBG_ ( Iface, Addr(aMessage.address()) ( << "return-invalidate sent to one of " << std::hex << theCMPDirEntry.theSharers[1] << "-" << theCMPDirEntry.theSharers[0] << std::dec << " reply: " << theSnoopMessage ) );
        //CMPDir in exclusive state
        // theCMPDirState = (theCMPDirState & kLineMask) | kExclusive | cacheIdx;
        theCMPDirEntry.theState = (theCMPDirEntry.theState & kLineMask) | kExclusive;
        setOwner(cacheIdx, theCMPDirEntry);
        ++theStats->theMisses_Upgrade_HierarchyWrMiss;
        if (aMessage.isPriv()) {
          ++theStats->theMisses_OS_Upgrade_HierarchyWrMiss;
        }
        aMessage.fillLevel() = cfg.CacheLevel;
        break;
      }
      case kDowngrade: {
        unsigned int old_owner = getOwner(theCMPDirEntry);
        // For software coherent ICaches
        // DBG_Assert( (old_owner != coreIdx) || !fromDCache);
        DBG_Assert( old_owner != cacheIdx ,
                    ( << " old_owner=" << old_owner
                      << " cacheIdx=" << cacheIdx
                      << " CMPDir=" << std::hex << theCMPDirEntry.theState << "-" << theCMPDirEntry.theSharers[1] << "-" << theCMPDirEntry.theSharers[0]
                      << " Message: " << aMessage
                  ) );
        downgrade(theCMPDirEntry, blockAddr);
        DBG_ ( Iface, Addr(aMessage.address()) ( << "downgrade sent to " << old_owner << " reply: " << theSnoopMessage ) );
        DBG_Assert( !miss );
        if (fromDCache) {
          ++theStats->theHits_Read_HierarchyDowngrade;
          if (aMessage.isPriv()) {
            ++theStats->theHits_OS_Read_HierarchyDowngrade;
          }
        }
        else {
          ++theStats->theHits_Fetch_HierarchyDowngrade;
          if (aMessage.isPriv()) {
            ++theStats->theHits_OS_Fetch_HierarchyDowngrade;
          }
        }
        //CMPDir in shared state
        // For software coherent ICaches
        // theCMPDirState = (theCMPDirState & kLineMask) | sharerMask(old_owner) | (fromDCache ? sharerMask(cacheIdx) : 0ULL);
        // theCMPDirState = (theCMPDirState & kLineMask) | sharerMask(old_owner) | sharerMask(cacheIdx) | pastSharerMask(old_owner) | pastSharerMask(cacheIdx); /* CMU-ONLY */
        theCMPDirEntry.theState = (theCMPDirEntry.theState & kLineMask);
        clearSharers(theCMPDirEntry);
        addSharer(old_owner, theCMPDirEntry);
        addSharer(cacheIdx, theCMPDirEntry);
        if (theSnoopMessage.type() == MemoryMessage::DownUpdateAck) {
          theCMPDirEntry.theState = (theCMPDirEntry.theState & ~kLineMask) | kLineDirty;
        }
        aMessage.fillLevel() = ePeerL1Cache;
        break;
      }
      default:
        DBG_Assert( false, ( << std::hex
                             << "snoop: " << snoop
                             << " theCMPDirState= " << theCMPDirEntry.theState << "-" << theCMPDirEntry.theSharers[1] << "-" << theCMPDirEntry.theSharers[0]
                             << std::dec
                             << " msg: " << aMessage ) );
    }
    //keep the snoop reply type, it may be needed below.
    snoopReplyType = theSnoopMessage.type();

    ////////////////send the miss, if there is one
    MemoryMessage::MemoryMessageType missReplyType = MemoryMessage::MissReply;
    if (!miss) {
      DBG_( Iface, Addr(aMessage.address()) ( << "Hit: " << aMessage ));
      ++(*hit_counter);
      if (aMessage.isPriv()) {
        ++(*hit_os_counter);
      }
      aMessage.fillLevel() = cfg.CacheLevel;
      switch (access_type) {
        case kFetchAccess:
          DBG_( Verb, Addr(aMessage.address()) ( << "Fetch Hit: " << aMessage ));
          // Remove if software coherent ICaches
          // theCMPDirState |= sharerMask(cacheIdx) | pastSharerMask(cacheIdx); /* CMU-ONLY */
          addSharer(cacheIdx, theCMPDirEntry);
          notifyFetch( aMessage );
          break;
        case kReadAccess:
          DBG_( Verb, Addr(aMessage.address()) ( << "Read Hit: " << aMessage ));
          // theCMPDirState |= sharerMask(cacheIdx) | pastSharerMask(cacheIdx); /* CMU-ONLY */
          addSharer(cacheIdx, theCMPDirEntry);
          notifyRead( aMessage );
          break;
        case kWriteAccess:
          DBG_( Verb, Addr(aMessage.address()) ( << "Write Hit: " << aMessage ));
          // theCMPDirState = (theCMPDirState & kLineMask) | kExclusive | cacheIdx;
          theCMPDirEntry.theState = (theCMPDirEntry.theState  & kLineMask) | kExclusive;
          setOwner(cacheIdx, theCMPDirEntry);
          notifyWrite( aMessage );
          break;
        case kUpgradeAccess:
          DBG_( Verb, Addr(aMessage.address()) ( << "Upgrade Hit: " << aMessage ));
          // theCMPDirState = (theCMPDirState & kLineMask) | kExclusive | cacheIdx;
          theCMPDirEntry.theState = (theCMPDirEntry.theState  & kLineMask) | kExclusive;
          setOwner(cacheIdx, theCMPDirEntry);
          notifyWrite( aMessage );
          break;

        case kDirtyEvictAccess:
          DBG_( Verb, Addr(aMessage.address()) ( << "DirtyEvict Hit: " << aMessage ));
          // theCMPDirState = kLineDirty;
          theCMPDirEntry.theState = kLineDirty;
          clearSharers(theCMPDirEntry);
          notifyL1DirtyEvict( aMessage );
          break;
        case kWritableEvictAccess:
          DBG_( Verb, Addr(aMessage.address()) ( << "WritableEvict Hit: " << aMessage ));
          // theCMPDirState = (theCMPDirState & kLineMask);
          theCMPDirEntry.theState = (theCMPDirEntry.theState & kLineMask);
          clearSharers(theCMPDirEntry);
          notifyL1DirtyEvict( aMessage );
          break;
        case kCleanEvictNonAllocatingAccess: /* CMU-ONLY */
        case kCleanEvictAccess:
          DBG_( Verb, Addr(aMessage.address()) ( << "CleanEvict Hit: " << aMessage ));
          // theCMPDirState &= ~sharerMask(cacheIdx);
          rmSharer(cacheIdx, theCMPDirEntry);
          if (fromDCache) {
            notifyL1CleanEvict( aMessage );
          }
          else {
            notifyL1IEvict( aMessage );
          }
          break;
        // For software coherent ICaches
        //
        // case kICacheEvictAccess:
        //   DBG_( Verb, Addr(aMessage.address()) ( << "ICacheEvict Hit: " << aMessage ));
        //   if (theCMPDirState == 0ULL) {
        //     theCMPDirState = kLineValid;
        //   }
        //   break;
        default:
          DBG_Assert( false );
      }
    }
    else {
      //Handle misses
      switch (miss) {
        case kReadMiss:
          DBG_( Iface, Addr(aMessage.address()) ( << "Read Miss: " << aMessage ));
          ++theStats->theMisses_Read;
          if (aMessage.isPriv()) {
            ++theStats->theMisses_OS_Read;
          }
          aMessage.type() = MemoryMessage::ReadReq;
          FLEXUS_CHANNEL( MissClassifier ) << aMessage;  /* CMU-ONLY */
          FLEXUS_CHANNEL( RequestOut ) << aMessage;
          if (   aMessage.type() == MemoryMessage::MissReplyWritable
              || aMessage.type() == MemoryMessage::MissReplyDirty)
          {
            DBG_( VVerb, ( << "Read Miss with ReplyWritable: " << aMessage ));
            DBG_Assert(fill == kMissDependantFill);
            // theCMPDirState = kExclusive | cacheIdx;
            theCMPDirEntry.theState = kExclusive;
            setOwner(cacheIdx, theCMPDirEntry);
          }
          else {
            // theCMPDirState |= sharerMask(cacheIdx) | pastSharerMask(cacheIdx); /* CMU-ONLY */
            addSharer(cacheIdx, theCMPDirEntry);
          }
          notifyRead( aMessage );
          break;
        case kWriteMiss:
          DBG_( Iface, Addr(aMessage.address()) ( << "Write Miss: " << aMessage ));
          ++theStats->theMisses_Write;
          if (aMessage.isPriv()) {
            ++theStats->theMisses_OS_Write;
          }
          aMessage.type() = MemoryMessage::WriteReq;
          FLEXUS_CHANNEL( MissClassifier ) << aMessage;  /* CMU-ONLY */
          FLEXUS_CHANNEL( RequestOut ) << aMessage;
          // theCMPDirState = kExclusive | cacheIdx;
          theCMPDirEntry.theState = kExclusive;
          setOwner(cacheIdx, theCMPDirEntry);
          notifyWrite( aMessage );
          break;
        case kUpgradeMiss:
          DBG_( Iface, Addr(aMessage.address()) ( << "Upgrade Miss: " << aMessage ));
          ++theStats->theMisses_Upgrade;
          if (aMessage.isPriv()) {
            ++theStats->theMisses_OS_Upgrade;
          }
          aMessage.type() = MemoryMessage::UpgradeReq;
          FLEXUS_CHANNEL( MissClassifier ) << aMessage;  /* CMU-ONLY */
          FLEXUS_CHANNEL( RequestOut ) << aMessage;
          // theCMPDirState = kExclusive | cacheIdx;
          theCMPDirEntry.theState = kExclusive;
          setOwner(cacheIdx, theCMPDirEntry);
          notifyWrite( aMessage );
          break;
        case kFetchMiss:
          DBG_( Iface, Addr(aMessage.address()) ( << "Fetch Miss: " << aMessage ));
          ++theStats->theMisses_Fetch;
          if (aMessage.isPriv()) {
            ++theStats->theMisses_OS_Fetch;
          }
          aMessage.type() = MemoryMessage::FetchReq;
          FLEXUS_CHANNEL( MissClassifier ) << aMessage;   /* CMU-ONLY */
          FLEXUS_CHANNEL( RequestOut ) << aMessage;
          // Remove if software coherent ICaches
          // theCMPDirState |= sharerMask(cacheIdx) | pastSharerMask(cacheIdx); /* CMU-ONLY */
          addSharer(cacheIdx, theCMPDirEntry);
          notifyFetch( aMessage );
          break;
        case kLineDependantMiss:
          DBG_( Iface, Addr(aMessage.address()) ( << "Line-dependant Miss: " << aMessage ));
          if (   (oldCMPDirEntry.theState & kLineMask) == kLineWritable
              || (oldCMPDirEntry.theState & kLineMask) == kLineDirty
             )
          {
            DBG_( Iface, Addr(aMessage.address()) ( << "Upgrade Miss hit on chip: " << aMessage ));
            FLEXUS_CHANNEL( MissClassifier ) << aMessage;  /* CMU-ONLY */
            aMessage.type() = MemoryMessage::MissReplyWritable;
          }
          else {
            DBG_( Iface, Addr(aMessage.address()) ( << "Upgrade Miss: " << aMessage ));
            ++theStats->theMisses_Upgrade;
            if (aMessage.isPriv()) {
              ++theStats->theMisses_OS_Upgrade;
            }
            aMessage.type() = MemoryMessage::UpgradeReq;
            FLEXUS_CHANNEL( MissClassifier ) << aMessage;  /* CMU-ONLY */
            FLEXUS_CHANNEL( RequestOut ) << aMessage;
          }
          // theCMPDirState = kExclusive | cacheIdx;
          theCMPDirEntry.theState = kExclusive;
          setOwner(cacheIdx, theCMPDirEntry);
          notifyWrite( aMessage );
          break;

        /* CMU-ONLY-BLOCK-BEGIN */
        case kCleanEvictNonAllocatingMiss:
          DBG_( Iface, Addr(aMessage.address()) ( << "CleanEvictNonAllocating Miss: " << aMessage ));
          // remove the evicting cache from the sharers bitmask
          // theCMPDirState = theCMPDirState & ~sharerMask(cacheIdx);
          rmSharer(cacheIdx, theCMPDirEntry);
          // if there are no more sharers in the tile
          if (!someSharers(theCMPDirEntry)) {
            ++theStats->theMisses_CleanEvictNonAllocating;
            if (aMessage.isPriv()) {
              ++theStats->theMisses_OS_CleanEvictNonAllocating;
            }
            if (kValid & theEvictMask) {
              // Delete CMPDirEntry. No longer needed.
              theCMPDirectory.erase(iter);
              //Send eviction to lower level
              DBG_( Iface, Addr(tagset) ( << "EvictNonAllocating: " << aMessage ));
              aMessage.type() = MemoryMessage::EvictCleanNonAllocating;
              FLEXUS_CHANNEL( MissClassifier ) << aMessage;   /* CMU-ONLY */
              FLEXUS_CHANNEL( RequestOut ) << aMessage;
            }
          }
          // FIXME: may want to implement the notification below
          // notifyEvict( aMessage );
          break;
        /* CMU-ONLY-BLOCK-END */

        default:
          DBG_Assert ( false );
      }
      DBG_ ( Verb, Addr(aMessage.address()) ( << " Miss reply: " << aMessage ) );

      missReplyType = aMessage.type();

      switch( missReplyType ) {
        case MemoryMessage::MissReply:
          // theCMPDirState |= kLineValid;
          theCMPDirEntry.theState |= kLineValid;
          break;
        case MemoryMessage::MissReplyWritable:
          // theCMPDirState |= kLineWritable;
          theCMPDirEntry.theState |= kLineWritable;
          break;
        case MemoryMessage::MissReplyDirty:
          // theCMPDirState |= kLineDirty;
          theCMPDirEntry.theState |= kLineDirty;
          break;
        case MemoryMessage::FetchReq:
        case MemoryMessage::ReadReq:
        case MemoryMessage::LoadReq:
          //This case indicates nothing is hooked up below this cache.
          // theCMPDirState |= kLineValid;
          theCMPDirEntry.theState |= kLineValid;
          break;
        case MemoryMessage::WriteReq:
        case MemoryMessage::UpgradeReq:
        case MemoryMessage::StoreReq:
          //This case indicates nothing is hooked up below this cache.
          // theCMPDirState |= kLineWritable;
          theCMPDirEntry.theState |= kLineWritable;
          break;

        /* CMU-ONLY-BLOCK-BEGIN */
        case MemoryMessage::EvictCleanNonAllocating:
          // A CleanEvictNonAllocating sent below in the hierarchy. No reply is expected. Do nothing.
          break;
        /* CMU-ONLY-BLOCK-END */

        default:
          DBG_Assert( (false), ( << "Message: " << aMessage ) ); //Unhandled message type
      }
    }

    /* CMU-ONLY-BLOCK-BEGIN */
    if (missReplyType != MemoryMessage::EvictCleanNonAllocating) {
      DBG_ ( Verb, Addr(aMessage.address()) ( << std::hex
                                              << "new CMPDir state for " << blockAddr
                                              << " is " << theCMPDirEntry.theState << "-" << theCMPDirEntry.theSharers[1] << "-" << theCMPDirEntry.theSharers[0]
                                              << std::dec ) );
    }
    else {
      DBG_ ( Verb, Addr(aMessage.address()) ( << std::hex << "new CMPDir state for " << blockAddr << " is " << "DELETED" << std::dec ) );
    }
    /* CMU-ONLY-BLOCK-END */

    ////////////////send the fill to the previous cache level
    switch ( fill ) {
      case kNoFill:
        //no fill
        break;
      case kFillValid:
        aMessage.type() = MemoryMessage::MissReply;
        break;
      case  kFillWritable:
        aMessage.type() = MemoryMessage::MissReplyWritable;
        break;
      case kFillDirty:
        aMessage.type() = MemoryMessage::MissReplyDirty;
        break;
      case kSnoopDependantFill:
        switch (snoopReplyType) {
          case MemoryMessage::InvalidateAck:
          case MemoryMessage::DowngradeAck:
            aMessage.type() = MemoryMessage::MissReplyWritable;
            break;
          case MemoryMessage::InvUpdateAck:
          case MemoryMessage::DownUpdateAck:
            aMessage.type() = MemoryMessage::MissReplyDirty;
            break;
          default:
            DBG_Assert( (false), ( << "SnoopMessage: " << theSnoopMessage ) ); //Unhandled message type
        }
        break;
      case kMissDependantFill:
        switch( missReplyType ) {
          case MemoryMessage::MissReply:
          case MemoryMessage::MissReplyWritable:
          case MemoryMessage::MissReplyDirty:
            aMessage.type() = missReplyType;
            break;
          case MemoryMessage::LoadReq:
          case MemoryMessage::PrefetchReadNoAllocReq:
          case MemoryMessage::PrefetchReadAllocReq:
            //This case indicates nothing is hooked up below this cache.
            aMessage.type() = MemoryMessage::MissReply;
            break;
          default:
            DBG_Assert( (false), ( << "Message: " << aMessage ) ); //Unhandled message type
        }
        break;
      default:
        DBG_Assert( false );
    }
    DBG_( Verb, Addr(aMessage.address()) ( << "Fill (if sent): " << aMessage ));

    ////////////////fix state of line in the shared cache if necessary
    switch ( next_state ) {
      case kCMPDirDependantState:
        switch( theCMPDirEntry.theState & kLineMask ) {
          case kLineValid:
            theCache->apply_returned_state( frame_ptr, kValid );
            break;
          case kLineWritable:
            theCache->apply_returned_state( frame_ptr, kWritable );
            break;
          case kLineDirty:
            theCache->apply_returned_state( frame_ptr, kDirty );
            break;
          default:
            DBG_Assert( (false), ( << "LineState: " << std::hex << (theCMPDirEntry.theState & kLineMask) << std::dec << " Message: " << aMessage ) );
        }
        break;

      case kSnoopDependantState:
        switch( snoopReplyType ) {
          case MemoryMessage::InvalidateAck:
          case MemoryMessage::DowngradeAck:
            theCache->apply_returned_state( frame_ptr, kWritable );
            break;
          case MemoryMessage::InvUpdateAck:
          case MemoryMessage::DownUpdateAck:
            theCache->apply_returned_state( frame_ptr, kDirty );
            break;
          default:
            DBG_Assert( (false), ( << "SnoopMessage: " << theSnoopMessage ) ); //Unhandled message type
        }
        break;

      case kMissDependantState:
        switch( missReplyType ) {
          case MemoryMessage::MissReply:
            theCache->apply_returned_state( frame_ptr, kValid );
            break;
          case MemoryMessage::MissReplyWritable:
            theCache->apply_returned_state( frame_ptr, kWritable );
            break;
          case MemoryMessage::MissReplyDirty:
            theCache->apply_returned_state( frame_ptr, kDirty );
            break;
          case MemoryMessage::LoadReq:
          case MemoryMessage::PrefetchReadNoAllocReq:
          case MemoryMessage::PrefetchReadAllocReq:
            //This case indicates nothing is hooked up below this cache.
            theCache->apply_returned_state( frame_ptr, kValid );
            break;
          default:
            DBG_Assert( (false), ( << "Message: " << aMessage ) ); //Unhandled message type
        }
        break;

      default:
        ; // no state fixup is necessary
    }

    /* CMU-ONLY-BLOCK-BEGIN */
    //make sure we get the correct line state passed to the perfect (re)placement algorithm
    //otherwise the number of upgrade mises in the perfect cache may be unrepresentative
    processPerfPlcState(aMessage, frame_ptr);
    /* CMU-ONLY-BLOCK-END */

    DBG_( Verb, Addr(aMessage.address()) ( << std::hex << "New State: " << *frame_ptr << std::dec ) );
  }


  void evict(const unsigned long long aTagset,
             const access_t      anEvictionType)
  {
    // Actions to perform on CMP evictions:
    //   input      state    stateCMPDir   ->  snoop     ret     miss     next
    //   e(xxx)     i(000)   i(000)            X         X       X        X
    //   e(xxx)     i(000)   s(001)            X         X       X        X
    //   e(xxx)     i(000)   e(011)            X         X       X        X

    //   e(xxx)     v(001)   i(000)            -         -       ce       i(000)
    //   e(xxx)     v(001)   s(001)            -         -       drop     i(000)
    //   e(xxx)     v(001)   e(011)            X         X       X        X

    //   e(xxx)     w(011)   i(000)            -         -       we       i(000)
    //   e(xxx)     w(011)   s(001)            -         -       drop     i(000)  CMPDirDependantState will fix it
    //   e(xxx)     w(011)   e(011)            -         -       drop     i(000)  Assumes same block sizes

    //   e(xxx)     d(111)   i(000)            -         -       de       i(000)
    //   e(xxx)     d(111)   s(001)            -         -       drop     i(000)  CMPDirDependantState will fix it
    //   e(xxx)     d(111)   e(011)            -         -       drop     i(000)  Assumes same block sizes

    //Lookup or insert the new block in the directory
    PhysicalMemoryAddress physAddr = PhysicalMemoryAddress(aTagset);
    block_address_t blockAddr = blockAddress(physAddr);
    directory_t::iterator iter;
    bool is_new;
    boost::tie(iter, is_new) = theCMPDirectory.insert( std::make_pair( blockAddr, anInvalCMPDirEntry ) );
    DBG_Assert( !is_new, ( << std::hex << "Evict: for " << blockAddr << " type: " << anEvictionType << std::dec ) );
    tCMPDirEntry & theCMPDirEntry = iter->second;

    DBG_( Iface, Addr(aTagset) ( << std::hex << "Evict: for " << blockAddr << " type: " << anEvictionType << std::dec ) );
    switch(anEvictionType) {
      case kDirty:
        ++theStats->theEvicts_Dirty;
        if (isInvalid(theCMPDirEntry)) {
          theEvictMessage.type() = MemoryMessage::EvictDirty;
          /* CMU-ONLY-BLOCK-BEGIN */
          if (cfg.DirtyRingWritebacks)
            theEvictMessage.type() = MemoryMessage::EvictDirtyNonAllocating;
          /* CMU-ONLY-BLOCK-END */
          DBG_( Verb, Addr(aTagset) ( << std::hex << "Evict: dirty-invalid eviction for " << blockAddr << std::dec ) );
        }
        else {
          DBG_( Verb, Addr(aTagset) ( << std::hex << "Evict: dirty-exclusive/shared drop for " << blockAddr << std::dec ) );
          if (isExclusive(theCMPDirEntry)) {
            ++theStats->theEvicts_Dirty_ExclusiveState;
          }
          else {
            ++theStats->theEvicts_Dirty_SharedState;
          }
          return; //drop the line on the floor
        }
        break;
      case kWritable:
        ++theStats->theEvicts_Writable;
        if (isInvalid(theCMPDirEntry)) {
          theEvictMessage.type() = MemoryMessage::EvictWritable;
          /* CMU-ONLY-BLOCK-BEGIN */
          if (cfg.DirtyRingWritebacks)
            theEvictMessage.type() = MemoryMessage::EvictDirtyNonAllocating;
          /* CMU-ONLY-BLOCK-END */
          DBG_( Verb, Addr(aTagset) ( << std::hex << "Evict: writable-invalid eviction for " << blockAddr << std::dec ) );
        }
        else {
          DBG_( Verb, Addr(aTagset) ( << std::hex << "Evict: writable-exclusive/shared drop for " << blockAddr << std::dec ) );
          if (isExclusive(theCMPDirEntry)) {
            ++theStats->theEvicts_Writable_ExclusiveState;
          }
          else {
            ++theStats->theEvicts_Writable_SharedState;
          }
          return; //drop the line on the floor
        }
        break;
      case kValid:
        ++theStats->theEvicts_Valid;
        if (isInvalid(theCMPDirEntry)) {
          theEvictMessage.type() = MemoryMessage::EvictClean;
          DBG_( Verb, Addr(aTagset) ( << std::hex << "Evict: shared-invalid eviction for " << blockAddr << std::dec ) );
        }
        else if (isExclusive(theCMPDirEntry)) {
          DBG_Assert( false );
        }
        else { //shared
          DBG_( Verb, Addr(aTagset) ( << std::hex << "Evict: shared-shared drop for " << blockAddr << std::dec ) );
          ++theStats->theEvicts_Valid_SharedState;
          return; //drop the line on the floor
        }
        break;
      default:
        DBG_Assert( false, ( << std::hex << "addr: " << blockAddr << " anEvictionType: " << anEvictionType << std::dec ) );
    }

    /* CMU-ONLY-BLOCK-BEGIN */
    if (cfg.InvalidateFriendIndices) {
      // NIKOS: WARNING!!! ATTENTION!!! AHTUNG!!! UGLY HACK!!! FIXME
      // Make sure no copies of the cache block are left in the L2 cache before erasing the CMP directory.
      // There are cache blocks that have both instructions and data.
      // In a CMP with a NUCA cache, such a cache block can be in 2 places in the respective L2 slice:
      // 1. the "instruction" interleave, where the index is shifted by 2 additional bits
      // 2. the "data" interleave, where the index is shifted by 0 or 4 bits (for private or shared data, respectively)
      // The NUCA model assumes that *NO* such cache lines exist. With the hack below we enforce this assumption.
      theCache->invalidate(aTagset, 2);
      theCache->invalidate(aTagset, 0);
      theCache->invalidate(aTagset, 4);
    }
    /* CMU-ONLY-BLOCK-END */

    //Delete the item from the CMP directory
    theCMPDirectory.erase(iter);

    //Send eviction to lower level
    access_t evict_type = anEvictionType & theEvictMask;
    if (evict_type) {
      theEvictMessage.address() = physAddr;
      DBG_( Iface, Addr(aTagset) ( << "Evict: " << theEvictMessage ));
      FLEXUS_CHANNEL( MissClassifier ) << theEvictMessage;  /* CMU-ONLY */
      FLEXUS_CHANNEL( RequestOut ) << theEvictMessage;
    }

    return;
  }

  ////// snoops to upper levels
  void sendSnoopToAllSharers(const tCMPDirEntry &                   theCMPDirEntry,
                             const block_address_t                  anAddress,
                             const MemoryMessage::MemoryMessageType aReqType,
                             const bool                             stopWhenFound)
  {
    if (!someSharers(theCMPDirEntry)) { return; }
    theSnoopMessage.address() = PhysicalMemoryAddress(anAddress);
    unsigned int width = theCMPWidth;
    //ICache
    for (unsigned int i = 0; i < width; ++i) {
      if (isSharer(i, /* isData */ false, theCMPDirEntry)) {
        theSnoopMessage.type() = aReqType;
        DBG_ ( VVerb, Addr(anAddress) ( << theSnoopMessage << " sent to SnoopOutI[" << i << "]" ));
        FLEXUS_CHANNEL_ARRAY( SnoopOutI, i ) << theSnoopMessage;
        if (stopWhenFound) { return; }
      }
    }
    //DCache
    for (unsigned int i = 0; i < width; ++i) {
      if (isSharer(i, /* isData */ true, theCMPDirEntry)) {
        theSnoopMessage.type() = aReqType;
        DBG_ ( VVerb, Addr(anAddress) ( << theSnoopMessage << " sent to SnoopOutD[" << i << "]" ));
        FLEXUS_CHANNEL_ARRAY( SnoopOutD, i ) << theSnoopMessage;
        if (stopWhenFound) { return; }
      }
    }
  }

  void returnReq(const tCMPDirEntry &  theCMPDirEntry,
                 const block_address_t anAddress)
  {
    sendSnoopToAllSharers( theCMPDirEntry, anAddress, MemoryMessage::ReturnReq, true );
  }

  void invalidate(const tCMPDirEntry &  theCMPDirEntry,
                  const block_address_t anAddress)
  {
    sendSnoopToAllSharers( theCMPDirEntry, anAddress, MemoryMessage::Invalidate, false );
  }

  /* CMU-ONLY-BLOCK-BEGIN */
  void purge(const tCMPDirEntry &    theCMPDirEntry
             , const block_address_t anAddress
             , const bool purge_I)
  {
    if (!someSharers(theCMPDirEntry)) { return; }
    bool found = false;
    theSnoopMessage.address() = PhysicalMemoryAddress(anAddress);
    //ICache
    if (purge_I) {
      for (unsigned int i = 0; i < theCMPWidth; ++i) {
        if (purge_I && isSharer(i, /* isData */ false, theCMPDirEntry)) {
          theSnoopMessage.type() = MemoryMessage::PurgeIReq;
          DBG_ ( VVerb, Addr(anAddress) ( << theSnoopMessage << " sent to SnoopOutI[" << i << "]" ));
          FLEXUS_CHANNEL_ARRAY( SnoopOutI, i ) << theSnoopMessage;
          if (theSnoopMessage.type() != MemoryMessage::PurgeIReq) {
            found = true;
          }
        }
      }
    }
    //DCache
    else {
      for (unsigned int i = 0; i < theCMPWidth; ++i) {
        if (isSharer(i, /* isData */ true, theCMPDirEntry)) {
          theSnoopMessage.type() = MemoryMessage::PurgeDReq;
          DBG_ ( VVerb, Addr(anAddress) ( << theSnoopMessage << " sent to SnoopOutD[" << i << "]" ));
          FLEXUS_CHANNEL_ARRAY( SnoopOutD, i ) << theSnoopMessage;
          if (theSnoopMessage.type() != MemoryMessage::PurgeDReq) {
            found = true;
          }
        }
      }
    }
    if (found) {
      theSnoopMessage.type() = MemoryMessage::PurgeAck;
    }
  }
  /* CMU-ONLY-BLOCK-END */

  void downgrade(const tCMPDirEntry &  theCMPDirEntry,
                 const block_address_t anAddress)
  {
    unsigned int ownerCache = getOwner(theCMPDirEntry);
    theSnoopMessage.type() = MemoryMessage::Downgrade;
    theSnoopMessage.address() = PhysicalMemoryAddress(anAddress);
    FLEXUS_CHANNEL_ARRAY( SnoopOutD, getCoreIdxFromCacheIdx(ownerCache)) << theSnoopMessage;  //only DCaches can have a block in exclusive state
  }

  void invalidateOne(const tCMPDirEntry &  theCMPDirEntry,
                     const block_address_t anAddress,
                     const index_t         aCacheIdx)
  {
    theSnoopMessage.type() = MemoryMessage::Invalidate;
    theSnoopMessage.address() = PhysicalMemoryAddress(anAddress);
    if (isDCache(aCacheIdx)) {
      FLEXUS_CHANNEL_ARRAY( SnoopOutD, getCoreIdxFromCacheIdx(aCacheIdx)) << theSnoopMessage;
    }
    else {
      FLEXUS_CHANNEL_ARRAY( SnoopOutI, getCoreIdxFromCacheIdx(aCacheIdx)) << theSnoopMessage;
    }
  }

  /* CMU-ONLY-BLOCK-BEGIN */
  void purgeOne(const tCMPDirEntry &    theCMPDirEntry
                , const block_address_t anAddress
                , const index_t         aCacheIdx
                , const bool            purge_I)
  {
    theSnoopMessage.type() = (purge_I ? MemoryMessage::PurgeIReq : MemoryMessage::PurgeDReq);
    theSnoopMessage.address() = PhysicalMemoryAddress(anAddress);
    if (!purge_I && isDCache(aCacheIdx)) {
      FLEXUS_CHANNEL_ARRAY( SnoopOutD, getCoreIdxFromCacheIdx(aCacheIdx)) << theSnoopMessage;
    }
    else {
      FLEXUS_CHANNEL_ARRAY( SnoopOutI, getCoreIdxFromCacheIdx(aCacheIdx)) << theSnoopMessage;
    }
  }
  /* CMU-ONLY-BLOCK-END */

  /////////////////////////////////////////////////////////////////////////////////////
  // Checkpoint functions

  static const int kDirectoryOwner = -1;

  typedef enum PiranhaDirState_t {
    D_I,
    D_M,
    D_O,
    D_S,
    D_MMW,
    D_MMU,
    D_S2MW,
    D_S2MU,
    D_SFWD,
    D_M2O,
    D_ExtInvalidation,
    D_ExtDowngrade,
    D_GetExtShared,
    D_GetExtModified,
    D_GetExtModifiedInvalidationPending,
    D_EvictWait
  } PiranhaDirState_t;

  int getSomeSharer( const tCMPDirEntry & theCMPDirEntry )
  {
    if (!someSharers(theCMPDirEntry)) {
      // DBG_( VVerb, ( << "immediate exit for " << std::hex << theCMPDirEntry.theState << std::dec ));
      return kDirectoryOwner;
    }

    int theSharersCount = 0;
    for (unsigned int i=0; i < theCMPWidth * 2; i++) {
      if (isSharer(i, theCMPDirEntry)) {
        theSharersCount++;
      }
    }

    int someSharer = (random() % theSharersCount) + 1;
    // DBG_(VVerb, ( << "count=" << theSharersCount << " some=" << someSharer ));
    int j = 0;
    for (unsigned int i=0; i < theCMPWidth * 2; i++) {
      if (isSharer(i, theCMPDirEntry)) {
        someSharer--;
        if (someSharer == 0) {
          return j;
        }
      }
      j++;
    }
    DBG_Assert(false, ( << std::hex
                        << "SharersList=" << theCMPDirEntry.theSharers[1] << "-" << theCMPDirEntry.theSharers[0]
                        << std::dec
                        << " count=" << theSharersCount
                        << " some=" << someSharer ));

    return kDirectoryOwner;
  }

  void saveState(std::string const & aDirName) {
    // save the cache array
    std::string fname( aDirName );
    fname += "/" + statName();
    std::ofstream ofs( fname.c_str() );
    theCache->saveState( ofs );
    ofs.close();

    // save the directory information
    std::string fnameCMPDir( aDirName );
    fnameCMPDir += "/" + statName() + "-PiranhaDirectory";
    std::ofstream ofsCMPDir( fnameCMPDir.c_str() );

    // Write the block mask and the number of entries
    ofsCMPDir << theBlockMask << " " << theCMPDirectory.size() << std::endl;

    // Write out each directory entry
    for ( directory_t::iterator iter = theCMPDirectory.begin();
          iter != theCMPDirectory.end();
          iter++ )
    {
      tCMPDirEntry & theCMPDirEntry = iter->second;

      unsigned long long theAddress;
      PiranhaDirState_t  theState;
      tSharers           theSharersList[2]; // FIXME in timing
      int                theSharersCount;
      int                theOwner;
      int                theAcount;
      // Information for the next state
      int                theNextOwner;
      PiranhaDirState_t  theNextState;
      int                theNextMsg;
      // Information for after an ext state (GetShared/GetModified states)
      // We need to know which transient or stable state to go to.
      PiranhaDirState_t  theExtNextState;
      bool               theEvictionQueued;

      theAddress = iter->first;

      // invalid for the outside world
      if ((theCMPDirEntry.theState & kLineMask) == 0ULL) {
        theState = D_I;
        theSharersList[0] = 0ULL;
        theSharersList[1] = 0ULL;
        theOwner = kDirectoryOwner;
        theSharersCount = 0;
      }
      // shared for the outside world
      else if ((theCMPDirEntry.theState & kLineMask) == kLineValid) {
        theState = D_S;
        theSharersList[0] = theCMPDirEntry.theSharers[0];
        theSharersList[1] = theCMPDirEntry.theSharers[1];
        theOwner = getSomeSharer(theCMPDirEntry);
        theSharersCount = 0;
        for (unsigned int i=0; i < theCMPWidth * 2; i++) {
          if (isSharer(i, theCMPDirEntry)) {
            theSharersCount++;
          }
        }
      }
      // exclusive for the outside world
      else {
        if (isExclusive(theCMPDirEntry)) {
          tCMPDirEntry aCMPDirEntry;
          clearDirEntry(aCMPDirEntry);
          addSharer(getOwner(theCMPDirEntry), aCMPDirEntry);
          theState = D_M;
          theSharersList[0] = aCMPDirEntry.theSharers[0];
          theSharersList[1] = aCMPDirEntry.theSharers[1];
          theOwner = getOwner(theCMPDirEntry);
          theSharersCount = 0;
          for (unsigned int i=0; i < theCMPWidth * 2; i++) {
            if (isSharer(i, aCMPDirEntry)) {
              theSharersCount++;
            }
          }
        }
        else if (isInvalid(theCMPDirEntry)) {
          theState = D_M;
          theSharersList[0] = 0ULL;
          theSharersList[1] = 0ULL;
          theOwner = kDirectoryOwner;
          theSharersCount = 0;
        }
        else {
          theState = D_O;
          theSharersList[0] = theCMPDirEntry.theSharers[0];
          theSharersList[1] = theCMPDirEntry.theSharers[1];
          theOwner = getSomeSharer(theCMPDirEntry);
          theSharersCount = 0;
          for (unsigned int i=0; i < theCMPWidth * 2; i++) {
            if (isSharer(i, theCMPDirEntry)) {
              theSharersCount++;
            }
          }
        }
      }

      DBG_ ( VVerb,
             Addr(theAddress)
             ( << std::hex
               << "Address: " << theAddress
               << " CMPDirState: " << theCMPDirEntry.theState
               << " CMPsharers=" << theCMPDirEntry.theSharers[1] << "-" << theCMPDirEntry.theSharers[0]
               << " to state=" << theState
               << " owner=" << theOwner
               << " sharers=" << theSharersList[1] << "-" << theSharersList[0]
               << std::dec
           ));

      theAcount = 0;

      // next state
      theNextOwner = theOwner;
      theNextState = theState;
      theNextMsg = 0; // LOAD_REQ

      // external
      theExtNextState = theState;
      theEvictionQueued = false;

      ofsCMPDir << "{ "
                << theAddress           << " "
                << (int)theState        << " "
                << theSharersList[0]    << " "
                << theSharersList[1]    << " "
                << theSharersCount      << " "
                << theOwner             << " "
                << theAcount            << " "
                << theNextOwner         << " "
                << (int)theNextState    << " "
                << theNextMsg           << " "
                << (int)theExtNextState << " "
                << theEvictionQueued    << " }"
                << std::endl;
    }
    ofsCMPDir.close();
  }


  void loadState( std::string const & aDirName ) {
    std::string fname( aDirName );
    fname += "/" + statName();
    std::ifstream ifs( fname.c_str() );
    if (! ifs.good()) {
      DBG_( Crit, ( << " saved checkpoint state " << fname << " not found.  Resetting to empty cache. " )  );
    } else {
      ifs >> std::skipws;

      if ( ! theCache->loadState( ifs ) ) {
        DBG_ ( Crit, ( << "Error loading checkpoint state from file: " << fname
                       << ".  Make sure your checkpoints match your current cache configuration." ) );
        DBG_Assert ( false );
      }
      ifs.close();

      std::string fnameCMPDir ( aDirName );
      fnameCMPDir += "/" + statName() + "-PiranhaDirectory";
      std::ifstream ifsCMPDir ( fnameCMPDir.c_str() );

      theCMPDirectory.clear(); // empty the directory

      // Read the block mask and the number of entries
      int dirSize = 0;
      ifsCMPDir >> theBlockMask >> dirSize;
      theBlockSizeLog2 = log_base2(~theBlockMask + 1);
      DBG_Assert ( dirSize >= 0 );

      // Read in each block
      for ( int i=0; i < dirSize; i++ ) {
        char paren;
        ifsCMPDir >> paren;
        DBG_Assert ( (paren == '{'), (<< "Expected '{' when loading CMPDir from checkpoint" ) );

        tCMPDirEntry theCMPDirEntry;
        clearDirEntry(theCMPDirEntry);

        unsigned long long theAddress;
        PiranhaDirState_t  theState;
        tSharers           theSharersList[2];
        int                theSharersCount;
        int                theOwner;
        int                theAcount;
        // Information for the next state
        int                theNextOwner;
        PiranhaDirState_t  theNextState;
        int                theNextMsg;
        // Information for after an ext state (GetShared/GetModified states)
        // We need to know which transient or stable state to go to.
        PiranhaDirState_t  theExtNextState;
        bool               theEvictionQueued;

        ifsCMPDir >> theAddress
                  >> (int&)theState
                  >> theSharersList[0]
                  >> theSharersList[1]
                  >> theSharersCount
                  >> theOwner
                  >> theAcount
                  >> theNextOwner
                  >> (int&)theNextState
                  >> (int&)theNextMsg
                  >> (int&)theExtNextState
                  >> theEvictionQueued;

        ifsCMPDir >> paren;
        DBG_Assert ( (paren == '}'), (<< "Expected '}' when loading CMPDir from checkpoint" ) );

        if (theState == D_I) {
          // theCMPDirEntry = 0ULL;
        }
        else if (theState == D_S) {
          // theCMPDirState = (kLineValid | theSharersList);
          theCMPDirEntry.theState = kLineValid;
          theCMPDirEntry.theSharers[0] = theSharersList[0];
          theCMPDirEntry.theSharers[1] = theSharersList[1];
        }
        else if (theState == D_M) {
          if (theOwner != kDirectoryOwner) {
            // theCMPDirState = (kExclusive | kLineWritable | theOwner);
            theCMPDirEntry.theState = (kExclusive | kLineWritable);
            setOwner(theOwner, theCMPDirEntry);
          }
          else {
            // theCMPDirState = kLineDirty;
            theCMPDirEntry.theState = kLineDirty;
          }
        }
        else if (theState == D_O) {
          // theCMPDirState = (kLineDirty | theSharersList);
          theCMPDirEntry.theState = kLineDirty;
          theCMPDirEntry.theSharers[0] = theSharersList[0];
          theCMPDirEntry.theSharers[1] = theSharersList[1];
        }
        directory_t::iterator iter;
        bool is_new;
        boost::tie(iter, is_new) = theCMPDirectory.insert( std::make_pair( theAddress, theCMPDirEntry ) );
        DBG_Assert(is_new);

        DBG_ ( VVerb,
               Addr(theAddress) 
               ( << std::hex
                 << "Address: " << theAddress
                 << " CMPDirState: " << theCMPDirEntry.theState
                 << " from state=" << theState
                 << " owner=" << theOwner
                 << " sharers=" << theSharersList[1] << "-" << theSharersList[0]
                 << std::dec
             ));
      }
      ifsCMPDir.close();
      DBG_( Dev, ( << "Checkpoint loaded." ));
    }
  }

  void notifyFetch( MemoryMessage & aMessage) {
    if (cfg.NotifyFetches) {
      FLEXUS_CHANNEL( Fetches ) << aMessage;
    }
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

  void notifyL1CleanEvict( MemoryMessage & aMessage) {
    if (cfg.NotifyL1CleanEvicts) {
      FLEXUS_CHANNEL( L1CleanEvicts ) << aMessage;
    }
  }

  void notifyL1DirtyEvict( MemoryMessage & aMessage) {
    if (cfg.NotifyL1DirtyEvicts) {
      FLEXUS_CHANNEL( L1DirtyEvicts ) << aMessage;
    }
  }

  void notifyL1IEvict( MemoryMessage & aMessage) {
    if (cfg.NotifyL1IEvicts) {
      FLEXUS_CHANNEL( L1IEvicts ) << aMessage;
    }
  }

};  // end class FastCMPCache


}  // end Namespace nFastCMPCache

FLEXUS_COMPONENT_INSTANTIATOR( FastCMPCache, nFastCMPCache);

FLEXUS_PORT_ARRAY_WIDTH( FastCMPCache, RequestIn)      { return (cfg.CMPWidth ? cfg.CMPWidth : Flexus::Core::ComponentManager::getComponentManager().systemWidth()); }
FLEXUS_PORT_ARRAY_WIDTH( FastCMPCache, FetchRequestIn) { return (cfg.CMPWidth ? cfg.CMPWidth : Flexus::Core::ComponentManager::getComponentManager().systemWidth()); }
FLEXUS_PORT_ARRAY_WIDTH( FastCMPCache, SnoopOutD)      { return (cfg.CMPWidth ? cfg.CMPWidth : Flexus::Core::ComponentManager::getComponentManager().systemWidth()); }
FLEXUS_PORT_ARRAY_WIDTH( FastCMPCache, SnoopOutI)      { return (cfg.CMPWidth ? cfg.CMPWidth : Flexus::Core::ComponentManager::getComponentManager().systemWidth()); }

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT FastCMPCache

  #define DBG_Reset
  #include DBG_Control()
