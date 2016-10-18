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

using boost::intrusive_ptr;

template < int N >
struct HistoryEntry : boost::counted_base {
  // NOTE: read accesses go into the current event, whereas
  //       writes go directly into the proper history
  HistoryEntry(const bool write, unsigned int node) {
    DBG_Assert(N > 1);
    if(write) {
      theEvents[0].set(true, node);
    } else {
      theCurrent.set(false, node);
    }
  }

  unsigned int currReaders() {
    return theCurrent.readers();
  }

  int currWriter() {
    if(theCurrent.value != 0) {
      return -1;
    }
    return theEvents[0].writer();
  }

  void pushRead(unsigned int node) {
    if(theCurrent.value != 0) {
      // the current state is reading - add another sharer
      theCurrent.addReader(node);
    }
    else {
      // currently writing - start using the "current" event
      theCurrent.set(false, node);
      // also mark the old writer as a sharer, because it
      // still has a read-only copy of the cache block
      theCurrent.addReader(theEvents[0].writer());
    }
  }
  void pushWrite(unsigned int node) {
    if(theCurrent.value != 0) {
      // the current state is reading - push the "current" into
      // the second-most-recent history entry, the write into
      // the most recent, and everything else moves back two
      for(int ii = N-1; ii > 1; ii--) {
        theEvents[ii] = theEvents[ii-2];
      }
      theEvents[1] = theCurrent;
      theEvents[0].set(true, node);
      theCurrent.clear();
    }
    else {
      // currently writing - push the history events back
      for(int ii = N-1; ii > 0; ii--) {
        theEvents[ii] = theEvents[ii-1];
      }
      theEvents[0].set(true, node);
    }
  }

  HistoryEvent * current() {
    return &theCurrent;
  }

  HistoryEvent theEvents[N];
  HistoryEvent theCurrent;
};  // end struct HistoryEntry


template < int N, class SignatureMapping >
class MrpHistoryTableType {
  typedef typename SignatureMapping::Signature Signature;
  typedef typename SignatureMapping::MemoryAddress MemoryAddress;

private:
  typedef HistoryEntry<N> HistEntry;
  typedef intrusive_ptr<HistEntry> HistEntry_p;
  typedef std::map<MemoryAddress,HistEntry_p> HistMap;
  typedef typename HistMap::iterator Iterator;
  typedef std::pair<MemoryAddress,HistEntry_p> HistPair;
  typedef std::pair<Iterator,bool> InsertPair;

public:
  MrpHistoryTableType(SignatureMapping & aMapper)
    : theSigMapper(aMapper)
  {}

  void readAccess(MemoryAddress anAddr, unsigned int node) {
    DBG_(VVerb, ( << "node " << node << " read addr 0x" << std::hex << anAddr ) );
    Iterator iter = theHistory.find(anAddr);
    if(iter == theHistory.end()) {
      // not in table - start a new history
      HistEntry_p newEntry( new HistEntry(false,node) );
      add(anAddr, newEntry);
    }
    else {
      // the table already contains a history trace for this
      // address - munge the new access in
      iter->second->pushRead(node);
    }
  }

  void writeAccess(MemoryAddress anAddr, unsigned int node) {
    DBG_(VVerb, ( << "node " << node << " write addr 0x" << std::hex << anAddr ) );
    Iterator iter = theHistory.find(anAddr);
    if(iter == theHistory.end()) {
      // not in table - start a new history
      HistEntry_p newEntry( new HistEntry(true,node) );
      add(anAddr, newEntry);
    }
    else {
      // munge the new address into the existing history trace -
      // always push the history back for a write
      iter->second->pushWrite(node);
    }
  }

  Signature makeHistory(MemoryAddress anAddr) {
    DBG_(VVerb, ( << "history for addr 0x" << std::hex << anAddr ) );
    Signature ret(0);
    Iterator iter = theHistory.find(anAddr);
    if(iter != theHistory.end()) {
      // munge the existing history events together
      ret = theSigMapper.encodeHistory(iter->second->theEvents);
    }
    return ret;
  }

  unsigned int currReaders(MemoryAddress anAddr) {
    // if currently writing, returns the list of readers for the previous write
    Iterator iter = theHistory.find(anAddr);
    if(iter != theHistory.end()) {
      return iter->second->currReaders();
    }
    return 0;
  }

  int currWriter(MemoryAddress anAddr) {
    // if currently reading, returns -1; otherwise the writer's node
    Iterator iter = theHistory.find(anAddr);
    if(iter != theHistory.end()) {
      return iter->second->currWriter();
    }
    return -1;
  }


private:
  void add(MemoryAddress anAddr, HistEntry_p anEntry) {
    // try to insert a new entry - if successful, we're done;
    // otherwise abort
    HistPair insert = std::make_pair(anAddr,anEntry);
    InsertPair result = theHistory.insert(insert);
    DBG_Assert(result.second);
  }

  // use a Map to allow fast lookup and flexible size
  HistMap theHistory;
  SignatureMapping & theSigMapper;

};  // end class MrpHistoryTableType


}  // end namespace nMrpTable
