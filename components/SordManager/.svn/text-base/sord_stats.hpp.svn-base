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
#ifndef SORD_STATS_INCLUDED
#define SORD_STATS_INCLUDED

#include <sstream>

#include <core/stats.hpp>

namespace nSordManager {

using namespace Flexus::Stat;

struct BaseStats {

  //Stats on stream length
  StatMax     theMaxLength;      //Per consumer
  StatAverage theAvgLength; //Per consumer

  StatCounter theCreatedSORDS;
  StatCounter theExhaustedSORDS;
  StatMax     theMaxLiveSORDS;
  StatCounter theOverwrittenTemplateBlocks;

  StatCounter theCreatedStreams;
  StatCounter theDemandInMaskStreams;
  StatCounter theDemandNotInMaskStreams;
  StatCounter theDemandNoMaskStreams;
  StatCounter theGlobalStreams;
  StatCounter thePerConsumerStreams;

  StatCounter theKilledStreams;
  StatCounter theKilledByEndOfStream;
  StatCounter theKilledByEndOfInput;
  StatCounter theKilledByReDemand;

  StatAverage theAvgFwdBlocks;
  StatCounter theTotFwdBlocks;

  StatMax     theMaxLiveStreams;

  StatCounter theSORDMisses;
  StatCounter theSORDMissesInMask;
  StatCounter theSORDMissesNotInMask;
  StatCounter theSORDMissesNoMask;
  StatCounter theSORDMissesGlobal;
  StatCounter theSORDMissesPerConsumer;


  StatCounter theSORDHits;
  StatCounter theSORDHitsMRP;
  StatCounter theSORDHitsMRPHead;
  StatCounter theSORDHitsDemandInMask;
  StatCounter theSORDHitsDemandNotInMask;
  StatCounter theSORDHitsDemandNoMask;
  StatCounter theSORDHitsGlobal;
  StatCounter theSORDHitsPerConsumer;

  StatCounter theProductions;
  StatCounter theInvalidations;
  StatCounter theConsumptions;
  StatCounter theMispredictAdvances;

  StatCounter theDirProductions;
  StatCounter theDirConsumptions;
  StatCounter theDirUpgrades;
  StatCounter theDirFlushes;
  Flexus::Stat::StatInstanceCounter<Flexus::SharedTypes::PhysicalMemoryAddress> theProductionAddresses;

  BaseStats(std::string const & aName)
    : theMaxLength                 ( aName + ".Max Stream Length"                   )
    , theAvgLength                 ( aName + ".Avg Stream Length"                   )
    , theCreatedSORDS              ( aName + ".Created SORDS"                       )
    , theExhaustedSORDS            ( aName + ".Exhausted SORDS"                     )
    , theMaxLiveSORDS              ( aName + ".Max Live SORDS"                      )
    , theOverwrittenTemplateBlocks ( aName + ".Overwritten Template Blocks"         )
    , theCreatedStreams            ( aName + ".Created Streams"                     )
    , theDemandInMaskStreams       ( aName + ".Created Streams(demand in-mask)"     )
    , theDemandNotInMaskStreams    ( aName + ".Created Streams(demand not-in-mask)" )
    , theDemandNoMaskStreams       ( aName + ".Created Streams(demand no-mask)"     )
    , theGlobalStreams             ( aName + ".Created Streams(global)"             )
    , thePerConsumerStreams        ( aName + ".Created Streams(per consumer)"       )
    , theKilledStreams             ( aName + ".Killed Streams"                      )
    , theKilledByEndOfStream       ( aName + ".Killed Streams(end of stream)"       )
    , theKilledByEndOfInput        ( aName + ".Killed Streams(end of input)"        )
    , theKilledByReDemand          ( aName + ".Killed Streams(re-demand)"           )
    , theAvgFwdBlocks              ( aName + ".Avg Forwarded Blocks"                )
    , theTotFwdBlocks              ( aName + ".Tot Forwarded Blocks"                )
    , theMaxLiveStreams            ( aName + ".Max Live Streams"                    )
    , theSORDMisses                ( aName + ".SORD Misses"                         )
    , theSORDMissesInMask          ( aName + ".SORD Misses(in-mask)"                )
    , theSORDMissesNotInMask       ( aName + ".SORD Misses(not in-mask)"            )
    , theSORDMissesNoMask          ( aName + ".SORD Misses(no mask)"                )
    , theSORDMissesGlobal          ( aName + ".SORD Misses(global)"                 )
    , theSORDMissesPerConsumer     ( aName + ".SORD Misses(per consumer)"           )
    , theSORDHits                  ( aName + ".SORD Hits"                           )
    , theSORDHitsMRP               ( aName + ".SORD Hits(mrp)"                      )
    , theSORDHitsMRPHead           ( aName + ".SORD Hits(mrp head)"                 )
    , theSORDHitsDemandInMask      ( aName + ".SORD Hits(demand in-mask)"           )
    , theSORDHitsDemandNotInMask   ( aName + ".SORD Hits(demand not-in-mask)"       )
    , theSORDHitsDemandNoMask      ( aName + ".SORD Hits(demand no-maks)"           )
    , theSORDHitsGlobal            ( aName + ".SORD Hits(global)"                   )
    , theSORDHitsPerConsumer       ( aName + ".SORD Hits(per consumer)"             )
    , theProductions               ( aName + ".Productions"                         )
    , theInvalidations             ( aName + ".Invalidations"                       )
    , theConsumptions              ( aName + ".Consumptions"                        )
    , theMispredictAdvances        ( aName + ".Mispredict Advances"                 )
    , theDirProductions            ( aName + ".DirProductions"                      )
    , theDirConsumptions           ( aName + ".DirConsumptions"                     )
    , theDirUpgrades               ( aName + ".DirUpgrades"                         )
    , theDirFlushes                ( aName + ".DirFlushes"                          )
    , theProductionAddresses       ( aName + ".ProductionAddresses"                 )
    {}
};

struct NodeStats : public BaseStats {

  std::string theName;
  StatCounter theHomeBlocks;

  std::string makeName(int aNodeId) {
    std::stringstream name;
    name << "node-" << std::setw(2) << std::setfill('0') << aNodeId;
    return name.str();
  }

  NodeStats(int aNodeId)
    : BaseStats( makeName(aNodeId) )
    , theName( makeName(aNodeId) )
    , theHomeBlocks(theName + ".Home Blocks")
    {}

};

struct OverallStats : public BaseStats {

  std::string theName;

  StatLog2Histogram theLengthHistogram;

  StatCounter theWaste;
  StatCounter theGlobalWaste;
  StatCounter thePerConsumerWaste;

  StatCounter theConsumptionsMissingAllSORDS;
  StatCounter theConsumptionsBeatDowngrade;

  StatCounter theTotalBlocks;


  OverallStats()
    : BaseStats( "overall" )
    , theName( "overall" )
    , theLengthHistogram(theName + ".Length Histogram")
    , theWaste(theName + ".Forwarding Waste")
    , theGlobalWaste(theName + ".Forwarding Waste (Global)")
    , thePerConsumerWaste(theName + ".Forwarding Waster (Per Consumer)")
    , theConsumptionsMissingAllSORDS(theName + ".Consumptions Missing All SORDS")
    , theConsumptionsBeatDowngrade(theName + ".Consumptions Beat Downgrade")
    , theTotalBlocks(theName + ".Total Blocks")
    {}

};

extern OverallStats theOverallStats;
extern NodeStats theNodeStats[16];

enum StreamDeathReason {
  eKilledByEndOfStream,
  eKilledByEndOfInput,
  eKilledByReDemand,
};

} //end namespace nSordManager

#endif //SORD_STATS_INCLUDED
