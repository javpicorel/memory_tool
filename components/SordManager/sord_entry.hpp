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
#ifndef SORDENTRY_INCLUDED
#define SORDENTRY_INCLUDED

namespace nSordManager {

struct SORDEntry {
  tAddress theAddress;

  unsigned short theConsumedBy;
  unsigned short theMRPVector; //or theTemplate for perCons SORDS
  int theIndex; //The offset from the top of the in the SORD - for debugging

  unsigned char theValid:1;
  unsigned char theMispredicted:1;

  SORDEntry( tAddress anAddress, unsigned int anIndex, unsigned short anMRPVector)
    : theAddress(anAddress)
    , theConsumedBy(0)
    , theMRPVector(anMRPVector)
    , theIndex(anIndex)
    , theValid(true)
    , theMispredicted(false)
  {}

  ~SORDEntry( ) {  }

  bool hasConsumed(tID aNodeId) {
    return ( theConsumedBy & ( 1 << aNodeId ));
  }
  void markConsumed(tID aNodeId) {
    DBG_(VVerb, ( << "Marking " << & std::hex << theAddress << & std::dec << " consumed by " << aNodeId) );
    theConsumedBy |= ( 1 << aNodeId );
  }

  tAddress address() {
    return theAddress;
  }

  void invalidate() {
    theValid = false;
  }

  void mispredict() {
    theMispredicted = true;
  }

  bool valid() {
    return theValid;
  }

  bool mispredicted() {
    return theMispredicted;
  }

  unsigned short mrpVector() {
    return theMRPVector;
  }

  unsigned char streamTemplate() {
    return theMRPVector;
  }

  int index() {
    return theIndex;
  }

  friend std::ostream & operator << ( std::ostream & anOstream, SORDEntry & aSORDEntry) {
    anOstream <<  '@' << &std::hex << aSORDEntry.theAddress << &std::dec;
    anOstream <<  "{#" << aSORDEntry.theIndex << " /" << & std::hex << aSORDEntry.theMRPVector << & std::dec << "/}";
    return anOstream;
  }
};


typedef std::list< SORDEntry > SORDList;
typedef std::list< SORDEntry >::iterator SORDLocation;

class SORDMapEntry {
    SORDLocation theLocation;
    char theTemplateOrProducer;
  public:
    SORDMapEntry(SORDLocation const & aLocation, char aTemplateOrProducer)
      : theLocation(aLocation)
      , theTemplateOrProducer(aTemplateOrProducer)
      {}
    SORDMapEntry()
     : theTemplateOrProducer(-1)
      { }
    SORDLocation location() { return theLocation; }
    char streamTemplate() { return theTemplateOrProducer; }
    char producer() { return theTemplateOrProducer; }

};

typedef std::map< tAddress, SORDMapEntry  > GlobalSORDLocatorMap;         //Locates all addresses in all SORDS
typedef std::multimap< tAddress, SORDLocation  > PerConsSORDLocatorMap; //Locates all addresses in a SORD
typedef std::map< tAddress, char  > ProducerLocatorMap;                 //Locates the producer of all addresses

} //nSordManager

#endif //SORDENTRY_INCLUDED

