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

#include <vector>

#include <components/Directory/DirTest.hpp>
#include <components/Common/Slices/DirectoryMessage.hpp>
#include <components/Common/Slices/DirectoryEntry.hpp>
#include <components/Common/Slices/Mux.hpp>

#define FLEXUS_BEGIN_COMPONENT DirTest
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nDirTest {

using namespace Flexus::Core;
using namespace Flexus::SharedTypes;
using boost::intrusive_ptr;

typedef PhysicalMemoryAddress MemAddr;

enum TestOp {
  TGet,    // ask for a new address
  TTest,   // compare a retrieved value
  TSet,    // set a new value for an address
  TReply,  // set a new value from a retrieved entry
  TMod,    // modify a value (locally) for a retreived entry
  TEnd     // end of test cases
};

char* TestOpStr[] = {
  "Get",
  "Test",
  "Set",
  "Reply",
  "Mod",
  "End"
};

struct TestCase {
  TestCase(TestOp o, MemAddr a, int v)
    : op(o)
    , addr(a)
    , value(v)
  { }

  TestOp op;
  MemAddr addr;
  unsigned int value;
};

TestCase testCases[] = {
  TestCase(TSet,   MemAddr(4), 10),
  TestCase(TGet,   MemAddr(4), 0),
  TestCase(TTest,  MemAddr(4), 10),
  TestCase(TReply, MemAddr(4), 23),
  TestCase(TSet,   MemAddr(9), 18),
  TestCase(TGet,   MemAddr(9), 0),
  TestCase(TTest,  MemAddr(9), 18),
  TestCase(TMod,   MemAddr(9), 97),
  TestCase(TGet,   MemAddr(9), 0),
  TestCase(TTest,  MemAddr(9), 18),
  TestCase(TEnd,   MemAddr(0), 0)
};


class FLEXUS_COMPONENT(DirTest) {
  FLEXUS_COMPONENT_IMPL(DirTest);

 public:
  FLEXUS_COMPONENT_CONSTRUCTOR(DirTest)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {
  }

  virtual ~DirTestComponent() {}

  // Initialization
  void initialize() {

    DBG_( Dev, ( << "Initializing DirTestComponent" ) Comp(*this) ) ;

    pending = false;
    currTest = 0;
    pass = true;
  }

  // Ports
    FLEXUS_PORT_ALWAYS_AVAILABLE(ResponseIn);

    void push(interface::ResponseIn const &, DirectoryTransport & aDirTransport) {
      intrusive_ptr<DirectoryMessage> msg = aDirTransport[DirectoryMessageTag];
      intrusive_ptr<DirectoryEntry> entry = aDirTransport[DirectoryEntryTag];
      entry->markModified();
      DBG_(Dev,  ( << " Received a message (" << *msg << "; " << *entry << ')' ) Comp(*this) );
      if(!pending) {
        DBG_(VVerb,  ( << "... while not in pending state.") );
        pass = false;
        return;
      }
      pending = false;

      bool ok = true;
      if(testCases[currTest].op == TTest) {
        if(msg->addr != testCases[currTest].addr) {
          DBG_( Crit, ( << "Received response with incorrect address." ) Comp(*this) );
          pass = false;
          ok = false;
        }
        if(entry->sharers() != testCases[currTest].value) {
          DBG_( Crit, ( << "Received response with incorrect value." ) Comp(*this));
          pass = false;
          ok = false;
        }
        if(ok) {
          DBG_( VVerb, ( << "...test passed, currTest=" << currTest  ) Comp(*this));
        }
        currTest++;
      }
      if(testCases[currTest].op == TReply) {
        if(msg->addr != testCases[currTest].addr) {
          DBG_( Crit, ( << "\"Reply\" operation with incorrect address."  ) Comp(*this));
          pass = false;
        }
        DirectoryTransport transport;
        intrusive_ptr<DirectoryMessage> reply = new DirectoryMessage(DirectoryCommand::Set, msg->addr);
        transport.set(DirectoryMessageTag, reply);
        entry->setSharers(testCases[currTest].value);
        transport.set(DirectoryEntryTag, entry);
        outQueue.push_back(transport);
        currTest++;
      }
      else if(testCases[currTest].op == TMod) {
        saveEntry = entry;
        saveEntry->setSharers(testCases[currTest].value);
        currTest++;
      }
    }

  //Drive Interfaces
  void drive(DirTestDrive const & ) {
      // if the the outgoing queue is not empty, send the head element
      if(!outQueue.empty()) {
        FLEXUS_CHANNEL( RequestOut ) << outQueue.front();
        outQueue.erase(outQueue.begin());
      }
      else {
        // if something is pending, do nothing while we wait for a response;
        // otherwise, start the next test case
        if(!pending) {
          nextTestCase ();
        }
      }
   }

private:
  void nextTestCase() {
    DBG_( VVerb, ( << "DoCycle: DirTest op=" << TestOpStr[testCases[currTest].op]
                  << " addr=" << testCases[currTest].addr
                  << " value=" << testCases[currTest].value ) Comp(*this));
    DirectoryTransport transport;
    if(testCases[currTest].op == TGet) {
      intrusive_ptr<DirectoryMessage> msg = new DirectoryMessage(DirectoryCommand::Get, testCases[currTest].addr);
      transport.set(DirectoryMessageTag, msg);
      outQueue.push_back(transport);
      pending = true;
      currTest++;
    }
    else if(testCases[currTest].op == TSet) {
      intrusive_ptr<DirectoryMessage> msg = new DirectoryMessage(DirectoryCommand::Set, testCases[currTest].addr);
      transport.set(DirectoryMessageTag, msg);
      intrusive_ptr<DirectoryEntry> entry = new DirectoryEntry();
      entry->setSharers(testCases[currTest].value);
      transport.set(DirectoryEntryTag, entry);
      outQueue.push_back(transport);
      currTest++;
    }
    else if(testCases[currTest].op == TEnd) {
      if(pass) {
        std::cout << "Directory test PASSED.\n";
      }
      else {
        std:: cout << "Directory test FAILED.\n";
      }
      exit(0);
    }
  }

  // Expecting a response?
  bool pending;

  // Current test case
  int currTest;

  // Did we pass?
  bool pass;

  // Keep an old DirectoryEntry around
  intrusive_ptr<DirectoryEntry> saveEntry;

  // Queue of reqeust message to send
  std::vector<DirectoryTransport> outQueue;

};

} //end nDirTest

FLEXUS_COMPONENT_INSTANTIATOR( DirTest, nDirTest );

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT DirTest
