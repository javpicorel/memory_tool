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

#ifndef FLEXUS_FASTCACHE_ASSOCIATIVECACHE_HPP_INCLUDED
#define FLEXUS_FASTCACHE_ASSOCIATIVECACHE_HPP_INCLUDED

#include <components/Common/TraceTracker.hpp>

#include <cstring>

#include <boost/function.hpp>

namespace nFastCache {

unsigned int log_base2(unsigned int num) {
  unsigned int ii = 0;
  while(num > 1) {
    ii++;
    num >>= 1;
  }
  return ii;
}

typedef unsigned short access_t;
typedef boost::function<void( unsigned long tagset, access_t evict_type) > evict_function_t;

access_t theAccessTable[256];

static const access_t kLoadAccess =           0x00; //    000----
static const access_t kReadAccess =           0x10; //    001----
static const access_t kStoreAccess =          0x20; //    010----
static const access_t kWriteAccess =          0x30; //    011----
static const access_t kDirtyEvictAccess =     0x40; //    100----
static const access_t kWritableEvictAccess =  0x50; //    101----
static const access_t kCleanEvictAccess =     0x60; //    110----
static const access_t kFetchAccess =          0x70; //    111----
static const access_t kPrefetchReadNoAlloc =  0x80; //   1000----
static const access_t kPrefetchReadAccess =   0x90; //   1001----


static const unsigned long kMiss =          0x070; //  ---111----
static const access_t kReadMiss =           0x010; //  ---001----
static const access_t kWriteMiss =          0x030; //  ---011----
static const access_t kUpgradeMiss =        0x070; //  ---111----
static const access_t kFetchMiss =          0x050; //  ---101----
static const access_t kReadNoAllocMiss =    0x060; //  ---110----
static const access_t kPrefetchHit =        0x080; //  --1000----
static const access_t kPrefHitUpgMiss =     0x0f0; //  --1111----

static const unsigned long kFill =          0x700; // 111--------
static const access_t kMissDependantFill =  0x100; // 001--------
static const access_t kFillValid =          0x100; // 001--------
static const access_t kFillWritable =       0x300; // 011--------
static const access_t kFillDirty =          0x700; // 111--------
static const access_t kPrefetchRedundant =  0x500; // 101--------

static const unsigned long kState =         0x00f; //   -----1111
static const unsigned long kNormalState =   0x007; //   -----0111
static const access_t kMissDependantState = 0x000; //   ------000
static const access_t kInvalid =            0x000; //   ------000
static const access_t kValid =              0x001; //   ------001
static const access_t kValidBit =           0x001; //   ------001
static const access_t kWritableBit =        0x002; //   ------010
static const access_t kWritable =           0x003; //   ------011
static const access_t kDirty =              0x007; //   ------111
static const access_t kDirtyBit =           0x004; //   ------100
static const access_t kDowngrade =          0x006; //   ------110
static const access_t kPrefetchedBit =      0x008; //   -----1000
static const access_t kPValid =             0x009; //   -----1001
static const access_t kPWritable =          0x00b; //   -----1011
static const access_t kPDirty =             0x00f; //   -----1111

static const access_t kInvalidTagMatch =    0x800; //1-----------

static const unsigned long kAllEvicts =     kState;
static const unsigned long kDirtyEvicts =   kDirtyBit;

void fillAccessTable() {
  //States Transition table:
    //Requests
    //  input      state   -> ret     miss      next
    //   ld(000)    i(000)     -       rd(001)  *(000)
    //   ld(000)    v(001)     -       -        v(001)
    //   ld(000)    w(011)     -       -        w(011)
    //   ld(000)    d(111)     -       -        d(111)
  theAccessTable[ kLoadAccess          | kInvalid  ] =                      kReadMiss    | kMissDependantState ;
  theAccessTable[ kLoadAccess          | kValid    ] =                                     kValid              ;
  theAccessTable[ kLoadAccess          | kWritable ] =                                     kWritable           ;
  theAccessTable[ kLoadAccess          | kDirty    ] =                                     kDirty              ;
  theAccessTable[ kLoadAccess          | kPValid   ] =                      kPrefetchHit | kValid              ;
  theAccessTable[ kLoadAccess          | kPWritable] =                      kPrefetchHit | kWritable           ;
  theAccessTable[ kLoadAccess          | kPDirty   ] =                      kPrefetchHit | kDirty              ;

    //  input      state   -> ret     miss      next
    //   rd(001)    i(000)     *(000)  rd(001)  *(000)
    //   rd(001)    v(001)     v(001)  -        v(001)
    //   rd(001)    w(011)     w(011)  -        w(011)
    //   rd(001)    d(111)     d(111)  -        w(011)
  theAccessTable[ kReadAccess          | kInvalid  ] = kMissDependantFill | kReadMiss    | kMissDependantState ;
  theAccessTable[ kReadAccess          | kValid    ] = kFillValid                        | kValid              ;
  theAccessTable[ kReadAccess          | kWritable ] = kFillWritable                     | kWritable           ;
  theAccessTable[ kReadAccess          | kDirty    ] = kFillDirty                        | kWritable           ;
  theAccessTable[ kReadAccess          | kPValid   ] = kFillValid         | kPrefetchHit | kValid              ;
  theAccessTable[ kReadAccess          | kPWritable] = kFillWritable      | kPrefetchHit | kWritable           ;
  theAccessTable[ kReadAccess          | kPDirty   ] = kFillDirty         | kPrefetchHit | kWritable           ;

    //  input      state   -> ret     miss      next
    //   st(010)    i(000)     -       wr(011)  d(111)
    //   st(010)    v(001)     -       ug(111)  d(111)
    //   st(010)    w(011)     -       -        d(111)
    //   st(010)    d(111)     -       -        d(111)
  theAccessTable[ kStoreAccess         | kInvalid  ] =                      kWriteMiss      | kDirty              ;
  theAccessTable[ kStoreAccess         | kValid    ] =                      kUpgradeMiss    | kDirty              ;
  theAccessTable[ kStoreAccess         | kWritable ] =                                        kDirty              ;
  theAccessTable[ kStoreAccess         | kDirty    ] =                                        kDirty              ;
  theAccessTable[ kStoreAccess         | kPValid   ] =                      kPrefHitUpgMiss | kDirty              ;
  theAccessTable[ kStoreAccess         | kPWritable] =                      kPrefetchHit    | kDirty              ;
  theAccessTable[ kStoreAccess         | kPDirty   ] =                      kPrefetchHit    | kDirty              ;

    //  input      state   -> ret     miss      next
    //   wr(011)    i(000)     -       wr(011)  w(011)
    //   wr(011)    v(001)     -       ug(111)  w(011)
    //   wr(011)    w(011)     -       -        w(011)
    //   wr(011)    d(111)     -       -        w(011)
  theAccessTable[ kWriteAccess         | kInvalid  ] =                      kWriteMiss      | kWritable           ;
  theAccessTable[ kWriteAccess         | kValid    ] =                      kUpgradeMiss    | kWritable           ;
  theAccessTable[ kWriteAccess         | kWritable ] =                                        kWritable           ;
  theAccessTable[ kWriteAccess         | kDirty    ] =                                        kWritable           ;
  theAccessTable[ kWriteAccess         | kPValid   ] =                      kPrefHitUpgMiss | kWritable           ;
  theAccessTable[ kWriteAccess         | kPWritable] =                      kPrefetchHit    | kWritable           ;
  theAccessTable[ kWriteAccess         | kPDirty   ] =                      kPrefetchHit    | kWritable           ;

    //  input      state   -> ret     miss      next
    //   de(100)    i(000)     -       -        d(111)
    //   de(100)    v(001)     -       -        d(111)
    //   de(100)    w(011)     -       -        d(111)
    //   de(100)    d(111)     -       -        d(111)
  theAccessTable[ kDirtyEvictAccess    | kInvalid  ] =                                     kDirty              ;
  theAccessTable[ kDirtyEvictAccess    | kValid    ] =                                     kDirty              ;
  theAccessTable[ kDirtyEvictAccess    | kWritable ] =                                     kDirty              ;
  theAccessTable[ kDirtyEvictAccess    | kDirty    ] =                                     kDirty              ;
  theAccessTable[ kDirtyEvictAccess    | kPValid   ] =                      kPrefetchHit | kDirty              ;
  theAccessTable[ kDirtyEvictAccess    | kPWritable] =                      kPrefetchHit | kDirty              ;
  theAccessTable[ kDirtyEvictAccess    | kPDirty   ] =                      kPrefetchHit | kDirty              ;

    //  input      state   -> ret     miss      next
    //   we(101)    i(000)     -       -        w(011)
    //   we(101)    v(001)     -       -        w(011)
    //   we(101)    w(011)     -       -        w(011)
    //   we(101)    d(111)     -       -        d(111)
  theAccessTable[ kWritableEvictAccess | kInvalid  ] =                                     kWritable           ;
  theAccessTable[ kWritableEvictAccess | kValid    ] =                                     kWritable           ;
  theAccessTable[ kWritableEvictAccess | kWritable ] =                                     kWritable           ;
  theAccessTable[ kWritableEvictAccess | kDirty    ] =                                     kDirty              ;
  theAccessTable[ kWritableEvictAccess | kPValid   ] =                      kPrefetchHit | kWritable           ;
  theAccessTable[ kWritableEvictAccess | kPWritable] =                      kPrefetchHit | kWritable           ;
  theAccessTable[ kWritableEvictAccess | kPDirty   ] =                      kPrefetchHit | kDirty              ;

    //  input      state   -> ret     miss      next
    //   ce(110)    i(000)     -       -        v(001)
    //   ce(110)    v(001)     -       -        v(001)
    //   ce(110)    w(011)     -       -        w(011)
    //   ce(110)    d(111)     -       -        d(111)
  theAccessTable[ kCleanEvictAccess    | kInvalid  ] =                                     kValid              ;
  theAccessTable[ kCleanEvictAccess    | kValid    ] =                                     kValid              ;
  theAccessTable[ kCleanEvictAccess    | kWritable ] =                                     kWritable           ;
  theAccessTable[ kCleanEvictAccess    | kDirty    ] =                                     kDirty              ;
  theAccessTable[ kCleanEvictAccess    | kPValid   ] =                      kPrefetchHit | kValid              ;
  theAccessTable[ kCleanEvictAccess    | kPWritable] =                      kPrefetchHit | kWritable           ;
  theAccessTable[ kCleanEvictAccess    | kPDirty   ] =                      kPrefetchHit | kDirty              ;

    //  input      state   -> ret     miss      next
    //   f(111)    i(000)     -       f(001)   v(001)
    //   f(111)    v(001)     -       -        v(001)
    //   f(111)    w(011)     -       -        w(011)
    //   f(111)    d(111)     -       -        d(111)
  theAccessTable[ kFetchAccess          | kInvalid  ] =                     kFetchMiss   | kValid              ;
  theAccessTable[ kFetchAccess          | kValid    ] =                                    kValid              ;
  theAccessTable[ kFetchAccess          | kWritable ] =                                    kWritable           ;
  theAccessTable[ kFetchAccess          | kDirty    ] =                                    kDirty              ;
  theAccessTable[ kFetchAccess          | kPValid   ] =                     kPrefetchHit | kValid              ;
  theAccessTable[ kFetchAccess          | kPWritable] =                     kPrefetchHit | kWritable           ;
  theAccessTable[ kFetchAccess          | kPDirty   ] =                     kPrefetchHit | kDirty              ;

    //  input      state   -> ret     miss      next
    //  pr(1000)    i(000)     -       rd(001)  *(000)
    //  pr(1000)    v(001)     -       -        v(001)
    //  pr(1000)    w(011)     -       -        w(011)
    //  pr(1000)    d(111)     -       -        d(111)
  theAccessTable[ kPrefetchReadNoAlloc  | kInvalid  ] =                 kReadNoAllocMiss | kInvalid            ;
  theAccessTable[ kPrefetchReadNoAlloc  | kValid    ] = kPrefetchRedundant               | kValid              ;
  theAccessTable[ kPrefetchReadNoAlloc  | kWritable ] = kPrefetchRedundant               | kWritable           ;
  theAccessTable[ kPrefetchReadNoAlloc  | kDirty    ] = kPrefetchRedundant               | kDirty              ;
  theAccessTable[ kPrefetchReadNoAlloc  | kPValid   ] = kPrefetchRedundant               | kPValid             ;
  theAccessTable[ kPrefetchReadNoAlloc  | kPWritable] = kPrefetchRedundant               | kPWritable          ;
  theAccessTable[ kPrefetchReadNoAlloc  | kPDirty   ] = kPrefetchRedundant               | kPDirty             ;

    //  input      state   -> ret     miss      next
    //  pr(1001)    i(000)     -       rd(001)  *(000)
    //  pr(1001)    v(001)     -       -        v(001)
    //  pr(1001)    w(011)     -       -        w(011)
    //  pr(1001)    d(111)     -       -        d(111)
  theAccessTable[ kPrefetchReadAccess   | kInvalid  ] =                      kReadMiss   | kMissDependantState ;
  theAccessTable[ kPrefetchReadAccess   | kValid    ] =                                    kValid              ;
  theAccessTable[ kPrefetchReadAccess   | kWritable ] =                                    kWritable           ;
  theAccessTable[ kPrefetchReadAccess   | kDirty    ] =                                    kDirty              ;
  theAccessTable[ kPrefetchReadAccess   | kPValid   ] =                                    kPValid             ;
  theAccessTable[ kPrefetchReadAccess   | kPWritable] =                                    kPWritable          ;
  theAccessTable[ kPrefetchReadAccess   | kPDirty   ] =                                    kPDirty             ;

    //*'d next state, mod, and ret fields depend on the return value from the
    //next cache level as follows:
      //input ret  -> next ret  mod
      //  ld   v       v    -    !
      //  ld   w       w    -    !
      //  ld   d       d    -    d
      //  rd   v       v    v    !
      //  rd   w       w    w    !
      //  rd   d       w    d    !

};

struct AssociativeCache {

  static const unsigned long kStateMask = 3;

  unsigned long * theArray;
  int theSetShift;
  int theTagShift;
  unsigned long theSetMask;
  unsigned long theSetSize;
  int theAssociativity;
  unsigned long theEvictMask;
  evict_function_t evict;
  int theIndex;
  Flexus::SharedTypes::tFillLevel theLevel;

  unsigned char * theArray_base;
  unsigned long theNumSets;

  bool theTraceTrackerFlag;

  AssociativeCache(int aBlockSize, int aNumSets, int anAssociativity, bool aCleanEvict, evict_function_t anEvict, int anIndex, Flexus::SharedTypes::tFillLevel aLevel, bool aTraceTrackerFlag) {
    theTraceTrackerFlag = aTraceTrackerFlag;

    long array_size = sizeof(unsigned long) * aNumSets * anAssociativity;
    theArray_base =  new unsigned char [ array_size + 128];
    theArray = reinterpret_cast<unsigned long *>
      ( (reinterpret_cast<unsigned long>(theArray_base) + 127UL) & (~127UL)
      ); //128-byte align theArray

    theNumSets = aNumSets;
    theAssociativity = anAssociativity;
    if (aCleanEvict) {
      theEvictMask = kAllEvicts;
    } else {
      theEvictMask = kDirtyEvicts;
    }
    evict = anEvict;

    theIndex = anIndex;
    theLevel = aLevel;

    //Calculate masks and shifts
    theSetShift = log_base2( aBlockSize ) ;
    theTagShift = log_base2( aNumSets ) ;
    theSetMask = ( aBlockSize * aNumSets - 1);
    theSetMask &= ~kStateMask;

    std::memset( theArray, 0, array_size );
  }


  void apply_returned_state( unsigned long * frame, unsigned long returned_state) {
    *frame = *frame | ( returned_state);
  }

  //Return 0 for no miss
  unsigned long * access( unsigned long tagset, access_t & access_type) {

    unsigned long set_bits = (tagset & theSetMask);
    unsigned long set = (set_bits) >> theSetShift;
    unsigned long * set_base = theArray + set * theAssociativity;

    bool allocateOnMiss = true;
    if(access_type == kPrefetchReadNoAlloc) {
      allocateOnMiss = false;
    }

    //Check for MRU hit
    unsigned long mru_tag = *set_base;
    DBG_(VVerb, ( << std::hex << "access_type: " << access_type << " tagset: " << tagset << " set_bits: " << set_bits << " set: " << set << " set_base: " << set_base << " mru_tag: " << mru_tag << std::dec) );
    if (   /* tags match   */    ((mru_tag & ~kState) == tagset) ) {
        access_t apply_after = 0;
        if(~mru_tag & kValid) {
          if (theTraceTrackerFlag) {
            theTraceTracker.invalidTagRefill(theIndex, theLevel, tagset);
          }
          apply_after = kInvalidTagMatch;
        }

        //Tag match - compute state transition and further action
        access_t access = access_type | (mru_tag & kState);
        access_type = theAccessTable[access];
        *set_base = (*set_base & ~kState) | (access_type & kState);
        DBG_(VVerb, ( << std::hex << "TAG MATCH - result: *set_base: " << *set_base << " access_type: " << access_type << std::dec ));
        access_type |= apply_after;
        return set_base;
    }

    //Non-MRU hits imply reordering of LRU chain.
    unsigned long * set_iter = set_base;
    for (int i = 1; i < theAssociativity; ++i) {
      set_iter += 1;
      unsigned long mru_tag = *set_iter;
      DBG_(VVerb, ( << "way: " << i << std::hex << " tag: " << mru_tag << std::dec ));
      if (  /* tags match   */  ((mru_tag & ~kState ) == tagset) ) {
          access_t apply_after = 0;
          if(~mru_tag & kValid) {
            if (theTraceTrackerFlag) {
              theTraceTracker.invalidTagRefill(theIndex, theLevel, tagset);
            }
            apply_after = kInvalidTagMatch;
          }

          //Compute state transition and further actions
          access_t access = access_type | (mru_tag & kState);
          access_type = theAccessTable[access];
          mru_tag = (mru_tag & ~kState) | (access_type & kState);

          if(allocateOnMiss) {
            //Slide other set contents down MRU chain
            memmove(set_base+1,set_base, sizeof(unsigned long)*i);

            //Insert this tag at head of MRU chain
            *set_base = mru_tag;
          }
          DBG_(VVerb, ( << std::hex << "TAG MATCH - result: *set_base: " << *set_base << " access_type: " << access_type << std::dec ));
          access_type |= apply_after;
          return set_base;
      }
    }

    if(!allocateOnMiss) {
      access_type = theAccessTable[access_type];
      return 0;
    }

    //Miss Handler

      //Handle WB/Evict of LRU block
        //set_iter points to LRU block
        access_t evict_type = *set_iter & theEvictMask;
        unsigned long evict_tagset = (*set_iter & ~kState) | set_bits;

        if (theTraceTrackerFlag) {
          if(*set_iter & kValid) {
            theTraceTracker.eviction(theIndex, theLevel, evict_tagset, (evict_type==0));
          } else {
            theTraceTracker.invalidTagReplace(theIndex, theLevel, evict_tagset);
          }
        }

        if(*set_iter & kPrefetchedBit) {
          // evicted prefetch
        }

        if (evict_type) {
          unsigned long state = (*set_iter & kNormalState);
          *set_iter = (*set_iter & ~kState); // make sure line is invalidated
          evict( evict_tagset, state );
        }

      //Slide other set contents down MRU chain
      memmove(set_base+1,set_base, sizeof(unsigned long)*(theAssociativity-1));

      //Compute state transition and further actions
      access_type = theAccessTable[access_type];
      *set_base = tagset | (access_type & kState);
      DBG_(VVerb, ( << std::hex << "MISS after update *set_base: " << *set_base << " access_type: " << access_type << std::dec ));
      return set_base;
  }


  std::pair<bool,bool> invalidate( unsigned long tagset) {
    unsigned long set_bits = (tagset & theSetMask);
    unsigned long set = (set_bits) >> theSetShift;
    unsigned long * set_base = theArray + set * theAssociativity;

    DBG_(VVerb, ( << std::hex << "Invalidate tagset: " << tagset << " set_bits: " << set_bits << " set: " << set << " set_base: " << set_base << std::dec) );

    unsigned long * set_iter = set_base;
    unsigned long * lru_frame = set_base + (theAssociativity-1);
    bool ret_valid_flag = false, ret_dirty_flag = false;

    //Find matching way
    for (int i = 1; i <= theAssociativity; ++i) {
      DBG_(VVerb, ( << "way: " << i-1 << std::hex << " tag: " << *set_iter << std::dec ));
      if (  /* tags match   */  ((*set_iter & ~kState ) == tagset) ) {
          DBG_(VVerb, ( << std::hex << "Invalidate matched: " << *set_iter << std::dec ));
          if(*set_iter & kValid) {
            ret_valid_flag = true;
            if (theTraceTrackerFlag) {
              theTraceTracker.invalidTagCreate(theIndex, theLevel, tagset);
            }
          }
          if(*set_iter & kPrefetchedBit) {
            // invalidated prefetch
          }
          ret_dirty_flag = (*set_iter & kDirtyBit);

          ret_dirty_flag = (*set_iter & kDirtyBit);

          //Slide all sets up MRU chain
          memmove(set_iter,set_iter+1, sizeof(unsigned long)*(theAssociativity-i));
          //Set block to invalid state at MRU tail
          *lru_frame = tagset;
          return std::make_pair(ret_valid_flag,ret_dirty_flag);
      }
      set_iter += 1;
    }
    return std::make_pair(false, false);
  }

  bool returnReq( unsigned long tagset) {
    unsigned long set_bits = (tagset & theSetMask);
    unsigned long set = (set_bits) >> theSetShift;
    unsigned long * set_base = theArray + set * theAssociativity;

    DBG_(VVerb, ( << std::hex << "ReturnReq tagset: " << tagset << " set_bits: " << set_bits << " set: " << set << " set_base: " << set_base << std::dec) );
    unsigned long * set_iter = set_base;

    //Find matching way
    for (int i = 1; i <= theAssociativity; ++i) {
      DBG_(VVerb, ( << "way: " << i-1 << std::hex << " tag: " << *set_iter << std::dec ));
      if (  /* tags match   */  ((*set_iter & ~kState ) == tagset) ) {
          DBG_(VVerb, ( << std::hex << "ReturnReq matched: " << *set_iter << std::dec ));
          return true;
      }
      set_iter += 1;
    }
    return false;
  }

  bool downgrade( unsigned long tagset) {
    unsigned long set_bits = (tagset & theSetMask);
    unsigned long set = (set_bits) >> theSetShift;
    unsigned long * set_iter = theArray + set * theAssociativity;
    bool ret_dirty_flag;

    DBG_(VVerb, ( << std::hex << "Downgrade tagset: " << tagset << " set_bits: " << set_bits << " set: " << set << std::dec) );

    //Find matching way
    for (int i = 0; i < theAssociativity; ++i) {
      DBG_(VVerb, ( << "way: " << i << std::hex << " tag: " << *set_iter << std::dec ));
      if (  /* tags match   */  ((*set_iter & ~kState ) == tagset) ) {
          DBG_(VVerb, ( << std::hex << "Downgrade matched: " << *set_iter << std::dec ));
          //Remove dirty/writable state
          ret_dirty_flag = (*set_iter & kDirtyBit);
          *set_iter &= ~(kDowngrade);
          return ret_dirty_flag;
      }
      set_iter += 1;
    }
    return false;
  }

  void saveState( std::ostream & s ) {
    static const int kSave_ValidBit = 1;
    static const int kSave_DirtyBit = 2;
    static const int kSave_ModifiableBit = 4;
    static const int kSave_PrefetchedBit = 8;

    int shift = theSetShift + log_base2( theNumSets ) ;

    unsigned long * block_iter = theArray;
    for ( unsigned int i = 0; i < theNumSets ; i++ ) {
      s << "{";
      for ( int j = 0; j < theAssociativity; j++ ) {
        unsigned long state = *block_iter;
        unsigned long tag = (*block_iter) >> shift;
        int save_state
              =  ( state & kValidBit ? kSave_ValidBit : 0)
              |  ( state & kDirtyBit ? kSave_DirtyBit : 0)
              |  ( state & kWritableBit ? kSave_ModifiableBit : 0)
              |  ( state & kPrefetchedBit ? kSave_PrefetchedBit : 0)
              ;
        s << "[ " << save_state
          << " "  << static_cast<unsigned long long>(tag)
          << " ]";
        ++block_iter;
      }
      s << "} < ";
      for ( int j = 0; j < theAssociativity; j++ ) {
        s << j << " ";
      }
      s << "> " << std::endl;
    }
  }

  bool loadState( std::istream & s ) {
    static const int kSave_ValidBit = 1;
    static const int kSave_DirtyBit = 2;
    static const int kSave_ModifiableBit = 4;
    static const int kSave_PrefetchedBit = 8;

    unsigned long * block_iter = theArray;
    char paren; int dummy;
    int load_state, state;
    unsigned long long load_tag;
    for ( unsigned int i = 0; i < theNumSets ; i++ ) {
      s >> paren; // {
      if ( paren != '{' ) {
        DBG_ (Crit, (<< "Expected '{' when loading checkpoint" ) );
        return false;
      }
      for ( int j = 0; j < theAssociativity; j++ ) {
        s >> paren >> load_state >> load_tag >> paren;
        state = (load_state & kSave_ValidBit ? kValidBit : 0)
              | (load_state & kSave_DirtyBit ? kDirtyBit : 0)
              | (load_state & kSave_ModifiableBit ? kWritableBit : 0)
              | (load_state & kSave_PrefetchedBit ? kPrefetchedBit : 0)
              ;
        *block_iter = (load_tag << (theTagShift+theSetShift)) | (i << theSetShift) | state;
        block_iter++;
      }
      s >> paren; // }
      if ( paren != '}' ) {
        DBG_ (Crit, (<< "Expected '}' when loading checkpoint" ) );
        return false;
      }

      // useless associativity information
      s >> paren; // <
      if ( paren != '<' ) {
        DBG_ (Crit, (<< "Expected '<' when loading checkpoint" ) );
        return false;
      }
      for ( int j = 0; j < theAssociativity; j++ ) {
        s >> dummy;
      }
      s >> paren; // >
      if ( paren != '>' ) {
        DBG_ (Crit, (<< "Expected '>' when loading checkpoint" ) );
        return false;
      }
    }
    return true;
  }

};


}  // namespace nFastCache

#endif /* FLEXUS_FASTCACHE_ASSOCIATIVECACHE_HPP_INCLUDED */
