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

#ifndef _CACHE_ARRAY_HPP
#define _CACHE_ARRAY_HPP

#include <exception>
#include <iostream>
#include <stdlib.h>

#include <boost/throw_exception.hpp>

#include <core/target.hpp>
#include <core/types.hpp>
#include <core/debug/debug.hpp>

namespace nCache
{

  // Useful utility function
  unsigned int log_base2(unsigned int num);

  typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

  class InvalidCacheAccessException : public std::exception {};

  // Supported replacement policies
  enum ReplacementPolicy
    {
      REPLACEMENT_LRU,       // LRU replacement policy
      REPLACEMENT_RANDOM,    // Random replacement policy
      REPLACEMENT_EXTERNAL  // External agent specifies replacement
    }; // enum ReplacementPolicy


  // These are the internal memory access types used within the cache.
  // I do not know why the memory message types are not good enough.
  enum UniAccessType {
    LOAD_REQ,
    STORE_REQ,
    STORE_PREFETCH_REQ,
    NON_ALLOCATING_STORE_REQ,
    FETCH_REQ,
    RMW_REQ,
    CMP_SWAP_REQ,
    ATOMIC_PRELOAD_REQ,
    FLUSH_REQ,
    PURGE_REQ, /* CMU-ONLY */
    READ_REQ,
    WRITE_REQ,
    WRITE_ALLOC,
    UPGRADE_REQ,
    UPGRADE_ALLOC,
    FLUSH,
    EVICT_DIRTY,
    EVICT_WRITABLE,
    EVICT_CLEAN,
    MISS_REPLY,
    MISS_REPLY_WRITABLE,
    MISS_REPLY_DIRTY,
    FETCH_REPLY,
    UPGRADE_REPLY,
    INVALIDATE,
    DOWNGRADE,
    PROBE,
    PREFETCH_READ_NOALLOC_REQ,
    PREFETCH_READ_ALLOC_REQ,
    PREFETCH_INSERT,
    PREFETCH_INSERT_WRITABLE,
    RETURN_REQ,
    RETURN_REPLY
  };

  typedef unsigned long BlockOffset;
  typedef unsigned long long Tag;
  typedef int       SetIndex;


  // For more than 32 cores, we need to make this a bigger type!
  typedef unsigned long CoreBitmask;

  // The block state is a bitmask
  // Note that the bitmask is externally viewable in the checkpoints
#define STATE_VALID      0x01
#define STATE_DIRTY      0x02
#define STATE_MODIFIABLE 0x04
#define STATE_PREFETCHED 0x08

  // This is a cache block.  The accessor functions are braindead simple.
  class Block
  {
  public:

    Block ( void ) :
      theTag ( 0 ),
      theState ( 0 )
    {}

  public:

    Tag tag ( void ) const
    {
      return theTag;
    }

    Tag & tag ( void )
    {
      return theTag;
    }

    int state ( void ) const { return theState; }

    int & state ( void ) { return theState; }

    bool valid ( void ) const
    {
      return (theState & STATE_VALID) ? true : false ;
    }

    bool modifiable ( void ) const
    {
      return (theState & STATE_MODIFIABLE) ? true : false;
    }

    bool dirty ( void ) const
    {
      return ( theState & STATE_DIRTY ) ? true : false;
    }

    bool prefetched ( void ) const
    {
      return ( theState & STATE_PREFETCHED ) ? true : false;
    }

    void setDirty ( const bool val )
    {
      if ( val )
        theState |=  STATE_DIRTY;
      else
        theState &= ~STATE_DIRTY;
    }

    void setModifiable ( const bool val )
    {
      if ( val )
        theState |=  STATE_MODIFIABLE;
      else
        theState &= ~STATE_MODIFIABLE;
    }

    void setValid ( const bool val )
    {
      if ( val )
        theState |=  STATE_VALID;
      else
        theState &= ~STATE_VALID;
    }

    void setPrefetched ( const bool val )
    {
      if ( val )
        theState |=  STATE_PREFETCHED;
      else
        theState &= ~STATE_PREFETCHED;
    }

  private:
    Tag   theTag;
    int   theState;

  }; // class Block

#undef STATE_VALID
#undef STATE_MODIFIABLE
#undef STATE_DIRTY

  class LookupResult;

  // The cache set, implements a set of cache blocks and applies a replacement policy
  // (from a derived class)
  class Set
  {
  public:
    Set ( const int aAssociativity, const Tag aAddressBits, const int aTagShift );
    virtual ~Set ( ) {}

    virtual LookupResult lookupBlock ( const Tag           aTag,
                                       const MemoryAddress anAddress );

    // Defines the replacement policy.  Must be overridden.
    virtual Block * victim ( const UniAccessType aAccess ) = 0;
    virtual bool access ( const UniAccessType   aAccess,
                          Block               * aBlock);

    virtual bool saveState ( std::ostream & s );
    virtual bool loadState ( std::istream & s );

    MemoryAddress blockAddress ( const Block * theBlock )
    {
      int
        blockNum = theBlock - theBlocks;

      DBG_Assert ( blockNum >= 0 &&
                   blockNum < theAssociativity );

      // return MemoryAddress ( (theBlocks[blockNum].tag() << theTagShift ) + theAddressBits );
      return MemoryAddress ( theBlocks[blockNum].tag() << theTagShift );
    }

    int count ( const Tag aTag )
    {
      int
        i,
        res = 0;

      for(i = 0; i < theAssociativity; i++) {
        if(theBlocks[i].tag() == aTag) {
          res++;
        }
      }

      return res;
    }

  protected:

    Tag     theAddressBits;
    Block * theBlocks;
    int     theAssociativity, theTagShift;

  }; // class Set

  class SetLRU : public Set
  {
  public:
    SetLRU ( const int aAssociativity, const Tag aAddressBits, const int aTagShift );
    virtual Block * victim ( const UniAccessType aAccess );
    virtual bool access ( const UniAccessType   aAccess,
                          Block               * aBlock );

    virtual bool saveState ( std::ostream & s );
    virtual bool loadState ( std::istream & s );

  protected:

    int lruListHead ( void ) { return theMRUOrder[0]; }
    int lruListTail ( void ) { return theMRUOrder[theAssociativity-1]; }

    bool moveToHead ( const SetIndex aBlock );
    bool moveToTail ( const SetIndex aBlock );
    bool moveToInvTail ( const SetIndex aBlock );

  protected:

    SetIndex * theMRUOrder;

  }; // class SetLRU


  class SetRandom : public Set
  {
  public:
    SetRandom ( const int aAssociativity, const Tag aAddressBits, const int aTagShift );
    virtual Block * victim ( const UniAccessType aAccess );
  }; // class SetRandom

  class SetExternal : public Set
  {
  public:
    SetExternal ( const int aAssociativity, const Tag aAddressBits, const int aTagShift );

    virtual Block * victim ( const UniAccessType aAccess );

  }; // class SetExternal

  // The output of a cache lookup and cache line replacement/victim finder.  ARGH.
  class LookupResult
  {
  public:
    virtual ~LookupResult () {}
    LookupResult ( Set           * aSet,
                   Block         * aBlock,
                   MemoryAddress   aBlockAddress,
                   bool            aIsHit ) :
      theSet          ( aSet ),
      theBlock        ( aBlock ),
      theBlockAddress ( aBlockAddress ),
      isHit           ( aIsHit )
    {}

  public:

    // JCS note: the morphing of a LookupResult into
    // a dual-purposed lookup result and victim location
    // object is a horrible thing.  User beware.
    virtual Block & victim ( const UniAccessType access )
    {
      if(!theBlock) {
        theBlock = theSet->victim ( access );
        theBlockAddress = theSet->blockAddress ( theBlock );
      }
      isHit = true;
      return *theBlock;
    }

    bool access ( const UniAccessType access )
    {
      return theSet->access ( access, theBlock );
    }

    bool hit  ( void ) const { return isHit; }
    bool miss ( void ) const { return !isHit; }
    bool found( void ) const { return theBlock != NULL; }

    int countTag ( void ) const { return theSet->count(theBlock->tag()); }

    MemoryAddress blockAddress ( void ) const
    {
      return theBlockAddress;
    }

    // EWWWWWW!!!! ABOMINATION!!!!
    Block & block ( void ) const { return *theBlock; }

  protected:
    Set           * theSet;
    Block         * theBlock;
    MemoryAddress   theBlockAddress;
    bool            isHit;
  }; // class LookupResult


  class CacheArray
  {

  public:

    virtual ~CacheArray () {}
    CacheArray ( const int               aCacheSize,
                 const int               aAssociativity,
                 const int               aBlockSize,
                 const int               aSlices,
                 const int               aSliceNumber,
                 const ReplacementPolicy aReplacementPolicy);

    // Main array lookup function
    // virtual LookupResult operator[] ( const MemoryAddress & anAddress );
    virtual LookupResult lookup ( const MemoryAddress & anAddress, const unsigned int anIdxExtraOffsetBits );

    // Checkpoint reading/writing functions
    virtual bool saveState ( std::ostream & s );
    virtual bool loadState ( std::istream & s );

    // Addressing helper functions
    MemoryAddress blockAddress ( MemoryAddress const & anAddress ) const
    {
      return MemoryAddress ( anAddress & ~(blockOffsetMask) );
    }

    BlockOffset blockOffset ( MemoryAddress const & anAddress ) const
    {
      return BlockOffset ( anAddress & blockOffsetMask );
    }

    SetIndex makeSet ( const MemoryAddress & anAddress, const unsigned int anIdxExtraOffsetBits ) const
    {
      /* CMU-ONLY-BLOCK-BEGIN */
      //fixme
      // Another way to support R-NUCA: ignore the IdxExtraOffxetBits and call CacheArray with aSlices=NumL2Slices.
      // return ((anAddress >> setIndexShift) & setIndexMask);
      /* CMU-ONLY-BLOCK-END */
      return ((anAddress >> (setIndexShift + anIdxExtraOffsetBits)) & setIndexMask);
    }

    Tag makeTag ( const MemoryAddress & anAddress ) const
    {
      return (anAddress >> tagShift);
    }

  protected:

    int
      theCacheSize,
      theAssociativity,
      theBlockSize,
      theSlices,
      theSliceNumber,
      theReplacementPolicy;


    // Masks to address portions of the cache
    Tag
      tagMask;

    int
      setCount,
      setIndexShift,
      setIndexMask,
      tagShift;

    Tag
      blockOffsetMask;

    Set
      ** theSets;
  }; // class CacheArray

};  // namespace nCache

#endif /* _CACHE_ARRAY_HPP */
