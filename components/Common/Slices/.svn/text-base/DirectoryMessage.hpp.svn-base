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

#ifndef FLEXUS_SLICES__DIRECTORY_MESSAGE_HPP_INCLUDED
#define FLEXUS_SLICES__DIRECTORY_MESSAGE_HPP_INCLUDED

#include <iostream>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/types.hpp>

#ifdef FLEXUS_DirectoryMessage_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::DirectoryMessage data type"
#endif
#define FLEXUS_DirectoryMessage_TYPE_PROVIDED

namespace Flexus {
namespace SharedTypes {

  typedef Flexus::SharedTypes::PhysicalMemoryAddress DirectoryAddress;
  namespace DirectoryCommand {
  enum DirectoryCommand {
    Get,
    Found,
    Set,
    //Saved,
    Lock,
    Acquired,
    Unlock,
    //Released,
    Squash
  };
  }
  namespace {
    char* DirectoryCommandStr[] = {
      "GetEntry",
      "EntryRetrieved",
      "SetEntry",
      //"EntryCommitted",
      "LockRequest",
      "LockAcquired",
      "UnlockRequest",
      //"LockReleased",
      "SquashPending"
    };
  }

  struct DirectoryMessage : public boost::counted_base {
    DirectoryMessage(DirectoryCommand::DirectoryCommand anOp)
      : op(anOp)
      , addr(0)
    { }
    DirectoryMessage(DirectoryCommand::DirectoryCommand anOp, DirectoryAddress anAddr)
      : op(anOp)
      , addr(anAddr)
    { }
    DirectoryMessage(const DirectoryMessage & oldMsg)
      : op(oldMsg.op)
      , addr(oldMsg.addr)
    { }

    DirectoryCommand::DirectoryCommand op;
    DirectoryAddress addr;
  };

  inline std::ostream & operator<< (std::ostream & aStream, const DirectoryMessage & msg) {
    aStream << "DirMsg: op=" << DirectoryCommandStr[msg.op] << " addr=" << &std::hex << msg.addr << &std::dec;
    return aStream;
  }

} //End SharedTypes
} //End Flexus

#endif //FLEXUS_COMMON_MEMORYMESSAGE_HPP_INCLUDED
