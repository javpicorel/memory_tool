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

#define DBG_DeclareCategories MRPTrace
#include DBG_Control()


#include <boost/lambda/bind.hpp>

#define HIST_DEPTH 4

#include "MrpCommon.hpp"
namespace nMrpTable {
  typedef ReadersConf Readers;
}

#include "MrpHistTable.hpp"

#ifdef MRP_INFINITE
  #include "MrpInfSigTable.hpp"
#else
  #include "MrpSigTable.hpp"
#endif

#include "MrpStats.hpp"

#include "MrpPredTable.hpp"


namespace nMrpTable {

  namespace ll = boost::lambda;

  // General types
  typedef SignatureMappingN<HIST_DEPTH>::SignatureMapping SignatureMapping;
  typedef SignatureMapping::MemoryAddress MemoryAddress;
  typedef SignatureMapping::Signature Signature;

  // MRP History Table types
  typedef nMrpTable::MrpHistoryTableType<HIST_DEPTH,SignatureMapping> MrpHistoryTable;

  // MRP Signature Table types
#ifdef MRP_INFINITE
  typedef nMrpTable::MrpSigTableType<SignatureMapping> MrpSigTable;
#else
  typedef nMrpTable::MrpSigTableType<SignatureMappingN<HIST_DEPTH> > MrpSigTable;
  typedef MrpSigTable::MrpBlock SigEntry;
  typedef MrpSigTable::MrpLookupResult SigLookup;
#endif

  // MRP Prediction Table types
  typedef nMrpTable::MrpPredictionTableType<SignatureMapping> MrpPredTable;
  typedef MrpPredTable::PredEntry PredEntry;




class MrpMain {

  unsigned int theMRPNode;
  unsigned int theLastPrediction;
  MemoryAddress theLastPredictionAddress;

public:

  void finalizeStats() {
    DBG_(Iface, ( << "MRP Finalizing Stats") );
    //Process all predictions left in the prediction table
    MrpPredTable::Iterator iter = myPredictions.begin();
    MrpPredTable::Iterator end = myPredictions.end();
    while (iter != end) {
      iter->second->resolve(true, stats, iter->first);

      ++iter;
    }
    myPredictions.clear();

    DBG_(Iface, ( << "MRP Stats Finalized") );
  }

  MrpMain(std::string const & aName, int aNode, int blockAddrBits, int l2BlockSize, int numSets, int assoc)
    : theMRPNode(aNode)
    , theLastPrediction(0)
    , theLastPredictionAddress(0)
    , mySigMapper(HIST_DEPTH, blockAddrBits, l2BlockSize)
    , myHistoryTable(mySigMapper)
    , mySignatureTable(numSets, assoc)
    , stats(aName)
    , myPredictions(stats)
  {
    Flexus::Stat::getStatManager()->addFinalizer( ll::bind( &MrpMain::finalizeStats, this) );
  }

  unsigned int lastPrediction() {
    return theLastPrediction;
  }

  unsigned int lastPrediction(MemoryAddress anAddress) {
    if(theLastPredictionAddress == anAddress) {
      return theLastPrediction;
    }
    return getPrediction(anAddress);
  }

  unsigned int getPrediction(MemoryAddress anAddress) {
    intrusive_ptr<PredEntry> pred = myPredictions.find(anAddress);
    if (pred) {
      return pred->predicted.readers();
    } else {
      //No prediction
      return 0;
    }
  }

  ~MrpMain() {
  }


  void read(MemoryAddress anAddr, int aNode, bool isPriv) {
    // first look up in the predictions
    intrusive_ptr<PredEntry> pred = myPredictions.find(anAddr);
    if(pred) {
      // record this read in the list of actual readers
      if(pred->read(aNode)) {
        DBG_Assert(pred->actual.isReader(aNode));
        // initial read by this node
        if (pred->predicted.isReader(aNode)) {
          DBG_(VVerb, ( << "MRP @" << & std::hex << anAddr  << " /" << pred->predicted.readers() << & std::dec << "/ HIT - C[" << aNode << "] confirmed. ") );
          DBG_(Trace, Cat(MRPTrace)
            SetNumeric( (Node) theMRPNode )
            Set( (Address) << "0x" << std::hex << std::setw(8) << std::setfill('0') << anAddr )
            Set( (Signature) << std::hex << std::setw(8) << std::setfill('0') << pred->signature )
            Set( (PredictedReaders) << std::hex << std::setw(4) << std::setfill('0') << pred->predicted.readers() )
            SetNumeric( (Reader) aNode )
            Set( (Type) << "Hit" )
          );
        } else {
          DBG_(VVerb, ( << "MRP @" << & std::hex << anAddr << " /" << pred->predicted.readers() << & std::dec << "/ MISS - C[" << aNode << "] was not predicted: ") );
          DBG_(Trace, Cat(MRPTrace)
            SetNumeric( (Node) theMRPNode )
            Set( (Address) << "0x" << std::hex << std::setw(8) << std::setfill('0') << anAddr )
            Set( (Signature) << std::hex << std::setw(8) << std::setfill('0') << pred->signature )
            Set( (PredictedReaders) << std::hex << std::setw(4) << std::setfill('0') << pred->predicted.readers() )
            SetNumeric( (Reader) aNode )
            Set( (Type) << "Miss" )
          );
        }
        stats.ReadInitial++;
      } else {
        DBG_(VVerb, ( << "repeat read by node " << aNode << " for addr " << std::hex << anAddr ) );
        stats.ReadAgain++;
      }
    }
    else {
      DBG_(Iface, ( << "MRP @" << & std::hex << anAddr  << & std::dec << " TRAIN - C[" << aNode << "]. ") );
      DBG_(Trace, Cat(MRPTrace)
        SetNumeric( (Node) theMRPNode )
        Set( (Address) << "0x" << std::hex << std::setw(8) << std::setfill('0') << anAddr )
        Set( (Signature) << "xxxxxxxx" )
        Set( (PredictedReaders) << "0000" )
        SetNumeric( (Reader) aNode )
        Set( (Type) << "Train" )
      );

#ifdef MRP_INSTANCE_COUNTERS
      stats.TrainingInstances << std::make_pair(anAddr, 1);
#endif //MRP_INSTANCE_COUNTERS
      if (isPriv) {
        stats.TrainingRead_OS++;
      } else {
        stats.TrainingRead_User++;
      }
    }

    // update the history table
    myHistoryTable.readAccess(anAddr, aNode);

    // don't look in the signature table since we should not predict based
    // on a read - only the last write to an address should be able to
    // trigger a MRP prediction
  }

  void write(MemoryAddress anAddr, int aNode, bool isPriv) {
    // if this is a repeat write by the same node, completely ignore it
    if(myHistoryTable.currWriter(anAddr) == aNode) {
      return;
    }

    theLastPrediction = 0;
    theLastPredictionAddress = MemoryAddress(0);

    // first look up in the predictions
    intrusive_ptr<PredEntry> pred = myPredictions.findAndRemove(anAddr);
    if(pred) {
      // this write completes a series of reads, so we can now evaluate
      // the prediction
      pred->resolve(false, stats, anAddr);

      HistoryEvent hits( pred->actual.value & pred->predicted.value );
      HistoryEvent training( pred->actual.value & (~hits.value) );
      HistoryEvent misses( pred->predicted.value & (~hits.value) );

      // look up the corresponding entry in the signature table
#ifdef MRP_INFINITE
      Readers * theseReaders = mySignatureTable.lookup(anAddr, pred->signature);
      if(theseReaders) {
#else
      SigLookup lookup = mySignatureTable[pred->signature];
      if(lookup.hit()) {
        lookup.access(nMrpTable::AnyAccess);
        Readers * theseReaders = &( lookup.block().readers() );
#endif
        // in the future, use a larger set of readers - those already listed
        // in the table, as well as the actual readers this time
        if(theseReaders->predict() != pred->predicted.readers()) {
          DBG_(Iface, ( << "pred_readers=" << std::hex << pred->predicted.readers()
                       << " sig_table_readers=" << theseReaders->predict() ) );
        }
        //DBG_Assert(theseReaders->predict() == pred->predicted.readers());

        DBG_(Iface, ( << "MRP P[" << aNode <<"] Write @" << & std::hex << anAddr  << " #" << pred->signature << & std::dec << "# (" << *theseReaders << ") Pred: /" << & std::hex << pred->predicted.readers() << "/ Actual: \\" << pred->actual.readers() << & std::dec << "\\ hits: " << hits.count() << " misses: " << misses.count() << " training: " << training.count() ) );

        theseReaders->updateReaders( pred->actual.readers() );
        if(pred->predicted.readers() != pred->actual.readers()) {
          DBG_(VVerb, ( << "updated readers: old=" << std::hex << pred->predicted.readers()
                       << " actual=" << pred->actual.readers() << " updated=" << theseReaders->predict() ) );
        }
        DBG_(Iface, ( << "MRP @" << & std::hex << anAddr  << " #" << pred->signature << & std::dec << "# Updated: (" << *theseReaders << ").") );

        DBG_(Trace, Cat(MRPTrace)
          SetNumeric( (Node) theMRPNode )
          Set( (Address) << "0x" << std::hex << std::setw(8) << std::setfill('0') << anAddr )
          Set( (Signature) << std::hex << std::setw(8) << std::setfill('0') << pred->signature )
          Set( (PredictedReaders) << std::hex << std::setw(4) << std::setfill('0') << pred->predicted.readers() )
          Set( (Readers) << std::hex << std::setw(4) << std::setfill('0') << pred->actual.readers() )
          SetNumeric( (Writer) aNode )
          Set( (Type) << "UpdateSignature" )
        );

      }
      else {
        // the entry in the table must have been evicted since the prediction was made
        // TODO: put signature back in table??
        DBG_(Iface, ( << "MRP P[" << aNode <<"] Write @" << & std::hex << anAddr  << " #" << pred->signature << "# (Evicted) Pred: /" << pred->predicted.readers() << "/ Actual: \\" << pred->actual.readers() << & std::dec << "\\ hits: " << hits.count() << " misses: " << misses.count() << " training: " << training.count() ) );
        stats.EvictedPred++;
      }
    } else {
      DBG_(Iface, ( << "MRP P[" << aNode <<"] Write @" << & std::hex << anAddr  << " #No prior pred# " ) );
    }

    // create a history encoding based on the existing contents
    Signature mangled = myHistoryTable.makeHistory(anAddr);

    // lookup in the signature table
    if(mangled != 0) {
      mangled = mySigMapper.makeSig(anAddr, mangled);
#ifdef MRP_INFINITE
      Readers * theseReaders = mySignatureTable.lookup(anAddr, mangled);
      if(theseReaders) {
#else
      SigLookup result = mySignatureTable[mangled];
      if(result.hit()) {
        result.access(nMrpTable::AnyAccess);
        Readers * theseReaders = &( result.block().readers() );
#endif
        // make a prediction on subsequent readers
        myPredictions.addEntry( anAddr, isPriv, mangled, HistoryEvent(theseReaders->predict()) );
        stats.Predict++;
        theLastPredictionAddress = anAddr;
        theLastPrediction = theseReaders->predict();
        DBG_(Iface, ( << "MRP @" << & std::hex << anAddr  << " #" << mangled << & std::dec <<"# (" << *theseReaders  << ") Predicts: /" << & std::hex << theseReaders->predict() << & std::dec <<"/.") );

        DBG_(Trace, Cat(MRPTrace)
          SetNumeric( (Node) theMRPNode )
          Set( (Address) << "0x" << std::hex << std::setw(8) << std::setfill('0') << anAddr )
          Set( (Signature) << std::hex << std::setw(8) << std::setfill('0') << mangled )
          Set( (PredictedReaders) << std::hex << std::setw(4) << std::setfill('0') << theseReaders->predict() )
          SetNumeric( (Writer) aNode )
          Set( (Type) << "Predict" )
        );

      }
      else {
        // no entry in signature table - we haven't seen this pattern before
        unsigned int readers = myHistoryTable.currReaders(anAddr);
        if(readers != 0) {
          // this is something to predict next time!
#ifdef MRP_INFINITE
          mySignatureTable.add(anAddr, mangled, Readers(readers));
#else
          SigEntry & victim = result.victim();
          if(victim.valid()) {
            Signature vicaddr = mySignatureTable.makeAddress(victim.tag(),result.set().index());
            DBG_(VVerb, ( << "evicted block of addr " << std::hex << vicaddr ) );
            stats.SigEviction++;
          }
          victim.tag() = mySignatureTable.makeTag(mangled);
          DBG_(VVerb, ( << "new tag: 0x" << std::hex << victim.tag() ) );
          victim.readers().setReaders(readers);
          victim.setValid(true);
          result.access(nMrpTable::AnyAccess);
#endif
          DBG_(Iface, ( << "MRP @" << & std::hex << anAddr  << " #" << mangled << "# New signature for \\" << readers << & std::dec << "\\.") );

          DBG_(Trace, Cat(MRPTrace)
            SetNumeric( (Node) theMRPNode )
            Set( (Address) << "0x" << std::hex << std::setw(8) << std::setfill('0') << anAddr )
            Set( (Signature) << std::hex << std::setw(8) << std::setfill('0') << mangled )
            Set( (PredictedReaders) << std::hex << std::setw(4) << std::setfill('0') << readers )
            SetNumeric( (Writer) aNode )
            Set( (Type) << "NewSignature" )
          );


          stats.SigCreation++;
        }
        else {
          DBG_(Iface, ( << "MRP @" << & std::hex << anAddr  << " #" << mangled << & std::dec <<"# predicts no read.") );
          // no previous readers - nothing to predict
          stats.TrainingWrite++;
        }
      }
    }
    else {
      DBG_(Iface, ( << "MRP @" << & std::hex << anAddr  << & std::dec << " No history.") );
      stats.NoHistory++;
    }

    // update the history for this address
    myHistoryTable.writeAccess(anAddr, aNode);
  }

private:
  // principle data structures
  SignatureMapping mySigMapper;
  MrpHistoryTable myHistoryTable;
  MrpSigTable mySignatureTable;

  // statistics
  nMrpStats::MrpStats stats;

  MrpPredTable myPredictions;



};  // end class MrpMain

}  // end namespace nMrpTable
