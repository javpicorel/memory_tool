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
#ifndef STRIDE_STREAM_INCLUDED
#define STRIDE_STREAM_INCLUDED

#include "common.hpp"

#include <boost/enable_shared_from_this.hpp>

class StrideStream;
class Strider {
  long myIndex;
  const unsigned int theStreamLength;
  int myStride;
  std::deque<tAddress> thePredictions;

  //Statistics
  int theForwardedBlocks;
  int theHitBlocks;

public:
  Strider(long anIndex, unsigned int aStreamLength);

  long index() const { return myIndex; }
  int stride() const { return myStride; }

  bool locate(tAddress anAddress);
  void advance(tAddress anAddress);
  void predict(StrideStream * aForwarder, tAddress anAddress);
  void reset(long aNewIndex, int aStride);
  void invalidate(tAddress anAddress);
  void hit();

private:
  void forward(StrideStream * aForwarder, tAddress anAddress);
};


class StrideStream : public SuperStream, public boost::enable_shared_from_this<StrideStream> {
  PrefetchReceiver * theReceiver;
  bool theFinalized;
  bool theDead;
  long theMaxStreams;
  long theHistoryDepth;
  long theStreamLength;

  long theNextStrideIndex;
  tAddress * theLastAddresses;
  int * theMruList;
  std::vector< Strider * > theStreams;

public:
  StrideStream( PrefetchReceiver * aReceiver, tAddress aMissAddress, tVAddress aMissPC, long aMaxStreams, long aHistory, long aStreamLength );
  virtual ~StrideStream();
  virtual void finalize();

  friend std::ostream & operator << ( std::ostream & anOstream, StrideStream & aStrideStream);

  void beginForward();
  void notifyHit(tAddress anAddress, tVAddress aPC, long aStrideIndex, long long aSequenceNo, long long aHeadSequenceNo, long aCurrBlocks);
  bool notifyMiss(tAddress anAddress, tVAddress aPC);
  int id(long aStrideIndex) const { return 0; }

  bool forwardBlock(tAddress anAddress, long aStrideIndex);

private:
  void addHistory(tAddress anAddress, tVAddress aPC);
  int victim();
  void accessed(int aStream);
  void consume(tAddress anAddress, tVAddress aPC, bool wasHit);
  void invalidate(tAddress anAddress);

};


#endif //STRIDE_STREAM_INCLUDED
