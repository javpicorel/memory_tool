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
/*
  V9 Memory Op
*/

namespace nInorderSimicsFeeder {

  class SimicsCycleManager {
      std::vector< boost::shared_ptr<SimicsTraceConsumer> > & theConsumers;

    public:
      SimicsCycleManager(std::vector< boost::shared_ptr<SimicsTraceConsumer> > & aConsumers, bool aClientServer)
        : theConsumers(aConsumers)
        {
          for(unsigned int i = 0; i < theConsumers.size(); ++i) {
            std::string name("cpu");
            if (aClientServer) {
              name = "server_cpu";
            }
            name += boost::lexical_cast<std::string>(i);
            Simics::API::conf_object_t * cpu = Simics::API::SIM_get_object( name.c_str() );
            DBG_Assert(cpu != 0, ( << "CycleManager cannot locate " << name << " object. No such object in Simics" ));

            Simics::Processor p = Simics::Processor(cpu);
            theConsumers[i]->setInitialCycleCount(p->cycleCount());
          }
        }

      void advanceFlexus() {
        while (allConsumersReady()) {
          theFlexus->doCycle();
        }
      }

      Simics::API::cycles_t reconcileTime(Simics::API::cycles_t aPendingInstructions) {
        Simics::Processor cpu = Simics::Processor::current();
        int current_cpu = Simics::APIFwd::SIM_get_processor_number(cpu);
        Simics::API::cycles_t simics_cycle_count = cpu->cycleCount() - aPendingInstructions - theConsumers[current_cpu]->initialCycleCount();
        Simics::API::cycles_t flexus_cycle_count = theFlexus->cycleCount();
        //DBG_(Dev, ( << "CPU " << current_cpu << " Reconciling simics: " << simics_cycle_count << " Flexus: " << flexus_cycle_count));
        if (simics_cycle_count < 2 * flexus_cycle_count) {
          return 2*flexus_cycle_count - simics_cycle_count;
        } else {
          return 0;
        }
/*        if (simics_cycle_count > flexus_cycle_count) {
            DBG_Assert( (simics_cycle_count < 2 * flexus_cycle_count) );
            return 0;
        } else {
            if (theConsumers.size() == 1) {
              //For uniprocessors, we return the exact stall time for maximum efficiency
              return flexus_cycle_count - simics_cycle_count;
            } else {
              //For muliprocessors, we return 1.  This maximizes our chances of being able to reclaim "slip" cycles
              return 1;
            }
        }
 */
      }

    private:
      bool allConsumersReady() {
        if (theFlexus->quiescing()) {
          return true; //Always ready to advance time when quiescing.
        }
        bool ready = true;
        for(unsigned int i = 0; (i < theConsumers.size()) && ready ; ++i) {
          ready =  (theConsumers[i]->isReady() ||  theConsumers[i]->isInProgress());
        }
        return ready;
      }
  };  // struct SimicsCycleManager

}
