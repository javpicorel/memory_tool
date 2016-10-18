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
namespace nDgpTable {

unsigned int log_base2(unsigned int num) {
  unsigned int ii = 0;
  while(num > 1) {
    ii++;
    num >>= 1;
    }
  return ii;
}

unsigned int max(unsigned int a, unsigned int b) {
  return (a>b ? a : b);
}

class SignatureMapping {
public:
  // assume at most 32-bit signatures
  typedef Flexus::Core::MemoryAddress_<Flexus::Core::Word32Bit> Signature;

  // we still need the full address for indexing
  typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

  SignatureMapping(unsigned int blockAddrBits, unsigned int pcBits, unsigned int blockSize)
    : myHistBits(pcBits)
    , myAddrBits(blockAddrBits)
    , mySigBits(max(pcBits,blockAddrBits))
    , myBlockBits(log_base2(blockSize))
  {}

  Signature makeHist(MemoryAddress aPC) {
    // ignore the lowest two bits since PCs increment by 4
    unsigned int temp = aPC >> 2;
    // truncate
    temp &= (1<<myHistBits) - 1;
    //DBG_(Tmp, ( << "from pc=" << std::hex << aPC << ", made hist=" << temp ) );
    return Signature(temp);
  }

  Signature updateHist(Signature aHist, MemoryAddress aPC) {
    // ignore the lowest two bits since PCs increment by 4
    unsigned int temp = aPC >> 2;
    // use addition to munge the two together
    temp += aHist;
    // now truncate
    temp &= (1<<myHistBits) - 1;
    //DBG_(Tmp, ( << "from pc=" << std::hex << aPC << " hist=" << aHist <<", updated hist=" << temp ) );
    return Signature(temp);
  }

  Signature makeSig(MemoryAddress anAddr, Signature aHist) {
    // ignore bits that map within the coherency unit
    unsigned int temp = anAddr >> myBlockBits;
    // truncate the address
    temp &= (1<<myAddrBits) - 1;

    // shuffle left?
    temp <<= 8;

    // XOR with the history (should already be truncated)
    temp ^= aHist;
    // truncate (no longer necessary): temp &= (1<<mySigBits) - 1;
    //DBG_(Tmp, ( << "from addr=" << std::hex << anAddr << " hist=" << aHist << ", made sig=" << temp ) );
    return Signature(temp);
  }

  const unsigned int myHistBits;
  const unsigned int myAddrBits;
  const unsigned int mySigBits;
  const unsigned int myBlockBits;
};  // end class SignatureMapping


class Confidence {
  // the confidence is encoded in the lowest seven bits
  // the highest bit is set for the initial access
  unsigned char bitVector;

  // tracks if the confidence has flip-flopped
  //unsigned char flipFlop;

public:
#ifdef DGP_SUBTRACE
  // track subtraces
  std::set<Confidence*> tracePtrs;
  bool subtrace;
  std::vector<int> numPCseq;
#endif

public:
  Confidence()
    : bitVector(0x80 | theDGPInitialConf)
    //, flipFlop(0)
#ifdef DGP_SUBTRACE
    , subtrace(false)
#endif
  {}

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & bitVector;
  } 

  void reset() {
    bitVector = 0x80 | theDGPInitialConf;   // init=true, conf=1
    //flipFlop = 0;
#ifdef DGP_SUBTRACE
    subtrace = false;
    tracePtrs.clear();
    numPCseq.clear();
#endif
  }

  void raise() {
    if(!initial()) {
      if(bitVector == (theDGPThresholdConf-1)) {
        //flipFlop |= 0x1;
      }
    }

    bitVector &= 0x7f;  // clear init bit
    if(bitVector < theDGPMaxConf) {
      bitVector++;
    }
  }
  bool lower() {
    if(bitVector == theDGPThresholdConf) {
      //flipFlop |= 0x2;
    }

    bitVector &= 0x7f;  // clear init bit
    if(bitVector > 0) {
      bitVector--;
      return false;
    }
    return true;
  }

  void raise(DgpStats * stats, int table) {
    if(!initial()) {
      if(bitVector == (theDGPThresholdConf-1)) {
        //flipFlop |= 0x1;
      }
    }

    bitVector &= 0x7f;  // clear init bit
    if(bitVector < theDGPMaxConf) {
      int val = (static_cast<int>(bitVector)) * 10 + static_cast<int>(bitVector) + 1;
      stats->ConfTransitions << std::make_pair(val, 1);
      if(table == 0) {
        stats->ConfTransitions0Table << std::make_pair(val, 1);
      } else if(table == 1) {
        stats->ConfTransitions1Table << std::make_pair(val, 1);
      }
      bitVector++;
    } else {
      int val = (static_cast<int>(bitVector)) * 10 + static_cast<int>(bitVector);
      stats->ConfTransitions << std::make_pair(val, 1);
      if(table == 0) {
        stats->ConfTransitions0Table << std::make_pair(val, 1);
      } else if(table == 1) {
        stats->ConfTransitions1Table << std::make_pair(val, 1);
      }
    }
  }
  bool lower(DgpStats * stats, int table) {
    if(bitVector == theDGPThresholdConf) {
      //flipFlop |= 0x2;
    }

    bitVector &= 0x7f;  // clear init bit
    if(bitVector > 0) {
      int val = (static_cast<int>(bitVector)) * 10 + static_cast<int>(bitVector) - 1;
      stats->ConfTransitions << std::make_pair(val, 1);
      if(table == 0) {
        stats->ConfTransitions0Table << std::make_pair(val, 1);
      } else if(table == 1) {
        stats->ConfTransitions1Table << std::make_pair(val, 1);
      }
      bitVector--;
      return false;
    } else {
      int val = (static_cast<int>(bitVector)) * 10 + static_cast<int>(bitVector);
      stats->ConfTransitions << std::make_pair(val, 1);
      if(table == 0) {
        stats->ConfTransitions0Table << std::make_pair(val, 1);
      } else if(table == 1) {
        stats->ConfTransitions1Table << std::make_pair(val, 1);
      }
      return true;
    }
  }

  unsigned char val() {
    return (bitVector & 0x7f);
  }

  bool predict() {
    if( (bitVector&0x7f) >= theDGPThresholdConf ) {
      return true;
    }
    return false;
  }

  bool initial() {
    return (bitVector & 0x80);
  }

  bool min() {
    return (bitVector == 0);
  }

  bool max() {
    return ( (bitVector&0x7f) == theDGPMaxConf );
  }

  bool isFlipping() {
    //return (flipFlop == 0x3);
    return false;
  }

#ifdef DGP_SUBTRACE
  bool isSubtrace() {
    return subtrace;
  }
  bool isSupertrace() {
    if(!subtrace) {
      if(!tracePtrs.empty()) {
        return true;
      }
    }
    return false;
  }

  int numSubtraces() {
    if(subtrace) {
      return -1;
    }
    return tracePtrs.size();
  }
  Confidence * getSupertrace() {
    DBG_Assert(isSubtrace());
    DBG_Assert(tracePtrs.size() == 1);
    return *(tracePtrs.begin());
  }
  void addSubtraces(std::set<Confidence*> & newSubtraces) {
    subtrace = false;
    std::set<Confidence*>::iterator iter = newSubtraces.begin();
    while(iter != newSubtraces.end()) {
      (*iter)->subtrace = true;
      (*iter)->tracePtrs.clear();
      (*iter)->tracePtrs.insert(this);
      tracePtrs.insert(*iter);
      ++iter;
    }
  }
  void numStores(int number) {
    numPCseq.push_back(number);
  }
#endif

  friend std::ostream & operator << (std::ostream & anOstream, const Confidence & aConf) {
    return anOstream << (int)(aConf.bitVector /*& 0x7f*/);
  }

};  // end class Confidence


}  // end namespace nDgpTable
