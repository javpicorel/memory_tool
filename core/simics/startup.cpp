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

#include <iostream>

#include <core/debug/debug.hpp>

#include <core/target.hpp>
#include <core/simulator_name.hpp>
#include <core/configuration.hpp>
#include <core/component.hpp>

#include <core/simics/simics_interface.hpp>


namespace Flexus {


namespace Core {
  void Break() { Flexus::Simics::BreakSimulation("Simulation break caused by debugger."); } //Does nothing
  void CreateFlexusObject();
  void PrepareFlexusObject();

}

namespace Simics {

using namespace Flexus::Core;
namespace Simics = Flexus::Simics;

Simics::SimicsInterface_Obj theSimicsInterface;

Simics::SimicsInterfaceFactory* theSimicsInterfaceFactory;


void CreateFlexus() {
    CreateFlexusObject();

    theSimicsInterface = theSimicsInterfaceFactory->create("flexus-simics-interface");
    if (!theSimicsInterface) {
        throw SimicsException("Unable to create SimicsInterface object in Simics");
    }

    Flexus::Core::ComponentManager::getComponentManager().instantiateComponents( theSimicsInterface->getSystemWidth() );
    ConfigurationManager::getConfigurationManager().processCommandLineConfiguration(0, 0);
}

void PrepareFlexus() {
  PrepareFlexusObject();
  theSimicsInterfaceFactory = new Simics::SimicsInterfaceFactory ();

  CallOnConfigReady(CreateFlexus);
}

} //end namespace Core
} //end namespace Flexus

namespace {

using std::cerr;
using std::endl;

	void print_copyright() {
		cerr << "\nFlexus (C) 2006 The SimFlex Project" << endl;
		cerr << "Eric Chung, Michael Ferdman, Brian Gold, Nikos Hardavellas, Jangwook Kim," << endl;
		cerr << "Ippokratis Pandis, Minglong Shao, Jared Smolens, Stephen Somogyi," << endl;
		cerr << "Evangelos Vlachos, Thomas Wenisch, Roland Wunderlich" << endl;
		cerr << "Anastassia Ailamaki, Babak Falsafi and James C. Hoe." << endl << endl;
		cerr << "Flexus Simics simulator - Built as " << Flexus::theSimulatorName << endl << endl;
	}

}

extern "C" void init_local(void) {
	std::cerr << "Entered init_local\n";
	DBG_(Dev, (<< "Initializing Flexus." ));

	print_copyright();

	//Do all the stuff we need to get Simics to know we are here
	Flexus::Simics::PrepareFlexus();

	DBG_(Iface, (<< "Flexus Initialized." ));
}

extern "C" void fini_local(void) {
	//Theoretically, we would delete Flexus here, but Simics currently does not call this function.
	//delete theFlexusFactory;
}

