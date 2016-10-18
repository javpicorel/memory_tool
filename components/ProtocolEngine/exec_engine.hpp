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

#ifndef _EXEC_ENGINE_H_
#define _EXEC_ENGINE_H_

#include "tSrvcProvider.hpp"
#include "ProtSharedTypes.hpp"
#include "util.hpp"

#include "thread.hpp"


namespace nProtocolEngine {

template <class ExecEngine>
class tProtocolEngine;
class tInputQueueController;
class tThreadScheduler;


class tMicrocodeEmulator {

  ///////////////////////////////////////////////////////////
  //
  // Some private variables
  //
 protected:
  std::string       theEngineName;            // for debug output

 private:

  std::string       mcd_filename;           // microcode rom filename
  std::string       mdef_filename;          // microcode definitions filename
  std::string       cnt_filename;           // counters filename
  unsigned          mcd_magic_no;           // microcode expected magic number

  std::string       mcd_id_string;          // Microcode ID string
  unsigned          mcd_actual_size;        // actual size of microcode in #instructions


  ///////////////////////////////////////////////////////////
  //
  // Microcode store
  //
 private:

  static const unsigned MCD_SIZE = 2048;     // Max size of microcode in #instructions

  struct m_code {
    unsigned      op_code;   // operation code (4 bits)
    unsigned      args;      // arguments (12 bits)

    unsigned      next;      // address of next micro code instruction

    unsigned long cnt;       // # of times this instruction was executed

    char        * memo;      // for debugging purposes only
  } mcd[MCD_SIZE];

  ///////////////////////////////////////////////////////////
  //
  // protected variables and methods
  //
 protected:

  tSrvcProvider &         theSrvcProv;       // pointer to service provider
  tInputQueueController & theInputQCntl;     // input queue
  tThreadScheduler &      theThreadScheduler;// thread scheduler

  unsigned                theConfigurationReg;   // configuration of system


  ///////////////////////////////////////////////////////////
  //
  // methods to calculate the microcode pc
  // NOTE: some are engine specific
  //
 protected:

 public:
  // deliver response packet to RECEIVE instruction
  virtual void deliverReply( tThread thread, tMessageType type) = 0;

  // calculate the pc for the entry point of remote requests
  // this is engine specific

  virtual unsigned getEntryPoint(tMessageType aType, tThread aThread, tDirState aState = DIR_STATE_INVALID) = 0;


  ///////////////////////////////////////////////////////////
  //
  // Standard routines
  //
 public:

  // constructor
  tMicrocodeEmulator(std::string const &     engine_str,
                     std::string const &     mcd_fname,
                     std::string const &     mdef_fname,
                     std::string const &     cnt_fname,
                     const unsigned          mcd_magic_n,
                     tSrvcProvider &         aSrvcProv,
                     tInputQueueController & anInQCntl,
                     tThreadScheduler &      aThreadScheduler);

  // destructor
  virtual ~tMicrocodeEmulator();

  // initialization
  virtual void init(void);    // initializer

  // stats
  virtual void dump_mcd_stats(void) const;       // printout stats
  virtual void dump_mcd_counters(void) const;    // dump counters to xe.cnt file

  void runThread(tThread aThread);


  ///////////////////////////////////////////////////////////
  //
  // main execution loop routines
  //
 protected:
  // Execute state: work on it
  // actual execution of instructions (engine dependent)
  virtual void execute(tThread active,
                       const unsigned      op_code,
                       const unsigned long args,
                       const unsigned long ms_pc)    = 0;

  unsigned configurationReg() const { return theConfigurationReg; }
  void setConfigurationReg(unsigned aVal) { theConfigurationReg = aVal; }

};


}  // namespace nProtocolEngine

#endif // _EXEC_ENGINE_H_
