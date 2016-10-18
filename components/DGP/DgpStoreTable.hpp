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


class DgpStoreTable {
  typedef SignatureMapping::MemoryAddress MemoryAddress;

  typedef std::map<MemoryAddress,MemoryAddress> StoreMap;   // <addr,pc>
  typedef StoreMap::iterator Iterator;
  typedef std::pair<MemoryAddress,MemoryAddress> StorePair;
  typedef std::pair<Iterator,bool> InsertPair;

public:

  MemoryAddress retrievePC(MemoryAddress anAddr) {
    Iterator iter = theStores.find(anAddr);
    DBG_Assert(iter != theStores.end(), ( << "no PC entry for addr " << std::hex << anAddr ) );
    theStores.erase(iter);
    return iter->second;
  }

  void insertStore(MemoryAddress anAddr, MemoryAddress aPC) {
    // try to insert a new entry - if successful, we're done;
    // otherwise abort
    StorePair insert = std::make_pair(anAddr, aPC);
    InsertPair result = theStores.insert(insert);
    DBG_Assert(result.second);
  }

private:
  // use a Map to allow fast lookup (this is intended to be a small
  // fully-associative structure)
  StoreMap theStores;

};  // end class DgpStoreTable


}  // end namespace nDgpTable
