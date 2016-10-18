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

#include "profile.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>

#include <boost/lexical_cast.hpp>

#define __STDC_CONSTANT_MACROS
#include <boost/date_time/posix_time/posix_time.hpp>

namespace nProfile {

ProfileManager * theProfileManager = 0;
Profiler * theProfileTOS = 0;

ProfileManager * ProfileManager::profileManager() {
  if (theProfileManager == 0) {
    theProfileManager = new ProfileManager();
  }
  return theProfileManager;
}

bool sortTotalTime( Profiler * left, Profiler * right) {
  return left->totalTime() > right->totalTime();
}

bool sortSelfTime( Profiler * left, Profiler * right) {
  return left->selfTime() > right->selfTime();
}

std::string rightmost( std::string const & aString, unsigned int N) {
    std::string retval(aString);
    if (retval.length() > N) {
      retval.erase(0, retval.length() - N);
    } else if (retval.length() < N) {
      retval.append(N - retval.length(), ' ');
    }
    return retval;
}

std::string leftmost( std::string const & aString, unsigned int N) {
    std::string retval(aString.substr(0, N));
    if (retval.length() < N) {
      retval.append(N - retval.length(), ' ');
    }
    return retval;
}

boost::posix_time::ptime last_reset(boost::posix_time::second_clock::local_time());

void ProfileManager::reset() {
  std::vector< Profiler *>::iterator iter, end;
  for (iter = theProfilers.begin(), end = theProfilers.end(); iter != end; ++iter) {
    (*iter)->reset();
  }

  last_reset = boost::posix_time::second_clock::local_time();
  theStartTime = rdtsc();
}


void ProfileManager::report(std::ostream & out) {
  std::vector< Profiler *>::iterator iter, end;
  int i = 0;

  float program_time = programTime() / 100 ;

  out << "Ticks Since Reset: " << program_time << std::endl << std::endl;
  boost::posix_time::ptime now(boost::posix_time::second_clock::local_time());
  out << "Wall Clock Since Reset: " << boost::posix_time::to_simple_string( boost::posix_time::time_period(last_reset, now).length() ) << std::endl << std::endl;

  out << "Worst sources, by Self Time \n";
  std::sort( theProfilers.begin(), theProfilers.end(), &sortSelfTime);

  out << std::setiosflags( std::ios::fixed) << std::setprecision(2);
  out << "File                          " << " ";
  out << "Function/Name                 " << " ";
  out << "Self Time  " << "   ";
  out << "% ";
  out << "\n";
  for (iter = theProfilers.begin(), end = theProfilers.end(), i = 0; iter != end && i < 200; ++iter, ++i) {
    std::string file_line = (*iter)->file() + ":" + boost::lexical_cast<std::string>((*iter)->line());
    out << rightmost(file_line, 30) << " ";
    out << leftmost((*iter)->name(), 30) << " ";
    out << std::setiosflags(std::ios::right) << std::setw(11) << (*iter)->selfTime() << "   ";
    out << std::setw(5) << static_cast<float>((*iter)->selfTime()) / program_time << "% ";
    out << std::endl;
  }
  out << "\n";

  out << "Worst sources, sorted by Total Time \n";
  std::sort( theProfilers.begin(), theProfilers.end(), &sortTotalTime);

  out << std::setiosflags( std::ios::fixed) << std::setprecision(2);
  out << "File                          " << " ";
  out << "Function/Name                 " << " ";
  out << "Total Time " << "   ";
  out << "% ";
  out << "\n";
  for (iter = theProfilers.begin(), end = theProfilers.end(), i = 0; iter != end && i < 200; ++iter, ++i) {
    std::string file_line = (*iter)->file() + ":" + boost::lexical_cast<std::string>((*iter)->line());
    out << rightmost(file_line, 30) << " ";
    out << leftmost((*iter)->name(), 30) << " ";
    out << std::setiosflags(std::ios::right) << std::setw(11) << (*iter)->totalTime() << "   ";
    out << std::setw(5) << static_cast<float>((*iter)->totalTime()) / program_time << "% ";
    out << std::endl;
  }
  out << "\n";

}

} //nProfile
