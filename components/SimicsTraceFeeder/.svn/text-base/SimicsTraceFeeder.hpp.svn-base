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

#define FLEXUS_BEGIN_COMPONENT SimicsTraceFeeder
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#define SimicsTraceFeeder_IMPLEMENTATION (<components/SimicsTraceFeeder/SimicsTraceFeederImpl.hpp>)




#ifdef FLEXUS_ArchitecturalInstruction_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::ArchitecturalInstruction data type"
#endif

namespace Flexus {
namespace SimicsTraceFeeder {

    // Debug Setting for Instruction data type
    typedef Flexus::Debug::Debug InstructionDebugSetting;

    using boost::counted_base;
    using SharedTypes::PhysicalMemoryAddress;
    using namespace Flexus::MemoryPool;

    class SimicsX86MemoryOpImpl;
    struct SimicsX86MemoryOp : public counted_base {
        FLEXUS_DECLARE_DEBUG(InstructionDebugSetting, "SimicsX86MemoryOp" );      

        // x86 is 32-bit, so logical and physical addresses are 32-bit
        typedef int RegisterName;
        typedef int * RegisterName_iterator;

        // data word is also 32-bit
        typedef unsigned long data_word;

        enum eOpType {
          Read,
          Write
        };

        bool isMemory() const;  
        bool isLoad() const;    
        bool isStore() const;   
        bool isBranch() const;
        bool isInterrupt() const;
        bool requiresSync() const;
        RegisterName_iterator inputRegisters_begin() const;
        RegisterName_iterator inputRegisters_end() const;
        RegisterName_iterator outputRegisters_begin() const;
        RegisterName_iterator outputRegisters_end() const;
        PhysicalMemoryAddress physicalMemoryAddress() const; 
        void execute(); 
        void perform(); 
        void commit();  
        void squash();
        void release(); 
        bool isValid() const;
        bool isFetched() const;
        bool isDecoded() const;
        bool isExecuted() const;
        bool isPerformed() const;
        bool isCommitted() const;
        bool isSquashed() const;
        bool isExcepted() const;
        bool wasTaken() const;
        bool wasNotTaken() const;

        bool canExecute() const;  
        bool canPerform() const;  

        virtual ~SimicsX86MemoryOp() {}
      protected:
        SimicsX86MemoryOp(SimicsX86MemoryOpImpl * anImpl): theImpl(anImpl) {}       
      private:
        friend class SimicsX86MemoryOpImpl;
        template <class , class> friend class SimicsTraceFeederComponent;
    
        SimicsX86MemoryOpImpl * getImpl();
        // Note: raw ptr as a MemoryOp does not have any ownership
        // over its impl.  In fact, the reverse is true - the MemoryOp is
        // a sub-object of the Impl.
        SimicsX86MemoryOpImpl * theImpl;
    };  // end struct SimicsX86MemoryOp

} // end namespace SimicsTraceFeeder

namespace SharedTypes {

    #define FLEXUS_ArchitecturalInstruction_TYPE_PROVIDED
    typedef SimicsTraceFeeder::SimicsX86MemoryOp ArchitecturalInstruction;

} // end namespace SharedTypes
} // end namespace Flexus




namespace Flexus {
namespace SimicsTraceFeeder {
    typedef boost::mpl::push_front< 
        FLEXUS_PREVIOUS_InstructionTransport_Typemap,  
        std::pair< 
                Flexus::SharedTypes::ArchitecturalInstructionTag_t
            ,   Flexus::SharedTypes::ArchitecturalInstruction
            > 
        >::type InstructionTransport_Typemap;

    #undef FLEXUS_PREVIOUS_InstructionTransport_Typemap
    #define FLEXUS_PREVIOUS_InstructionTransport_Typemap Flexus::SimicsTraceFeeder::InstructionTransport_Typemap
} // end namespace SimicsTraceFeeder
} // end namespace Flexus



#ifdef FLEXUS_FeederCommand_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::FeederCommand data type"
#endif

namespace Flexus {
namespace SimicsTraceFeeder {

  using boost::intrusive_ptr;

  class Command {
    public:
      enum eCommands {
        Oracle_NextInstruction  // for compatibility with BWFetch component
      };
      typedef Core::FlexusException FeederException;
    public:
      Command() {}
      static Command OracleNextInstruction() { return Command(); } 
        static Command TakeException(boost::intrusive_ptr<Flexus::SharedTypes::ArchitecturalInstruction>) { return Command(); }
      friend std::ostream & operator << (std::ostream &, Command const &);

      // Default copy, assignment, and destructor
      eCommands getCommand() const;
    };  // end class Command

} // end namespace SimicsTraceFeeder

namespace SharedTypes {

    #define FLEXUS_FeederCommand_TYPE_PROVIDED
    typedef SimicsTraceFeeder::Command FeederCommand;

} // end namespace SharedTypes
} // end namespace Flexus


#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT SimicsTraceFeeder
