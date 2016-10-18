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
#ifndef PULL_STREAM_INCLUDED
#define PULL_STREAM_INCLUDED

#include "common.hpp"
#include "ordermgr.hpp"

#include <boost/enable_shared_from_this.hpp>

struct WindowEntry {
  tAddress address;
  long long sequenceNo;
  bool common;
  WindowEntry(tAddress a, long long b)
    : address(a)
    , sequenceNo(b)
    , common(false)
  {}
};

class PullStream : public SuperStream, public boost::enable_shared_from_this<PullStream> {
  PrefetchReceiver * theReceiver;
  bool theFinalized;
  bool theDead;
  long theInitBackwardSize;
  long theInitForwardSize;
  long theBodyForwardSize;
  long theStreamMaxBlocks;
  long theWindow;
  bool theAggressive;
  bool theUseDelta;

  long theNumOrders;
  long theGoodOrder;
  std::vector< OrderMgr * > theOrders;
  std::vector< long long > theOrigSequenceNos;
  std::vector< long long > theCurrSequenceNos;
  std::vector< std::vector<WindowEntry> > theWindows;
  tAddress theCreatePC;
  tAddress theMissAddress;
  tAddress theFirstHitAddress;
  tAddress theDeltaAddress;

  std::set< tAddress > theIntersection;
  std::set< tAddress > theUnion;

  tAddress thePrev3Address;
  tAddress thePrev2Address;
  tAddress thePrev1Address;

  //Statistics
  int theForwardedBlocks;
  int theHitBlocks;

public:
  PullStream( PrefetchReceiver * aReceiver, OrderMgr * order, long long sequenceNo, tAddress aCreatePC, tAddress aMissAddress );
  virtual ~PullStream ();
  virtual void finalize();

  void add(OrderMgr * order, long long sequenceNo);

  friend std::ostream & operator << ( std::ostream & anOstream, PullStream & aPullStream);

  void setInitBackwardSize(int aSize) { theInitBackwardSize = aSize; }
  void setInitForwardSize(int aSize) { theInitForwardSize = aSize; }
  void setBodyForwardSize(int aSize) { theBodyForwardSize = aSize; }
  void setMaxBlocks(long aNum) { theStreamMaxBlocks = aNum; }
  void setWindow(long aWindow) { theWindow = aWindow; }
  void setAggressive(bool aFlag) { theAggressive = aFlag; }
  void setDelta(bool aFlag) { theUseDelta = aFlag; }

  void beginForward();
  void notifyHit(tAddress anAddress, tVAddress aPC, long aStreamIndex, long long aSequenceNo, long long aHeadSequenceNo, long aCurrBlocks);
  bool notifyMiss(tAddress anAddress, tVAddress aPC);

  int id(long aStreamIndex) const { DBG_Assert(aStreamIndex>=0 && aStreamIndex<theNumOrders); return theOrders[aStreamIndex]->id(); }
  tAddress initialPC() { return theCreatePC; }
  OrderMgr * getOrder(long aStreamIndex) { DBG_Assert(aStreamIndex>=0 && aStreamIndex<theNumOrders); return theOrders[aStreamIndex]; }
  long long firstSeqNo(long aStreamIndex) { return theOrigSequenceNos[aStreamIndex]; }
  long hits() { return theHitBlocks; }
  long forwards() { return theForwardedBlocks; }

private:
  void internalHit(tAddress anAddress, long aStreamIndex, long long aSequenceNo, long long aHeadSequenceNo);
  void forwardBlock(tAddress anAddress, long aStreamIndex, long long aSequenceNo, long long aHeadSequenceNo);
  void forwardAddressFromWindow(tAddress anAddress, long window);
  void forwardGoodStream(long aCurrBlocks, long aForwardSize);
  void forwardWindow(long aCurrBlocks);
  void fillWindow(bool init);
  void compareWindows();
  void advanceWindowPtr(long & order, long & depth, long initial);

};


#endif //PULL_STREAM_INCLUDED
