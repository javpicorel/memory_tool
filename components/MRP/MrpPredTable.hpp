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
namespace nMrpTable {

namespace Stat = Flexus::Stat;

using boost::intrusive_ptr;

template < class SignatureMapping >
class MrpPredictionTableType {
public:
  typedef typename SignatureMapping::MemoryAddress MemoryAddress;
  typedef typename SignatureMapping::Signature Signature;

  struct PredEntry : boost::counted_base {
    // maintain node list of both prediction and actual readers
    PredEntry(nMrpStats::MrpStats & theStats, Signature sig, HistoryEvent pred, bool isPriv)
      : signature(sig)
      , predicted(pred)
      , actual()
/*      , PredExact               ( theStats.PredExact.predict() )
      , PredMoreActual          ( theStats.PredMoreActual.predict() )
      , PredMorePredicted       ( theStats.PredMorePredicted.predict() )
      , PredInexactMoreActual   ( theStats.PredInexactMoreActual.predict() )
      , PredInexactMorePredicted( theStats.PredInexactMorePredicted.predict() )
      , PredDifferent           ( theStats.PredDifferent.predict() )
      , PredOpposite            ( theStats.PredOpposite.predict() )
*/
      , RealReaders             ( isPriv ? theStats.RealReaders_OS.predict() : theStats.RealReaders_User.predict() )
      , RealHits                ( isPriv ? theStats.RealHits_OS.predict() : theStats.RealHits_User.predict() )
      , RealTraining            ( isPriv ? theStats.RealTraining_OS.predict() : theStats.RealTraining_User.predict() )
      , RealMisses              ( isPriv ? theStats.RealMisses_OS.predict() : theStats.RealMisses_User.predict() )
      , DanglingMisses          ( isPriv ? theStats.DanglingMisses_OS.predict(): theStats.DanglingMisses_User.predict() )

    {}
    // returns true for the first actual read by this node
    bool read(unsigned int node) {
      bool ret = !actual.isReader(node);
      actual.addReader(node);
      return ret;
    }

    void resolve(bool isFinalize, nMrpStats::MrpStats & theStats, MemoryAddress anAddress) {

/*
      if(evalExact()) {
        PredExact->confirm();
      } else if(evalMoreActual()) {
        PredMoreActual->confirm();
      } else if(evalMorePredicted()) {
        PredMorePredicted->confirm();
      } else if(evalInexactMoreActual()) {
        PredInexactMoreActual->confirm();
      } else if(evalInexactMorePredicted()) {
        PredInexactMorePredicted->confirm();
      } else if(evalDifferent()) {
        PredDifferent->confirm();
      } else {
        DBG_Assert(evalOpposite());
        PredOpposite->confirm();
      }
*/

      HistoryEvent hits( actual.value & predicted.value );
      HistoryEvent training( actual.value & (~hits.value) );
      HistoryEvent misses( predicted.value & (~hits.value) );

      // do the per-reader statistics
      RealReaders->confirm( actual.count() );
      RealHits->confirm( hits.count() );
#ifdef MRP_INSTANCE_COUNTERS
      if (hits.count() > 0) {
        theStats.HitInstances << std::make_pair(anAddress, hits.count());
      }
      if (training.count() > 0) {
        theStats.TrainingInstances << std::make_pair(anAddress, training.count());
      }
      if (! isFinalize && misses.count() > 0) {
        theStats.MissInstances << std::make_pair(anAddress, misses.count());
      }
#endif //MRP_INSTANCE_COUNTERS
      RealTraining->confirm( training.count() );
      if (isFinalize) {
        DanglingMisses->confirm( misses.count() );
      } else {
        RealMisses->confirm( misses.count() );
      }

    }

    // evaluation of a prediction
    bool evalExact() {
      // the actual readers exactly matched the predicted readers
      return (predicted.value == actual.value);
    }
    bool evalMoreActual() {
      // more nodes actually consumed the block than were predicted
      if(predicted.value & (~actual.value)) {
        // there cannot be predicted readers that did not consume
        return false;
      }
      // no need to check if we really have extra actual readers,
      // since this would be "exact" and been checked previously
      return true;
    }
    bool evalMorePredicted() {
      // more nodes were predicted to consume the block than actually did
      if(actual.value & (~predicted.value)) {
        // there cannot be actual consumers that were not predicted
        return false;
      }
      // no need to check if we really have extra predicted readers,
      // since this would be "exact" and been checked previously
      return true;
    }
    bool evalInexactMoreActual() {
      // overall, there were more actual consumers than predicted,
      // but also some predicted that did not consume
      return (actual.count() > predicted.count());
    }
    bool evalInexactMorePredicted() {
      // overall, there were more predicted consumers than actual,
      // but also some actual that were not predicted
      return (predicted.count() > actual.count());
    }
    bool evalDifferent() {
      // at least one predicted node that actually consumed, as well as
      // an equal number of predicted but not actual, and vice versa
      return ( (predicted.count() == actual.count()) &&
               ((predicted.value & actual.value) != 0) );
    }
    bool evalOpposite() {
      // there were no predicted nodes that actually consumed, and vice versa
      return ((predicted.value & actual.value) == 0);
    }

    Signature signature;
    HistoryEvent predicted;
    HistoryEvent actual;
/*
    boost::intrusive_ptr<Stat::Prediction> PredExact;
    boost::intrusive_ptr<Stat::Prediction> PredMoreActual;
    boost::intrusive_ptr<Stat::Prediction> PredMorePredicted;
    boost::intrusive_ptr<Stat::Prediction> PredInexactMoreActual;
    boost::intrusive_ptr<Stat::Prediction> PredInexactMorePredicted;
    boost::intrusive_ptr<Stat::Prediction> PredDifferent;
    boost::intrusive_ptr<Stat::Prediction> PredOpposite;
*/
    boost::intrusive_ptr<Stat::Prediction> RealReaders;
    boost::intrusive_ptr<Stat::Prediction> RealHits;
    boost::intrusive_ptr<Stat::Prediction> RealTraining;
    boost::intrusive_ptr<Stat::Prediction> RealMisses;
    boost::intrusive_ptr<Stat::Prediction> DanglingMisses;

  };

private:
  typedef intrusive_ptr<PredEntry> PredEntry_p;
  typedef std::map<MemoryAddress,PredEntry_p> PredMap;
public:
  typedef typename PredMap::iterator Iterator;
private:
  typedef std::pair<MemoryAddress,PredEntry_p> PredPair;
  typedef std::pair<Iterator,bool> InsertPair;


public:
  MrpPredictionTableType(nMrpStats::MrpStats & aStats)
   : theStats(aStats)
   {}

  PredEntry_p find(MemoryAddress anAddr) {
    Iterator iter = thePredictions.find(anAddr);
    if(iter != thePredictions.end()) {
      return iter->second;
    }
    return PredEntry_p(0);
  }

  PredEntry_p findAndRemove(MemoryAddress anAddr) {
    Iterator iter = thePredictions.find(anAddr);
    if(iter != thePredictions.end()) {
      PredEntry_p temp = iter->second;
      thePredictions.erase(iter);
      return temp;
    }
    return PredEntry_p(0);
  }

  void addEntry(MemoryAddress anAddr, bool isPriv, Signature aSig, HistoryEvent aPrediction) {
    // try to insert a new entry - if successful, we're done;
    // otherwise abort
    PredEntry_p anEntry = new PredEntry(theStats, aSig, aPrediction, isPriv);
    PredPair insert = std::make_pair(anAddr, anEntry);
    InsertPair result = thePredictions.insert(insert);
    DBG_Assert(result.second);
  }

  void clear() {
    thePredictions.clear();
  }

  Iterator begin() { return thePredictions.begin(); }
  Iterator end() { return thePredictions.end(); }

private:
  // use a Map to allow fast lookup (also, we don't care about structure)
  PredMap thePredictions;
  nMrpStats::MrpStats & theStats;

};  // end class MrpPredictionTableType


}  // end namespace nMrpTable
