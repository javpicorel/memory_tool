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
#ifndef MULTI_STREAM_INCLUDED
#define MULTI_STREAM_INCLUDED

#include "common.hpp"
#include "ordermgr.hpp"
#include "stream_tracker.hpp"

#include <boost/enable_shared_from_this.hpp>


class MultiStream : public boost::enable_shared_from_this<MultiStream> {
  MultiReceiver * theReceiver;
  int theId;

  long theTargetBlocks;
  long theCachedBlocks;

  tAddress theMissAddress;
#ifdef ENABLE_PC_TRACKING
  unsigned long theMissPC;
#endif //ENABLE_PC_TRACKING

  OrderMgr * theMRCOrder;
  long long theMRCForward;

#ifdef ENABLE_2MRC
  OrderMgr * the2MRCOrder;
  long long the2MRCForward;
#endif //ENABLE_2MRC

#ifdef ENABLE_PC_TRACKING
  unsigned long theMRCPC;
#endif //ENABLE_PC_TRACKING

#ifdef ENABLE_ALTERNATIVE_SEARCH
  std::list< std::pair< OrderMgr::iterator, OrderMgr::iterator> > theAltLocations;
#endif //ENABLE_ALTERNATIVE_SEARCH
  eStreamType theStreamType;
  bool theMismatchPending;

  unsigned long theHits;
  unsigned long theDiscards;

#ifdef ENABLE_STREAM_TRACKING
  stream_t theStream;
#endif

public:
  MultiStream( MultiReceiver * aReceiver, tAddress aMissAddress, unsigned long aMissPC );
  void setTargetBlocks(int aSize) { theTargetBlocks = aSize; }

  virtual ~MultiStream();
  virtual void finalize();

  friend std::ostream & operator << ( std::ostream & anOstream, MultiStream & aMultiStream);

  void setMRC(OrderMgr * order, long long sequenceNo);
#ifdef ENABLE_2MRC
  void set2MRC(OrderMgr * order, long long sequenceNo);
#endif //ENABLE_2MRC
  void addAltLocation( std::pair< OrderMgr::iterator, OrderMgr::iterator > range );


  void beginForward();

  void notifyInvalidate(tAddress anAddress, eForwardType aFwdType);
  void notifyReplace(tAddress anAddress, eForwardType aFwdType);
  void notifyHit(tAddress anAddress, eForwardType aFwdType);

  int id() const { return theId; }

private:
  bool forwardBlock(tAddress anAddress, unsigned long aPC, eForwardType aFwdType);
  void doForward();
  void doMRCForward();
  void doAltForward();

};


#endif //MULTI_STREAM_INCLUDED
