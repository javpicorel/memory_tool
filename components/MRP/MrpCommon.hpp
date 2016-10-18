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
namespace nMrpTable {

inline unsigned int log_base2(unsigned int num) {
  unsigned int ii = 0;
  while(num > 1) {
    ii++;
    num >>= 1;
    }
  return ii;
}

inline unsigned int max(unsigned int a, unsigned int b) {
  return (a>b ? a : b);
}


struct ReadersConf {
  // the lower 16 bits contain the MSB of the counter for each node -
  // since we predict on (binary) values 10 and 11, these lower 16 bits
  // are the prediction
  ReadersConf()
    : theReaders(0)
  {}
  ReadersConf(unsigned int readers)
    : theReaders(0)
  {
    setReaders(readers);
  }

  const unsigned int predict() const {
    return (theReaders & 0xffff);
  }
  void setReaders(unsigned int readers) {
      // to initialize values to 10 (i.e. predict by default), copy the values
      // into the lower 16 bits
      // to initialize values to 01 (i.e. not predict by default), copy the values
      // into the upper 16 bits
      theReaders = (readers << 16) & 0xffff0000;
  }
  void updateReaders(unsigned int actual) {
    unsigned int newConf = 0;
    // increase the confidence of every actual reader and decrease confidence
    // of the others
    for(int ii = 15; ii >= 0; ii--) {
      unsigned int conf = (((theReaders>>ii) & 0x1) << 1) | ((theReaders>>(ii+16)) & 0x1);
      if(actual & (1<<ii)) {
        // this node actually read => increase confidence
        if(conf < 3) {
          conf++;
        }
      } else {
        // this node didn't read => decrease confidence
        if(conf > 0) {
          conf--;
        }
      }
      if(conf & 0x2) {  // MSB
        newConf |= (1 << ii);
      }
      if(conf & 0x1) {  // LSB
        newConf |= (1 << (ii+16));
      }
    }
    DBG_(VVerb, Condition(theReaders != newConf)
               ( << "update conf: old=" << std::hex << theReaders
                 << " actual=" << actual << " new=" << newConf ) );
    theReaders = newConf;
  }

  friend std::ostream & operator << (std::ostream & anOstream, ReadersConf const & aConf) {
    anOstream << "/" << & std::hex << aConf.predict() << "/ ^" << aConf.theReaders << & std::dec <<"^";
    return anOstream;
  }

  unsigned int theReaders;
};  // class ReadersConf

struct ReadersNoConf {
  ReadersNoConf()
    : theReaders(0)
  {}
  ReadersNoConf(unsigned int readers)
    : theReaders(readers)
  {}

  const unsigned int predict() {
    return theReaders;
  }
  void setReaders(unsigned int readers) {
    theReaders = (readers & 0xffff);
  }
  void updateReaders(unsigned int actual) {
    // next time, use the superset of both the previously predicted readers
    // and the actual readers
    theReaders |= (actual & 0xffff);
  }

  unsigned int theReaders;
};  // class ReadersNoConf

struct GniadyConf {
  GniadyConf()
    : theReaders(0)
    , theConf(0)
  {}
  GniadyConf(unsigned int readers)
    : theReaders(readers)
    , theConf(/*theFastWarming ? 2 :*/ 1)
  {}

  const unsigned int predict() {
    unsigned int prediction = 0;
    if (theConf >= 2) {
      prediction = theReaders;
    }
    theLastPredict = theReaders;
    return prediction;
  }
  void setReaders(unsigned int readers) {
    theReaders = (readers & 0xffff);
  }
  void updateReaders(unsigned int actual) {
    //See if we should increase or decrease confidence
    int correctly_predicted = actual & theLastPredict;
    if (correctly_predicted) {
      if ( static_cast<float>(count(correctly_predicted)) / count(actual) >= 0.8 ) {
        //>= 80% accuracy on last prediction
        if (theConf != 3) {
           ++theConf;
        }
      } else {
        //< 80% accuracy on last prediction
        if (theConf != 0) {
           --theConf;
        }
      }
    } else {
      //0% accuracy on last prediction
      if (theConf != 0) {
         --theConf;
      }
    }

    //Next time, use the list of readers from this time
    theReaders = (actual & 0xffff);
  }

  int count(unsigned int value) {
    int ret = 0;
    unsigned int sharers = value;
    for(int ii = 0; ii < 16; ii++) {
      if(sharers & 0x01) {
        ret++;
      }
      sharers >>= 1;
    }
    return ret;
  }

  friend std::ostream & operator << (std::ostream & anOstream, GniadyConf const & aConf) {
    anOstream << & std::hex << "/" << aConf.theLastPredict << "/ \\" << aConf.theReaders << & std::dec << "\\ ^" << aConf.theConf <<"^";
    return anOstream;
  }

  unsigned int theReaders;
  unsigned int theLastPredict;
  unsigned int theConf;
};  // class ReadersNoConf


struct HistoryEvent {
  HistoryEvent()
    : value(0)
  {}
  HistoryEvent(unsigned int readers)
    : value(readers)
  {}

  void set(bool write, unsigned int node) {
    if(write) {
      value = (1<<16) | (node & 0xf);
    } else {
      value = (1 << node);
    }
  }
  bool valid() {
    return (value != 0);
  }
  bool isWrite() {
    return (value & (1<<16));
  }
  unsigned int writer() {
    DBG_Assert(isWrite());
    return (value & 0xf);
  }
  unsigned int readers() {
    DBG_Assert(!isWrite());
    return (value & 0xffff);
  }
  bool isReader(unsigned int node) {
    DBG_Assert(!isWrite());
    return (value & (1<<node));
  }
  void addReader(unsigned int node) {
    DBG_Assert(!isWrite());
    value |= (1 << node);
  }
  void clear() {
    value = 0;
  }

  int count() {
    int ret = 0;
    unsigned int sharers = value;
    for(int ii = 0; ii < 16; ii++) {
      if(sharers & 0x01) {
        ret++;
      }
      sharers >>= 1;
    }
    return ret;
  }

  unsigned int value;
};


class SignatureMapping32Bit {
public:
  // use at most 32-bit signatures
  typedef Flexus::Core::MemoryAddress_<Flexus::Core::Word32Bit> Signature;

  // we still need the full address for indexing
  typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

  SignatureMapping32Bit(unsigned int histDepth, unsigned int blockAddrBits, unsigned int blockSize)
    : myHistDepth(histDepth)
    , myAddrBits(blockAddrBits)
    , myBlockBits(log_base2(blockSize))
  {}

  Signature encodeHistory(HistoryEvent * histEvents) {
    unsigned int hist = 0;
    unsigned int offset = 0;

    // put the newest history event in the lowest bits
    for(unsigned int ii = 0; ii < myHistDepth; ++ii) {
      if(histEvents[ii].valid()) {
        if(histEvents[ii].isWrite()) {
          DBG_(VVerb, ( << "hist event " << ii << ": writer=" << histEvents[ii].writer() ) );
          hist |= (histEvents[ii].writer() << offset);
          hist |= (1 << (4+offset));
          offset += 5;
        }
        else {
          DBG_(VVerb, ( << "hist event " << ii << ": readers=" << std::hex << histEvents[ii].readers() ) );
          hist |= (histEvents[ii].readers() << offset);
          offset += 17;
        }
      }
    }
    // make sure we haven't gone too far
    DBG_Assert(offset <= 32);
    DBG_(VVerb, ( << "created history trace 0x" << std::hex << hist ) );

    return Signature(hist);
  }

  Signature makeSig(MemoryAddress anAddr, Signature aHistory) {
    unsigned int sig = aHistory;

    // first shift and truncate the address
    unsigned int addr = (anAddr >> myBlockBits);
    addr &= (1<<myAddrBits) - 1;

    // then XOR the address with the history
    sig ^= addr;
    DBG_(VVerb, ( << "from addr 0x" << std::hex << anAddr << " created signature 0x" << sig ) );
    return Signature(sig);
  }

  const unsigned int myHistDepth;
  const unsigned int myAddrBits;
  const unsigned int myBlockBits;
};  // end class SignatureMapping32Bit

class SignatureMapping64Bit {
public:
  // use at most 64-bit signatures
  typedef Flexus::Core::MemoryAddress_<Flexus::Core::Word64Bit> Signature;

  // we still need the full address for indexing
  typedef Flexus::SharedTypes::PhysicalMemoryAddress MemoryAddress;

  SignatureMapping64Bit(unsigned int histDepth, unsigned int blockAddrBits, unsigned int blockSize)
    : myHistDepth(histDepth)
    , myAddrBits(blockAddrBits)
    , myBlockBits(log_base2(blockSize))
  {}

  Signature encodeHistory(HistoryEvent * histEvents) {
    long long hist = 0;
    unsigned int offset = 0;

    // put the newest history event in the lowest bits
    for(unsigned int ii = 0; ii < myHistDepth; ++ii) {
      if(histEvents[ii].valid()) {
        if(histEvents[ii].isWrite()) {
          DBG_(VVerb, ( << "hist event " << ii << ": writer=" << histEvents[ii].writer() ) );
          hist |= ((long long)histEvents[ii].writer() << offset);
          hist |= ((long long)1 << (4+offset));
          offset += 5;
        }
        else {
          DBG_(VVerb, ( << "hist event " << ii << ": readers=" << std::hex << histEvents[ii].readers() ) );
          hist |= ((long long)histEvents[ii].readers() << offset);
          offset += 17;
        }
      }
    }
    // make sure we haven't gone too far
    DBG_Assert(offset <= 64);
    DBG_(VVerb, ( << "created history trace 0x" << std::hex << hist ) );

    return Signature(hist);
  }

  Signature makeSig(MemoryAddress anAddr, Signature aHistory) {
    long long sig = aHistory;

    // first shift and truncate the address
    long long addr = (anAddr >> myBlockBits);
    addr &= ((long long)1 << myAddrBits) - (long long)1;

    // then XOR the address with the history
    sig ^= addr;
    DBG_(VVerb, ( << "from addr 0x" << std::hex << anAddr << " created signature 0x" << sig ) );
    return Signature(sig);
  }

  const unsigned int myHistDepth;
  const unsigned int myAddrBits;
  const unsigned int myBlockBits;
};  // end class SignatureMapping64Bit

template < int N >
struct SignatureMappingN;


template<> struct SignatureMappingN<2> {
  typedef Flexus::Core::Address32Bit AddressLength;
  typedef SignatureMapping32Bit SignatureMapping;
};
template<> struct SignatureMappingN<3> {
  typedef Flexus::Core::Address32Bit AddressLength;
  typedef SignatureMapping32Bit SignatureMapping;
};
template<> struct SignatureMappingN<4> {
  typedef Flexus::Core::Address64Bit AddressLength;
  typedef SignatureMapping64Bit SignatureMapping;
};
template<> struct SignatureMappingN<5> {
  typedef Flexus::Core::Address64Bit AddressLength;
  typedef SignatureMapping64Bit SignatureMapping;
};


}  // end namespace nMrpTable
