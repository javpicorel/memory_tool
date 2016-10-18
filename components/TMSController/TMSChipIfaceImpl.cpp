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

#include <components/TMSController/TMSChipIface.hpp>

#define FLEXUS_BEGIN_COMPONENT TMSChipIface
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

namespace nTMSChipIface {

class FLEXUS_COMPONENT(TMSChipIface) {
  FLEXUS_COMPONENT_IMPL(TMSChipIface);

  //CMOB controller state
    struct CMOBReadTransaction : boost::counted_base {
      int theTMSc;
      int theCMOBIndex;
      int theStartLocation;
      int theRequestId;
      int theRequestTag;
      std::vector< MemoryAddress > theAddresses;
      std::vector< bool > theWasHit;
      friend std::ostream &  operator << (std::ostream & anOstream, CMOBReadTransaction const & t) {
        anOstream << "CMOB[" << t.theCMOBIndex <<"]#" << t.theRequestId << " " << t.theStartLocation << " => " << t.theTMSc << " " << t.theAddresses.size() << " addresses"; 
        return anOstream; 
      }
      
    };
  
    long theNextRequestTag;
    
    std::vector< int > theNextOffset;
    std::vector< CMOBLine > theAppendLines;

    std::map< long, boost::intrusive_ptr<CMOBReadTransaction> > thePendingReads;

 public:
   FLEXUS_COMPONENT_CONSTRUCTOR(TMSChipIface)
     : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  { }

  bool isQuiesced() const { return true;  }

  void saveState(std::string const & aDirName) { }

  void loadState(std::string const & aDirName) {  }


  // Initialization
  void initialize() {
    theNextRequestTag = 1;
    theNextOffset.resize( Flexus::Core::ComponentManager::getComponentManager().systemWidth(), -1 );
    theAppendLines.resize( Flexus::Core::ComponentManager::getComponentManager().systemWidth() );
  }

  // Ports
  bool available(interface::MonitorMemRequests_In const &) {
      return FLEXUS_CHANNEL(MonitorMemRequests_Out).available();
  }
  void push(interface::MonitorMemRequests_In const &,  MemoryTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received on Port Monitor_OffChipRequest_In: " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    
    FLEXUS_CHANNEL_ARRAY(TMSc_MemRequests, aMessage[MemoryMessageTag]->coreIdx() / 2) << aMessage;         
    
    FLEXUS_CHANNEL(MonitorMemRequests_Out) << aMessage;
  }

  bool available(interface::MonitorMemPrefetch_In const &) {
      return FLEXUS_CHANNEL(MonitorMemPrefetch_Out).available();
  }
  void push(interface::MonitorMemPrefetch_In const &,  MemoryTransport & aMessage) {
    DBG_(Iface, Comp(*this) ( << "Received on Port Monitor_OffChipPrefetch_In: " << *(aMessage[MemoryMessageTag]) ) Addr(aMessage[MemoryMessageTag]->address()) );
    
    FLEXUS_CHANNEL_ARRAY(TMSc_MemRequests, aMessage[MemoryMessageTag]->coreIdx() / 2) << aMessage;         
    
    FLEXUS_CHANNEL(MonitorMemPrefetch_Out) << aMessage;
  }

  //TMSc to CMOB Interface

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(TMSc_NextCMOBIndex );
  long pull(interface::TMSc_NextCMOBIndex const &,  index_t aCMOB) {
    DBG_Assert( aCMOB < theAppendLines.size() );
    if (theNextOffset[aCMOB] == -1) {
      initializeAppend(aCMOB); 
    }
    long index = 0;
    FLEXUS_CHANNEL_ARRAY( CMOB_NextAppendIndex, aCMOB) >> index;    
    return index + theNextOffset[aCMOB];
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(TMSc_CMOBAppend);
  void push(interface::TMSc_CMOBAppend const &,  index_t aTMSc, std::pair<MemoryAddress, bool> & anAppend) {
    DBG_(Iface, Comp(*this) ( << "TMSc[" << aTMSc <<"] append " << anAppend.first <<  (anAppend.second ? "(hit) " : "(nohit) ")) );

    doCMOBAppend( aTMSc, anAppend.first, anAppend.second);    
  }

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(TMSc_CMOBRequest);
  void push(interface::TMSc_CMOBRequest const &,  index_t aTMSc, cmob_request_t & aRequest) {
    DBG_(Iface, Comp(*this) ( << "TMSc[" << aTMSc <<"] request " << aRequest.get<0>() << ":" << aRequest.get<1>() << " Id: " << aRequest.get<2>()));
    doCMOBRequest( aTMSc, aRequest.get<0>(), aRequest.get<1>(), aRequest.get<2>());    
  }


  //TMSc to CMOB Interface  
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(CMOB_Reply);
  void push(interface::CMOB_Reply const &,  index_t aCMOB, boost::intrusive_ptr<CMOBMessage> & aMessage) {
    DBG_(Iface, Comp(*this) ( << *aMessage) );
    
    handleCMOBReply( *aMessage);
  }

  long roundUp( long aValue, long aMultiple ) {
    return (aValue - (aValue % aMultiple) + aMultiple);
  }
  
  long roundDown( long aValue, long aMultiple ) {
    return (aValue - (aValue % aMultiple) );
  }

  //CMOB Control
  void doCMOBRequest( int aTMSc, int aCMOB, long aLocation, long anId) {
    DBG_( Iface, ( << "TMSc[" << aTMSc <<"] Request Addresses from CMOB[" << aCMOB <<"] at " << aLocation << " id " << anId));
    
    //Create CMOB Request Transaction
    boost::intrusive_ptr<CMOBReadTransaction> trans(new CMOBReadTransaction);
    trans->theTMSc = aTMSc;
    trans->theCMOBIndex = aCMOB;
    trans->theStartLocation = aLocation;
    trans->theRequestId = anId;
    trans->theRequestTag = theNextRequestTag++;

    thePendingReads.insert( std::make_pair( trans->theRequestTag, trans));
    
    //Ignore request size!
    boost::intrusive_ptr<CMOBMessage> msg = new CMOBMessage;
    msg->theCommand = CMOBCommand::eRead;
    msg->theCMOBId = aCMOB;
    msg->theCMOBOffset = roundDown(trans->theStartLocation, CMOBLine::kLineSize);
    msg->theRequestTag = trans->theRequestTag;

    DBG_(Iface, Comp(*this) ( << " CMOB[" << aCMOB << "] read " << *msg) );
    FLEXUS_CHANNEL_ARRAY(CMOB_Request, aCMOB) << msg;           
  }

  void handleCMOBReply( CMOBMessage & msg ) {
    std::map< long, boost::intrusive_ptr<CMOBReadTransaction> >::iterator iter = thePendingReads.find( msg.theRequestTag );
    DBG_Assert( iter != thePendingReads.end() );
    boost::intrusive_ptr<CMOBReadTransaction> trans = iter->second;
    DBG_Assert( msg.theCMOBId == trans->theCMOBIndex );
    trans->theAddresses.resize( msg.theCMOBOffset - trans->theStartLocation + CMOBLine::kLineSize );
    trans->theWasHit.resize( msg.theCMOBOffset - trans->theStartLocation + CMOBLine::kLineSize );
    for (int i = 0; i < CMOBLine::kLineSize; ++i) {
      if( msg.theCMOBOffset + i >= trans->theStartLocation) {
        int offset = msg.theCMOBOffset - trans->theStartLocation + i;
        trans->theAddresses[offset] = msg.theLine.theAddresses[i];
        trans->theWasHit[offset] = msg.theLine.theWasHit[i];      
      }
    }
    finishCMOBRead( trans ); 
  }

  void finishCMOBRead( boost::intrusive_ptr<CMOBReadTransaction> trans) {
    DBG_(Verb, ( << "Finish " << *trans));
    cmob_reply_t reply( trans->theCMOBIndex, trans->theStartLocation, trans->theRequestId, trans->theAddresses, trans->theWasHit);
    FLEXUS_CHANNEL_ARRAY(TMSc_CMOBReply, trans->theTMSc) << reply;       
    thePendingReads.erase( trans->theRequestTag ); 
  }

  void doCMOBAppend( int aCMOB, MemoryAddress anAddress, bool wasHit ) {
    DBG_(Iface, Comp(*this) ( << " CMOB[" << aCMOB << "] append " << anAddress << ( wasHit ? "(hit)" : "(nohit)" ) ));

    DBG_Assert( aCMOB < theAppendLines.size());
    if (theNextOffset[aCMOB] == -1) {
      //Very first append.  Need to initialize 
      initializeAppend(aCMOB);
    }
    
    //Append address to active CMOB line
    DBG_Assert( theNextOffset[aCMOB] < CMOBLine::kLineSize );
    theAppendLines[aCMOB].theAddresses[ theNextOffset[aCMOB] ] = anAddress;
    theAppendLines[aCMOB].theWasHit[ theNextOffset[aCMOB] ] = wasHit;
    ++theNextOffset[aCMOB];
    
    //If the CMOB line is full, send it over to the CMOB to be recorded.
    if (theNextOffset[aCMOB] == CMOBLine::kLineSize ) {
      doCMOBWriteback(aCMOB);
    }
  }

  void doCMOBWriteback( int aCMOB ) {
    long append_index = 0;
    FLEXUS_CHANNEL_ARRAY( CMOB_NextAppendIndex, aCMOB) >> append_index;
    boost::intrusive_ptr<CMOBMessage> write = new CMOBMessage;
    write->theCommand = CMOBCommand::eWrite;
    write->theCMOBId = aCMOB;
    write->theCMOBOffset = append_index;
    write->theLine = theAppendLines[aCMOB];
    write->theRequestTag = 0; //Not used for CMOB writes
    theNextOffset[aCMOB] = 0;
    DBG_(Iface, Comp(*this) ( << " CMOB[" << aCMOB << "] writeback " << *write ) );
    FLEXUS_CHANNEL_ARRAY(CMOB_Request, aCMOB) << write;
  }

  void initializeAppend(int aCMOB) {
    long append_index = 0;
    FLEXUS_CHANNEL_ARRAY( CMOB_NextAppendIndex, aCMOB) >> append_index;
    CMOBMessage init;
    FLEXUS_CHANNEL_ARRAY( CMOB_Initialize, aCMOB) >> init;
    DBG_(Iface, Comp(*this) ( << " CMOB[" << aCMOB << "] initialize " << init ) );
    theAppendLines[aCMOB] = init.theLine;
    theNextOffset[aCMOB] = init.theRequestTag;
    DBG_Assert( theNextOffset[aCMOB]  < CMOBLine::kLineSize );       
  } 
  

};


} //End Namespace nTMSChipIface

FLEXUS_COMPONENT_INSTANTIATOR( TMSChipIface, nTMSChipIface);
FLEXUS_PORT_ARRAY_WIDTH( TMSChipIface, TMSc_MemRequests) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH( TMSChipIface, TMSc_NextCMOBIndex) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH( TMSChipIface, TMSc_CMOBAppend) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH( TMSChipIface, TMSc_CMOBRequest) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH( TMSChipIface, TMSc_CMOBReply) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH( TMSChipIface, CMOB_NextAppendIndex) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH( TMSChipIface, CMOB_Initialize) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH( TMSChipIface, CMOB_Request) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH( TMSChipIface, CMOB_Reply) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT TMSChipIface

