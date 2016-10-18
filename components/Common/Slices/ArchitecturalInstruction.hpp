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


#ifndef FLEXUS_SLICES__ARCHITECTURAL_INSTRUCTION_HPP_INCLUDED
#define FLEXUS_SLICES__ARCHITECTURAL_INSTRUCTION_HPP_INCLUDED

#ifdef FLEXUS_ArchitecturalInstruction_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::ArchitecturalInstruction data type"
#endif
#define FLEXUS_ArchitecturalInstruction_TYPE_PROVIDED

#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/types.hpp>
#include <components/Common/DoubleWord.hpp>


namespace Flexus {
namespace SharedTypes {

  //Forward declare
  class SimicsTraceConsumer;
  class StoreBuffer;
	
	enum execType{
		Normal,
		Second_MemPart,
		Hardware_Walk,
		Halt_instr
	};

  //InorderInstructionImpl - Implementation class for SPARC v9 memory operations
  //==================================
  class ArchitecturalInstruction : public boost::counted_base /*, public FastAlloc*/ {
    enum eOpType {
      Nop,
      Read,
      Write,
      Rmw,
      Membar
    };

    // A memory operation needs to know its address, data, and operation type
    PhysicalMemoryAddress thePhysicalAddress;
    VirtualMemoryAddress theVirtualAddress;
    PhysicalMemoryAddress thePhysicalInstructionAddress;    // i.e. the PC
    VirtualMemoryAddress theVirtualInstructionAddress;      // i.e. the PC
    DoubleWord theData;
    eOpType theOperation;
    SimicsTraceConsumer * theConsumer;
    bool theReleased;
    bool thePerformed;
    bool theCommitted;
    bool theSync;
    bool thePriv;
    bool theShadow;
    bool theTrace;  // a traced instruction
    bool theIsAtomic;
    unsigned long long theStartTime;
    StoreBuffer * theStoreBuffer;
    unsigned long theOpcode;
    char theSize;
    char instructionSize;//the size of the instruction...for x86 purposes
    execType hasIfetchPart;//for instructions that perform 2 memory operations, or hardware walks
    char theIO;

  private:
      ArchitecturalInstruction(ArchitecturalInstruction * anOriginal)
        : thePhysicalAddress(anOriginal->thePhysicalAddress)
        , theVirtualAddress(anOriginal->theVirtualAddress)
        , thePhysicalInstructionAddress(anOriginal->thePhysicalInstructionAddress)
        , theVirtualInstructionAddress(anOriginal->theVirtualInstructionAddress)
        , theData(anOriginal->theData)
        , theOperation(anOriginal->theOperation)
        , theConsumer(0)
        , theReleased(false)
        , thePerformed(false)
        , theCommitted(false)
        , theSync(anOriginal->theSync)
        , thePriv(anOriginal->thePriv)
        , theShadow(true)
        , theTrace(anOriginal->theTrace)
        , theStartTime(0)
        , theStoreBuffer(anOriginal->theStoreBuffer)
        , theOpcode(anOriginal->theOpcode)
        , theSize(anOriginal->theSize)
        , instructionSize(anOriginal->instructionSize)
        , hasIfetchPart(Normal)
        , theIO(anOriginal->theIO)
        {}

  public:
    ArchitecturalInstruction()
      : thePhysicalAddress(0)
      , theVirtualAddress(0)
      , thePhysicalInstructionAddress(0)
      , theVirtualInstructionAddress(0)
      , theData(0)
      , theOperation(Nop)
      , theConsumer(0)
      , theReleased(false)
      , thePerformed(false)
      , theCommitted(false)
      , theSync(false)
      , thePriv(false)
      , theShadow(false)
      , theTrace(false)
      , theStartTime(0)
      , theStoreBuffer(0)
      , theOpcode(0)
      , theSize(0)
      , instructionSize(0)
      , hasIfetchPart(Normal)
      , theIO(false)
    {}

      explicit ArchitecturalInstruction(SimicsTraceConsumer * aConsumer)
        : thePhysicalAddress(0)
        , theVirtualAddress(0)
        , thePhysicalInstructionAddress(0)
        , theVirtualInstructionAddress(0)
        , theData(0)
        , theOperation(Nop)
        , theConsumer(aConsumer)
        , theReleased(false)
        , thePerformed(false)
        , theCommitted(false)
        , theSync(false)
        , thePriv(false)
        , theShadow(false)
        , theTrace(false)
        , theIsAtomic(false)
        , theStartTime(0)
        , theStoreBuffer(0)
        , theOpcode(0)
        , theSize(0)
        , instructionSize(0)
        , hasIfetchPart(Normal)
        , theIO(false)
        {}

    boost::intrusive_ptr<ArchitecturalInstruction> createShadow() {
        return boost::intrusive_ptr<ArchitecturalInstruction> (new ArchitecturalInstruction(this)  );
    }

  //ArchitecturalInstruction Interface functions
    //Query for type of operation
    bool isNOP() const {
      return (theOperation == Nop)  ;
    }
    bool isMemory() const {
      return (theOperation == Read) || (theOperation == Write) || (theOperation == Rmw) || (theOperation == Membar) ;
    }
    bool isLoad() const {
      return (theOperation == Read);
    }
    bool isStore() const {
      return (theOperation == Write);
    }
    bool isRmw() const {
      return (theOperation == Rmw);
    }
    bool isMEMBAR() const {
      return (theOperation == Membar);
    }
    bool isSync() const {
      return theSync;
    }
    bool isPriv() const {
      return thePriv;
    }

    bool isShadow() const {
      return theShadow;
    }

    bool isTrace() const {
      return theTrace;
    }

    unsigned long long getStartTime() const {
      return theStartTime;
    }

    bool isCommitted() const {
      return theCommitted;
    }
    bool isIO() const {
      return theIO;
    }
    void setIO() {
      theIO = true;
    }

    void commit() {
      DBG_Assert( (theCommitted == false) );
      theCommitted = true;;
    }

    bool isReleased() const {
      return theReleased;
    }

    bool canRelease() const {
      //TSO
      if (isSync()) {
        return isPerformed() && isCommitted() && !isReleased();
      } else {
        return isCommitted() && !isReleased();
      }
    }

    void release();

    bool isPerformed() const {
      return thePerformed;
    }

    DoubleWord const & data() const {
      return theData;
    }

    unsigned int size() const {
      return theSize;
    }

	unsigned int InstructionSize() const {
	  return instructionSize;
	}

	unsigned long opcode() const {
      return theOpcode;
    }

    //Perform the instruction
    void perform();

  //InorderInstructionImpl Interface functions
    void setSync() {
      theSync = true;
    }
    void setPriv() {
      thePriv = true;
    }

    //Set operation types
    void setIsNop() {
      theOperation = Nop;
    }
    void setIsLoad() {
      theOperation = Read;
    }
    void setIsMEMBAR() {
      theOperation = Membar;
      setSync();
    }
    void setIsStore() {
      theOperation = Write;
    }
    void setIsRmw() {
      theOperation = Rmw;
      setSync();
    }


    void setShadow() {
      theShadow = true;
    }

    void setTrace() {
      theTrace = true;
    }

    void setStartTime(unsigned long long start) {
      theStartTime = start;
    }

    void setSize(char aSize) {
      theSize = aSize;
    }

    void setInstructionSize(char const aSize) {
      instructionSize = aSize;
    }

    //Set the address for a memory operation
    void setAddress(PhysicalMemoryAddress const & addr) {
      thePhysicalAddress = addr;
    }

    //Set the virtual address for a memory operation
    void setVirtualAddress(VirtualMemoryAddress const & addr) {
      theVirtualAddress = addr;
    }

    void setIfPart(execType const hasifpart){
      hasIfetchPart = hasifpart;
    }

    //Set the PC for the operation
    void setPhysInstAddress(PhysicalMemoryAddress const & addr) {
      thePhysicalInstructionAddress = addr;
    }
    void setVirtInstAddress(VirtualMemoryAddress const & addr) {
      theVirtualInstructionAddress = addr;
    }

    void setStoreBuffer(StoreBuffer * aStoreBuffer) {
      theStoreBuffer = aStoreBuffer;
    }

    void setData(DoubleWord const & aData) {
      theData = aData;
    }

    void setOpcode(unsigned long anOpcode) {
      theOpcode = anOpcode;
    }

    //Get the address for the instruction reference
    PhysicalMemoryAddress physicalInstructionAddress() const {
      return thePhysicalInstructionAddress;
    }

    VirtualMemoryAddress virtualInstructionAddress() const {
      return theVirtualInstructionAddress;
    }

    //Get the address for a memory operation
    PhysicalMemoryAddress physicalMemoryAddress() const {
      return thePhysicalAddress;
    }
	
    VirtualMemoryAddress virtualMemoryAddress() const {
      return theVirtualAddress;
    }

    execType getIfPart() const {
      return hasIfetchPart;
    }

    const char * opName() const {
      const char * opTypeStr[] = {"nop", "read", "write", "rmw", "membar"};
      return opTypeStr[theOperation];
    }

  //Forwarding functions from SimicsV9MemoryOp
  bool isBranch() const { return false; }
  bool isInterrupt() const { return false; }
  bool requiresSync() const { return false; }
  void execute() { if(!isMemory()) perform(); }
  void squash() { DBG_Assert(false, (<< "squash not supported") ); }
  bool isValid() const { return true; }
  bool isFetched() const { return true; }
  bool isDecoded() const { return true; }
  bool isExecuted() const { return true; }
  bool isSquashed() const { return false; } //Squashing not supported
  bool isExcepted() const { return false; } //Exceptions not supported
  bool wasTaken() const { return false; }   //Branches not supported
  bool wasNotTaken() const { return false; }//Branches not supported
  bool canExecute() const { return true; }  //execute() always true
  bool canPerform() const { return true; }  //execute() always true

  }; //End ArchitecturalInstruction


  std::ostream & operator <<(std::ostream & anOstream, const ArchitecturalInstruction & aMemOp);
  bool operator == (const ArchitecturalInstruction & a, const ArchitecturalInstruction & b);


} //End SharedTypes
} //End Flexus

#endif //FLEXUS_SLICES__ARCHITECTURAL_INSTRUCTION_HPP_INCLUDED
