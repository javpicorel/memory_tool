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

#define FLEXUS_BEGIN_COMPONENT SordTraceMemory
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#ifndef HACK_WIDTH
#error "HACK_WIDTH must be defined to use the SordTraceMemory component"
#endif

#include "../DGP/DgpMain.hpp"

  #define DBG_DefineCategories SordTrace, Memory
  #define DBG_SetDefaultOps AddCat(SordTrace | Memory)
  #include DBG_Control()


namespace nSordTraceMemory {

using namespace Flexus;

using namespace Core;
using namespace SharedTypes;

using boost::intrusive_ptr;

typedef FILE   * FILE_p;
typedef FILE_p * FILE_pp;

typedef SharedTypes::PhysicalMemoryAddress MemoryAddress;
typedef unsigned long Time;

enum BlockStateEnum {
  StateShared,
  StateExclusive
};
struct BlockState : boost::counted_base {
  BlockState(int node)
    : state(StateShared)
    , producer(-1)
    , time(0)
    , sharers(1<<node)
  {}
  BlockState(int node, Time t)
    : state(StateExclusive)
    , producer(node)
    , time(t)
    , sharers(0)
  {}

  bool isSharer(int node) {
    return( sharers & (1<<node) );
  }
  void setSharer(int node) {
    sharers |= (1<<node);
  }
  void newShareList(int node, int oldOwner) {
    sharers = (1<<node);
    sharers |= (1<<oldOwner);
  }

  BlockStateEnum state;
  int producer;
  Time time;
  unsigned int sharers;
};
typedef intrusive_ptr<BlockState> BlockState_p;

class MemoryStateTable {
  typedef std::map<MemoryAddress,BlockState_p> StateMap;
  typedef StateMap::iterator Iterator;
  typedef std::pair<MemoryAddress,BlockState_p> BlockPair;
  typedef std::pair<Iterator,bool> InsertPair;

public:
  MemoryStateTable()
  {}

  BlockState_p find(MemoryAddress anAddr) {
    Iterator iter = theBlockStates.find(anAddr);
    if(iter != theBlockStates.end()) {
      return iter->second;
    }
    return BlockState_p(0);
  }

  void remove(MemoryAddress anAddr) {
    Iterator iter = theBlockStates.find(anAddr);
    if(iter != theBlockStates.end()) {
      theBlockStates.erase(iter);
    }
  }

  void addEntry(MemoryAddress anAddr, BlockState_p anEntry) {
    // try to insert a new entry - if successful, we're done;
    // otherwise abort
    BlockPair insert = std::make_pair(anAddr, anEntry);
    InsertPair result = theBlockStates.insert(insert);
    DBG_Assert(result.second);
  }

private:
  StateMap theBlockStates;

};  // end class MemoryStateTable


template <class Configuration>
class SordTraceMemoryComponent : public FlexusComponentBase<SordTraceMemoryComponent, Configuration> {
  FLEXUS_COMPONENT_IMPL(SordTraceMemoryComponent, Configuration);

  typedef nDgpTable::DgpMain DgpMain;

public:
  static const int K = 1024;
  static const int M = 1024 * K;
  static const int kMaxFileSize = 512 * M;

  SordTraceMemoryComponent( FLEXUS_COMP_CONSTRUCTOR_ARGS )
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )
    , prodFiles(0)
    , consFiles(0)
    , theDgps(0)
  {}

  ~SordTraceMemoryComponent() {
    // close the files
    if(prodFiles) {
      for(int ii = 0; ii < HACK_WIDTH; ii++) {
        fclose(prodFiles[ii]);
        fclose(consFiles[ii]);
      }
      delete [] prodFiles;
      delete [] consFiles;
    }

    // free the DGP memory
    if(theDgps) {
      for(int ii = 0; ii < HACK_WIDTH; ++ii) {
        theDgps[ii].~DgpMain();
      }
      delete [] theDgps;
      theDgps = 0;
    }
  }

  std::string nextConsumerFile(int aCpu) {
    return ( std::string("consumer") + boost::lexical_cast<std::string>(aCpu) + ".sordtrace." + boost::lexical_cast<std::string>(consFileNo[aCpu]++) );
  }

  std::string nextProducerFile(int aCpu) {
    return ( std::string("producer") + boost::lexical_cast<std::string>(aCpu) + ".sordtrace." + boost::lexical_cast<std::string>(prodFileNo[aCpu]++) );
  }

  // Initialization
  void initialize() {
    if(cfg.Trace.value) {
      prodFiles = new FILE_p[HACK_WIDTH];
      consFiles = new FILE_p[HACK_WIDTH];

      for(int ii = 0; ii < HACK_WIDTH; ii++) {
        prodFileNo[ii] = 0;
        prodFileLengths[ii] = 0;
        consFileNo[ii] = 0;
        consFileLengths[ii] = 0;
        prodFiles[ii] = fopen( nextProducerFile(ii).c_str(), "w" );
        consFiles[ii] = fopen( nextConsumerFile(ii).c_str(), "w" );
      }
    }

    if(cfg.DGP.value) {
      // create the DGP for each node
      theDgps = (DgpMain*)( new char[HACK_WIDTH * sizeof(DgpMain)] );
      // this uses placement new, so on delete, make sure to manually call
      // the destructor for each DGP, then delete[] the array
      for(int ii = 0; ii < HACK_WIDTH; ++ii) {
        new(theDgps+ii) DgpMain(cfg.AddrBits.value, cfg.PCBits.value, cfg.BlockSize.value,
                                cfg.DgpSets.value, cfg.DgpAssoc.value);
        theDgps[ii].init(std::string("dgp"), ii);
      }
    }
  }

  // Ports
  struct ToNode : public PushOutputPortArray<MemoryTransport, HACK_WIDTH> { };

  struct FromNode : public PushInputPortArray<MemoryTransport, HACK_WIDTH>, AlwaysAvailable {
    typedef FLEXUS_IO_LIST( 2, Availability<ToNode>
                               , Value<FromNode> ) Inputs;
    typedef FLEXUS_IO_LIST( 1, Value<ToNode> ) Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void push(self& aMemory, index_t anIndex, MemoryTransport transport) {
      DBG_(Iface, Comp(aMemory) Addr(transport[MemoryMessageTag]->address())
                  ( << "request from node " << anIndex << " received: " << *transport[MemoryMessageTag] ) );

      intrusive_ptr<MemoryMessage> reply;
      MemoryAddress blockAddr = aMemory.blockAddress(transport[MemoryMessageTag]->address());
      switch (transport[MemoryMessageTag]->type()) {
        case MemoryMessage::LoadReq:
          aMemory.recvRead(static_cast<int>(anIndex), blockAddr);
          reply = new MemoryMessage(MemoryMessage::LoadReply, transport[MemoryMessageTag]->address());
          break;
        case MemoryMessage::StoreReq:
          {
            nExecute::ExecuteState & ex = static_cast<nExecute::ExecuteState &>(*transport[ExecuteStateTag]);
            MemoryAddress pc = ex.instruction().physicalInstructionAddress();
            aMemory.recvWrite(static_cast<int>(anIndex), blockAddr, pc);
            reply = new MemoryMessage(MemoryMessage::StoreReply, transport[MemoryMessageTag]->address());
          }
          break;
        default:
          DBG_Assert(false, Component(aMemory) (<< "Don't know how to handle message.  No reply sent.") );
          return;
      }

      transport.set(MemoryMessageTag, reply);
      DBG_Assert( FLEXUS_CHANNEL_ARRAY(aMemory, ToNode, anIndex).available() );
      DBG_(Iface, Comp(aMemory) Addr(transport[MemoryMessageTag]->address())
                  ( << "reply for node " << anIndex << ": " << *transport[MemoryMessageTag] ) );
      FLEXUS_CHANNEL_ARRAY(aMemory, ToNode, anIndex) << transport;
    }
  };

  // Declare the list of Drive interfaces
  typedef FLEXUS_DRIVE_LIST_EMPTY DriveInterfaces;


private:
  //--- Base functions for maintaining state -----------------------------------
  void recvRead(int node, MemoryAddress addr) {
    DBG_(VVerb, ( << "got read from node " << node << " for addr " << addr ) );
    // grab the current state of this block
    BlockState_p block = myStateTable.find(addr);
    if(block) {
      DBG_(VVerb, ( << "retrieved block state" ) );

      if(block->state == StateShared) {
        DBG_(VVerb, ( << "state shared" ) );

        if(!block->isSharer(node)) {
          DBG_(VVerb, ( << "new sharer" ) );

          // new sharer - record first access to this block by this node
          consumption(addr, node, currentTime(), block->producer, block->time);
          block->setSharer(node);
        }
      }
      else {  // StateExclusive
        // check if the reader is already the node with exclusive access
        if(block->producer == node) {
          DBG_(VVerb, ( << "state exclusive - read by owner" ) );
        }
        else {
          DBG_(VVerb, ( << "state exclusive - read by another" ) );

          // producer gets a snoop
          snoop(block->producer, addr);
          // record the last store time of this address to file
          production(block->producer, addr, block->time);
          // transition to shared and record this first read time
          block->state = StateShared;
          block->newShareList(node, block->producer);
          consumption(addr, node, currentTime(), block->producer, block->time);
        }
      }
    }
    else {
      DBG_(VVerb, ( << "no existing block state" ) );

      // no current state for this block
      block = new BlockState(node);
      myStateTable.addEntry(addr, block);
      // record this first read time
      consumption(addr, node, currentTime(), block->producer, block->time);
    }
    DBG_(VVerb, ( << "done read" ) );
  }

  void recvWrite(int node, MemoryAddress addr, MemoryAddress pc) {
    DBG_(VVerb, ( << "got write from node " << node << " for addr " << addr ) );

    // the current node gets a write
    write(node, addr, pc);

    // grab the current state of this block
    BlockState_p block = myStateTable.find(addr);
    if(block) {
      DBG_(VVerb, ( << "retrieved block state" ) );

      if(block->state == StateShared) {
        DBG_(VVerb, ( << "state shared" ) );

        // every sharer gets a snoop (except the current node)
        for(int ii = 0; ii < HACK_WIDTH; ++ii) {
          if(block->isSharer(ii) && (node != ii)) {
            snoop(ii, addr);
          }
        }
        // transition the block to exclusive
        block->state = StateExclusive;
        block->producer = node;
        block->time = currentTime();
      }
      else {  // StateExclusive
        DBG_(VVerb, ( << "state exclusive" ) );

        if(node == block->producer) {
          DBG_(VVerb, ( << "same producer" ) );

          // same node - just update production time
          block->time = currentTime();
        }
        else {
          DBG_(VVerb, ( << "new producer" ) );

          // the old producer gets a snoop
          snoop(block->producer, addr);
          // different node - record the last production for that node
          production(block->producer, addr, block->time);
          // transition to new producer
          block->producer = node;
          block->time = currentTime();
        }
      }
    }
    else {
      DBG_(VVerb, ( << "no existing block state" ) );

      // no current state for this block
      block = new BlockState(node, currentTime());
      myStateTable.addEntry(addr, block);
    }
    DBG_(VVerb, ( << "done read" ) );
  }


  //--- Functions for SORD trace generation ------------------------------------
  void switchProdFile(int aProducer) {
     fclose(prodFiles[aProducer]);
     std::string new_file = nextProducerFile(aProducer);
     prodFiles[aProducer] = fopen( new_file.c_str(), "w" );
     prodFileLengths[aProducer] = 0;
     DBG_(Dev, Comp(*this)
       ( << "switched to new producer file: " << new_file ) );
  }

  void switchConsFile(int aConsumer) {
     fclose(prodFiles[aConsumer]);
     std::string new_file = nextConsumerFile(aConsumer);
     consFiles[aConsumer] = fopen( new_file.c_str(), "w" );
     consFileLengths[aConsumer] = 0;
     DBG_(Dev, Comp(*this)
       ( << "switched to new consumer file: " << new_file ) );

  }

  void writeProdFile( int aLength, int aProducer) {
    fwrite(theBuffer, aLength, 1, prodFiles[aProducer]);

    prodFileLengths[aProducer] += aLength;
    if (prodFileLengths[aProducer] > kMaxFileSize) {
      switchProdFile(aProducer);
    }
  }

  void writeConsFile( int aLength, int aConsumer) {
    fwrite(theBuffer, aLength, 1, consFiles[aConsumer]);

    consFileLengths[aConsumer] += aLength;
    if (consFileLengths[aConsumer] > kMaxFileSize) {
      switchConsFile(aConsumer);
    }
  }

  void production(int producer, MemoryAddress address, Time time) {
    if(!cfg.Trace.value) {
      return;
    }

    DBG_(VVerb, (  << "production: node=" << producer
                  << " address=" << address
                  << " time=" << time
               ) );

    int offset = 0;
    int len;

    long long addr = address;
    len = sizeof(long long);
    memcpy(theBuffer+offset, &addr, len);
    offset += len;

    len = sizeof(Time);
    memcpy(theBuffer+offset, &time, len);
    offset += len;

    writeProdFile(offset, producer);
  }

  void consumption(MemoryAddress address, int consumer, Time consumptionTime,
                   int producer, Time productionTime) {

    if(!cfg.Trace.value) {
      return;
    }

    DBG_(VVerb, (  << "consumption: address=" << address
                  << " consumer=" << consumer
                  << " consumption time=" << consumptionTime
                  << " producer=" << producer
                  << " production time=" << productionTime
                ) );

    int offset = 0;
    int len;

    long long addr = address;
    len = sizeof(long long);
    memcpy(theBuffer+offset, &addr, len);
    offset += len;

    len = sizeof(Time);
    memcpy(theBuffer+offset, &consumptionTime, len);
    offset += len;

    len = sizeof(int);
    memcpy(theBuffer+offset, &producer, len);
    offset += len;

    len = sizeof(Time);
    memcpy(theBuffer+offset, &productionTime, len);
    offset += len;

    writeConsFile(offset, consumer);
  }

  //--- Functions for DGP simulation -------------------------------------------
  void write(int node, MemoryAddress addr, MemoryAddress pc) {
    if(!cfg.DGP.value) {
      return;
    }

    theDgps[node].write(addr, pc);
  }

  void snoop(int node, MemoryAddress addr) {
    if(!cfg.DGP.value) {
      return;
    }

    theDgps[node].snoop(addr);
  }


  //--- Utility functions ------------------------------------------------------
  MemoryAddress blockAddress(MemoryAddress addr) {
    return MemoryAddress( addr & ~(cfg.BlockSize.value-1) );
  }

  Time currentTime() {
    return theFlexus->cycleCount();
  }

  // state for all active blocks
  MemoryStateTable myStateTable;

  // output files
  FILE_pp prodFiles;
  FILE_pp consFiles;
  int     prodFileNo[HACK_WIDTH];
  int     consFileNo[HACK_WIDTH];
  long    prodFileLengths[HACK_WIDTH];
  long    consFileLengths[HACK_WIDTH];

  // file I/O buffer
  unsigned char theBuffer[32];

  // the DGP array
  DgpMain * theDgps;
};

FLEXUS_COMPONENT_CONFIGURATION_TEMPLATE(SordTraceMemoryConfiguration,
  FLEXUS_PARAMETER( Trace, int, "Enable trace generation", "trace", Static<1>, "1" )
  FLEXUS_PARAMETER( DGP, int, "Enable DGP simulation", "dgp", Static<0>, "0" )
  FLEXUS_PARAMETER( BlockSize, int, "Size of coherence unit", "bsize", Static<64>, "64" )
  FLEXUS_PARAMETER( DgpSets, int, "# Sets in DGP sig table", "sets", Static<128>, "128" )
  FLEXUS_PARAMETER( DgpAssoc, int, "Assoc of DGP sig table", "assoc", Static<4>, "4" )
  FLEXUS_PARAMETER( AddrBits, int, "Address bits in sig", "addr_bits", Static<12>, "12" )
  FLEXUS_PARAMETER( PCBits, int, "PC bits used in history", "pc_bits", Static<15>, "15" )
);

} //End Namespace nSordTraceMemory


#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT SordTraceMemory

  #define DBG_Reset
  #include DBG_Control()
