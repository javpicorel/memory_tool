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

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include <sys/stat.h>
#include <sys/types.h>

#include <core/target.hpp>


#include <core/performance/profile.hpp>
#include <core/debug/debug.hpp>
#include <core/configuration.hpp>
#include <core/component.hpp>


#include <boost/function.hpp>

#include <core/metaprogram.hpp>
#include <core/drive_reference.hpp>

#include <core/stats.hpp>

#include <core/simics/configuration_api.hpp>
#include <core/exception.hpp>

#include <core/boost_extensions/padded_string_cast.hpp>


#define __STDC_CONSTANT_MACROS
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lambda/lambda.hpp>

#include <core/flexus.hpp>

#include <core/simics/api_wrappers.hpp>
#include <core/simics/control_api.hpp>
#include <core/simics/mai_api.hpp>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>


namespace Flexus {
namespace Core {

using Flexus::Wiring::theDrive;

class FlexusImpl : public FlexusInterface {
   private:
    unsigned long long theWatchdogTimeout;
    std::vector<unsigned long> theWatchdogCounts;
    unsigned long theNumWatchdogs;
    bool theInitialized;
    unsigned long long theCycleCount;
    unsigned long long theStatInterval;
    unsigned long long theRegionInterval;
    unsigned long long theProfileInterval;
    unsigned long long theTimestampInterval;
    unsigned long long theStopCycle;
    Stat::StatCounter theCycleCountStat;

    std::string theCurrentStatRegionName;
    unsigned long theCurrentStatRegion;

    typedef std::vector<boost::function< void ()> > void_fn_vector;
    void_fn_vector theTerminateFunctions;

    bool theWatchdogWarning;
    bool theQuiesceRequested;
    bool theSaveRequested;
    std::string theSaveName;

    bool theFastMode;

    int theBreakCPU;
    unsigned long long theBreakInsn;
    int theSaveCtr;

   public:

    //Initialization functions
      void initializeComponents();

    //The main cycle function
      void doCycle();
      void advanceCycles(long long aCycleCount);
      void invokeDrives();

    //Simulator state inquiry
      bool isFastMode() const {
        return theFastMode;
      }
      bool isQuiesced() const {
        if ( ! initialized() ) { return true; }
        return ComponentManager::getComponentManager().isQuiesced();
      }
      bool quiescing() const {
        return theQuiesceRequested;
      }
      unsigned long long cycleCount() const {
        return theCycleCount;
      }
      bool initialized() const { return theInitialized; }

    //Watchdog Functions
      void setWatchdogTimeout(std::string const & aTimeoutStr);
      void watchdogCheck();
      void watchdogIncrement();
      void watchdogReset(unsigned int anIndex);

    //Debugging support functions
      int breakCPU() const {
        return theBreakCPU;
      }
      int breakInsn() const {
        return theBreakInsn;
      }

    //Flexus command line interface
    void printCycleCount();
    void setStopCycle(std::string const & aValue);
    void setStatInterval(std::string const & aValue);
    void setRegionInterval(std::string const & aValue);
    void setBreakCPU(int aCPU);
    void setBreakInsn(std::string const & aValue);
    void setProfileInterval(std::string const & aValue);
    void setTimestampInterval(std::string const & aValue);
    void printProfile();
    void resetProfile();
    void writeProfile(std::string const & aFilename);
    void printConfiguration();
    void writeConfiguration(std::string const & aFilename);
    void parseConfiguration(std::string const & aFilename);
    void setConfiguration(std::string const & aName, std::string const & aValue);
    void printMeasurement(std::string const & aMeasurement);
    void listMeasurements();
    void writeMeasurement(std::string const & aMeasurement, std::string const & aFilename);
    void enterFastMode();
    void leaveFastMode();
    void quiesce();
    void quiesceAndSave(unsigned int aSaveNum);
    void quiesceAndSave();
    void saveState(std::string const & aDirName);
    void saveJustFlexusState(std::string const & aDirName);
    void loadState(std::string const & aDirName);
    void doLoad(std::string const & aDirName);
    void doSave(std::string const & aDirName, bool justFlexus = false);
    void backupStats(std::string const & aFilename) const;
    void saveStats(std::string const & aFilename) const;
    void saveStatsUncompressed(std::ofstream & anOstream) const;
    void saveStatsCompressed(std::ofstream & anOstream) const;
    void reloadDebugCfg();
    void addDebugCfg(std::string const & aFilename);
    void setDebug(std::string const & aDebugSeverity);
    void enableCategory(std::string const & aComponent);
    void disableCategory(std::string const & aComponent);
    void listCategories();
    void enableComponent(std::string const & aComponent, std::string const & anIndex);
    void disableComponent(std::string const & aComponent, std::string const & anIndex);
    void listComponents();
    void printDebugConfiguration();
    void writeDebugConfiguration(std::string const & aFilename);
    void onTerminate( boost::function<void () > );
    void terminateSimulation();
    void log(std::string const & aName, std::string const & anInterval, std::string const & aRegEx);
    void printMMU(int aCPU);

   public:
    FlexusImpl(Simics::API::conf_object_t * anObject )
      : theWatchdogTimeout(100000)
      , theNumWatchdogs(0)
      , theInitialized(false)
      , theCycleCount(0)
      , theStatInterval(1000000)
      , theRegionInterval(100000000)
      , theProfileInterval(1000000)
      , theTimestampInterval(100000)
      , theStopCycle(0)
      , theCycleCountStat("sys-cycles")
      , theWatchdogWarning(false)
      , theQuiesceRequested(false)
      , theSaveRequested(false)
      , theFastMode(false)
      , theBreakCPU(-1)
      , theBreakInsn(0)
      , theSaveCtr(1)
      {
        Flexus::Dbg::Debugger::theDebugger->connectCycleCount(&theCycleCount);
    }
    virtual ~FlexusImpl() {}
};


void FlexusImpl::printMMU( int aCPU ) {
   Flexus::Simics::Processor::getProcessor(aCPU)->dumpMMU();
   Flexus::Simics::Processor::getProcessor(aCPU)->validateMMU();
}


void FlexusImpl::initializeComponents() {
  Stat::getStatManager()->initialize();
  theCurrentStatRegion = 0;
  theCurrentStatRegionName = std::string("Region ") + boost::padded_string_cast<3,'0'>(theCurrentStatRegion++);
  Stat::getStatManager()->openMeasurement(theCurrentStatRegionName);
  ConfigurationManager::getConfigurationManager().checkAllOverrides();
  ComponentManager::getComponentManager().initComponents();
  writeConfiguration("configuration.out");
  theInitialized = true;
}

void FlexusImpl::advanceCycles(long long aCycleCount) {

    theCycleCount += aCycleCount;
    theCycleCountStat += aCycleCount;

    if (theQuiesceRequested && isQuiesced() ) {
      DBG_( Crit, ( << "Flexus is quiesced as of cycle " << theCycleCount ) );
      theQuiesceRequested = false;
      if (theSaveRequested) {
        theSaveRequested = false;
        doSave( theSaveName );
      } else {
        Simics::BreakSimulation( "Flexus is quiesced." );
        return;
      }
    }

    //Check how much time has elapsed every 1024*1024 cycles
    static unsigned long long last_timestamp = 0;
    if ( !theCycleCount || theCycleCount - last_timestamp >= theTimestampInterval ) {
      boost::posix_time::ptime now(boost::posix_time::second_clock::local_time());
      DBG_(Dev, Core() ( << "Timestamp: " << boost::posix_time::to_simple_string(now)));
      last_timestamp = theCycleCount;
    }

    if( (theStopCycle > 0) && (theCycleCount >= theStopCycle) ) {
      DBG_(Dev, ( << "Reached target cycle count. Ending simulation.") );
      terminateSimulation();
    }

    static unsigned long long last_stats = 0;
    if (theCycleCount - last_stats >= theStatInterval) {
      DBG_(Dev, Core() ( << "Saving stats at: " << theCycleCount));
      backupStats("stats_db");

      writeMeasurement("all","all.measurement.out");

      last_stats = theCycleCount;
    }

    static unsigned long long last_region = 0;
    if (theCycleCount - last_region >= theRegionInterval ) {
      Stat::getStatManager()->closeMeasurement(theCurrentStatRegionName);
  	  theCurrentStatRegionName = std::string("Region ") + boost::padded_string_cast<3,'0'>(theCurrentStatRegion++);
      Stat::getStatManager()->openMeasurement(theCurrentStatRegionName);

      last_region = theCycleCount;
    }

    static unsigned long long last_profile = 0;
    if (theProfileInterval > 0 && theCycleCount - last_profile >= theProfileInterval) {
      //DBG_(Dev, Core() ( << "Writing profile at: " << theCycleCount));
      DBG_(Dev, Core() ( << "Profiling disabled" ) );

      //writeProfile("profile.out");
      //resetProfile();
      last_profile = theCycleCount;
    }

    Flexus::Dbg::Debugger::theDebugger->checkAt();

    Stat::getStatManager()->tick(aCycleCount);

}

void FlexusImpl::invokeDrives() {
    theDrive.doCycle();
}

void FlexusImpl::doCycle() {
  FLEXUS_PROFILE();
    DBG_(VVerb, Core() ( << "Start of Cycle" ) );

    advanceCycles(1);

    unsigned long recent_watchdog_count = ((unsigned long)theCycleCount) & 0xFF;
    if ( recent_watchdog_count == 0) {
      //Check for watchdog timeout
      watchdogCheck();
      watchdogIncrement();
    }

    invokeDrives();

    DBG_(VVerb, Core() ( << "End of Cycle" ) );

}

void FlexusImpl::setWatchdogTimeout(std::string const & aTimeoutStr) {
  std::istringstream ss(aTimeoutStr, std::istringstream::in);
  ss >> theWatchdogTimeout;
}

void FlexusImpl::watchdogCheck() {
  for (unsigned int i = 0; i < theNumWatchdogs; ++i) {
    //We get 10k cycles of Iface trace after a watchdog timeout before we assert and kill Flexus
    if (!( theWatchdogCounts[i] < theWatchdogTimeout)) {
      if (! theWatchdogWarning) {
        theWatchdogWarning = true;
        DBG_( Crit, ( << "Watchdog timer expired.  No progress by CPU " << i << " for  " << theWatchdogCounts[i] << "cycles") ) ;
        Flexus::Dbg::Debugger::theDebugger->setMinSev(Dbg::Severity( DBG_internal_Sev_to_int( Iface )  ));
      }
    }
    DBG_Assert( theWatchdogCounts[i] < theWatchdogTimeout + 10, Core() ( << "Watchdog timer expired.  No progress by CPU " << i << " for  " << theWatchdogCounts[i] << "cycles") ) ;
  }
}

void FlexusImpl::watchdogIncrement() {
  std::for_each(theWatchdogCounts.begin(), theWatchdogCounts.end(), boost::lambda::_1 += 255 );
}

void FlexusImpl::watchdogReset(unsigned int anIndex) {
    if (anIndex >= theNumWatchdogs) {
      theNumWatchdogs = anIndex + 1;
      theWatchdogCounts.resize(theNumWatchdogs, 0);
    } else {
      theWatchdogCounts[anIndex] = 0;
    }
}

void FlexusImpl::printCycleCount() {
  DBG_(Crit, Cat(Stats) Set( (Source) << "flexus") ( << "Cycle count: " << theCycleCount ) );
}

void FlexusImpl::setStopCycle(std::string const & aValue) {
  theStopCycle = boost::lexical_cast<unsigned long long>(aValue);
}

void FlexusImpl::setStatInterval(std::string const & aValue) {
  theStatInterval = boost::lexical_cast<unsigned long long>(aValue);
  DBG_(Dev, Set( (Source) << "flexus") ( << "Set stat interval to : " << theStatInterval) );
}

void FlexusImpl::setRegionInterval(std::string const & aValue) {
  theRegionInterval = boost::lexical_cast<unsigned long long>(aValue);
  DBG_(Dev, Set( (Source) << "flexus") ( << "Set region interval to : " << theRegionInterval) );
}

void FlexusImpl::setBreakCPU(int aCPU) {
  theBreakCPU = aCPU;
  DBG_(Dev, Set( (Source) << "flexus") ( << "Set break CPU to : " << aCPU) );
}

void FlexusImpl::setBreakInsn(std::string const & aValue) {
  theBreakInsn = boost::lexical_cast<unsigned long long>(aValue);
  DBG_(Dev, Set( (Source) << "flexus") ( << "Set break instruction # to : " << theBreakInsn ) );
}

void FlexusImpl::setProfileInterval(std::string const & aValue) {
  theProfileInterval = boost::lexical_cast<unsigned long long>(aValue);
  DBG_(Dev, Set( (Source) << "flexus") ( << "Set profile interval to : " << theProfileInterval) );
}

void FlexusImpl::setTimestampInterval(std::string const & aValue) {
  theTimestampInterval = boost::lexical_cast<unsigned long long>(aValue);
  DBG_(Dev, Set( (Source) << "flexus") ( << "Set timestamp interval to : " << theTimestampInterval) );
}

void FlexusImpl::enterFastMode() {
  if (! theFastMode) {
    theFastMode = true;
    DBG_(Dev, Set( (Source) << "flexus") ( << "Fast Mode enabled") );
  }
}

void FlexusImpl::leaveFastMode() {
  if (theFastMode) {
    theFastMode = false;
    DBG_(Dev, Set( (Source) << "flexus") ( << "Fast Mode disabled") );
  }
}

void FlexusImpl::printProfile() {
  nProfile::ProfileManager::profileManager()->report(std::cout);
}

void FlexusImpl::resetProfile() {
  nProfile::ProfileManager::profileManager()->reset();
}

void FlexusImpl::writeProfile(std::string const & aFilename) {
  std::ofstream out(aFilename.c_str(), std::ios::app);
  out << "Profile as of " << theCycleCount << std::endl;
  nProfile::ProfileManager::profileManager()->report(out);
  out << std::endl;
}

void FlexusImpl::printConfiguration() {
  ConfigurationManager::getConfigurationManager().printConfiguration(std::cout);
}

void FlexusImpl::writeConfiguration(std::string const & aFilename) {
  std::ofstream out(aFilename.c_str());
  ConfigurationManager::getConfigurationManager().printConfiguration(out);
}

void FlexusImpl::parseConfiguration(std::string const & aFilename) {
  std::ifstream in(aFilename.c_str());
  ConfigurationManager::getConfigurationManager().parseConfiguration(in);
}

void FlexusImpl::setConfiguration(std::string const & aName, std::string const & aValue) {
  ConfigurationManager::getConfigurationManager().set(aName, aValue);
}

void FlexusImpl::printMeasurement(std::string const & aMeasurement) {
  Stat::getStatManager()->printMeasurement(aMeasurement, std::cout);
}

void FlexusImpl::listMeasurements() {
  Stat::getStatManager()->listMeasurements(std::cout);
}

void FlexusImpl::log(std::string const & aName, std::string const & anInterval, std::string const & aRegEx) {
  unsigned long long interval = boost::lexical_cast<unsigned long long>(anInterval);
  DBG_(Dev, Set( (Source) << "flexus") ( << "Logging: " << aRegEx << " every " << anInterval << " cycles as measurement " << aName << " to " << aName + ".out" ) );
  std::string filename(aName + ".out");
  std::ofstream * log =  new std::ofstream( filename.c_str()); //Leaks intentionally
  Stat::getStatManager()->openLoggedPeriodicMeasurement(aName.c_str(), interval, Stat::Accumulate, *log, aRegEx.c_str());
}

void FlexusImpl::writeMeasurement(std::string const & aMeasurement, std::string const & aFilename) {
  std::ofstream out(aFilename.c_str());
  Stat::getStatManager()->printMeasurement(aMeasurement, out);
  out.close();
}

void FlexusImpl::quiesce() {
  if (! isQuiesced() ) {
    DBG_( Crit, ( << "Flexus will quiesce when simulation continues" ) );
    theQuiesceRequested = true;
  }
}

void FlexusImpl::quiesceAndSave() {
  if (! isQuiesced() ) {
    DBG_( Crit, ( << "Flexus will quiesce when simulation continues" ) );
    theQuiesceRequested = true;
    theSaveRequested = true;
    theSaveName = "newckpt-" + boost::padded_string_cast<4,'0'>(theSaveCtr);
  } else {
    doSave("newckpt-" + boost::padded_string_cast<4,'0'>(theSaveCtr));
  }
  ++theSaveCtr;
}

void FlexusImpl::quiesceAndSave(unsigned int aSaveNum) {
  if (! isQuiesced() ) {
    DBG_( Crit, ( << "Flexus will quiesce when simulation continues" ) );
    theQuiesceRequested = true;
    theSaveRequested = true;
    theSaveName = "ckpt-" + boost::padded_string_cast<4,'0'>(aSaveNum);
  } else {
    doSave("ckpt-" + boost::padded_string_cast<4,'0'>(aSaveNum));
  }
}

void FlexusImpl::saveState(std::string const & aDirName) {
  if ( isQuiesced() ) {
    doSave(aDirName);
  } else {
    DBG_( Crit, ( << "Flexus cannot save unless the system is quiesced. Use the command \"flexus.quiesce\" to quiesce the system" ) );
  }
}

void FlexusImpl::saveJustFlexusState(std::string const & aDirName) {
  if ( isQuiesced() ) {
    doSave(aDirName, true);
  } else {
    DBG_( Crit, ( << "Flexus cannot save unless the system is quiesced. Use the command \"flexus.quiesce\" to quiesce the system" ) );
  }
}

void FlexusImpl::loadState(std::string const & aDirName) {
  if ( isQuiesced() ) {
    doLoad(aDirName);
  } else {
    DBG_( Crit, ( << "Flexus cannot load unless the system is quiesced. Use the command \"flexus.quiesce\" to quiesce the system" ) );
  }
}

void FlexusImpl::doLoad(std::string const & aDirName) {
  DBG_( Crit, ( << "Loading Flexus state from subdirectory " << aDirName ) );

  if (! initialized() ) {
    initializeComponents();
  }
  ComponentManager::getComponentManager().doLoad( aDirName );
}

void FlexusImpl::doSave(std::string const & aDirName, bool justFlexus) {
	if (justFlexus) {
    DBG_( Crit, ( << "Saving Flexus state in subdirectory " << aDirName ) );
	} else {	
    DBG_( Crit, ( << "Saving Flexus and Simics state in subdirectory " << aDirName ) );
	}	
  mkdir(aDirName.c_str(), 0777);
	if (!justFlexus) {
    std::string simics_cfg_name(aDirName);
    simics_cfg_name += "/simics-state";
    Simics::WriteCheckpoint(simics_cfg_name.c_str());
	}
  ComponentManager::getComponentManager().doSave( aDirName );
}

void FlexusImpl::backupStats(std::string const & aFilename) const {
  std::string fullName = aFilename + std::string(".out.gz");
  std::string last1Name = aFilename + std::string(".001.out.gz");
  remove(last1Name.c_str());
  rename(fullName.c_str(), last1Name.c_str());

  saveStats(fullName);

  remove(last1Name.c_str());
}

void FlexusImpl::saveStats(std::string const & aFilename) const {
  std::ofstream anOstream(aFilename.c_str(), std::ios::binary);
  size_t loc = aFilename.rfind(".gz");
  if(loc == std::string::npos) {
    saveStatsUncompressed(anOstream);
  }  else {
    saveStatsCompressed(anOstream);
  }
}

void FlexusImpl::saveStatsUncompressed(std::ofstream & anOstream) const {
  Stat::getStatManager()->save(anOstream);
  anOstream.close();
}

void FlexusImpl::saveStatsCompressed(std::ofstream & anOstream) const {
  boost::iostreams::filtering_ostream out;
  out.push(boost::iostreams::gzip_compressor());
  out.push(anOstream);
  Stat::getStatManager()->save(out);
  out.reset();
}

void FlexusImpl::reloadDebugCfg() {
  Flexus::Dbg::Debugger::theDebugger->reset();
  Flexus::Dbg::Debugger::theDebugger->initialize();
}

void FlexusImpl::addDebugCfg(std::string const & aFilename) {
  Flexus::Dbg::Debugger::theDebugger->addFile(aFilename);
}

void FlexusImpl::setDebug(std::string const & aDebugSeverity) {
  if ( aDebugSeverity.compare("crit") == 0 || aDebugSeverity.compare("critical") == 0 ) {
    Flexus::Dbg::Debugger::theDebugger->setMinSev(Dbg::Severity( DBG_internal_Sev_to_int( Crit )  ));
    DBG_(Dev, ( << "Switched to Crit debugging.") );
  } else if ( aDebugSeverity.compare("dev") == 0 || aDebugSeverity.compare("development") == 0 ) {
    Flexus::Dbg::Debugger::theDebugger->setMinSev(Dbg::Severity( DBG_internal_Sev_to_int( Dev )  ));
    DBG_(Dev, ( << "Switched to Dev debugging.") );
  } else if ( aDebugSeverity.compare("trace") == 0 ) {
    Flexus::Dbg::Debugger::theDebugger->setMinSev(Dbg::Severity( DBG_internal_Sev_to_int( Trace )  ));
    DBG_(Dev, ( << "Switched to Trace debugging.") );
  } else if ( aDebugSeverity.compare("iface") == 0 || aDebugSeverity.compare("interface") == 0 ) {
    Flexus::Dbg::Debugger::theDebugger->setMinSev(Dbg::Severity( DBG_internal_Sev_to_int( Iface )  ));
    DBG_(Dev, ( << "Switched to Iface debugging.") );
  } else if ( aDebugSeverity.compare("verb") == 0 || aDebugSeverity.compare("verbose") == 0 ) {
    Flexus::Dbg::Debugger::theDebugger->setMinSev(Dbg::Severity( DBG_internal_Sev_to_int( Verb )  ));
    DBG_(Dev, ( << "Switched to Verb debugging.") );
  } else if ( aDebugSeverity.compare("vverb") == 0 || aDebugSeverity.compare("veryverbose") == 0 ) {
    Flexus::Dbg::Debugger::theDebugger->setMinSev(Dbg::Severity( DBG_internal_Sev_to_int( VVerb )  ));
    DBG_(Dev, ( << "Switched to VVerb debugging.") );
  } else if ( aDebugSeverity.compare("inv") == 0 || aDebugSeverity.compare("invocation") == 0 ) {
    Flexus::Dbg::Debugger::theDebugger->setMinSev(Dbg::Severity( DBG_internal_Sev_to_int( Inv )  ));
    DBG_(Dev, ( << "Switched to Inv debugging.") );
  } else {
    std::cout << "Unknown debug severity: " << aDebugSeverity << ". Severity unchanged." << std::endl;
  }
}

void FlexusImpl::enableCategory(std::string const & aCategory) {
  if ( Flexus::Dbg::Debugger::theDebugger->setCategory( aCategory, true) ) {
    DBG_(Dev, ( <<  "Enabled debugging for " << aCategory ) );
  } else {
    DBG_(Dev, ( <<  "No category named " << aCategory ) );
  }
}
void FlexusImpl::disableCategory(std::string const & aCategory) {
  if ( Flexus::Dbg::Debugger::theDebugger->setCategory( aCategory, false) ) {
    DBG_(Dev, ( <<  "Disabled debugging for " << aCategory ) );
  } else {
    DBG_(Dev, ( <<  "No category named " << aCategory ) );
  }
}

void FlexusImpl::listCategories() {
  Flexus::Dbg::Debugger::theDebugger->listCategories(std::cout);
}
void FlexusImpl::enableComponent(std::string const & aComponent, std::string const & anIndexStr) {
  int anIndex = -1;
  if (anIndexStr != "all") {
    try {
      anIndex = boost::lexical_cast<int>(anIndexStr);
    } catch (boost::bad_lexical_cast & ignored) {
      DBG_(Dev, ( <<  "Invalid component index " << anIndexStr ) );
      return;
    }
  }
  if ( Flexus::Dbg::Debugger::theDebugger->setComponent( aComponent, anIndex, true) ) {
    DBG_(Dev, ( <<  "Enabled debugging for " << aComponent << "[" << anIndexStr << "]" ) );
  } else {
    DBG_(Dev, ( <<  "No such component " << aComponent << "[" << anIndexStr << "]" ) );
  }
}
void FlexusImpl::disableComponent(std::string const & aComponent, string const & anIndexStr) {
  int anIndex = -1;
  if (anIndexStr != "all") {
    try {
      anIndex = boost::lexical_cast<int>(anIndexStr);
    } catch (boost::bad_lexical_cast & ignored) {
      DBG_(Dev, ( <<  "Invalid component index " << anIndexStr ) );
      return;
    }
  }
  if ( Flexus::Dbg::Debugger::theDebugger->setComponent( aComponent, anIndex, false) ) {
    DBG_(Dev, ( <<  "Disabled debugging for " << aComponent << "[" << anIndexStr << "]" ) );
  } else {
    DBG_(Dev, ( <<  "No such component " << aComponent << "[" << anIndexStr << "]" ) );
  }
}
void FlexusImpl::listComponents() {
  Flexus::Dbg::Debugger::theDebugger->listComponents(std::cout);
}

void FlexusImpl::printDebugConfiguration() {
  Flexus::Dbg::Debugger::theDebugger->printConfiguration(std::cout);
}

void FlexusImpl::writeDebugConfiguration(std::string const & aFilename) {
  std::ofstream out(aFilename.c_str());
  Flexus::Dbg::Debugger::theDebugger->printConfiguration(out);
}


void FlexusImpl::onTerminate(boost::function<void () > aFn) {
  theTerminateFunctions.push_back(aFn);
}

void FlexusImpl::terminateSimulation() {
  boost::posix_time::ptime now(boost::posix_time::second_clock::local_time());
  DBG_(Dev, Core() ( << "Terminating simulation. Timestamp: " << boost::posix_time::to_simple_string(now)));
  DBG_(Dev, Core() ( << "Saving final stats_db."));
  ;
  for
    ( void_fn_vector::iterator iter = theTerminateFunctions.begin()
    ; iter != theTerminateFunctions.end()
    ; ++iter
    ) {
      (*iter)();
  }

  backupStats("stats_db");
  writeMeasurement("all","all.measurement.out");
  Flexus::Stat::getStatManager()->finalize();
  Flexus::Simics::BreakSimulation("Simulation terminated by flexus.");
}

class Flexus_Obj : public Simics::AddInObject<FlexusImpl> {
    typedef Simics::AddInObject<FlexusImpl> base;
   public:
    static const Simics::Persistence  class_persistence = Simics::Session;
    //These constants are defined in Simics/simics.cpp
    static std::string className() { return "Flexus"; }
    static std::string classDescription() { return "Flexus main class"; }

    Flexus_Obj() : base() {}
    Flexus_Obj(Simics::API::conf_object_t * anObject) : base(anObject) {}
    Flexus_Obj(FlexusImpl * anImpl) : base(anImpl) {}

    template <class Class>
    static void defineClass(Class & aClass) {

      //Statistics-related commands
      aClass.addCommand
        ( & FlexusImpl::listMeasurements
        , "list-measurements"
        , "List all available measurments"
        );

      aClass.addCommand
        ( & FlexusImpl::printMeasurement
        , "print-measurement"
        , "Print out all stats in the specified measurement"
        , "measurement"
        );

      aClass.addCommand
        ( & FlexusImpl::writeMeasurement
        , "write-measurement"
        , "Print out all stats in the specified measurement"
        , "measurement"
        , "filename"
        );

      aClass.addCommand
        ( & FlexusImpl::log
        , "log"
        , "log stats to a logged measurement"
        , "name"
        , "interval"
        , "regex"
        );

      aClass.addCommand
        ( & FlexusImpl::printCycleCount
        , "print-cycle-count"
        , "Print out the Flexus timing-module cycle-count"
        );

      aClass.addCommand
        ( & FlexusImpl::saveStats
        , "save-stats"
        , "Save statistics database"
        , "filename"
        );

      aClass.addCommand
        ( & FlexusImpl::setStatInterval
        , "set-stat-interval"
        , "Interval between writing stats to disk"
        , "value"
        );

      aClass.addCommand
        ( & FlexusImpl::setRegionInterval
        , "set-region-interval"
        , "Interval between stats regions"
        , "value"
        );

      //State saving/loading commands
      aClass.addCommand
        ( & FlexusImpl::saveState
        , "save-state"
        , "Write out a checkpoint of Flexus state"
        , "dirname"
        );

      aClass.addCommand
        ( & FlexusImpl::saveJustFlexusState
        , "save-just-flexus-state"
        , "Write out a checkpoint of JUST Flexus state"
        , "dirname"
        );

      aClass.addCommand
        ( & FlexusImpl::loadState
        , "load-state"
        , "Read in a checkpoint of Flexus state"
        , "dirname"
        );

      aClass.addCommand
        ( & FlexusImpl::quiesce
        , "quiesce"
        , "quiesce Flexus"
        );

      aClass.addCommand
        ( & FlexusImpl::terminateSimulation
        , "terminate"
        , "terminate Flexus"
        );


      //Profiling commands
      aClass.addCommand
        ( & FlexusImpl::setProfileInterval
        , "set-profile-interval"
        , "Interval for collecting profile data"
        , "value"
        );

      aClass.addCommand
        ( & FlexusImpl::printProfile
        , "print-profile"
        , "Print out the Flexus execution profile"
        );

      aClass.addCommand
        ( & FlexusImpl::writeProfile
        , "write-profile"
        , "Write the Flexus execution profile to a file"
        , "filename"
        );

      aClass.addCommand
        ( & FlexusImpl::resetProfile
        , "reset-profile"
        , "Clear execution profile counters"
        );


      //Configuration-related commands
      aClass.addCommand
        ( & FlexusImpl::printConfiguration
        , "print-configuration"
        , "Print out the Flexus configuration"
        );

      aClass.addCommand
        ( & FlexusImpl::writeConfiguration
        , "write-configuration"
        , "Write the Flexus configuration to a file"
        , "filename"
        );

      aClass.addCommand
        ( & FlexusImpl::parseConfiguration
        , "parse-configuration"
        , "Parse a Flexus configuration file"
        , "filename"
        );

      aClass.addCommand
        ( & FlexusImpl::setConfiguration
        , "set"
        , "set a configuration parameter"
        , "parameter"
        , "value"
        );

    //Simulation control commands
      aClass.addCommand
        ( & FlexusImpl::setStopCycle
        , "set-stop-cycle"
        , "Cycle to terminate simulation"
        , "value"
        );

      aClass.addCommand
        ( & FlexusImpl::enterFastMode
        , "fast-mode"
        , "Enter fast mode"
        );

      aClass.addCommand
        ( & FlexusImpl::leaveFastMode
        , "normal-mode"
        , "Enter normal execution mode"
        );

    //Debugging related commands
      aClass.addCommand
	( & FlexusImpl::setWatchdogTimeout
        , "set-watchdog-timeout"
	, "Set the watchdog timeout value"
        , "timeout"
	);


      aClass.addCommand
        ( & FlexusImpl::reloadDebugCfg
        , "debug-reload-cfg"
        , "Reprocess debug.cfg"
        );

      aClass.addCommand
        ( & FlexusImpl::addDebugCfg
        , "debug-add-cfg"
        , "Parse an additional debug cfg script"
        , "filename"
        );

      aClass.addCommand
        ( & FlexusImpl::setDebug
        , "debug-set-severity"
        , "Set debug Severity"
        , "severity"
        );

      aClass.addCommand
        ( & FlexusImpl::enableCategory
        , "debug-enable-category"
        , "Enable debugging for the specified category (all of a DBG_ statements categories must be enabled for it to generate output)"
        , "category"
        );

      aClass.addCommand
        ( & FlexusImpl::disableCategory
        , "debug-disable-category"
        , "Disable debugging for the specified category (all of a DBG_ statements categories must be enabled for it to generate output)"
        , "category"
        );

      aClass.addCommand
        ( & FlexusImpl::listCategories
        , "debug-list-categories"
        , "Lists all debugging categories"
        );

      aClass.addCommand
        ( & FlexusImpl::enableComponent
        , "debug-enable-component"
        , "Enable debugging for the specified component"
        , "component"
        , "index"
        );

      aClass.addCommand
        ( & FlexusImpl::disableComponent
        , "debug-disable-component"
        , "Disable debugging for the specified component"
        , "component"
        , "index"
        );

      aClass.addCommand
        ( & FlexusImpl::listComponents
        , "debug-list-components"
        , "Lists all components"
        );

      aClass.addCommand
        ( & FlexusImpl::printDebugConfiguration
        , "debug-print-configuration"
        , "Print active debug filter configuration script"
        );

      aClass.addCommand
        ( & FlexusImpl::writeDebugConfiguration
        , "debug-write-configuration"
        , "Write active debug filter configuration script to file"
        , "filename"
        );

      aClass.addCommand
        ( & FlexusImpl::setBreakCPU
        , "set-break-cpu"
        , "Set the CPU to have an active break instruction"
        , "cpu"
        );

      aClass.addCommand
        ( & FlexusImpl::setBreakInsn
        , "set-break-instruction"
        , "Set the instruction number on which to break"
        , "insn"
        );

      aClass.addCommand
        ( & FlexusImpl::printMMU
        , "print-mmu"
        , "Print the contents of an MMU"
        , "cpu"
        );


    }
};

  typedef Simics::Factory< Flexus_Obj > FlexusFactory;
  Flexus_Obj theFlexusObj;
  FlexusFactory * theFlexusFactory;
  FlexusInterface * theFlexus = 0; //This is initialized from startup.cpp

void CreateFlexusObject() {
    theFlexusObj = Core::theFlexusFactory->create("flexus");
    theFlexus = &Core::theFlexusObj;
    if (!theFlexus) {
        DBG_Assert(false, ( << "Unable to create Flexus object in Simics"));
    }
}

void PrepareFlexusObject() {
    theFlexusFactory = new FlexusFactory();
}

}
}
