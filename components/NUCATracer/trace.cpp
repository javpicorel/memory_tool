// DO-NOT-REMOVE begin-copyright-block 
//                                     
// Redistributions of any form whatsoever must retain and/or include the     
// following acknowledgment, notices and disclaimer:                         
//                                                                           
// This product includes software developed by Carnegie Mellon University.   
//                                                                           
// Copyright 2006 by Eric Chung, Brian Gold, Nikos Hardavellas, Jangwoo Kim, 
// Ippokratis Pandis, Minglong Shao, Jared Smolens, Stephen Somogyi,         
// Tom Wenisch, Anastassia Ailamaki, Babak Falsafi and James C. Hoe for      
// the SimFlex Project, Computer Architecture Lab at Carnegie Mellon,        
// Carnegie Mellon University.                                               
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

#include <zlib.h>
#include <list>
#include <fstream>

#include "common.hpp"
#include "trace.hpp"

namespace trace {

struct FileRecord {
  std::string theBaseName;
  int theNumL2s;
  std::vector<gzFile> theFiles;
  std::vector<int>    theFileNo;
  std::vector<long>   theFileLengths;

  bool theReadOnlyFlag;

  static const int K = 1024;
  static const int M = 1024 * K;
  static const int kMaxFileSize = 512 * M;

  FileRecord(std::string const & aName, int aNumL2s, const bool aReadOnlyFlag)
    : theBaseName(aName)
    , theNumL2s(aNumL2s)
    , theFiles(0)
    , theReadOnlyFlag(aReadOnlyFlag)
  {
    theFileNo.resize(theNumL2s,0);
    theFileLengths.resize(theNumL2s,0);
  }
  ~FileRecord() {
    for(unsigned ii = 0; ii < theFiles.size(); ii++) {
      gzclose(theFiles[ii]);
    }
  }

  void init() {
    theFiles.resize(theNumL2s, 0);
    for(int ii = 0; ii < theNumL2s; ii++) {
      theFiles[ii] = gzopen( nextFile(ii).c_str(), (theReadOnlyFlag ? "r" : "w") );
    }
  }
  void flush() {
    for(int ii = 0; ii < theNumL2s; ii++) {
      switchFile(ii);
    }
  }
  std::string nextFile(int aCpu) {
    return ( theBaseName + boost::lexical_cast<std::string>(aCpu) + ".trace." + boost::lexical_cast<std::string>(theFileNo[aCpu]++) + ".gz" );
  }
  gzFile switchFile(int aCpu) {
    gzclose(theFiles[aCpu]);
    std::string new_file = nextFile(aCpu);
    theFiles[aCpu] = gzopen( new_file.c_str(), (theReadOnlyFlag ? "r" : "w") );
    theFileLengths[aCpu] = 0;
    DBG_(Dev, ( << "switched to new trace file: " << new_file ) );
    return theFiles[aCpu];
  }
  int readFile(unsigned char * aBuffer, int aLength, int aCpu) {
    int ret = gzread(theFiles[aCpu], aBuffer, aLength);
  
    if (ret == 0) {
      if (switchFile(aCpu) == NULL) {
        DBG_(Dev, ( << "no switch"));
        return 0;
      }
      ret = gzread(theFiles[aCpu], aBuffer, aLength);
      
        DBG_(Dev, ( << "switch"));
    }
    return ret;
  }
  void writeFile(unsigned char const * aBuffer, int aLength, int aCpu) {
    gzwrite(theFiles[aCpu], const_cast<unsigned char *>(aBuffer), aLength);

    theFileLengths[aCpu] += aLength;
    if (theFileLengths[aCpu] > kMaxFileSize) {
      switchFile(aCpu);
    }
  }
  std::string getName( int aCpu ) {
    return (theBaseName + boost::lexical_cast<std::string>(aCpu) + ".trace." + boost::lexical_cast<std::string>(theFileNo[aCpu]) + ".gz" );
  }
};

FileRecord * theL2Accesses = 0;
FileRecord * theOffChipAccesses = 0;

unsigned char theBuffer[256];


Flexus::Stat::StatCounter statL2Accesses("sys-trace-L2Accesses");
Flexus::Stat::StatCounter statOCAccesses("sys-trace-OffChipAccesses");


void initialize( std::string const & aPath, const bool aReadOnlyFlag ) {
  theL2Accesses= new FileRecord( aPath + "/L2_trace", 1, aReadOnlyFlag);
  theL2Accesses->init();
  theOffChipAccesses = new FileRecord( aPath + "/OC_trace", 1, aReadOnlyFlag);
  theOffChipAccesses->init();
}

void flushFiles() {
  DBG_(Dev, ( << "Switching all output files" ) );
  if (theL2Accesses) {
    theL2Accesses->flush();
  }
  if (theOffChipAccesses) {
    theOffChipAccesses->flush();
  }
}

void addL2Access(TraceData const & theRecord) {
  DBG_(Verb, ( << theRecord ));

  ++statL2Accesses;

  DBG_Assert( theRecord.theFillLevel <= Flexus::SharedTypes::eCore);
  DBG_Assert( theRecord.theFillType <= Flexus::SharedTypes::eNAW, ( << "C[" << theRecord.theNode << "] 0x" << std::hex << theRecord.theAddress
                  << " PC:" << theRecord.thePC << std::dec << ( theRecord.theOS ? "s " : "u " ) << theRecord.theFillLevel ));
  
  theL2Accesses->writeFile(reinterpret_cast<const unsigned char *>(&theRecord), sizeof(TraceData), 0);
}

void addOCAccess(TraceData const & theRecord) {
  DBG_(Verb, ( << theRecord ));

  ++statOCAccesses;

  DBG_Assert( theRecord.theFillLevel <= Flexus::SharedTypes::eCore);
  DBG_Assert( theRecord.theFillType <= Flexus::SharedTypes::eNAW, ( << "C[" << theRecord.theNode << "] 0x" << std::hex << theRecord.theAddress
                  << " PC:" << theRecord.thePC << std::dec << ( theRecord.theOS ? "s " : "u " ) << theRecord.theFillLevel ));
  
  theOffChipAccesses->writeFile(reinterpret_cast<const unsigned char *>(&theRecord), sizeof(TraceData), 0);
}

int getL2Access( TraceData * theRecord) {
  int ret = theL2Accesses->readFile(reinterpret_cast<unsigned char *>(theRecord), sizeof(TraceData), 0);

  if (ret != 0) {
    DBG_(Verb, ( << *theRecord ));

    ++statL2Accesses;

    DBG_Assert( theRecord->theFillLevel <= Flexus::SharedTypes::eCore, ( << *theRecord) );
    DBG_Assert( theRecord->theFillType <= Flexus::SharedTypes::eNAW, ( << "C[" << theRecord->theNode << "] 0x" << std::hex << theRecord->theAddress
                  << " PC:" << theRecord->thePC << std::dec << ( theRecord->theOS ? " s " : " u " ) << theRecord->theFillLevel ));
  }
  return ret;
}

int getOCAccess( TraceData * theRecord ) {
  int ret = theOffChipAccesses->readFile(reinterpret_cast<unsigned char *>(theRecord), sizeof(TraceData), 0);

  if (ret != 0) {
    DBG_(Verb, ( << *theRecord ));

    ++statOCAccesses;

    DBG_Assert( theRecord->theFillLevel <= Flexus::SharedTypes::eCore );
    DBG_Assert( theRecord->theFillType <= Flexus::SharedTypes::eNAW, ( << "C[" << theRecord->theNode << "] 0x" << std::hex << theRecord->theAddress
                  << " PC:" << theRecord->thePC << std::dec << ( theRecord->theOS ? "s " : "u " ) << theRecord->theFillLevel ));
  }
  return ret;
}

} //end namespace trace

