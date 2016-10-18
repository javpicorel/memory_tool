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
#ifndef _SHARING_TRACKER_HPP_
#define _SHARING_TRACKER_HPP_

#include <core/simics/configuration_api.hpp>
#include <boost/pool/pool.hpp>


namespace Flexus {
namespace Simics {
namespace API {
extern "C" {
  #define restrict
  #include FLEXUS_SIMICS_API_HEADER(memory)
  #undef restrict
}
}
}
}

#include <ext/hash_map>

using namespace Flexus;
using namespace Core;

#define MyLevel Iface

#include DBG_Control()

namespace nTraceTracker {

enum SharingType {
  eFalseSharing,
  eTrueSharing,
  eSilentStore
};

typedef unsigned long long SharingVector;

struct SharingInfo {
private:
  struct flags_t {
    unsigned pendingInvTag:1;
    unsigned pendingInv:1;
    unsigned filledAfterInval:1;
    unsigned presentL1:1;
    unsigned presentL2:1;
    unsigned potentialSilentStore:1;
    unsigned potentialOnlyReadAfterWrite:1;  // temporary
    flags_t()
      : pendingInvTag(true)
      , pendingInv(false)
      , filledAfterInval(false)
      , presentL1(false)
      , presentL2(false)
    {}
  } flags;

  SharingVector updatedSinceInval;
  SharingVector accessedAfterFill;
  SharingVector updatedAfterFill;
  SharingVector valueDiff;
  unsigned long long * valueAtInval;

public:
  SharingInfo()
    : flags()
    , valueAtInval(0)
  {}
  void reset(unsigned long long * blockValue) {
    flags.pendingInvTag = false;
    flags.pendingInv = false;
    flags.filledAfterInval = false;

    flags.potentialSilentStore = true;
    flags.potentialOnlyReadAfterWrite = true;

    updatedSinceInval = 0;
    accessedAfterFill = 0;
    updatedAfterFill = 0;
    valueAtInval = blockValue;
  }

  void setValueDiff(SharingVector diff) {
    valueDiff = diff;
    valueAtInval = 0;
  }
  unsigned long long * getValueAtInval() {
    return valueAtInval;
  }

  SharingType finalize() {
    SharingType res = eFalseSharing;
    SharingVector sharing = updatedSinceInval & accessedAfterFill;
    if(sharing != 0) {
      if(flags.potentialSilentStore) {
        res = eSilentStore;
      } else {
        res = eTrueSharing;
      }
    }
    return res;
  }

  void updated(unsigned long offset, int size, bool otherNode) {
    // work in 8-byte chunks
    offset >>= 3;
    unsigned long final = ((size - 1) >> 3) + offset;
    for(; offset <= final; offset++) {
      if(otherNode) {
        updatedSinceInval |= (1 << offset);
      } else {
        updatedAfterFill |= (1 << offset);
      }
    }
  }
  void accessed(unsigned long offset, int size) {
    // work in 8-byte chunks
    offset >>= 3;
    unsigned long final = ((size - 1) >> 3) + offset;
    for(; offset <= final; offset++) {
      if( (1<<offset) & valueDiff ) {
        if( (1<<offset) & (~updatedAfterFill) ) {
          flags.potentialSilentStore = false;
        }
      }

      accessedAfterFill |= (1 << offset);

      if( (1<<offset) & (~updatedAfterFill) ) {
        flags.potentialOnlyReadAfterWrite = false;
      }
    }
  }

  void dataIn(int level) {
    if(level == 1) {
      flags.presentL1 = true;
    } else if(level == 2) {
      flags.presentL2 = true;
    } else {
      DBG_Assert(false, ( << "marking data present for cache level " << level ) );
    }
  }
  void dataOut(int level) {
    if(level == 1) {
      flags.presentL1 = false;
    } else if(level == 2) {
      flags.presentL2 = false;
    } else {
      DBG_Assert(false, ( << "marking data absent for cache level " << level ) );
    }
  }
  bool dataPresent() {
    return (flags.presentL1 || flags.presentL2);
  }

  void setFilledAfterInval() {
    flags.filledAfterInval = true;
  }
  bool filledAfterInval() {
    return flags.filledAfterInval;
  }

  void setPendingInv() {
    flags.pendingInv = true;
  }
  bool pendingInv() {
    return flags.pendingInv;
  }

  bool potentialOnlyReadAfterWrite() {
    return flags.potentialOnlyReadAfterWrite;
  }

  void setPendingInvTag() {
    flags.pendingInvTag = true;
  }
  bool pendingInvTag() {
    return flags.pendingInvTag;
  }

  bool firstAccessAfterInval() {
    return (valueAtInval != 0);
  }
  bool noAccesses() {
    return (accessedAfterFill == 0);
  }
};

struct IntHash {
  std::size_t operator()(unsigned long key) const {
    key = key >> 6;
    return key;
  }
};

class SharingTracker {
  int theNumNodes;
  int theNumChunks;
  boost::pool<> theBlockValuePool;
  Simics::API::conf_object_t * theCPU;
  unsigned long long * theCurrValue;

  typedef __gnu_cxx::hash_map<address_t,SharingInfo,IntHash> SharingMap;
  std::vector<SharingMap> theInvalidTags;

  Stat::StatCounter statFalseSharing;
  Stat::StatCounter statTrueSharing;
  Stat::StatCounter statSilentStores;
  Stat::StatCounter statInvTagReplace;
  Stat::StatCounter statNoAccesses;
  Stat::StatCounter statOnlyReadAfterWrite;
  Stat::StatCounter statInvTagReplaceData;
  Stat::StatCounter statInvTagReplaceNoData;

public:
  SharingTracker(int numNodes, long blockSize)
    : theNumNodes(numNodes)
    , theNumChunks(blockSize >> 3)
    , theBlockValuePool(blockSize)
    , theCPU(0)
    , theCurrValue(0)
    , statFalseSharing("sys-SharingTracker-FalseSharing")
    , statTrueSharing("sys-SharingTracker-TrueSharing")
    , statSilentStores("sys-SharingTracker-SilentStores")
    , statInvTagReplace("sys-SharingTracker-InvalidTagReplace")
    , statNoAccesses("sys-SharingTracker-OnlyWrites")
    , statOnlyReadAfterWrite("sys-SharingTracker-OnlyReadAfterWrite")
    , statInvTagReplaceData("sys-SharingTracker-InvTagReplaceData")
    , statInvTagReplaceNoData("sys-SharingTracker-InvTagReplaceNoData")
  {
    theInvalidTags.resize(theNumNodes);

    theCPU = Simics::API::SIM_get_object( "cpu0" );
    if(!theCPU) {
      theCPU = Simics::API::SIM_get_object( "server_cpu0" );
    }
    if(!theCPU) {
      DBG_Assert(false, ( << "Unable to find CPU Simics conf object" ) );
    }

    theCurrValue = (unsigned long long *) theBlockValuePool.malloc();
  }

  void finalize() {
    // do nothing
  }

  void accessLoad(int aNode, address_t block, unsigned long offset, int size) {
    if(block==0xd170080/*FS*/ || block==0x1da87240/*SS*/)
      DBG_(MyLevel, (<< "[" << aNode << "] accessLoad 0x" << std::hex << block << "," << offset));

    SharingMap::iterator iter = theInvalidTags[aNode].find(block);
    if(iter != theInvalidTags[aNode].end()) {
      if(iter->second.firstAccessAfterInval()) {
        iter->second.setValueDiff( calcValueDiff(block, iter->second.getValueAtInval()) );
      }
      iter->second.accessed(offset, size);
    }
  }

  void accessStore(int aNode, address_t block, unsigned long offset, int size) {
    if(block==0xd170080/*FS*/ || block==0x1da87240/*SS*/)
      DBG_(MyLevel, (<< "[" << aNode << "] accessStore 0x" << std::hex << block << "," << offset));

    int ii;
    for(ii = 0; ii < theNumNodes; ii++) {
      SharingMap::iterator iter = theInvalidTags[ii].find(block);
      if(iter != theInvalidTags[ii].end()) {
        if(ii == aNode) {
          if(iter->second.firstAccessAfterInval()) {
            iter->second.setValueDiff( calcValueDiff(block, iter->second.getValueAtInval()) );
          }
        }
        iter->second.updated(offset, size, (ii != aNode));
      }
    }
  }

  void accessAtomic(int aNode, address_t block, unsigned long offset, int size) {
    if(block==0xd170080/*FS*/ || block==0x1da87240/*SS*/)
      DBG_(MyLevel, (<< "[" << aNode << "] accessAtomic 0x" << std::hex << block << "," << offset));

    accessLoad(aNode, block, offset, size);
    accessStore(aNode, block, offset, size);
  }

  void fill(int aNode, int aCacheLevel, address_t block) {
    if(block==0xd170080/*FS*/ || block==0x1da87240/*SS*/)
      DBG_(MyLevel, (<< "[" << aNode << ":" << aCacheLevel << "] fill 0x" << std::hex << block));

    SharingMap::iterator iter = theInvalidTags[aNode].find(block);
    if(iter != theInvalidTags[aNode].end()) {
      iter->second.dataIn(aCacheLevel);
      if(aCacheLevel == 2) {
        iter->second.setFilledAfterInval();
      }
    }
  }

  void insert(int aNode, int aCacheLevel, address_t block) {
    if(block==0xd170080/*FS*/ || block==0x1da87240/*SS*/)
      DBG_(MyLevel, (<< "[" << aNode << ":" << aCacheLevel << "] insert 0x" << std::hex << block));

    SharingMap::iterator iter = theInvalidTags[aNode].find(block);
    if(iter != theInvalidTags[aNode].end()) {
      iter->second.dataIn(aCacheLevel);
      DBG_Assert(iter->second.filledAfterInval());
    }
  }

  void evict(int aNode, int aCacheLevel, address_t block, bool drop) {
    if(block==0xd170080/*FS*/ || block==0x1da87240/*SS*/)
      DBG_(MyLevel, (<< "[" << aNode << ":" << aCacheLevel << "] evict 0x" << std::hex << block ));

    SharingMap::iterator iter = theInvalidTags[aNode].find(block);
    if(iter != theInvalidTags[aNode].end()) {
      iter->second.dataOut(aCacheLevel);
      if(!iter->second.pendingInv()) {
        if(!iter->second.dataPresent()) {
          if(drop || aCacheLevel==2) {
            doStats(iter->second, block);
            DBG_Assert(!iter->second.pendingInvTag(), ( << "[" << aNode << "]: 0x" << std::hex << block ) );
            theInvalidTags[aNode].erase(freeSharing(iter));
          }
        }
      }
    }
  }

  void invalidate(int aNode, int aCacheLevel, address_t block) {
    if(block==0xd170080/*FS*/ || block==0x1da87240/*SS*/)
      DBG_(MyLevel, (<< "[" << aNode << ":" << aCacheLevel << "] invalidate 0x" << std::hex << block ));

    SharingMap::iterator iter = theInvalidTags[aNode].find(block);
    if(iter != theInvalidTags[aNode].end()) {
      iter->second.dataOut(aCacheLevel);
      if(aCacheLevel == 2) {
        DBG_Assert(!iter->second.pendingInv());
        iter->second.setPendingInv();
      }
    }
  }

  void invalidAck(int aNode, address_t block) {
    if(block==0xd170080/*FS*/ || block==0x1da87240/*SS*/)
      DBG_(MyLevel, (<< "[" << aNode << "] invalidAck 0x" << std::hex << block ));

    SharingMap::iterator iter = theInvalidTags[aNode].find(block);
    if(iter != theInvalidTags[aNode].end()) {
      DBG_Assert(iter->second.pendingInv(), ( << "[" << aNode << "]: 0x" << std::hex << block ) );
      DBG_Assert(!iter->second.dataPresent(),  ( << "[" << aNode << "]: 0x" << std::hex << block ) );
      if(iter->second.filledAfterInval()) {
        doStats(iter->second, block);
      }
      if(iter->second.pendingInvTag()) {
        iter->second.reset( readBlockValue(block) );
      } else {
        theInvalidTags[aNode].erase(freeSharing(iter));
      }
    }
  }

  void invTagCreate(int aNode, address_t block) {
    if(block==0xd170080/*FS*/ || block==0x1da87240/*SS*/)
      DBG_(MyLevel, (<< "[" << aNode << "] tagCreate 0x" << std::hex << block ));

    SharingMap::iterator iter = theInvalidTags[aNode].find(block);
    if(iter != theInvalidTags[aNode].end()) {
      iter->second.setPendingInvTag();
    } else {
      theInvalidTags[aNode].insert( std::make_pair(block,SharingInfo()) );
    }
  }

  void invTagRefill(int aNode, address_t block) {
    if(block==0xd170080/*FS*/ || block==0x1da87240/*SS*/)
      DBG_(MyLevel, (<< "[" << aNode << "] tagRefill 0x" << std::hex << block ));
  }

  void invTagReplace(int aNode, address_t block) {
    if(block==0xd170080/*FS*/ || block==0x1da87240/*SS*/)
      DBG_(MyLevel, (<< "[" << aNode << "] tagReplace 0x" << std::hex << block ));

    SharingMap::iterator iter = theInvalidTags[aNode].find(block);
    if(iter != theInvalidTags[aNode].end()) {
      if(!iter->second.pendingInv()) {
        if(iter->second.dataPresent()) {
          DBG_Assert(iter->second.filledAfterInval(), ( << "[" << aNode << "]: 0x" << std::hex << block ) );
          statInvTagReplaceData++;
        }
        else {
          DBG_Assert(!iter->second.filledAfterInval(), ( << "[" << aNode << "]: 0x" << std::hex << block ) );
          DBG_Assert(iter->second.noAccesses(), ( << "[" << aNode << "]: 0x" << std::hex << block ) );
          statInvTagReplaceNoData++;
          theInvalidTags[aNode].erase(freeSharing(iter));
        }
      }
      statInvTagReplace++;
    }
  }

private:
  void readBlockValue(address_t block, unsigned long long * array) {
    int ii;
    for(ii = 0; ii < theNumChunks; ii++) {
      array[ii] = Simics::API::SIM_read_phys_memory(theCPU, block, 8);
      block += 8;
    }
  }
  unsigned long long * readBlockValue(address_t block) {
    unsigned long long * array = (unsigned long long *) theBlockValuePool.malloc();
    readBlockValue(block, array);
    return array;
  }

  SharingVector calcValueDiff(address_t block, unsigned long long * priorValue) {
    DBG_Assert(priorValue, ( << "address 0x" << std::hex << block ) );
    readBlockValue(block, theCurrValue);

    SharingVector valueDiff = 0;
    int ii;
    for(ii = 0; ii < theNumChunks; ii++) {
      if(theCurrValue[ii] != priorValue[ii]) {
        valueDiff |= (1 << ii);
      }
    }

    theBlockValuePool.free(priorValue);

    return valueDiff;
  }

  SharingMap::iterator & freeSharing(SharingMap::iterator & iter) {
    unsigned long long * array = iter->second.getValueAtInval();
    if(array) {
      theBlockValuePool.free(array);
    }
    return iter;
  }

  void doStats(SharingInfo & data, address_t block) {
    if(data.noAccesses()) {
      statNoAccesses++;
    }
    if(data.potentialOnlyReadAfterWrite()) {
      statOnlyReadAfterWrite++;
    }

    SharingType sharing = data.finalize();
    switch(sharing) {
    case eFalseSharing:
      //DBG_(Dev, ( << "     FS:" << std::hex << block ) );
      statFalseSharing++;
      break;
    case eTrueSharing:
      //DBG_(Dev, ( << "     TS:" << std::hex << block ) );
      statTrueSharing++;
      break;
    case eSilentStore:
      //DBG_(Dev, ( << "     SS:" << std::hex << block ) );
      statSilentStores++;
      break;
    default:
      DBG_Assert(false);
    }
  }

};

} // namespace nTraceTracker

#undef MyLevel

#endif
