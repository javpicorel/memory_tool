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
#include "netswitch.hpp"

namespace nNetShim
{

  NetSwitch::NetSwitch  ( const int name_,      // Name/id of the switch
                          const int numNodes_,  // Number of nodes in the system
                          const int numPorts_,  // Number of ports/bidirectional channels on the sw
                          const int inputBufferDepth_, 
                          const int outputBufferDepth_,
                          const int vcBufferDepth_,
                          const int crossbarBandwidth_, 
                          const int channelLatency_ ) :
    name              ( name_ ),
    numNodes          ( numNodes_ ),
    numPorts          ( numPorts_ ),
    inputBufferDepth  ( inputBufferDepth_ ),
    outputBufferDepth ( outputBufferDepth_ ),
    vcBufferDepth     ( vcBufferDepth_ ),
    crossbarBandwidth ( crossbarBandwidth_ ),
    nextStartingPort  ( 0 )
  {  
    int
      i,
      j;

    // Check basic assumptions for switch parameters
    assert ( numNodes > 0 );
    assert ( numPorts > 0 );
    assert ( inputBufferDepth > 0 );
    assert ( outputBufferDepth > 0 );
    assert ( vcBufferDepth >= 0 );
    assert ( crossbarBandwidth != 0 );

    // Initialize all buffers and input/output ports
    inputPorts      = new ChannelInputPortP[numPorts];
    outputPorts     = new ChannelOutputPortP[numPorts];
      
    for ( i = 0; i < numPorts; i++ ) {
      inputPorts[i]  = new ChannelInputPort ( inputBufferDepth, channelLatency_, this, NULL );
      outputPorts[i] = new ChannelOutputPort ( outputBufferDepth );
    }
    
    internalBuffer = new NetSwitchInternalBuffer ( vcBufferDepth,
                                                   this );
  
    // Initialize the routing table to bogus values.  We can't route yet.
    routingTable = new intP[numNodes];
    vcTable      = new intP[numNodes];

    for ( i = 0; i < numNodes; i++ ) {
      routingTable[i] = new int[numPorts*MAX_NET_VC];
      vcTable[i]      = new int[numPorts*MAX_NET_VC];
      for ( j = 0; j < numPorts*MAX_NET_VC; j++ ) {
        vcTable[i][j] = routingTable[i][j] = -1;
      }
    }

    for ( i = 0; i < MAX_VC; i++ ) 
      messagesWaiting[i] = 0;

  } 
  
  bool NetSwitch::attachChannel ( Channel    * channel,
                                  const int    port,
                                  const bool   isInput )
  {
    assert ( channel );
    assert ( port >= 0 && port < numPorts );
  
    if ( isInput ) {
      
      if ( channel->setToPort ( inputPorts[port] ) ) 
        return true;
      
    } else {
      
      if ( channel->setFromPort ( outputPorts[port] ) ) 
        return true;
    }

    return false;
  }

  bool NetSwitch::drive ( void )
  {
    MessageState
      * msg;

    int 
      vc,
      currPort,
      bandwidthRemaining = crossbarBandwidth;

    // For once and for all, here is the prioritized, no inversion, fair 
    // arbitration algorithm:
    //
    // for each vc { 
    //   for messages in internalBuffers ( vc ) { 
    //      AttemptDelivery()
    //   }
    //   for messages in each port ( vc ) { (start port RR each cycle)
    //     AttemptDelivery()
    //   }
    // }
    // 

    for ( vc = 0; vc < MAX_VC; vc++ ) {
      
      // Look at the internal buffers
      internalBuffer->gotoFirstMessage ( vc );
      while ( !internalBuffer->getMessage ( msg ) && bandwidthRemaining > 0 ) {
        
        if ( !routingPolicy ( msg ) ) {

          if ( internalBuffer->removeMessage() || sendMessageToOutput ( msg ) ) 
            return true;

          bandwidthRemaining--;
        }
        
        // Next message in at this priority level
        if ( internalBuffer->nextMessage() ) break;
      }
      
      // Don't look at an input port if there are no messages waiting
      // on that VC.
      if ( !messagesWaiting[vc] ) 
        continue;

      // Look at the ports 
      currPort = nextStartingPort;
      do {

        if ( !inputPorts[currPort]->peekMessage ( vc, msg ) ) {
          
          // If we can route it to an output port, do so
          if ( !routingPolicy ( msg ) ) { 
            
            if ( inputPorts[currPort]->removeMessage ( vc, msg ) || sendMessageToOutput ( msg ) ) 
              return true;
            
            bandwidthRemaining--;
            messagesWaiting[vc]--;
              
            // Otherwise if we can send it to an internal buffer...
          } else if ( !internalBuffer->isFull ( msg->networkVC ) ) { 
            TRACE ( msg, " SW" << name << " transferring to internal buffer" );
            if ( inputPorts[currPort]->removeMessage ( vc, msg ) || internalBuffer->insertMessage ( msg ) )
              return true;
            messagesWaiting[vc]--;
          }
        }

        currPort = ( currPort + 1 ) % numPorts;
      } while ( currPort != nextStartingPort && bandwidthRemaining );
      
    } // For each vc

    nextStartingPort = ( nextStartingPort + 1 ) % numPorts;

    return false;
  }

  bool NetSwitch::sendMessageToOutput ( MessageState * msg )
  {
    TRACE ( msg, " SW" << name << " routing message to output port " << msg->nextHop );
    
    msg->networkVC = msg->nextVC;
    msg->nextVC    = -1;
    msg->hopCount++;
    
    // Send the message to its output port
    if ( outputPorts[msg->nextHop]->insertMessage ( msg ) ) {
      cerr << "ERROR: sending message from internal buffer to output buffer" << endl;
      return true;
    }

    return false;
  }

  bool NetSwitch::driveInputPorts ( void )
  {
    int 
      i;

    for ( i = 0; i < numPorts; i++ )
      if ( inputPorts[i]->drive() ) return true;

    return false;
  }

  bool NetSwitch::routingPolicy ( MessageState * msg )
  {
    int 
      i,
      routingPort,
      routingVC;

    // Search for an available and acceptable output port, if any  
    for ( i = 0; i < numPorts * MAX_NET_VC; i++ ) {

      if ( routingTable[msg->destNode][i] >= 0 ) {

        routingPort = routingTable[msg->destNode][i];
        routingVC   = vcTable[msg->destNode][i];

        // If there is buffer space, send the message this way
        if ( outputPorts[routingPort]->hasBufferSpace ( BUILD_VC ( msg->priority, routingVC ) ) ) {
          msg->nextHop = routingPort;
          msg->nextVC  = BUILD_VC ( msg->priority, routingVC );
          TRACE ( msg, " SW" << name << " routing destination " << msg->destNode 
                  << " to port " << routingPort << ":" << routingVC );
          return false;
        }

      } else {
        break;
      }
    }

    // No alternative routes
    return true;
  }
  
  bool NetSwitch::checkTopology ( void ) const 
  {
    int
      i;
    
    bool
      foundErrors = false;

    for ( i = 0; i < numPorts; i++ ) { 
      if ( !inputPorts[i]->isConnected() ||
           !outputPorts[i]->isConnected() ) {
        
        std::cerr << "WARNING: switch " << name 
                  << " port " << i << " left unused (may be safe)" 
                  << endl;
        foundErrors = true;
      }
    }
    
    // We do not report these problems to the caller, since
    // they are generally non-fatal at this point (there may be 
    // no route to them)
    return false;
  }

  bool NetSwitch::checkRoutingTable ( void ) const 
  {
    int 
      i,
      j;

    bool 
      foundErrors = false;
    
    // The most basic test we can make is to see that each
    // node has at least one valid routing entry
    assert ( routingTable );

    for ( i = 0; i < numNodes; i++ ) { 

      if ( routingTable[i][0] == -1 ) { 
        std::cerr << "ERROR: partitioned network possible: Switch " << name
                  << " has no routing table entries for node "
                  << i << endl;
        foundErrors = true;
      }

      for ( j = 0; j < numPorts * MAX_NET_VC; j++ ) { 
        if ( routingTable[i][j] != -1 && 
             !outputPorts[ routingTable[i][j] ]->isConnected() ) { 
          
          // This is a topology problem that actually matters!
          std::cerr << "ERROR: routing table specifies route to node " << i 
                    << " exiting switch " << name << " on unconnected port " 
                    << routingTable[i][j]<< ":" << vcTable[i][j] << endl;
          foundErrors = true;
        }
      }
      
    }
    
    return foundErrors;
  }

  bool NetSwitch::addRoutingEntry ( const int node,
                                    const int port,
                                    const int vc )
  {
    int
      i;

    assert ( node < numNodes && node >= 0 );
    assert ( port < numPorts && port >= 0 );
    assert ( vc   < MAX_NET_VC && vc >= 0 );

    for ( i = 0; i < numPorts*MAX_NET_VC; i++ ) {
      if ( routingTable[node][i] == port && vcTable[node][i] == vc ) { 
        std::cerr << "WARNING: duplicate routing entry for switch " << name
                  << " to node " << node << " through port " << port 
                  << " along virtual channel " << vc << endl;
        return false;
      }
      if ( routingTable[node][i] < 0 ) { 
        routingTable[node][i] = port;
        vcTable[node][i]      = vc;
        return false;
      }
    }

    // If we have not found a place in the routing table by now, there
    // are too many entries
    std::cerr << "ERROR: too many routing table entries in switch " << name 
              << " for destination node " << node
              << " (port " << port << ", vc " << vc << ")" << endl; 

    return true;
  }

  bool NetSwitch::dumpState ( ostream & out, const int flags ) 
  {
    int
      i;

    out << "Switch " << name << endl;
    
    if ( flags & NS_DUMP_SWITCHES_IN ) {
      for ( i = 0; i < numPorts; i++ ) {
        out << " IN (" << i << "): ";
        inputPorts[i]->dumpState ( out );
      }
    }
    
    if ( flags & NS_DUMP_SWITCHES_OUT ) { 
      for ( i = 0; i < numPorts; i++ ) {
        out << " OUT(" << i << "):  ";
        outputPorts[i]->dumpState ( out );
      }
    }
    
    if ( flags & NS_DUMP_SWITCHES_INTERNAL ) {
      internalBuffer->dumpState ( out );
    }

    out << endl;

    return false;
  }

  bool NetSwitch::setLocalDelayOnly ( const int port )
  {
    assert ( port >= 0 && port < numPorts );

    return inputPorts[port]->setLocalDelay();
  }

  bool NetSwitchInternalBuffer::dumpState ( ostream & out )
  {
    int
      i;

    out << " INTERNAL BUFFERS: " << endl;
    for ( i = 0; i < MAX_VC; i++ ) {
      out << "  VC" << i << ": " << buffersUsed[i] << "/" << bufferCount[i] << endl;
    }

    for ( i = 0; i < MAX_VC; i++ ) { 
      if ( ageBufferHead[i] != NULL ) {
        out << "  Head serial: " << ageBufferHead[i]->msg->serial << endl;
        break;
      }
    } 
    
    return false;
  }

  /////////////////////////////////////////////////////////////////////////////////
  // Internal Buffer code

  NetSwitchInternalBuffer::NetSwitchInternalBuffer ( const int bufferCount_,
                                                     NetSwitch * netSwitch_ ) :
    currMessage   ( NULL ),
    currPriority  ( 0 ),
    netSwitch     ( netSwitch_ )
  {
    int
      i;
    
    assert ( bufferCount_ >= 0 );
    assert ( netSwitch != NULL );
    
    for ( i = 0; i < MAX_VC; i++ ) {
      buffersUsed[i] = 0;
      bufferCount[i] = bufferCount_;
      ageBufferHead[i] = ageBufferTail[i] = NULL;
    }
  }

  bool NetSwitchInternalBuffer::insertMessage ( MessageState * msg )
  {
    MessageStateList
      * msl;
    
    int
      vc = msg->networkVC;

    assert ( msg );
    assert ( !isFull ( vc ) );

    buffersUsed[vc]++;

    msl = allocMessageStateList ( msg );

    if ( ageBufferTail[vc] == NULL ) {
      ageBufferHead[vc] = ageBufferTail[vc] = msl;
    } else {
      msl->prev = ageBufferTail[vc];
      ageBufferTail[vc]->next = msl;
      ageBufferTail[vc] = msl;
    }

    // Buffer occupancy statistics
    msg->atHeadTime -= currTime;
    msg->bufferTime -= currTime;

    TRACE ( msg, "NetSwitch received message"
            << " to node " << msg->destNode << " with priority "
            << msg->priority );

    return false;
  }
  
  bool NetSwitchInternalBuffer::removeMessage ( void )
  {
    MessageStateList 
      * msl = currMessage;

    assert ( msl != NULL );

    buffersUsed[msl->msg->networkVC]--;
    
    // Buffer occupancy statistics
    msl->msg->atHeadTime += currTime;
    msl->msg->bufferTime += currTime;

    if ( msl->next == NULL ) {
      ageBufferTail[msl->msg->networkVC] = msl->prev;
    } else {
      msl->next->prev = msl->prev;
    }

    if ( msl->prev == NULL ) {
      ageBufferHead[msl->msg->networkVC] = msl->next;
    } else {
      msl->prev->next = msl->next;
    }

    nextMessage();

    freeMessageStateList ( msl );


    return false;
  }

} // namespace nNetShim
