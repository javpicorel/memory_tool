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
#ifndef _MEMORY_TRACE_
#define _MEMORY_TRACE_

#include <fstream>
#include <string>
#include <sstream>
#include <deque>

#include <core/target.hpp>
#include <core/debug/debug.hpp>

#define MAX_LINE 256

namespace nMemoryTrace {

typedef enum {NOP, LD, ST, TSTART, TSTOP, MSG} CommandType;
typedef unsigned long long AddressType;
typedef unsigned long long SequenceType;

class MemoryOp
{
public:
  MemoryOp() {}
  MemoryOp(SequenceType s, CommandType t, AddressType a)
    : seq(s), type(t), addr(a), msg("") {}
  MemoryOp(SequenceType s, CommandType t)
    : seq(s), type(t), addr(0), msg("") {}
  MemoryOp(CommandType t)
    : seq(0), type(t), addr(0), msg("") {} // for NOP
  MemoryOp(SequenceType s, std::string msg_)
    : seq(s), type(MSG), addr(0), msg(msg_) {}

  SequenceType seq;
  CommandType type;
  AddressType addr;
  std::string msg;
};

class MemoryTrace
{
  int theNumCPUs;
public:

  MemoryTrace(int aNumCPUs) : theNumCPUs(aNumCPUs) {
    optable = new std::deque<MemoryOp> [theNumCPUs];
    seqCounters = new SequenceType [theNumCPUs];
  }
  MemoryTrace(int aNumCPUs, const char *fname) : theNumCPUs(aNumCPUs)
  {
    optable = new std::deque<MemoryOp> [theNumCPUs];
    seqCounters = new SequenceType [theNumCPUs];
    init(fname);
  }


  void init(const char *fname)
  {
    // read and parse file 'fname'

    std::ifstream fs(fname);
    SequenceType  currTime = 0;

    if (!fs) {
      std::cout << "Couldn't open file " << fname << " for reading." << std::endl;
      assert(0);
      return;
    }

    std::cout << "Opened " << fname << " for reading." << std::endl;

    char line[MAX_LINE+1];

    while (1) {
      fs.getline(line, MAX_LINE);
      if (fs.eof()) break;

      std::string str(line);
      size_t loc = str.find('#', 0);
      if (loc != std::string::npos) {
	// remove comment
	std::string temp = str.substr(0, loc);
	str = temp;
      }

      if (!str.length()) continue;

      //std::cout << "-> " << str << std::endl;

      // dirt simple tokenizer (for now)
      // we expect four tokens:
      // sequence cpu command address
      SequenceType sequence;
      AddressType address;
      int cpu;
      std::string cmd_str, marker;
      CommandType cmd = NOP;

      std::istringstream ss(str);
      if (ss.peek() == '-') {
	// expecting a transaction marker
	ss >> marker;
	if (marker == "-trans-start") {
	  cmd = TSTART;
	} else if (marker == "-trans-stop") {
	  cmd = TSTOP;
	} else {
	  assert(0);
	}

	// put markers in CPU 0 (it doesn't really matter)
	optable[0].push_back(MemoryOp(sequence, cmd));

      } else {
	ss >> sequence;
        if ( sequence < currTime ) {
          DBG_ ( Crit, (<< "MemoryTrace WARNING: sequence numbers not ordered, expect the unexpected!\n"
                        << "      " << sequence << " < " << currTime ) );
        }
        currTime = sequence;

        while ( isspace( ss.peek() ) )
          ss.get();

        if ( ss.peek() == '\"' ) {
          // comment to print to the screen, pulling off the quotes (or last
          // char if somebody forgot to put in the closing quotes)
          unsigned int start = ss.str().find('\"') + 1;
          unsigned int end   = ss.str().rfind('\"');

          end = end - start;
          if ( end < 0 )  end = 0;

          std::string s = ss.str().substr ( start, end );
          optable[0].push_back ( MemoryOp ( sequence, s ) );

        } else {
          // normal memory command
          ss >> cpu;
          ss >> cmd_str;
          ss.get(); ss.get(); ss.get();  // throw away the ' 0x'
          ss >> &std::hex >> address;

          // map cmd_str into enumeration
          if (cmd_str == "LD") {
            cmd = LD;
          } else if (cmd_str == "ST") {
            cmd = ST;
          } else {
            assert(0);
          }
          //std::cout << "-> " << sequence << " " << cpu << " " << cmd << " " << address << std::endl;

          // add op to table
          assert(cpu < theNumCPUs);
          optable[cpu].push_back(MemoryOp(sequence, cmd, address));
        }
      }
    }

    // initialize done vector ('1' for each CPU, initially)
    doneVec = 0;
    for (int i = 0; i < theNumCPUs; i++) {
      doneVec |= (1<<i);
    }

    // initialize sequence counters
    for (int i = 0; i < theNumCPUs; i++) {
      seqCounters[i] = 0;
    }

  }

  MemoryOp getMemoryOp(int cpu, SequenceType seq)
  {
    // if there's an op at seq, return it
    // otherwise, return NOP
    // if no more ops remain, return NOP
    assert(cpu < theNumCPUs);
    if (optable[cpu].size()) {
      if (seq == optable[cpu].front().seq) {
	MemoryOp op = optable[cpu].front();
	optable[cpu].pop_front();
	return op;
      } else {
	return MemoryOp(NOP);
      }
    } else {
      doneVec &= ~(1<<cpu);
      return MemoryOp(NOP);
    }
  }

  MemoryOp getNextMemoryOp(int cpu)
  {
    assert(cpu < theNumCPUs);
    SequenceType seq = Flexus::Core::theFlexus->cycleCount();
    MemoryOp op(getMemoryOp(cpu, seq));
    ++seqCounters[cpu];
    if ( op.type == MSG ) {
      DBG_ ( Crit, ( << " TRACE MESSAGE: " << op.msg ) );
      return getNextMemoryOp ( cpu );
    }
    return op;
  }


  bool isComplete()
  {
    return !doneVec;
  }

private:
  int doneVec;
  std::deque<MemoryOp> * optable;
  SequenceType * seqCounters;
};

#undef MAX_LINE

} // namespace nMemoryTrace

#endif
