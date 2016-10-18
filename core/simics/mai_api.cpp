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

#include <boost/throw_exception.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/lambda/bind.hpp>
namespace ll = boost::lambda;

#include <core/target.hpp>
#include <core/types.hpp>
#include <core/simics/api_wrappers.hpp>

#include "mai_api.hpp"



namespace Flexus {
namespace Simics {

void onInterrupt (void * aPtr, Simics::API::conf_object_t * anObj, long long aVector);

struct InterruptManager {
  int theHandle;
  typedef std::map< Simics::API::conf_object_t *, boost::function< void( long long) > > dispatch_map;
  dispatch_map theInterruptDispatch;

  void registerHAP() {
		theHandle = API::SIM_hap_add_callback( "Core_Asynchronous_Trap", reinterpret_cast< API::obj_hap_func_t>(&onInterrupt), 0);
  }

  void registerCPU(API::conf_object_t * aCPU, boost::function< void( long long) > aDispatchFn ) {
    if (theInterruptDispatch.empty()) {
      registerHAP();
    }
    theInterruptDispatch.insert( std::make_pair( aCPU, aDispatchFn) );
  }

  void interrupt(API::conf_object_t * anObj, long long aVector ) {
    dispatch_map::iterator iter = theInterruptDispatch.find(anObj);
    if (iter != theInterruptDispatch.end()) {
      iter->second( aVector );
    }
  }

} theInterruptManager;

void onInterrupt (void * aPtr, API::conf_object_t * anObj, long long aVector) {
  theInterruptManager.interrupt( anObj, aVector );
}

#if FLEXUS_TARGET_IS(v9)

void v9ProcessorImpl::initialize() {
  DBG_( Verb, ( << "CPU[" << Simics::APIFwd::SIM_get_processor_number(*this) << "] Registering for interrupts "));
  theInterruptManager.registerCPU( *this, ll::bind( &v9ProcessorImpl::handleInterrupt, this, ll::_1 ) );
}


void v9ProcessorImpl::handleInterrupt( long long aVector) {
  DBG_( Verb, ( << "CPU[" << Simics::APIFwd::SIM_get_processor_number(*this) << "] Interrupt " << std::hex << aVector << std::dec ));
  thePendingInterrupt = aVector;
}


static const int kTL = 46;
static const int kTT1 = 78;

int v9ProcessorImpl::getPendingException() const {
  int tl = readRegister( kTL );
  if (tl > 0) {
    return readRegister( kTT1 + tl - 1);
  } else {
    return 0;
  }
}

int v9ProcessorImpl::getPendingInterrupt() const {
  if (thePendingInterrupt == API::Sim_PE_No_Exception) {
    return 0;
  }
  return thePendingInterrupt;
}


int v9ProcessorImpl::advance(bool anAcceptInterrupt) {
  int exception = 0;

  if (! theInterruptsConnected) {
    theInterruptsConnected = true;
    initialize();
  }

  if (thePendingInterrupt != API::Sim_PE_No_Exception && anAcceptInterrupt) {
    //Try to take the interrupt now.
    int interrupt = thePendingInterrupt;
    switch(API::SIM_instruction_handle_interrupt(*this, thePendingInterrupt)) {
      case API::Sim_IE_No_Exception:
        thePendingInterrupt = API::Sim_PE_No_Exception;
        DBG_( Crit, ( << "Got Sim_IE_No_Exception trying to take an interrupt" ) );
        break;
      case API::Sim_IE_OK:
        thePendingInterrupt = API::Sim_PE_No_Exception;
        return interrupt;
      case API::Sim_IE_Interrupts_Disabled:
        // OK try later */
        DBG_( Crit, ( << "Got Sim_IE_Interruts_Disabled trying to take an interrupt on CPU[" << Simics::APIFwd::SIM_get_processor_number(*this) << "]"  ) );
        return -1;
      case API::Sim_IE_Illegal_Interrupt_Point:
        // OK try later */
        DBG_( Crit, ( << "Got Sim_IE_Illegal_Interrupt_Point trying to take an interrupt" ) );
        return -1;
      case API::Sim_IE_Step_Breakpoint:
        //Return to prompt next cycle
        API::SIM_break_cycle(*this, 0);
        break;
      default:
        DBG_( Crit, ( << "Bad return value from SIM_instruction_handle_interrupt" ) );
    }
  }

  int except = API::SIM_get_pending_exception();
  if (except != API::SimExc_No_Exception) {
        DBG_( Crit, ( << "Pending Exception: " << except ) );
   }

  //Normal instruction processing
    API::instruction_id_t inst = API::SIM_instruction_begin(*this);
    API::SIM_instruction_insert(0, inst);

    bool retry= false;
    do {
      retry = false;
      API::instruction_error_t err = API::SIM_instruction_commit(inst);
      if (err == API::Sim_IE_Exception ) {
        err = API::SIM_instruction_handle_exception(inst);
        exception = getPendingException();
        DBG_( Verb, ( << "Exception raised by CPU[" << Simics::APIFwd::SIM_get_processor_number(*this) << "]: " << err << " Exception #:" << exception) );
        API::SIM_instruction_end(inst);
        if (err != API::Sim_IE_OK) {
          DBG_( Crit, ( << "Got an error trying to handle exception on CPU[" << Simics::APIFwd::SIM_get_processor_number(*this) << ": " << err << " exception was: " << exception) );
        }
        return exception; //Instruction is already ended
      } else if (err == API::Sim_IE_Code_Breakpoint || err == API::Sim_IE_Step_Breakpoint || err == API::Sim_IE_Hap_Breakpoint ) {
        DBG_( Verb, ( << "Triggerred Breakpoint on CPU[" << Simics::APIFwd::SIM_get_processor_number(*this) << "]" ) );
        API::SIM_break_cycle(*this, 0);
        retry = true;
      } else if (err != API::Sim_IE_OK) {
        DBG_( Crit, ( << "Got an error trying to advance CPU[" << Simics::APIFwd::SIM_get_processor_number(*this) << "]: " << err) );
        //Some error other than an exception
      }
    } while( retry );
    API::SIM_instruction_end(inst);

  return exception;
}

unsigned long long endianFlip(unsigned long long val, int aSize);

unsigned long long v9ProcessorImpl::interruptRead(VirtualMemoryAddress anAddress, int anASI) const {
  DBG_( Verb, ( << "Reading " << anAddress << " in ASI: " << std::hex << anASI << std::dec ) );
  API::logical_address_t addr(anAddress);
  unsigned long long aValue = 0;
  API::v9_memory_transaction_t xact;
  memset( &xact, 0, sizeof(API::v9_memory_transaction_t ) );
  xact.priv = 1;
  #if SIM_VERSION < 1200
    xact.align_kind = API::Align_Natural;
  #else
    //align_kind was replaced by access_type in Simics 2.2.x
    xact.access_type = API::V9_Access_Normal;
  #endif
  xact.address_space = anASI;
  xact.s.logical_address = addr;
  xact.s.physical_address = addr;
  xact.s.size = 8;
  xact.s.type = API::Sim_Trans_Load;
  xact.s.inquiry = 1;
  xact.s.ini_type = API::Sim_Initiator_CPU_UIII;
  xact.s.ini_ptr = theProcessor;
  xact.s.real_address = reinterpret_cast<char*>(&aValue);
  xact.s.exception = API::Sim_PE_No_Exception;

  API::exception_type_t except = sparc()->access_asi_handler(theProcessor, &xact);
  if (except != API::Sim_PE_No_Exception) {
    DBG_( Dev, ( << "except: " << except  ) );
  }
  return endianFlip(aValue, 8);
}


#endif //FLEXUS_TARGET_IS(v9)


} //end Namespace Simics
} //end namespace Flexus

