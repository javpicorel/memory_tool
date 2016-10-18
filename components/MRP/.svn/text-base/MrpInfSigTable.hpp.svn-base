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


template < class Signature >
class PerAddressType {
  typedef std::map<Signature,Readers> HistReadersMap;
  typedef typename HistReadersMap::iterator Iterator;
  typedef std::pair<Signature,Readers> HistReadersPair;
  typedef std::pair<Iterator,bool> InsertPair;

  HistReadersMap theHistoryTraces;

public:
  PerAddressType() {
  }
  PerAddressType(Signature history, Readers readers) {
    add(history, readers);
  }

  Readers * lookup(Signature history) {
    Iterator iter = theHistoryTraces.find(history);
    if(iter != theHistoryTraces.end()) {
      return &(iter->second);
    }
    return 0;
  }

  void add(Signature history, Readers readers) {
    HistReadersPair insert = std::make_pair(history, readers);
    InsertPair result = theHistoryTraces.insert(insert);
    DBG_Assert(result.second);
  }

};  // end class PerAddressType

#ifdef MRP_GLOBAL

template < class SignatureMapping >
class MrpSigTableType {
  typedef typename SignatureMapping::Signature Signature;
  typedef typename SignatureMapping::MemoryAddress MemoryAddress;

public:
  MrpSigTableType(unsigned int numSets,
                  unsigned int assoc)
  {
    // ignore everything - this constructor is just here for
    // compatibility with the non-infinite version
  }

  Readers * lookup(MemoryAddress addr, Signature history) {
    return theTable.lookup(history);
  }

  void add(MemoryAddress addr, Signature history, Readers readers) {
    theTable.add(history,readers);
  }

  friend std::ostream & operator << (std::ostream & anOstream, const MrpSigTableType & aTable) {
    return anOstream << "Infinite entries, global organization";
  }

private:

  PerAddressType<Signature> theTable;

};  // end class MrpSigTableType

#else

template < class SignatureMapping >
class MrpSigTableType {
  typedef typename SignatureMapping::Signature Signature;
  typedef typename SignatureMapping::MemoryAddress MemoryAddress;

public:
  MrpSigTableType(unsigned int numSets,
                  unsigned int assoc)
  {
    // ignore everything - this constructor is just here for
    // compatibility with the non-infinite version
  }

  Readers * lookup(MemoryAddress addr, Signature history) {
    Iterator iter = thePerAddressTables.find(addr);
    if(iter != thePerAddressTables.end()) {
      return iter->second->lookup(history);
    }
    return 0;
  }

  void add(MemoryAddress addr, Signature history, Readers readers) {
    // check if an entry exists for this address
    Iterator iter = thePerAddressTables.find(addr);
    if(iter != thePerAddressTables.end()) {
      // already exists - ask it to add the new history trace
      iter->second->add(history, readers);
    }
    else {
      // doesn't exist - create a per address entry
      PerAddress_p entry = new PerAddress(history, readers);
      PerAddressPair insert = std::make_pair(addr, entry);
      InsertPair result = thePerAddressTables.insert(insert);
      DBG_Assert(result.second);
    }
  }

  friend std::ostream & operator << (std::ostream & anOstream, const MrpSigTableType & aTable) {
    return anOstream << "Infinite entries, per address organization";
  }

private:
  typedef PerAddressType<Signature> PerAddress;
  typedef PerAddress * PerAddress_p;
  typedef std::map<MemoryAddress,PerAddress_p> PerAddressMap;
  typedef typename PerAddressMap::iterator Iterator;
  typedef std::pair<MemoryAddress,PerAddress_p> PerAddressPair;
  typedef std::pair<Iterator,bool> InsertPair;

  PerAddressMap thePerAddressTables;

};  // end class MrpSigTableType
#endif //MRP_GLOBAL


}  // end namespace nMrpTable
