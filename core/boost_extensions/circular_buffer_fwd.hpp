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



#if !defined(BOOST_CIRCULAR_BUFFER_FWD_HPP)
#define BOOST_CIRCULAR_BUFFER_FWD_HPP

#if defined(_MSC_VER) && _MSC_VER >= 1200
    #pragma once
#endif

#include <boost/config.hpp>
#if !defined(BOOST_NO_STD_ALLOCATOR)
    #include <memory>
#else
    #include <vector>
#endif

namespace boost {

#if !defined(BOOST_NO_STD_ALLOCATOR)
    #define BOOST_CB_DEFAULT_ALLOCATOR(T) std::allocator<T>
#else
    #define BOOST_CB_DEFAULT_ALLOCATOR(T) BOOST_DEDUCED_TYPENAME std::vector<T>::allocator_type
#endif

template <class T, class Alloc = BOOST_CB_DEFAULT_ALLOCATOR(T)>
class circular_buffer;

template <class T, class Alloc = BOOST_CB_DEFAULT_ALLOCATOR(T)>
class circular_buffer_space_optimized;

#undef BOOST_CB_DEFAULT_ALLOCATOR

} // namespace boost

#endif // #if !defined(BOOST_CIRCULAR_BUFFER_FWD_HPP)
