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


#define ENABLE_PC_TRACKING
#define ENABLE_PC_LOOKUP
#define ENABLE_BO_LOOKUP
#define ENABLE_2MRC
#define SPATIAL_GROUP_SIZE 1024UL
#define SPATIAL_GROUP_MASK ~(SPATIAL_GROUP_SIZE - 1UL)
#define ENABLE_TRACE
#define ENABLE_TRACE_OUTPUT
#define ENABLE_TRACE_SGPGENERATIONS
#define ENABLE_WHITE_BOX


#ifdef ENABLE_OLD_STATS
  #define OLD_STAT( x ) x
#else
  #define OLD_STAT( x )
#endif

#ifdef ENABLE_COMBO
#ifndef ENABLE_ENTRY_STATE
#error "ENABLE_COMBO requires ENABLE_ENTRY_STATE"
#endif
#endif

#ifdef ENABLE_PC_TRACKING
#ifdef ENABLE_ENTRY_STATE
#error "ENABLE_PC_TRACKING does not support ENABLE_ENTRY_STATE"
#endif
#endif

#ifdef ENABLE_PC_LOOKUP
#ifndef ENABLE_PC_TRACKING
#error "ENABLE_PC_LOOKUP requires ENABLE_PC_TRACKING"
#endif
#endif

#ifdef ENABLE_SPATIAL_GROUPS
  extern unsigned long theGlobalBlockSize;
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

#include <core/stats.hpp>

using namespace Flexus::Stat;

typedef unsigned int tMRPVector;
typedef unsigned long long tTime;
typedef unsigned long tAddress;
typedef unsigned long long tVAddress;
typedef int tID;

static const int kBITS_OS = 1;
static const int kBITS_TSE_EOS = 2;
static const int kBITS_SGP_EOS = 4;

enum tEventType {
  eUpgrade,
  eProduction,
  eConsumption,
  eAccess,
  eEviction
};

enum eStreamType
 { eNonStream
 , eZOne
 , eZMatch
 , eZMismatch
 , eNZHot1
 , eNZHot2
 , eNZOne
 , eNZMatch
 , eNZMatch_to_Hot1
 , eNZMatch_to_Hot2
 };

enum eForwardType
 { eFwdMatch
 , eFwdHot
 , eFwdOne
 , eFwdWatch_Hot_1
 , eFwdWatch_Hot_2
 , eFwdWatch_One
 };

using Flexus::SharedTypes::tFillType;
using Flexus::SharedTypes::eCold;
using Flexus::SharedTypes::eReplacement;
using Flexus::SharedTypes::eCoherence;
using Flexus::SharedTypes::eDMA;
using Flexus::SharedTypes::ePrefetch;
using Flexus::SharedTypes::eFetch;
using Flexus::SharedTypes::tFillLevel;
using Flexus::SharedTypes::eLocalMem;

enum eSGPOutcome
  { eSGP_Hit
  , eSGP_Head
  , eSGP_Sparse
  , eSGP_Miss
  };

inline std::ostream & operator << (std::ostream & anOstream, eSGPOutcome anOutcome) {
  static char * txt[] = { "SGP_Hit", "SGP_Head", "SGP_Sparse", "SGP_Miss" };
  anOstream << txt[anOutcome];
  return anOstream;
}
enum eTSEOutcome
  { eTSE_Hit
  , eTSE_Watch
  , eTSE_Miss
  , eTSE_Unavoidable
  };
inline std::ostream & operator << (std::ostream & anOstream, eTSEOutcome anOutcome) {
  static char * txt[] = { "TSE_Hit", "TSE_Watch", "TSE_Miss", "TSE_Unavoidable" };
  anOstream << txt[anOutcome];
  return anOstream;
}

struct TraceCoordinator {
  virtual ~TraceCoordinator() {}
  virtual void event( tEventType anEventType, tID aNode, tAddress anAddress, tMRPVector aBitVector, tTime aTimestamp, tVAddress aPC, bool anOS ) = 0;
  virtual void access( tID aNode, tAddress anAddress, tVAddress aPC, bool anOS ) = 0;
  virtual void eviction( tID aNode, tAddress anAddress) = 0;
  virtual void consumption( tID aNode, tAddress anAddress, tVAddress aPC, tFillType aFillType, tFillLevel aFillLevel, bool anOS) = 0;
  virtual void upgrade( tID aNode, tAddress anAddress, tVAddress aPC, bool anOS) = 0;
  virtual void finalize() = 0;
};

struct TraceData {
  TraceData( tEventType anEventType, tID aNode, tAddress anAddress, tVAddress aPC, tFillType aFillType, tFillLevel aFillLevel,  bool isOS )
    : theEventType(anEventType)
    , theNode(aNode)
    , theAddress(anAddress)
    , thePC(aPC)
    , theFillType(aFillType)
    , theFillLevel(aFillLevel)
    , theOS(isOS)
    {}
  TraceData( tEventType anEventType, tID aNode, tID aProducerNode, tID aDirectoryNode, tAddress anAddress, long anInstance, tMRPVector aBitVector, tTime aTimestamp, tVAddress aPC, bool isOS )
    : theEventType(anEventType)
    , theNode(aNode)
    , theProducerNode(aProducerNode)
    , theDirectoryNode(aDirectoryNode)
    , theAddress(anAddress)
    , theInstance(anInstance)
#ifdef ENABLE_ENTRY_STATE
    , theBitVector(aBitVector)
#endif //ENABLE_ENTRY_STATE
    , theTimestamp(aTimestamp)
    , thePC(aPC)
#ifdef ENABLE_ENTRY_STATE
    , theWasConsumed(false)
    , theWasHit(false)
#endif //ENABLE_ENTRY_STATE
#ifdef ENABLE_COMBO
    , theHitId(0)
    , theHitSeqNo(0)
#endif //ENABLE_COMBO
    , theOS(isOS)
    {}
  tEventType theEventType;
  tID theNode;
  tID theProducerNode;
  tID theDirectoryNode;
  tAddress theAddress;
  long theInstance;
#ifdef ENABLE_ENTRY_STATE
  tMRPVector theBitVector;
#endif //ENABLE_ENTRY_STATE
  tTime theTimestamp;
  tVAddress thePC;
  tFillType theFillType;
  tFillLevel theFillLevel;
#ifdef ENABLE_ENTRY_STATE
  bool theWasConsumed;
  bool theWasHit;
#endif //ENABLE_ENTRY_STATE
#ifdef ENABLE_COMBO
  tID theHitId;
  long long theHitSeqNo;
#endif //ENABLE_COMBO
  bool theOS;
};

#ifdef ENABLE_ENTRY_STATE
#define ORDER_ENTRY_VALID 0
#define ORDER_ENTRY_HIT 1
#define ORDER_ENTRY_PRODUCED 2
#endif //ENABLE_ENTRY_STATE

struct OrderEntry {
  tAddress theAddress;
  long long theSequenceNo;

#ifdef ENABLE_PC_TRACKING
  tVAddress thePC;
#endif //ENABLE_PC_TRACKING

#ifdef ENABLE_ENTRY_STATE
  mutable unsigned short theConsumedMask;
  mutable unsigned char theBitVector;
#endif //ENABLE_ENTRY_STATE

#ifndef ENABLE_PC_TRACKING
  OrderEntry( tAddress anAddress, long long aSequenceNo, bool wasHit)
    : theAddress(anAddress)
    , theSequenceNo(aSequenceNo)
#ifdef ENABLE_ENTRY_STATE
    , theConsumedMask(0)
    , theBitVector(0)
#endif //ENABLE_ENTRY_STATE
  {
#ifdef ENABLE_ENTRY_STATE
    theBitVector |= (1 << ORDER_ENTRY_VALID);
    if(wasHit) {
      theBitVector |= (1 << ORDER_ENTRY_HIT);
    }
#endif //ENABLE_ENTRY_STATE
  }
#else //ENABLE_PC_TRACKING
  OrderEntry( tAddress anAddress, long long aSequenceNo, unsigned long long aPC )
    : theAddress(anAddress)
    , theSequenceNo(aSequenceNo)
    , thePC(aPC)
  { }
#endif //ENABLE_PC_TRACKING

  ~OrderEntry( ) {  }

#ifdef ENABLE_ENTRY_STATE
  bool hasConsumed(tID aNodeId) const {
    return ( theConsumedMask & ( 1 << aNodeId ));
  }
  void markConsumed(tID aNodeId) const {
    DBG_(Verb, ( << "Marking " << & std::hex << theAddress << & std::dec << " consumed by " << aNodeId) );
    theConsumedMask |= ( 1 << aNodeId );
  }

  bool isValid() const {
    return ( theBitVector & (1<<ORDER_ENTRY_VALID) );
  }
  void invalidate() const {
    theBitVector &= ~(1 << ORDER_ENTRY_VALID);
  }

  bool wasHit() const {
    return ( theBitVector & (1<<ORDER_ENTRY_HIT) );
  }

  bool wasProduced() const {
    return ( theBitVector & (1<<ORDER_ENTRY_PRODUCED) );
  }
  void produce() const {
    theBitVector |= (1 << ORDER_ENTRY_PRODUCED);
  }
#endif //ENABLE_ENTRY_STATE

#ifdef ENABLE_PC_TRACKING
  tVAddress pc() const {
    return thePC;
  }
#else //!ENABLE_PC_TRACKING
  unsigned long pc() const {
    return 0;
  }
#endif //ENABLE_PC_TRACKING

  tAddress address() const {
    return theAddress;
  }

  long long sequenceNo() const {
    return theSequenceNo;
  }

  friend std::ostream & operator << ( std::ostream & anOstream, OrderEntry const & anOrderEntry);
} __attribute__((packed));

std::ostream & operator << (std::ostream & str, TraceData const & aTraceData);

struct TraceProcessor {
  virtual ~TraceProcessor() {};
  virtual void finalize() = 0;
  virtual void event( TraceData & ) = 0;
};

class OrderMgr;
struct HitNotifier {
  virtual ~HitNotifier() {};
  virtual void finalize() = 0;
  virtual void setFinalForward(long long aSequenceNo) = 0;
  virtual void notifyChunkHit(long aChunkID, long anIndex, long long aSequenceNo, long long aHeadSequenceNo, tID aDirNode, bool aWatch) = 0;
  virtual void notifyStreamHit(long long aSequenceNo, long long aHeadSequenceNo, tID aDirNode, long aCurrBlocks, bool aWatch) = 0;
  virtual tAddress initialPC() = 0;
  virtual OrderMgr * getOrder() = 0;
  virtual int id() = 0;
  virtual long hits() = 0;
#ifdef ENABLE_ENTRY_STATE
  virtual bool createdAtHit() = 0;
#endif //ENABLE_ENTRY_STATE
  virtual long long firstSeqNo() = 0;
  virtual long long lastSeqNo() = 0;
};

struct SuperStream {
  virtual ~SuperStream() {};
  virtual void finalize() = 0;
  //virtual OrderMgr * getOrder(long aStreamIndex) = 0;
  virtual void beginForward() = 0;
  virtual void notifyHit(tAddress anAddress, tVAddress aPC, long aStreamIndex, long long aSequenceNo, long long aHeadSequenceNo, long aCurrBlocks) = 0;
  virtual bool notifyMiss(tAddress anAddress, tVAddress aPC) = 0;
  virtual int id(long aStreamIndex) const = 0;
};

struct MultiStream;

struct ForwardReceiver {
  virtual ~ForwardReceiver() {};
  virtual void finalize() = 0;
  virtual bool forward( tAddress anAddress, boost::shared_ptr<HitNotifier> aHitNotifier, long aChunkId, long anIndex, long long aSequenceNo, long long aHeadSequenceNo, bool aWatch ) = 0;
  virtual void notifyStream( tAddress anAddress, boost::shared_ptr<HitNotifier> aHitNotifier, long long aSequenceNo ) = 0;
  virtual std::string const & name( ) const = 0;
  virtual int id( ) const = 0;
};

struct PrefetchReceiver {
  virtual ~PrefetchReceiver() {};
  virtual void finalize() = 0;
  virtual std::string const & name() const = 0;
  virtual int id() const = 0;
  virtual bool forward(tAddress anAddress, boost::shared_ptr<SuperStream> aStream, long anIndex, long long aSequenceNo, long long aHeadSequenceNo) = 0;
};

struct MultiReceiver {
  virtual ~MultiReceiver() {};
  virtual void finalize() = 0;
  virtual std::string const & name() const = 0;
  virtual int id() const = 0;
  virtual bool forwardTSE(tAddress anAddress, tVAddress aPC, boost::shared_ptr<MultiStream> aStream, eForwardType aFwdType) = 0;
  virtual void clearAlt( ) = 0;
  virtual void forwardAlt( tAddress anAddress, bool ) = 0;
  virtual int altSize() = 0;
};

struct AppendListener {
  virtual ~AppendListener() {};
  virtual void finalize() = 0;
  virtual void appended(tAddress anAddress, long long aSequenceNo, long long aHeadSequenceNo) = 0;
};

struct Directory {
  virtual ~Directory() {}
  virtual tID node(tAddress const & anAddress, const tID aRequestingNode) = 0;
  virtual tID node(tAddress const & anAddress) = 0;
};


typedef boost::function< bool ( TraceData & ) > Predicate;
typedef boost::function< int ( TraceData & ) > SplitFn;
typedef boost::function< boost::shared_ptr<HitNotifier> ( tAddress, ForwardReceiver *, tAddress ) > StreamRequestFn;
typedef boost::function< boost::shared_ptr<SuperStream> ( PrefetchReceiver *, tAddress, tAddress ) > PullStreamRequestFn;
typedef boost::function< boost::shared_ptr<MultiStream> ( MultiReceiver *, tAddress, tAddress ) > MultiStreamRequestFn;

struct by_LRU {};
struct by_location {};
struct by_sequence_no{};
struct by_address {};
struct by_seq {};
struct by_stream {};
struct by_replacement {};
struct by_delta {};

TraceCoordinator * initialize_OrderHW(std::string options);

struct StreamTrackerBase {
   virtual ~StreamTrackerBase() {}
   virtual void finalize() = 0;
};

extern StreamTrackerBase * theStreamTracker;

#endif //COMMON_INCLUDED
