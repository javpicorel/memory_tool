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
#ifndef _PIRANHACACHECONTROLLERIMPL_HPP
#define _PIRANHACACHECONTROLLERIMPL_HPP

#include <deque>
#include <vector>
#include <iterator>
#include <fstream>

#include <core/performance/profile.hpp>
#include <core/metaprogram.hpp>
#include <core/stats.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
using namespace boost::multi_index;

using namespace Flexus;

#include "CacheControllerImpl.hpp"
#include "NewCacheArray.hpp"
#include "CacheBuffers.hpp"
#include "MissTracker.hpp"


namespace nCache
{

  namespace Stat = Flexus::Stat;

  const int kDirectoryOwner = -1;
  typedef long long SharersList;

  const MemoryMessage::MemoryMessageType memoryReply ( MemoryMessage::MemoryMessageType req );

  enum PiranhaDirState
    {
      D_I,
      D_M,
      D_O,
      D_S,
      D_MMW,
      D_MMU,
      D_S2MW,
      D_S2MU,
      D_SFWD,
      D_M2O,
      D_ExtInvalidation,
      D_ExtDowngrade,
      D_GetExtShared,
      D_GetExtModified,
      D_GetExtModifiedInvalidationPending,
      D_EvictWait,
      D_IPR,
      D_IPW
    };

  typedef PiranhaDirState PDState; // shorter name

  std::ostream & operator << ( std::ostream & s, PDState const & state );

  struct PiranhaDirEntry : boost::counted_base
  {
  public:
    explicit PiranhaDirEntry ( Tag anAddress = 0 )
      : theAddress          ( anAddress )
        , theState          ( D_I )
        , theSharersList    ( 0 )
        , theSharersCount   ( 0 )
        , theOwner          ( kDirectoryOwner )
        , theAcount         ( 0 )
        , theNextOwner      ( kDirectoryOwner )
        , theNextState      ( D_I )
        , theNextMsg        ( LOAD_REQ  )
        , theExtNextState   ( D_I )
        , theEvictionQueued ( false )
        , theIsModified     ( false )
    {}

    explicit PiranhaDirEntry ( std::ifstream & ifs );

  protected:
    Tag           theAddress;
    PDState       theState;
    SharersList   theSharersList;
    int           theSharersCount;

    int           theOwner;
    int           theAcount;
    // Information for the next state
    int           theNextOwner;
    PDState       theNextState;
    UniAccessType theNextMsg;
    // Information for after an ext state (GetShared/GetModified states)
    // We need to know which transient or stable state to go to.
    PDState       theExtNextState;
    bool          theEvictionQueued;
    bool          theIsModified;

  public:

    void saveState ( std::ofstream & ofs );

    // Accessor functions

    const Tag         address          ( void ) const { return theAddress; }
    const SharersList sharersList      ( void ) const { return theSharersList; }
    const int         sharersCount     ( void ) const { return theSharersCount; }
    const int         aCount           ( void ) const { return theAcount; }
    const int         owner            ( void ) const { return theOwner; }
    const int         nextOwner        ( void ) const { return theNextOwner; }
    const bool        directoryOwned   ( void ) const { return theOwner == kDirectoryOwner; }
    const PDState     extNextState     ( void ) const { return theExtNextState; }
    const bool        evictionQueued   ( void ) const { return theEvictionQueued; }
    const bool        isModified       ( void ) const { return theIsModified; }

    const void        incrementAcount ( void )        { theAcount++; }
    const bool        decrementAcount ( void )        { return ((--theAcount) == 0); }
    int &             nextOwner       ( void )        { return theNextOwner; }
    PDState &         nextState       ( void )        { return theNextState; }
    PDState &         extNextState    ( void )        { return theExtNextState; }
    bool    &         evictionQueued  ( void )        { return theEvictionQueued; }
    bool    &         isModified      ( void )        { return theIsModified; }

    const void setOwner ( int owner )
    {
      DBG_ ( Trace, ( << "PD[" << std::hex << MemoryAddress ( theAddress )
                      << "] changing owner to " << owner ) );
      theOwner = owner;
    }

    const bool isSharer ( const int sharer )
    {
      return ( ( 1LL << sharer ) & theSharersList ) ? true : false;
    }

    const void setAcount       ( const int aCount )
    {
      DBG_Assert ( aCount > 0 );
      DBG_Assert ( aCount <= theSharersCount);
      theAcount = aCount;
    }

    // Get the lowest index sharer
    const int firstSharer ( void )
    {
      SharersList
        list = theSharersList;

      int
        i = 0;

      for ( i = 0; list != 0; i++ ) {

        if ( (list & 1ULL) == 1ULL )
          return i;

        list = list >> 1;
      }

      return kDirectoryOwner;
    }

    void addSharer ( const int idx )
    {
      SharersList
        mask = ( 1LL << idx );

      if ( ( theSharersList & mask ) == 0 ) {
        theSharersList |= mask;
        theSharersCount++;
      }
    }

    void removeSharer ( const int idx )
    {
      if ( isSharer ( idx ) ) {
        theSharersList &= ~( 1LL << idx );
        theSharersCount--;
      }
      DBG_Assert ( theSharersCount >= 0 );
    }

    const PDState state ( void ) const { return theState; }
    void setState ( PDState newState )
    {
      DBG_ ( Trace, Addr(theAddress) ( << "PD[" << std::hex << MemoryAddress ( theAddress ) << "] changing state from " << theState << " to " << newState ) );
      theState = newState;
    }

    const bool isStable ( void ) const
    {
      switch ( theState ) {
      case D_I:
      case D_O:
      case D_M:
      case D_S:
        return true;
      default:
        return false;
      }
    }

  };

  typedef boost::intrusive_ptr<PiranhaDirEntry> PiranhaDirEntry_p;

  std::ostream & operator << ( std::ostream & s, PiranhaDirEntry const & dirEntry );

  class PiranhaDir
  {
  protected:
    typedef std::map<Tag, PiranhaDirEntry_p> DirMap;

    DirMap theDir;
    Tag               blockMask;

  public:
    PiranhaDir ( const int aBlockSize )
    {
      blockMask = ~(( 1LL << log_base2 ( aBlockSize ) ) - 1LL);
    }

    PiranhaDirEntry_p lookup ( MemoryAddress memAddr )
    {
      Tag
        blockAddr = (memAddr & blockMask);

      DirMap::iterator
        iter = theDir.find ( blockAddr );

      if ( iter == theDir.end() )
        return (new PiranhaDirEntry ( blockAddr ) );

      return iter->second;
    }

    void insert ( PiranhaDirEntry_p entry )
    {
      theDir.insert ( std::make_pair(entry->address(), entry) );
    }

    void remove ( Tag tag )
    {
      DirMap::iterator
        iter = theDir.find ( tag );

      if ( iter != theDir.end() )
        theDir.erase ( iter );
    }

    void remove ( MemoryAddress memAddr )
    {
      return remove ( memAddr & blockMask );
    }

    void clear ( void )
    {
      theDir.clear();
    }

    void loadState ( std::ifstream & ifs );
    void saveState ( std::ofstream & ofs );

  };


  class PiranhaCacheControllerImpl : public BaseCacheControllerImpl
  {

  int theASR_ReplicationThresholds[8]; /* CMU-ONLY */

  // fixme: move to a separate namespace and share with CmpNetworkControl
  // 2D torus topology (for steering ReturnReq)
  unsigned int theNumCoresPerTorusRow;  // the number of cores per torus row
  std::vector<unsigned int> theNTileID; // north tile neighbor in torus
  std::vector<unsigned int> theSTileID; // south tile neighbor in torus
  std::vector<unsigned int> theWTileID; // west tile neighbor in torus
  std::vector<unsigned int> theETileID; // east tile neighbor in torus

  protected:
    PiranhaDir  * theDir;

  public:

    PiranhaCacheControllerImpl ( BaseCacheController * aController,
                                 CacheInitInfo       * aInit );


    virtual Action
    performOperation ( MemoryMessage_p        msg,
                       TransactionTracker_p   tracker,
                       LookupResult         & l2Lookup,
                       bool                   wasHit__,
                       bool                   anyInvs,
                       bool                   blockAddressOnMiss );

    virtual Action handleMiss ( MemoryMessage_p         msg,
                                TransactionTracker_p    tracker,
                                LookupResult          & lookup,
                                bool                    address_conflict_on_miss,
                                bool                    probeIfetchMiss );

    virtual Action handleBackMessage ( MemoryMessage_p      msg,
                                        TransactionTracker_p tracker );

    virtual Action handleSnoopMessage ( MemoryMessage_p      msg,
                                         TransactionTracker_p tracker );

    virtual Action handleIprobe ( bool aHit,
                           MemoryMessage_p        fetchReq,
                           TransactionTracker_p   tracker );

    virtual void saveState(std::string const & aDirName);

    virtual void loadState(std::string const & aDirName);

  protected:

    virtual void handle_D_I ( MemoryMessage_p        msg,
                              Action               & action,
                              LookupResult         & l2Lookup,
                              PiranhaDirEntry_p      dirEntry,
                              bool                   inEvictBuffer );
    virtual void handle_D_M ( MemoryMessage_p        msg,
                              Action               & action,
                              LookupResult         & l2Lookup,
                              PiranhaDirEntry_p      dirEntry,
                              bool                   inEvictBuffer );
    virtual void handle_D_O ( MemoryMessage_p        msg,
                              Action               & action,
                              LookupResult         & l2Lookup,
                              PiranhaDirEntry_p      dirEntry,
                              bool                   inEvictBuffer );
    virtual bool skipAllocateInLocalSlice();     /* CMU-ONLY */
    virtual void handle_D_S ( MemoryMessage_p        msg,
                              Action               & action,
                              LookupResult         & l2Lookup,
                              PiranhaDirEntry_p      dirEntry,
                              bool                   inEvictBuffer );
    virtual void handle_D_ExtGet ( MemoryMessage_p        msg,
                                   Action               & action,
                                   LookupResult         & l2Lookup,
                                   PiranhaDirEntry_p      dirEntry,
                                   bool                   inEvictBuffer );
    virtual void handle_D_ExtMessage ( MemoryMessage_p        msg,
                                       Action               & action,
                                       LookupResult         & l2Lookup,
                                       PiranhaDirEntry_p      dirEntry,
                                       bool                   inEvictBuffer );
    virtual void handle_D_Transient ( MemoryMessage_p        msg,
                                      Action               & action,
                                      LookupResult         & l2Lookup,
                                      PiranhaDirEntry_p      dirEntry,
                                      bool                   inEvictBuffer );

    virtual bool sendOwnerRequest ( MemoryMessage_p                  origMessage,
                                    MemoryMessage::MemoryMessageType type,
                                    Action                           & action,
                                    PiranhaDirEntry_p                dirEntry,
                                    PDState                  nextState,
                                    PDState                  nextNextState,
                                    int                              nextOwner = kDirectoryOwner );

    /* CMU-ONLY-BLOCK-BEGIN */
    virtual bool sendPurgeRequest ( MemoryMessage_p                  origMessage,
                                    MemoryMessage::MemoryMessageType type,
                                    Action                           & action,
                                    PiranhaDirEntry_p                dirEntry,
                                    PDState                          nextState,
                                    PDState                          nextNextState );
    /* CMU-ONLY-BLOCK-END */

    virtual bool sendBroadcast ( MemoryMessage_p                    origMessage,
                                 MemoryMessage::MemoryMessageType   type,
                                 Action                           & action,
                                 PiranhaDirEntry_p                  dirEntry,
                                 int                                skipCore = kDirectoryOwner );

    virtual bool setupExternalRequest ( MemoryMessage_p                  originalReq,
                                        MemoryMessage::MemoryMessageType type,
                                        Action                           & action,
                                        PiranhaDirEntry_p                dirEntry,
                                        PDState                  extState,
                                        PDState                  extNextState,
                                        PDState                  nextState = D_I );

    virtual bool setupEviction ( LookupResult    & l2Lookup,
                                 AccessType        accessType,
                                 MemoryMessage_p   replacement );

    virtual bool updateMissStats ( MemoryMessage_p        msg,
                                   TransactionTracker_p   tracker,
                                   LookupResult         & l2Lookup,
                                   PiranhaDirEntry_p      dirEntry,
                                   bool                   inEvictBuffer );

    virtual bool resurrectEvictedBlock ( MemoryMessage_p      msg,
                                         LookupResult      & l2Lookup,
                                         PiranhaDirEntry_p   dirEntry );

    tFillType pageStateToFillType ( const tPageState aPageState ) const;

    void overwriteStatsHitLocal  ( TransactionTracker_p           tracker,
                                   tFillType                      fillType );

    void updateStatsHitLocal  ( TransactionTracker_p           tracker,
                                tFillType                      fillType );

    void updateStatsHitPeerL1 ( TransactionTracker_p           tracker,
                                tStateType stateType );

  }; // class PiranhaCacheControllerImpl


}; // namespace nCache


#endif // _PIRANHACACHECONTROLLERIMPL_HPP


