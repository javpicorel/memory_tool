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
#include "page_map.hpp"

#include <core/boost_extensions/padded_string_cast.hpp>
using boost::padded_string_cast;

#include <fstream>

PageMap::PageMap(long aNumNodes, long aPageSize)
  : theNumNodes(aNumNodes)
  , thePageSize(aPageSize)
{
  //Ensure that PageSize is a non-zero power of 2
  DBG_Assert(thePageSize != 0);
  DBG_Assert(((thePageSize - 1) & thePageSize) == 0);
  DBG_Assert(((theNumNodes - 1) & theNumNodes) == 0);

  int temp = thePageSize;
  int page_size_log2 = 0;
  while (temp) {
    ++page_size_log2;
    temp >>= 1;
  }
  --page_size_log2;

  theNode_shift = page_size_log2;
  theNode_mask = (theNumNodes - 1);

  for (int i = 0; i < theNumNodes; i++) {
    thePageCounts.push_back(new StatCounter(std::string(padded_string_cast<3,'0'>(i) + "-memory-Pages")));
  }

  readPageMap();

  theHomeMapFile.reset( new std::ofstream("page_map.out", std::ios::out ) );
  HomeMap::iterator iter = theHomeMap.begin();
  while (iter != theHomeMap.end()) {
    (*theHomeMapFile) << iter->second << " " << static_cast<long long>(iter->first) << "\n";
    ++iter;
  }
  theHomeMapFile->flush();
}

PageMap::~PageMap() {}

void PageMap::readPageMap() {
  std::ifstream in("page_map.in");
  if (in) {
    DBG_(Verb, ( << "Page map file page_map.in found.  Reading contents...") );

    int count = 0;
    while (in) {
      int node;
      long long addr;
      in >> node;
      in >> addr;
      if (in.good()) {
        DBG_(Verb, ( << "Page " << addr << " assigned to node " << node ) );
        HomeMap::iterator ignored;
        bool is_new;
        boost::tie(ignored, is_new) = theHomeMap.insert( std::make_pair(addr,node) );
        if (! is_new ) {
          DBG_(Dev, ( << "Page " << addr << " previously assigned to node " << ignored->second << " will not be reassigned to " << node) );
        }
        ++count;
      }
    }

    DBG_(Dev, ( << "Assigned " << count << " pages from page_map.in"));
  } else {
    DBG_(Dev, ( << "Page map file page_map.in was not found.") );
  }
}

tID PageMap::newPage(tAddress const & aPageAddr, const tID aRequestingNode) {
  tID node = aRequestingNode;
  DBG_Assert(node < theNumNodes);

  (*thePageCounts[node])++;
  std::pair<HomeMap::iterator,bool> insert_result = theHomeMap.insert( std::make_pair(aPageAddr,node) );
  DBG_Assert(insert_result.second);

  (*theHomeMapFile) << node << " " << static_cast<long long>(aPageAddr) << "\n";
  theHomeMapFile->flush();

  return node;
}
tID PageMap::node(tAddress const & anAddress, const tID aRequestingNode) {
  tAddress pageAddr = tAddress(anAddress >> theNode_shift);

  HomeMap::iterator iter = theHomeMap.find(pageAddr);
  if(iter != theHomeMap.end()) {
    // this is not the first touch - we know which node this page belongs to
    DBG_Assert(iter->second < theNumNodes);
    return iter->second ;
  } else {
    return newPage(pageAddr, aRequestingNode);
  }
}

tID PageMap::newPage(tAddress const & aPageAddr) {
  tID node = static_cast<tID>( (aPageAddr) & theNode_mask );
  DBG_Assert(node < theNumNodes);

  (*thePageCounts[node])++;
  std::pair<HomeMap::iterator,bool> insert_result = theHomeMap.insert( std::make_pair(aPageAddr,node) );
  DBG_Assert(insert_result.second);

  (*theHomeMapFile) << node << " " << static_cast<long long>(aPageAddr) << "\n";
  theHomeMapFile->flush();

  return node;
}
tID PageMap::node(tAddress const & anAddress) {
  tAddress pageAddr = tAddress(anAddress >> theNode_shift);

  HomeMap::iterator iter = theHomeMap.find(pageAddr);
  if(iter != theHomeMap.end()) {
    // this is not the first touch - we know which node this page belongs to
    DBG_Assert(iter->second < theNumNodes);
    return iter->second ;
  } else {
    return newPage(pageAddr);
  }
}
