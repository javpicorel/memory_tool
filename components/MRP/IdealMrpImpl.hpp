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

#define FLEXUS_BEGIN_COMPONENT IdealMrpComponent
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

  #define DBG_DefineCategories MRP
  #define DBG_SetDefaultOps AddCat(MRP)
  #include DBG_Control()


#include "MrpMain.hpp"
#include "SordTraceReader.hpp"

namespace nIdealMrp {


using namespace Flexus;
using namespace Core;
using namespace SharedTypes;


using boost::intrusive_ptr;

typedef nSordTraceReader::SordTraceEntry TraceEntry;
typedef SharedTypes::PhysicalMemoryAddress MemoryAddress;


template <class Cfg>
class IdealMrpComponent : public FlexusComponentBase<IdealMrpComponent, Cfg> {
  FLEXUS_COMPONENT_IMPL(IdealMrpComponent, Cfg);

  // Encompassing MRP type
  typedef nMrpTable::MrpMain MrpMain;


public:
  IdealMrpComponent( FLEXUS_COMP_CONSTRUCTOR_ARGS )
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , theMrps(0)
    , myLastTime(0)
  {
    // verify address lengths match
    DBG_Assert(sizeof(nSordTraceReader::Address) == sizeof(MemoryAddress));
  }

  ~IdealMrpComponent() {
    if(theMrps) {
      for(int ii = 0; ii < HACK_WIDTH; ++ii) {
        theMrps[ii].~MrpMain();
      }
      delete [] theMrps;
      theMrps = 0;
    }
  }

  // Initialization
  void initialize() {
    theMemoryMap = SharedTypes::MemoryMap::getMemoryMap(flexusIndex());

    myTraceReader.init();

    // create the MRP for each node
    theMrps = (MrpMain*)( new char[HACK_WIDTH * sizeof(MrpMain)] );
    // this uses placement new, so on delete, make sure to manually call
    // the destructor for each MRP, then delete[] the array
    for(int ii = 0; ii < HACK_WIDTH; ++ii) {
      new(theMrps+ii) MrpMain(cfg.BlockAddrBits.value, cfg.L2BlockSize.value,
                              cfg.NumSets.value, cfg.Assoc.value);
      theMrps[ii].init(std::string("mrp"), ii);
    }
  }

  //Ports - no ports!!

  //Drive Interfaces
  struct IdealMrpDrive {
    typedef FLEXUS_IO_LIST_EMPTY Inputs;
    typedef FLEXUS_IO_LIST_EMPTY Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void doCycle(self & aMrp) {
      TraceEntry nextEntry;
      aMrp.myTraceReader.next(nextEntry);
      DBG_Assert(nextEntry.time >= aMrp.myLastTime);
      aMrp.myLastTime = nextEntry.time;

      // figure out the home node and pass the info along
      int home = aMrp.theMemoryMap->node(MemoryAddress(nextEntry.address));
      DBG_Assert( (home>=0) && (home<HACK_WIDTH) );
      if(nextEntry.production) {
        DBG_(VVerb, Comp(aMrp) ( << "got write for " << std::hex << nextEntry.address ) );
        aMrp.theMrps[home].write(MemoryAddress(nextEntry.address), nextEntry.node);
      }
      else {
        DBG_(VVerb, Comp(aMrp) ( << "got read for " << std::hex << nextEntry.address ) );
        aMrp.theMrps[home].read(MemoryAddress(nextEntry.address), nextEntry.node);
      }
    }

  };  // end IdealMrpDrive

  // Declare the list of Drive interfaces
  typedef FLEXUS_DRIVE_LIST(1, IdealMrpDrive) DriveInterfaces;

private:

  // the MRPs for each node
  nMrpTable::MrpMain * theMrps;

  // for determining node IDs
  boost::intrusive_ptr<SharedTypes::MemoryMap> theMemoryMap;

  // the trace reader
  nSordTraceReader::SordTraceReader myTraceReader;

  nSordTraceReader::Time myLastTime;
};

FLEXUS_COMPONENT_CONFIGURATION_TEMPLATE(IdealMrpConfiguration,
  FLEXUS_PARAMETER( NumSets, int, "# Sets in sig table", "sets", Static<128>, "128" )
  FLEXUS_PARAMETER( Assoc, int, "Assoc of sig table", "assoc", Static<4>, "4" )
  FLEXUS_PARAMETER( BlockAddrBits, int, "Addr bits in sig", "addr_bits", Static<12>, "12" )
  FLEXUS_PARAMETER( L2BlockSize, int, "Block size of L2 cache", "bsize", Static<64>, "64" )
);

} //End Namespace nIdealMrp

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT IdealMrpComponent

  #define DBG_Reset
  #include DBG_Control()

