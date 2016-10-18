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


#include "NewCacheArray.hpp"

#include <core/target.hpp>
#include <core/debug/debug.hpp>
using namespace Flexus::Core;
using namespace nCache;
using namespace std;

  #define DBG_DeclareCategories Cache
  #define DBG_SetDefaultOps AddCat(Cache)
  #include DBG_Control()

namespace nCache {

  // Useful utility function
  unsigned int log_base2(unsigned int num) {
    unsigned int ii = 0;
    while(num > 1) {
      ii++;
      num >>= 1;
    }
    return ii;
  }

  //////////////////////////////////////////////////////////////
  // The CacheArray class.  Implements the cache array storage
  // and lookup functions
  //
  // aNumOfSlices is just used to leave some low order bits for slice select between
  // block offset and index. However, the real cache slicing is done by instantiating
  // multiple components in the wiring file
  CacheArray::CacheArray ( const int               aCacheSize,         // Total size in bytes
                           const int               aAssociativity,     // Associativity of each set
                           const int               aBlockSize,         // Size of each cache block in bytes
                           const int               aSlices,            // Number of slices in the cache (must be >=1)
                           const int               aSliceNumber,       // Number of this slice (0 for single-slice)
                           const ReplacementPolicy aReplacementPolicy  // Replacement policy for the sets
                         )			
  {
    int
      indexBits,
      sliceBits,
      blockOffsetBits,
      i;

    Tag
      setAddressBits;

    DBG_Assert ( aCacheSize     > 0 );
    DBG_Assert ( aAssociativity > 0 );
    DBG_Assert ( aBlockSize     > 0 );
    DBG_Assert ( aSlices        > 0 );
    DBG_Assert ( aSliceNumber >= 0 && (aSliceNumber < aSlices || aSlices == 1));

    theCacheSize         = aCacheSize;
    theAssociativity     = aAssociativity;
    theBlockSize         = aBlockSize;
    theSlices            = aSlices;
    theSliceNumber       = aSliceNumber;
    theReplacementPolicy = aReplacementPolicy;

    //  Physical address layout:
    //
    //  +---------+-------+-------+-------------+
    //  |         | Index | Slice | BlockOffset |
    //  +---------+-------+-------+-------------+
    //
    //                    |<-- setIndexShift -->|
    //
    //            |<----->|
    //             setIndexMask (shifted right)
    //
    //  |<---------------------->|
    //            tagMask
    //
    //                           |<------------>|
    //                               tagShift


    // Set indexes and masks
    setCount = theCacheSize / theAssociativity / theBlockSize;

    blockOffsetBits = log_base2 ( theBlockSize );
    sliceBits       = log_base2 ( theSlices );
    indexBits       = log_base2 ( setCount );

    blockOffsetMask = (1ULL << blockOffsetBits)-1;

    setIndexShift = blockOffsetBits + sliceBits;
    setIndexMask  = (1ULL << indexBits) - 1;

    // Tag masks
    // tagShift = indexBits + sliceBits + blockOffsetBits;
    // tagMask = (-1LL) - ((1 << tagShift) - 1);
    // use full tags to support R-NUCA and private-NUCA schemes
    tagShift = blockOffsetBits;
    tagMask = ~((1ULL << tagShift) - 1);

    DBG_(Verb, ( << " theCacheSize=" << theCacheSize
                 << " theAssociativity=" << theAssociativity
                 << " theBlockSize=" << theBlockSize
                 << " theSlices=" << theSlices
                 << " theSliceNumber=" << theSliceNumber
                 << std::hex
                 << " setIndexShift=0x" << setIndexShift
                 << " setIndexMask=0x" << setIndexMask
                 << std::dec
                 << " tagShift=" << tagShift
                 << std::hex << " tagMask=0x" << tagMask << std::dec
                 ));

    // Allocate the sets
    theSets = new Set*[setCount];
    DBG_Assert ( theSets );

    for ( i = 0; i < setCount; i++ ) {

      // setAddressBits = ( i << setIndexShift ) + ( theSliceNumber << blockOffsetBits );
      // R-NUCA and private-NUCA do not always place blocks according to the slice# in their address,
      // thus, forego using the slice number in the address calculation
      setAddressBits = ( i << setIndexShift );

      switch ( theReplacementPolicy ) {
      case REPLACEMENT_LRU:
        theSets[i] = new SetLRU ( theAssociativity, setAddressBits, tagShift );
        break;
      case REPLACEMENT_RANDOM:
        theSets[i] = new SetRandom ( theAssociativity, setAddressBits, tagShift );
        break;
      case REPLACEMENT_EXTERNAL:
        theSets[i] = new SetExternal ( theAssociativity, setAddressBits, tagShift );
        break;
      default:
        DBG_Assert ( 0 );
      };
    }
  }

  // Saves the state of the cache array in a homebrew checkpoint file
  bool CacheArray::saveState ( std::ostream & s )
  {
    int
      i;

    for ( i = 0; i < setCount; i++ ) {
      if ( theSets[i]->saveState ( s ) )
        return true;
      s << endl;
    }

    return false;
  }

  // Restores the cache array state in a homebrew checkpoint file.
  // Beware: very little error checking exists, particularly
  // for the number of sets.
  bool CacheArray::loadState ( std::istream & s )
  {
    int
      i;

    for ( i = 0; i < setCount; i++ ) {
      if ( theSets[i]->loadState ( s ) ) {
        DBG_ ( Crit, ( << " Error loading state for set: line number " << i ) );
        return true;
      }
    }

    return false;
  }

  // The master cache array lookup function, based on an address
  // LookupResult CacheArray::operator[] ( const MemoryAddress & anAddress )
  LookupResult CacheArray::lookup ( const MemoryAddress & anAddress, const unsigned int anIdxExtraOffsetBits )
  {
    return theSets[makeSet(anAddress, anIdxExtraOffsetBits)]->lookupBlock ( makeTag ( anAddress ),
                                                      anAddress );
  }

  ////////////////////////////////////////////////////////////
  // The cache Set base class.  This implements the basic
  // functions of the set.  A derived class implements the
  // replacement policy.
  //
  Set::Set ( const int aAssociativity,   // The associativity of the set
             const Tag aAddressBits,     // The values of any address that maps to this set
             const int aTagShift )       // The number of bits to shift to and from a tag
  {
    theAssociativity = aAssociativity;
    theAddressBits   = aAddressBits;
    theTagShift      = aTagShift;
    theBlocks        = new Block[theAssociativity];
    DBG_Assert ( theBlocks );
  }

  // The master lookup function to find a block over the ways in the set
  LookupResult Set::lookupBlock ( const Tag           aTag,
                                  const MemoryAddress aBlockAddress )
  {
    int
      i,
      t = -1;

    // Linearly search through the set for the matching block
    // This could be made faster for higher-associativity sets
    // Through other search methods
    for ( i = 0; i < theAssociativity; i++ ) {
      if ( theBlocks[i].tag() == aTag ) {
        if ( theBlocks[i].valid() ) {
          DBG_(VVerb, Addr(aBlockAddress)
                      ( << std::hex
                        << " Hit on 0x" << aBlockAddress
                        << " tag=0x" << aTag
                        << " block=0x" << theBlocks[i].tag()
                        << " valid=" << theBlocks[i].valid()
                        << " dirty=" << theBlocks[i].dirty()
                        << std::dec
                      ));
          return LookupResult ( this, &(theBlocks[i]), aBlockAddress, true );
        }
        t = i;
      }
    }
    if(t >= 0) {
      return LookupResult ( this, &(theBlocks[t]), aBlockAddress, false );
    }

    // Miss on this set
    return LookupResult ( this, NULL, aBlockAddress, false );
  }

  // Updates the state bits for the cache block based upon the
  // request type
  bool Set::access ( const UniAccessType   aAccess,
                     Block               * aBlock )
  {
    // note: LookupResult will assert if this is not a hit
    switch ( aAccess ) {
    case LOAD_REQ:
    case FETCH_REQ:
      // no change required (since the block must already be valid)
      aBlock->setPrefetched ( false );
      break;

    case ATOMIC_PRELOAD_REQ:
      if (aBlock->modifiable()) {
        aBlock->setDirty ( true );
      }
      aBlock->setPrefetched ( false );
      break;
    case STORE_PREFETCH_REQ:
    case STORE_REQ:
    case NON_ALLOCATING_STORE_REQ:
      // this request writes data to the block
      aBlock->setDirty ( true );
      aBlock->setPrefetched ( false );
      break;

    case RMW_REQ:
    case CMP_SWAP_REQ:
      // this request swaps data into the block
      aBlock->setDirty ( true );
      aBlock->setPrefetched ( false );
      break;

    case FLUSH_REQ:
      // this request takes dirty data out of the block, and
      // writes it back to the next cache level (also FLUSH)
      aBlock->setDirty ( false );
      aBlock->setModifiable ( false );
      break;

    case READ_REQ:
      // clear the dirty token, since it will be passed above if
      // appropriate
      aBlock->setDirty ( false );
      aBlock->setPrefetched ( false );
      break;
    case WRITE_REQ:
      // this request returns a modifiable block of data, but no
      // operation is actually performed at this cache level...
      // clear the dirty token in case it had to be passed above
      aBlock->setDirty ( false );
      aBlock->setPrefetched ( false );
      break;
    case WRITE_ALLOC:
      // this request returns a modifiable block of data, and
      // also writes some data to the block at this cache level,
      // but the dirty token remains above
      aBlock->setDirty ( false );
      aBlock->setPrefetched ( false );
      break;
    case UPGRADE_REQ:
      // this request returns modify permission, but no operation
      // is performed at this cache level... don't worry about clearing
      // the dirty token, since it's guaranteed not to be set
      aBlock->setPrefetched ( false );
      break;
    case UPGRADE_ALLOC:
      // this request returns modify permission, and also writes
      // some data to the block at this cache level... don't worry about
      // the dirty token, since it's guaranteed not to be set
      aBlock->setPrefetched ( false );
      break;

    case FLUSH:
      // this request takes dirty data out of the block, and
      // writes it back to the next cache level (also FLUSH_REQ)
      aBlock->setDirty ( false );
      aBlock->setModifiable ( false );
      break;
    case EVICT_DIRTY:
      // this request writes dirty data from a previous cache
      // level to the block at this level
      aBlock->setDirty ( true );
      // since evict can also allocate in some cases, mark the block
      // as modifiable and valid - this will not affect evictions
      // that do not cause allocations (since they must already be
      // both valid and modifiable!)
      aBlock->setModifiable ( true );
      aBlock->setValid ( true );
      aBlock->setPrefetched ( false );
      break;
    case EVICT_WRITABLE:
      // this request indicates that the controller is allocating
      // in this cache level for a line that has been dropped
      // from a previous level (if the controller chose to ignore
      // this request, the access policy would never be invoked)
      aBlock->setValid ( true );
      aBlock->setDirty ( false );
      aBlock->setModifiable ( true );
      aBlock->setPrefetched ( false );
      break;
    case EVICT_CLEAN:
      // like EVICT_WRITABLE, except without modifiable permission
      aBlock->setValid ( true );
      aBlock->setDirty ( false );
      aBlock->setModifiable ( false );
      aBlock->setPrefetched ( false );
      break;
    case MISS_REPLY:
      // this is a fill reply that does not grant modify permission
      aBlock->setValid ( true );
      aBlock->setDirty ( false );
      aBlock->setModifiable ( false );
      aBlock->setPrefetched ( false );
      break;
    case MISS_REPLY_WRITABLE:
      // this is a fill reply that grants modify permission
      aBlock->setValid ( true );
      aBlock->setDirty ( false );
      aBlock->setModifiable ( true );
      aBlock->setPrefetched ( false );
      break;
    case MISS_REPLY_DIRTY:
      // this is a fill reply with modify permission and dirty data
      aBlock->setValid ( true );
      aBlock->setDirty ( true );
      aBlock->setModifiable ( true );
      aBlock->setPrefetched ( false );
      break;
    case FETCH_REPLY:
      aBlock->setValid ( true );
      aBlock->setDirty ( false );
      aBlock->setModifiable ( false );
      aBlock->setPrefetched ( false );
      break;
    case UPGRADE_REPLY:
      // this reply grants modify permission
      aBlock->setValid ( true );
      aBlock->setDirty ( false );
      aBlock->setModifiable ( true );
      aBlock->setPrefetched ( false );
      break;
    case INVALIDATE:
    case PURGE_REQ:   /* CMU-ONLY */
      // Invalidate leaves the block in the invalid state
      aBlock->setValid ( false );
      break;
    case DOWNGRADE:
      // Downgrade takes dirty data out of the block and
      // removes modify permission
      aBlock->setDirty ( false );
      aBlock->setModifiable ( false );
      break;
    case PROBE:
      // this request should never access the cache
      DBG_Assert(false, ( << "Probe in access policy") );
      break;
    case PREFETCH_READ_NOALLOC_REQ:
    case PREFETCH_READ_ALLOC_REQ:
      // do nothing here! - should only happen when a ReadRedundant
      // is being returned, which means no action was taken by the
      // cache (and hence its state shouldn't change at all)
      break;
    case PREFETCH_INSERT:
      // this requests that a prefetch read be inserted into this
      // cache array - no modifiable permission is granted
      aBlock->setValid ( true  );
      aBlock->setDirty ( false );
      aBlock->setModifiable ( false );
      aBlock->setPrefetched ( true );
      break;
    case PREFETCH_INSERT_WRITABLE:
      // this requests that a prefetch read be inserted into this
      // cache array with modifiable permission
      aBlock->setValid ( true );
      aBlock->setDirty ( false );
      aBlock->setModifiable ( true );
      aBlock->setPrefetched ( true );
      break;
    case RETURN_REQ:
    case RETURN_REPLY:
      // Changes nothing
      break;
    default:
      DBG_Assert( false, (<< "unknown access type in access policy") );
    }

    DBG_(VVerb, Addr(blockAddress(aBlock))
                      ( << std::hex
                        << " SetAccess " << aAccess
                        << " on 0x" << blockAddress(aBlock)
                        << " tag=0x" << aBlock->tag()
                        << " block=0x" << aBlock->tag()
                        << " valid=" << aBlock->valid()
                        << " dirty=" << aBlock->dirty()
                        << std::dec
                      ));

    return false;
  }

  // Saves the state of the set.  This function should still be called
  // if any derived class overrides the state saving function.
  bool Set::saveState ( std::ostream & s )
  {
    int
      i;

    s << "{";
    for ( i = 0; i < theAssociativity; i++ ) {
      s << "[ " << theBlocks[i].state()
        << " "  << static_cast<unsigned long long>(theBlocks[i].tag())
        << " ]";
    }
    s << "} ";
    return false;
  }

  // Loads the state of the set.  This function should still be called
  // if any derived class overrides the state saving function
  bool Set::loadState ( std::istream & s )
  {
    int
      i;

    char
      paren;

    s >> paren; // {
    if ( paren != '{' ) {
      DBG_ ( Crit, (<< "Expected '{' when loading checkpoint" ) );
      return true;
    }

    for ( i = 0; i < theAssociativity; i++ ) {
      s >> paren >> theBlocks[i].state() >> theBlocks[i].tag() >> paren;
    }

    s >> paren; // {
    if ( paren != '}' ) {
      DBG_ ( Crit, (<< "Expected '}' when loading checkpoint" ) );
      return true;
    }

    return false;
  }

  // Constructs a derived set that implements an LRU replacement policy.
  SetLRU::SetLRU ( const int aAssociativity,
                   const Tag aAddressBits,
                   const int aTagShift ) :
    Set ( aAssociativity, aAddressBits, aTagShift )
  {
    int
      i;

    theMRUOrder = new int[aAssociativity];

    for ( i = 0; i < aAssociativity; i++ ) {
      theMRUOrder[i] = i;
    }
  }

  Block * SetLRU::victim ( const UniAccessType aAccess )
  {
    // choose the LRU entry - no update of the MRU chain is required
    // also independent of AccessType
    return &theBlocks[lruListTail()];
  }


  // Based upon the specific access type, the LRU value
  // will change accordingly
  bool SetLRU::access ( const UniAccessType   aAccess,
                        Block               * aBlock )
  {
    int
      theBlockNum;

    if ( Set::access ( aAccess, aBlock ) )
      return true;

    theBlockNum = aBlock - theBlocks;

    switch ( aAccess ) {
    case LOAD_REQ:
    case STORE_REQ:
    case STORE_PREFETCH_REQ:
    case NON_ALLOCATING_STORE_REQ:
    case FETCH_REQ:
    case RMW_REQ:
    case CMP_SWAP_REQ:
    case ATOMIC_PRELOAD_REQ:
    case READ_REQ:
    case WRITE_REQ:
    case WRITE_ALLOC:
    case UPGRADE_REQ:
    case UPGRADE_ALLOC:
      // reads from or writes to a block - mark as MRU
      moveToHead ( theBlockNum );
      break;
    case FLUSH_REQ:
      // the processor is probably done with this data - mark as LRU
      moveToTail ( theBlockNum );
      break;
    case MISS_REPLY:
    case MISS_REPLY_WRITABLE:
    case MISS_REPLY_DIRTY:
    case UPGRADE_REPLY:
    case FETCH_REPLY:
      // fill or upgrade reply - may have been an eviction - mark as MRU
      moveToHead ( theBlockNum );
      break;
    case EVICT_DIRTY:
    case EVICT_WRITABLE:
    case EVICT_CLEAN:
      // allocation in the cache - mark as MRU
      moveToHead ( theBlockNum );
      break;
    case INVALIDATE:
    case PURGE_REQ:   /* CMU-ONLY */
      // mark as LRU, since the block is no longer valid
      moveToTail ( theBlockNum );
      break;
    case DOWNGRADE:
      // a lower entity in the memory hierarchy has downgraded this line,
      // but it is unknown if the processor still needs it, so don't modify
      // the MRU order
      break;
    case PROBE:
      // probing should have no effect on LRU order
      break;
    case PREFETCH_READ_NOALLOC_REQ:
    case PREFETCH_READ_ALLOC_REQ:
      // a ReadRedundant will be returned - this should have no effect here
      break;
    case PREFETCH_INSERT:
    case PREFETCH_INSERT_WRITABLE:
      // read and allocation operation - mark as MRU
      moveToHead ( theBlockNum );
      break;
    case RETURN_REQ:
    case RETURN_REPLY:
      moveToHead ( theBlockNum );
      break;
    default:
      DBG_Assert (false, ( << "unknown access type in replacement policy" ) );
    };

    DBG_(VVerb, Addr(blockAddress(aBlock))
                      ( << std::hex
                        << " LRUAccess " << aAccess
                        << " on 0x" << blockAddress(aBlock)
                        << " tag=0x" << aBlock->tag()
                        << " block=0x" << aBlock->tag()
                        << " valid=" << aBlock->valid()
                        << " dirty=" << aBlock->dirty()
                        << std::dec
                      ));

    return false;
  }

  bool SetLRU::moveToHead ( const int aBlock )
  {
    int
      i = 0;

    // Find the list entry for the specified index
    while ( theMRUOrder[i] != aBlock )
      i++;

    // Move entries down the MRU chain
    while ( i > 0 ) {
      theMRUOrder[i] = theMRUOrder[i-1];
      i--;
    }

    theMRUOrder[0] = aBlock;

    return false;
  }

  bool SetLRU::moveToTail ( const int aBlock )
  {
    int
      i = 0;

    // Find the list entry for the specified index
    while ( theMRUOrder[i] != aBlock )
      i++;

    // move appropriate entries up the MRU chain
    while ( i < theAssociativity - 1 ) {
      theMRUOrder[i] = theMRUOrder[i+1];
      i++;
    }

    theMRUOrder[theAssociativity-1] = aBlock;

    return false;
  }

  bool SetLRU::moveToInvTail ( const int aBlock )
  {
    int
      i = 0,
      loc = theAssociativity - 1;

    // Find the list entry for the specified index
    while ( theMRUOrder[i] != aBlock )
      i++;

    // Don't move this blocker older than previously-invalidated blocks
    while ( (loc > i) && !theBlocks[theMRUOrder[loc]].valid())
      loc--;

    // move appropriate entries up the MRU chain
    while ( i < loc ) {
      theMRUOrder[i] = theMRUOrder[i+1];
      i++;
    }

    theMRUOrder[loc] = aBlock;

    return false;
  }

  bool SetLRU::saveState ( std::ostream & s )
  {
    int
      i;

    Set::saveState ( s );

    s << "< ";
    for ( i = 0; i < theAssociativity; i++ ) {
      s << theMRUOrder[i] << " ";
    }
    s << "> ";

    return false;
  }

  bool SetLRU::loadState ( std::istream & s )
  {
    char
      paren;

    int
      i;

    if ( Set::loadState ( s ) )
      return true;

    s >> paren; // <
    if ( paren != '<' ) {
      DBG_ ( Crit, (<< "Expected '<' when loading LRU for checkpoint" ) );
      return true;
    }

    for ( i = 0; i < theAssociativity; i++ ) {
      s >> theMRUOrder[i];
    }

    s >> paren; // >
    if ( paren != '>' ) {
      DBG_ ( Crit, (<< "Expected '>' when loading LRU for checkpoint" ) );
      return true;
    }

    return false;
  }


  // Implements a random replacement set.
  // Note that the random replacement seed cannot be
  // checkpointed, so the resulting simulations may
  // not be deterministic
  SetRandom::SetRandom ( const int aAssociativity,
                         const Tag aAddressBits,
                         const int aTagShift ) :
    Set ( aAssociativity, aAddressBits, aTagShift )
  {}

  Block * SetRandom::victim ( const UniAccessType aAccess )
  {
    return &theBlocks[random() % (theAssociativity-1)];
  }

  // Implements an externally-specified replacement algorithm.
  // This requires some additional work to make the interface
  // run properly.
  SetExternal::SetExternal ( const int aAssociativity,
                             const Tag aAddressBits,
                             const int aTagShift ) :
    Set ( aAssociativity, aAddressBits, aTagShift )
  {}

  Block * SetExternal::victim ( const UniAccessType aAccess )
  {
    // This is a stub function that should not be called
    // for this type of set.  The user is supposed to specify
    // the exact line to replace, using another version of
    // this function.
    DBG_Assert ( 0 );
    return &theBlocks[0];
  }

}; // namespace nCache
