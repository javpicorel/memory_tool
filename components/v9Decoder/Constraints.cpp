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

#include <iostream>
#include <iomanip>
#include <bitset>

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
namespace ll = boost::lambda;

#include <core/target.hpp>
#include <core/debug/debug.hpp>
#include <core/types.hpp>

#include <components/uArch/uArchInterfaces.hpp>

#include "SemanticInstruction.hpp"
#include "Effects.hpp"
#include "Constraints.hpp"

  #define DBG_DeclareCategories v9Decoder
  #define DBG_SetDefaultOps AddCat(v9Decoder)
  #include DBG_Control()

namespace nv9Decoder {

using nuArch::SemanticAction;
using nuArch::eResourceStatus;
using nuArch::kReady;

using nuArch::kSC;
using nuArch::kTSO;


bool checkStoreQueueAvailable( SemanticInstruction * anInstruction ) {
  if (! anInstruction->core()) {
    return false;
  }
  if ( anInstruction->core()->sbFull()) {
    return false;
  }
  return true;
}

boost::function<bool()> storeQueueAvailableConstraint( SemanticInstruction * anInstruction ) {
  return ll::bind( &checkStoreQueueAvailable, anInstruction );
}

bool checkMembarStoreStoreConstraint( SemanticInstruction * anInstruction ) {
  if (! anInstruction->core()) {
    return false;
  }
  return anInstruction->core()->mayRetire_MEMBARStSt();
}

boost::function<bool()> membarStoreStoreConstraint( SemanticInstruction * anInstruction ) {
  return ll::bind( &checkMembarStoreStoreConstraint, anInstruction );
}

bool checkMembarStoreLoadConstraint( SemanticInstruction * anInstruction ) {
  if (! anInstruction->core()) {
    return false;
  }
  return anInstruction->core()->mayRetire_MEMBARStLd();
}

boost::function<bool()> membarStoreLoadConstraint( SemanticInstruction * anInstruction ) {
  return ll::bind( &checkMembarStoreLoadConstraint, anInstruction );
}

bool checkMembarSyncConstraint( SemanticInstruction * anInstruction ) {
  if (! anInstruction->core()) {
    return false;
  }
  return anInstruction->core()->mayRetire_MEMBARSync();
}


boost::function<bool()> membarSyncConstraint( SemanticInstruction * anInstruction ) {
  return ll::bind( &checkMembarSyncConstraint, anInstruction );
}

bool checkMemoryConstraint( SemanticInstruction * anInstruction ) {
  if (! anInstruction->core()) {
    return false;
  }
  switch (anInstruction->core()->consistencyModel() ) {
    case kSC:
      if (! anInstruction->core()->speculativeConsistency()) { 
        //Under nonspeculative SC, a load instruction may only retire when no stores are outstanding.
        if ( ! anInstruction->core()->sbEmpty()) {
          return false;
        }
      }
      break;
    case kTSO:
    case kRMO:
      //Under TSO and RMO, a load may always retire when it reaches the
      //head of the re-order buffer.
      break;
    default:
      DBG_Assert( false, ( << "Load Memory Instruction does not support consistency model " << anInstruction->core()->consistencyModel() ) );
  }
  return true;
}

boost::function<bool()> loadMemoryConstraint( SemanticInstruction * anInstruction ) {
  return ll::bind( &checkMemoryConstraint, anInstruction );
}

bool checkStoreQueueEmpty( SemanticInstruction * anInstruction ) {
  if (! anInstruction->core()) {
    return false;
  }
  return anInstruction->core()->sbEmpty();
}

boost::function<bool()> storeQueueEmptyConstraint( SemanticInstruction * anInstruction ) {
  return ll::bind( &checkStoreQueueEmpty, anInstruction );
}


bool checkSideEffectStoreConstraint( SemanticInstruction * anInstruction ) {
  if (! anInstruction->core()) {
    return false;
  }
  return anInstruction->core()->checkStoreRetirement(boost::intrusive_ptr<nuArch::Instruction>(anInstruction));
}

boost::function<bool()> sideEffectStoreConstraint( SemanticInstruction * anInstruction ) {
  return ll::bind( &checkSideEffectStoreConstraint, anInstruction );
}

} //nv9Decoder
