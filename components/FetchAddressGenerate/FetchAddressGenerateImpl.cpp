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

#include <components/FetchAddressGenerate/FetchAddressGenerate.hpp>

#define FLEXUS_BEGIN_COMPONENT FetchAddressGenerate
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

  #define DBG_DefineCategories FetchAddressGenerate
  #define DBG_SetDefaultOps AddCat(FetchAddressGenerate)
  #include DBG_Control()

#include <core/flexus.hpp>
#include <core/simics/mai_api.hpp>

#include <components/Common/BranchPredictor.hpp>
#include <components/MTManager/MTManager.hpp>

namespace nFetchAddressGenerate {

using namespace Flexus;
using namespace Core;

typedef Flexus::SharedTypes::VirtualMemoryAddress MemoryAddress;


class FLEXUS_COMPONENT(FetchAddressGenerate)  {
    FLEXUS_COMPONENT_IMPL(FetchAddressGenerate);

  std::vector<MemoryAddress> thePC;
  std::vector<MemoryAddress> theNextPC;
  std::vector<MemoryAddress> theRedirectPC;
  std::vector<MemoryAddress> theRedirectNextPC;
  std::vector<bool> theRedirect;
  boost::scoped_ptr<BranchPredictor> theBranchPredictor;
  unsigned int theCurrentThread;

  public:
   FLEXUS_COMPONENT_CONSTRUCTOR(FetchAddressGenerate)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    {}

    void initialize() {
      thePC.resize(cfg.Threads);
      theNextPC.resize(cfg.Threads);
      theRedirectPC.resize(cfg.Threads);
      theRedirectNextPC.resize(cfg.Threads);
      theRedirect.resize(cfg.Threads);
      for (unsigned int i = 0; i < cfg.Threads; ++i) {
        Simics::Processor cpu = Simics::Processor::getProcessor(flexusIndex()*cfg.Threads + i);
        thePC[i] = cpu->getPC();
        theNextPC[i] = cpu->getNPC();
        theRedirectPC[i] = MemoryAddress(0);
        theRedirectNextPC[i] = MemoryAddress(0);
        theRedirect[i] = false;
        DBG_( Dev, Comp(*this) ( << "Thread[" << flexusIndex() << "." << i <<"] connected to " << ((static_cast<Flexus::Simics::API::conf_object_t *>(cpu))->name ) << " Initial PC: " << thePC[i] ) );
      }
      theCurrentThread = cfg.Threads;
      theBranchPredictor.reset( BranchPredictor::combining(statName(), flexusIndex()) );
    }

    bool isQuiesced() const {
      //the FAG is always quiesced.
      return true;
    }

    void saveState(std::string const & aDirName) {
      theBranchPredictor->saveState(aDirName);
    }

    void loadState(std::string const & aDirName) {
      theBranchPredictor->loadState(aDirName);
    }

  public:
  //RedirectIn
  //----------
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(RedirectIn);
  void push(interface::RedirectIn const &, index_t anIndex, std::pair<MemoryAddress, MemoryAddress> & aRedirect) {
     theRedirectPC[anIndex] = aRedirect.first;
     theRedirectNextPC[anIndex] = aRedirect.second;
     theRedirect[anIndex] = true;
  }

  //BranchFeedbackIn
  //----------------
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(BranchFeedbackIn);
  void push(interface::BranchFeedbackIn const &, index_t anIndex, boost::intrusive_ptr<BranchFeedback> & aFeedback) {
        theBranchPredictor->feedback(*aFeedback);
  }

  //Drive Interfaces
  //----------------
  //The FetchDrive drive interface sends a commands to the Feeder and then fetches instructions,
  //passing each instruction to its FetchOut port.
  void drive( interface::FAGDrive const &) {
    int td = 0;
    if (cfg.Threads > 1) {
      td = nMTManager::MTManager::get()->scheduleFAGThread(flexusIndex());
    }
    doAddressGen(td);
  }

  private:
    //Implementation of the FetchDrive drive interface
    void doAddressGen(index_t anIndex) {

      if (theFlexus->quiescing()) {
        //We halt address generation when we are trying to quiesce Flexus
        return;
      }


      if (theRedirect[anIndex]) {
        thePC[anIndex] = theRedirectPC[anIndex];
        theNextPC[anIndex] = theRedirectNextPC[anIndex];
        theRedirect[anIndex] = false;
        DBG_(Iface, Comp(*this) ( << "Redirect Thread[" << anIndex << "] " << thePC[anIndex]) );
      }
      DBG_Assert( FLEXUS_CHANNEL_ARRAY( FetchAddrOut, anIndex).available() );
      DBG_Assert( FLEXUS_CHANNEL_ARRAY( AvailableFAQ, anIndex).available() );
      int available_faq = 0;
      FLEXUS_CHANNEL_ARRAY( AvailableFAQ, anIndex) >> available_faq;

      int max_addrs = cfg.MaxFetchAddress;
      if (max_addrs > available_faq) {
        max_addrs = available_faq;
      }
      int max_predicts = cfg.MaxBPred;

      boost::intrusive_ptr<FetchCommand> fetch(new FetchCommand());
      while ( max_addrs > 0 ) {
        FetchAddr faddr(thePC[anIndex]);

        //Advance the PC
        if ( theBranchPredictor->isBranch( faddr.theAddress ) ) {
          if (max_predicts == 0) {
            break;
          }
          thePC[anIndex] = theNextPC[anIndex];
          theNextPC[anIndex] = theBranchPredictor->predict( faddr );
          if (theNextPC[anIndex] == 0) {
            theNextPC[anIndex] = thePC[anIndex] + 4;
          }
          DBG_(Verb, ( << "Enqueing Fetch Thread[" << anIndex << "] " << faddr.theAddress ) );
          fetch->theFetches.push_back( faddr);
          -- max_predicts;
        } else {
          thePC[anIndex] = theNextPC[anIndex];
          DBG_(Verb, ( << "Enqueing Fetch Thread[" << anIndex << "] " << faddr.theAddress ) );
          fetch->theFetches.push_back( faddr );
          theNextPC[anIndex] = thePC[anIndex] + 4;
        }

        --max_addrs;
      }

      if (fetch->theFetches.size() > 0) {
        //Send it to FetchOut
        FLEXUS_CHANNEL_ARRAY(FetchAddrOut, anIndex) << fetch;
      }
    }

  public:
  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(Stalled);
  bool pull(Stalled const &, index_t anIndex) {
    int available_faq = 0;
    DBG_Assert( FLEXUS_CHANNEL_ARRAY( AvailableFAQ, anIndex ).available() ) ;
    FLEXUS_CHANNEL_ARRAY( AvailableFAQ, anIndex ) >> available_faq;
    return available_faq == 0;
  }

};

}//End namespace nFetchAddressGenerate

FLEXUS_COMPONENT_INSTANTIATOR( FetchAddressGenerate, nFetchAddressGenerate);


FLEXUS_PORT_ARRAY_WIDTH( FetchAddressGenerate, RedirectIn )           { return (cfg.Threads); }
FLEXUS_PORT_ARRAY_WIDTH( FetchAddressGenerate, BranchFeedbackIn )      { return (cfg.Threads); }
FLEXUS_PORT_ARRAY_WIDTH( FetchAddressGenerate, FetchAddrOut )    { return (cfg.Threads); }
FLEXUS_PORT_ARRAY_WIDTH( FetchAddressGenerate, AvailableFAQ )   { return (cfg.Threads); }
FLEXUS_PORT_ARRAY_WIDTH( FetchAddressGenerate, Stalled )   { return (cfg.Threads); }

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT FetchAddressGenerate

#define DBG_Reset
#include DBG_Control()
