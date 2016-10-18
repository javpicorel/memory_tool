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

#include <core/target.hpp>
#include <core/types.hpp>
#include <core/debug/debug.hpp>

#include <core/boost_extensions/lexical_cast.hpp>
#include <core/simics/configuration_api.hpp>

#include <core/flexus.hpp>

#include "Tracer.hpp"

#include <zlib.h>

namespace nConsort  {


using namespace Flexus::SharedTypes;
using namespace Flexus::Core;
using Flexus::SharedTypes::PhysicalMemoryAddress;

typedef PhysicalMemoryAddress MemoryAddress;
typedef unsigned long long Time;

struct TraceManagerImpl : public TraceManager {

  static const int kNumProcs = 16;
  static const int kBlockSize = 64;

  // output files
  struct OutfileRecord {
    std::string theBaseName;
    gzFile * theFiles;
    int      theFileNo[kNumProcs];
    long     theFileLengths[kNumProcs];

    static const int K = 1024;
    static const int M = 1024 * K;
    static const int kMaxFileSize = 512 * M;

    OutfileRecord(char * aName)
      : theBaseName(aName)
      , theFiles(0)
    {
      int ii;
      for(ii = 0; ii < kNumProcs; ii++) {
        theFileNo[ii] = 0;
        theFileLengths[ii] = 0;
      }
    }
    ~OutfileRecord() {
      if(theFiles != 0) {
        int ii;
        for(ii = 0; ii < kNumProcs; ii++) {
          gzclose(theFiles[ii]);
        }
        delete [] theFiles;
      }
    }

    void init() {
      int ii;
      theFiles = new gzFile[kNumProcs];
      for(ii = 0; ii < kNumProcs; ii++) {
        theFiles[ii] = gzopen( nextFile(ii).c_str(), "w" );
      }
    }
    void flush() {
      int ii;
      for(ii = 0; ii < kNumProcs; ii++) {
        switchFile(ii);
      }
    }
    std::string nextFile(int aCpu) {
      return ( theBaseName + boost::lexical_cast<std::string>(aCpu) + ".sordtrace64." + boost::lexical_cast<std::string>(theFileNo[aCpu]++) + ".gz" );
    }
    void switchFile(int aCpu) {
      DBG_Assert(theFiles);
      gzclose(theFiles[aCpu]);
      std::string new_file = nextFile(aCpu);
      theFiles[aCpu] = gzopen( new_file.c_str(), "w" );
      theFileLengths[aCpu] = 0;
      DBG_(Dev, ( << "switched to new trace file: " << new_file ) );
    }
    void writeFile(unsigned char * aBuffer, int aLength, int aCpu) {
      DBG_Assert(theFiles);
      gzwrite(theFiles[aCpu], aBuffer, aLength);

      theFileLengths[aCpu] += aLength;
      if (theFileLengths[aCpu] > kMaxFileSize) {
        switchFile(aCpu);
      }
    }
  };

  OutfileRecord * Upgrades;
  OutfileRecord * Consumptions;

  // file I/O buffer
  unsigned char theBuffer[64];

  TraceManagerImpl(Flexus::Simics::API::conf_object_t * /*ignored*/ )
    : Upgrades(0)
    , Consumptions(0)
  { }

  void start() {
    DBG_(Dev, ( << "Begin tracing upgrades and consumptions") );
    Upgrades = new OutfileRecord("upgrade");
    Consumptions = new OutfileRecord("consumer");
    Upgrades->init();
    Consumptions->init();
  }

  void finish() {
    DBG_(Dev, ( << "Stop tracing upgrades and consumptions") );
    delete Upgrades;
    Upgrades = 0;
    delete Consumptions;
    Consumptions = 0;
  }

  void upgrade(unsigned int producer, MemoryAddress address) {
    if (! Upgrades) {
      return;
    }

    MemoryAddress pc(0);
    bool priv(false);
    Time time = currentTime();


    DBG_(VVerb, (  << "upgrade: node=" << producer
                  << " address=" << address
                  << " time=" << time
               ) );

    int offset = 0;
    int len;

    long long addr = address & ~63LL;
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

    Upgrades->writeFile(theBuffer, offset, producer);
  }

  void consumption(unsigned int consumer, MemoryAddress address) {
    if (! Consumptions) {
      return;
    }

    MemoryAddress pc(0);
    int producer(0);
    unsigned int productionTime(0);
    bool priv(false);
    Time consumptionTime = currentTime();

    DBG_(VVerb, (  << "consumption: address=" << address
                  << " consumer=" << consumer
                  << " consumption time=" << consumptionTime
                  << " producer=" << producer
                  << " production time=" << productionTime
                ) );

    int offset = 0;
    int len;

    long long addr = address & ~63LL;
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

    Consumptions->writeFile(theBuffer, offset, consumer);

  }

  //--- Utility functions ------------------------------------------------------
  MemoryAddress blockAddress(MemoryAddress addr) {
    return MemoryAddress( addr & ~(kBlockSize-1) );
  }

  Time currentTime() {
    return theFlexus->cycleCount();
  }
};


class TraceManager_SimicsObject : public Flexus::Simics::AddInObject <TraceManagerImpl> {
    typedef Flexus::Simics::AddInObject<TraceManagerImpl> base;
   public:
    static const Flexus::Simics::Persistence  class_persistence = Flexus::Simics::Session;
    //These constants are defined in Simics/simics.cpp
    static std::string className() { return "TraceManager"; }
    static std::string classDescription() { return "TraceManager object"; }

    TraceManager_SimicsObject() : base() { }
    TraceManager_SimicsObject(Flexus::Simics::API::conf_object_t * aSimicsObject) : base(aSimicsObject) {}
    TraceManager_SimicsObject(TraceManagerImpl * anImpl) : base(anImpl) {}

    template <class Class>
    static void defineClass(Class & aClass) {

      aClass.addCommand
        ( & TraceManagerImpl::start
        , "start"
        , "begin tracing"
        );

      aClass.addCommand
        ( & TraceManagerImpl::finish
        , "finish"
        , "stop tracing"
        );

    }

};

Flexus::Simics::Factory<TraceManager_SimicsObject> theTraceManagerFactory;

TraceManager_SimicsObject theRealTraceManager;

struct StaticInit {
  StaticInit() {
    std::cerr << "Creating MRC trace manager\n";
    theRealTraceManager = theTraceManagerFactory.create("trace-mgr");
    theTraceManager = & theRealTraceManager;
  }
} StaticInit_;

TraceManager * theTraceManager = 0;



} //End Namespace nConsort

