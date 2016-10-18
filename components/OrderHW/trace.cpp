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

#include <zlib.h>
#include <list>
#include <fstream>

#include "common.hpp"
#include "trace.hpp"

namespace trace {

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
    return ( theBaseName + boost::lexical_cast<std::string>(aCpu) + ".trace." + boost::lexical_cast<std::string>(theFileNo[aCpu]++) + ".gz" );
  }
  void switchFile(int aCpu) {
    gzclose(theFiles[aCpu]);
    std::string new_file = nextFile(aCpu);
    theFiles[aCpu] = gzopen( new_file.c_str(), "w" );
    theFileLengths[aCpu] = 0;
    DBG_(Dev, ( << "switched to new trace file: " << new_file ) );
  }
  void writeFile(unsigned char const * aBuffer, int aLength, int aCpu) {
    gzwrite(theFiles[aCpu], const_cast<unsigned char *>(aBuffer), aLength);

    theFileLengths[aCpu] += aLength;
    if (theFileLengths[aCpu] > kMaxFileSize) {
      switchFile(aCpu);
    }
  }
};

OutfileRecord * theReadMisses = 0;
OutfileRecord * theGenerations = 0;

unsigned char theBuffer[256];


StatCounter statMisses("sys-trace-Misses");
#ifdef ENABLE_UNIQUE_COUNTS
  StatUniqueCounter<long long> statUniqueAddresses("sys-trace-UniqueAddresses");
#endif //ENABLE_UNIQUE_COUNTS

StatCounter statGenerations("sys-trace-Generations");


void initialize( ) {
  DBG_(Dev, ( << "Creating output file" ) );
  theReadMisses = new OutfileRecord( "misses", 1);
  theReadMisses->init();
  #ifdef ENABLE_TRACE_SGPGENERATIONS
  theGenerations = new OutfileRecord( "sgpgen", 1);
  theGenerations->init();
  #endif //ENABLE_TRACE_SGPGENERATIONS
}

void flushFiles() {
  DBG_(Dev, ( << "Switching all output files" ) );
  if (theReadMisses) {
    theReadMisses->flush();
  }
  if (theGenerations) {
    theGenerations->flush();
  }
}


void addMiss(MissRecord const & theRecord) {

  DBG_(Verb, (  << "C[" << theRecord.aNode << "] 0x" << std::hex << theRecord.anAddress
                  << " PC:" << theRecord.aPC << std::dec << ( theRecord.aBits & kBITS_OS ? "s " : "u " )
                  << theRecord.aTSE << "(" << theRecord.aTSEStream << ") "
                  << theRecord.anSGP << "(" << theRecord.anSGPStream << ")"
               ) );

  ++statMisses;

  DBG_Assert( theRecord.aFillLevel <= Flexus::SharedTypes::eCore);
  DBG_Assert( theRecord.aFillType <= Flexus::SharedTypes::eNAW, ( << "C[" << theRecord.aNode << "] 0x" << std::hex << theRecord.anAddress
                  << " PC:" << theRecord.aPC << std::dec << ( theRecord.aBits & kBITS_OS ? "s " : "u " ) << theRecord.aFillLevel ));
  
  #ifdef ENABLE_UNIQUE_COUNTS
    statUniqueAddresses << theRecord.anAddress;
  #endif //ENABLE_UNIQUE_COUNTS


  #ifdef ENABLE_TRACE_OUTPUT
    theReadMisses->writeFile(reinterpret_cast<const unsigned char *>(&theRecord), sizeof(MissRecord), 0);
  #endif //ENABLE_TRACE_OUTPUT

}

void addGeneration(Generation const & theRecord) {

  DBG_(Verb, (  << "C[" << theRecord.aNode << "] " << theRecord.theId << " 0x" << std::hex << theRecord.anAddress
                  << " PC:" << theRecord.aPC << " |" << std::setw(8) << std::setfill('0') << theRecord.theVector << "|"
               ) );

  ++statGenerations;

  #ifdef ENABLE_TRACE_SGPGENERATIONS
    theGenerations->writeFile(reinterpret_cast<const unsigned char *>(&theRecord), sizeof(Generation), 0);
  #endif //ENABLE_TRACE_SGPGENERATIONS

}


} //end namespace trace

