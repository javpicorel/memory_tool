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

#ifndef FLEXUS_v9DECODER_v9INSTRUCTION_HPP_INCLUDED
#define FLEXUS_v9DECODER_v9INSTRUCTION_HPP_INCLUDED

#include <sstream>
#include <list>

#include <components/uFetch/uFetchTypes.hpp>
#include <core/simics/sparcmmu.hpp>


namespace nv9Decoder {

using Flexus::SharedTypes::Opcode;
using Flexus::SharedTypes::VirtualMemoryAddress;

using namespace nuArch;



class v9Instruction : public nuArch::Instruction {
  protected:
    VirtualMemoryAddress thePC;
    VirtualMemoryAddress theNPC;
    boost::optional<VirtualMemoryAddress> theNPCReg;
    Opcode theOpcode;
    boost::intrusive_ptr<BPredState> theBPState;
    unsigned int theCPU;
    long long theSequenceNo;
    uArch * theuArch;
    int theRaisedException;
    bool theResync;
    int theWillRaise;
    bool theAnnulled;
    bool theRetired;
    bool theSquashed;
    bool theExecuted;
    eInstructionClass theInstructionClass;
    eInstructionCode theInstructionCode;
    eInstructionCode theOriginalInstructionCode;
    boost::intrusive_ptr<TransactionTracker> theTransaction;
    boost::intrusive_ptr<TransactionTracker> thePrefetchTransaction;
    boost::intrusive_ptr< nuArch::Instruction >  thePredecessor;
    int theASI;
    bool theHaltDispatch;
    bool theHasCheckpoint;
    unsigned long long theRetireStallCycles;
    bool theMayCommit;

  boost::optional<Flexus::Simics::MMU::mmu_t> theMMU;
    
	bool theUsesIntAlu;
	bool theUsesIntMult;
	bool theUsesIntDiv;
	bool theUsesFpAdd;
	bool theUsesFpCmp;
	bool theUsesFpCvt;
	bool theUsesFpMult;
	bool theUsesFpDiv;
	bool theUsesFpSqrt;

  public:

	virtual bool usesIntAlu() const;
	virtual bool usesIntMult() const;
	virtual bool usesIntDiv() const;
	virtual bool usesFpAdd() const;
	virtual bool usesFpCmp() const;
	virtual bool usesFpCvt() const;
	virtual bool usesFpMult() const;
	virtual bool usesFpDiv() const;
	virtual bool usesFpSqrt() const;

	virtual void setCanRetireCounter(const unsigned int numCycles) {}
  	virtual void decrementCanRetireCounter() {}
  	
    virtual void connectuArch(uArch & auArch) {
      theuArch = &auArch;
    };

    virtual void doDispatchEffects();
    virtual void squash() {}
    virtual void doRescheduleEffects() {}
    virtual void doRetirementEffects() {}
    virtual void checkTraps() {}
    virtual void doCommitEffects() {}
    virtual void annul() { theAnnulled = true; }
    virtual void reinstate() { theAnnulled = false; }

    virtual bool preValidate() { return true; }
    virtual bool advancesSimics() const { return true; }
    virtual bool postValidate() { return true; }


    virtual bool mayRetire() const { return false; }
    virtual void resolveSpeculation() { theMayCommit = true; }
    virtual void setMayCommit(bool aMayCommit) { theMayCommit = false; }
    virtual bool mayCommit() const { return theMayCommit; }


    virtual int willRaise() const { return theWillRaise ; }
    virtual void setWillRaise(int aSetting);
    virtual int raised() { return theRaisedException; }
    virtual void raise(int anException) { theRaisedException = anException; }
    virtual bool resync() const { return theResync; }
    void forceResync() { theResync = true; }

    virtual void setTransactionTracker(boost::intrusive_ptr<TransactionTracker> aTransaction) { theTransaction = aTransaction; }
    virtual boost::intrusive_ptr<TransactionTracker> getTransactionTracker() const { return theTransaction; }
    virtual void setPrefetchTracker(boost::intrusive_ptr<TransactionTracker> aTransaction) { thePrefetchTransaction = aTransaction; }
    virtual boost::intrusive_ptr<TransactionTracker> getPrefetchTracker() const { return thePrefetchTransaction; }

    virtual bool willOverrideSimics() const { return false; }


    virtual void describe(std::ostream & anOstream) const;
    virtual std::string disassemble() const;
    virtual void overrideSimics() {}
    virtual long long sequenceNo() const { return theSequenceNo; }
    virtual bool isAnnulled() const { return theAnnulled; }
    bool isSquashed() const { return theSquashed; }
    bool isRetired() const { return theRetired; }
    virtual bool isComplete() const { return isRetired() || isSquashed(); }

		virtual void setMMU(Flexus::Simics::MMU::mmu_t m) { theMMU = m; }
		virtual boost::optional<Flexus::Simics::MMU::mmu_t> getMMU() const { return theMMU; }

    virtual eInstructionClass instClass() const {
      return theInstructionClass;
    }

    virtual eInstructionCode instCode() const {
      return theInstructionCode;
    }
    virtual eInstructionCode originalInstCode() const {
     return theOriginalInstructionCode;
    }

    virtual void restoreOriginalInstCode() {
      DBG_ ( Trace, ( << "Restoring instruction code from " << theInstructionCode
                      << " to " << theOriginalInstructionCode << ": " << *this ) );
      theInstructionCode = theOriginalInstructionCode;
    }

    virtual void changeInstCode(eInstructionCode aCode) { theInstructionCode = aCode ; }


    void setClass( eInstructionClass anInstructionClass, eInstructionCode aCode) {
      theInstructionClass = anInstructionClass;
      theOriginalInstructionCode = theInstructionCode = aCode;
    }

    Opcode opcode() const ;
    void setASI( int anASI) { theASI = anASI; }
    int getASI() const { return theASI; }

    unsigned int cpu() { return theCPU; }
    virtual bool isMicroOp() const { return false; }

    std::string identify() const {
      std::stringstream id;
      id << "CPU[" << std::setfill('0') << std::setw(2) << theCPU << "]#" << theSequenceNo;
      return id.str();
    }
    virtual ~v9Instruction() {
      DBG_( VVerb, ( << identify() << " destroyed") );
    }

    virtual bool redirectNPC(VirtualMemoryAddress anNPC, boost::optional<VirtualMemoryAddress> anNPCReg = boost::none) {
      bool ret_val = (anNPC != theNPC);
      theNPC = anNPC;
      theNPCReg = anNPCReg;
      return ret_val;
    }

    virtual VirtualMemoryAddress pc() const { return thePC; }
    virtual VirtualMemoryAddress npc() const { return theNPC; }
    virtual VirtualMemoryAddress npcReg() const { if (theNPCReg) return *theNPCReg; else return theNPC; }
    virtual bool isPriv() const { return ((theuArch->getPSTATE() & 0x4 /* PSTATE.PRIV */) != 0); }
    virtual boost::intrusive_ptr<BPredState> bpState() const { return theBPState; }
    bool isBranch() const { return theInstructionClass == clsBranch; }
    virtual void setAccessAddress(PhysicalMemoryAddress anAddress) { };
    virtual PhysicalMemoryAddress getAccessAddress() const { return PhysicalMemoryAddress(0) ; }

    virtual void setPreceedingInstruction(boost::intrusive_ptr< Instruction > aPredecessor) {
      thePredecessor = aPredecessor;
    }
    virtual bool hasExecuted() const {
      return theExecuted;
    }
    void setExecuted(bool aVal) {
      theExecuted = aVal;
    }
    bool hasPredecessorExecuted() {
      if (thePredecessor) {
        return thePredecessor->hasExecuted();
      } else {
        return true;
      }
    }

    uArch * core() { return theuArch; }

    bool haltDispatch() const { return theHaltDispatch; }
    void setHaltDispatch() { theHaltDispatch = true; }

    bool hasCheckpoint() const { return theHasCheckpoint; }
    void setHasCheckpoint(bool aCkpt) { theHasCheckpoint = aCkpt; }

    void setRetireStallCycles(unsigned long long aDelay) { theRetireStallCycles = aDelay; }
    unsigned long long retireStallCycles() const { return theRetireStallCycles; }

  protected:
    v9Instruction(VirtualMemoryAddress aPC, VirtualMemoryAddress aNextPC, Opcode anOpcode, boost::intrusive_ptr<BPredState> bp_state, unsigned int aCPU, long long aSequenceNo)
      : thePC(aPC)
      , theNPC(aNextPC)
      , theOpcode(anOpcode)
      , theBPState(bp_state)
      , theCPU(aCPU)
      , theSequenceNo(aSequenceNo)
      , theuArch(0)
      , theRaisedException(0)
      , theResync(false)
      , theWillRaise(0)
      , theAnnulled(false)
      , theRetired(false)
      , theSquashed(false)
      , theExecuted(true)
      , theInstructionClass(clsSynchronizing)
      , theASI(0x80)
      , theHaltDispatch(false)
      , theHasCheckpoint(false)
      , theRetireStallCycles(0)
      , theMayCommit(true)
    {
    	// FIXME: merge with the already-implemented v9Decoder code so we don't have to decode twice
    	// Various possible fields used to identify instructions
		// Warning, some have the same names in the manual but represent DIFFERENT
		// bits.  eg see "rcond" below
		unsigned long op = (theOpcode >> 30) & 0x3;      // [31:30]
		unsigned long fcn = (theOpcode >> 25) & 0x1f;    // [29:25], used for DONE and RETRY
		unsigned long op2 = (theOpcode >> 22) & 0x7;     // [24:22]
		unsigned long op3 = (theOpcode >> 19) & 0x3f;    // [24:19]
		unsigned long iField = (theOpcode >> 13) & 0x1;  // [13], this one is called just 'i' in the manual
		unsigned long opf = (theOpcode >> 5) & 0x1ff;    // [13:5], for fp 
		unsigned long a36 = (theOpcode >> 10) & 7;		 // This field is called "rcond" for A.36, however "rcond" is used
			    										 // for different bits for branch instructions
	  
	  	theUsesIntAlu = false;
	  	theUsesIntMult = false;
	  	theUsesIntDiv = false;
	  	theUsesFpAdd = false;
	  	theUsesFpCmp = false;
	  	theUsesFpCvt = false;
	  	theUsesFpMult = false;
	  	theUsesFpDiv = false;
	  	theUsesFpSqrt = false;
	  	
		// All memory accesses require address computation
	  	if ((op == 0x3) || 
	  	// A.3 - Branch on Integer Register with Prediction
		((op == 0) && (op2 == 0x3) && (((theOpcode >> 28) & 1) == 0)) ||
	  	// A.48 - SETHI, but not NOP (which is a special case of SETHI)
		((op == 0) && (op2 == 0x4) && (fcn != 0) && ((theOpcode && 0x1fffff) == 0)) ||
		// A.2 - Add
		((op == 2) && ((op3 == 0) || (op3 == 0x10) || (op3 == 0x8) || (op3 == 0x18)) ) || 
		// A.24 - Jump and Link
		((op == 0x2) && (op3 == 0x38)) || 
		// A.31 - Logical Operations
		((op == 0x2) && ((op3 == 0x1) || (op3 == 0x11)  || (op3 == 0x5)  || (op3 == 0x15) || (op3 == 0x2)  || (op3 == 0x12)  || (op3 == 0x6)  || (op3 == 0x16) || (op3 == 0x3)  || (op3 == 0x13)  || (op3 == 0x7)  || (op3 == 0x17))) ||
		// A.34 - Move F-P Register on Integer Register Condtion (FMOVr)
		((op == 0x2) && (op3 == 0x35) && (iField == 0) && ((((theOpcode >> 5) & 0x1f) == 5) || (((theOpcode >> 5) & 0x1f) == 6) || (((theOpcode >> 5) & 0x1f) == 7))) ||
		// A.36 - Move Integer Register on Register Condition
		((op == 0x2) && (op3 == 0x2f) && (a36 != 0) && (a36 != 4)) ||
		// A.39 - Multiply Step
		((op == 0x2) && (op3 == 0x24)) ||
		// A.41 - Population Count
		((op == 0x2) && (op3 == 0x2e) && (((theOpcode >> 14) & 0x1f) == 0) )  ||
		// A.45 - Return
		((op == 0x2) && (op3 == 0x39)) ||
		// A.46 - Save and Restore
		((op == 0x2) && (op3 == 0x3c)) ||
		((op == 0x2) && (op3 == 0x3d)) ||
		// A.49 - Shift
		((op == 0x2) && ((op3 == 0x25) || (op3 == 0x26) || (op3 == 0x27)) ) ||
		// A.56 - Subtract
		((op == 0x2) && ( (op3 == 0x4) || (op3 == 0x14) || (op3 == 0xc) || (op3 == 0x1c)) ) ||
		// A.59 - Tagged Add
		((op == 0x2) && ( (op3 == 0x20) || (op3 == 0x22)) ) || 
		// A.60 - Tagged Subtract
		((op == 0x2) && ((op3 == 0x21) || (op3 == 0x23)) )||
		// A.61 - Trap on Integer Condition Codes
		((op == 0x2) && (op3 == 0x3a)) ||
		// A.62 - Write Privileged Register
		((op == 0x2) && (op3 == 0x32)) ||
		// A.63 - Write State Register
		((op == 0x2) && (op3 == 0x30)) ) {
			theUsesIntAlu = true;
		}
		// A.10 Divide, A.37 - Multiply and Divide (64) (1 half)
		else if ((op == 0x2) && ((op3 == 0xe) || (op3 == 0xf) || (op3 == 0x1e) || (op3 == 0x1f)	|| (op3 == 0x2d) || (op3 == 0xd))) {
			theUsesIntDiv = true;
		}
		// A.37 - Multiply and Divide (64) (1 half), A.38 - Multiply (32)
		else if ((op == 0x2) && ((op3 == 0x9)  || (op3 == 0xa) || (op3 == 0xb) || (op3 == 0x1a) || (op3 == 0x1b))) {
			theUsesIntMult = true;
		}
		// A.12 - Floating-Point Add and Subtract
		else if ((op == 0x2) && (op3 == 0x34) && ((opf == 0x41) || (opf == 0x42) || (opf == 0x43) || (opf == 0x45) || (opf == 0x46) || (opf == 0x47))) {
			theUsesFpAdd = true;
		}
		// A.14 - Convert Floating-Point to Integer, A.15 - Convert Between Floating-Point Formats, A.16 - Convert Integer To Floating-Point
		else if ((op == 0x2) && (op3 == 0x34)
					 && (
							((opf == 0x81) || (opf == 0x82) || (opf == 0x83) || (opf == 0xd1) || (opf == 0xd2) || (opf == 0xd3)) ||
							((opf == 0xc9) || (opf == 0xcd) || (opf == 0xc6) || (opf == 0xce) || (opf == 0xc7) || (opf == 0xcb)) || 
							((opf == 0x84) || (opf == 0x88) || (opf == 0x8c) || (opf == 0xc4) || (opf == 0xc8) || (opf == 0xcc)) 
						)) {
			theUsesFpCvt = true;
		}
		// A.13 - Floating-Point Compare
		else if ((op == 0x2) && (((theOpcode >> 27) & 0x7) == 0) && (op3 == 0x35) && ((opf == 0x51) || (opf == 0x52) || (opf == 0x53) || (opf == 0x55) || (opf == 0x56) || (opf == 0x57))) {		
			theUsesFpCmp = true;	
		}
		// A.18 - Floating-Point Multiply and Divide, A.19 - Floating-Point Square Root
		else if ((op == 0x2) && (op3 == 0x34)) {
			if ((opf == 0x49) || (opf == 0x4a)  || (opf == 0x4b) || (opf == 0x69) || (opf == 0x6e)) {
				theUsesFpMult = true;
			}
			else if ((opf == 0x4d)  || (opf == 0x4e)  || (opf == 0x4f)) {
				theUsesFpDiv = true;
			}
			else if ((opf == 0x29) || (opf == 0x2a) || (opf == 0x2b)) {
				theUsesFpSqrt = true;
			}
		}
    }
    
	// So that v9Decoder can send opcodes out to PowerTracker
	public:
		Opcode getOpcode() { return theOpcode; }
};


} //nv9Decoder

#endif //FLEXUS_v9DECODER_v9INSTRUCTION_HPP_INCLUDED
