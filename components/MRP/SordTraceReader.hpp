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
#include <queue>

namespace nSordTraceReader {

typedef unsigned long Time;
typedef long long Address;

struct ProdEntry {
  Address address;
  Address pc;
  Time time;
  char priv;
} __attribute__ ((packed)) ;

struct ConsEntry {
  Address address;
  Address pc;
  Time consTime;
  char producer;
  Time prodTime;
  char priv;
} __attribute__ ((packed)) ;

struct SordTraceEntry {
  SordTraceEntry()
  {}
  SordTraceEntry(long long anAddr, int aNode, Time aTime)
    : production(true)
    , address(anAddr)
    , node(aNode)
    , time(aTime)
  {}
  SordTraceEntry(long long anAddr, int aNode, Time aTime, int pNode, Time pTime)
    : production(false)
    , address(anAddr)
    , node(aNode)
    , time(aTime)
    , producer(pNode)
    , prodTime(pTime)
  {}

  bool production;    // true=>production  false=>consumption
  Address address;    // address
  int node;           // node of production or consumption
  Time time;          // time of production or consumption
  int producer;       // producer node - only valid for consumptions
  Time prodTime;      // production time - only valid for consumptions

  friend std::ostream & operator << (std::ostream & anOstream, const SordTraceEntry & anEntry) {
    if(anEntry.production) {
      anOstream << "production";
    } else {
      anOstream << "consumption";
    }
    anOstream << " address=" << std::hex << anEntry.address
              << " node=" << anEntry.node
              << " time=" << anEntry.time;
    if(!anEntry.production) {
      anOstream << " producer=" << anEntry.producer
                << " prodTime=" << anEntry.prodTime;
    }
    return anOstream;
  }

};
bool operator < (SordTraceEntry const & left, SordTraceEntry const & right) {
  if(left.time != right.time) {
    return left.time > right.time;
  }
  return left.node > right.node;
}

const int NumToRead = 10;

class SordTraceReader {
  FILE* myProdFiles[HACK_WIDTH];
  FILE* myConsFiles[HACK_WIDTH];
  int   myProdPending[HACK_WIDTH];
  int   myConsPending[HACK_WIDTH];
  bool  myAllPending;

  ProdEntry myProdBuffer[NumToRead];
  ConsEntry myConsBuffer[NumToRead];

  std::priority_queue<SordTraceEntry> theQueue;

public:
  SordTraceReader()
  {}

  void init() {
    char buf[32];
    for(int ii = 0; ii < HACK_WIDTH; ii++) {
      sprintf(buf, "fixed_producer%d.sordtrace.0", ii);
      myProdFiles[ii] = fopen(buf, "r");
      DBG_Assert(myProdFiles[ii], ( << "could not open file \"" << buf << "\"." ) );
      sprintf(buf, "consumer%d.sordtrace.0", ii);
      myConsFiles[ii] = fopen(buf, "r");
      DBG_Assert(myConsFiles[ii], ( << "could not open file \"" << buf << "\"." ) );
      myProdPending[ii] = 0;
      myConsPending[ii] = 0;
    }
    myAllPending = false;
  }

  void read() {
    int ii, jj;
    int num;

    // first try the productions
    for(ii = 0; ii < HACK_WIDTH; ii++) {
      if(myProdPending[ii] == 0) {
        num = fread(myProdBuffer, sizeof(ProdEntry), NumToRead, myProdFiles[ii]);
        for(jj = 0; jj < num; jj++) {
          SordTraceEntry newEntry(myProdBuffer[jj].address, ii, myProdBuffer[jj].time);
          DBG_(VVerb, ( << "read prod file " << ii << ": " << newEntry ) );
          theQueue.push(newEntry);
          myProdPending[ii]++;
        }
        if(num == 0) {
          DBG_(Crit, ( << "end of production file for node " << ii ) );
          throw Flexus::Core::FlexusException();
        }
      }
    }

    // now the consumptions
    for(ii = 0; ii < HACK_WIDTH; ii++) {
      if(myConsPending[ii] == 0) {
        num = fread(myConsBuffer, sizeof(ConsEntry), NumToRead, myConsFiles[ii]);
        for(jj = 0; jj < num; jj++) {
          SordTraceEntry newEntry(myConsBuffer[jj].address, ii, myConsBuffer[jj].consTime,
                                  myConsBuffer[jj].producer, myConsBuffer[jj].prodTime);
          DBG_(VVerb, ( << "read cons file " << ii << ": " << newEntry ) );
          theQueue.push(newEntry);
          myConsPending[ii]++;
        }
        if(num == 0) {
          DBG_(Crit, ( << "end of consumption file for node " << ii ) );
          throw Flexus::Core::FlexusException();
        }
      }
    }

    // there must now be something oustanding from every file
    myAllPending = true;
  }  // end read()

  void next(SordTraceEntry & entry) {
    // read from disk until at least one entry from every file is outstanding
    while(!myAllPending) {
      read();
    }

    entry = theQueue.top();
    DBG_(VVerb, ( << "next entry: " << entry ) );
    //SordTraceEntry tester = theQueue.top();
    theQueue.pop();
    if(entry.production) {
      myProdPending[entry.node]--;
      if(myProdPending[entry.node] == 0) {
        myAllPending = false;
      }
    } else {
      myConsPending[entry.node]--;
      if(myConsPending[entry.node] == 0) {
        myAllPending = false;
      }
    }
  }

};  // end class SordTraceReader

}  // end namespace nSordTraceReader
