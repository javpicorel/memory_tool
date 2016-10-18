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

#include "common.hpp"
#include "trace.hpp"


#define DBG_DefineCategories ExperimentDbg, ReaderDbg, IgnoreDbg
#define DBG_SetInitialGlobalMinSev Iface
#include DBG_Control()

#include <fstream>

#include <boost/shared_ptr.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace l = boost::lambda;

#include <boost/utility.hpp>

#define __STDC_CONSTANT_MACROS
#include <boost/date_time/posix_time/posix_time.hpp>


#include "coordinator.hpp"
#include "trace.hpp"



std::ostream & operator << (std::ostream & str, TraceData const & aTraceData) {
  str << "[" << fill<2>(aTraceData.theNode) << "]";
  switch (aTraceData.theEventType) {
    case eRead:
      str << " R @";
      break;
    case eWrite:
      str << " W @";
      break;
    case eFetch:
      str << " F @";
      break;
  }
  str << std::hex << aTraceData.theAddress << std::dec ;

  return str;
}

tCoordinator::tCoordinator(std::string const & aPath ){
  trace::initialize(aPath);
}

tCoordinator::~tCoordinator() {}


void tCoordinator::accessL2(unsigned long long aTime, tEventType aType, tID aNode, tAddress anAddress, tVAddress aPC, tFillType aFillType, tFillLevel aFillLevel, bool anOS) {
  TraceData evt(aTime, aType, aNode, anAddress, aPC, aFillType, aFillLevel, anOS );
  trace::addL2Access(evt);
}

void tCoordinator::accessOC(unsigned long long aTime, tEventType aType, tID aNode, tAddress anAddress, tVAddress aPC, tFillType aFillType, tFillLevel aFillLevel, bool anOS) {
  TraceData evt(aTime, aType, aNode, anAddress, aPC, aFillType, aFillLevel, anOS );
  trace::addOCAccess(evt);
}

void tCoordinator::finalize() {
  boost::posix_time::ptime now(boost::posix_time::second_clock::local_time());
  DBG_(Dev, Cat(ExperimentDbg) ( << "Finalizing state. " << boost::posix_time::to_simple_string(now)));

  trace::flushFiles();
}

