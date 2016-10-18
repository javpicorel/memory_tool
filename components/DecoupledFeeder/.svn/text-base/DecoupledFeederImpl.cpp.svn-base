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


#include <components/DecoupledFeeder/DecoupledFeeder.hpp>

#include <components/DecoupledFeeder/SimicsTracer.hpp>

#include <core/stats.hpp>
#include <core/flexus.hpp>

#include <core/simics/simics_interface.hpp>
#include <core/simics/hap_api.hpp>
#include <core/simics/mai_api.hpp>

#include <boost/function.hpp>
#include <boost/bind.hpp>


  #define DBG_DefineCategories Feeder
  #define DBG_SetDefaultOps AddCat(Feeder)
  #include DBG_Control()

#define FLEXUS_BEGIN_COMPONENT DecoupledFeeder
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()



namespace nDecoupledFeeder {

using namespace Flexus;


class FLEXUS_COMPONENT(DecoupledFeeder){
  FLEXUS_COMPONENT_IMPL( DecoupledFeeder );

  //The Simics objects (one for each processor) for getting trace data
  int theNumCPUs;
  int theCMPWidth;
  SimicsTracerManager * theTracer;
  long long * theLastICounts;
  Stat::StatCounter ** theICounts;




public:
  FLEXUS_COMPONENT_CONSTRUCTOR(DecoupledFeeder)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
  {
    theNumCPUs = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
    theTracer = SimicsTracerManager::construct(theNumCPUs, boost::bind( &DecoupledFeederComponent::toL1D, this, _1, _2), boost::bind( &DecoupledFeederComponent::toL1I, this, _1, _2, _3), boost::bind( &DecoupledFeederComponent::toDMA, this, _1), boost::bind( &DecoupledFeederComponent::toNAW, this, _1, _2)  );
  }

  //InstructionOutputPort
  //=====================
  bool isQuiesced() const {
    return true;
  }

  void initialize(void) {
    //Disable cycle-callback
    Flexus::Simics::theSimicsInterface->disableCycleHook();

    theTracer->setSimicsQuantum(cfg.SimicsQuantum);
    if (cfg.TrackIFetch) {
      theTracer->enableInstructionTracing();
    }

    if (cfg.SystemTickFrequency > 0.0) {
      theTracer->setSystemTick(cfg.SystemTickFrequency);
    }

    theLastICounts = new long long [theNumCPUs];
    theICounts = new Stat::StatCounter * [theNumCPUs];
    for (int i = 0; i < theNumCPUs; ++i) {
      theICounts[i] = new Stat::StatCounter( boost::padded_string_cast<2,'0'>(i) + "-feeder-ICount" );
      theLastICounts[i] = Simics::Processor::getProcessor(i)->stepCount();
    }

    thePeriodicHap = new periodic_hap_t(this, cfg.HousekeepingPeriod);
    theFlexus->advanceCycles(0);

    theCMPWidth = cfg.CMPWidth;
    if (theCMPWidth == 0) {
       theCMPWidth = Flexus::Core::ComponentManager::getComponentManager().systemWidth();
    }
  }

  std::pair< unsigned long, unsigned long> theFetchInfo;

  void toL1D(int anIndex, MemoryMessage & aMessage) {
    FLEXUS_CHANNEL_ARRAY( ToL1D, anIndex ) << aMessage;
  }

  void toNAW(int anIndex, MemoryMessage & aMessage) {
    FLEXUS_CHANNEL_ARRAY( ToNAW, anIndex / theCMPWidth ) << aMessage;
  }

  void toDMA(MemoryMessage & aMessage) {
    FLEXUS_CHANNEL( ToDMA ) << aMessage;
  }

  void toL1I(int anIndex, MemoryMessage & aMessage, unsigned long anOpcode) {
    FLEXUS_CHANNEL_ARRAY( ToL1I, anIndex ) << aMessage;
		//DBG_( Dev, ( << "---------------------> [" << anIndex << "] pc:" << aMessage.pc() ) );
    theFetchInfo.first = aMessage.pc();
    theFetchInfo.second = anOpcode;
    FLEXUS_CHANNEL_ARRAY( ToBPred, anIndex/*0*/ ) << theFetchInfo;
  }

  void updateInstructionCounts() {
      //Count instructions
    for (int i = 0; i < theNumCPUs; ++i) {
      long long temp = Simics::Processor::getProcessor(i)->stepCount() ;
      *(theICounts[i]) += temp - theLastICounts[i];
      theLastICounts[i] = temp;
    }
  }

  void doHousekeeping() {
    updateInstructionCounts();
    theTracer->updateStats();

    theFlexus->advanceCycles(cfg.HousekeepingPeriod);
    theFlexus->invokeDrives();
  }

  void OnPeriodicEvent(Simics::API::conf_object_t * ignored, long long aPeriod) {
    doHousekeeping();
  }

  typedef Simics::HapToMemFnBinding<Simics::HAPs::Core_Periodic_Event, self, &self::OnPeriodicEvent> periodic_hap_t;
  periodic_hap_t * thePeriodicHap;

};  // end class DecoupledFeeder


}  // end Namespace nDecoupledFeeder

FLEXUS_COMPONENT_INSTANTIATOR( DecoupledFeeder, nDecoupledFeeder);
FLEXUS_PORT_ARRAY_WIDTH( DecoupledFeeder, ToL1D ) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH( DecoupledFeeder, ToL1I ) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH( DecoupledFeeder, ToBPred ) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH( DecoupledFeeder, ToNAW ) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT DecoupledFeeder

  #define DBG_Reset
  #include DBG_Control()
