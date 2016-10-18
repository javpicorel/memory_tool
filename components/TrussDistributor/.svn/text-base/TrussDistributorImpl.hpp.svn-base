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

#include <memory>

#include <deque>


#define FLEXUS_BEGIN_COMPONENT TrussDistributor
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

  #define DBG_DefineCategories TrussDistributor, Distributor
  #define DBG_SetDefaultOps AddCat(TrussDistributor | Distributor)
  #include DBG_Control()


namespace nTrussDistributor {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

typedef unsigned long long CycleCount;

typedef std::pair<CycleCount, boost::intrusive_ptr<ArchitecturalInstruction> > InstructionCyclePair;

template <class Cfg>
class TrussDistributorComponent : public FlexusComponentBase< TrussDistributorComponent, Cfg> {
    FLEXUS_COMPONENT_IMPL(nTrussDistributor::TrussDistributorComponent, Cfg);

  public:
   TrussDistributorComponent( FLEXUS_COMP_CONSTRUCTOR_ARGS )
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    {}

    std::deque< InstructionCyclePair > theSlaveBuffers[cfg_t::NumProcs_t::value][cfg_t::NumReplicas_t::value];

    void initialize() { }

    //Ports
  public:
    struct InstructionIn : public PullInputPortArray< InstructionTransport, cfg_t::NumProcs_t::value > { };

    struct InstructionOut : public PullOutputPortArray< InstructionTransport , (1 + cfg_t::NumReplicas_t::value)*(cfg_t::NumProcs_t::value) >, AvailabilityComputedOnRequest {
      FLEXUS_WIRING_TEMPLATE
      static InstructionTransport pull(self & aDistributor, index_t anIndex) {
	// This function is called when a node's fetch component wants an instruction
	// If this is a master node, we get the instruction from the feeder and 
	//   also copy it into two buffers
	// If this is a slave node, we get the instruction from the appropriate buffer

	InstructionTransport anInsn;

	boost::intrusive_ptr<Flexus::SharedTypes::TrussManager> theTrussManager = Flexus::SharedTypes::TrussManager::getTrussManager(0);
	int proc = theTrussManager->getProcIndex(anIndex);
	CycleCount cycle = Flexus::Core::theFlexus->cycleCount();

	if (theTrussManager->isMasterNode(anIndex)) {
	  DBG_Assert(FLEXUS_CHANNEL_ARRAY(aDistributor, InstructionIn, proc).available() );
	  //Fetch it from the feeder
	  FLEXUS_CHANNEL_ARRAY(aDistributor, InstructionIn, proc) >> anInsn;

	  // buffer it for the slave nodes
	  for (int i = 0; i < cfg_t::NumReplicas_t::value; i++) {
	    aDistributor.theSlaveBuffers[proc][i].push_back(InstructionCyclePair(cycle, anInsn[ArchitecturalInstructionTag]->createShadow()));
	  }

	} else {
	  int idx = theTrussManager->getSlaveIndex(anIndex);
	  //DBG_(Crit, (<< "idx = " << idx << " -- queue size: " << aDistributor.theSlaveBuffers[proc][idx].size()));
	  boost::intrusive_ptr<ArchitecturalInstruction> aSlave = aDistributor.theSlaveBuffers[proc][idx].front().second;
	  anInsn.set(ArchitecturalInstructionTag, aSlave);
	  aDistributor.theSlaveBuffers[proc][idx].pop_front();
	}

	DBG_(Iface, ( << "done pulling from Distributor [" << anIndex << "]: " << *anInsn[ArchitecturalInstructionTag] ) );

        return anInsn;
      }

      FLEXUS_WIRING_TEMPLATE
      static bool available(self & aDistributor, index_t anIndex) {
	boost::intrusive_ptr<Flexus::SharedTypes::TrussManager> theTrussManager = Flexus::SharedTypes::TrussManager::getTrussManager(0);
	int proc = theTrussManager->getProcIndex(anIndex);
	if (theTrussManager->isMasterNode(anIndex)) {
	  return FLEXUS_CHANNEL_ARRAY(aDistributor, InstructionIn, proc).available();  
	} else {
	  int idx = theTrussManager->getSlaveIndex(anIndex);
	  if (!aDistributor.theSlaveBuffers[proc][idx].empty()) {
	    CycleCount cycle = aDistributor.theSlaveBuffers[proc][idx].front().first;
	    DBG_Assert(Flexus::Core::theFlexus->cycleCount() <= cycle + theTrussManager->getFixedDelay(), 
		       ( << "Instruction not distributed correctly! cycle: " << cycle+theTrussManager->getFixedDelay()));
	    return (Flexus::Core::theFlexus->cycleCount() == cycle + theTrussManager->getFixedDelay());
	  } else {
	    return false;
	  }
	}// master/slave
      }
    };

  typedef FLEXUS_DRIVE_LIST_EMPTY DriveInterfaces;

  private:
    //Implementation of the FetchDrive drive interface
    FLEXUS_WIRING_TEMPLATE void doFetch() {
    }
};

FLEXUS_COMPONENT_CONFIGURATION_TEMPLATE(TrussDistributorComponentConfiguration,
  FLEXUS_STATIC_PARAMETER( NumProcs, int, "Number of Processors", "procs", 2)
  FLEXUS_STATIC_PARAMETER( NumReplicas, int, "Number of Replicas", "replicas", 1)
);

}//End namespace nTrussDistributor

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT TrussDistributor

#define DBG_Reset
#include DBG_Control()
