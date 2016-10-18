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


typedef unsigned long long tTime;
typedef unsigned long tAddress;
typedef unsigned long long tVAddress;
typedef int tCoreId;
typedef unsigned long long tThreadId;

enum tEventType {
  eRead,
  eWrite,
  eFetch,
  eL1CleanEvict,
  eL1DirtyEvict,
  eL1IEvict
};

using Flexus::SharedTypes::tFillType;
using Flexus::SharedTypes::eCold;
using Flexus::SharedTypes::eReplacement;
using Flexus::SharedTypes::eCoherence;
using Flexus::SharedTypes::eDMA;
using Flexus::SharedTypes::ePrefetch;
using Flexus::SharedTypes::tFillLevel;
using Flexus::SharedTypes::eLocalMem;


struct TraceCoordinator {
  virtual ~TraceCoordinator() {}

  virtual void accessL2(tTime aTime
                        , tEventType aType
                        , tCoreId aNode
                        , tThreadId aThread
                        , tAddress anAddress
                        , tVAddress aPC
                        , tFillType aFillType
                        , tFillLevel aFillLevel
                        , bool anOS) 
    = 0;
  virtual void accessOC(tTime aTime
                        , tEventType aType
                        , tCoreId aNode
                        , tThreadId aThread
                        , tAddress anAddress
                        , tVAddress aPC
                        , tFillType aFillType
                        , tFillLevel aFillLevel
                        , bool anOS) 
    = 0;
  virtual void finalize( void ) = 0;
  virtual void processTrace( void ) = 0;
};

struct TraceData {
  TraceData( unsigned long long aTime
             , tEventType anEventType
             , tCoreId aNode
             , tThreadId aThread
             , tAddress anAddress
             , tVAddress aPC
             , tFillType aFillType
             , tFillLevel aFillLevel
             , bool isOS 
             )
    : theTime(aTime)
    , theEventType(anEventType)
    , theNode(aNode)
    , theThread(aThread)
    , theAddress(anAddress)
    , thePC(aPC)
    , theFillType(aFillType)
    , theFillLevel(aFillLevel)
    , theOS(isOS)
  {}
  TraceData(void) { }

  tTime theTime;
  tEventType theEventType;
  tCoreId theNode;
  tThreadId theThread;
  tAddress theAddress;
  tVAddress thePC;
  tFillType theFillType;
  tFillLevel theFillLevel;
  bool theOS;
};

std::ostream & operator << (std::ostream & str, TraceData const & aTraceData);


TraceCoordinator * initialize_NUCATracer(int aNumNodes);


#endif //COMMON_INCLUDED
