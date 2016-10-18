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


class PerAddress {
  typedef SignatureMapping::Signature Signature;

  typedef std::map<Signature,Confidence> HistConfMap;
  typedef HistConfMap::iterator Iterator;
  typedef std::pair<Signature,Confidence> HistConfPair;
  typedef std::pair<Iterator,bool> InsertPair;

  HistConfMap theHistoryTraces;

public:
  PerAddress() {}

  PerAddress(Signature history) {
    add(history);
  }

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & theHistoryTraces;
  }

  Confidence * lookup(Signature history) {
    Iterator iter = theHistoryTraces.find(history);
    if(iter != theHistoryTraces.end()) {
      return &(iter->second);
    }
    return 0;
  }

  void add(Signature history) {
    HistConfPair insert = std::make_pair(history, Confidence());
    InsertPair result = theHistoryTraces.insert(insert);
    DBG_Assert(result.second);
  }

  void remove(Signature history) {
    Iterator iter = theHistoryTraces.find(history);
    DBG_Assert(iter != theHistoryTraces.end());
    theHistoryTraces.erase(iter);
  }

  void finalize(DgpStats * stats) {
    int max = 0;
    Iterator iter;

    for(iter = theHistoryTraces.begin(); iter != theHistoryTraces.end(); ++iter) {
      if(iter->second.isFlipping()) {
        stats->SigFlipflop++;
      }
#ifdef DGP_SUBTRACE
      if(!iter->second.isSubtrace()) {
        stats->NumSubtraces << std::make_pair(iter->second.numSubtraces(),1);
        if(iter->second.numSubtraces() > max) {
          max = iter->second.numSubtraces();
        }
      }
    }

    max = (int)( 0.95 * (double)max );
    for(iter = theHistoryTraces.begin(); iter != theHistoryTraces.end(); ++iter) {
      if(iter->second.numSubtraces() >= max) {
        unsigned int ii;
        for(ii = 0; ii < iter->second.numPCseq.size(); ii++) {
          std::cout << "_" << iter->second.numPCseq[ii];
        }
        std::cout << std::endl;
      }
#endif
    }
  }

};  // end class PerAddress

#ifdef DGP_PER_ADDRESS
/************************ Per-address Signature table ************************/

class DgpSigTable {
  typedef SignatureMapping::Signature Signature;
  typedef SignatureMapping::MemoryAddress MemoryAddress;

public:
  DgpSigTable(unsigned int numSets,
              unsigned int assoc)
  {
    // ignore everything - this constructor is just here for
    // compatibility with the non-infinite version
  }

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & thePerAddressTables;
  }

  Confidence * lookup(MemoryAddress addr, Signature sig) {
    Iterator iter = thePerAddressTables.find(addr);
    if(iter != thePerAddressTables.end()) {
      return iter->second->lookup(sig);
    }
    return 0;
  }

  void add(MemoryAddress addr, Signature sig) {
    // check if an entry exists for this address
    Iterator iter = thePerAddressTables.find(addr);
    if(iter != thePerAddressTables.end()) {
      // already exists - ask it to add the new history trace
      iter->second->add(sig);
    }
    else {
      // doesn't exist - create a per address entry
      PerAddress_p entry = new PerAddress(sig);
      PerAddressPair insert = std::make_pair(addr, entry);
      InsertPair result = thePerAddressTables.insert(insert);
      DBG_Assert(result.second);
    }
  }

  void remove(MemoryAddress addr, Signature sig) {
    Iterator iter = thePerAddressTables.find(addr);
    DBG_Assert(iter != thePerAddressTables.end());
    iter->second->remove(sig);
  }

  void finalize(DgpStats * stats) {
    Iterator iter = thePerAddressTables.begin();
    while(iter != thePerAddressTables.end()) {
      iter->second->finalize(stats);
      ++iter;
    }
  }

  friend std::ostream & operator << (std::ostream & anOstream, const DgpSigTable & aTable) {
    return anOstream << "Infinite entries, per address organization";
  }

private:
  //typedef intrusive_ptr<PerAddress> PerAddress_p;
  typedef PerAddress * PerAddress_p;
  typedef std::map<MemoryAddress,PerAddress_p> PerAddressMap;
  typedef PerAddressMap::iterator Iterator;
  typedef std::pair<MemoryAddress,PerAddress_p> PerAddressPair;
  typedef std::pair<Iterator,bool> InsertPair;

  PerAddressMap thePerAddressTables;

};  // end class DgpSigTable

#else
/************************ Global Signature table ************************/

class DgpSigTable {
  typedef SignatureMapping::Signature Signature;
  typedef SignatureMapping::MemoryAddress MemoryAddress;

public:
  DgpSigTable(unsigned int numSets,
              unsigned int assoc)
  {
    // ignore everything - this constructor is just here for
    // compatibility with the non-infinite version
  }

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & theTable;
  }

  Confidence * lookup(MemoryAddress addr, Signature sig) {
    return theTable.lookup(sig);
  }

  void add(MemoryAddress addr, Signature sig) {
    // check if an entry exists for this address
    theTable.add(sig);
  }

  void remove(MemoryAddress addr, Signature sig) {
    theTable.remove(sig);
  }

  void finalize(DgpStats * stats) {
    theTable.finalize(stats);
  }

  friend std::ostream & operator << (std::ostream & anOstream, const DgpSigTable & aTable) {
    return anOstream << "Infinite entries, global organization";
  }

private:

  PerAddress theTable;
};  // end class DgpSigTable

#endif

}  // end namespace nDgpTable
