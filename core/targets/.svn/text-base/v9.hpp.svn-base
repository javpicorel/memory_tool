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

#ifdef FLEXUS_TARGET
#error "Only a single target may be defined in a simulator"
#endif //FLEXUS_TARGET

#define FLEXUS_LITTLE_ENDIAN 0
#define FLEXUS_BIG_ENDIAN 1

#define FLEXUS_TARGET_x86 0
#define FLEXUS_TARGET_v9 1

#define FLEXUS_TARGET FLEXUS_TARGET_v9
#define FLEXUS_TARGET_WORD_BITS 64
#define FLEXUS_TARGET_VA_BITS 64
#define FLEXUS_TARGET_PA_BITS 64
#define FLEXUS_TARGET_ENDIAN FLEXUS_BIG_ENDIAN

#define FLEXUS_TARGET_IS(target) ( FLEXUS_TARGET == FLEXUS_TARGET_ ## target )

#define TARGET_VA_BITS 64
#define TARGET_PA_BITS 64
#define TARGET_SPARC_V9
#define TARGET_ULTRA

#define TARGET_MEM_TRANS v9_memory_transaction_t

#define FLEXUS_SIMICS_TARGET_REGISTER_ID() <core/simics/v9/register_id.hpp>
#define FLEXUS_SIMICS_TARGET_INSTRUCTION_EXTENSTIONS() <core/simics/v9/instructions.hpp>

#include <simics/global.h>
#if SIM_VERSION > 1300 
#define FLEXUS_SIMICS_API_HEADER(HEADER) <simics/core/HEADER.h>
#define FLEXUS_SIMICS_API_ARCH_HEADER <simics/arch/sparc.h>
#else
#define FLEXUS_SIMICS_API_HEADER(HEADER) <simics/HEADER##_api.h>
#define FLEXUS_SIMICS_API_ARCH_HEADER <simics/sparc_api.h>
#endif

