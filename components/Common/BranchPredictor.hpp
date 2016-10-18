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

#ifndef FLEXUS_FETCHADDRESSGENERATE_BRANCHPREDICTOR_HPP_INCLUDED
#define FLEXUS_FETCHADDRESSGENERATE_BRANCHPREDICTOR_HPP_INCLUDED

#include <iostream>
#include <core/target.hpp>
#include <core/types.hpp>

#include <components/uFetch/uFetchTypes.hpp>


namespace Flexus {
namespace SharedTypes {

struct BranchPredictor {
  static BranchPredictor * combining(std::string const & aName, unsigned int anIndex);
  virtual bool isBranch( VirtualMemoryAddress anAddress) = 0;
  virtual void feedback( BranchFeedback const & aFeedback) = 0;
  virtual VirtualMemoryAddress predict( FetchAddr & aFetchAddr) = 0;
  virtual ~BranchPredictor() {}
  virtual void loadState( std::string const & aDirName) = 0;
  virtual void saveState( std::string const & aDirName) const = 0;
};


struct FastBranchPredictor {
  static FastBranchPredictor * combining(std::string const & aName, unsigned int anIndex);
  //virtual void feedback( BranchFeedback const & aFeedback) = 0;
  virtual void predict( VirtualMemoryAddress anAddress, BPredState & aBPState) = 0;
  virtual void feedback( VirtualMemoryAddress anAddress,  eBranchType anActualType, eDirection anActualDirection, VirtualMemoryAddress anActualAddress, BPredState & aBPState) = 0;
  virtual ~FastBranchPredictor() {}
  virtual void loadState( std::string const & aDirName) = 0;
  virtual void saveState( std::string const & aDirName) const = 0;
};


} //SharedTypes
} //Flexus


#endif //FLEXUS_FETCHADDRESSGENERATE_BRANCHPREDICTOR_HPP_INCLUDED
