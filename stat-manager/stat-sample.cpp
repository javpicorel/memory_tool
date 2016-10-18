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
#include <iostream>
#include <sstream>
#include <fstream>
#include <deque>
#include <cstdlib>

#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>

namespace ll = boost::lambda;
using ll::_1;
using ll::_2;
using ll::_3;


#include <core/stats.hpp>

namespace Flexus { namespace Core {
  void Break() {}
} }

using namespace Flexus::Stat;

std::deque< boost::function< void () > > theCommands;


void usage() {
  std::cout << "Usage: stat-sample <output stat file> <input stat files>* " << std::endl;
}

void help(std::string const & command) {
  usage();
}

void loadDatabaseFile( std::istream & anIstream, std::string const & aPrefix, bool aFirst) {
  if(aFirst) {
    getStatManager()->load(anIstream);
  } else {
    getStatManager()->loadMore(anIstream, aPrefix);
  }
}

void loadDatabase( std::string const & aName, std::string const & aPrefix, bool aFirst) {
  size_t loc = aName.rfind(".gz");

  try {
    std::ifstream in(aName.c_str(), std::ios::binary);
    if(in) {
      if(loc == std::string::npos) {
        loadDatabaseFile(in, aPrefix, aFirst);
        in.close();
      } else {
        boost::iostreams::filtering_istream inGzip;
        inGzip.push(boost::iostreams::gzip_decompressor());
        inGzip.push(in);
        loadDatabaseFile(inGzip, aPrefix, aFirst);
        inGzip.reset();
      }
    }
    else {
      std::cout << "Cannot open stats database " << aName << std::endl;
      std::exit(-1);
    }
  } catch (...) {
    std::cout << "Unable to load stats from database " << aName << std::endl;
    std::exit(-1);
  }
}


void reduceSum() {
  getStatManager()->reduce(eSum, ".*selection", "sum", std::cout);
}

void reduceAvg() {
  getStatManager()->reduce(eAverage, ".*selection", "avg", std::cout);
}

void reduceStdev(std::string const & aMsmt) {
  getStatManager()->reduce(eStdDev, ".*selection", aMsmt, std::cout);
}

void reduceCount() {
  getStatManager()->reduce(eCount, ".*selection", "count", std::cout);
}

void reduceNodes() {
  getStatManager()->reduceNodes(".*selection");
}


void save(std::string const & aFilename, std::string const & aMeasurementRestriction) {
  getStatManager()->saveMeasurements(aMeasurementRestriction, aFilename);
}

std::string region_string;


void processCmdLine(int aCount, char ** anArgList) {

  if (aCount < 2) {
    usage();
    std::exit(-1);
  }

  std::string output_file = anArgList[1];
  std::string first_file = anArgList[2];

  theCommands.push_back(  ll::bind( &loadDatabase, first_file, std::string(""), true ) );
  for (int i = 3; i < aCount; ++i) {
    std::stringstream prefix;
    prefix << std::setw(2) << std::setfill('0') << (i-1) << '-';
    theCommands.push_back(  ll::bind( &loadDatabase, std::string(anArgList[i]), prefix.str(), false) );
  }
  theCommands.push_back(  ll::bind( &reduceSum ) );
  theCommands.push_back(  ll::bind( &reduceAvg ) );
  theCommands.push_back(  ll::bind( &reduceStdev, "pernode-stdev"  ) );
  theCommands.push_back(  ll::bind( &reduceCount) );
  theCommands.push_back(  ll::bind( &reduceNodes ) );
  theCommands.push_back(  ll::bind( &reduceStdev, "stdev" ) );
  theCommands.push_back(  ll::bind( &save, output_file, "(sum|count|avg|stdev|pernode-stdev)" ) );

}


int main(int argc, char ** argv) {

  getStatManager()->initialize();

  processCmdLine(argc, argv);

  while (! theCommands.empty() ) {
    theCommands.front()();
    theCommands.pop_front();
  }

}
