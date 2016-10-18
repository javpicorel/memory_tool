// DO-NOT-REMOVE begin-copyright-block 
//                                     
// Redistributions of any form whatsoever must retain and/or include the     
// following acknowledgment, notices and disclaimer:                         
//                                                                           
// This product includes software developed by Carnegie Mellon University.   
//                                                                           
// Copyright 2006 by Eric Chung, Brian Gold, Nikos Hardavellas, Jangwoo Kim, 
// Ippokratis Pandis, Minglong Shao, Jared Smolens, Stephen Somogyi,         
// Tom Wenisch, Anastassia Ailamaki, Babak Falsafi and James C. Hoe for      
// the SimFlex Project, Computer Architecture Lab at Carnegie Mellon,        
// Carnegie Mellon University.                                               
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
#ifndef _NS_REGRESS_HPP_
#define _NS_REGRESS_HPP_

#include "netcontainer.hpp"
#include "netcommon.hpp"
#include <stdlib.h>
#include <fstream>

using namespace std;
using namespace nNetShim;

extern nNetShim::NetContainer * nc;

extern bool ( * timeStep ) ( void );
extern int errorCount;
extern int localPendingMsgs;

bool timeStepDefault ( void );
bool waitUntilEmpty ( void );
bool deliverMessage ( const MessageState * msg );
bool isNodeAvailable( const int node, const int vc );
bool dumpStats ( void );
bool dumpCSVStats ( void );
bool runRegressSuite ( void );
bool idleNetwork ( int length );
bool reinitNetwork ( void );
bool singlePacket1 ( int src, int dest );
bool floodNode ( int numNodes, int numMessages, int targetNode );
bool worstCaseLatency ( int srcNode, int destNode );
bool uniformRandomTraffic ( int numNodes, int totalMessages, float intensity );
bool uniformRandomTraffic2 ( int numNodes, int totalMessages, int hopLatency, float intensity );
bool poissonRandomTraffic ( long long totalCycles, int numNodes, int hopLatency, float intensity, bool usePriority0 = true );


#define REGRESS_ERROR(MSG) do { cout << "REGRESSION ERROR at cycle " << currTime << "  IN: " << __FUNCTION__ << ", " << __FILE__ << ":" << __LINE__ << ": " << MSG << endl;  return true;} while ( 0 )

#define INSERT(MSG) do { if ( nc->insertMessage ( (MSG) ) ) REGRESS_ERROR ( " inserting message" ); } while ( 0 )

#define TRY_TEST(T)  do { if ( reinitNetwork() ) return true; \
                           cout << "\n*** REGRESSION: "#T << endl;\
                           if ( T ) { cout << "*** TEST "#T << " FAILED" << endl; \
                              errorCount++; }\
                            else { \
                              cout << "*** Test succeeded: "; dumpStats(); dumpCSVStats(); } \
                      } while ( 0 )

#ifdef LOG_RESULTS
extern ofstream outFile;
extern long long totalLatency;
extern long long messagesDelivered;
#endif

#endif /* _NS_REGRESS_HPP_ */

