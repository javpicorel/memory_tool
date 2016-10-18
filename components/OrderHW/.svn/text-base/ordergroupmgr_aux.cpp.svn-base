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

#include <iomanip>
#include <fstream>

#include <boost/throw_exception.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <boost/optional.hpp>
#include <core/boost_extensions/padded_string_cast.hpp>
using boost::padded_string_cast;

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include "common.hpp"
#include "ordergroupmgr.hpp"
#include "ordermgr.hpp"
#include "pull_stream.hpp"
#include "multi_stream.hpp"

#define __STDC_CONSTANT_MACROS
#include <boost/date_time/posix_time/posix_time.hpp>


OrderGroupMgr::~OrderGroupMgr() {
  boost::posix_time::ptime now(boost::posix_time::second_clock::local_time());
  DBG_(Dev, ( << "Destructing order group manager " << theName << " at " << now ) );
  theMostRecentAppends.clear();
  while(!theOrders.empty()) {
    delete theOrders.back();
    theOrders.pop_back();
  }
}

void OrderGroupMgr::finalize() {
  DBG_(Dev, ( << "Finalizing order group manager " << theName ) );
  unsigned int ii;
  for(ii = 0; ii < theOrders.size(); ii++) {
    theOrders[ii]->finalize();
  }
}


#ifndef ENABLE_PC_LOOKUP

void OrderGroupMgr::loadReflector(std::string aDirName, int anId) {
  static std::map< PhysicalMemoryAddress, int > seen;
  static int unique_count = 0;

  reflector_map theReflectorMap;

   std::string fname( aDirName);
   fname += "/" + padded_string_cast<3,'0'>(anId) + "-reflector";
   DBG_(Dev, ( << theName << " load state from " << fname) );
   std::ifstream ifs(fname.c_str(), std::ios::binary);
   boost::archive::binary_iarchive ia(ifs);

   ia >> theReflectorMap;
   // close archive
   ifs.close();


   reflector_map::iterator iter = theReflectorMap.begin();
   while (iter != theReflectorMap.end()) {
     std::map< PhysicalMemoryAddress, int >::iterator seen_iter = seen.find(iter->first);
     if (seen_iter != seen.end()) {
        DBG_( Dev, ( << "Address: " << iter->first << " previously seen on reflector " << seen_iter->second) );
     } else {
       ++unique_count;
     }
     int dir = theDirectory->node( iter->first  );
     if (anId != dir) {
        DBG_( Dev, ( << "Address: " << iter->first << " Page map says " << dir << " but reflector id is " << anId ) );
     }

     seen[iter->first] = anId;

     theMostRecentAppends[ iter->first ].push_back( theOrders[iter->second.theMRC] );
     ++iter;
   }
   DBG_( Dev, ( << theName << " unique addresses: " << unique_count) );
}


struct Info {
  int mrc;
  long mrcLocation;
  int two;
  long twoLocation;
  Info()
   : mrc(-1)
   , mrcLocation(0)
   , two(-1)
   , twoLocation(0)
   {}
};

std::ostream & operator <<( std::ostream & anOstream, Info const & anInfo) {
  anOstream << "mrc: " << anInfo.mrc << " @" << anInfo.mrcLocation << " two: " << anInfo.two <<" @" << anInfo.twoLocation;
  return anOstream;
}

std::map< tAddress, Info> all_mrc_map;


void OrderGroupMgr::saveReflector(std::string aDirName, int anId) {

   std::string fname( aDirName);
   fname += "/" + padded_string_cast<3,'0'>(anId) + "-reflector";
   DBG_(Dev, ( << theName << " save state to" << fname) );
   std::ofstream ofs(fname.c_str(), std::ios::binary);
   boost::archive::binary_oarchive oa(ofs);

   oa << theSaveMaps[anId];
   // close archive
   ofs.close();

   fname = aDirName+ "/" + padded_string_cast<3,'0'>(anId) + "-reflector2";
   DBG_(Dev, ( << theName << " save two state to" << fname) );
   std::ofstream ofs2(fname.c_str(), std::ios::binary);
   boost::archive::binary_oarchive oa2(ofs2);

   oa2 << theSave2Maps[anId];
   // close archive
   ofs2.close();

  if (anId == 15) {
     std::map< tAddress, Info>::iterator iter = all_mrc_map.begin();
     std::map< tAddress, Info>::iterator end = all_mrc_map.end();
     while (iter != end) {
       std::cout << std::hex << iter->first << std::dec << " " << iter->second << std::endl;
       ++iter;
     }
  }
}

void OrderGroupMgr::reflect( tAddress anAddress, int anMRC, long aLocation ) {
  int dir = theDirectory->node( anAddress );
  std::map<tAddress, std::list<OrderMgr*> >::iterator iter = theMostRecentAppends.find(anAddress);

  if ( iter->second.front() == theOrders[anMRC] ) {
    //Record the reflection only if this node is in fact the MRC.
    theSaveMaps[dir].insert( std::make_pair( PhysicalMemoryAddress(anAddress), ReflectorEntry( anMRC, aLocation) ) );
    all_mrc_map[anAddress].mrc = anMRC;
    all_mrc_map[anAddress].mrcLocation = aLocation;
  }
  else {
    std::list<OrderMgr*>::iterator iter2 = iter->second.begin();
    ++iter2;
    if(iter2 != iter->second.end()) {
      if ( *iter2 == theOrders[anMRC] ) {
        theSave2Maps[dir].insert( std::make_pair( PhysicalMemoryAddress(anAddress), ReflectorEntry( anMRC, aLocation) ) );
        all_mrc_map[anAddress].two = anMRC;
         all_mrc_map[anAddress].twoLocation = aLocation;
      }
    }
  }

}

void OrderGroupMgr::save( std::string const & aDirName, Directory * aDirectory ) {
  theDirectory = aDirectory;
  for (int i = 0; i < 16; ++i) {
    theSaveMaps[i].clear();
    theSave2Maps[i].clear();
  }

  for (unsigned int i = 0; i < theOrders.size(); ++i) {
    theOrders[i]->save(aDirName, ll::bind( &OrderGroupMgr::reflect, this, ll::_1, ll::_2, ll::_3) );
  }

  for (int i = 0; i < 16; ++i) {
    saveReflector(aDirName, i);
  }
}

void OrderGroupMgr::reconstructTwo() {
  //For all MRC entries
  std::cerr << "Reconstructing 2MRC for " << theMostRecentAppends.size() << " addresses\n";

  std::map< tAddress, std::list<OrderMgr*> >::iterator iter = theMostRecentAppends.begin();
  std::map< tAddress, std::list<OrderMgr*> >::iterator end = theMostRecentAppends.end();
  while (iter != end) {
    OrderMgr * mro = iter->second.front();
    long long mro_loc = mro->getSequenceNo(iter->first, 0);
    if (mro_loc != 0) {
      OrderMgr * two = 0;
      long long shortest_dist = -1;
      long long two_loc = 0;

      //Get bottom offset for second-most-recent on mro
      long long test_loc = mro->getSequenceNo(iter->first, 1);
      if  (test_loc != 0 ) {
        long long dist = mro->nextSequenceNo() - test_loc;
        two = mro;
        two_loc = test_loc;
        shortest_dist = dist ;
      }

      //Now check most-recent on all the other orders
      for (unsigned int i = 0; i < theOrders.size(); ++i) {
        //Don't check on the mro again, as we did this above
        if (theOrders[i] != mro) {
          test_loc = theOrders[i]->getSequenceNo(iter->first, 0);
          if  (test_loc != 0 ) {
            //The address is present on this order
            long long dist = theOrders[i]->nextSequenceNo() - test_loc;
            if (shortest_dist < 0 || dist < shortest_dist) {
              two = theOrders[i];
              shortest_dist = dist ;
            }
          }
        }
      }

      //Record two into theMostRecentAppends, bucket #2 for the address
      if (shortest_dist > 0) {
        DBG_Assert( two );
        iter->second.push_back(two);
        //std::cout << std::hex << iter->first << std::dec << " MRC[" << mro->id() << "] @" << mro_loc << " 2MRC[" << two->id() << "] @" << two_loc << " (+" << shortest_dist <<")\n";
      }

    }

    ++iter;
  }

  std::cerr << "Reconstruction complete.\n";

}

#endif //!ENABLE_PC_LOOKUP


struct AddStreamEntry {
  OrderMgr * mgr;
  unsigned int lookBack;
  long long seq;
  AddStreamEntry(OrderMgr * aMgr, unsigned int aLookBack)
    : mgr(aMgr)
    , lookBack(aLookBack)
    , seq(0)
  {}
  AddStreamEntry(OrderMgr * aMgr, unsigned int aLookBack, long long aSeq)
    : mgr(aMgr)
    , lookBack(aLookBack)
    , seq(aSeq)
  {}
};

boost::shared_ptr<PullStream> OrderGroupMgr::pullRequest(PrefetchReceiver * aReceiver, tAddress aRequestedAddress, tAddress aRequestPC) {
  std::list<AddStreamEntry> streams;

#ifndef ENABLE_BO_LOOKUP
    aRequestedAddress = aRequestedAddress & theBlockMask;
#endif //ENABLE_SPATIAL_LOOKUP

  // generate the order/lookback pairs
  if(theNumOrders > 1) {
    mrc_table_t::iterator iter = findMR( aRequestedAddress, aRequestPC );
    if (iter != theMostRecentAppends.end()) {
      int ii;

      // count of the number of occurences of each order
      std::map<OrderMgr*,int> counts;

      std::list<OrderMgr*>::iterator iter2 = iter->second.begin();
      for(ii = 0; ii < theNumRecent; ii++) {
        if(iter2 == iter->second.end()) {
          break;
        }
        streams.push_back( AddStreamEntry(*iter2,counts[*iter2]) );
        counts[*iter2]++;
        ++iter2;
      }

    }
  }
  else {
    if(theDeltas > 0) {
      #ifdef ENABLE_DELTAS
        tAddress delta = aRequestedAddress - theOrders[0]->mostRecentAppend();
        delta_table::index<by_delta>::type::iterator iter = theDeltaIndex.get<by_delta>().find(delta);
        if(iter != theDeltaIndex.get<by_delta>().end()) {
          streams.push_back( AddStreamEntry(theOrders[0],0,iter->theSequenceNo) );
        }
      #endif //ENABLE_DELTAS
    }
    else {
      for(int ii = 0; ii < theNumRecent; ii++) {
        streams.push_back( AddStreamEntry(theOrders[0],ii) );
      }
    }
  }

  // peform the lookup on each order
    if(theDeltas == 0) {
      for(std::list<AddStreamEntry>::iterator iter = streams.begin(); iter != streams.end(); ++iter) {
        #ifdef ENABLE_PC_LOOKUP
          iter->seq = (iter->mgr)->getSequenceNo(aRequestedAddress, aRequestPC, iter->lookBack);
        #else //!ENABLE_PC_LOOKUP
          iter->seq = (iter->mgr)->getSequenceNo(aRequestedAddress, iter->lookBack);
        #endif //ENABLE_PC_LOOKUP
      }
    }

  // look at the most recent to determine if we can start a stream
  if( !streams.empty() && (streams.front().seq > 0) ) {
    std::list<AddStreamEntry>::iterator iter = streams.begin();
    boost::shared_ptr<PullStream> stream( new PullStream(aReceiver, iter->mgr, iter->seq, aRequestPC, aRequestedAddress) );
    while( (++iter) != streams.end() ) {
      if(iter->seq > 0) {
        stream->add(iter->mgr, iter->seq);
      }
    }
    stream->setDelta( (theDeltas > 0) );
    return stream;
  }

  //Could not create stream
  return boost::shared_ptr<PullStream>();
}
