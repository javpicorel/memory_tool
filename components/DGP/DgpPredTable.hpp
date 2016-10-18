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
namespace nDgpTable {

using namespace Flexus::Stat;

using boost::intrusive_ptr;


class DgpPredictionTable {
public:
  typedef SignatureMapping::Signature Signature;
  typedef SignatureMapping::MemoryAddress MemoryAddress;

  struct PredEntry : boost::counted_base {
    PredEntry(Signature sig, bool pred, bool init, bool privilege, int table, long long confs, DgpStats & theStats)
      : signature(sig)
      , predicted(pred)
      , initial(init)
      , priv(privilege)
      , sigTable(table)
      , confVals(confs)
      , PredCorrect(theStats.CorrectPredictions.predict())
      , PredMispredict(theStats.Mispredictions.predict())
      , PredDangling(theStats.DanglingPredictions.predict())
    {
      if(predicted) {
        PredDangling->guess();
      }
    }

    Signature signature;
    bool predicted;
    bool initial;       // the initial prediction on this signature?
    bool priv;          // OS access?
    int  sigTable;      // which signature table generated this prediction?
    long long confVals; // confidence values from both tables
    boost::intrusive_ptr<Prediction> PredCorrect;
    boost::intrusive_ptr<Prediction> PredMispredict;
    boost::intrusive_ptr<Prediction> PredDangling;

    enum ePredResult {
        kBad,
        kGood,
        kDangling
    };

    void resolve(ePredResult aResult) {
      switch (aResult) {
        case kBad:
          PredDangling->reject();
          PredMispredict->confirm();
          break;
        case kGood:
          PredDangling->reject();
          PredCorrect->confirm();
          break;
        case kDangling:
          PredDangling->goodGuess();
          break;
      }
      PredCorrect->dismiss();
      PredMispredict->dismiss();
      PredDangling->dismiss();
    }


  };

private:
  typedef intrusive_ptr<PredEntry> PredEntry_p;
  typedef std::map<MemoryAddress,PredEntry_p> PredMap;
  typedef PredMap::iterator Iterator;
  typedef std::pair<MemoryAddress,PredEntry_p> PredPair;
  typedef std::pair<Iterator,bool> InsertPair;

public:
  DgpPredictionTable(SignatureMapping & aMapper)
    : theSigMapper(aMapper)
  {}

  PredEntry_p findAndRemove(MemoryAddress anAddr) {
    Iterator iter = thePredictions.find(anAddr);
    if(iter != thePredictions.end()) {
      PredEntry_p temp = iter->second;
      thePredictions.erase(iter);
      return temp;
    }
    return PredEntry_p(0);
  }

  void addEntry(MemoryAddress anAddr, Signature aSig, bool aPrediction, bool initial, bool priv, int aSigTable, long long aConfVals, DgpStats & theStats) {
    // try to insert a new entry - if successful, we're done;
    // otherwise abort
    PredEntry_p anEntry = new PredEntry(aSig, aPrediction, initial, priv, aSigTable, aConfVals, theStats);
    PredPair insert = std::make_pair(anAddr, anEntry);
    InsertPair result = thePredictions.insert(insert);
    DBG_Assert(result.second);
  }

  void finalizeStats() {
    DBG_(Iface, ( << "DGP Finalizing Stats") );
    //Process all predictions left in the prediction table
    Iterator iter = thePredictions.begin();
    Iterator end = thePredictions.end();
    while (iter != end) {
      iter->second->resolve(PredEntry::kDangling);
      ++iter;
    }
    thePredictions.clear();

    DBG_(Iface, ( << "DGP Stats Finalized") );
  }

private:
  // use a Map to allow fast lookup (also, we don't care about structure)
  PredMap thePredictions;
  SignatureMapping & theSigMapper;

};  // end class DgpPredictionTable


}  // end namespace nDgpTable
