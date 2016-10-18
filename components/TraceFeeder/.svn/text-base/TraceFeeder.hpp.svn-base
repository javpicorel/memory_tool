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


#define FLEXUS_BEGIN_COMPONENT TraceFeeder
#include FLEXUS_BEGIN_COMPONENT_DECLARATION()

#define TraceFeeder_IMPLEMENTATION (<components/TraceFeeder/TraceFeederImpl.hpp>)


#ifdef FLEXUS_ArchitecturalInstruction_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::ArchitecturalInstruction data type"
#endif

namespace Flexus {
namespace SharedTypes {

  // tag for extracting the arch inst part of an InstructionTransport
	struct ArchitecturalInstructionTag_t {} ArchitecturalInstructionTag;

} // namespace SharedTypes
} // namespace Flexus

namespace nTraceFeeder {

    using boost::counted_base;
    using Flexus::SharedTypes::PhysicalMemoryAddress;

    //Debug Setting for Instruction data type
    typedef Flexus::Debug::Debug InstructionDebugSetting;

    class TraceInstructionImpl;
    struct TraceInstruction : public counted_base {
        FLEXUS_DECLARE_DEBUG(InstructionDebugSetting, "TraceInstruction" );

        //Need to create a register type once this is useful
        typedef int RegisterName;
        typedef int * RegisterName_iterator;

        bool isMemory() const;
        bool isLoad() const;
        bool isStore() const;
        bool isRmw() const;
        bool isSync() const;
        bool isMEMBAR() const;
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
        bool isReleased() const;
        bool wasTaken() const;
        bool wasNotTaken() const;

        bool canExecute() const;
        bool canPerform() const;
        bool canRelease() const;


      protected:
        TraceInstruction(TraceInstructionImpl * anImpl): theImpl(anImpl) {}
      private:
        //Note: raw ptr as an Instruction does not have any ownership
        //over its impl.  In fact, the reverse is true - the Instruction is
        //a sub-object of the Impl.
        TraceInstructionImpl * theImpl;
    };

    std::ostream & operator << (std::ostream & anOstream, TraceInstruction const & anInstruction);

} //End nTraceFeeder
namespace Flexus {
namespace SharedTypes {

    #define FLEXUS_ArchitecturalInstruction_TYPE_PROVIDED
    typedef nTraceFeeder::TraceInstruction ArchitecturalInstruction;

} //End SharedTypes
} //End Flexus




#ifdef FLEXUS_FeederCommand_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::FeederCommand data type"
#endif

namespace nTraceFeeder {

    struct TraceCommand {
        typedef Flexus::Core::FlexusException FeederException;
        static TraceCommand OracleNextInstruction() { return TraceCommand(); }
        static TraceCommand TakeException(boost::intrusive_ptr<Flexus::SharedTypes::ArchitecturalInstruction>) { return TraceCommand(); }
        TraceCommand const & command() const { return *this; }
        //Default copy, assignment, and destructor
    };

} //End nTraceFeeder
namespace Flexus {
namespace SharedTypes {

    #define FLEXUS_FeederCommand_TYPE_PROVIDED
    typedef nTraceFeeder::TraceCommand FeederCommand;

} //End SharedTypes
} //End Flexus


namespace nTraceFeeder {
  typedef boost::mpl::push_front<
    FLEXUS_PREVIOUS_InstructionTransport_Typemap,
      std::pair<
          Flexus::SharedTypes::ArchitecturalInstructionTag_t
        , Flexus::SharedTypes::ArchitecturalInstruction
        >
      >::type InstructionTransport_Typemap;

  #undef FLEXUS_PREVIOUS_InstructionTransport_Typemap
  #define FLEXUS_PREVIOUS_InstructionTransport_Typemap nTraceFeeder::InstructionTransport_Typemap
} //End nTraceFeeder


#include FLEXUS_END_COMPONENT_DECLARATION()
#define FLEXUS_END_COMPONENT TraceFeeder


