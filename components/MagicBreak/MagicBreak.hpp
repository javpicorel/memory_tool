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

#include <core/simulator_layout.hpp>

#define FLEXUS_BEGIN_COMPONENT MagicBreak
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#define MagicBreak_IMPLEMENTATION    (<components/MagicBreak/MagicBreakImpl.hpp>)

COMPONENT_PARAMETERS(
    PARAMETER( EnableIterationCounts, bool, "Enable Iteration Counts", "iter", false )
    PARAMETER( TerminateOnMagicBreak, int, "Terminate simulation on a specific magic breakpoint", "stop_on_magic", -1 )
    PARAMETER( TerminateOnIteration, int, "Terminate simulation when CPU 0 reaches iteration.  -1 disables", "end_iter", -1 )
    PARAMETER( CheckpointOnIteration, bool, "Checkpoint simulation when CPU 0 reaches each iteration.", "ckpt_iter", false )
    PARAMETER( TerminateOnTransaction, int, "Terminate simulation after ## transactions.  -1 disables", "end_trans", -1 )
    PARAMETER( EnableTransactionCounts, bool, "Enable Transaction Counts", "trans", false )
    PARAMETER( TransactionType, int, "Workload type.  0=TPCC/JBB  1=WEB", "trans_type", 0 )
    PARAMETER( TransactionStatsInterval, int, "Statistics interval on ## transactions.  -1 disables", "stats_trans", -1 )
    PARAMETER( CheckpointEveryXTransactions, int, "Quiesce and save every X transactions. -1 disables", "ckpt_trans", -1 )
    PARAMETER( FirstTransactionIs, int, "Transaction number for first transaction.", "first_trans", 0 )
    PARAMETER( CycleMinimum, unsigned long long, "Minimum number of cycles to run when TerminateOnTransaction is enabled.", "min_cycle", 0 )
    PARAMETER( StopCycle, unsigned long long, "Cycle on which to halt simulation.", "stop_cycle", 0 )
    PARAMETER( CkptCycleInterval, unsigned long long, "# of cycles between checkpoints.", "ckpt_cycle", 0 )
    PARAMETER( CkptCycleName, unsigned int, "Base cycle # from which to build checkpoint names.", "ckpt_cycle_name", 0 )
);

COMPONENT_INTERFACE(
  DRIVE( TickDrive )
);

#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT MagicBreak


