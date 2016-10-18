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


#define HACK_WIDTH 16

typedef unsigned long Time;

struct ProdEntry {
  long long address;
  Time time;
};

struct ConsEntry {
  long long address;
  Time consTime;
  int producer;
  Time prodTime;
};

int main() {
  char str[32];
  ProdEntry production;
  ConsEntry consumption;
  int ii;
  FILE *fp;

  for(ii = 0; ii < HACK_WIDTH; ii++) {
    sprintf(str, "producer%d.sordtrace", ii);
    fp = fopen(str, "r");
    while( fread(&production, sizeof(ProdEntry), 1, fp) > 0 ) {
      std::cout << "production: node=" << ii
                << " address=" << production.address
                << " time=" << production.time
                << std::endl;
    }
    fclose(fp);
  }

  for(ii = 0; ii < HACK_WIDTH; ii++) {
    sprintf(str, "consumer%d.sordtrace", ii);
    fp = fopen(str, "r");
    while( fread(&consumption, sizeof(ConsEntry), 1, fp) > 0 ) {
      std::cout << "consumption: address=" << consumption.address
                << " consumer=" << ii
                << " consumption time=" << consumption.consTime
                << " producer=" << consumption.producer
                << " production time=" << consumption.prodTime
                << std::endl;
    }
    fclose(fp);
  }

  return 0;
}
