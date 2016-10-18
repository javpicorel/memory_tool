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

#include "DgpStats.hpp"
#include "DgpCommon.hpp"
#include "DgpHistTable.hpp"

#ifdef DGP_INFINITE
  #include "DgpInfSigTable.hpp"
#else
  #include "DgpSigTable.hpp"
#endif

#include "DgpPredTable.hpp"


#include <fstream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

  #define DBG_DefineCategories DGPTrace
  #include DBG_Control()


namespace nDgpTable {

namespace ll = boost::lambda;


class DgpMain {
public:
  // General types
  typedef nDgpTable::Confidence Confidence;
  typedef nDgpTable::SignatureMapping SignatureMapping;
  typedef SignatureMapping::MemoryAddress MemoryAddress;
  typedef SignatureMapping::Signature Signature;

  // DGP History Table types
  typedef nDgpTable::DgpHistoryTable DgpHistoryTable;
  typedef DgpHistoryTable::HistoryEntry HistEntry;

  // DGP Signature Table types
  typedef nDgpTable::DgpSigTable DgpSigTable;
#ifndef DGP_INFINITE
  typedef DgpSigTable::DgpBlock SigEntry;
  typedef DgpSigTable::DgpLookupResult SigLookup;
#endif

  // DGP Prediction Table types
  typedef nDgpTable::DgpPredictionTable DgpPredTable;
  typedef DgpPredTable::PredEntry PredEntry;

  int theNode;
  bool thePredictsDowngrade;
  bool theWasMispredict;
  long long theSignature;
public:
  void finalizeStats() {
    myPredictions.finalizeStats();

    // generate per-signature end stats
    mySignatureTable0.finalize(stats);
    mySignatureTable1.finalize(stats);
  }

  DgpMain(int blockAddrBits, int pcBits, int l2BlockSize, int numSets, int assoc)
    : mySigMapper0(0, pcBits, l2BlockSize)
    , mySigMapper1(blockAddrBits, pcBits, l2BlockSize)
    , myHistoryTable()
    , mySignatureTable0(numSets, assoc)
    , mySignatureTable1(numSets, assoc)
    , myPredictions(mySigMapper0)
    , theUseHier(false)
    , theUseBoth(false)
    , theSmartBoth(false)
    , the1TableFlippingAddresses(".Common1TableFlipAddresses")
    /*
    , theDowngradesPerAddress(".DowngradesPerAddress")
    , theNumFirstPCs(".NumFirstPCs")
    , theNumFirstPCsPerAddress(".NumFirstPCsPerAddress")
    , theNumLastPCsPerFirstPC(".NumLastPCsPerFirstPC")
    , theNumPCsPerFirstPC(".NumPCsPerFirstPC")
    , theNumTracesPerFirstPC(".NumTracesPerFirstPC")
    */
  {}

  DgpMain(std::string const & name, int aNode, int blockAddrBits, int pcBits, int l2BlockSize, int numSets, int assoc)
    : theNode(aNode)
    , thePredictsDowngrade(false)
    , theWasMispredict(false)
    , mySigMapper0(0, pcBits, l2BlockSize)
    , mySigMapper1(blockAddrBits, pcBits, l2BlockSize)
    , myHistoryTable()
    , mySignatureTable0(numSets, assoc)
    , mySignatureTable1(numSets, assoc)
    , myPredictions(mySigMapper0)
    , stats(new DgpStats(name, blockAddrBits, pcBits, l2BlockSize, numSets, assoc))
    , theUseHier(false)
    //, theUseBoth(!theIgnoreOS)
    , theUseBoth(true)
    , theSmartBoth(false)
    , the1TableFlippingAddresses(name + ".Common1TableFlipAddresses")
    /*
    , theDowngradesPerAddress(name + ".DowngradesPerAddress")
    , theNumFirstPCs(name + ".NumFirstPCs")
    , theNumFirstPCsPerAddress(name + ".NumFirstPCsPerAddress")
    , theNumLastPCsPerFirstPC(name + ".NumLastPCsPerFirstPC")
    , theNumPCsPerFirstPC(name + ".NumPCsPerFirstPC")
    , theNumTracesPerFirstPC(name + ".NumTracesPerFirstPC")
    */
  {
    DBG_Assert(!(theUseBoth && theSmartBoth));
    Flexus::Stat::getStatManager()->addFinalizer( ll::bind( &DgpMain::finalizeStats, this) );
    myHistoryTable.setSigMapper( &mySigMapper0 );
  }

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version) {
    ar & myHistoryTable ;
    ar & mySignatureTable0 ;
    ar & mySignatureTable1 ;
    //DBG_(Crit, (<< "I'm resetting the history table... go smack Brian for this"));
    //myHistoryTable.reset();
    myHistoryTable.setSigMapper( &mySigMapper0 ); //Only needed on load, but harmless on save
  }

  void writeSigTable(std::string fname) {
    std::ofstream ofs(fname.c_str(), std::ios::binary);
    DBG_Assert(ofs.good(), (<< "Couldn't write to " << fname));
    boost::archive::binary_oarchive oa(ofs);

    oa << mySignatureTable0;
    oa << mySignatureTable1;

    ofs.close();
  }

  void readSigTable(std::string fname) {
    std::ifstream ifs(fname.c_str(), std::ios::binary);
    DBG_Assert(ifs.good(), (<< "Couldn't read from " << fname));
    boost::archive::binary_iarchive ia(ifs);

    ia >> mySignatureTable0;
    ia >> mySignatureTable1;

    ifs.close();
  }

  ~DgpMain() { }

  bool predictsDowngrade() { return thePredictsDowngrade; }
  bool wasMispredict() { return theWasMispredict; }
  long long signature() { return theSignature; }
  /*
  void saveHistTable(boost::archive::binary_oarchive& oa) {
    oa << myHistoryTable;
  }

  void saveSigTable(boost::archive::binary_oarchive& oa) {
    oa << mySignatureTable;
  }
  */

  friend std::ostream & operator << (std::ostream & anOstream, const DgpMain & aDgp) {
    anOstream << "*** DGP ***" << std::endl;
    anOstream << "History Table>>>>" << std::endl;
    anOstream << aDgp.myHistoryTable;
    anOstream << "<<<<History Table" << std::endl;
    anOstream << "Signature 0 Table>>>>" << std::endl;
    anOstream << aDgp.mySignatureTable0;
    anOstream << "<<<<Signature 0 Table" << std::endl;
    anOstream << "Signature 1 Table>>>>" << std::endl;
    anOstream << aDgp.mySignatureTable1;
    anOstream << "<<<<Signature 1 Table" << std::endl;
    return anOstream;
  }

  void predResolveWrite(MemoryAddress addr) {
    // check if there is an outstanding prediction for this address
    intrusive_ptr<PredEntry> pred = myPredictions.findAndRemove(addr);
    if(pred) {
      // a write access now indicates that a downgrade should NOT have
      // been predicted
      if(pred->predicted) {
        DBG_(VVerb, ( << "bad prediction 0x" << std::hex << addr << ": did predict when shouldn't have" ) );
        theWasMispredict = true;

        // incorrect prediction
        if(pred->initial) {
          DBG_Assert(false);
        } else {

          DBG_(Trace, Cat(DGPTrace)
            SetNumeric( (Node) theNode )
            Set( (Address) << "0x" << std::hex << std::setw(8) << std::setfill('0') << addr )
            Set( (Type) << "Mispredict" )
          );

          stats->PredBad++;
          pred->resolve(PredEntry::kBad);
          if(pred->priv) {
            stats->PredBadOS++;
          } else {
            stats->PredBadUser++;
          }
          if(pred->sigTable == 0) {
            stats->PredBad0Table++;
          } else if(pred->sigTable == 1) {
            stats->PredBad1Table++;
          }
          stats->PredBadConfs << std::make_pair(pred->confVals,1);

        }
      }
      else {
        DBG_(VVerb, ( << "good prediction 0x" << std::hex << addr << ": didn't predict when shouldn't have" ) );
        // good prediction
        if(pred->initial) {

          stats->InitialGood++;
          if(pred->priv) {
            stats->InitialGoodOS++;
          } else {
            stats->InitialGoodUser++;
          }
          if(pred->sigTable == 0) {
            stats->InitialGood0Table++;
          } else if(pred->sigTable == 1) {
            stats->InitialGood1Table++;
          }

        } else {

          stats->LowConfGood++;
          if(pred->priv) {
            stats->LowConfGoodOS++;
          } else {
            stats->LowConfGoodUser++;
          }
          if(pred->sigTable == 0) {
            stats->LowConfGood0Table++;
          } else if(pred->sigTable == 1) {
            stats->LowConfGood1Table++;
          }
          stats->NoPredGoodConfs << std::make_pair(pred->confVals,1);

        }
      }
    }
  }

  void predResolveSnoop(MemoryAddress addr) {
    // check if there is an outstanding prediction for this address
    intrusive_ptr<PredEntry> pred = myPredictions.findAndRemove(addr);
    if(pred) {
      // a snoop access now indicates that a downgrade SHOULD have
      // been predicted
      if(pred->predicted) {
        DBG_(VVerb, ( << "good prediction 0x" << std::hex << addr << ": did predict when should have" ) );
        // good prediction
        if(pred->initial) {
          DBG_Assert(false);
        } else {

          DBG_(Trace, Cat(DGPTrace)
            SetNumeric( (Node) theNode )
            Set( (Address) << "0x" << std::hex << std::setw(8) << std::setfill('0') << addr )
            Set( (Type) << "Confirm" )
          );

          stats->PredGood++;
          pred->resolve(PredEntry::kGood);
          if(pred->priv) {
            stats->PredGoodOS++;
          } else {
            stats->PredGoodUser++;
          }
          if(pred->sigTable == 0) {
            stats->PredGood0Table++;
          } else if(pred->sigTable == 1) {
            stats->PredGood1Table++;
          }
          stats->PredGoodConfs << std::make_pair(pred->confVals,1);

        }
      }
      else {
        DBG_(VVerb, ( << "bad prediction 0x" << std::hex << addr << ": didn't predict when should have" ) );
        // incorrect prediction
        if(pred->initial) {

          stats->InitialBad++;
          if(pred->priv) {
            stats->InitialBadOS++;
          } else {
            stats->InitialBadUser++;
          }
          if(pred->sigTable == 0) {
            stats->InitialBad0Table++;
          } else if(pred->sigTable == 1) {
            stats->InitialBad1Table++;
          }

        } else {

          stats->LowConfBad++;
          if(pred->priv) {
            stats->LowConfBadOS++;
          } else {
            stats->LowConfBadUser++;
          }
          if(pred->sigTable == 0) {
            stats->LowConfBad0Table++;
          } else if(pred->sigTable == 1) {
            stats->LowConfBad1Table++;
          }
          //stats->LowConfBadSigs << std::make_pair(pred->signature,1);

        }

        stats->NoPredBadConfs << std::make_pair(pred->confVals,1);
      }
    }
  }

  void handleWrite(MemoryAddress addr, intrusive_ptr<HistEntry> currHistory) {
    bool accounted = false;
    // create the signatures
    Signature oldSig0 = mySigMapper0.makeSig(addr, currHistory->theHist);
    Signature oldSig1 = mySigMapper1.makeSig(addr, currHistory->theHist);
    Confidence * conf0 = 0;
    Confidence * conf1 = 0;

    // first lookup in the high-bit signature table
#ifdef DGP_INFINITE
    conf1 = mySignatureTable1.lookup(addr, oldSig1);
#else
    SigLookup oldResult1 = mySignatureTable1[oldSig1];
    if(oldResult1.hit()) {
      conf1 = &( oldResult1.block().conf() );
    }
#endif

    // if a signature exists, lower confidence by 1 since it would
    // have been wrong to predict
    if(conf1) {
      bool oldPred = conf1->predict();
      //bool remove = conf1->lower();
      bool remove = conf1->lower(stats, 1);

      if(conf1->isFlipping()) {
        if(oldPred) {
          stats->FlippingPredBad++;
          stats->FlippingPredBad1Table++;
        } else {
          stats->FlippingLowConfGood++;
          stats->FlippingLowConfGood1Table++;
        }
      }

#ifdef DGP_SUBTRACE
      if(oldPred) {
        // this signature is a possible subtrace
        if(!conf1->isSubtrace()) {
          intrusive_ptr<HistEntry> hist = myHistoryTable.find(addr);
          if(hist) {
            hist->addSubtraces(conf1);
          }
        }
      }
#endif

      if(remove) {
#ifdef DGP_INFINITE
        // don't remove from an infinite table
        //mySignatureTable1.remove(addr, oldSig1);
#else
        oldResult1.remove();
#endif
      }

      accounted = true;
      stats->ConfidenceLower++;
      stats->ConfidenceLower1Table++;
    }

    // lookup in the low-bit signature table only if the high-bit table
    // did not contain the desired signature
    if(theUseHier && (!conf1 || theUseBoth || theSmartBoth)) {
#ifdef DGP_INFINITE
      conf0 = mySignatureTable0.lookup(addr, oldSig0);
#else
      SigLookup oldResult0 = mySignatureTable0[oldSig0];
      if(oldResult0.hit()) {
        conf0 = &( oldResult0.block().conf() );
      }
#endif

      // if a signature exists, lower confidence by 1 since it would
      // have been wrong to predict
      if(conf0) {
        bool oldPred = conf0->predict();
        //bool remove = conf0->lower();
        bool remove = conf0->lower(stats, 0);

        // never actually remove from the low-bit table

        if(conf0->isFlipping()) {
          if(oldPred) {
            stats->FlippingPredBad++;
            stats->FlippingPredBad0Table++;
          } else {
            stats->FlippingLowConfGood++;
            stats->FlippingLowConfGood0Table++;
          }
        }

        if(!accounted) {
          accounted = true;
          stats->ConfidenceLower++;
        }
        stats->ConfidenceLower0Table++;
      }
    }

    if(!conf1 && !conf0) {
      // probably no signature has been created for this history
      // (sig eviction is also possible, but less likely)
      DBG_(VVerb, ( << "no signature for lower confidence by 1 0x" << &std::hex << addr << " history " << currHistory) );
      stats->NoSigLower++;
    }
  }

  void handleSnoop(MemoryAddress addr, intrusive_ptr<HistEntry> currHistory) {
    bool accounted = false;
    // create the signatures
    Signature sig0 = mySigMapper0.makeSig(addr, currHistory->theHist);
    Signature sig1 = mySigMapper1.makeSig(addr, currHistory->theHist);
    Confidence * conf0 = 0;
    Confidence * conf1 = 0;

    // first lookup in the high-bit signature table
#ifdef DGP_INFINITE
    conf1 = mySignatureTable1.lookup(addr, sig1);
#else
    SigLookup result1 = mySignatureTable1[sig1];
    if(result1.hit()) {
      result1.access(nDgpTable::SnoopAccess);
      conf1 = &( result1.block().conf() );
    }
#endif

    // if a signature exists, raise confidence by 1 because it would
    // have been correct to predict
    if(conf1) {
      DBG_(VVerb, ( << "signature already exists for addr 0x" << std::hex << addr ) );
      bool oldPred = conf1->predict();
      // raise confidence
      //conf1->raise();
      conf1->raise(stats, 1);

      accounted = true;
      stats->ConfidenceRaise++;
      stats->ConfidenceRaise1Table++;
      //DBG_(Crit, (<< "DGP Production - ConfidenceRaise on " << std::hex << addr));
      //the1TableFlippingAddresses << std::make_pair(addr,1);

      if(conf1->isFlipping()) {
        if(oldPred) {
          stats->FlippingPredGood++;
          stats->FlippingPredGood1Table++;
        } else {
          stats->FlippingLowConfBad++;
          stats->FlippingLowConfBad1Table++;
        }
      }

#ifdef DGP_SUBTRACE
      if(oldPred) {
        // if subtraces exist, add them to the appropriate list
        intrusive_ptr<HistEntry> subHist = myHistoryTable.find(addr);
        if(subHist) {
          if(!subHist->theSubtraces.empty()) {
            if(conf1->isSubtrace()) {
              Confidence * super = conf1->getSupertrace();
              while(super->isSubtrace()) {
                super = super->getSupertrace();
              }
              if(!super->isSupertrace()) {
                DBG_(Dev, ( << "super-most is not a Supertrace" ) );
              }
              super->addSubtraces(subHist->theSubtraces);
            }
            else {
              conf-1>addSubtraces(subHist->theSubtraces);
            }
          }
        }
      }

      // subtrace info
      if(conf1->isSubtrace()) {
        conf->getSupertrace()->numStores(currHistory->theNumPCs);
      } else {
        conf->numStores(currHistory->theNumPCs);
      }
#endif

    }

    // if not found in the high-bit table, lookup in the low-bit table
    bool createInHigh = false;
    if(theUseHier && (!conf1 || theUseBoth || theSmartBoth)) {
#ifdef DGP_INFINITE
      conf0 = mySignatureTable0.lookup(addr, sig0);
#else
      SigLookup result0 = mySignatureTable0[sig0];
      if(result0.hit()) {
        result0.access(nDgpTable::SnoopAccess);
        conf0 = &( result0.block().conf() );
      }
#endif

      if(conf0) {
        // if the signature exists with minimum confidence, then create a
        // new signature in the high-bit signature table (we know the sig
        // is not already present in the high-bit table)
        if(conf0->min() || theUseBoth || theSmartBoth) {
          createInHigh = true;
        }

        //if(!conf0->min() || theUseBoth /*|| theSmartBoth*/) {  // G
        //if(!createInHigh || theUseBoth /*|| theSmartBoth*/) {  // H
        //if(theSmartBoth) {  // I
        if(!createInHigh || theUseBoth) {
          // otherwise raise confidence by 1 because it would have been correct to predict
          DBG_(VVerb, ( << "signature already exists for addr 0x" << std::hex << addr ) );
          bool oldPred = conf0->predict();
          // raise confidence
          //conf0->raise();
          conf0->raise(stats, 0);

          if(!accounted) {
            accounted = true;
            stats->ConfidenceRaise++;
          }
          stats->ConfidenceRaise0Table++;
          //DBG_(Crit, (<< "DGP Production - ConfidenceRaise on " << std::hex << addr));

          if(conf0->isFlipping()) {
            if(oldPred) {
              stats->FlippingPredGood++;
              stats->FlippingPredGood0Table++;
            } else {
              stats->FlippingLowConfBad++;
              stats->FlippingLowConfBad0Table++;
            }
          }

#ifdef DGP_SUBTRACE
          if(oldPred) {
            // if subtraces exist, add them to the appropriate list
            intrusive_ptr<HistEntry> subHist = myHistoryTable.find(addr);
            if(subHist) {
              if(!subHist->theSubtraces.empty()) {
                if(conf0->isSubtrace()) {
                  Confidence * super = conf0->getSupertrace();
                  while(super->isSubtrace()) {
                    super = super->getSupertrace();
                  }
                  if(!super->isSupertrace()) {
                    DBG_(Dev, ( << "super-most is not a Supertrace" ) );
                  }
                  super->addSubtraces(subHist->theSubtraces);
                }
                else {
                  conf0->addSubtraces(subHist->theSubtraces);
                }
              }
            }
          }

          // subtrace info
          if(conf0->isSubtrace()) {
            conf0->getSupertrace()->numStores(currHistory->theNumPCs);
          } else {
            conf0->numStores(currHistory->theNumPCs);
          }
#endif

        }
      }
      else {
        DBG_(VVerb, ( << "adding signature for addr 0x" << std::hex << addr ) );
#ifdef DGP_INFINITE
        // not in the table - add it
        mySignatureTable0.add(addr, sig0);
#else
        // not in the table - find a victim and replace it
        SigEntry & victim = result0.victim();
        if(victim.valid()) {
          Signature vicaddr = mySignatureTable0.makeAddress(victim.tag(),result0.set().index());
          DBG_(VVerb, ( << "evicted block of addr 0x" << std::hex << vicaddr ) );
          stats->SigEviction++;
          stats->SigEviction0Table++;
        }
        victim.tag() = mySignatureTable0.makeTag(sig0);
        victim.conf().reset();
        victim.setValid(true);
        result0.access(nDgpTable::SnoopAccess);
#endif
        if(!accounted) {
          accounted = true;
          stats->SigCreation++;
        }
        stats->SigCreation0Table++;
        //DBG_(Crit, (<< "DGP Production - SigCreation on " << std::hex << addr));
      }
    }

    // if directed, add the signature to the high-bit table
    if(!conf1 && (!theUseHier || createInHigh)) {
      DBG_(VVerb, ( << "adding signature for addr 0x" << std::hex << addr ) );
#ifdef DGP_INFINITE
      // not in the table - add it
      mySignatureTable1.add(addr, sig1);
#else
      // not in the table - find a victim and replace it
      SigEntry & victim = result1.victim();
      if(victim.valid()) {
        Signature vicaddr = mySignatureTable1.makeAddress(victim.tag(),result1.set().index());
        DBG_(VVerb, ( << "evicted block of addr 0x" << std::hex << vicaddr ) );
        stats->SigEviction++;
        stats->SigEviction1Table++;
      }
      victim.tag() = mySignatureTable1.makeTag(sig1);
      victim.conf().reset();
      victim.setValid(true);
      result1.access(nDgpTable::SnoopAccess);
#endif
      if(!accounted) {
        accounted = true;
        stats->SigCreation++;
      }
      stats->SigCreation1Table++;
      //DBG_(Crit, (<< "DGP Production - SigCreation on " << std::hex << addr));
    }

    // a snoop indicates consumption (i.e. end of the history trace)
    DBG_(VVerb, ( << "History removed 3 0x" << &std::hex << addr << " history " << currHistory) );
    myHistoryTable.remove(addr);
  }

  void makePrediction(MemoryAddress addr, Signature history, bool isPriv) {
    bool accounted = false;
    // create the signatures
    Signature sig0 = mySigMapper0.makeSig(addr, history);
    Signature sig1 = mySigMapper1.makeSig(addr, history);
    Confidence * conf0 = 0;
    Confidence * conf1 = 0;
    unsigned char confVal0 = 4;
    unsigned char confVal1 = 4;

    bool predict = false;
    int table = 0;

    // first lookup in the high-bit signature table
#ifdef DGP_INFINITE
    conf1 = mySignatureTable1.lookup(addr, sig1);
#else
    SigLookup result1 = mySignatureTable1[sig1];
    if(result1.hit()) {
      result1.access(nDgpTable::WriteAccess);
      conf1 = &( result1.block().conf() );
    }
#endif

    // use the current confidence to determine if a self-downgrade
    // should be initiated
    if(conf1) {
      confVal1 = conf1->val();
      if(conf1->predict()) {
        predict = true;
        table = 1;
        accounted = true;
      }
      else {
        if(!theUseHier || !(theUseBoth || theSmartBoth)) {
          predict = false;
          table = 1;
          accounted = true;
        }
      }
    }

    // now check the low-bit signature table if necessary
    if(!accounted || theUseHier) {
#ifdef DGP_INFINITE
      conf0 = mySignatureTable0.lookup(addr, sig0);
#else
      SigLookup result0 = mySignatureTable0[sig0];
      if(result0.hit()) {
        result0.access(nDgpTable::WriteAccess);
        conf0 = &( result0.block().conf() );
      }
#endif

      if(conf0) {
        confVal0 = conf0->val();

        if(!accounted) {
          // we will make a pred entry with table zero in all cases
          accounted = true;
          table = 0;

          // use the current confidence to determine if a self-downgrade
          // should be initiated
          if(conf0->predict()) {
            predict = true;
            /*
            if(!conf1) {
              // no matching high-bit signature, so predict based on low-bit
              predict = true;
            }
            // what to do?  (already know hi-bit has low confidence)
            else if(theUseBoth || conf0->max()) {
              // predict if low-bit counter has saturated
              predict = true;
            }
            else {
              // don't predict - unsaturated confidence
              predict = false;
            }
            */
          }
          else {
            predict = false;
          }
        }

      }  // if conf0
    }

    if(!accounted && conf1) {
      predict = false;
      table = 1;
      accounted = true;
    }

    if(!conf1 && !conf0) {
      // no entry in signature table - we haven't seen this pattern before
      stats->Training++;
    }
    else {
      DBG_Assert(accounted);
      // perform the actual prediction
      Confidence * predConf( conf0 );
      Signature predSig( sig0 );
      if(table == 1) {
        predConf = conf1;
        predSig = sig1;
      }
      long long confVals = ((long long)confVal0 * 10) + confVal1;

      if(predict) {
        // time to predict the downgrade!
        DBG_(VVerb, ( << "predicting on addr 0x" << std::hex << addr ) );
        myPredictions.addEntry(addr, predSig, true, predConf->initial(), isPriv, table, confVals, *stats);
        if(predConf->initial()) {
          DBG_Assert(false);
        } else {

          DBG_(Trace, Cat(DGPTrace)
            SetNumeric( (Node) theNode )
            Set( (Address) << "0x" << std::hex << std::setw(8) << std::setfill('0') << addr )
            Set( (Type) << "Predict" )
          );

          thePredictsDowngrade = true;
          theSignature = predSig;
          stats->Predict++;
        }
      }
      else {
        DBG_(VVerb, ( << "not predicting on addr 0x" << std::hex << addr ) );
        // remember that we decided to not predict this
        myPredictions.addEntry(addr, predSig, false, predConf->initial(), isPriv, table, confVals, *stats);
        if(predConf->initial()) {
          stats->Initial++;
        } else {
          stats->LowConf++;
        }
      }
    }

  }

#define DEBUG_ADDR 0x7c00e400

  bool write(MemoryAddress addr, MemoryAddress pc, bool isPriv) {
    if((theNode == 9) && (addr == DEBUG_ADDR)) {
      //DBG_(Dev, ( << "write, PC = 0x" << std::hex << pc ) );
    }

    thePredictsDowngrade = false;
    theWasMispredict = false;

    // first update the prediction statistics
    predResolveWrite(addr);

    // retrieve an existing history trace for this address, and update
    // its entry in the signature table if necessary
    intrusive_ptr<HistEntry> oldHistory = myHistoryTable.find(addr);
    if(oldHistory) {
      handleWrite(addr, oldHistory);
    }
    else {
      // no existing history - this was either cold, the first write
      // after an invalidation, or the history was evicted before this
      // message arrived to resolve the prediction
      stats->NoHistory++;
    }

    // add this access to the history table - doesn't matter if it continues
    // an existing trace or if it creates a new entry
    Signature history = myHistoryTable.access(addr, pc);

    // make a prediction, if appropriate
    makePrediction(addr, history, isPriv);

    return thePredictsDowngrade;
  }

  void snoop(MemoryAddress addr) {
    if((theNode == 9) && (addr == DEBUG_ADDR)) {
      //DBG_(Dev, ( << "snoop" ) );
    }

    // first update the prediction statistics
    predResolveSnoop(addr);

    // check the history table for a trace for this address
    intrusive_ptr<HistEntry> history = myHistoryTable.find(addr);

    if(history) {
      handleSnoop(addr, history);

      /*
      std::map<tAddress,long>::iterator iter = theCountedDowngrades.find(addr);
      if(iter != theCountedDowngrades.end()) {
        theDowngradesPerAddress << std::make_pair(iter->second,-1);
        ++(iter->second);
        theDowngradesPerAddress << std::make_pair(iter->second,1);
      } else {
        theCountedDowngrades.insert( std::make_pair(addr,1) );
        theDowngradesPerAddress << std::make_pair(1,1);
      }

      if(theFirstPCs.find(history->theFirstPC) == theFirstPCs.end()) {
        theFirstPCs.insert(history->theFirstPC);
        theNumFirstPCs++;
      }

      std::map< tAddress,std::set<tAddress> >::iterator iter2 = theFirstPCsPerAddress.find(addr);
      if(iter2 != theFirstPCsPerAddress.end()) {
        if(iter2->second.find(history->theFirstPC) == iter2->second.end()) {
          // new first PC for this address
          theNumFirstPCsPerAddress << std::make_pair(iter2->second.size(),-1);
          iter2->second.insert(history->theFirstPC);
          theNumFirstPCsPerAddress << std::make_pair(iter2->second.size(),1);
        }
      }
      else {
        iter2 = theFirstPCsPerAddress.insert( std::make_pair(addr,std::set<tAddress>()) ).first;
        iter2->second.insert(history->theFirstPC);
        theNumFirstPCsPerAddress << std::make_pair(1,1);
      }

      std::map< tAddress,std::set<tAddress> >::iterator iter3 = theLastPCsPerFirstPC.find(history->theFirstPC);
      if(iter3 != theLastPCsPerFirstPC.end()) {
        if(iter3->second.find(history->theLastPC) == iter3->second.end()) {
          // new last PC for this start PC
          theNumLastPCsPerFirstPC << std::make_pair(iter3->second.size(),-1);
          iter3->second.insert(history->theLastPC);
          theNumLastPCsPerFirstPC << std::make_pair(iter3->second.size(),1);
        }
      }
      else {
        iter3 = theLastPCsPerFirstPC.insert( std::make_pair(history->theFirstPC,std::set<tAddress>()) ).first;
        iter3->second.insert(history->theLastPC);
        theNumLastPCsPerFirstPC << std::make_pair(1,1);
      }

      std::map< tAddress,std::set<long> >::iterator iter4 = thePCsPerFirstPC.find(history->theFirstPC);
      if(iter4 != thePCsPerFirstPC.end()) {
        if(iter4->second.find(history->theNumPCs) == iter4->second.end()) {
          // new number of PCs for this start PC
          theNumPCsPerFirstPC << std::make_pair(iter4->second.size(),-1);
          iter4->second.insert(history->theNumPCs);
          theNumPCsPerFirstPC << std::make_pair(iter4->second.size(),1);
        }
      }
      else {
        iter4 = thePCsPerFirstPC.insert( std::make_pair(history->theFirstPC,std::set<long>()) ).first;
        iter4->second.insert(history->theNumPCs);
        theNumPCsPerFirstPC << std::make_pair(1,1);
      }

      std::map< tAddress,std::set<Signature> >::iterator iter5 = theTracesPerFirstPC.find(history->theFirstPC);
      if(iter5 != theTracesPerFirstPC.end()) {
        if(iter5->second.find(history->theHist) == iter5->second.end()) {
          // new PC-trace for this start PC
          theNumTracesPerFirstPC << std::make_pair(iter5->second.size(),-1);
          iter5->second.insert(history->theHist);
          theNumTracesPerFirstPC << std::make_pair(iter5->second.size(),1);
        }
      }
      else {
        iter5 = theTracesPerFirstPC.insert( std::make_pair(history->theFirstPC,std::set<Signature>()) ).first;
        iter5->second.insert(history->theHist);
        theNumTracesPerFirstPC << std::make_pair(1,1);
      }
      */

    }
    else {
      // no history - this might be a spurious snoop, or for a read-only
      // block... regardless, with no history trace, we can't build a
      // signature (and hence do anything with the sig table)
      stats->SpuriousSnoop++;
    }
  }


private:
  // principle data structures
  SignatureMapping mySigMapper0;
  SignatureMapping mySigMapper1;
  DgpHistoryTable myHistoryTable;
  DgpSigTable mySignatureTable0;
  DgpSigTable mySignatureTable1;
  DgpPredTable myPredictions;

  // statistics
  DgpStats *stats;
  bool theUseHier;
  bool theUseBoth;
  bool theSmartBoth;

  // DGP investigation
  Flexus::Stat::StatInstanceCounter<long long> the1TableFlippingAddresses;

  /*
  Flexus::Stat::StatInstanceCounter<long long> theDowngradesPerAddress;
  std::map<tAddress,long> theCountedDowngrades;

  Flexus::Stat::StatCounter theNumFirstPCs;
  std::set<tAddress> theFirstPCs;
  Flexus::Stat::StatInstanceCounter<long long> theNumFirstPCsPerAddress;
  std::map< tAddress,std::set<tAddress> > theFirstPCsPerAddress;
  Flexus::Stat::StatInstanceCounter<long long> theNumLastPCsPerFirstPC;
  std::map< tAddress,std::set<tAddress> > theLastPCsPerFirstPC;
  Flexus::Stat::StatInstanceCounter<long long> theNumPCsPerFirstPC;
  std::map< tAddress,std::set<long> > thePCsPerFirstPC;
  Flexus::Stat::StatInstanceCounter<long long> theNumTracesPerFirstPC;
  std::map< tAddress,std::set<Signature> > theTracesPerFirstPC;
  */

};  // end class DgpMain

}  // end namespace nDgpTable
