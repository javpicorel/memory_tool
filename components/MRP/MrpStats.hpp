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
namespace nMrpStats {

using namespace Flexus::Stat;

struct MrpStats {


/*
  StatPredictionCounter PredExact;
  StatPredictionCounter PredMoreActual;
  StatPredictionCounter PredMorePredicted;
  StatPredictionCounter PredInexactMoreActual;
  StatPredictionCounter PredInexactMorePredicted;
  StatPredictionCounter PredDifferent;
  StatPredictionCounter PredOpposite;
*/

  StatPredictionCounter RealReaders_User;
  StatPredictionCounter RealHits_User;
  StatPredictionCounter RealTraining_User;
  StatPredictionCounter RealMisses_User;
  StatPredictionCounter DanglingMisses_User;

  StatPredictionCounter RealReaders_OS;
  StatPredictionCounter RealHits_OS;
  StatPredictionCounter RealTraining_OS;
  StatPredictionCounter RealMisses_OS;
  StatPredictionCounter DanglingMisses_OS;

#ifdef MRP_INSTANCE_COUNTERS
  StatInstanceCounter<long long> HitInstances;
  StatInstanceCounter<long long> TrainingInstances;
  StatInstanceCounter<long long> MissInstances;

#endif

  StatCounter ReadInitial;
  StatCounter ReadAgain;

  StatCounter Predict;
  StatCounter SigCreation;
  StatCounter SigEviction;
  StatCounter TrainingRead_User;
  StatCounter TrainingRead_OS;
  StatCounter TrainingWrite;
  StatCounter EvictedPred;
  StatCounter NoHistory;

  StatAnnotation Organization;

  MrpStats(std::string const & aName)
   :
/*     PredExact                ( aName + "-PredExact"               )
   , PredMoreActual           ( aName + "-PredMoreActual"          )
   , PredMorePredicted        ( aName + "-PredMorePredicted"       )
   , PredInexactMoreActual    ( aName + "-PredInexactMoreActual"   )
   , PredInexactMorePredicted ( aName + "-PredInexactMorePredicted")
   , PredDifferent            ( aName + "-PredDifferent"           )
   , PredOpposite             ( aName + "-PredOpposite"            )
*/
     RealReaders_User         ( aName + "-user-RealReaders"       )
   , RealHits_User            ( aName + "-user-RealHits"          )
   , RealTraining_User        ( aName + "-user-RealTraining"      )
   , RealMisses_User          ( aName + "-user-RealMisses"        )
   , DanglingMisses_User      ( aName + "-user-DanglingMisses"    )
   , RealReaders_OS           ( aName + "-OS-RealReaders"         )
   , RealHits_OS              ( aName + "-OS-RealHits"            )
   , RealTraining_OS          ( aName + "-OS-RealTraining"        )
   , RealMisses_OS            ( aName + "-OS-RealMisses"          )
   , DanglingMisses_OS        ( aName + "-OS-DanglingMisses"      )

#ifdef MRP_INSTANCE_COUNTERS
   , HitInstances             ( std::string("allOnly-") + aName + "-HitInstances"      )
   , TrainingInstances        ( std::string("allOnly-") + aName + "-TrainingInstances" )
   , MissInstances            ( std::string("allOnly-") + aName + "-MissInstances"     )
#endif //MRP_INSTANCE_COUNTERS

   , ReadInitial              ( aName + "-ReadInitial"             )
   , ReadAgain                ( aName + "-ReadAgain"               )
   , Predict                  ( aName + "-Predict"                 )
   , SigCreation              ( aName + "-SigCreation"             )
   , SigEviction              ( aName + "-SigEviction"             )
   , TrainingRead_User        ( aName + "-user-TrainingRead"       )
   , TrainingRead_OS          ( aName + "-OS-TrainingRead"         )
   , TrainingWrite            ( aName + "-TrainingWrite"           )
   , EvictedPred              ( aName + "-EvictedPred"             )
   , NoHistory                ( aName + "-NoHistory"               )
#ifdef MRP_INFINITE
    , Organization            ( aName + "-Organization", std::string("infinite"))
#else
    , Organization            ( aName + "-Organization", std::string("finite"))
#endif
   {}

};  // end struct MrpStats

} //end nMrpStats
