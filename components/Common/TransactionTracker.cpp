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

#include <algorithm>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include <boost/shared_ptr.hpp>
#include <core/stats.hpp>

#include <components/Common/Slices/TransactionTracker.hpp>


#define DBG_DefineCategories TransactionTrace, TransactionDetailsTrace
#include DBG_Control()

static const int kMinLatency = 20;

namespace Flexus {
namespace SharedTypes {

namespace Stat = Flexus::Stat;
namespace ll = boost::lambda;

unsigned long long theGloballyUniqueId = 0;
unsigned long long getTTGUID() {
  return theGloballyUniqueId++;
}
boost::shared_ptr<TransactionTracer> TransactionTracker::theTracer;
boost::shared_ptr<TransactionStatManager> TransactionTracker::theTSM;


class TransactionTracerImpl : public TransactionTracer {
  bool includeTransaction(TransactionTracker const & aTransaction) {
  //Fields required for trace
    bool meets_requirements
         = aTransaction.initiator()
        && (*aTransaction.initiator() == 0)
        && aTransaction.address()
        && aTransaction.completionCycle()
         ;
    if (meets_requirements) {
      if (aTransaction.fillLevel() && *aTransaction.fillLevel() == ePrefetchBuffer) {
        //Always include prefetch buffer fills
        meets_requirements = true;
      } else {
        //Include fills with a latency of > 20 cycles
        long latency = *aTransaction.completionCycle() - aTransaction.startCycle();
        meets_requirements = (latency > kMinLatency);
      }
    }
    return meets_requirements ;
  }

  static void processCycles( boost::tuple<std::string, std::string, int> const & aCause) {
    DBG_(VVerb,
      Cat(TransactionDetailsTrace)
      Set( (Component) << aCause.get<0>() )
      Set( (Cause) << aCause.get<1>() )
      SetNumeric( (Cycles) aCause.get<2>() )
    );
  }

  void processTransaction(TransactionTracker const & aTransaction) {
    int initiator = *aTransaction.initiator(); (void) initiator; //suppress warning
    long long latency = *aTransaction.completionCycle() - aTransaction.startCycle(); (void) latency; //suppress warning

    int responder = -1;
    if (aTransaction.responder()) {
      responder = *aTransaction.responder();
    }

    std::string OS = "Non-OS";
    if (aTransaction.OS() && *aTransaction.OS()) {
      OS = "OS";
    }

    std::string critical = "Non-Critical";
    if (aTransaction.criticalPath() && *aTransaction.criticalPath()) {
      critical = "Critical";
    }

    std::string source = "Unknown";
    if (aTransaction.source()) {
      source = *aTransaction.source();
    }


    std::string fill_level= "Unknown";
    if (aTransaction.fillLevel()) {
      switch(*aTransaction.fillLevel()) {
        case eL1:
          fill_level = "L1";
          break;
        case eL2:
          fill_level = "L2";
          break;
        case eL3:
          fill_level = "L3";
          break;
        case eLocalMem:
        case eRemoteMem:
          fill_level = "Memory";
          break;
        case ePrefetchBuffer:
          fill_level = "PrefetchBuffer";
          break;
        default:
          break;
      }
    }

    std::string complete= "";
    if (! aTransaction.completionCycle()) {
      complete = "Incomplete";
    }


    DBG_(VVerb,
      Cat(TransactionTrace)
      SetNumeric( (InitiatingNode) initiator)
      Set ( (Source) << source)
      SetNumeric( (RespondingNode) responder)
      Set( (FillLevel) << fill_level )
      Set( (Address) << "0x" << std::hex << std::setw(8) << std::setfill('0') << *aTransaction.address() )
      SetNumeric( (Latency) latency)
      Set( (OS) << OS )
      Set( (Critical) << critical )
      Set( (Complete) << complete )
    );

    // std::for_each( aTransaction.cycleAccounting().begin(), aTransaction.cycleAccounting().end(), &processCycles );

  }

  void trace(TransactionTracker const & aTransaction) {
    if ( includeTransaction(aTransaction) ) {
      processTransaction(aTransaction);
    }
  }
};

boost::shared_ptr<TransactionTracer> TransactionTracer::createTracer() {
  return boost::shared_ptr<TransactionTracer>(new TransactionTracerImpl());
}



struct XactCatStats {
  std::string theCategoryName;

  Stat::StatCounter theCount;
  Stat::StatAverage theAvgLatency;
  std::map<std::string, boost::intrusive_ptr<Stat::StatCounter> > theTotalDelayCounters;

  XactCatStats (std::string aCategoryName)
    : theCategoryName(aCategoryName)
    , theCount(std::string("Xacts<") + aCategoryName + ">-Count")
    , theAvgLatency(std::string("Xacts<") + aCategoryName + ">-AvgLatency")
    {}

  void accountDelay(std::string aReason, int aDelay) {
    if ( ! theTotalDelayCounters[aReason] ) {
      theTotalDelayCounters[aReason] =
        boost::intrusive_ptr<Stat::StatCounter>(
          new Stat::StatCounter( std::string("Xacts<") + theCategoryName + ">-" + aReason + "Delay")
        );
    }
    *theTotalDelayCounters[aReason] += aDelay;
  }
};

class TransactionStatManagerImpl : public TransactionStatManager {

  XactCatStats theCriticalXacts;
  XactCatStats theNonCriticalXacts;

  XactCatStats theOSXacts;
  XactCatStats theNonOSXacts;

  XactCatStats theL1FillXacts;
  XactCatStats theL2FillXacts;
  XactCatStats theLocalFillXacts;
  XactCatStats theRemoteFillXacts;
  XactCatStats thePrefetchFillXacts;

  Stat::StatCounter theTransactionsMissingRequiredFields;
public:
  TransactionStatManagerImpl()
    : theCriticalXacts("Critical")
    , theNonCriticalXacts("NonCritical")
    , theOSXacts("OS")
    , theNonOSXacts("NonOS")
    , theL1FillXacts("L1")
    , theL2FillXacts("L2")
    , theLocalFillXacts("Local")
    , theRemoteFillXacts("Remote")
    , thePrefetchFillXacts("Prefetch")
    , theTransactionsMissingRequiredFields("Xacts-MissingRequiredFields")
    {}

  virtual ~TransactionStatManagerImpl() {}

private:
  void categorize(TransactionTracker const & aTransaction, std::vector<XactCatStats *> & aCategoryList) {
    //Required fields for any category
    if ( ! ( aTransaction.initiator() && aTransaction.address() && aTransaction.completionCycle() ) ) {
      ++theTransactionsMissingRequiredFields;
      return; //No categories allowed for
    }

    if (aTransaction.criticalPath() && *aTransaction.criticalPath()) {
      aCategoryList.push_back( & theCriticalXacts );
    } else {
      aCategoryList.push_back( & theNonCriticalXacts );
    }

    if (aTransaction.OS() && *aTransaction.OS()) {
      aCategoryList.push_back( & theOSXacts );
    } else {
      aCategoryList.push_back( & theNonOSXacts );
    }

    if (aTransaction.fillLevel()) {
      switch (*aTransaction.fillLevel()) {
        case eL1:
          aCategoryList.push_back( & theL1FillXacts );
          break;
        case eL2:
          aCategoryList.push_back( & theL2FillXacts );
          break;
        case eLocalMem:
          aCategoryList.push_back( & theLocalFillXacts );
          break;
        case eRemoteMem:
          aCategoryList.push_back( & theRemoteFillXacts );
          break;
        case ePrefetchBuffer:
          aCategoryList.push_back( & thePrefetchFillXacts );
          break;
        default:
          break;
      }
    }

  }

  template <class StatT, class UpdateT >
  void accum( StatT XactCatStats::* aCounter, UpdateT anUpdate, std::vector<XactCatStats  *> & aCategoryList) {
    using ll::_1;
    using ll::bind;
    std::for_each( aCategoryList.begin(), aCategoryList.end(), bind( aCounter, _1) += anUpdate);
  }

  template <class StatT, class UpdateT >
  void append( StatT XactCatStats::* aCounter, UpdateT anUpdate, std::vector<XactCatStats *> & aCategoryList) {
    using ll::_1;
    using ll::bind;
    std::for_each( aCategoryList.begin(), aCategoryList.end(), bind( aCounter, _1) << anUpdate);
  }

  static void accountCycles( std::vector<XactCatStats *> const & aCategoryList, boost::tuple<std::string, std::string, int> const & aDetail ) {
    using ll::_1;
    using ll::bind;

    std::for_each
      ( aCategoryList.begin()
      , aCategoryList.end()
      , ( bind( & XactCatStats::accountDelay , _1, aDetail.get<0>(), aDetail.get<2>() )
        , bind( & XactCatStats::accountDelay , _1, aDetail.get<1>(), aDetail.get<2>() )
        )
      );
  }


public:
  virtual void count(TransactionTracker const & aTransaction) {
    /*
    using ll::bind;
    using ll::_1;

    std::vector<XactCatStats *> categories;
    categorize(aTransaction, categories);

    if (categories.size() > 0) {

      accum( & XactCatStats::theCount, 1, categories);

      long long latency = *aTransaction.completionCycle() - aTransaction.startCycle();
      append( & XactCatStats::theAvgLatency, latency, categories);

      std::for_each
        ( aTransaction.cycleAccounting().begin()
        , aTransaction.cycleAccounting().end()
        , bind(& TransactionStatManagerImpl::accountCycles, categories, _1)
        );
    }
    */
  }


};

boost::shared_ptr<TransactionStatManager> TransactionStatManager::createTSM() {
  return boost::shared_ptr<TransactionStatManager>(new TransactionStatManagerImpl());
}


} //SharedTypes
} //Flexus
