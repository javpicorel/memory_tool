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

#ifndef FLEXUS_FASTCMPCACHE_ASSOCIATIVECACHE_HPP_INCLUDED
#define FLEXUS_FASTCMPCACHE_ASSOCIATIVECACHE_HPP_INCLUDED

#include <cstring>

#include <boost/function.hpp>

namespace nFastCMPCache {

unsigned int log_base2(unsigned int num) {
  unsigned int ii = 0;
  while(num > 1) {
    ii++;
    num >>= 1;
  }
  return ii;
}

typedef unsigned long long  block_address_t;
typedef unsigned short access_t;
typedef boost::function<void( unsigned long long tagset, access_t evict_type) > evict_function_t;

typedef boost::function<void( unsigned long long * set_base, int hit_way,   unsigned long long hit_tag   )> repl_hit_t;
typedef boost::function<void( unsigned long long * set_base, int alloc_way, unsigned long long new_tag   )> repl_miss_t;
typedef boost::function<int ( unsigned long long * set_base                                         )> repl_victim_t;
typedef boost::function<void( unsigned long long * set_base, int inval_way, unsigned long long inval_tag )> repl_inval_t;

static const unsigned long long kAccess =              0x00F8; // ---- ---- 1111 1---
static const access_t kLoadAccess =                    0x0000; // ---- ---- 0000 0---
static const access_t kReadAccess =                    0x0008; // ---- ---- 0000 1---
static const access_t kFetchAccess =                   0x0018; // ---- ---- 0001 1---
static const access_t kStoreAccess =                   0x0020; // ---- ---- 0010 0---
static const access_t kWriteAccess =                   0x0028; // ---- ---- 0010 1---
static const access_t kUpgradeAccess =                 0x0038; // ---- ---- 0011 1---
static const access_t kCleanEvictAccess =              0x0048; // ---- ---- 0100 1---
static const access_t kWritableEvictAccess =           0x0058; // ---- ---- 0101 1---
static const access_t kDirtyEvictAccess =              0x0078; // ---- ---- 0111 1--- ///////
static const access_t kCleanEvictNonAllocatingAccess = 0x0068; // ---- ---- 0110 1---
static const access_t kCleanEvictAllocOnFreeAccess =   0x0040; // ---- ---- 0100 0---
static const access_t kDirtyEvictAllocOnFreeAccess =   0x00F8; // ---- ---- 1111 1---

static const unsigned long long kCMPDirState =         0x0700; // ---- -111 ---- ----
static const access_t kCMPDirInvalid =                 0x0000; // ---- -000 ---- ----
static const access_t kCMPDirShared =                  0x0100; // ---- -001 ---- ----
static const access_t kCMPDirExclusive =               0x0300; // ---- -011 ---- ----

static const unsigned long long kState =               0x0007; // ---- ---- ---- -111
  // final line states
static const access_t kInvalid =                       0x0000; // ---- ---- ---- -000
static const access_t kValid =                         0x0001; // ---- ---- ---- -001
static const access_t kWritable =                      0x0002; // ---- ---- ---- -010
static const access_t kDirty =                         0x0003; // ---- ---- ---- -011
static const access_t kDirtyBitMask =                  0x0002; // ---- ---- ---- -010
  // transient states, resolved by the controller impl.
static const access_t kMissDependantState =            0x0004; // ---- ---- ---- -100
static const access_t kSnoopDependantState =           0x0005; // ---- ---- ---- -101
static const access_t kCMPDirDependantState =          0x0006; // ---- ---- ---- -110
static const access_t kAllocOnFreeFrameState =         0x0007; // ---- ---- ---- -111 ///////

static const unsigned long long kMiss =                0x00F8; // ---- ---- 1111 1---
static const access_t kReadMiss =                      0x0008; // ---- ---- 0000 1---
static const access_t kFetchMiss =                     0x0018; // ---- ---- 0001 1---
static const access_t kWriteMiss =                     0x0028; // ---- ---- 0010 1---
static const access_t kUpgradeMiss =                   0x0038; // ---- ---- 0011 1---
static const access_t kLineDependantMiss =             0x0048; // ---- ---- 0100 1---
static const access_t kCleanEvictNonAllocatingMiss =   0x00B0; // ---- ---- 1011 0---

static const unsigned long long kFill =                0x0700; // ---- -111 ---- ----
static const access_t kNoFill =                        0x0000; // ---- -000 ---- ----
static const access_t kMissDependantFill =             0x0200; // ---- -010 ---- ----
static const access_t kSnoopDependantFill =            0x0600; // ---- -110 ---- ----
static const access_t kFillValid =                     0x0100; // ---- -001 ---- ----
static const access_t kFillWritable =                  0x0300; // ---- -011 ---- ----
static const access_t kFillDirty =                     0x0700; // ---- -111 ---- ---- ///////

static const unsigned long long kSnoop =               0x3800; // --11 1--- ---- ----
static const access_t kNoSnoop =                       0x0000; // --00 0--- ---- ----
static const access_t kReturnReq =                     0x0800; // --00 1--- ---- ----
static const access_t kInvalidate =                    0x1000; // --01 0--- ---- ----
static const access_t kReturnInval =                   0x1800; // --01 1--- ---- ----
static const access_t kDowngrade =                     0x2800; // --10 1--- ---- ----

static const unsigned long long kAllEvicts =           kState;
static const unsigned long long kDirtyEvicts =         kDirtyBitMask;

static const access_t kPoison =                        0xFFFF; // 111 111 111 1111 111

access_t theAccessTable[kAccess | kCMPDirState | kState];

void fillAccessTable() {
  //States Transition table:
    //Requests
    //   input       state    stateCMPDir   ->  snoop     ret     miss      next
    //   ld(0000)    i(000)   i(000)            -         -       rd(0001)  *(010)
    //   ld(0000)    i(000)   s(001)            X         X       X         X
    //   ld(0000)    i(000)   e(011)            X         X       X         X

    //   ld(0000)    v(001)   i(000)            -         -       -         v(001)
    //   ld(0000)    v(001)   s(001)            X         X       X         X
    //   ld(0000)    v(001)   e(011)            X         X       X         X

    //   ld(0000)    w(011)   i(000)            -         -       -         w(011)
    //   ld(0000)    w(011)   s(001)            X         X       X         X
    //   ld(0000)    w(011)   e(011)            X         X       X         X

    //   ld(0000)    d(111)   i(000)            -         -       -         d(111)
    //   ld(0000)    d(111)   s(001)            X         X       X         X
    //   ld(0000)    d(111)   e(011)            X         X       X         X
    //
    // kMissDependantState:
    // *'d next state and mod fields depend on the return value from the next cache level as follows
    // input ret  -> next mod
    //   ld   v       v    !
    //   ld   w       w    !
    //   ld   d       d    d
    //            input                  state       stateCMPDir        -> snoop          ret                   miss           next
  theAccessTable[ kLoadAccess          | kInvalid  | kCMPDirInvalid   ] =                                       kReadMiss    | kMissDependantState  ;
  theAccessTable[ kLoadAccess          | kInvalid  | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kLoadAccess          | kInvalid  | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

  theAccessTable[ kLoadAccess          | kValid    | kCMPDirInvalid   ] =                                                      kValid               ;
  theAccessTable[ kLoadAccess          | kValid    | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kLoadAccess          | kValid    | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

  theAccessTable[ kLoadAccess          | kWritable | kCMPDirInvalid   ] =                                                      kWritable            ;
  theAccessTable[ kLoadAccess          | kWritable | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kLoadAccess          | kWritable | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

  theAccessTable[ kLoadAccess          | kDirty    | kCMPDirInvalid   ] =                                                      kDirty               ;
  theAccessTable[ kLoadAccess          | kDirty    | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kLoadAccess          | kDirty    | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

    //   input       state    stateCMPDir   ->  snoop     ret     miss      next
    //   rd(0001)    i(000)   i(000)            -         *(010)  rd(0001)  i(000)
    //   rd(0001)    i(000)   s(001)            ret(001)  v(001)  -         *(101)
    //   rd(0001)    i(000)   e(011)            dng(101)  v(001)  -         *(110)

    //   rd(0001)    v(001)   i(000)            -         v(001)  -         v(001)
    //   rd(0001)    v(001)   s(001)            -         v(001)  -         v(001)
    //   rd(0001)    v(001)   e(011)            X         X       X         X     

    //   rd(0001)    w(011)   i(000)            -         v(001)  -         w(011)
    //   rd(0001)    w(011)   s(001)            -         v(001)  -         w(011)
    //   rd(0001)    w(011)   e(011)            dng(101)  v(001)  -         *(110)

    //   rd(0001)    d(111)   i(000)            -         v(001)  -         d(111)
    //   rd(0001)    d(111)   s(001)            -         v(001)  -         d(111)
    //   rd(0001)    d(111)   e(011)            dng(101)  v(001)  -         d(111)  Assumes same block sizes
    //
    // kMissDependantFill (010):
    // *'d ret field depends on the return value from the next cache level as follows
    // input ret  -> ret
    //   rd   v       v
    //   rd   w       w
    //   rd   d       d
    //
    // kCMPDirDependantState (101):
    // *'d next field depends on the state of the cache line on chip w.r.t. the off-chip world
    // state -> next
    //   v       v
    //   w       w
    //   d       d
    //
    // kSnoopDependantState (110):
    // *'d next state field depends on the return value from the snoop to the previous cache level as follows
    // input   ret      -> next
    //   rd  xxxAck          w
    //   rd  xxxUpdAck       d
    //            input                  state       stateCMPDir        -> snoop          ret                   miss           next
  theAccessTable[ kReadAccess          | kInvalid  | kCMPDirInvalid   ] =                 kMissDependantFill  | kReadMiss    | kInvalid             ;
  theAccessTable[ kReadAccess          | kInvalid  | kCMPDirShared    ] =  kReturnReq   | kFillValid                         | kCMPDirDependantState;
  theAccessTable[ kReadAccess          | kInvalid  | kCMPDirExclusive ] =  kDowngrade   | kFillValid                         | kSnoopDependantState ;

  theAccessTable[ kReadAccess          | kValid    | kCMPDirInvalid   ] =                 kFillValid                         | kValid               ;
  theAccessTable[ kReadAccess          | kValid    | kCMPDirShared    ] =                 kFillValid                         | kValid               ;
  theAccessTable[ kReadAccess          | kValid    | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

  theAccessTable[ kReadAccess          | kWritable | kCMPDirInvalid   ] =                 kFillValid                         | kWritable            ;
  theAccessTable[ kReadAccess          | kWritable | kCMPDirShared    ] =                 kFillValid                         | kWritable            ;
  theAccessTable[ kReadAccess          | kWritable | kCMPDirExclusive ] =  kDowngrade   | kFillValid                         | kSnoopDependantState ;

  theAccessTable[ kReadAccess          | kDirty    | kCMPDirInvalid   ] =                 kFillValid                         | kDirty               ;
  theAccessTable[ kReadAccess          | kDirty    | kCMPDirShared    ] =                 kFillValid                         | kDirty               ;
  theAccessTable[ kReadAccess          | kDirty    | kCMPDirExclusive ] =  kDowngrade   | kFillValid                         | kDirty               ;

    //   input       state    stateCMPDir   ->  snoop     ret     miss      next
    //    f(0011)    i(000)   i(000)            -         v(001)  fm(0011)  i(000)
    //    f(0011)    i(000)   s(001)            ret(001)  v(001)  -         v(001)
    //    f(0011)    i(000)   e(011)            dng(101)  v(001)  -         *(110)

    //    f(0011)    v(001)   i(000)            -         v(001)  -         v(001)
    //    f(0011)    v(001)   s(001)            -         v(001)  -         v(001)
    //    f(0011)    v(001)   e(011)            X         X       X         X     

    //    f(0011)    w(011)   i(000)            -         v(001)  -         w(011)
    //    f(0011)    w(011)   s(001)            -         v(001)  -         w(011)
    //    f(0011)    w(011)   e(011)            dng(101)  v(001)  -         *(110)

    //    f(0011)    d(111)   i(000)            -         v(001)  -         d(111)
    //    f(0011)    d(111)   s(001)            -         v(001)  -         d(111)
    //    f(0011)    d(111)   e(011)            dng(101)  v(001)  -         d(111)  Assumes same block sizes
    //
    // kSnoopDependantState:
    // *'d next state field depends on the return value from the snoop to the previous cache level as follows
    // input   ret      -> next
    //   rd  xxxAck          w
    //   rd  xxxUpdAck       d
    //            input                  state       stateCMPDir        -> snoop          ret                   miss           next
  theAccessTable[ kFetchAccess         | kInvalid  | kCMPDirInvalid   ] =                 kFillValid          | kFetchMiss   | kInvalid             ;
  theAccessTable[ kFetchAccess         | kInvalid  | kCMPDirShared    ] =  kReturnReq   | kFillValid                         | kCMPDirDependantState;
  theAccessTable[ kFetchAccess         | kInvalid  | kCMPDirExclusive ] =  kDowngrade   | kFillValid                         | kSnoopDependantState ;

  theAccessTable[ kFetchAccess         | kValid    | kCMPDirInvalid   ] =                 kFillValid                         | kValid               ;
  theAccessTable[ kFetchAccess         | kValid    | kCMPDirShared    ] =                 kFillValid                         | kValid               ;
  theAccessTable[ kFetchAccess         | kValid    | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

  theAccessTable[ kFetchAccess         | kWritable | kCMPDirInvalid   ] =                 kFillValid                         | kWritable            ;
  theAccessTable[ kFetchAccess         | kWritable | kCMPDirShared    ] =                 kFillValid                         | kWritable            ;
  theAccessTable[ kFetchAccess         | kWritable | kCMPDirExclusive ] =  kDowngrade   | kFillValid                         | kSnoopDependantState ;

  theAccessTable[ kFetchAccess         | kDirty    | kCMPDirInvalid   ] =                 kFillValid                         | kDirty               ;
  theAccessTable[ kFetchAccess         | kDirty    | kCMPDirShared    ] =                 kFillValid                         | kDirty               ;
  theAccessTable[ kFetchAccess         | kDirty    | kCMPDirExclusive ] =  kDowngrade   | kFillValid                         | kDirty               ;

    //   input       state    stateCMPDir   ->  snoop     ret     miss      next
    //   st(0100)    i(000)   i(000)            -         -       wr(0101)  d(111)
    //   st(0100)    i(000)   s(001)            X         X       X         X
    //   st(0100)    i(000)   e(011)            X         X       X         X

    //   st(0100)    v(001)   i(000)            -         -       ug(0111)  d(111)
    //   st(0100)    v(001)   s(001)            X         X       X         X
    //   st(0100)    v(001)   e(011)            X         X       X         X

    //   st(0100)    w(011)   i(000)            -         -       -         d(111)
    //   st(0100)    w(011)   s(001)            X         X       X         X
    //   st(0100)    w(011)   e(011)            X         X       X         X

    //   st(0100)    d(111)   i(000)            -         -       -         d(111)
    //   st(0100)    d(111)   s(001)            X         X       X         X
    //   st(0100)    d(111)   e(011)            X         X       X         X
    //            input                  state       stateCMPDir        -> snoop          ret                   miss           next
  theAccessTable[ kStoreAccess         | kInvalid  | kCMPDirInvalid   ] =                                       kWriteMiss   | kDirty               ;
  theAccessTable[ kStoreAccess         | kInvalid  | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kStoreAccess         | kInvalid  | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

  theAccessTable[ kStoreAccess         | kValid    | kCMPDirInvalid   ] =                                       kUpgradeMiss | kDirty               ;
  theAccessTable[ kStoreAccess         | kValid    | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kStoreAccess         | kValid    | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

  theAccessTable[ kStoreAccess         | kWritable | kCMPDirInvalid   ] =                                                      kDirty               ;
  theAccessTable[ kStoreAccess         | kWritable | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kStoreAccess         | kWritable | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

  theAccessTable[ kStoreAccess         | kDirty    | kCMPDirInvalid   ] =                                                      kDirty               ;
  theAccessTable[ kStoreAccess         | kDirty    | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kStoreAccess         | kDirty    | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

    //   input       state    stateCMPDir   ->  snoop     ret     miss      next
    //   wr(0101)    i(000)   i(000)            -         w(011)  wr(0101)  i(000)
    //   wr(0101)    i(000)   s(001)            rei(011)  w(011)  ug(0111)  w(011)
    //   wr(0101)    i(000)   e(011)            inv(010)  *(110)  -         w(011)  Assumes same block sizes

    //   wr(0101)    v(001)   i(000)            -         w(011)  ug(0111)  w(011)
    //   wr(0101)    v(001)   s(001)            inv(010)  w(011)  ug(0111)  w(011)
    //   wr(0101)    v(001)   e(011)            X         X       X         X

    //   wr(0101)    w(011)   i(000)            -         w(011)  -         w(011)
    //   wr(0101)    w(011)   s(001)            inv(010)  w(011)  -         w(011)
    //   wr(0101)    w(011)   e(011)            inv(010)  *(110)  -         w(011)  Assumes same block sizes

    //   wr(0101)    d(111)   i(000)            -         d(111)  -         w(011)  Assumes same block sizes
    //   wr(0101)    d(111)   s(001)            inv(010)  d(111)  -         w(011)  Assumes same block sizes
    //   wr(0101)    d(111)   e(011)            inv(010)  d(111)  -         w(011)  Assumes same block sizes
    //
    // kSnoopDependantFill:
    // *'d ret field depends on the return value from the snoop to the previous cache level as follows
    // input   ret     -> next
    //   wr  xxxAck         w
    //   wr  xxxUpdAck      d
    //            input                  state       stateCMPDir        -> snoop          ret                   miss           next
  theAccessTable[ kWriteAccess         | kInvalid  | kCMPDirInvalid   ] =                 kFillWritable       | kWriteMiss   | kInvalid             ;
  theAccessTable[ kWriteAccess         | kInvalid  | kCMPDirShared    ] =  kReturnInval | kFillWritable       | kLineDependantMiss | kWritable            ;
  theAccessTable[ kWriteAccess         | kInvalid  | kCMPDirExclusive ] =  kInvalidate  | kSnoopDependantFill                | kWritable            ;

  theAccessTable[ kWriteAccess         | kValid    | kCMPDirInvalid   ] =                 kFillWritable       | kUpgradeMiss | kWritable            ;
  theAccessTable[ kWriteAccess         | kValid    | kCMPDirShared    ] =  kInvalidate  | kFillWritable       | kLineDependantMiss | kWritable            ;
  theAccessTable[ kWriteAccess         | kValid    | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

  theAccessTable[ kWriteAccess         | kWritable | kCMPDirInvalid   ] =                 kFillWritable                      | kWritable            ;
  theAccessTable[ kWriteAccess         | kWritable | kCMPDirShared    ] =  kInvalidate  | kFillWritable                      | kWritable            ;
  theAccessTable[ kWriteAccess         | kWritable | kCMPDirExclusive ] =  kInvalidate  | kSnoopDependantFill                | kWritable            ;

  theAccessTable[ kWriteAccess         | kDirty    | kCMPDirInvalid   ] =                 kFillDirty                         | kWritable            ;
  theAccessTable[ kWriteAccess         | kDirty    | kCMPDirShared    ] =  kInvalidate  | kFillDirty                         | kWritable            ;
  theAccessTable[ kWriteAccess         | kDirty    | kCMPDirExclusive ] =  kInvalidate  | kFillDirty                         | kWritable            ;

    //   input       state    stateCMPDir   ->  snoop     ret     miss      next
    //   ug(0111)    i(000)   i(000)            X         X       X         X
    //   ug(0111)    i(000)   s(001)            inv(010)  w(011)  ug(0111)  i(000)
    //   ug(0111)    i(000)   e(011)            X         X       X         X

    //   ug(0111)    v(001)   i(000)            X         X       X         X
    //   ug(0111)    v(001)   s(001)            inv(010)  w(011)  ug(0111)  w(011)
    //   ug(0111)    v(001)   e(011)            X         X       X         X

    //   ug(0111)    w(011)   i(000)            X         X       X         X
    //   ug(0111)    w(011)   s(001)            inv(010)  w(011)  -         w(011)
    //   ug(0111)    w(011)   e(011)            X         X       X         X

    //   ug(0111)    d(111)   i(000)            X         X       X         X
    //   ug(0111)    d(111)   s(001)            inv(010)  d(111)  -         w(011)  Assumes same block sizes
    //   ug(0111)    d(111)   e(011)            X         X       X         X
    //            input                  state       stateCMPDir        -> snoop          ret                   miss           next
  theAccessTable[ kUpgradeAccess       | kInvalid  | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kUpgradeAccess       | kInvalid  | kCMPDirShared    ] =  kInvalidate  | kFillWritable       | kLineDependantMiss | kInvalid             ;
  theAccessTable[ kUpgradeAccess       | kInvalid  | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

  theAccessTable[ kUpgradeAccess       | kValid    | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kUpgradeAccess       | kValid    | kCMPDirShared    ] =  kInvalidate  | kFillWritable       | kLineDependantMiss | kWritable            ;
  theAccessTable[ kUpgradeAccess       | kValid    | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

  theAccessTable[ kUpgradeAccess       | kWritable | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kUpgradeAccess       | kWritable | kCMPDirShared    ] =  kInvalidate  | kFillWritable                      | kWritable            ;
  theAccessTable[ kUpgradeAccess       | kWritable | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

  theAccessTable[ kUpgradeAccess       | kDirty    | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kUpgradeAccess       | kDirty    | kCMPDirShared    ] =  kInvalidate  | kDirty                             | kWritable            ;
  theAccessTable[ kUpgradeAccess       | kDirty    | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

    //   input       state    stateCMPDir   ->  snoop     ret     miss      next
    //   de(1111)    i(000)   i(000)            X         X       X         X     
    //   de(1111)    i(000)   s(001)            X         X       X         X     
    //   de(1111)    i(000)   e(011)            -         -       -         d(111)     

    //   de(1111)    v(001)   i(000)            X         X       X         X     
    //   de(1111)    v(001)   s(001)            X         X       X         X     
    //   de(1111)    v(001)   e(011)            X         X       X         X     

    //   de(1111)    w(011)   i(000)            X         X       X         X     
    //   de(1111)    w(011)   s(001)            X         X       X         X     
    //   de(1111)    w(011)   e(011)            -         -       -         d(111)     

    //   de(1111)    d(111)   i(000)            X         X       X         X     
    //   de(1111)    d(111)   s(001)            X         X       X         X     
    //   de(1111)    d(111)   e(111)            -         -       -         d(111)  Assumes same block sizes
    //            input                  state       stateCMPDir        -> snoop          ret                   miss           next
  theAccessTable[ kDirtyEvictAccess    | kInvalid  | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kDirtyEvictAccess    | kInvalid  | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kDirtyEvictAccess    | kInvalid  | kCMPDirExclusive ] =                                                      kDirty               ;

  theAccessTable[ kDirtyEvictAccess    | kValid    | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kDirtyEvictAccess    | kValid    | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kDirtyEvictAccess    | kValid    | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

  theAccessTable[ kDirtyEvictAccess    | kWritable | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kDirtyEvictAccess    | kWritable | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kDirtyEvictAccess    | kWritable | kCMPDirExclusive ] =                                                      kDirty               ;

  theAccessTable[ kDirtyEvictAccess    | kDirty    | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kDirtyEvictAccess    | kDirty    | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kDirtyEvictAccess    | kDirty    | kCMPDirExclusive ] =                                                      kDirty               ;

    //   input       state    stateCMPDir   ->  snoop     ret     miss      next
    //   we(1011)    i(000)   i(000)            X         X       X         X     
    //   we(1011)    i(000)   s(001)            X         X       X         X     
    //   we(1011)    i(000)   e(011)            -         -       -         w(011)

    //   we(1011)    v(001)   i(000)            X         X       X         X     
    //   we(1011)    v(001)   s(001)            X         X       X         X     
    //   we(1011)    v(001)   e(011)            X         X       X         X     

    //   we(1011)    w(011)   i(000)            X         X       X         X     
    //   we(1011)    w(011)   s(001)            X         X       X         X     
    //   we(1011)    w(011)   e(011)            -         -       -         w(011)

    //   we(1011)    d(111)   i(000)            X         X       X         X     
    //   we(1011)    d(111)   s(001)            X         X       X         X     
    //   we(1011)    d(111)   e(011)            X         X       X         X       Assumes same block sizes
    //            input                  state       stateCMPDir        -> snoop          ret                   miss           next
  theAccessTable[ kWritableEvictAccess | kInvalid  | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kWritableEvictAccess | kInvalid  | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kWritableEvictAccess | kInvalid  | kCMPDirExclusive ] =                                                      kWritable            ;

  theAccessTable[ kWritableEvictAccess | kValid    | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kWritableEvictAccess | kValid    | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kWritableEvictAccess | kValid    | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

  theAccessTable[ kWritableEvictAccess | kWritable | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kWritableEvictAccess | kWritable | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kWritableEvictAccess | kWritable | kCMPDirExclusive ] =                                                      kWritable            ;

  theAccessTable[ kWritableEvictAccess | kDirty    | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kWritableEvictAccess | kDirty    | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kWritableEvictAccess | kDirty    | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

    //   input       state    stateCMPDir   ->  snoop     ret     miss     next
    //   ce(1001)    i(000)   i(000)            X         X       X        X     
    //   ce(1001)    i(000)   s(001)            -         -       -        *(101)
    //   ce(1001)    i(000)   e(011)            X         X       X        X

    //   ce(1001)    v(001)   i(000)            X         X       X        X     
    //   ce(1001)    v(001)   s(001)            -         -       -        v(001)
    //   ce(1001)    v(001)   e(011)            X         X       X        X     

    //   ce(1001)    w(011)   i(000)            X         X       X        X     
    //   ce(1001)    w(011)   s(001)            -         -       -        w(011)
    //   ce(1001)    w(011)   e(011)            X         X       X        X

    //   ce(1001)    d(111)   i(000)            X         X       X        X     
    //   ce(1001)    d(111)   s(001)            -         -       -        d(111)
    //   ce(1001)    d(111)   e(011)            X         X       X        X

    // *'d next state field depends on the state of the CMP Dir for that line as follows
    // dir  -> next
    //  v       v
    //  w       w
    //  d       d
    //            input                  state       stateCMPDir        -> snoop          ret                   miss           next
  theAccessTable[ kCleanEvictAccess    | kInvalid  | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kCleanEvictAccess    | kInvalid  | kCMPDirShared    ] =                                                      kCMPDirDependantState;
  theAccessTable[ kCleanEvictAccess    | kInvalid  | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

  theAccessTable[ kCleanEvictAccess    | kValid    | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kCleanEvictAccess    | kValid    | kCMPDirShared    ] =                                                      kValid               ;
  theAccessTable[ kCleanEvictAccess    | kValid    | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

  theAccessTable[ kCleanEvictAccess    | kWritable | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kCleanEvictAccess    | kWritable | kCMPDirShared    ] =                                                      kWritable            ;
  theAccessTable[ kCleanEvictAccess    | kWritable | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

  theAccessTable[ kCleanEvictAccess    | kDirty    | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison      | kPoison              ;
  theAccessTable[ kCleanEvictAccess    | kDirty    | kCMPDirShared    ] =                                                      kDirty               ;
  theAccessTable[ kCleanEvictAccess    | kDirty    | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison      | kPoison              ;

    //   input       state    stateCMPDir   ->  snoop     ret     miss     next
    //   cena(1001)    i(000)   i(000)            X         X       X        X     
    //   cena(1001)    i(000)   s(001)            -         -       -        i(000)
    //   cena(1001)    i(000)   e(011)            X         X       X        X

    //   cena(1001)    v(001)   i(000)            X         X       X        X     
    //   cena(1001)    v(001)   s(001)            -         -       -        v(001)
    //   cena(1001)    v(001)   e(011)            X         X       X        X     

    //   cena(1001)    w(011)   i(000)            X         X       X        X     
    //   cena(1001)    w(011)   s(001)            -         -       -        w(011)
    //   cena(1001)    w(011)   e(011)            X         X       X        X

    //   cena(1001)    d(111)   i(000)            X         X       X        X     
    //   cena(1001)    d(111)   s(001)            -         -       -        d(111)
    //   cena(1001)    d(111)   e(011)            X         X       X        X

    //            input                            state       stateCMPDir        -> snoop          ret                   miss                           next
  theAccessTable[ kCleanEvictNonAllocatingAccess | kInvalid  | kCMPDirInvalid   ] =                                       kCleanEvictNonAllocatingMiss | kInvalid  ;
  theAccessTable[ kCleanEvictNonAllocatingAccess | kInvalid  | kCMPDirShared    ] =                                       kCleanEvictNonAllocatingMiss | kInvalid  ;
  theAccessTable[ kCleanEvictNonAllocatingAccess | kInvalid  | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison                      | kPoison   ;

  theAccessTable[ kCleanEvictNonAllocatingAccess | kValid    | kCMPDirInvalid   ] =                                                                      kValid    ;
  theAccessTable[ kCleanEvictNonAllocatingAccess | kValid    | kCMPDirShared    ] =                                                                      kValid    ;
  theAccessTable[ kCleanEvictNonAllocatingAccess | kValid    | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison                      | kPoison   ;

  theAccessTable[ kCleanEvictNonAllocatingAccess | kWritable | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison                      | kPoison   ;
  theAccessTable[ kCleanEvictNonAllocatingAccess | kWritable | kCMPDirShared    ] =                                                                      kWritable ;
  theAccessTable[ kCleanEvictNonAllocatingAccess | kWritable | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison                      | kPoison   ;

  theAccessTable[ kCleanEvictNonAllocatingAccess | kDirty    | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison                      | kPoison   ;
  theAccessTable[ kCleanEvictNonAllocatingAccess | kDirty    | kCMPDirShared    ] =                                                                      kDirty    ;
  theAccessTable[ kCleanEvictNonAllocatingAccess | kDirty    | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison                      | kPoison   ;

    //   input       state    stateCMPDir   ->  snoop     ret     miss     next
    //   ceaf(1001)    i(000)   i(000)            X         X       X        X     
    //   ceaf(1001)    i(000)   s(001)            -         -       -        *(100)
    //   ceaf(1001)    i(000)   e(011)            X         X       X        X

    //   ceaf(1001)    v(001)   i(000)            X         X       X        X     
    //   ceaf(1001)    v(001)   s(001)            -         -       -        v(001)
    //   ceaf(1001)    v(001)   e(011)            X         X       X        X     

    //   ceaf(1001)    w(011)   i(000)            X         X       X        X     
    //   ceaf(1001)    w(011)   s(001)            -         -       -        w(011)
    //   ceaf(1001)    w(011)   e(011)            X         X       X        X

    //   ceaf(1001)    d(111)   i(000)            X         X       X        X     
    //   ceaf(1001)    d(111)   s(001)            -         -       -        d(111)
    //   ceaf(1001)    d(111)   e(011)            X         X       X        X

    // *'d next state field is defined as follows
    // free_cache_frame  ->  next
    //      yes          ->   v
    //      no           ->   i

    //            input                            state       stateCMPDir        -> snoop          ret                   miss                           next
  theAccessTable[ kCleanEvictAllocOnFreeAccess | kInvalid  | kCMPDirInvalid   ] =                                       kCleanEvictNonAllocatingMiss | kAllocOnFreeFrameState ;
  theAccessTable[ kCleanEvictAllocOnFreeAccess | kInvalid  | kCMPDirShared    ] =                                       kCleanEvictNonAllocatingMiss | kAllocOnFreeFrameState ;
  theAccessTable[ kCleanEvictAllocOnFreeAccess | kInvalid  | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison                      | kPoison   ;

  theAccessTable[ kCleanEvictAllocOnFreeAccess | kValid    | kCMPDirInvalid   ] =                                                                      kValid    ;
  theAccessTable[ kCleanEvictAllocOnFreeAccess | kValid    | kCMPDirShared    ] =                                                                      kValid    ;
  theAccessTable[ kCleanEvictAllocOnFreeAccess | kValid    | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison                      | kPoison   ;

  theAccessTable[ kCleanEvictAllocOnFreeAccess | kWritable | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison                      | kPoison   ;
  theAccessTable[ kCleanEvictAllocOnFreeAccess | kWritable | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison                      | kPoison   ;
  theAccessTable[ kCleanEvictAllocOnFreeAccess | kWritable | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison                      | kPoison   ;

  theAccessTable[ kCleanEvictAllocOnFreeAccess | kDirty    | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison                      | kPoison   ;
  theAccessTable[ kCleanEvictAllocOnFreeAccess | kDirty    | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison                      | kPoison   ;
  theAccessTable[ kCleanEvictAllocOnFreeAccess | kDirty    | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison                      | kPoison   ;

    //   input       state    stateCMPDir   ->  snoop     ret     miss     next
    //   deaf(1001)    i(000)   i(000)            X         X       X        X     
    //   deaf(1001)    i(000)   s(001)            -         -       -        *(100)
    //   deaf(1001)    i(000)   e(011)            X         X       X        X

    //   deaf(1001)    v(001)   i(000)            X         X       X        X     
    //   deaf(1001)    v(001)   s(001)            -         -       -        v(001)
    //   deaf(1001)    v(001)   e(011)            X         X       X        X     

    //   deaf(1001)    w(011)   i(000)            X         X       X        X     
    //   deaf(1001)    w(011)   s(001)            -         -       -        w(011)
    //   deaf(1001)    w(011)   e(011)            X         X       X        X

    //   deaf(1001)    d(111)   i(000)            X         X       X        X     
    //   deaf(1001)    d(111)   s(001)            -         -       -        d(111)
    //   deaf(1001)    d(111)   e(011)            X         X       X        X

    // *'d next state field is defined as follows
    // free_cache_frame  ->  next
    //      yes          ->   v
    //      no           ->   i

    //            input                            state       stateCMPDir        -> snoop          ret                   miss                           next
  theAccessTable[ kDirtyEvictAllocOnFreeAccess | kInvalid  | kCMPDirInvalid   ] =                                                               kAllocOnFreeFrameState ;
  theAccessTable[ kDirtyEvictAllocOnFreeAccess | kInvalid  | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison                      | kPoison ;
  theAccessTable[ kDirtyEvictAllocOnFreeAccess | kInvalid  | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison                      | kPoison ;

  theAccessTable[ kDirtyEvictAllocOnFreeAccess | kValid    | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison                      | kPoison ;
  theAccessTable[ kDirtyEvictAllocOnFreeAccess | kValid    | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison                      | kPoison ;
  theAccessTable[ kDirtyEvictAllocOnFreeAccess | kValid    | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison                      | kPoison ;

  theAccessTable[ kDirtyEvictAllocOnFreeAccess | kWritable | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison                      | kPoison ;
  theAccessTable[ kDirtyEvictAllocOnFreeAccess | kWritable | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison                      | kPoison ;
  theAccessTable[ kDirtyEvictAllocOnFreeAccess | kWritable | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison                      | kPoison ;

  theAccessTable[ kDirtyEvictAllocOnFreeAccess | kDirty    | kCMPDirInvalid   ] =  kPoison      | kPoison             | kPoison                      | kPoison ;
  theAccessTable[ kDirtyEvictAllocOnFreeAccess | kDirty    | kCMPDirShared    ] =  kPoison      | kPoison             | kPoison                      | kPoison ;
  theAccessTable[ kDirtyEvictAllocOnFreeAccess | kDirty    | kCMPDirExclusive ] =  kPoison      | kPoison             | kPoison                      | kPoison ;

    // For software coherent ICaches

    //   input       state    stateCMPDir   ->  snoop     ret     miss     next
    //   ie(1010)    i(000)   i(000)            -         -       -        v(001)
    //   ie(1010)    i(000)   s(001)            -         -       -        i(000)   Discard (dcache more up-to-date)
    //   ie(1010)    i(000)   e(011)            -         -       -        i(000)   Discard (dcache more up-to-date)

    //   ie(1010)    v(001)   i(000)            -         -       -        v(001)
    //   ie(1010)    v(001)   s(001)            -         -       -        v(001)
    //   ie(1010)    v(001)   e(011)            X         X       X        X     

    //   ie(1010)    w(011)   i(000)            -         -       -        w(011)
    //   ie(1010)    w(011)   s(001)            -         -       -        w(011)
    //   ie(1010)    w(011)   e(011)            -         -       -        w(011)

    //   ie(1010)    d(111)   i(000)            -         -       -        d(111)
    //   ie(1010)    d(111)   s(001)            -         -       -        d(111)
    //   ie(1010)    d(111)   e(011)            -         -       -        d(111)
    //
    //            input                  state       stateCMPDir        -> snoop          ret                   miss           next




    // Actions to perform on CMP evictions (this is the current implementation):
    //   input       state    stateCMPDir   ->  snoop     ret     miss      next
    //   e(xxxx)     i(000)   i(000)            X         X       X         X     
    //   e(xxxx)     i(000)   s(001)            X         X       X         X
    //   e(xxxx)     i(000)   e(011)            X         X       X         X

    //   e(xxxx)     v(001)   i(000)            -         -       ce        i(000)
    //   e(xxxx)     v(001)   s(001)            -         -       drop      i(000)
    //   e(xxxx)     v(001)   e(011)            X         X       X         X     

    //   e(xxxx)     w(011)   i(000)            -         -       we        i(000)
    //   e(xxxx)     w(011)   s(001)            -         -       drop      i(000)  CMPDirDependantState will fix it
    //   e(xxxx)     w(011)   e(011)            -         -       drop      i(000)  Assumes same block sizes

    //   e(xxxx)     d(111)   i(000)            -         -       de        i(000)
    //   e(xxxx)     d(111)   s(001)            -         -       drop      i(000)  CMPDirDependantState will fix it
    //   e(xxxx)     d(111)   e(011)            -         -       drop      i(000)  Assumes same block sizes

    // Actions to perform on CMP evictions without relying on the CMPDir state:
    //   input       state    stateCMPDir   ->  snoop     ret     miss      next
    //   e(xxxx)     i(000)   i(000)            X         X       X         X     
    //   e(xxxx)     i(000)   s(001)            X         X       X         X
    //   e(xxxx)     i(000)   e(011)            X         X       X         X

    //   e(xxxx)     v(001)   i(000)            -         -       ce        i(000)
    //   e(xxxx)     v(001)   s(001)            -         -       drop      i(000)
    //   e(xxxx)     v(001)   e(011)            X         X       X         X     

    //   e(xxxx)     w(011)   i(000)            -         -       we        i(000)
    //   e(xxxx)     w(011)   s(001)            inv(000)  -       we        i(000)
    //   e(xxxx)     w(011)   e(011)            -         -       drop      i(000)  Assumes same block sizes

    //   e(xxxx)     d(111)   i(000)            -         -       de        i(000)
    //   e(xxxx)     d(111)   s(001)            inv(000)  -       de        i(000)
    //   e(xxxx)     d(111)   e(011)            -         -       drop      i(000)  Assumes same block sizes

};

struct AssociativeCache {

  unsigned long long * theArray;
  unsigned long theSetSize;
  unsigned int theAssociativity;
  evict_function_t evict;
  repl_hit_t repl_hit;
  repl_miss_t repl_miss;
  repl_victim_t repl_victim;
  repl_inval_t repl_inval;
  unsigned long theNumSets;
  unsigned int theBlockSize;
  unsigned int theBlockSizeLog2;
  unsigned int theBlockSizeTimesNumSets;

  // NIKOS: FIXME: This is here to support the old checkpoints (pre-NUCA)
  int theSetShift;
  int theTagShift;

  unsigned char * theArray_base;

  unsigned long long getSetBits( const unsigned long long tagset
                            , const unsigned int anIdxExtraOffsetBits
                          ) const
  {
    const unsigned long long theSetMask = (( theBlockSizeTimesNumSets << anIdxExtraOffsetBits ) - 1) & ~((theBlockSize << anIdxExtraOffsetBits) - 1);
    return (tagset & theSetMask);
  }

  unsigned long long getSet( const unsigned long long aSetBits
                        , const unsigned int anIdxExtraOffsetBits
                      ) const
  {
    const unsigned long long theSetShift = theBlockSizeLog2 + anIdxExtraOffsetBits;
    return (aSetBits >> theSetShift);
  }

  AssociativeCache(int              aBlockSize,
                   int              aNumSets,
                   int              anAssociativity,
                   evict_function_t anEvict,
                   repl_hit_t       aReplHit,
                   repl_miss_t      aReplMiss,
                   repl_victim_t    aReplVictim,
                   repl_inval_t     aReplInval )
  {
    theNumSets = aNumSets;
    long array_size = sizeof(unsigned long long) * theNumSets * anAssociativity;
    theArray_base =  new unsigned char [ array_size + 128];
    theArray = reinterpret_cast<unsigned long long *>
      ( (reinterpret_cast<unsigned long long>(theArray_base) + 127UL) & (~127UL)
      ); //128-byte align theArray

    theAssociativity = anAssociativity;
    evict = anEvict;
    repl_hit = aReplHit;
    repl_miss = aReplMiss;
    repl_victim = aReplVictim;
    repl_inval = aReplInval;

    //Calculate masks and shifts
    theBlockSize = aBlockSize;
    theBlockSizeLog2 = log_base2( aBlockSize );
    theBlockSizeTimesNumSets = aBlockSize * theNumSets;

    // NIKOS: FIXME: This is here to support the old checkpoints (pre-NUCA)
    theSetShift = log_base2( theBlockSize ) ;
    theTagShift = log_base2( theNumSets ) ;

    //make sure there is enough space in the block offset to hide the line state
    DBG_Assert( ((unsigned int) log_base2( aBlockSize )) >= log_base2(kState)+1 );

    //zero-fill the array
    std::memset( theArray, 0, array_size );

    DBG_Assert(sizeof(access_t) * 8 >= 16);
  }


  void apply_returned_state( unsigned long long * const frame, const unsigned long long returned_state) {
    *frame = (*frame & ~kState) | ( returned_state);
  }

  unsigned long long getState (const unsigned long long * const frame) const {
    return (*frame & kState);
  }

  //Return 0 for no miss
  unsigned long long * access( const unsigned long long tagset
                               , access_t &access_type
                               , const access_t CMPDirState
                               , const unsigned int anExtraOffsetBits
                             )
  {
    unsigned long long set_bits = getSetBits(tagset, anExtraOffsetBits);
    unsigned long long set = getSet(set_bits, anExtraOffsetBits);
    unsigned long long * set_base = theArray + set * theAssociativity;

    //Check for MRU hit
    unsigned long long mru_tag = *set_base;
    DBG_( Verb, Addr(tagset) ( << std::hex << "access_type: " << access_type << " CMPDir: " << CMPDirState << " tagset: " << tagset << " set_bits: " << set_bits << " set: " << set << " set_base: " << *set_base << " mru_tag: " << mru_tag << std::dec) );
    if (   /* tags match   */    (mru_tag & ~kState) == tagset) {
        //Tag match - compute state transition and further action
        access_t access = access_type | (mru_tag & kState) | CMPDirState;
        access_type = theAccessTable[access];
        mru_tag = (*set_base & ~kState) | (access_type & kState);

        //process hit
        repl_hit(set_base, 0, mru_tag);

        DBG_(Verb, Addr(tagset) ( << std::hex << "TAG MATCH - result: *set_base: " << *set_base << " access_type: " << access_type << std::dec ));
        return set_base;
    }

    //Non-MRU hits
    unsigned long long * set_iter = set_base;
    for (unsigned int i = 1; i < theAssociativity; ++i) {
      set_iter += 1;
      unsigned long long mru_tag = *set_iter;
      DBG_(VVerb, Addr(tagset) ( << "way: " << i << std::hex << " tag: " << mru_tag << std::dec ));
      if (  /* tags match   */  (mru_tag & ~kState ) == tagset) {
          //Compute state transition and further actions
          access_t access = access_type | (mru_tag & kState) | CMPDirState;
          access_type = theAccessTable[access];
          mru_tag = (mru_tag & ~kState) | (access_type & kState);

          //process hit
          repl_hit(set_base, i, mru_tag);

          DBG_(Verb, Addr(tagset) ( << std::hex << "TAG MATCH - result: *set_base: " << *set_base << " access_type: " << access_type << std::dec ));
          return set_base;
      }
    }

    //Miss Handler
    //Compute further actions
    access_t access = access_type | kInvalid | CMPDirState;
    DBG_( Verb, Addr(tagset) ( << std::hex  << "theAccess[ " << access_type << " | " << kInvalid << " | " << CMPDirState << " ] = " << theAccessTable[access] << std::dec ) );
    access_type = theAccessTable[access];

    // thePrivateWithASR || DirtyRingWritebacks /* CMU-ONLY */
    if ((access_type & kState) == kAllocOnFreeFrameState) {
      // allocate only on free frame
      int alloc_way = repl_victim(set_base);
      unsigned long long * frame_to_alloc = set_base + alloc_way;
      access_t frame_state = (*frame_to_alloc & kState);
      DBG_Assert (frame_state == kInvalid);

      //process miss and compute state transition
      //repl_miss(set_base, alloc_way, tagset | kValid);
      // allocate in place. Do NOT move the victim up the LRU chain...
      *frame_to_alloc = (tagset | kValid);
      set_base = frame_to_alloc;

      DBG_(Verb, Addr(tagset) ( << std::hex << "ALLOC ON FREE after update *set_base: " << *set_base << " access_type: " << access_type << std::dec ));
      access_type = ( (access_type & ~kState) | kInvalid ); // if the block is not in the cache and there is no space, let the cache controller know
    }

    else if ((access_type & kState) == kInvalid) {
      //Do not allocate the line. Bye bye now...
      DBG_(Verb, Addr(tagset) ( << std::hex << "MISS NO ALLOC *set_base: " << *set_base << " access_type: " << access_type << std::dec ));
    }

    else {

      //Handle WB/Evict
      int evict_way = repl_victim(set_base);
      unsigned long long * evict_tag = set_base + evict_way;
      access_t evict_state = (*evict_tag & kState);
      if (evict_state != kInvalid) {
        *evict_tag = (*evict_tag & ~kState); // make sure line is invalidated
        evict( *evict_tag , evict_state );
      }

      //process miss and compute state transition
      repl_miss(set_base, evict_way, tagset | (access_type & kState));

      DBG_(Verb, Addr(tagset) ( << std::hex << "MISS after update *set_base: " << *set_base << " access_type: " << access_type << std::dec ));
    }
    return set_base;
  }

  /* CMU-ONLY-BLOCK-BEGIN */
  std::pair<bool,bool>  purge( const unsigned long long tagset
                               , const unsigned int anExtraOffsetBits
                             )
  {
    unsigned long long set_bits = getSetBits(tagset, anExtraOffsetBits);
    unsigned long long set = getSet(set_bits, anExtraOffsetBits);
    unsigned long long * set_base = theArray + set * theAssociativity;

    DBG_(Verb, Addr(tagset) ( << std::hex << "Purge tagset: " << tagset << " set_bits: " << set_bits << " set: " << set << " set_base: " << set_base << std::dec) );
    unsigned long long * set_iter = set_base;
    bool ret_valid_flag;
    bool ret_dirty_flag;

    //Find matching way
    for (unsigned int i = 1; i <= theAssociativity; ++i) {
      DBG_(VVerb, Addr(tagset) ( << "way: " << i-1 << std::hex << " tag: " << *set_iter << std::dec ));
      if (  /* tags match   */  (*set_iter & ~kState ) == tagset) {
          DBG_(VVerb, Addr(tagset) ( << std::hex << "Purge matched: " << *set_iter << std::dec ));
          //Get exclusive/dirty state
          ret_valid_flag = ( (*set_iter & kState) != kInvalid ); // is line valid
          ret_dirty_flag = ( (*set_iter & kDirtyBitMask) != 0UL ); // is line writable or dirty

          //Handle WB/Evict
          access_t evict_state = (*set_iter & kState);
          if (evict_state != kInvalid) {
            *set_iter = (*set_iter & ~kState); // make sure line is invalidated
            evict( *set_iter , evict_state );
          }

          return std::make_pair(ret_valid_flag, ret_dirty_flag);
      }
      set_iter += 1;
    }
    return std::make_pair(false, false);
  }
  /* CMU-ONLY-BLOCK-END */

  bool invalidate( const unsigned long long tagset
                   , const unsigned int anExtraOffsetBits
                 )
  {
    unsigned long long set_bits = getSetBits(tagset, anExtraOffsetBits);
    unsigned long long set = getSet(set_bits, anExtraOffsetBits);
    unsigned long long * set_base = theArray + set * theAssociativity;

    DBG_(Verb, Addr(tagset) ( << std::hex << "Invalidate tagset: " << tagset << " set_bits: " << set_bits << " set: " << set << " set_base: " << set_base << std::dec) );
    unsigned long long * set_iter = set_base;
    bool ret_dirty_flag;

    //Find matching way
    for (unsigned int i = 1; i <= theAssociativity; ++i) {
      DBG_(VVerb, Addr(tagset) ( << "way: " << i-1 << std::hex << " tag: " << *set_iter << std::dec ));
      if (  /* tags match   */  (*set_iter & ~kState ) == tagset) {
          DBG_(VVerb, Addr(tagset) ( << std::hex << "Invalidate matched: " << *set_iter << std::dec ));
          ret_dirty_flag = ((*set_iter & kDirtyBitMask) != 0UL); // is line writable or dirty

          //perform replacement update upon invalidation
          repl_inval(set_base, i, tagset);

          return ret_dirty_flag;
      }
      set_iter += 1;
    }
    return false;
  }

  bool returnReq( const unsigned long long tagset
                  , const unsigned int anExtraOffsetBits
                )
  {
    unsigned long long set_bits = getSetBits(tagset, anExtraOffsetBits);
    unsigned long long set = getSet(set_bits, anExtraOffsetBits);
    unsigned long long * set_base = theArray + set * theAssociativity;

    DBG_(Verb, Addr(tagset) ( << std::hex << "ReturnReq tagset: " << tagset << " set_bits: " << set_bits << " set: " << set << " set_base: " << set_base << std::dec) );
    unsigned long long * set_iter = set_base;

    //Find matching way
    for (unsigned int i = 1; i <= theAssociativity; ++i) {
      DBG_(VVerb, Addr(tagset) ( << "way: " << i-1 << std::hex << " tag: " << *set_iter << std::dec ));
      if (  /* tags match   */  (*set_iter & ~kState ) == tagset) {
          DBG_(VVerb, Addr(tagset) ( << std::hex << "ReturnReq matched: " << *set_iter << std::dec ));
          return ( (*set_iter & kState) != kInvalid );  // return true if line is valid
      }
      set_iter += 1;
    }
    return false;
  }

  bool downgrade( const unsigned long long tagset
                  , const unsigned int anExtraOffsetBits
                )
  {
    unsigned long long set_bits = getSetBits(tagset, anExtraOffsetBits);
    unsigned long long set = getSet(set_bits, anExtraOffsetBits);
    unsigned long long * set_iter = theArray + set * theAssociativity;
    bool ret_dirty_flag;

    DBG_(Verb, Addr(tagset) ( << std::hex << "Downgrade tagset: " << tagset << " set_bits: " << set_bits << " set: " << set << std::dec) );

    //Find matching way
    for (unsigned int i = 0; i < theAssociativity; ++i) {
      DBG_(VVerb, Addr(tagset) ( << "way: " << i << std::hex << " tag: " << *set_iter << std::dec ));
      if (  /* tags match   */  (*set_iter & ~kState ) == tagset) {
          DBG_(VVerb, Addr(tagset) ( << std::hex << "Downgrade matched: " << *set_iter << std::dec ));
          ret_dirty_flag = ((*set_iter & kDirtyBitMask) != 0UL); // is line writable or dirty
          //Remove dirty/writable state
          *set_iter = (*set_iter & ~kState) | kValid;
          return ret_dirty_flag;
      }
      set_iter += 1;
    }

    return false;
  }

  MemoryMessage::MemoryMessageType probe( const unsigned long long tagset
                                          , const unsigned int anExtraOffsetBits
                                        )
  {
    unsigned long long set_bits = getSetBits(tagset, anExtraOffsetBits);
    unsigned long long set = getSet(set_bits, anExtraOffsetBits);
    unsigned long long * set_base = theArray + set * theAssociativity;

    DBG_(Verb, Addr(tagset) ( << std::hex << "Probe tagset: " << tagset << " set_bits: " << set_bits << " set: " << set << " set_base: " << set_base << std::dec) );
    unsigned long long * set_iter = set_base;

    //Find matching way
    bool existsFreeCacheFrame = false;
    for (unsigned int i = 1; i <= theAssociativity; ++i) {
      DBG_(VVerb, Addr(tagset) ( << "way: " << i-1 << std::hex << " tag: " << *set_iter << std::dec ));
      if (  /* tags match   */  (*set_iter & ~kState ) == tagset) {
          DBG_(VVerb, Addr(tagset) ( << std::hex << "Probe matched: " << *set_iter << std::dec ));
          if ( (*set_iter & kState) == kInvalid) {
            existsFreeCacheFrame = true;
            break;
          }
          else if ( (*set_iter & kState) == kValid) {
            return MemoryMessage::ProbedClean;
          }
          else if ( (*set_iter & kState) == kDirty) {
            return MemoryMessage::ProbedDirty;
          }
          else {
            DBG_Assert ( (*set_iter & kState) == kWritable, ( << "Addr: 0x" << std::hex << tagset << " state: " << *set_iter << std::dec ));
            return MemoryMessage::ProbedWritable;
          }
          break;
      }
      set_iter += 1;
    }
    if (existsFreeCacheFrame) {
      return MemoryMessage::ProbedNotPresentSetNotFull;
    }
    else {
      return MemoryMessage::ProbedNotPresent;
    }
  }

  /////////////////////////////////////////////////////////////////////////////////////
  // Checkpoint functions

  void saveState( std::ostream & s ) {
    static const int kSave_ValidBit = 1;
    static const int kSave_DirtyBit = 2;
    static const int kSave_ModifiableBit = 4;

    // NIKOS: FIXME to support both checkpoint versions. NUCA needs full tags.
    // int shift = theSetShift + log_base2( theNumSets ) ;
    int shift = theBlockSizeLog2;

    unsigned long long * block_iter = theArray;
    for ( unsigned int i = 0; i < theNumSets ; i++ ) {
      s << "{";
      for ( unsigned int j = 0; j < theAssociativity; j++ ) {
        unsigned long long state = *block_iter;
        unsigned long long tag = (*block_iter) >> shift;
        int save_state
          = (   (state & kState) == kDirty ?    (kSave_ValidBit | kSave_ModifiableBit | kSave_DirtyBit)
            : ( (state & kState) == kWritable ? (kSave_ValidBit | kSave_ModifiableBit)
            : ( (state & kState) == kValid ?     kSave_ValidBit
            : 0 )))
          ;
        s << "[ " << save_state
          << " "  << static_cast<unsigned long long>(tag)
          << " ]";
        ++block_iter;
      }
      s << "} < ";
      for ( unsigned int j = 0; j < theAssociativity; j++ ) {
        s << j << " ";
      }
      s << "> " << std::endl;
    }
  }

  bool loadState( std::istream & s ) {
    static const int kSave_ValidBit = 1;
    static const int kSave_DirtyBit = 2;
    static const int kSave_ModifiableBit = 4;

    unsigned long long * block_iter = theArray;
    char paren;
    int dummy;
    int load_state, state;
    unsigned long long load_tag;
    for ( unsigned int i = 0; i < theNumSets ; i++ ) {
      s >> paren; // {
      if ( paren != '{' ) {
        DBG_ (Crit, (<< "Expected '{' when loading checkpoint" ) );
        return false;
      }  
      for ( unsigned int j = 0; j < theAssociativity; j++ ) {
        s >> paren >> load_state >> load_tag >> paren;
        state = (   (load_state & kSave_DirtyBit) ?      kDirty
                : ( (load_state & kSave_ModifiableBit) ? kWritable
                : ( (load_state & kSave_ValidBit) ?      kValid
                : 0)));

        // NIKOS: FIXME to support both checkpoint versions. NUCA needs full tags.
        // *block_iter = (load_tag << (theTagShift+theSetShift)) | (i << theSetShift) | state;
        *block_iter = (load_tag << theBlockSizeLog2) | state;

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
      for ( unsigned int j = 0; j < theAssociativity; j++ ) {
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


}  // namespace nFastCMPCache

#endif /* FLEXUS_FASTCMPCACHE_ASSOCIATIVECACHE_HPP_INCLUDED */
