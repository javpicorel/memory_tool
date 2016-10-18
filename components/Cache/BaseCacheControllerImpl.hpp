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
#ifndef _BASECACHECONTROLLERIMPL_HPP
#define _BASECACHECONTROLLERIMPL_HPP

#include <list>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>
#include <core/stats.hpp>

#include <components/Common/Slices/MemoryMessage.hpp>
#include <components/Common/Slices/TransactionTracker.hpp>
#include <components/Common/CachePlacementPolicyDefn.hpp>

#include "NewCacheArray.hpp"
#include "CacheBuffers.hpp"
#include "MissTracker.hpp"

namespace nCache {

  using namespace Flexus::SharedTypes;
  typedef MemoryMessage::MemoryAddress MemoryAddress;

  // For the love of god, please simplify type names.
  typedef boost::intrusive_ptr<MemoryMessage>      MemoryMessage_p;
  typedef boost::intrusive_ptr<TransactionTracker> TransactionTracker_p;

  enum enumAction
    { kNoAction                   //Drop the message on the floor
    , kSend                       //Send the message on its way
    , kInsertMAF_WaitAddress      //Create a MAF entry for the message in WaitAddress state
    , kInsertMAF_WaitResponse     //Create a MAF entry for the message in WaitResponse state
    , kInsertMAF_WaitProbe        //Create a MAF entry for the message in WaitProbe state
    , kReplyAndRemoveMAF          //Remove the MAF entry and send the stored message on its way
    , kReplyAndRemoveResponseMAF  //Remove the MAF entry for a response MAF, not probe MAF (Nikos: only Piranha uses this)
    };

  struct Action {
    enumAction theAction;
    std::list<MemoryMessage_p> theOutputMessage;
    TransactionTracker_p theOutputTracker;

    int theRequiresData;
    int theRequiresTag;
    int theRequiresDuplicateTag;

    Action(enumAction anAction, int aRequiresData = 0)
     : theAction(anAction)
     , theRequiresData(aRequiresData)
     , theRequiresTag(1)
     , theRequiresDuplicateTag(0)
     {}

    Action(enumAction anAction, TransactionTracker_p aTracker, int aRequiresData = 0)
      : theAction(anAction)
     , theOutputTracker(aTracker)
     , theRequiresData(aRequiresData)
     , theRequiresTag(1)
     , theRequiresDuplicateTag(0)
    {}

    void addOutputMessage ( MemoryMessage_p msg )
    {
      theOutputMessage.push_back ( msg );
    }

  };

  enum eCacheType
    {
      kBaselineCache,
      kPiranhaCache,
      kFCMPCache /* CMU-ONLY */
    };

  eCacheType cacheTypeName ( std::string type );

  std::ostream & operator << ( std::ostream & s, const enumAction eAction );
  std::ostream & operator << ( std::ostream & s, const Action     action );

  // A structure that consolidates the initial info for creating any cache
  // Passing this structure is less error-prone than trying to pass all of the
  // arguments piecemeal.

  class CacheInitInfo
  {
  public:

    CacheInitInfo ( std::string aName,
                    int  aCores,
                    int  aCacheSize,
                    int  anAssociativity,
                    int  aBlockSize,
                    unsigned long long aPageSize,
                    int  aNodeId,
                    tFillLevel aCacheLevel,
                    int  anEBSize,
                    bool aProbeOnIfetchMiss,
                    int  aSlices,
                    int  aSliceNumber,
                    bool aDoCleanEvictions,
                    ReplacementPolicy aReplacementPolicy,
                    bool aIsPiranhaCache,
                    tPlacement aPlacement,
                    bool aPrivateWithASR, /* CMU-ONLY */
                    bool anAllowOffChipStreamFetch                    
                  )
      :    theName              ( aName )
         , theCores             ( aCores )
         , theCacheSize         ( aCacheSize )
         , theAssociativity     ( anAssociativity )
         , theBlockSize         ( aBlockSize )
         , theBlockSizeLog2     ( log_base2(aBlockSize) )
         , thePageSize          ( aPageSize )
         , thePageSizeLog2      ( log_base2(aPageSize) )
         , theNodeId            ( aNodeId )
         , theCacheLevel        ( aCacheLevel )
         , theEBSize            ( anEBSize )
         , theProbeOnIfetchMiss ( aProbeOnIfetchMiss )
         , theSlices            ( aSlices )
         , theSliceNumber       ( aSliceNumber )
         , theDoCleanEvictions  ( aDoCleanEvictions )
         , theReplacementPolicy ( aReplacementPolicy )
         , theIsPiranhaCache    ( aIsPiranhaCache    )
         , thePlacement         ( aPlacement )
         , thePrivateWithASR    ( aPrivateWithASR ) /* CMU-ONLY */
         , theAllowOffChipStreamFetch( anAllowOffChipStreamFetch    )
    {}

    std::string theName;
    int  theCores;
    int  theCacheSize;
    int  theAssociativity;
    int  theBlockSize;
    int  theBlockSizeLog2;
    unsigned long long thePageSize;
    unsigned long long thePageSizeLog2;
    int  theNodeId;
    tFillLevel theCacheLevel;
    int  theEBSize;
    bool theProbeOnIfetchMiss;
    int  theSlices;
    int  theSliceNumber;
    bool theDoCleanEvictions;
    ReplacementPolicy theReplacementPolicy;
    bool theIsPiranhaCache;
    tPlacement thePlacement;
    bool thePrivateWithASR; /* CMU-ONLY */
    bool theAllowOffChipStreamFetch;

    unsigned int theASRRandomContext; /* CMU-ONLY */
    std::vector<int> theASR_ReplicationLevel; /* CMU-ONLY */
  };

 //The CacheController owns the message/process queues and MAFs, and knows about
  //Transport objects.  It manages processes and checks the resource constraints
  //of the various processes the cache runs
  struct BaseCacheController {
    virtual std::list<MemoryMessage_p> getAllMessages( const MemoryAddress  & aBlockAddress) = 0;
    virtual std::list<MemoryMessage_p> getAllUncompletedMessages( const MemoryAddress  & aBlockAddress) = 0;
    virtual
      std::pair
        < MemoryMessage_p
        , TransactionTracker_p
        > getWaitingMAFEntry( const MemoryAddress  & aBlockAddress) = 0;
    virtual ~BaseCacheController() {}

  };

  //The CacheControllerImpl owns the CacheArray, EvictBuffer, and SnoopBuffer.
  //It understands the cache protocol and does all the work.  It does NOT know
  //about transport object, just MemoryMessages and TransactionTrackers.  This
  //allows it to compile separately from the Flexus wiring.
  struct BaseCacheControllerImpl {

  protected:
    BaseCacheController * theController;

    typedef UniAccessType AccessType;

    // Basic structures that every cache needs
    CacheArray    theArray;
    EvictBuffer   theEvictBuffer;
    SnoopBuffer   theSnoopBuffer;

    // The front side configuration
    typedef MemoryAccessTranslator<MemoryMessage,UniAccessType>
      frontAccessTranslator;

    CacheInitInfo * theInit;

    MissTracker theReadTracker;

    // Statistics
    Stat::StatCounter accesses;
      Stat::StatCounter accesses_user_I;
      Stat::StatCounter accesses_user_D;
      Stat::StatCounter accesses_system_I;
      Stat::StatCounter accesses_system_D;
    Stat::StatCounter requests;
    Stat::StatCounter hits;
      Stat::StatCounter hits_user_I;
      Stat::StatCounter hits_user_D;
      Stat::StatCounter hits_user_D_Read;
      Stat::StatCounter hits_user_D_Write;
      Stat::StatCounter hits_user_D_PrefetchRead;
      Stat::StatCounter hits_user_D_PrefetchWrite;
      Stat::StatCounter hits_system_I;
      Stat::StatCounter hits_system_D;
      Stat::StatCounter hits_system_D_Read;
      Stat::StatCounter hits_system_D_Write;
      Stat::StatCounter hits_system_D_PrefetchRead;
      Stat::StatCounter hits_system_D_PrefetchWrite;
      Stat::StatCounter hitsEvict;
    Stat::StatCounter misses;
      Stat::StatCounter misses_user_I;
      Stat::StatCounter misses_user_D;
      Stat::StatCounter misses_user_D_Read;
      Stat::StatCounter misses_user_D_Write;
      Stat::StatCounter misses_user_D_PrefetchRead;
      Stat::StatCounter misses_user_D_PrefetchWrite;
      Stat::StatCounter misses_system_I;
      Stat::StatCounter misses_system_D;
      Stat::StatCounter misses_system_D_Read;
      Stat::StatCounter misses_system_D_Write;
      Stat::StatCounter misses_system_D_PrefetchRead;
      Stat::StatCounter misses_system_D_PrefetchWrite;
      Stat::StatCounter misses_peerL1_system_I;
      Stat::StatCounter misses_peerL1_system_D;
      Stat::StatCounter misses_peerL1_user_I;
      Stat::StatCounter misses_peerL1_user_D;
    Stat::StatCounter upgrades;
    Stat::StatCounter fills;
    Stat::StatCounter upg_replies;
    Stat::StatCounter evicts_clean;
    Stat::StatCounter evicts_dirty;
    Stat::StatCounter snoops;
    Stat::StatCounter purges; /* CMU-ONLY */
    Stat::StatCounter probes;
    Stat::StatCounter iprobes;
    Stat::StatCounter lockedAccesses;
    Stat::StatCounter tag_match_invalid;
    Stat::StatCounter prefetchReads;
    Stat::StatCounter prefetchHitsRead;
    Stat::StatCounter prefetchHitsPrefetch;
    Stat::StatCounter prefetchHitsWrite;
    Stat::StatCounter prefetchHitsButUpgrade;
    Stat::StatCounter prefetchEvicts;
    Stat::StatCounter prefetchInvals;
    Stat::StatCounter atomicPreloadWrites;

  public:

    static BaseCacheControllerImpl * construct
    ( BaseCacheController * aController,
      CacheInitInfo       * aInfo,
      eCacheType            type );

    BaseCacheControllerImpl ( BaseCacheController * aController,
                              CacheInitInfo       * aInit );

    virtual ~BaseCacheControllerImpl() {}

    virtual unsigned int freeEvictBuffer() const
    {
      return theEvictBuffer.freeEntries();
    }

    virtual bool emptyEvictBuffer() const
    {
      return theEvictBuffer.empty();
    }

    virtual bool fullEvictBuffer() const
    {
      return theEvictBuffer.full();
    }

    virtual void reserveEvictBuffer()
    {
      theEvictBuffer.reserve();
    }

    virtual void unreserveEvictBuffer()
    {
      theEvictBuffer.unreserve();
    }

    virtual bool evictableBlockExists(int anIndex) const
    {
      return theEvictBuffer.headEvictable(anIndex);
    }

    virtual bool isQuiesced() const
    {
      return theSnoopBuffer.empty();
    }

    virtual void saveState(std::string const & aDirName);
    virtual void loadState(std::string const & aDirName);

    virtual MemoryAddress getBlockAddress( MemoryAddress const & anAddress) const
    {
      return theArray.blockAddress ( anAddress );
    }

    virtual BlockOffset getBlockOffset( MemoryAddress const & anAddress) const
    {
      return theArray.blockOffset ( anAddress );
    }

  protected:

    //Simple accessor functions for getting information about a memory message
    SetIndex getSet(const MemoryAddress & anAddress, const unsigned int anIdxExtraOffsetBits) {
      return theArray.makeSet(anAddress, (theInit->theCacheLevel == eL1) ? 0 : anIdxExtraOffsetBits);
    }

    // These are used for simple translations to various address types
    MemoryAddress getAddress(MemoryMessage const & aMessage) {
      return aMessage.address();
    }
    MemoryAddress getBlockAddress(MemoryMessage const & aMessage) {
      return theArray.blockAddress(aMessage.address());
    }
    BlockOffset getBlockOffset(MemoryMessage const & aMessage) {
      return theArray.blockOffset(aMessage.address());
    }
    SetIndex getSet(MemoryMessage const & aMessage) {
      return theArray.makeSet(aMessage.address(), (theInit->theCacheLevel == eL1) ? 0 : aMessage.idxExtraOffsetBits());
    }
    Tag getTag(MemoryMessage const & aMessage) {
      return theArray.makeTag(aMessage.address());
    }
    Tag getTag(MemoryAddress const anAddress) {
      return theArray.makeTag(anAddress);
    }

    ///////////////////////////
    // Eviction Processing

  protected:
    // This queues a block for eviction
    virtual void evictBlock(Block & aBlock, MemoryAddress aBlockAddress, bool evictable = true );

  public:
    virtual Action  doEviction();

    ///////////////////////////////
    // Request Channel processing
  public:
    virtual Action handleRequestMessage ( MemoryMessage_p      msg,
                                          TransactionTracker_p tracker,
                                          bool waiting_for_response );

  public:
    virtual Action wakeMaf ( MemoryMessage_p      msg,
                             TransactionTracker_p tracker,
                             TransactionTracker_p aWakingTracker);

  protected:
    // Examine a request, given that the related set is not locked
    // - returns the action to be taken by the CacheController
    virtual Action examineRequest ( MemoryMessage_p      msg,
                                    TransactionTracker_p tracker,
                                    bool                 has_maf_entry,
                                    TransactionTracker_p aWakingTracker =  TransactionTracker_p() );

    virtual Action handleMiss ( MemoryMessage_p        msg,
                                TransactionTracker_p   tracker,
                                LookupResult         & lookup,
                                bool                   address_conflict_on_miss,
                                bool                   probeIfetchMiss ) = 0;

    virtual Action performOperation ( MemoryMessage_p        msg,
                                      TransactionTracker_p   tracker,
                                      LookupResult         & lookup,
                                      bool                   wasHit,
                                      bool                   anyInvs,
                                      bool                   blockAddressOnMiss ) = 0;
  public:

    /////////////////////////////////////////
    // Back Channel Processing
    virtual Action handleBackMessage ( MemoryMessage_p      msg,
                                        TransactionTracker_p tracker ) = 0;

  protected:

    virtual void initializeSnoop ( MemoryMessage_p        msg,
                                   TransactionTracker_p   tracker,
                                   LookupResult         & lookup );

    virtual void initializeProbe ( MemoryMessage_p        msg,
                                   TransactionTracker_p   tracker,
                                   LookupResult         & lookup );


    virtual Action finalizeProbe ( MemoryMessage_p        msg,
                                    TransactionTracker_p   tracker,
                                    LookupResult         & lookup );

    virtual Action finalizeSnoop ( MemoryMessage_p        msg,
                                    TransactionTracker_p   tracker,
                                    LookupResult         & lookup );
  public:
    /////////////////////////////////////////
    // Snoop Channel Processing
    virtual Action handleSnoopMessage ( MemoryMessage_p      msg,
                                         TransactionTracker_p tracker) = 0;

    virtual Action handleIprobe ( bool aHit,
                           MemoryMessage_p        fetchReq,
                           TransactionTracker_p   tracker ) = 0;

    unsigned long long makeGroup(MemoryAddress addr) {
      return (addr & ~( ( 1 << theInit->thePageSizeLog2 ) - 1) );
    }

    unsigned long long makeOffset(MemoryAddress addr) {
      return ((addr & ( (1 << theInit->thePageSizeLog2) - 1)) >> theInit->theBlockSizeLog2);
    }

  }; // class BaseCacheControllerImpl

}; // namespace nCache

#endif // _BASECACHECONTROLLERIMPL_HPP
