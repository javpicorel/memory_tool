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

#define FLEXUS_BEGIN_COMPONENT SordReceiver
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

  #define DBG_DefineCategories SordRecv
  #define DBG_SetDefaultOps AddCat(SordRecv)
  #include DBG_Control()


namespace nSordReceiver {


using namespace Flexus;
using namespace Core;
using namespace SharedTypes;


using boost::intrusive_ptr;

typedef SharedTypes::PhysicalMemoryAddress MemoryAddress;


template <class Cfg>
class SordReceiverComponent : public FlexusComponentBase<SordReceiverComponent, Cfg> {
  FLEXUS_COMPONENT_IMPL(SordReceiverComponent, Cfg);

public:
  SordReceiverComponent( FLEXUS_COMP_CONSTRUCTOR_ARGS )
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {}

  ~SordReceiverComponent() {
  }

  // Initialization
  void initialize() {
    theSordManager = SordManager::getSordManager(flexusIndex());
  }

  // Ports
  struct ToCache : public PushOutputPort< MemoryTransport > { };

  struct ToCpu : public PushOutputPort< MemoryTransport > { };
  struct FromCache : public PushInputPort<MemoryTransport>, AvailabilityDeterminedByInputs {
    typedef FLEXUS_IO_LIST(1, Availability<ToCpu>) Inputs;
    typedef FLEXUS_IO_LIST(1, Value<ToCpu>) Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void push(self& aSordRecv, MemoryTransport aMessageTransport) {
      DBG_(Iface, Comp(aSordRecv) Addr(aMessageTransport[MemoryMessageTag]->address())
                  ( << "message received from Cache: " << *aMessageTransport[MemoryMessageTag] ) );

      if(!aMessageTransport[SordReceiverMarkTag]) {
        // since we didn't mark this transaction, we must pass it along
        DBG_(Iface, Comp(aSordRecv) Addr(aMessageTransport[MemoryMessageTag]->address())
                    ( << "message passed to cpu: " << *aMessageTransport[MemoryMessageTag] ) );
        FLEXUS_CHANNEL(aSordRecv, ToCpu) << aMessageTransport;
      }
      else {
        // must be a response a "fake" prediction request
        aSordRecv.recv(*aMessageTransport[MemoryMessageTag]);
      }
    }

    FLEXUS_WIRING_TEMPLATE
    static bool available(self& aSordRecv) {
      return FLEXUS_CHANNEL(aSordRecv, ToCpu).available();
    }
  };

  //Drive Interfaces
  struct SordRecvDrive {
    typedef FLEXUS_IO_LIST(1, Availability<ToCpu>) Inputs;
    typedef FLEXUS_IO_LIST(1, Value<ToCpu>) Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void doCycle(self & aSordRecv) {
      aSordRecv.handlePredictions<FLEXUS_PASS_WIRING>();
    }
  };

  // Declare the list of Drive interfaces
  typedef FLEXUS_DRIVE_LIST(1, SordRecvDrive) DriveInterfaces;

private:

  FLEXUS_WIRING_TEMPLATE
  void handlePredictions() {
    while(FLEXUS_CHANNEL(*this, ToCache).available()) {
      intrusive_ptr<SordPrediction> pred = theSordManager->takePrediction();
      if(!pred) {
        break;
      }

      // there's a prediction request
      intrusive_ptr<MemoryMessage> req;
      switch(pred->type) {
      case nSordManager::ReadPred:
        req = new MemoryMessage(MemoryMessage::ReadReq, pred->address);
        break;
      case nSordManager::WritePred:
        req = new MemoryMessage(MemoryMessage::WriteReq, pred->address);
        break;
      default:
        DBG_(Crit, ( << "bad prediction type: " << *pred ) );
        DBG_Assert(false);
      }

      MemoryTransport trans;
      trans.set(MemoryMessageTag, req);
      trans.set(SordReceiverMarkTag, new SordReceiverMark(pred));
      DBG_(Iface, Comp(*this) Addr(pred->address)
                  ( << "request for SORD stream: " << *req ) );
      FLEXUS_CHANNEL(*this, ToCache) << trans;
    }
  }

  void recv(MemoryMessage & aMsg) {
    // do nothing - at the moment, we don't record what requests are sent out,
    // so there is no need to match responses to pending requests
  }


  // the SORD manager
  intrusive_ptr<SordManager> theSordManager;

};

FLEXUS_COMPONENT_EMPTY_CONFIGURATION_TEMPLATE(SordReceiverConfiguration);

} //End Namespace nSordReceiver

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT SordReceiverComponent

  #define DBG_Reset
  #include DBG_Control()

