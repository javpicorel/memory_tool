// DO-NOT-REMOVE begin-copyright-block
//
// Redistributions of any form whatsoever must retain and/or include the
// following acknowledgment, notices and disclaimer:
//
// This product includes software developed by Carnegie Mellon University.
//
// Copyright 2006 by Eric Chung, Brian Gold, Nikos Hardavellas, Jangwoo Kim,
// Ippokratis Pandis, Minglong Shao, Jared Smolens, Stephen Somogyi,
// Tom Wenisch, Anastassia Ailamaki, Babak Falsafi and James C. Hoe for
// the SimFlex Project, Computer Architecture Lab at Carnegie Mellon,
// Carnegie Mellon University.
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

#ifndef COMMON_INCLUDED
#define COMMON_INCLUDED

#include <map>
#include <list>
#include <vector>
#include <iostream>

#include <boost/function.hpp>
#include <boost/utility.hpp>

#include <core/debug/debug.hpp>
#include <core/stats.hpp>

#define TARGET_PLATFORM v9
#include <core/target.hpp>
#include <core/types.hpp>


#include <core/boost_extensions/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include <components/Common/Slices/FillType.hpp>
#include <components/Common/Slices/FillLevel.hpp>
using namespace Flexus::SharedTypes;

#include <components/Common/seq_map.hpp>

//#define ENABLE_UNIQUE_COUNTS
//#define ENABLE_SAVE_SETS
#define ENABLE_ROUND_TO_BLOCK
#define ENABLE_RULE_REMAP
//#define ENABLE_SEQUITUR_PC_EXPERIMENT
#define ENABLE_MOST_RECENT_EXPERIMENT
#define ENABLE_DIGRAM_EXPERIMENT
#define ENABLE_STRIDE_EXPERIMENT
//#define ENABLE_GRAMMAR_DUMP
//#define ENABLE_CORRELATION_EXPERIMENT
//#define ENABLE_STACK_TRACE_OUTPUT
//#define ENABLE_WHITE_BOX
#define ENABLE_RULE_REUSE_DISTANCE
#define ENABLE_RULE_REUSE_TLDISTANCE
#define ENABLE_FILL_LEVEL
//#define ENABLE_HIT_TRACKING
//#define ENABLE_CUCKOO
#define ENABLE_CUCKOO_NUKE_VICTIM
//#define ENABLE_CUCKOO_SEARCH
#define ENABLE_CUCKOO_NOMOVE
#define ENABLE_CUCKOO_ENFORCE
//#define ENABLE_CUCKOO_ONEHASH
#define ENABLE_CUCKOO_BIASED_VICTIM
#define ENABLE_LONG_SPATIAL_HISTORY
//#define ENABLE_CMOB_PC_LOOKUP

#define OLD_STAT( x )

#ifdef ENABLE_CUCKOO_ONEHASH
  #ifndef ENABLE_CUCKOO_NOMOVE
    #error "ONEHASH requires NOMOVE"
  #endif
  #ifdef ENABLE_CUCKOO_SEARCH
    #error "ONEHASH prohibits SEARCH"
  #endif
#endif


#include <boost/preprocessor/expand.hpp>
#define FDT_HELPER , 0
#define FDT_NOT_DEFINED_FDT_HELPER 1 , 1
#define FLEXUS_DEFINED_TEST(x) ( FDT_NOT_DEFINED_ ## x )
#define FLEXUS_GET_SECOND(x,y) y
#define FLEXUS_IS_EMPTY( x ) BOOST_PP_EXPAND( FLEXUS_GET_SECOND FLEXUS_DEFINED_TEST(x FDT_HELPER) )

#include <boost/preprocessor/control/iif.hpp>
#define DECLARE_STATIC_PARAM( x )   StatAnnotation * Cfg ## x ;

#define ANNOTATE_STATIC_PARAM( x )                                 \
  Cfg ## x = new StatAnnotation( "Configuration." # x );           \
  char * Cfg ## x ## str =                                         \
  BOOST_PP_IIF                                                     \
    ( FLEXUS_IS_EMPTY( x )                                         \
    , "true"                                                       \
    , "false"                                                      \
    ) ;                                                            \
  * Cfg ## x = Cfg ## x ## str;                                    \
  DBG_(Dev, ( << "Cfg: " # x ": " << Cfg ## x ## str ) );             /**/

template <int w, typename Source>
std::string fill(Source s) {
  std::ostringstream ss;
  ss << std::setfill('0') << std::setw(w) << boost::lexical_cast<std::string>(s);
  return ss.str();
}

using namespace Flexus::Stat;

typedef unsigned long long tTime;
typedef unsigned long long tVAddress;
typedef unsigned long tAddress;
typedef int tID;


static const int kBITS_OS = 1;
static const int kBITS_TSE_EOS = 2;
static const int kBITS_SGP_EOS = 4;
static const int kBITS_WRITE = 8;
static const int kBITS_ATOMIC = 16;
static const int kBITS_EVICT = 32;
static const int kBITS_INVAL = 64;
static const int kBITS_MISS = 128;

enum eSGPOutcome
  { eSGP_Hit
  , eSGP_Head
  , eSGP_Singular
  , eSGP_Miss
  };

inline std::ostream & operator << (std::ostream & anOstream, eSGPOutcome anOutcome) {
  static char * txt[] = { "SGP_Hit", "SGP_Head", "SGP_Singular", "SGP_Miss" };
  anOstream << txt[anOutcome];
  return anOstream;
}
enum eTSEOutcome
  { eTSE_Hit
  , eTSE_Watch
  , eTSE_Miss
  , eTSE_Unavoidable
  , eTSE_HitSpatial
  , eTSE_HitLateSpatial
  };
inline std::ostream & operator << (std::ostream & anOstream, eTSEOutcome anOutcome) {
  static char * txt[] = { "TSE_Hit", "TSE_Watch", "TSE_Miss", "TSE_Unavoidable", "TSE_HitSpatial", "TSE_HitLateSpatial" };
  anOstream << txt[anOutcome];
  return anOstream;
}


struct MissRecord {
  tTime aCycle;
  int aNode;
  tAddress anAddress;
  tVAddress aPC;
  tFillType aFillType;
#ifdef ENABLE_FILL_LEVEL
  tFillLevel aFillLevel;
#else //ENABLE_FILL_LEVEL
  static const tFillLevel aFillLevel = eLocalMem;
#endif //ENABLE_FILL_LEVEL
  eTSEOutcome aTSE;
  unsigned long aTSEStream;
  eSGPOutcome anSGP;
  unsigned long anSGPStream;
  char aBits;
  int aCurrentTrap;
  tVAddress aThread;
  tVAddress aStackTrace[16];
};
std::ostream & operator << (std::ostream & str, MissRecord const & a);


struct FixedRecord {
  tTime aCycle;
  int aNode;
  tAddress anAddress;
  tVAddress aPC;
  tFillType aFillType;
  tFillLevel aFillLevel;
  eTSEOutcome aTSE;
  unsigned long aTSEStream;
  eSGPOutcome anSGP;
  unsigned long anSGPStream;
  char aBits;
  int aCurrentTrap;
  tVAddress aThread;
  tVAddress aStackTrace[16];
  
  FixedRecord(MissRecord aRecord) {
    aCycle = aRecord.aCycle; 
    aNode = aRecord.aNode; 
    anAddress = aRecord.anAddress;
    aPC = aRecord.aPC;
    aFillType= aRecord.aFillType;
    aFillLevel= aRecord.aFillLevel;
    aTSE = aRecord.aTSE;
    aTSEStream = aRecord.aTSEStream;
    anSGP = aRecord.anSGP;
    anSGPStream = aRecord.anSGPStream;
    aBits = aRecord.aBits;
    aCurrentTrap = aRecord.aCurrentTrap;
    aThread = aRecord.aThread;
    for (int i = 0; i < 15; ++i) {
      aStackTrace[i] = aRecord.aStackTrace[i];
    }
  }
};


struct TraceCoordinator {
  virtual ~TraceCoordinator() {};
  virtual void event( MissRecord  & aRecord) = 0;
  virtual void go() = 0;
  virtual void finalize() = 0;
	virtual int numNodes() = 0;
  virtual void saveResults(bool) = 0;
};



struct TraceProcessor {
  virtual ~TraceProcessor() {};
  virtual void finalize() = 0;
  virtual void event( MissRecord & ) = 0;
};



TraceCoordinator * initialize_OrderHW(std::string options);

struct Generation {
  unsigned long theID;
  int theNode;
  tAddress theHeadAddress;
  tVAddress theHeadPC;
  unsigned long theBitVector;
  Generation()
    : theID( 0 )
    , theNode( 0 )
    , theHeadAddress( 0 )
    , theHeadPC( 0 )
    , theBitVector(0)
  { }
  Generation(unsigned long anID, int aNode, tAddress aHeadAddress, tVAddress aHeadPC, unsigned long aBitVector)
    : theID( anID )
    , theNode( aNode )
    , theHeadAddress( aHeadAddress )
    , theHeadPC( aHeadPC )
    , theBitVector(aBitVector)
  { }
};

struct Sort2nd {
  template <class X, class Y>
  bool operator ()( std::pair< X, Y > const & l, std::pair< X, Y > const & r) {
    if (l.second == r.second)
      return l.first > r.first;
    return l.second > r.second;
  }
};



struct stream {
  int theId;
  std::vector< tAddress > theString;
  stream(int anId)
   : theId(anId) {}
  bool operator < (stream const & aRHS) const {
    return theString <aRHS.theString;
  }
};

enum eStreamType
 { eNonStream
 , eZOne
 , eZMatch
 , eZMismatch
 , eZPastHit
 , eNZHot1
 , eNZHot2
 , eNZOne
 , eNZMatch
 , eNZPastHit
 , eNZMatch_to_Hot1
 , eNZMatch_to_Hot2
 , eNZMatch_to_PastHit
 };

enum eForwardType
 { eFwdMatch
 , eFwdHot
 , eFwdOne
 , eFwdPastHit
 , eFwdSpatial
 , eFwdLateSpatial
 , eFwdWatch_Hot_1
 , eFwdWatch_Hot_2
 , eFwdWatch_One
 , eFwdWatch_PastHit
 , eFwdTrigger
 };

enum eGenStatus
	{ tGenInactive
	, tGenBadBlock
	, tGenGoodBlockBadTag
	, tGenCorrectTrigger
	};
inline std::ostream & operator << (std::ostream & anOstream, eGenStatus aStatus) {
  static char * txt[] = { "Inactive", "BadBlock", "GoodBlock_BadTag", "CorrectTrigger" };
  anOstream << txt[aStatus];
  return anOstream;
}

struct OrderMgr;
struct MultiCache;
struct SpatialPredictor;

struct Stream {
  virtual ~Stream() {};
  virtual int id() const = 0;
  virtual bool spatial() const = 0;

  virtual void setMRC(OrderMgr * order, long long sequenceNo) = 0;
  virtual void set2MRC(OrderMgr * order, long long sequenceNo) = 0;
  virtual void clearMRC() = 0;
  virtual bool beginForward() = 0;

  virtual void notifyInvalidate(tAddress anAddress, eForwardType aFwdType) = 0;
  virtual void notifyReplace(tAddress anAddress, eForwardType aFwdType) = 0;
  virtual void notifyUpgrade(tAddress anAddress, eForwardType aFwdType) = 0;
  virtual void notifyHit(tAddress anAddress, eForwardType aFwdType) = 0;
  virtual void notifyHit(tAddress anAddress, eForwardType aFwdType, eSGPOutcome anSGPtype) { DBG_Assert(false); }
  virtual void notifyConvert(tAddress anAddress, eForwardType aFwdType) { }

  virtual void cascadedForwards(long aNumForwards) { DBG_Assert(false); }
  virtual eSGPOutcome getPriorSGPOutcome(tAddress anAddress) { DBG_Assert(false); return eSGP_Miss; }
	virtual void setTargetSpatialGens(long aTargetSpatialGens) { DBG_Assert(false); }
  virtual bool insertSpatialPrediction(tAddress anAddress, long aSequenceDelta, bool anInitial, tFillLevel aPrevFill) { DBG_Assert(false); return false; }
  virtual bool lateSpatialPrediction(tAddress anAddress, long aSequenceDelta) { DBG_Assert(false); return false; }
	virtual void setHeadSpatialType(eSGPOutcome anSGPtype) { }
	virtual void setSpatial(SpatialPredictor * aPred) { }
	virtual void genStatus(unsigned long anIndex, eGenStatus aStatus) { }
};

struct PrefetchReceiver {
  virtual ~PrefetchReceiver() {};
  virtual void finalize() = 0;
  virtual std::string const & name() const = 0;
  virtual int id() const = 0;
  virtual bool forward(tAddress anAddress, tVAddress aPC, boost::shared_ptr<Stream> aStream, eForwardType aFwdType) = 0;
};

typedef boost::function< boost::shared_ptr<Stream> ( PrefetchReceiver *, tAddress, tAddress ) > StreamRequestFn;
typedef boost::function< int ( PrefetchReceiver *, tAddress, tAddress ) > SpatialRequestFn;
typedef boost::function< int ( boost::shared_ptr<Stream>, tAddress, unsigned long ) > SpatialReconstructFn;
typedef boost::function< bool ( int, unsigned long, bool ) > CacheForwardFn;
typedef boost::function< bool ( int, unsigned long ) > CacheProbeFn;

//struct by_LRU {};
struct by_location {};
struct by_sequence_no{};
struct by_address {};
struct by_address_nopc {};
struct by_seq {};
struct by_stream {};
struct by_replacement {};
struct by_delta {};

static const int kHitBit = 0x10000;

struct OrderEntry {
  tAddress theAddress;
  long long theSequenceNo;
  tVAddress thePC;
  unsigned long theHitAndFill;
  mutable eSGPOutcome theSGP;
  mutable unsigned long long theSpatialSequenceNo;
  
  OrderEntry( tAddress anAddress, long long aSequenceNo, unsigned long long aPC, bool aWasHit, tFillLevel aFillLevel )
    : theAddress(anAddress)
    , theSequenceNo(aSequenceNo)
    , thePC(aPC)
		, theHitAndFill(aFillLevel)
    , theSGP(eSGP_Miss)
    , theSpatialSequenceNo(0)
	{
		if(aWasHit) theHitAndFill |= kHitBit;
	}

  ~OrderEntry( ) {  }


  tVAddress pc() const {
    return thePC;
  }

  tAddress address() const {
    return theAddress;
  }

  long long sequenceNo() const {
    return theSequenceNo;
  }

  bool wasHit() const {
    return (theHitAndFill & kHitBit);
  }

	tFillLevel fillLevel() const {
		return (tFillLevel)(theHitAndFill & (kHitBit - 1));
	}

  friend std::ostream & operator << ( std::ostream & anOstream, OrderEntry const & anOrderEntry);
} __attribute__((packed));


static const int kIndexAddr = 0;
static const int kIndexPC = 1;
static const int kIndexPCaddr = 2;
static const int kIndexPCoff = 3;
static const int kIndexPCrot = 4;
static const int kIndexPCrotWD = 5;

static const int kSpHistLast = 0;
static const int kSpHist2bit = 1;
static const int kSpHistInter = 2;
static const int kSpHistUnion = 3;


typedef boost::function< void ( int, unsigned long, std::pair<bool,bool> ) > SimReplaceFn;
static const int kCacheBits_Write = 1;
static const int kCacheBits_Prefetch = 2;

extern tAddress theInvestRegion;
extern tTime theGlobalTime;
extern bool theTraceNode14;


#endif //COMMON_INCLUDED
