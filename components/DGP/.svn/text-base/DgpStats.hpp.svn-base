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

struct DgpStats {
  StatCounter Predict;
  StatCounter PredGood;
  StatCounter PredBad;
  StatCounter LowConf;
  StatCounter LowConfGood;
  StatCounter LowConfBad;
  StatCounter Initial;
  StatCounter InitialGood;
  StatCounter InitialBad;

  StatCounter PredGoodOS;
  StatCounter PredGoodUser;
  StatCounter PredBadOS;
  StatCounter PredBadUser;
  StatCounter LowConfGoodOS;
  StatCounter LowConfGoodUser;
  StatCounter LowConfBadOS;
  StatCounter LowConfBadUser;
  StatCounter InitialGoodOS;
  StatCounter InitialGoodUser;
  StatCounter InitialBadOS;
  StatCounter InitialBadUser;

  StatCounter PredGood0Table;
  StatCounter PredGood1Table;
  StatCounter PredBad0Table;
  StatCounter PredBad1Table;
  StatCounter LowConfGood0Table;
  StatCounter LowConfGood1Table;
  StatCounter LowConfBad0Table;
  StatCounter LowConfBad1Table;
  StatCounter InitialGood0Table;
  StatCounter InitialGood1Table;
  StatCounter InitialBad0Table;
  StatCounter InitialBad1Table;

  StatCounter ConfidenceLower;
  StatCounter NoSigLower;
  StatCounter ConfidenceRaise;
  StatCounter NoHistory;
  StatCounter Training;
  StatCounter SigCreation;
  StatCounter SigEviction;
  StatCounter SigFlipflop;
  StatCounter SpuriousSnoop;

  StatCounter ConfidenceLower0Table;
  StatCounter ConfidenceLower1Table;
  StatCounter ConfidenceRaise0Table;
  StatCounter ConfidenceRaise1Table;
  StatCounter SigCreation0Table;
  StatCounter SigCreation1Table;
  StatCounter SigEviction0Table;
  StatCounter SigEviction1Table;

  StatPredictionCounter CorrectPredictions;
  StatPredictionCounter Mispredictions;
  StatPredictionCounter DanglingPredictions;

  StatCounter FlippingPredGood;
  StatCounter FlippingPredBad;
  StatCounter FlippingLowConfGood;
  StatCounter FlippingLowConfBad;

  StatCounter FlippingPredGood0Table;
  StatCounter FlippingPredGood1Table;
  StatCounter FlippingPredBad0Table;
  StatCounter FlippingPredBad1Table;
  StatCounter FlippingLowConfGood0Table;
  StatCounter FlippingLowConfGood1Table;
  StatCounter FlippingLowConfBad0Table;
  StatCounter FlippingLowConfBad1Table;

  //StatInstanceCounter<Flexus::Core::MemoryAddress_<Flexus::Core::Word32Bit> > LowConfBadSigs;
  StatInstanceCounter<long long> ConfTransitions;
  StatInstanceCounter<long long> ConfTransitions0Table;
  StatInstanceCounter<long long> ConfTransitions1Table;

  StatInstanceCounter<long long> PredGoodConfs;
  StatInstanceCounter<long long> PredBadConfs;
  StatInstanceCounter<long long> NoPredGoodConfs;
  StatInstanceCounter<long long> NoPredBadConfs;

  StatInstanceCounter<long long> NumSubtraces;
  StatInstanceCounter<long long> NumStores;

  StatAnnotation Organization;
  StatAnnotation Configuration;

  DgpStats( std::string const & aName, int blockAddrBits, int pcBits, int l2BlockSize, int numSets, int assoc)
   : Predict                ( aName + "-Predict"                )
   , PredGood               ( aName + "-PredGood"               )
   , PredBad                ( aName + "-PredBad"                )
   , LowConf                ( aName + "-LowConf"                )
   , LowConfGood            ( aName + "-LowConfGood"            )
   , LowConfBad             ( aName + "-LowConfBad"             )
   , Initial                ( aName + "-Initial"                )
   , InitialGood            ( aName + "-InitialGood"            )
   , InitialBad             ( aName + "-InitialBad"             )

   , PredGoodOS             ( aName + ".PredGoodOS"             )
   , PredGoodUser           ( aName + ".PredGoodUser"           )
   , PredBadOS              ( aName + ".PredBadOS"              )
   , PredBadUser            ( aName + ".PredBadUser"            )
   , LowConfGoodOS          ( aName + ".LowConfGoodOS"          )
   , LowConfGoodUser        ( aName + ".LowConfGoodUser"        )
   , LowConfBadOS           ( aName + ".LowConfBadOS"           )
   , LowConfBadUser         ( aName + ".LowConfBadUser"         )
   , InitialGoodOS          ( aName + ".InitialGoodOS"          )
   , InitialGoodUser        ( aName + ".InitialGoodUser"        )
   , InitialBadOS           ( aName + ".InitialBadOS"           )
   , InitialBadUser         ( aName + ".InitialBadUser"         )

   , PredGood0Table         ( aName + ".PredGood0Table"         )
   , PredGood1Table         ( aName + ".PredGood1Table"         )
   , PredBad0Table          ( aName + ".PredBad0Table"          )
   , PredBad1Table          ( aName + ".PredBad1Table"          )
   , LowConfGood0Table      ( aName + ".LowConfGood0Table"      )
   , LowConfGood1Table      ( aName + ".LowConfGood1Table"      )
   , LowConfBad0Table       ( aName + ".LowConfBad0Table"       )
   , LowConfBad1Table       ( aName + ".LowConfBad1Table"       )
   , InitialGood0Table      ( aName + ".InitialGood0Table"      )
   , InitialGood1Table      ( aName + ".InitialGood1Table"      )
   , InitialBad0Table       ( aName + ".InitialBad0Table"       )
   , InitialBad1Table       ( aName + ".InitialBad1Table"       )

   , ConfidenceLower        ( aName + "-ConfidenceLower"        )
   , NoSigLower             ( aName + "-NoSigLower"             )
   , ConfidenceRaise        ( aName + "-ConfidenceRaise"        )
   , NoHistory              ( aName + "-NoHistory"              )
   , Training               ( aName + "-Training"               )
   , SigCreation            ( aName + "-SigCreation"            )
   , SigEviction            ( aName + "-SigEviction"            )
   , SigFlipflop            ( aName + ".SigFlipflop"            )
   , SpuriousSnoop          ( aName + "-SpuriousSnoop"          )

   , ConfidenceLower0Table  ( aName + "-ConfidenceLower0Table"  )
   , ConfidenceLower1Table  ( aName + "-ConfidenceLower1Table"  )
   , ConfidenceRaise0Table  ( aName + "-ConfidenceRaise0Table"  )
   , ConfidenceRaise1Table  ( aName + "-ConfidenceRaise1Table"  )
   , SigCreation0Table      ( aName + "-SigCreation0Table"      )
   , SigCreation1Table      ( aName + "-SigCreation1Table"      )
   , SigEviction0Table      ( aName + "-SigEviction0Table"      )
   , SigEviction1Table      ( aName + "-SigEviction1Table"      )

   , CorrectPredictions     ( aName + "-CorrectPrediction"      )
   , Mispredictions         ( aName + "-Mispredictions"         )
   , DanglingPredictions    ( aName + "-DanglingPredictions"    )

   , FlippingPredGood       ( aName + ".FlippingPredGood"       )
   , FlippingPredBad        ( aName + ".FlippingPredBad"        )
   , FlippingLowConfGood    ( aName + ".FlippingLowConfGood"    )
   , FlippingLowConfBad     ( aName + ".FlippingLowConfBad"     )

   , FlippingPredGood0Table     ( aName + ".FlippingPredGood0Table"     )
   , FlippingPredGood1Table     ( aName + ".FlippingPredGood1Table"     )
   , FlippingPredBad0Table      ( aName + ".FlippingPredBad0Table"      )
   , FlippingPredBad1Table      ( aName + ".FlippingPredBad1Table"      )
   , FlippingLowConfGood0Table  ( aName + ".FlippingLowConfGood0Table"  )
   , FlippingLowConfGood1Table  ( aName + ".FlippingLowConfGood1Table"  )
   , FlippingLowConfBad0Table   ( aName + ".FlippingLowConfBad0Table"   )
   , FlippingLowConfBad1Table   ( aName + ".FlippingLowConfBad1Table"   )

   , ConfTransitions        ( aName + ".ConfidenceTransitions"  )
   , ConfTransitions0Table  ( aName + ".ConfTransitions0Table"  )
   , ConfTransitions1Table  ( aName + ".ConfTransitions1Table"  )
   , PredGoodConfs          ( aName + ".PredGoodConfs"          )
   , PredBadConfs           ( aName + ".PredBadConfs"           )
   , NoPredGoodConfs        ( aName + ".NoPredGoodConfs"        )
   , NoPredBadConfs         ( aName + ".NoPredBadConfs"         )
   , NumSubtraces           ( aName + ".NumSubtraces"           )
   , NumStores              ( aName + ".NumStores"              )

#ifdef DGP_INFINITE
   , Organization        ( aName + "-Organization", std::string("infinite"))
#else
   , Organization        ( aName + "-Organization", std::string("finite"))
#endif
   , Configuration        (   std::string("Block Addr Bits: ")
                            + boost::lexical_cast<std::string>(blockAddrBits)
                            + "PC Bits: "
                            + boost::lexical_cast<std::string>(pcBits)
                            + "Cache Block Size: "
                            + boost::lexical_cast<std::string>(l2BlockSize)
                            + "Num Sets: "
                            + boost::lexical_cast<std::string>(numSets)
                            + "Associativity: "
                            + boost::lexical_cast<std::string>(assoc)
                          )
   {}

};  // end struct DgpStats

}

