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

#ifndef FLEXUS_TARGET
#error "Wiring.cpp must contain a flexus target section before the declaration section"
#endif

#ifdef FLEXUS__LAYOUT_COMPONENTS_DECLARED
#error "Wiring.cpp may contain only one declaration section"
#endif //FLEXUS__LAYOUT_COMPONENTS_DECLARED

#ifdef FLEXUS__LAYOUT_IN_SECTION
#error "Previous wiring.cpp section is missing the end of section #include"
#endif //FLEXUS__LAYOUT_IN_SECTION

#define FLEXUS__LAYOUT_IN_SECTION
#define FLEXUS__LAYOUT_DECLARATION_SECTION



#define FLEXUS_END_COMPONENT NIL

#define FLEXUS__COMPONENT_CHAIN_CURRENT   FLEXUS_BEGIN_COMPONENT
#define FLEXUS__COMPONENT_CHAIN_PREVIOUS  FLEXUS_END_COMPONENT

#define FLEXUS__PREVIOUS_COMP_DECL() 	BOOST_PP_CAT( FLEXUS__COMP_DECL_, FLEXUS__COMPONENT_CHAIN_PREVIOUS )
#define FLEXUS__COMP_DECL() 				  BOOST_PP_CAT( FLEXUS__COMP_DECL_, FLEXUS__COMPONENT_CHAIN_CURRENT )

#define FLEXUS__PREVIOUS_IMPLEMENTATION_LIST() 	BOOST_PP_CAT( FLEXUS__IMPLEMENTATION_LIST_, FLEXUS__COMPONENT_CHAIN_PREVIOUS )
#define FLEXUS__IMPLEMENTATION_LIST() 				  BOOST_PP_CAT( FLEXUS__IMPLEMENTATION_LIST_, FLEXUS__COMPONENT_CHAIN_CURRENT )

