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

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace l = boost::lambda;

#include "common.hpp"
#include "instance.hpp"

void InstanceManager::finalizeStats() {
}

InstanceManager::InstanceManager()
 : theUniqueNames ( "instances.UniqueNames" )
 , theInstances   ( "instances.Instances"   )
 {
}

std::pair<long, tID> InstanceManager::getInstance(tEventType anEvent, tID aNode, tAddress anAddress, tMRPVector aBitVector) {
   InstanceMap::iterator iter = theInstanceMap.find(anAddress);

   if (iter == theInstanceMap.end()) {
      theUniqueNames++;
      if (anEvent != eConsumption) {
        theInstanceMap[anAddress] = std::make_pair(0, aNode);
      } else {
        theInstanceMap[anAddress] = std::make_pair(0, -1);
      }
      return theInstanceMap[anAddress];
   } else {
      //If the event is an upgrade, increment and return
      if (anEvent == eUpgrade) {
        theInstances++;
        ++(iter->second.first);
        iter->second.second = aNode;

        return iter->second;
      } else {
        //Sanity check on producer node id's
        if (anEvent == eProduction && aBitVector != 0 && aNode != iter->second.second) {
          DBG_( Dev, ( << "Terminate Condition: " << anEvent << " " << aNode << " @" << std::hex << anAddress << std::dec << " " << iter->second.second));
          DBG_Assert(false);
        }
        //Else return current instance number
        return iter->second;
      }
   }
}
