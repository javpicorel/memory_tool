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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <core/debug/debug.hpp>

#include "exec_engine.hpp"
#include "protocol_engine.hpp"
#include "tSrvcProvider.hpp"
#include "input_q_cntl.hpp"
#include "util.hpp"
#include "tsrf.hpp"


  #define DBG_DefineCategories ExecEngine
  #define DBG_DeclareCategories ProtocolEngine
  #define DBG_SetDefaultOps AddCat(ProtocolEngine) AddCat(ExecEngine)
  #include DBG_Control()

namespace nProtocolEngine {


tMicrocodeEmulator::tMicrocodeEmulator(std::string const &      engine_str,
                                       std::string const &      mcd_fname,
                                       std::string const &      mdef_fname,
                                       std::string const &      cnt_fname,
                                       const unsigned           mcd_magic_n,
                                       tSrvcProvider &          aSrvc,
                                       tInputQueueController &  anInQCntl,
                                       tThreadScheduler &       aThreadScheduler)

  : theEngineName(engine_str)
  , mcd_filename(mcd_fname)
  , mdef_filename(mdef_fname)
  , cnt_filename(cnt_fname)
  , theSrvcProv(aSrvc)
  , theInputQCntl(anInQCntl)
  , theThreadScheduler(aThreadScheduler)
  , theConfigurationReg(0)
{
  mcd_magic_no = mcd_magic_n;
}

tMicrocodeEmulator::~tMicrocodeEmulator()
{
  DBG_(VVerb, ( << theEngineName << " dumping MCD counter") );
  // always dump the counters
  dump_mcd_counters();
}

void
tMicrocodeEmulator::init(void)
{
  unsigned char  mcd_loaded = 0;   // flag to prevent multiple loading of microcode
  FILE         * mcd_fp;
  char           buf[200];
  char           tmp[60];
  unsigned long  i, addr, op, a1, a2, a3, nx;

  tmp[59] = 0;                     // be prepared to truncate strings

  if (!mcd_loaded) {
    // clear microcode debugging store
    for (i = 0; i < MCD_SIZE; i++)
      mcd[i].memo = 0;

    // open mcd file for read
    if (!(mcd_fp = fopen(mcd_filename.c_str(), "r")))
      ___fatal("rxx_cntl: failed to open microcode file %s", mcd_filename.c_str());
    strcpy(buf, ""); // clean up buf
    if (!fgets(buf, 160, mcd_fp) || 2 != sscanf(buf, "%lu%59c", &i, tmp) || i != mcd_magic_no)
      ___fatal("rxx_cntl: wrong magic number %u, expected %u", i, mcd_magic_no);
    mcd_id_string = strdup(buf);

    // default action: zero counters
    for (i = 0; i < MCD_SIZE; i++)
      mcd[i].cnt  = 0;

    // load microcode
    for (i = 0; i < MCD_SIZE; i++) {
      if (!fgets(buf, 160, mcd_fp))
        break;
      if (   6 != sscanf(buf, "%lu%lu%lu%lu%lu%lu", &addr, &op, &a1, &a2, &a3, &nx)
          || addr >= MCD_SIZE)
        ___fatal("rxx_cntl: illegal microcode");

      // debug stuff
      if (IS_DEBUG_LEVEL(MCD_LOADER_DBG)) {
        char *p1, *p2;
        for (p1 = buf, p2 = tmp; *p1 && *p1 != ';'; p1++)
          ;

        if (*p1++ == ';') {     // got a comment
          while (*p1  && *p1 != '\n')
            *p2++ = *p1++;
          *p2 = 0;
          mcd[addr].memo = strdup(tmp);
        } else {
          mcd[addr].memo = "";
        }
      }
      else
        mcd[addr].memo = "";

      // store microcode
      mcd[addr].op_code = op;
      mcd[addr].args = (a3 << 12) | (a2 << 8) | (a1 << 4);
      mcd[addr].next = nx;
    }

    // done loading
    DBG_(VVerb, ( << theEngineName << " microcode OK: " << i << "/" << MCD_SIZE <<" instructions loaded"));
    fclose(mcd_fp);
    mcd_loaded = 1;
    mcd_actual_size = i;

    // we have a valid counter file
    if (   (mcd_fp = fopen(cnt_filename.c_str(), "r"))
        && fgets(buf, 160, mcd_fp) && !strcmp(buf, mcd_id_string.c_str()))
    {
      for (i = 0; i < MCD_SIZE; i++)
        if (!fgets(buf, 160, mcd_fp) || 1 != sscanf(buf, "%lu", &(mcd[i].cnt)))
          break;
      if (i < MCD_SIZE)
        ___error("rxx_cntl: truncated counter file");
      fclose(mcd_fp);
    }
  }

  // debug stuff
  if (IS_DEBUG_LEVEL(MCD_LOADER_MCD_DUMP_DBG)) {
    for (i = 0; i < mcd_actual_size; i++)
      DEBUG(MCD_LOADER_MCD_DUMP_DBG, NIL_INDEX,
            ("op=%2u   args=%#12x   next=%4u   memo=%s\n",
             mcd[i].op_code, mcd[i].args >> 4, mcd[i].next, mcd[i].memo));
  }


  return;
}


void
tMicrocodeEmulator::dump_mcd_stats(void) const
{
  unsigned i;

  for (i=0; i < mcd_actual_size; i++)
    ___printf("microinstruction %4u times executed %lu\n", i, mcd[i].cnt);

  return;
}

void
tMicrocodeEmulator::dump_mcd_counters(void) const
{
  unsigned i;
  FILE *fp;

  if (!(fp = fopen(cnt_filename.c_str(), "w")))
    ___error("%s Failed to open microcode counter file", theEngineName.c_str());
  fputs (mcd_id_string.c_str(), fp);
  for (i = 0; i < MCD_SIZE; i++)
    fprintf(fp, "%lu\n", mcd[i].cnt);
  fclose(fp);

  return;
}


void tMicrocodeEmulator::runThread(tThread aThread) {
  DBG_Assert(aThread.isRunnable());

  DBG_(VVerb, ( << theEngineName << " run thread " << aThread) );

  unsigned long ms_pc;

  bool done = false;
  while (aThread.isRunnable() && !done) {
    if (aThread.getStallCycles() <= 0) { // hack: includes both 0 and -1 (special case of CPI=0)

      ms_pc = aThread.programCounter();              // this is the current program counter
      DBG_Assert(ms_pc >= 0);
      DBG_Assert(ms_pc < MCD_SIZE);
      DBG_Assert(mcd[ms_pc].next >= 0);
      DBG_Assert(mcd[ms_pc].next < MCD_SIZE);
      aThread.setProgramCounter(mcd[ms_pc].next);

      mcd[ms_pc].cnt ++;                                          // update execution counter

      DBG_(VVerb, ( << theEngineName
                   << " [" << aThread << "]"
                   << " Execute pc=" << ms_pc
                   << " op=" << mcd[ms_pc].op_code
                   << " args=" << &std::hex << (mcd[ms_pc].args >> 4)
                   << " next=" << &std::dec << mcd[ms_pc].next
                   << " memo=" << mcd[ms_pc].memo));

      //
      // Execute instruction - defers execution to derived class
      //
      execute(aThread, mcd[ms_pc].op_code, mcd[ms_pc].args, ms_pc);

      aThread.incrUopCount();
      aThread.resetStallCycles();
    } else {
      aThread.decStallCycles();
    }

    if (aThread.getStallCycles() != -1) done = true;
  }


}


}  // namespace nProtocolEngine
