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
#ifndef _NS_NETSWITCH_HPP_
#define _NS_NETSWITCH_HPP_

#include "channel.hpp"

namespace nNetShim
{

  // Forward declaration
class NetSwitch;

class NetSwitchInternalBuffer
{
public:

  NetSwitchInternalBuffer ( const int   bufferCount_,
                            NetSwitch * netSwitch_ );

public:

  inline bool isFull ( const int vc ) const
  {
    return ( buffersUsed[vc] >= bufferCount[vc] );
  }

  // Nobody calls this function, so far, so it can be slow
  bool hasMessage ( void ) const
  {
    int
      i;

    for ( i = 0; i < MAX_VC; i++ ) {
      if ( buffersUsed[i] > 0 ) {
        return true;
      }
    }

    return false;
  }

  bool insertMessage ( MessageState * msg );

  bool gotoFirstMessage ( const int vc )
  {
    currPriority = vc;
    currMessage = ageBufferHead[vc];
    return false;
  }

  inline bool getMessage ( MessageState *& msg )
  {
    msg = NULL;

    if ( currMessage == NULL )
      return true;

    msg = currMessage->msg;
    return false;
  }

  bool nextMessage ( void )
  {
    if ( currMessage == NULL )
      return true;

    currMessage = currMessage->next;

    if ( currMessage == NULL )
      return true;

    return false;
  }

  bool removeMessage ( void );

  bool dumpState ( ostream & out );

private:

  int
    bufferCount[MAX_VC],
    buffersUsed[MAX_VC];

  MessageStateList
    * ageBufferHead[MAX_VC],
    * ageBufferTail[MAX_VC],
    * currMessage;

  int
    currPriority;

  NetSwitch
    * netSwitch;
};


class NetSwitch
{
public:

  NetSwitch ( const int name_,      // Name/id of the switch
              const int numNodes_,  // Number of nodes in the system
              const int numPorts_,  // Number of ports/bidirectional channels
              const int inputBufferDepth_,
              const int outputBufferDepth_,
              const int vcBufferDepth_,
              const int crossbarBandwidth_,
              const int channelLatency_ );
  virtual ~NetSwitch () {}

public:

  bool drive ( void );
  bool driveInputPorts ( void );

  bool attachChannel ( Channel   * channel,
                       const int   port,
                       const bool  isInput );

  bool checkTopology ( void ) const;
  bool checkRoutingTable ( void ) const;

  bool addRoutingEntry ( const int destNode,
                         const int outPort,
                         const int outVC );

  bool dumpState ( ostream & out, const int flags );

  // These ports give the minimum delay possible - reserved for the local node
  bool setLocalDelayOnly ( const int port );

  bool notifyWaitingMessage ( const int vc )
  {
    messagesWaiting[vc]++;
    return false;
  }

protected:

  virtual bool routingPolicy ( MessageState * msg );

  bool sendMessageToOutput ( MessageState * msg );

protected:

  int
  name,
    numNodes,
    numPorts,
    inputBufferDepth,
    outputBufferDepth,
    vcBufferDepth,
    crossbarBandwidth,
    nextStartingPort;  // To fairly weight ports, we start arbitration RR

  ChannelInputPortP * inputPorts;
  ChannelOutputPortP * outputPorts;

  NetSwitchInternalBuffer
    * internalBuffer;

  int
    ** routingTable,
    ** vcTable;

  int
    messagesWaiting[MAX_VC];

};

typedef NetSwitch * NetSwitchP;

} // namespace nNetShim

#endif /* _NS_NETSWITCH_HPP_ */
