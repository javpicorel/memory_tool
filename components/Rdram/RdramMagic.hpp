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

#ifndef _RDRAM_MAGIC_H_
#define _RDRAM_MAGIC_H_


/*
 * some convenience micros
 */
#define RDRAM_NUM_BANKS 16
#define RDRAM_SPILL_INTERVAL 5000000
#define RDRAM_ADDR2BANK(banks, paddr)   \
       (&(banks)[( (paddr)>>7 ) & (RDRAM_NUM_BANKS-1)])
                                  // assuming 128B L2 cache line


/*
 * RDRAM timing micros
 */
#define RDRAM_RC       2  // interval between commands to the same bank
#define RDRAM_PACKET   0  // length of ROWA/ROWR/COLC/COLM/COLX packet
#define RDRAM_RR       2  // RAS-to-RAS time to different banks of same device
#define RDRAM_RP       2  // interval between PRER command and next ROWA command
#define RDRAM_CBUB1    2  // bubble between READ and WRITE command
#define RDRAM_CBUB2    2  // bubble between WRITE and READ command
#define RDRAM_RCD      2  // RAS-to-CAS delay
#define RDRAM_CAC      2  // CAS-to-Q delay
#define RDRAM_CWD      2  // CAS write delay

#define RDRAM_ACTIVE_ACCESS           1
#define RDRAM_STANDBY_TO_ACTIVE       6
#define RDRAM_NAP_TO_ACTIVE          20
#define RDRAM_POWERDOWN_TO_ACTIVE   600

#define RDRAM_THRESHOLD  500  // number of cycles has to elapse before
                               // a bank power state transition

/*
 * RDRAM BANK power
 */
#define RDRAM_ACTIVE_POWER     300
#define RDRAM_STANDBY_POWER    180
#define RDRAM_NAP_POWER        30
#define RDRAM_POWERDOWN_POWER  3

#define RDRAM_STANDBY_TO_ACTIVE_POWER    240
#define RDRAM_NAP_TO_ACTIVE_POWER        165
#define RDRAM_POWERDOWN_TO_ACTIVE_POWER  152


/*
 * RDRAM control commands in micros
 */
#define RDRAM_READ  0
#define RDRAM_WRITE 1


/*
 * RDRAM bank states
 */
enum rdram_bank_state_t {
  ACTIVE,
  STANDBY,
  NAP,
  POWERDOWN
};


typedef unsigned RdramPhysicalMemoryAddress;
typedef long long RdramCycles;
typedef char RdramCommand;


/*
 * RDRAM bank
 */
typedef struct rdram_bank {
  int id;        // bank ID
  int lastType;  // last transanction type (READ/WRITE)
  enum rdram_bank_state_t state;  // current state

  long long accesses;
  long long reads;
  long long writes;

  RdramCycles lastAccessed;
  RdramCycles ready;

  RdramCycles s2aCycles;
  RdramCycles n2aCycles;
  RdramCycles p2aCycles;

  RdramCycles activeCycles;  // cycles in ACTIVE state
  RdramCycles standbyCycles;  // cycles in STANDBY state
  RdramCycles napCycles;  // cycles in NAP state
  RdramCycles powerdownCycles;  // cycles in POWERDOWN state
} rdram_bank_t;


#endif  // _RDRAM_MAGIC_H_

