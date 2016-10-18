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

#ifndef FLEXUS_SLICES__DIRECTORY_ENTRY_HPP_INCLUDED
#define FLEXUS_SLICES__DIRECTORY_ENTRY_HPP_INCLUDED

#include <iostream>
#include <boost/serialization/version.hpp>
#include <boost/serialization/split_member.hpp>
#include <core/boost_extensions/intrusive_ptr.hpp>
#include <core/types.hpp>
#include <core/debug/debug.hpp>

#ifdef FLEXUS_DirectoryEntry_TYPE_PROVIDED
#error "Only one component may provide the Flexus::SharedTypes::DirectoryEntry data type"
#endif
#define FLEXUS_DirectoryEntry_TYPE_PROVIDED

namespace Flexus {
namespace SharedTypes {

  enum tDirState {
    DIR_STATE_INVALID  = 0,
    DIR_STATE_SHARED   = 1,
    DIR_STATE_MODIFIED = 2
  };

  std::ostream & operator << (std::ostream & anOstream, tDirState const x);

  typedef unsigned int node_id_t;

  ////////////////////////////////
  //
  // definition of the directory entry type
  //
  class DirectoryEntry : public boost::counted_base {
    public:
      static const unsigned long long kPastReaders = 0xFFFFFFFFFFFFFFFFULL;
      static const unsigned long kWasModified = 0x10000UL;
      static const unsigned long kStateMask = 0xC0000UL;
      static const unsigned long kStateShift = 18;

    private:
      //data members
      unsigned char theState;
      bool theWasModified;
      unsigned long long thePastReaders;
      unsigned long long theNodes;

      friend class boost::serialization::access;
      template<class Archive>
      void save(Archive & ar, const unsigned int version) const {
        ar & theState;
        ar & theWasModified;
        ar & thePastReaders;
        ar & theNodes;
      }

      template<class Archive>
      void load(Archive & ar, const unsigned int version) {
          switch(version) {
          case 0:
            unsigned long tempEncodedState;
            unsigned short tempNodes;

            ar & tempEncodedState;
            theState = (tempEncodedState & kStateMask) >> kStateShift;
            theWasModified = (tempEncodedState & kWasModified);
            thePastReaders = (tempEncodedState & 0xFFFFUL);
            ar & tempNodes;
            theNodes = (unsigned long long)tempNodes;
            break;
          case 1:
            ar & theState;
            ar & theWasModified;
            ar & thePastReaders;
            ar & theNodes;
            break;
          default:
            DBG_Assert(0, (<< "Unknown version for directory entry: " << version));
            break;
          }
      }
      BOOST_SERIALIZATION_SPLIT_MEMBER()

    void assertStateValid() const {
      int state = getState();
      DBG_Assert( state == DIR_STATE_INVALID || state == DIR_STATE_SHARED || state == DIR_STATE_MODIFIED );
    }
    tDirState getState() const {
      return tDirState(theState);
    }

    public:

    DirectoryEntry()
      : theState(DIR_STATE_INVALID)
      , theWasModified(false)
      , thePastReaders(0)
      , theNodes(0)
    { }
    DirectoryEntry(const DirectoryEntry & oldEntry)
      : theState (oldEntry.theState)
      , theWasModified (oldEntry.theWasModified)
      , thePastReaders (oldEntry.thePastReaders)
      , theNodes (oldEntry.theNodes)
    { }

    DirectoryEntry & operator= (DirectoryEntry const & oldEntry) {
      theState = oldEntry.theState;
      theWasModified = oldEntry.theWasModified;
      thePastReaders = oldEntry.thePastReaders;
      theNodes = oldEntry.theNodes;
      return *this;
    }

    friend std::ostream & operator << (std::ostream & aStream, DirectoryEntry const & anEntry);

    tDirState state() const {
      assertStateValid();
      return getState();
    }
    node_id_t owner() const {
      DBG_Assert( getState() == DIR_STATE_MODIFIED );
      DBG_Assert( theNodes < 64 );
      return theNodes;
    }
    unsigned long long sharers() const {
      DBG_Assert( getState() != DIR_STATE_MODIFIED);
      return theNodes;
    }
    bool isSharer(node_id_t aNode) const {
      DBG_Assert( getState() != DIR_STATE_MODIFIED );
      return theNodes & (1ULL << (unsigned long long)aNode);
    }
    bool wasSharer(node_id_t aNode) const {
      return getPastReaders() & (1ULL << (unsigned long long)aNode);
    }


    void markModified() {
      theWasModified = true;
    }
    bool wasModified() const {
      return theWasModified;
    }
    void setState(tDirState aState) {
      theState = aState;
    }
    void setOwner(node_id_t anOwner) {
      DBG_Assert(getState() == DIR_STATE_MODIFIED);
      thePastReaders |= 1ULL << (unsigned long long)anOwner;
      theNodes = anOwner;  // BTG: why don't we continue the one-hot encoding??
    }
    void setSharer(node_id_t aNode) {
      DBG_Assert(getState() != DIR_STATE_MODIFIED);
      thePastReaders |= 1ULL << (unsigned long long)aNode;
      theNodes |= 1ULL << (unsigned long long)aNode;
    }
    void setSharers(unsigned long long aVal) {
      DBG_Assert(getState() != DIR_STATE_MODIFIED);
      thePastReaders |= aVal;
      theNodes = aVal;
    }
    void clearSharer(node_id_t aNode) {
      DBG_Assert(getState() != DIR_STATE_MODIFIED);
      theNodes &= ~(1ULL << (unsigned long long)aNode);
    }
    void clearSharers() {
      DBG_Assert(getState() != DIR_STATE_MODIFIED);
      theNodes = 0;
    }
    void setAllSharers() {
      DBG_Assert(getState() != DIR_STATE_MODIFIED);
      thePastReaders |= kPastReaders;
      theNodes |= kPastReaders;
    }

    unsigned long long getPastReaders() const {
      return thePastReaders;
    }
		void setPastReaders (unsigned long long s) {
		  thePastReaders = s;
		}
  };



} //End SharedTypes
} //End Flexus

BOOST_CLASS_VERSION(Flexus::SharedTypes::DirectoryEntry, 1)  // all new checkpoints get version #1

#endif //FLEXUS_SLICES__DIRECTORY_ENTRY_HPP_INCLUDED
