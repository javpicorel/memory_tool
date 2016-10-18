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

#include <memory>

#include <iostream>

#define FLEXUS_BEGIN_COMPONENT TraceFeeder
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

  #define DBG_DefineCategories TraceFeeder, Feeder
  #define DBG_SetDefaultOps AddCat(TraceFeeder | Feeder)
  #include DBG_Control()


namespace nTraceFeeder {

using namespace Flexus::Core;
using namespace Flexus::Debug;
using namespace Flexus::SharedTypes;
using namespace Flexus::MemoryPool;

using boost::intrusive_ptr;
using Flexus::Debug::end;


struct TraceInstructionImpl : public TraceInstruction
                            //, UseMemoryPool<TraceInstructionImpl, GlobalResizing<32> >
{
    typedef PhysicalMemoryAddress MemoryAddress;

    //enumerated op code type
    enum eOpCode { iNOP, iLOAD, iSTORE };

    enum eState { sFetched, sExecuted, sPerformed, sCommitted};

    //Only defined if eOpCode is iLOAD or iSTORE
    MemoryAddress theAddress;
    eOpCode theOpCode;
    eState theState;

    TraceInstructionImpl()
      : TraceInstruction(this)
      , theAddress(0)
      , theState(sFetched)
      {}

    //Query for type of operation
    int isMemory() const { return (theOpCode != iNOP);   }
    int isLoad()   const { return (theOpCode == iLOAD);  }
    int isStore()  const { return (theOpCode == iSTORE); }

    void execute() { theState = sExecuted;  }
    void perform() { theState = sPerformed; }
    void commit()  { theState = sCommitted; }

    bool isExecuted() const { return (theState != sFetched); }
    bool isPerformed() const { return (theState == sPerformed || theState == sCommitted); }
    bool isCommitted() const { return (theState == sCommitted); }
    bool canExecute() const { return (theState == sFetched); }
    bool canPerform() const { return (theState == sExecuted); }

    //Get the address for a memory operation
    MemoryAddress physicalMemoryAddress() const { return theAddress; }
};



inline bool TraceInstruction::isMemory() const { return theImpl->isMemory(); }
inline bool TraceInstruction::isLoad() const { return theImpl->isLoad(); }
inline bool TraceInstruction::isStore() const { return theImpl->isStore(); }
inline bool TraceInstruction::isRmw() const { return false; }
inline bool TraceInstruction::isSync() const { return false; }
inline bool TraceInstruction::isMEMBAR() const { return false; }
inline bool TraceInstruction::isBranch() const { return false; }
inline bool TraceInstruction::isInterrupt() const { return false; }
inline bool TraceInstruction::requiresSync() const { return false; }
inline PhysicalMemoryAddress TraceInstruction::physicalMemoryAddress() const { return theImpl->physicalMemoryAddress(); }
inline void TraceInstruction::execute() { theImpl->execute(); }
inline void TraceInstruction::perform() { theImpl->perform(); }
inline void TraceInstruction::commit() { theImpl->commit(); }
inline void TraceInstruction::squash() {}
inline void TraceInstruction::release() {}
inline bool TraceInstruction::isValid() const { return true; }
inline bool TraceInstruction::isFetched() const { return true; }
inline bool TraceInstruction::isDecoded() const { return true; }
inline bool TraceInstruction::isExecuted() const { return theImpl->isExecuted(); }
inline bool TraceInstruction::isPerformed() const { return theImpl->isPerformed(); }
inline bool TraceInstruction::isCommitted() const { return theImpl->isCommitted(); }
inline bool TraceInstruction::isSquashed() const { return false; }
inline bool TraceInstruction::isExcepted() const { return false; }
inline bool TraceInstruction::isReleased() const { return true; }
inline bool TraceInstruction::wasTaken() const { return false; }
inline bool TraceInstruction::wasNotTaken() const { return false; }
inline bool TraceInstruction::canExecute() const { return theImpl->canExecute(); }
inline bool TraceInstruction::canPerform() const { return theImpl->canPerform(); }
inline bool TraceInstruction::canRelease() const { return true; }


inline std::ostream & operator <<(std::ostream & anOstream, const TraceInstruction & anInsn) {
  TraceInstructionImpl const & insn = static_cast<TraceInstructionImpl const &>(anInsn);
  switch (insn.theOpCode) {
  case TraceInstructionImpl::iNOP:
    anOstream << "NOP";
    break;
  case TraceInstructionImpl::iLOAD:
    anOstream << "Load " << insn.theAddress;
    break;
  case TraceInstructionImpl::iSTORE:
    anOstream << "Store " << insn.theAddress;
    break;
  default:
    //Something goes here
    break;
  }
  return anOstream;
}


typedef unsigned long long native;

typedef struct traceStruct {
  unsigned int seg24pc;       // top 24 bit (out of 52 bit) virtual PC adderss
  unsigned int pc;            // bottom 32 bit virtual PC address
  unsigned int seg24ea;       // top 24 bit (out of 52 bit) virtual address of operand
  unsigned int ea;            // bottom 32 bit virtual address of operand
  unsigned int length;        // size of data operand in byte
  unsigned int image;         // real PPC instruction image
} traceSet;



template <class Cfg>
class TraceFeederComponent : public FlexusComponentBase<TraceFeederComponent, Cfg> {
    FLEXUS_COMPONENT_IMPL(TraceFeederComponent, Cfg);

public:
  TraceFeederComponent( FLEXUS_COMP_CONSTRUCTOR_ARGS )
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , count(0)
  {}

  //Total number of instructions read
  long long count;

  ~TraceFeederComponent() {
    if(pipeIn) {
      pclose(pipeIn);
    }
    //std::cout << "Trace instructions read: " << count << std::endl;
  }

  //Initialization
  void initialize() {
    pipeIn = popen("gzip -d -c outtrace3.gz", "r");
    if(pipeIn == NULL) {
      DBG_(Crit, ( << "Error: could not open trace file!" ) );
      exit(-1);
    }
  }


private:
  FILE *pipeIn;

  unsigned int byteReorder(unsigned int value) {
    unsigned int ret = (value >> 24) & 0x000000ff;
    ret |= (value >> 8) & 0x0000ff00;
    ret |= (value << 8) & 0x00ff0000;
    ret |= (value << 24) & 0xff000000;
    return ret;
  }

  intrusive_ptr<TraceInstructionImpl> next_instruction() {
    // To consturct 52-bit virtual address,
    // strip the high NIBBLE (not a byte) of EA/PC.
    //
    // ex)  52 bit virtual address = (seg24pc) | (pc: 27-0 bit)
    //                               (seg24ea) | (ea:
    //
    //     if (Seg24 = 0x654321, EA = 0xA2345678)
    //       52-bit virtual address = 0x6543212345678
    traceSet rec;
    native pc = 0;
    native ea = 0;
    intrusive_ptr<TraceInstructionImpl> new_instruction = new TraceInstructionImpl();
    //int is_mem = 0;

    // search for a memory address
    while(!ea) {
      if (fread(&rec, sizeof(traceSet), 1, pipeIn) == 1) {
        count++;
        pc = byteReorder(rec.pc) & 0x0fffffff;
        pc |= (unsigned long long)byteReorder(rec.seg24pc) << 28;
        ea = byteReorder(rec.ea) & 0x0fffffff;
        ea |= (unsigned long long)byteReorder(rec.seg24ea) << 28;
      }
      else {
        DBG_(Crit, ( << "Error: could not read trace file!" ) );
        throw Flexus::Core::FlexusException();
      }
    }

    new_instruction->theAddress = (TraceInstructionImpl::MemoryAddress)ea;
    new_instruction->theOpCode = TraceInstructionImpl::iLOAD;
    // do the intelligent decode stuff here!

    return new_instruction;
  }

public:
  //Ports
	struct FeederCommandIn : public PushInputPort< FeederCommand >, AlwaysAvailable {
		FLEXUS_WIRING_TEMPLATE
		static void push(self & aFeeder, FeederCommand aCommand) {
			//Feeder commands are currently ignored by the TraceFeeder.  It always returns the
			//next instruction in the trace.
		}

	};


  struct InstructionOutputPort : public PullOutputPort< InstructionTransport >, AlwaysAvailable {

    FLEXUS_WIRING_TEMPLATE
    static InstructionTransport pull(self & aFeeder) {
      InstructionTransport aTransport;
      aTransport.set(ArchitecturalInstructionTag, aFeeder.next_instruction());
      return aTransport;
    }
  };

  typedef FLEXUS_DRIVE_LIST_EMPTY DriveInterfaces;

};





FLEXUS_COMPONENT_EMPTY_CONFIGURATION_TEMPLATE(TraceFeederComponentConfiguration);

}  //End Namespace nTraceFeeder

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT TraceFeeder

  #define DBG_Reset
  #include DBG_Control()
