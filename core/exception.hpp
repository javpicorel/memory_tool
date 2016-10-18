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

#ifndef FLEXUS_EXCEPTION_HPP_INCLUDED
#define FLEXUS_EXCEPTION_HPP_INCLUDED


#include <string>
#include <cstring>
#include <memory>

namespace Flexus {
namespace Core {

using std::string;
using std::memcpy;
using std::strlen;
using std::exception;
using std::size_t;


#define FLEXUS_EXCEPTION(anExplanation) 	FlexusException(__FILE__,__LINE__,anExplanation)
#define SIMICS_EXCEPTION(anExplanation) 	SimicsException (__FILE__,__LINE__,anExplanation)

class FlexusException : public exception {
   protected:
	mutable char * theExplanation; //Very ugly to make this mutable, but we need to have a
		//"transfer of ownership" copy constructor, and it needs to be usable with temporaries,
		//so the constructor must take a const reference, but needs to modify theExplanation.
	char const * theFile;
	long theLine;
   public:
   	FlexusException()  : theExplanation(0), theFile(0),theLine(0) {}
	explicit FlexusException(string anExplanation) :
	    theFile(0),
	    theLine(0) {
		theExplanation = new char[anExplanation.size() +1];
		memcpy(theExplanation,anExplanation.c_str(),anExplanation.size() +1);
	}
	explicit FlexusException(char const * anExplanation) :
	    theFile(0),
	    theLine(0) {
		size_t len = strlen(anExplanation);
		theExplanation = new char[len + 1];
		memcpy(theExplanation,anExplanation, len);
		theExplanation[len] = 0;
	}
	FlexusException(char const * aFile, long aLine, string anExplanation) :
	    theFile(aFile),
	    theLine(aLine) {
		theExplanation = new char[anExplanation.size() +1];
		memcpy(theExplanation,anExplanation.c_str(),anExplanation.size() +1);
	}
	FlexusException(char const * aFile, long aLine, char const * anExplanation) :
	    theFile(aFile),
	    theLine(aLine) {
		size_t len = strlen(anExplanation);
		theExplanation = new char[len + 1];
		memcpy(theExplanation,anExplanation, len);
		theExplanation[len] = 0;
	}

	FlexusException(FlexusException const & anException) :
	    theExplanation(anException.theExplanation),
	    theFile(anException.theFile),
	    theLine(anException.theLine) {
		//Copying an exception MOVES ownership of the explanation
		anException.theExplanation = 0;		
	}
	
	virtual ~FlexusException() throw() {
		if (theExplanation) {
			delete [] theExplanation;
		}
	}

	virtual char const * no_explanation_str() const {
		return "Unknown Flexus exception";
	}	
	virtual char const * what() const throw() {
	//Note: since only one FlexusException object can ever conceivably exist at a time, this
	//trick of using a static for returning the char const * is ok.  Note also that since calling
	//what() on an exception will almost always happen right before a call to exit(), it doesn't
	//matter how slow this code is.
		static std::string return_string;
		return_string.clear();
		if (theFile) {
			return_string += '[' ;
			return_string +=  theFile;
			return_string += ':';
			return_string += theLine;
			return_string += "] ";
		}
		if (theExplanation) {
			return_string += theExplanation;
		} else {
			return_string += no_explanation_str();
		}
		return return_string.c_str();
	}
};


}//End Core

namespace Simics {

using std::string;
using std::memcpy;
using std::strlen;
using std::exception;
using std::size_t;


class SimicsException : public Flexus::Core::FlexusException {
	typedef Flexus::Core::FlexusException base;
   public:
   	SimicsException()  : base() {}
	explicit SimicsException(string anExplanation): 
		base(anExplanation) {}
	explicit SimicsException(char const * anExplanation):
		base(anExplanation) {}
	SimicsException(char const * aFile, long aLine, string anExplanation) :
		base(aFile, aLine, anExplanation) {}
	SimicsException(char const * aFile, long aLine, char const * anExplanation) :
		base(aFile, aLine, anExplanation) {}
	virtual ~SimicsException() throw() {}
	virtual char const * no_explanation_str() const {
			return "Unknown Simics exception";
		}
};

}  //End Namespace Simics
} //namespace Flexus

#endif //FLEXUS_EXCEPTION_HPP_INCLUDED


