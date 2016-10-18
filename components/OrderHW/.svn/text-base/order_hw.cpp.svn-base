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

#include "common.hpp"
#include "coordinator.hpp"
#include "experiment.hpp"

#include <fstream>

#include <boost/tokenizer.hpp>

#include <boost/scoped_ptr.hpp>
#include <boost/optional.hpp>
#include <boost/none.hpp>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace l = boost::lambda;

#include <boost/utility.hpp>

#define __STDC_CONSTANT_MACROS
#include <boost/date_time/posix_time/posix_time.hpp>

#define BOOST_NO_WREGEX
#include <boost/regex.hpp>



class Option;
typedef std::map<std::string, Option *> option_map;
option_map theOptions;

struct Option {
  std::string theName;
  std::string theDescr;
  StatAnnotation theAnnotation;

  Option(std::string aName, std::string aDescr)
   : theName(aName)
   , theDescr(aDescr)
   , theAnnotation(std::string("Configuration.") + theName)
  {
    theOptions.insert( std::make_pair(aName, this) );
  }
  virtual ~Option() {}
  std::string const & name() { return theName; }
  virtual void assign(std::string aVal) = 0;
  virtual void report() = 0;
  friend std::ostream & operator << (std::ostream & anOstream, Option & anOption) {
    anOstream << anOption.theName << ": " << anOption.theDescr << std::endl;
    return anOstream;
  }
};

struct LongOption : public Option {
  long & theLong;
  LongOption(std::string aName, std::string aDescr, long & aLong)
   : Option(aName, aDescr)
   , theLong(aLong)
   {}
  void assign(std::string aVal) {
    try {
      theLong = boost::lexical_cast<long>(aVal);
    } catch (boost::bad_lexical_cast & e) {
      std::cout << "Unable to assign value " << aVal << " to option " << theName << std::endl;
      std::exit(1);
    }
  }
  void report() {
    theAnnotation = boost::lexical_cast<std::string>(theLong);
    DBG_(Dev, ( << "Cfg: " << theName << ": " << theLong) );
  }
};

struct ULLOption : public Option {
  unsigned long long & theULL;
  ULLOption(std::string aName, std::string aDescr, unsigned long long & aULL)
   : Option(aName, aDescr)
   , theULL(aULL)
   {}
  void assign(std::string aVal) {
    try {
      theULL = boost::lexical_cast<unsigned long long>(aVal);
    } catch (boost::bad_lexical_cast & e) {
      std::cout << "Unable to assign value " << aVal << " to option " << theName << std::endl;
      std::exit(1);
    }
  }
  void report() {
    theAnnotation = boost::lexical_cast<std::string>(theULL);
    DBG_(Dev, ( << "Cfg: " << theName << ": " << theULL) );
  }
};

struct TrueFalseOption : public Option {
  bool & theFlag;
  TrueFalseOption(std::string aName, std::string aDescr, bool & aFlag)
   : Option(aName, aDescr)
   , theFlag(aFlag)
   {}
  void assign(std::string aVal) {
    if (aVal == "true") {
      theFlag = true;
    } else if (aVal == "false") {
      theFlag = false;
    } else {
      std::cout << "Unable to assign value " << aVal << " to option " << theName << std::endl;
      std::exit(1);
    }
  }
  void report() {
    theAnnotation = (theFlag ? "true" : "false");
    DBG_(Dev, ( << "Cfg: " << theName << ": " << (theFlag ? "true" : "false")) );
  }
};


struct OptionalStringOption : public Option {
  boost::optional<std::string> & theString;
  OptionalStringOption(std::string aName, std::string aDescr, boost::optional<std::string> & aString)
   : Option(aName, aDescr)
   , theString(aString)
   {}
  void assign(std::string aVal) {
    if (aVal.length() == 0) {
      theString = boost::none;
    } else {
      theString = aVal;
    }
  }
  void report() {
    theAnnotation = (theString ? *theString : "<disabled>");
    DBG_(Dev, ( << "Cfg: " << theName << ": " << (theString ? *theString : "<disabled>")) );
  }
};

struct ChoiceOption : public Option {
  std::map<std::string, int> theChoices;
  std::string theTextVal;
  int & theChoice;
  ChoiceOption(std::string const & aName, std::string const & aDescr, std::map<std::string, int> & aChoices, int & aChoice, std::string const & aDefault)
   : Option(aName, aDescr)
   , theChoices(aChoices)
   , theTextVal(aDefault)
   , theChoice(aChoice)
   {}
  void assign(std::string aVal) {
    std::map<std::string, int>::iterator iter = theChoices.find(aVal);
    if (iter == theChoices.end()) {
      std::cout << "Invalid choice" << aVal << " for option " << theName << std::endl;
      std::exit(1);
    } else {
      theTextVal = aVal;
      theChoice = iter->second;
    }
  }
  void report() {
    theAnnotation = theTextVal;
    DBG_(Dev, ( << "Cfg: " << theName << ": " << theTextVal) );
  }

};

void printUsage() {
  option_map::iterator iter = theOptions.begin();
  option_map::iterator end = theOptions.end();
  std::cout << "Usage: " << std::endl;
  while (iter != end) {
    std::cout << "\t" << *iter->second;
    ++iter;
  }
}

void parseOptions(std::string options) {
  boost::regex options_parser("(.*)(=)(.*)");
  boost::smatch results;

  boost::char_separator<char> sep(" ");
  typedef boost::tokenizer< boost::char_separator<char> > tokenizer;
  tokenizer tokens(options,sep);

  for (tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter) {
    std::string opt(*tok_iter);

    if (opt == "help") {
      printUsage();
      std::exit(1);
    }

    if ( boost::regex_match(opt, results, options_parser)) {

        std::string name(results.str(1));
        std::string value(results.str(3));

        option_map::iterator iter = theOptions.find(name);
        if (iter == theOptions.end()) {
          std::cout << "No option named " << name << std::endl;
          std::exit(1);
        } else {
          iter->second->assign(value);
        }
    } else {
      std::cout << "Can't parse option: " << opt << std::endl;
      std::exit(1);
    }
  }
}

void reportConfiguration() {
  option_map::iterator iter = theOptions.begin();
  option_map::iterator end = theOptions.end();
  while (iter != end) {
    iter->second->report();
    ++iter;
  }
}

static const int kTwoMRC = 7;
static const int kTwoLORDS = 8;
static const int kStride = 9;
static const int kGHB = 10;
static const int kExhaustive = 11;
static const int kSpatioTemporal = 12;

int theExperiment = 0;


long theNumNodes = 16;
long thePageSize = 8192;
long theOrderCapacity = 250000;
long theCacheSize = 256;
long theSGPCacheSize = 1024;
long theBlockSize = 64;
long theSpatialGroupSize = 2048;
long theMaxStreams = 8;
long theInitFwdSize = 1;
long theInitBackSize = 0;
long theBodySize = 4;
long theMaxBlocks = 8;
long theIntersectDist = 0;
long theStreamWindow = 8;
long theNumRecentPtrs = 0;
long theDeltas = 0;
bool theSegregatedCaches = false;
bool theKillOnConsumed = false;
bool theKillOnInvalidate = false;
bool theAllowLiveStreams = false;
bool theUseChunks = false;
bool theWatchBlock = false;
bool theAggressive = false;
boost::optional<std::string> theLoadState;
boost::optional<std::string> theSaveState;
unsigned long long theStopCycle = 0;
bool theNoStreamOnSGPHit = false;


void registerOptions() {
  std::map<std::string, int> experiments;
  experiments["TwoMRC"] = kTwoMRC;
  experiments["TwoLORDS"] = kTwoLORDS;
#ifdef ENABLE_STRIDE
  experiments["Stride"] = kStride;
#endif //ENABLE_STRIDE
  experiments["GHB"] = kGHB;
  experiments["Exhaustive"] = kExhaustive;
  experiments["SpatioTemporal"] = kSpatioTemporal;

  new ChoiceOption( "experiment", "Experiment Type", experiments, theExperiment, "MRC");
  new LongOption( "nodes", "Number of Nodes", theNumNodes);
  new LongOption( "order-size", "Capacity of each Order", theOrderCapacity);
  new LongOption( "cache-size", "Size of each nodes cache", theCacheSize);
  new LongOption( "sgp-cache-size", "Size of each nodes SGP cache", theSGPCacheSize);
  new LongOption( "block-size", "Size of cache block", theBlockSize);
  new LongOption( "spatial-group-size", "Size of spatial group", theSpatialGroupSize);
  new LongOption( "max-streams", "Max simultaneous streams", theMaxStreams);
  new LongOption( "init-fwd-chunk", "Initial forward chunk size", theInitFwdSize);
  new LongOption( "init-back-chunk", "Initial backwards chunk size", theInitBackSize);
  new LongOption( "body-chunk", "Body chunk size", theBodySize);
  new LongOption( "max-blocks", "Max blocks per stream in cache", theMaxBlocks);
  new LongOption( "intersect-dist", "Distance to halt intersecting streams", theIntersectDist);
  new LongOption( "stream-window", "Length of address lists to consider", theStreamWindow);
  new LongOption( "num-recent", "Number of recent points (per order group)", theNumRecentPtrs);
  new LongOption( "deltas", "Size of GHB index table", theDeltas);
  new TrueFalseOption( "use-chunks", "Use chunks (not streams)", theUseChunks);
  new TrueFalseOption( "watch-block", "Wait for second block before streaming", theWatchBlock);
  new TrueFalseOption( "aggressive", "Forward unpaired pre-common blocks", theAggressive);
  new OptionalStringOption( "load-state", "Load order state from simulator checkpoint", theLoadState);
  new OptionalStringOption( "save-state", "Save order state from simulator checkpoint", theSaveState);
  new ULLOption( "stop-cycle", "Cycle on which to stop simulation", theStopCycle);
  new TrueFalseOption( "no-sgp-hit-streams", "Do not create TSE streams on SGP hits", theNoStreamOnSGPHit);
}

std::string version("15.0");

StatAnnotation * CfgVersion;
StatAnnotation * CfgExperiment;

DECLARE_STATIC_PARAM( ENABLE_INSTANCE )
DECLARE_STATIC_PARAM( ENABLE_COMBO )
DECLARE_STATIC_PARAM( ENABLE_ENTRY_STATE )
DECLARE_STATIC_PARAM( ENABLE_HAMMOCK )
DECLARE_STATIC_PARAM( ENABLE_PC_TRACKING )
DECLARE_STATIC_PARAM( ENABLE_OLD_STATS )
DECLARE_STATIC_PARAM( ENABLE_ALTERNATIVE_SEARCH )
DECLARE_STATIC_PARAM( ENABLE_COUNT_ALTERNATIVES )
DECLARE_STATIC_PARAM( ENABLE_STRIDE )
DECLARE_STATIC_PARAM( ENABLE_PC_LOOKUP )
DECLARE_STATIC_PARAM( ENABLE_BO_LOOKUP )
DECLARE_STATIC_PARAM( ENABLE_2MRC )
DECLARE_STATIC_PARAM( ENABLE_DELTAS )
DECLARE_STATIC_PARAM( ENABLE_SPATIAL_GROUPS )
DECLARE_STATIC_PARAM( ENABLE_SPATIAL_LOOKUP )
DECLARE_STATIC_PARAM( ENABLE_DEPRECATED )
DECLARE_STATIC_PARAM( ENABLE_STREAM_TRACKING )
DECLARE_STATIC_PARAM( ENABLE_TRACE )
DECLARE_STATIC_PARAM( ENABLE_TRACE_OUTPUT )
DECLARE_STATIC_PARAM( ENABLE_UNIQUE_COUNTS )
DECLARE_STATIC_PARAM( ENABLE_TRACE_SGPGENERATIONS )
DECLARE_STATIC_PARAM( ENABLE_WHITE_BOX )

TraceCoordinator * initialize_OrderHW(std::string options) {

  std::cout << "Order_hw Version " << version << std::endl;
  CfgVersion = new StatAnnotation("Configuration.Order_hw Version");
  *CfgVersion = version;

  ANNOTATE_STATIC_PARAM( ENABLE_INSTANCE )
  ANNOTATE_STATIC_PARAM( ENABLE_COMBO )
  ANNOTATE_STATIC_PARAM( ENABLE_ENTRY_STATE )
  ANNOTATE_STATIC_PARAM( ENABLE_HAMMOCK )
  ANNOTATE_STATIC_PARAM( ENABLE_PC_TRACKING )
  ANNOTATE_STATIC_PARAM( ENABLE_OLD_STATS )
  ANNOTATE_STATIC_PARAM( ENABLE_ALTERNATIVE_SEARCH )
  ANNOTATE_STATIC_PARAM( ENABLE_COUNT_ALTERNATIVES )
  ANNOTATE_STATIC_PARAM( ENABLE_STRIDE )
  ANNOTATE_STATIC_PARAM( ENABLE_PC_LOOKUP )
  ANNOTATE_STATIC_PARAM( ENABLE_BO_LOOKUP )
  ANNOTATE_STATIC_PARAM( ENABLE_2MRC )
  ANNOTATE_STATIC_PARAM( ENABLE_DELTAS )
  ANNOTATE_STATIC_PARAM( ENABLE_SPATIAL_GROUPS )
  ANNOTATE_STATIC_PARAM( ENABLE_SPATIAL_LOOKUP )
  ANNOTATE_STATIC_PARAM( ENABLE_DEPRECATED )
  ANNOTATE_STATIC_PARAM( ENABLE_STREAM_TRACKING )
  ANNOTATE_STATIC_PARAM( ENABLE_TRACE )
  ANNOTATE_STATIC_PARAM( ENABLE_TRACE_OUTPUT )
  ANNOTATE_STATIC_PARAM( ENABLE_UNIQUE_COUNTS )
  ANNOTATE_STATIC_PARAM( ENABLE_TRACE_SGPGENERATIONS )
  ANNOTATE_STATIC_PARAM( ENABLE_WHITE_BOX )

  registerOptions();
  parseOptions(options);
  reportConfiguration();


#ifdef ENABLE_ENTRY_STATE
  //Don't work when ENTRY_STATE is shut off
  theKillOnConsumed.assign("false");
  theKillOnInvalidate.assign("false");
#endif //#ENABLE_ENTRY_STATE


  ExperimentBase * experiment;

  switch(theExperiment) {
    case kTwoMRC:
      experiment =
        new TwoMRC
          ( theNumNodes
          , thePageSize
          , theOrderCapacity
          , theCacheSize
          , theBlockSize
          , theMaxStreams
          , theIntersectDist
          , theStreamWindow
          , theNumRecentPtrs
          , theAggressive
          , theInitFwdSize
          , theInitBackSize
          , theBodySize
          , theMaxBlocks
          , theLoadState
          , theSaveState
          , theStopCycle
          );
      break;
    case kTwoLORDS:
      experiment =
        new TwoLORDS
          ( theNumNodes
          , thePageSize
          , theOrderCapacity
          , theCacheSize
          , theBlockSize
          , theMaxStreams
          , theIntersectDist
          , theStreamWindow
          , theNumRecentPtrs
          , theAggressive
          , theInitFwdSize
          , theInitBackSize
          , theBodySize
          , theMaxBlocks
          , theLoadState
          );
      break;
#ifdef ENABLE_STRIDE
    case kStride:
      experiment =
        new Stride
          ( theNumNodes
          , thePageSize
          , theCacheSize
          , theBlockSize
          , theMaxStreams
          , theStreamWindow
          , theMaxBlocks
          );
      break;
#endif //ENABLE_STRIDE
    case kGHB:
      experiment =
        new GHB
          ( theNumNodes
          , thePageSize
          , theOrderCapacity
          , theCacheSize
          , theBlockSize
          , theMaxStreams
          , theIntersectDist
          , theStreamWindow
          , theNumRecentPtrs
          , theAggressive
          , theInitFwdSize
          , theInitBackSize
          , theBodySize
          , theMaxBlocks
          , theDeltas
          , theLoadState
          );
      break;
    case kExhaustive:
      experiment =
        new ExhaustiveLookup
          ( theNumNodes
          , thePageSize
          , theOrderCapacity
          , theCacheSize
          , theSGPCacheSize
          , theBlockSize
          , theBodySize
          );
      break;
    case kSpatioTemporal:
      experiment =
        new SpatioTemporal
          ( theNumNodes
          , thePageSize
          , theOrderCapacity
          , theCacheSize
          , theSGPCacheSize
          , theBlockSize
          , theSpatialGroupSize
          , theBodySize
          , theNoStreamOnSGPHit
          );
      break;
    default:
      std::cout << "Bad experiment number" << std::endl;
      std::exit(1);
  };

  std::cout << "Experiment: " << experiment->description();
  CfgExperiment = new StatAnnotation("Configuration.Experiment");
  *CfgExperiment = experiment->description();


  return experiment;
}

