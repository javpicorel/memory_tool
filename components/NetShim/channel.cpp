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
#include "channel.hpp"
#include "netswitch.hpp"
#include "netnode.hpp"

namespace nNetShim
{

  //////////////////////////////////////////////////////////////////////
  //

  ChannelPort::ChannelPort ( const int bufferCount_ )
  {
    int
      i;

    for ( i = 0; i < MAX_VC; i++) {
      // HACK - Jared will fix this with a more elegant,
      // per-VC solution involving a param file.
      if ((i == 0 || i == 1) && bufferCount_ == INT_MAX) {
        bufferCount[i] = 1;
      } else {
	bufferCount[i] = bufferCount_;
      }
    }

    channel = NULL;

    assert ( bufferCount_ > 0 );

    for ( i = 0; i < MAX_VC; i++ ) {
      mslHead[i]     = mslTail[i] = NULL;
      buffersUsed[i] = 0;
    }
  }

  bool ChannelPort::insertMessageHelper ( MessageState * msg )
  {
    MessageStateList * msl;

    msl = allocMessageStateList ( msg );

    assert(msg->networkVC >= 0 && msg->networkVC < MAX_VC);
    if ( mslHead[msg->networkVC] == NULL ) {
      mslHead[msg->networkVC] = mslTail[msg->networkVC] = msl;

      // For time at head statistics
      msg->atHeadTime -= currTime;
    } else {
      mslTail[msg->networkVC]->next = msl;
      msl->prev = mslTail[msg->networkVC];
      mslTail[msg->networkVC] = msl;
    }

    return false;
  }

  bool ChannelPort::removeMessage ( const int       vc,
                                    MessageState *& msg )
  {
    MessageStateList * msl;

    assert(vc >= 0 && vc < MAX_VC);

    msl = mslHead[vc];

    assert ( msl != NULL );

    msg = msl->msg;

    if ( msl->next == NULL )
      mslHead[vc] = mslTail[vc] = NULL;
    else {
      mslHead[vc] = mslHead[vc]->next;
      mslHead[vc]->msg->atHeadTime -= currTime;
    }

    freeMessageStateList ( msl );

    buffersUsed[vc]--;

    msg->atHeadTime += currTime;
    msg->bufferTime += currTime;

    return false;
  }

  bool ChannelPort::dumpState ( ostream & out )
  {
    int
      i;

    out << " ChannelPort: ";

    for ( i = 0; i < MAX_VC; i++ ) {
      out << " " << buffersUsed[i];

      out << " / " << bufferCount[i]
	  << " buffers used";

      out << endl;
    }

    return false;
  }

  //////////////////////////////////////////////////////////////////////
  //

  ChannelInputPort::ChannelInputPort ( const int bufferCount_,
                                       const int channelLatency_,
                                       NetSwitch * netSwitch_,
                                       NetNode   * netNode_ ) 
    : ChannelPort ( bufferCount_ )
  {
    channelLatency = channelLatency_;
    delayHead = delayTail = NULL;
    netSwitch = netSwitch_;
    netNode   = netNode_;
  }

  bool ChannelInputPort::insertMessage ( MessageState * msg )
  {
    MessageStateList * msl;

    assert ( buffersUsed[msg->networkVC] < bufferCount[msg->networkVC] );

    buffersUsed[msg->networkVC]++;

    // Queue for a delay
    msl = allocMessageStateList ( msg );
    if (msg->flexusInFastMode) {
      msl->delay = currTime + 1;
    } else {
      msl->delay = currTime + channelLatency;
    }

    // Insert into the ordered queue
    if ( delayHead == NULL ) {
      delayHead = delayTail = msl;
    } else {
      delayTail->next = msl;
      delayTail = msl;
    }

    // For time in buffer statistics
    msg->bufferTime -= currTime;

    return false;
  }


  bool ChannelInputPort::drive ( void )
  {
    while ( delayHead && delayHead->delay <= currTime ) {
      
      MessageStateList
        * oldNode = delayHead;
      
      delayHead = oldNode->next;
      if ( delayHead == NULL ) 
        delayTail = NULL;
      
      // Notify the switch (if any) that a message is ready
      if ( netSwitch != NULL ) { 
        netSwitch->notifyWaitingMessage ( oldNode->msg->networkVC );
      }
      
      if ( netNode != NULL ) {
        netNode->notifyWaitingMessage();
      }        
      
      if ( insertMessageHelper ( oldNode->msg ) )
        return true;
      
      freeMessageStateList ( oldNode );
    }

    return false;
  }

  bool ChannelInputPort::setLocalDelay ( void )
  {
    channelLatency = 1;
    return false;
  }

  //////////////////////////////////////////////////////////////////////
  //

  ChannelOutputPort::ChannelOutputPort ( const int bufferCount_ )
    : ChannelPort ( bufferCount_ )
  {  }

  bool ChannelOutputPort::insertMessage ( MessageState * msg )
  {
    assert ( hasBufferSpace ( msg->networkVC ) );
    msg->bufferTime -= currTime;

    if ( buffersUsed[msg->networkVC] == 0 )
      msg->acceptTime -= currTime;

    buffersUsed[msg->networkVC]++;
    
    channel->notifyWaitingMessage();
    
    return insertMessageHelper ( msg );
  }

  bool ChannelOutputPort::drive ( void )
  {
    // Nothing to do here
    return false;
  }

  //////////////////////////////////////////////////////////////////////
  //

  Channel::Channel ( const int id_ )
  {
    id = id_;
    busy = 0;
    localLatencyDivider = 1;
    state = CS_IDLE;

    fromPort = NULL;
    toPort   = NULL;

    messagesWaiting = 0;
  }

  bool Channel::drive ( void )
  {
    int
      i;

    switch ( state )
      {

      case CS_IDLE:
        {
          assert ( busy <= 0 );

          if ( messagesWaiting > 0 ) 
            state = CS_WAIT_FOR_ACCEPT;
        }
        break;

      case CS_WAIT_FOR_ACCEPT:
        {
          // Check the output port for available buffers
          // We give priority to the lowest numbered network
          // virtual channels
          assert ( busy <= 0 );
          assert ( messagesWaiting > 0 );
            
          for ( i = 0; i < MAX_VC; i++ ) { 
            MessageState * msg;
            
            if ( fromPort->hasMessage ( i ) && toPort->hasBufferSpace ( i ) ) {
              
              state = CS_TRANSFERRING;

              // Transfer the message
              if ( fromPort->removeMessage ( i, msg ) )
                return true;

              if ( toPort->insertMessage ( msg ) )
                return true;
              
              // For local node-switch channels, the channel occupancy is shorter 
              // than for switch-switch channels
              busy = msg->transmitLatency / localLatencyDivider;
              if ( busy <= 0 ) busy = 1;
              TRACE ( msg, "Channel setting busy time to: " << busy << " with divider: " 
                      << localLatencyDivider );

              messagesWaiting--;

              break;
            }
          }

        }
        break;

      case CS_TRANSFERRING:
        {
          // Count down the transfer timer, if finished, go
          // to idle and allow the next transfer to occur in
          // later cycles
          busy--;
          if ( busy <= 0 ) {
            state = CS_IDLE;
          }
        }
        break;

      default:
        assert ( 0 );
        break;
      }

    return false;
  }

  bool Channel::setFromPort ( ChannelOutputPort * port )
  {
    if ( fromPort )
      return true;

    fromPort = port;
    fromPort->setChannel ( this );
    return false;
  }

  bool Channel::setToPort ( ChannelInputPort * port )
  {
    if ( toPort )
      return true;

    toPort = port;
    toPort->setChannel ( this );
    return false;
  }

  bool Channel::dumpState ( ostream & out )
  {
    out << "Channel " << id << " state: " << state << endl;
    return false;
  }

} // namespace nNetShim
