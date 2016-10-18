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

#include <components/EnhancedSordTraceMemory/EnhancedSordTraceMemory.hpp>

#include <zlib.h>
#include <list>
#include <fstream>
#include <vector>

#include <core/stats.hpp>

#include <core/simics/configuration_api.hpp>

#include <components/Common/MemoryMap.hpp>


#define FLEXUS_BEGIN_COMPONENT EnhancedSordTraceMemory
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

  #define DBG_DefineCategories EnhancedSordTrace
  #define DBG_SetDefaultOps AddCat(EnhancedSordTrace)
  #include DBG_Control()


namespace nEnhancedSordTraceMemory {

using namespace Flexus;

using namespace Core;
using namespace SharedTypes;
using namespace Flexus::Stat;

using boost::intrusive_ptr;

class EnhTraceMemoryInterface {
public:
  virtual void flushFiles() {
    DBG_Assert(false);
  }
};

class EnhancedSordTraceMemory_SimicsObject_Impl  {
    EnhTraceMemoryInterface * theComponentInterface; //Non-owning pointer
  public:
    EnhancedSordTraceMemory_SimicsObject_Impl(Flexus::Simics::API::conf_object_t * /*ignored*/ ) : theComponentInterface(0) {}

    void setComponentInterface(EnhTraceMemoryInterface * aComponentInterface) {
      theComponentInterface = aComponentInterface;
    }

    void flushTraceFiles() {
      theComponentInterface->flushFiles();
    }

};

class EnhancedSordTraceMemory_SimicsObject : public Simics::AddInObject <EnhancedSordTraceMemory_SimicsObject_Impl> {
    typedef Simics::AddInObject<EnhancedSordTraceMemory_SimicsObject_Impl> base;
   public:
    static const Simics::Persistence  class_persistence = Simics::Session;
    //These constants are defined in Simics/simics.cpp
    static std::string className() { return "EnhancedSordTraceMemory"; }
    static std::string classDescription() { return "EnhancedSordTraceMemory object"; }

    EnhancedSordTraceMemory_SimicsObject() : base() { }
    EnhancedSordTraceMemory_SimicsObject(Simics::API::conf_object_t * aSimicsObject) : base(aSimicsObject) {}
    EnhancedSordTraceMemory_SimicsObject(EnhancedSordTraceMemory_SimicsObject_Impl * anImpl) : base(anImpl) {}

    template <class Class>
    static void defineClass(Class & aClass) {

      aClass.addCommand
        ( & EnhancedSordTraceMemory_SimicsObject_Impl::flushTraceFiles
        , "flush-trace-files"
        , "flush the current store/prod/cons files and switch to next"
        );

    }

};

Simics::Factory<EnhancedSordTraceMemory_SimicsObject> theEnhancedSordTraceMemoryFactory;



typedef FILE   * FILE_p;
typedef FILE_p * FILE_pp;

typedef SharedTypes::PhysicalMemoryAddress MemoryAddress;
typedef unsigned long long Time;

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
    , pc(0)
  {}
  BlockState(int node, Time t, MemoryAddress aPc, bool aPriv)
    : state(StateExclusive)
    , producer(node)
    , time(t)
    , sharers(0)
    , pc(aPc)
    , priv(false)
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
  MemoryAddress pc;
  bool priv;
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

  long long size() {
    return theBlockStates.size();
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


class FLEXUS_COMPONENT(EnhancedSordTraceMemory), public EnhTraceMemoryInterface {
  FLEXUS_COMPONENT_IMPL(EnhancedSordTraceMemory);

  EnhancedSordTraceMemory_SimicsObject theEnhancedSordTraceMemoryObject;

  struct Xact {
    int theNode;
    MemoryAddress theAddr;
    MemoryAddress thePC;
    bool thePriv;
    unsigned long theOpcode;
    Xact(int aNode, MemoryAddress anAddr, MemoryAddress aPC, bool aPriv, unsigned long anOpcode)
      : theNode(aNode)
      , theAddr(anAddr)
      , thePC(aPC)
      , thePriv(aPriv)
      , theOpcode(anOpcode)
      {}
  };


  bool disableTracing;
  long long memoryOpsReceived;

  std::list< Xact > theReads;
  std::list< Xact > theWrites;
  std::list< Xact > theRMWs;


  // state for all active blocks
  MemoryStateTable myStateTable;

  // output files
  struct OutfileRecord {
    std::string theBaseName;
    int theNumProcs;
    std::vector<gzFile> theFiles;
    std::vector<int>      theFileNo;
    std::vector<long>     theFileLengths;

    static const int K = 1024;
    static const int M = 1024 * K;
    static const int kMaxFileSize = 512 * M;

    OutfileRecord(char * aName, int aNumProcs)
      : theBaseName(aName)
      , theNumProcs(aNumProcs)
      , theFiles(0)
    {
      theFileNo.resize(theNumProcs,0);
      theFileLengths.resize(theNumProcs,0);
    }
    ~OutfileRecord() {
      for(unsigned ii = 0; ii < theFiles.size(); ii++) {
        gzclose(theFiles[ii]);
      }
    }

    void init() {
      theFiles.resize(theNumProcs, 0);
      for(int ii = 0; ii < theNumProcs; ii++) {
        theFiles[ii] = gzopen( nextFile(ii).c_str(), "w" );
      }
    }
    void flush() {
      for(int ii = 0; ii < theNumProcs; ii++) {
        switchFile(ii);
      }
    }
    std::string nextFile(int aCpu) {
      return ( theBaseName + boost::lexical_cast<std::string>(aCpu) + ".sordtrace64." + boost::lexical_cast<std::string>(theFileNo[aCpu]++) + ".gz" );
    }
    void switchFile(int aCpu) {
      gzclose(theFiles[aCpu]);
      std::string new_file = nextFile(aCpu);
      theFiles[aCpu] = gzopen( new_file.c_str(), "w" );
      theFileLengths[aCpu] = 0;
      DBG_(Dev, ( << "switched to new trace file: " << new_file ) );
    }
    void writeFile(unsigned char * aBuffer, int aLength, int aCpu) {
      gzwrite(theFiles[aCpu], aBuffer, aLength);

      theFileLengths[aCpu] += aLength;
      if (theFileLengths[aCpu] > kMaxFileSize) {
        switchFile(aCpu);
      }
    }
  };
  OutfileRecord Productions;
  OutfileRecord Consumptions;
  OutfileRecord Upgrades;
  //OutfileRecord Reads;
  //OutfileRecord Stores;
  //OutfileRecord Opcodes;
  OutfileRecord RMWs;

  // file I/O buffer
  unsigned char theBuffer[64];

  std::ofstream theMemoryOut;

  std::vector< boost::intrusive_ptr<MemoryMap> > theMemoryMaps;

  unsigned long long theLastSwitch;

  //Statistics

  StatCounter statReads;
  StatCounter statOSReads;
  StatCounter statWrites;
  StatCounter statOSWrites;
  StatCounter statRMWs;
  StatCounter statOSRMWs;
  StatCounter statProductions;
  StatCounter statOSProductions;
  StatCounter statConsumptions;
  StatCounter statOSConsumptions;
  StatCounter statUpgrades;
  StatCounter statOSUpgrades;
  StatCounter statColdReads;
  StatCounter statColdWrites;
  StatCounter statColdConsumptions;
  StatUniqueCounter<long long> statUniqueAddresses;
  StatUniqueCounter<long long> statUniqueOSAddresses;
  StatUniqueCounter<long long> statUniqueUserAddresses;
  StatUniqueCounter<long long> statUniqueSharedAddresses;
  StatUniqueCounter<long long> statUniqueSharedOSAddresses;
  StatUniqueCounter<long long> statUniqueSharedUserAddresses;
  StatInstanceCounter<long long> statSharedAddressInstances;

public:

  FLEXUS_COMPONENT_CONSTRUCTOR(EnhancedSordTraceMemory)
    : base( FLEXUS_PASS_CONSTRUCTOR_ARGS )

    , Productions("producer", cfg.NumProcs)
    , Consumptions("consumer", cfg.NumProcs)
    , Upgrades("upgrade", cfg.NumProcs)
    , RMWs("rmw", cfg.NumProcs)
    , theLastSwitch(0)

    //Stat initialization
    , statReads(statName() + "-Reads")
    , statOSReads(statName() + "-Reads:OS")
    , statWrites( statName() + "-Writes")
    , statOSWrites(statName() + "-Writes:OS")
    , statRMWs( statName() + "-RMWs")
    , statOSRMWs(statName() + "-RMWs:OS")
    , statProductions(statName() + "-Productions")
    , statOSProductions(statName() + "-Productions:OS")
    , statConsumptions(statName() + "-Consumptions")
    , statOSConsumptions(statName() + "-Consumptions:OS")
    , statUpgrades(statName() + "-Upgrades")
    , statOSUpgrades(statName() + "-Upgrades:OS")
    , statColdReads(statName() + "-Reads:cold")
    , statColdWrites(statName() + "-Writes:cold")
    , statColdConsumptions(statName() + "Consumptions:cold")
    , statUniqueAddresses(statName() + "-UniqueAddresses")
    , statUniqueOSAddresses(statName() + "-UniqueAddresses:OS")
    , statUniqueUserAddresses(statName() + "-UniqueAddresses:User")
    , statUniqueSharedAddresses(statName() + "-UniqueAddresses:Shared")
    , statUniqueSharedOSAddresses(statName() + "-UniqueAddresses:Shared:OS")
    , statUniqueSharedUserAddresses(statName() + "-UniqueAddresses:Shared:User")
    , statSharedAddressInstances(statName() + "-SharedAddressInstances")

  {
    theEnhancedSordTraceMemoryObject = theEnhancedSordTraceMemoryFactory.create("enh-sord-trace-mem");
    theEnhancedSordTraceMemoryObject->setComponentInterface(this);
  }

  bool isQuiesced() const {
    return theWrites.empty() && theRMWs.empty() && theReads.empty();
  }

  // Initialization
  void initialize() {
      if (cfg.TraceEnabled) {
        Productions.init();
        Consumptions.init();
        Upgrades.init();
        //Reads.init();
        //Stores.init();
        RMWs.init();
        //Opcodes.init();
        disableTracing = false;
        memoryOpsReceived = 0;
      }

      if (cfg.CurvesEnabled) {
        theMemoryOut.open("memory.out");
        getStatManager()->openLoggedPeriodicMeasurement("Memory over Time", 1000000, Accumulate, theMemoryOut ,statName() + "-(?!SharedAddressInstances).*");
      }

      for(int ii = 0; ii < cfg.NumProcs; ii++) {
        theMemoryMaps.push_back(MemoryMap::getMemoryMap(ii));
      }

      theLastSwitch = theFlexus->cycleCount();
  }

  // Ports

  FLEXUS_PORT_ARRAY_ALWAYS_AVAILABLE(FromNode);
  void push(interface::FromNode const &, index_t anIndex, MemoryTransport & transport) {
      DBG_(Iface, Comp(*this) Addr(transport[MemoryMessageTag]->address())
                  ( << "request from node " << anIndex << " received: " << *transport[MemoryMessageTag] ) );

      memoryOpsReceived++;
      if (cfg.TraceUntil > 0 &&
          memoryOpsReceived > cfg.TraceUntil) {
        disableTracing = true;
      }

      intrusive_ptr<MemoryMessage> reply;
      MemoryAddress blockAddr = blockAddress(transport[MemoryMessageTag]->address());
      //Allocate the page in the MemoryMap, if not already done
      theMemoryMaps[anIndex]->node(blockAddr);
      MemoryAddress pc = transport[MemoryMessageTag]->pc();
      bool priv = transport[MemoryMessageTag]->isPriv();
      unsigned long opcode = 0; //Not supported

      switch (transport[MemoryMessageTag]->type()) {
        case MemoryMessage::LoadReq:
          theReads.push_back( Xact( static_cast<int>(anIndex), blockAddr, pc, priv, opcode ) );
          reply = new MemoryMessage(MemoryMessage::LoadReply, transport[MemoryMessageTag]->address());
          break;
        case MemoryMessage::StoreReq:
          theWrites.push_back( Xact( static_cast<int>(anIndex), blockAddr, pc, priv, opcode ) );
          reply = new MemoryMessage(MemoryMessage::StoreReply, transport[MemoryMessageTag]->address());
          break;
        case MemoryMessage::RMWReq:
          theRMWs.push_back( Xact( static_cast<int>(anIndex), blockAddr, pc, priv, opcode ) );
          reply = new MemoryMessage(MemoryMessage::RMWReply, transport[MemoryMessageTag]->address());
          break;
        default:
          DBG_Assert(false, Component(*this) (<< "Don't know how to handle message.  No reply sent.") );
          return;
      }

      transport.set(MemoryMessageTag, reply);
      DBG_Assert( FLEXUS_CHANNEL_ARRAY(ToNode, anIndex).available() );
      DBG_(Iface, Comp(*this) Addr(transport[MemoryMessageTag]->address())
                  ( << "reply for node " << anIndex << ": " << *transport[MemoryMessageTag] ) );
      FLEXUS_CHANNEL_ARRAY(ToNode, anIndex) << transport;
  }

  void drive( interface::MemoryDrive const &) {
      DBG_( VVerb, Comp(theComponent) ( << "MemoryDrive" ));
      doMemoryDrive();
  }


public:
  void doMemoryDrive() {
    while (! theWrites.empty() ) {
      recvWrite( theWrites.front().theNode, theWrites.front().theAddr, theWrites.front().thePC,
                 theWrites.front().thePriv, theWrites.front().theOpcode);
      theWrites.pop_front();
    }
    while (! theRMWs.empty() ) {
      recvRMW( theRMWs.front().theNode, theRMWs.front().theAddr, theRMWs.front().thePC,
               theRMWs.front().thePriv, theRMWs.front().theOpcode);
      theRMWs.pop_front();
    }
    while (! theReads.empty() ) {
      recvRead( theReads.front().theNode, theReads.front().theAddr, theReads.front().thePC,
                theReads.front().thePriv, theReads.front().theOpcode); // BTG - the last arg was theWrites ??
      theReads.pop_front();
    }
    if (theFlexus->cycleCount() > theLastSwitch + 50000000) {
       if (cfg.TraceEnabled) flushFiles();
       theLastSwitch = theFlexus->cycleCount();
    }
  }

private:
  //--- Base functions for maintaining state -----------------------------------
  void recvRead(int node, MemoryAddress addr, MemoryAddress pc, bool priv, unsigned long opcode) {
    DBG_(VVerb, ( << "got read from node " << node << " for addr " << addr ) );

    ++statReads;
    if (priv) ++statOSReads;

    if (cfg.TrackUniqueAddress) {
      statUniqueAddresses << addr;
      if (priv) {
        statUniqueOSAddresses << addr;
      } else {
        statUniqueUserAddresses << addr;
      }
    }

    read(node, addr, pc, currentTime(), priv);

    // grab the current state of this block
    BlockState_p block = myStateTable.find(addr);
    if(block) {
      DBG_(VVerb, ( << "retrieved block state" ) );

      if(block->state == StateShared) {
        DBG_(VVerb, ( << "state shared" ) );

        if(!block->isSharer(node)) {
          DBG_(VVerb, ( << "new sharer" ) );

          // new sharer - record first access to this block by this node
          consumption(addr, pc, node, currentTime(), block->producer, block->time, priv);
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

          // record the last store time of this address to file
          production(block->producer, addr, block->pc, block->time, block->priv);
          // transition to shared and record this first read time
          block->state = StateShared;
          block->newShareList(node, block->producer);
          consumption(addr, pc, node, currentTime(), block->producer, block->time, priv);
        }
      }
    }
    else {
      DBG_(VVerb, ( << "no existing block state" ) );

      ++statColdReads;

      // no current state for this block
      block = new BlockState(node);
      myStateTable.addEntry(addr, block);
      // record this first read time
      consumption(addr, pc, node, currentTime(), block->producer, block->time, priv);
    }
    DBG_(VVerb, ( << "done read" ) );
  }

  void recvWrite(int node, MemoryAddress addr, MemoryAddress pc, bool priv, unsigned long opcode) {
    DBG_(VVerb, ( << "got write from node " << node << " for addr " << addr ) );

    ++statWrites;
    if (priv) ++statOSWrites;

    if (cfg.TrackUniqueAddress) {
      statUniqueAddresses << addr;
      if (priv) {
        statUniqueOSAddresses << addr;
      } else {
        statUniqueUserAddresses << addr;
      }
    }

    // the current node gets a write
    store(node, addr, pc, currentTime(), priv);

    // record the opcode if interesting
    //if(opcode != 0) {
      //doOpcode(node, addr, pc, currentTime(), priv, opcode);
    //}

    recvExclusive(node, addr, pc, priv, opcode);

    DBG_(VVerb, ( << "done write" ) );
  }

  void recvRMW(int node, MemoryAddress addr, MemoryAddress pc, bool priv, unsigned long opcode) {
    DBG_(VVerb, ( << "got RMW from node " << node << " for addr " << addr ) );

    ++statRMWs;
    if (priv) ++statOSRMWs;

    if (cfg.TrackUniqueAddress) {
      statUniqueAddresses << addr;
      if (priv) {
        statUniqueOSAddresses << addr;
      } else {
        statUniqueUserAddresses << addr;
      }
    }

    // the current node gets an RMW
    rmw(node, addr, pc, currentTime(), priv);

    // record the opcode if interesting
    if(opcode != 0) {
      //doOpcode(node, addr, pc, currentTime(), priv, opcode);
    }

    recvExclusive(node, addr, pc, priv, opcode);

    DBG_(VVerb, ( << "done rmw" ) );
  }

  void recvExclusive(int node, MemoryAddress addr, MemoryAddress pc, bool priv, unsigned long opcode)
  {
    // grab the current state of this block
    BlockState_p block = myStateTable.find(addr);
    if(block) {
      DBG_(VVerb, ( << "retrieved block state" ) );

      if(block->state == StateShared) {
        DBG_(VVerb, ( << "state shared" ) );

        // This access is an upgrade
        upgrade(node, addr, pc, currentTime(), priv);

        // transition the block to exclusive
        block->state = StateExclusive;
        block->producer = node;
        block->time = currentTime();
        block->pc = pc;
        block->priv = priv;
      }
      else {  // StateExclusive
        DBG_(VVerb, ( << "state exclusive" ) );

        if(node == block->producer) {
          DBG_(VVerb, ( << "same producer" ) );

          // same node - just update production time
          block->time = currentTime();
          block->pc = pc;
          block->priv = priv;
        }
        else {
          DBG_(VVerb, ( << "new producer" ) );

          //This access is an upgrade
          upgrade(node, addr, pc, currentTime(), priv);

          // different node - record the last production for that node
          production(block->producer, addr, block->pc, block->time, block->priv);
          // transition to new producer
          block->producer = node;
          block->time = currentTime();
          block->pc = pc;
          block->priv = priv;
        }
      }
    }
    else {
      DBG_(VVerb, ( << "no existing block state" ) );

      ++statColdWrites; // ambiguous -- could be RMW

      // no current state for this block
      block = new BlockState(node, currentTime(), pc, priv);
      myStateTable.addEntry(addr, block);
    }
  }


  //--- Functions for SORD trace generation ------------------------------------
  void flushFiles() {
    DBG_(Dev, Comp(*this) ( << "Switching all output files" ) );
    Productions.flush();
    Consumptions.flush();
    Upgrades.flush();
    //Reads.flush();
    //Stores.flush();
    RMWs.flush();
    //Opcodes.flush();
  }

  void production(int producer, MemoryAddress address, MemoryAddress pc, Time time, bool priv) {

    DBG_(VVerb, (  << "production: node=" << producer
                  << " address=" << address
                  << " time=" << time
               ) );

    ++statProductions;
    if (priv) ++ statOSProductions;


    if (cfg.TrackUniqueAddress) {
      statUniqueSharedAddresses << address;
      if (priv) {
        statUniqueSharedOSAddresses << address;
      } else {
        statUniqueSharedUserAddresses << address;
      }
    }

    if (cfg.TraceEnabled && !disableTracing) {

      int offset = 0;
      int len;

      long long addr = address;
      len = sizeof(long long);
      memcpy(theBuffer+offset, &addr, len);
      offset += len;

      addr = pc;
      len = sizeof(long long);
      memcpy(theBuffer+offset, &addr, len);
      offset += len;

      len = sizeof(Time);
      memcpy(theBuffer+offset, &time, len);
      offset += len;

      char priv_c = priv;
      len = sizeof(char);
      memcpy(theBuffer+offset, &priv_c, len);
      offset += len;

      Productions.writeFile(theBuffer, offset, producer);
    }
  }

  void consumption(MemoryAddress address, MemoryAddress pc, int consumer, Time consumptionTime,
                   int producer, Time productionTime, bool priv) {

    DBG_(VVerb, (  << "consumption: address=" << address
                  << " consumer=" << consumer
                  << " consumption time=" << consumptionTime
                  << " producer=" << producer
                  << " production time=" << productionTime
                ) );

    ++statConsumptions;
    if (priv) ++ statOSConsumptions;

    if (producer == -1) {
      ++statColdConsumptions;
      return;
    }

    if (cfg.TrackUniqueAddress) {
      statSharedAddressInstances << std::make_pair(address, 1);
    }

    if (cfg.TraceEnabled && !disableTracing) {
      int offset = 0;
      int len;

      long long addr = address;
      len = sizeof(long long);
      memcpy(theBuffer+offset, &addr, len);
      offset += len;

      addr = pc;
      len = sizeof(long long);
      memcpy(theBuffer+offset, &addr, len);
      offset += len;

      len = sizeof(Time);
      memcpy(theBuffer+offset, &consumptionTime, len);
      offset += len;

      char prod_c = producer;
      len = sizeof(char);
      memcpy(theBuffer+offset, &prod_c, len);
      offset += len;

      len = sizeof(Time);
      memcpy(theBuffer+offset, &productionTime, len);
      offset += len;

      char priv_c = priv;
      len = sizeof(char);
      memcpy(theBuffer+offset, &priv_c, len);
      offset += len;

      Consumptions.writeFile(theBuffer, offset, consumer);
    }
  }

  void read(int producer, MemoryAddress address, MemoryAddress pc, Time time, bool priv) {
/*
    DBG_(VVerb, (  << "read: node=" << producer
                  << " address=" << address
                  << " pc=" << pc
                  << " time=" << time
                  << " priv=" << priv
               ) );

    int offset = 0;
    int len;

    if (cfg.TraceEnabled && !disableTracing) {

      long long addr = address;
      len = sizeof(long long);
      memcpy(theBuffer+offset, &addr, len);
      offset += len;

      addr = pc;
      len = sizeof(long long);
      memcpy(theBuffer+offset, &addr, len);
      offset += len;

      len = sizeof(Time);
      memcpy(theBuffer+offset, &time, len);
      offset += len;

      char priv_c = priv;
      len = sizeof(char);
      memcpy(theBuffer+offset, &priv_c, len);
      offset += len;

      Reads.writeFile(theBuffer, offset, producer);
    }
*/
  }

  void upgrade(int producer, MemoryAddress address, MemoryAddress pc, Time time, bool priv) {

    DBG_(VVerb, (  << "upgrade: node=" << producer
                  << " address=" << address
                  << " time=" << time
               ) );

    int offset = 0;
    int len;

    if (cfg.TraceEnabled) {

      long long addr = address;
      len = sizeof(long long);
      memcpy(theBuffer+offset, &addr, len);
      offset += len;

      addr = pc;
      len = sizeof(long long);
      memcpy(theBuffer+offset, &addr, len);
      offset += len;

      len = sizeof(Time);
      memcpy(theBuffer+offset, &time, len);
      offset += len;

      char priv_c = priv;
      len = sizeof(char);
      memcpy(theBuffer+offset, &priv_c, len);
      offset += len;

      Upgrades.writeFile(theBuffer, offset, producer);
    }
  }

  void store(int producer, MemoryAddress address, MemoryAddress pc, Time time, bool priv) {
    //Stores tracing disabled
/*
    DBG_(VVerb, (  << "store: node=" << producer
                  << " address=" << address
                  << " time=" << time
               ) );

    int offset = 0;
    int len;

    if (cfg.TraceEnabled && !disableTracing) {

      long long addr = address;
      len = sizeof(long long);
      memcpy(theBuffer+offset, &addr, len);
      offset += len;

      addr = pc;
      len = sizeof(long long);
      memcpy(theBuffer+offset, &addr, len);
      offset += len;

      len = sizeof(Time);
      memcpy(theBuffer+offset, &time, len);
      offset += len;

      char priv_c = priv;
      len = sizeof(char);
      memcpy(theBuffer+offset, &priv_c, len);
      offset += len;

      Stores.writeFile(theBuffer, offset, producer);
    }
*/
  }

  void rmw(int producer, MemoryAddress address, MemoryAddress pc, Time time, bool priv) {
    DBG_(VVerb, (  << "rmw: node=" << producer
                  << " address=" << address
                  << " pc=" << pc
                  << " time=" << time
                  << " priv=" << priv
               ) );

    int offset = 0;
    int len;

    if (cfg.TraceEnabled && !disableTracing) {

      long long addr = address;
      len = sizeof(long long);
      memcpy(theBuffer+offset, &addr, len);
      offset += len;

      addr = pc;
      len = sizeof(long long);
      memcpy(theBuffer+offset, &addr, len);
      offset += len;

      len = sizeof(Time);
      memcpy(theBuffer+offset, &time, len);
      offset += len;

      char priv_c = priv;
      len = sizeof(char);
      memcpy(theBuffer+offset, &priv_c, len);
      offset += len;

      RMWs.writeFile(theBuffer, offset, producer);
    }
  }

  void doOpcode(int node, MemoryAddress address, MemoryAddress pc, Time time, bool priv, unsigned long opcode) {
    //Opcode tracing disabled
/*
    DBG_(VVerb, (  << "opcode: node=" << node
                  << " address=" << address
                  << " time=" << time
                  << " opcode=" << opcode
               ) );

    int offset = 0;
    int len;

    if (cfg.TraceEnabled) {

      long long addr = address;
      len = sizeof(long long);
      memcpy(theBuffer+offset, &addr, len);
      offset += len;

      addr = pc;
      len = sizeof(long long);
      memcpy(theBuffer+offset, &addr, len);
      offset += len;

      len = sizeof(Time);
      memcpy(theBuffer+offset, &time, len);
      offset += len;

      char priv_c = priv;
      len = sizeof(char);
      memcpy(theBuffer+offset, &priv_c, len);
      offset += len;

      len = sizeof(unsigned long);
      memcpy(theBuffer+offset, &opcode, len);
      offset += len;

      Opcodes.writeFile(theBuffer, offset, node);
    }
*/
  }

  //--- Utility functions ------------------------------------------------------
  MemoryAddress blockAddress(MemoryAddress addr) {
    return MemoryAddress( addr & ~(cfg.BlockSize-1) );
  }

  Time currentTime() {
    return theFlexus->cycleCount();
  }

};

} //End Namespace nEnhancedSordTraceMemory

FLEXUS_COMPONENT_INSTANTIATOR( EnhancedSordTraceMemory, nEnhancedSordTraceMemory );
FLEXUS_PORT_ARRAY_WIDTH( EnhancedSordTraceMemory, ToNode) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }
FLEXUS_PORT_ARRAY_WIDTH( EnhancedSordTraceMemory, FromNode) { return Flexus::Core::ComponentManager::getComponentManager().systemWidth(); }


#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT EnhancedSordTraceMemory

  #define DBG_Reset
  #include DBG_Control()
