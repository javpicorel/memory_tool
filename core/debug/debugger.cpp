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

#include <core/debug/debugger.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <map>
#include <utility>

namespace DBG_Cats {
  bool Core_debug_enabled = true;
  Flexus::Dbg::Category Core("Core", &Core_debug_enabled);
  bool Assert_debug_enabled = true;
  Flexus::Dbg::Category Assert("Assert", &Assert_debug_enabled);
  bool Stats_debug_enabled = true;
  Flexus::Dbg::Category Stats("Stats", &Stats_debug_enabled);
}

namespace Flexus {
namespace Dbg {

  using boost::lambda::_1;
  using boost::lambda::bind;
  using boost::lambda::var;
  using boost::lambda::delete_ptr;

Debugger::~Debugger() {
  std::for_each(theTargets.begin(), theTargets.end(), boost::lambda::delete_ptr()); //Clean up all pointers owned by theTargets
}

void Debugger::process(Entry const & anEntry) {
  std::for_each(theTargets.begin(), theTargets.end(), bind( &Target::process, _1, anEntry) );
}

void Debugger::printConfiguration(std::ostream & anOstream) {
  std::for_each(theTargets.begin(), theTargets.end(), bind( &Target::printConfiguration, _1, var(anOstream), std::string()) );
}

void Debugger::add(Target * aTarget) { //Ownership assumed by Debugger
  theTargets.push_back(aTarget); //Ownership assumed by theTargets
}

void Debugger::registerCategory(std::string const & aCategory, bool * aSwitch) {
  theCategories.insert( std::make_pair(aCategory, aSwitch) );
}

bool Debugger::setAllCategories(bool aValue) {
  std::map<std::string, bool *>::iterator iter, end;
  iter = theCategories.begin();
  end = theCategories.end();
  while (iter != end) {
    if (iter->first != "Assert" && iter->second != 0) {
      *(iter->second) = aValue;
    }
    ++iter;
  }
  return true;
}

bool Debugger::setCategory(std::string const & aCategory, bool aValue) {
  if (aCategory == "all") {
    return setAllCategories( aValue );
  }
  std::map<std::string, bool *>::iterator iter;
  iter = theCategories.find(aCategory);
  if (iter == theCategories.end()) {
    return false; //no such category
  }
  *(iter->second) = aValue;
  return true;
}

void Debugger::listCategories(std::ostream & anOstream) {
  std::map<std::string, bool *>::iterator iter, end;
  iter = theCategories.begin();
  end = theCategories.end();
  while (iter != end) {
    if (iter->second != 0) {
      if (* iter->second ) {
        anOstream << "   " << std::left << std::setw(40) << iter->first << "[enabled]\n";
      } else {
        anOstream << "   " << std::left << std::setw(40) << iter->first << "[disabled]\n";
      }
    }
    ++iter;
  }
}

void Debugger::registerComponent(std::string const & aComponent, unsigned int anIndex, bool * aSwitch) {
  std::map<std::string, std::vector< bool *> >::iterator iter;
  iter = theComponents.find(aComponent);
  if (iter == theComponents.end()) {
    bool ignored;
    boost::tie(iter,ignored) = theComponents.insert( std::make_pair( aComponent, std::vector<bool *>(anIndex+1,0) ));
  } else if (iter->second.size() < anIndex+1) {
    iter->second.resize(anIndex+1,0);
  }
  (iter->second)[anIndex] = aSwitch;
}

bool Debugger::setAllComponents(int anIndex, bool aValue) {
  std::map<std::string, std::vector<bool *> >::iterator iter, end;
  iter = theComponents.begin();
  end = theComponents.end();
  while (iter != end) {
    if (anIndex == -1) {
      for (unsigned i = 0; i < iter->second.size(); ++i) {
        *((iter->second)[i]) = aValue;
      }
    } else if (static_cast<unsigned>(anIndex) < iter->second.size()) {
      *((iter->second)[anIndex]) = aValue;
    } //Ignores out of raange indices
    ++iter;
  }
  return true;
}

bool Debugger::setComponent(std::string const & aCategory, int anIndex, bool aValue) {
  if (aCategory == "all") {
    return setAllComponents( anIndex, aValue );
  }
  std::map<std::string, std::vector<bool *> >::iterator iter;
  iter = theComponents.find(aCategory);
  if (iter == theComponents.end()) {
    return false; //no such component
  }
  if (anIndex == -1) {
    for (unsigned i = 0; i < iter->second.size(); ++i) {
      *((iter->second)[i]) = aValue;
    }
  } else if (static_cast<unsigned>(anIndex) < iter->second.size()) {
    *((iter->second)[anIndex]) = aValue;
  } else {
    return false; //anIndex out of range
  }
  return true;
}

void Debugger::listComponents(std::ostream & anOstream) {
  std::map<std::string, std::vector<bool *> >::iterator iter, end;
  iter = theComponents.begin();
  end = theComponents.end();
  anOstream << "   " << std::left << std::setw(40) << "Component" << "0-------8-------F\n";
  while (iter != end) {
    if (iter->second.size() > 0) {
      anOstream << "   " << std::setw(40) << iter->first;
      for (unsigned i = 0; i < iter->second.size(); ++i) {
        if ((iter->second)[i] != 0) {
          if (* ((iter->second)[i]) ) {
            anOstream << "X";
          } else {
            anOstream << ".";
          }
        }
      }
      anOstream << "\n";
    }
    ++iter;
  }
}

void Debugger::addAt(long long aCycle, Action * anAction) { //Ownership assumed by Debugger
  theAts.push( At(aCycle, anAction) );
}

void Debugger::checkAt() {
  if (!theAts.empty() && theAts.top().theCycle == cycleCount()) {
     doNextAt();
  }
}

void Debugger::doNextAt() {
  Entry entry = Entry(
      /*Severity*/ Flexus::Dbg::SevCrit
    , /*File*/ ""
    , /*Line*/ 0
    , /*Function*/ ""
    , /*GlobalCount*/ Flexus::Dbg::Debugger::theDebugger->count()
    , /*CycleCount*/ Flexus::Dbg::Debugger::theDebugger->cycleCount()
    );
  theAts.top().theAction->process(entry);
  delete theAts.top().theAction;
  theAts.pop();
}

void Debugger::reset() {
  std::for_each(theTargets.begin(), theTargets.end(), boost::lambda::delete_ptr()); //Clean up all pointers owned by theTargets
  theTargets.clear();
}

Debugger * Debugger::theDebugger( (Debugger *) 0);


void Debugger::constructDebugger() {
  if (theDebugger == 0) {
    theDebugger = new Debugger();
    theDebugger->initialize();
  }
}

Category::Category(std::string const & aName, bool * aSwitch, bool anIsDynamic)
   : theName(aName)
   , theIsDynamic(anIsDynamic) {
      theNumber = CategoryMgr::categoryMgr().addCategory(*this);
      if (! anIsDynamic) {
        Debugger::theDebugger->registerCategory(aName, aSwitch);
      }
}

std::string const & toString(Severity aSeverity) {
    static std::string theStaticSeverities[] =
      { "Invocations"
      , "VeryVerbose"
      , "Verbose"
      , "Interface"
      , "Trace"
      , "Development"
      , "Critical"
      , "Temp"
      };
    return theStaticSeverities[aSeverity];
}

} //Dbg
} //Flexus
