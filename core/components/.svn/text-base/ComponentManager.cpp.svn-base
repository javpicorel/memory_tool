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
#include <vector>
#include <algorithm>

#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace ll = boost::lambda;

#include <core/component.hpp>
#include <core/debug/debug.hpp>


namespace Flexus {
namespace Wiring {
  bool connectWiring();
}
}

namespace Flexus {
namespace Core {
namespace aux_ {

  class ComponentManagerImpl : public ComponentManager {

    typedef std::vector< boost::function< void (Flexus::Core::index_t ) > > instatiation_vector;
    std::vector< boost::function< void (Flexus::Core::index_t aSystemWidth ) > > theInstantiationFunctions;
    std::vector< ComponentInterface * > theComponents;
    Flexus::Core::index_t theSystemWidth;

  public:
    virtual ~ComponentManagerImpl() {}

    Flexus::Core::index_t systemWidth() const { return theSystemWidth; }


    void registerHandle( boost::function< void (Flexus::Core::index_t) > anInstantiator) {
      theInstantiationFunctions.push_back(anInstantiator);
    }

    void instantiateComponents(Flexus::Core::index_t aSystemWidth  ) {
      theSystemWidth = aSystemWidth;
      DBG_( Dev, ( << "Instantiating system with a width factor of: " << aSystemWidth ) );
      Flexus::Wiring::connectWiring();
      instatiation_vector::iterator iter = theInstantiationFunctions.begin();
      instatiation_vector::iterator end = theInstantiationFunctions.end();
      while (iter != end) {
       (*iter)( aSystemWidth );
        ++iter;
      }
    }

    void registerComponent( ComponentInterface * aComponent) {
      theComponents.push_back( aComponent );
    }

    void initComponents() {
      DBG_( Dev, ( << "Initalizing components..." ) );
			std::vector< ComponentInterface * >::iterator iter = theComponents.begin();
			std::vector< ComponentInterface * >::iterator end = theComponents.end();
		  while (iter != end){
	      DBG_( Dev, ( << "Initalizing " << (*iter)->name() ) );
		  	(*iter)->initialize(); 
		  	++iter;
		  }
    }

    bool isQuiesced() const {
      bool quiesced = true;
      std::for_each
        ( theComponents.begin()
        , theComponents.end()
        , ll::var(quiesced) = ll::var(quiesced) && ll::bind( &ComponentInterface::isQuiesced, ll::_1 )
        );
      return quiesced;
    }

    void doSave(std::string const & aDirectory) const {
      std::for_each
        ( theComponents.begin()
        , theComponents.end()
        , ll::bind( &ComponentInterface::saveState, ll::_1, aDirectory )
        );
    }

    void doLoad(std::string const & aDirectory) {
      std::vector< ComponentInterface * >::iterator iter, end;
      iter = theComponents.begin();
      end = theComponents.end();
      while (iter != end) {
        DBG_( Dev, ( << "Loading state: " << (*iter)->name() ) );
        (*iter)->loadState(aDirectory);
        ++iter;
      }
      DBG_( Crit, ( << " Done loading.") );
/*
      std::for_each
        ( theComponents.begin()
        , theComponents.end()
        , ll::bind( &ComponentInterface::loadState, ll::_1, aDirectory )
        );
*/
    }

  };


} //namespace aux_

boost::scoped_ptr<aux_::ComponentManagerImpl> theComponentManager(0);

ComponentManager & ComponentManager::getComponentManager() {
    if (theComponentManager == 0) {
      std::cerr << "Initializing Flexus::ComponentManager...";
      theComponentManager.reset(new aux_::ComponentManagerImpl());
      std::cerr << "done" << std::endl;
    }
    return *theComponentManager;
}


} //namespace Core
} //namespace Flexus

