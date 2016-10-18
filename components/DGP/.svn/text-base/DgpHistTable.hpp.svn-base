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

using boost::intrusive_ptr;


class DgpHistoryTable {
public:
  typedef SignatureMapping::Signature Signature;
  typedef SignatureMapping::MemoryAddress MemoryAddress;

  class HistoryEntry : public boost::counted_base {
  public:
    HistoryEntry() {} //For Serialization
    HistoryEntry(Signature aHist, MemoryAddress aPC)
      : theHist(aHist)
      , theFirstPC(aPC)
      , theLastPC(aPC)
      , theNumPCs(1)
    {}
    Signature theHist;
    MemoryAddress theFirstPC;
    MemoryAddress theLastPC;
    int theNumPCs;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & theHist;
    }

    friend std::ostream & operator << (std::ostream & os, const HistoryEntry & aHist) {
      os << aHist.theHist;
      return os;
    }

#ifdef DGP_SUBTRACE

    std::set<Confidence*> theSubtraces;

    void addSubtraces(Confidence * conf) {
      theSubtraces.insert(conf);
      std::set<Confidence*>::iterator iter = conf->tracePtrs.begin();
      while(iter != conf->tracePtrs.end()) {
        theSubtraces.insert(*iter);
        ++iter;
      }
    }
#endif
  };

private:
  typedef intrusive_ptr<HistoryEntry> HistEntry_p;
  typedef std::map<MemoryAddress,HistEntry_p> HistMap;
  typedef HistMap::iterator Iterator;
  typedef HistMap::const_iterator ConstIterator;
  typedef std::pair<MemoryAddress,HistEntry_p> HistPair;
  typedef std::pair<Iterator,bool> InsertPair;
  
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
        ar & theHistory;
  }

public:
  DgpHistoryTable() {}

  void reset() {
    theHistory.clear();
    theSigMapper = NULL;
  }

  void setSigMapper(SignatureMapping * aMapper) {
    theSigMapper = aMapper;
  }

  friend std::ostream & operator << (std::ostream & os, const DgpHistoryTable & aTable) {
    for ( ConstIterator it = aTable.theHistory.begin(); it != aTable.theHistory.end(); ++it) {
      os << it->first << ": " << *(it->second) << std::endl;
    }
    return os;
  }

  HistEntry_p find(MemoryAddress anAddr) {
    Iterator iter = theHistory.find(anAddr);
    if(iter != theHistory.end()) {
      return iter->second;
    }
    return HistEntry_p(0);
  }

  void remove(MemoryAddress anAddr) {
    Iterator iter = theHistory.find(anAddr);
    if(iter != theHistory.end()) {
      theHistory.erase(iter);
    }
  }

  HistEntry_p findAndRemove(MemoryAddress anAddr) {
    Iterator iter = theHistory.find(anAddr);
    if(iter != theHistory.end()) {
      HistEntry_p temp = iter->second;
      theHistory.erase(iter);
      return temp;
    }
    return HistEntry_p(0);
  }

  void add(MemoryAddress anAddr, HistEntry_p anEntry) {
    // try to insert a new entry - if successful, we're done;
    // otherwise abort
    HistPair insert = std::make_pair(anAddr,anEntry);
    InsertPair result = theHistory.insert(insert);
    DBG_Assert(result.second);
  }

  Signature access(MemoryAddress anAddr, MemoryAddress aPC) {
    Signature ret;
    Iterator iter = theHistory.find(anAddr);
    if(iter == theHistory.end()) {
      // not in table - start a new trace
      ret = theSigMapper->makeHist(aPC);
      HistoryEntry * entry = new HistoryEntry(ret, aPC);
      DBG_Assert( entry != 0);
      add(anAddr, entry);
    }
    else {
      // the table already contains a history trace for this
      // address - munge the new PC in
      ret = theSigMapper->updateHist(iter->second->theHist, aPC);
      iter->second->theHist = ret;
      iter->second->theLastPC = aPC;
      iter->second->theNumPCs++;
    }
    return ret;
  }

private:
  // use a Map to allow fast lookup and flexible size
  HistMap theHistory;
  SignatureMapping * theSigMapper;

};  // end class DgpHistoryTable


}  // end namespace nDgpTable
