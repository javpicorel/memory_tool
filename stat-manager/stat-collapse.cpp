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
  std::cout << "Usage: stat-collapse <start msmt cycle> <end msmt cycle> <input stat file> <output stat file>" << std::endl;
}

void help(std::string const & command) {
  usage();
}

void loadDatabase( std::string const & aName) {
  size_t loc = aName.rfind(".gz");

  try {
    std::ifstream in(aName.c_str(), std::ios::binary);
    if(in) {
      if(loc == std::string::npos) {
        getStatManager()->load(in);
        in.close();
      } else {
        boost::iostreams::filtering_istream inGzip;
        inGzip.push(boost::iostreams::gzip_decompressor());
        inGzip.push(in);
        getStatManager()->load(inGzip);
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


void reduceSum(std::string const & aMeasurementRestriction) {
  getStatManager()->reduce(eSum,aMeasurementRestriction, "selection", std::cout);
}

void save(std::string const & aFilename, std::string const & aMeasurementRestriction) {
  getStatManager()->saveMeasurements(aMeasurementRestriction, aFilename);
}

std::string region_string;



void computeRegions(long long aStartInsn, long long anEndInsn) {
  std::stringstream result;
  getStatManager()->format("Region 000", "{sys-cycles}", result);
  std::string out = result.str();
  //Strip trailing cr
  if (out[out.length()-1] == '\n') {
    out = out.substr(0, out.length()-1);
  }
  int region = boost::lexical_cast<long long>(out);
  int start_region = aStartInsn / region;
  int end_region = anEndInsn / region;
  std::cout << "Start region: " << start_region << std::endl;
  std::cout << "End region: " << end_region << std::endl;
  std::stringstream region_strstream;
  region_strstream << "Region (";
  region_strstream << std::setw(3) << std::setfill('0') << start_region;
  for ( ++ start_region ; start_region <= end_region ; ++ start_region) {
    region_strstream << '|' << std::setw(3) << std::setfill('0') << start_region;
  }
  region_strstream << ')';
  region_string = region_strstream.str();

  StatAnnotation * regions = new StatAnnotation("reduce-SelectedRegions");
  (*regions) << region_string;
}

void processCmdLine(int aCount, char ** anArgList) {

  if (aCount < 4) {
    usage();
    std::exit(-1);
  }

  long long start_insn = boost::lexical_cast<long long>( anArgList[1] );
  long long end_insn   = boost::lexical_cast<long long>( anArgList[2] );
  if (start_insn >= end_insn) {
    std::cout << "Start instruction must exceed end instruction\n";
    std::exit(-1);
  }
  std::string input_file = anArgList[3];
  std::string output_file = anArgList[4];

  theCommands.push_back(  ll::bind( &loadDatabase, input_file ) );
  theCommands.push_back(  ll::bind( &computeRegions, start_insn, end_insn ) );
  theCommands.push_back(  ll::bind( &reduceSum, ll::var(region_string) ) );
  theCommands.push_back(  ll::bind( &save, output_file, "selection" ) );

}


int main(int argc, char ** argv) {

  getStatManager()->initialize();

  processCmdLine(argc, argv);

  while (! theCommands.empty() ) {
    theCommands.front()();
    theCommands.pop_front();
  }

}
