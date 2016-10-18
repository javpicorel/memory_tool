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
/*! \file RdramBlackbox.hpp
 * \brief
 *
 *  Brief description goes here.
 *
 * Revision History:
 *     cfchen      23 Apr 03 - Initial Revision
 */

#include <deque>
#include <vector>
#include <iterator>
#include "RdramMagic.hpp"  // for rdram_bank_t and other strange things
                           // in this file  ;-)



namespace Flexus {
namespace Rdram {

  using namespace Flexus::Typelist;
  using Flexus::Debug::Debug;


  class RdramBlackbox {

  public:


    long long accesses;  // accesses = reads + writes
    long long reads;
    long long writes;

    long long delay;  // total latency in cycles
    long long currentCycle;  // total cycles during the simulation
                             // without taking into account the
                             // RDRAM latencies

    int lastBank;  // index to the most recently accessed bank
    rdram_bank_t banks[RDRAM_NUM_BANKS];


    /*
     * Constructor
     */
    RdramBlackbox() {

      currentCycle = 0;
      delay = 0;

      accesses = 0;
      reads = 0;
      writes = 0;

      lastBank = -1;

      // initialize banks
      memset(banks, 0, (RDRAM_NUM_BANKS*sizeof(rdram_bank_t)));
      for(int i=0; i < RDRAM_NUM_BANKS; i++) {
        banks[i].id = i;
        banks[i].lastType = -1;
        banks[i].state = ACTIVE;
      }
    }  // Constructor


    /*
     * Destructor
     */
    ~RdramBlackbox() {
      puke();
    }


    /*
     * Banks are set to a static state and remain at the state
     * through out the runtime
     */
    int accessStatic(RdramPhysicalMemoryAddress paddr, RdramCommand cmd) {
      rdram_bank_t * pbank = RDRAM_ADDR2BANK(banks, paddr);

      if(cmd == RDRAM_READ) {
        reads++;
      } else {
        writes++;
      }
      accesses++;

      int latency = 0;
      switch(pbank->state) {
        case ACTIVE:
          if(pbank->ready > currentCycle) {
            latency += RDRAM_ACTIVE_ACCESS;

            pbank->activeCycles += RDRAM_ACTIVE_ACCESS;
            // no transition cycles as the bank must be in transistion state
            pbank->ready += RDRAM_ACTIVE_ACCESS;
          } else {
            latency += RDRAM_ACTIVE_ACCESS;

            pbank->activeCycles += RDRAM_ACTIVE_ACCESS;
            pbank->ready = (currentCycle + latency);
          }
          break;

        case STANDBY:
          if(pbank->ready > currentCycle) {
            latency += RDRAM_ACTIVE_ACCESS;
            pbank->activeCycles += RDRAM_ACTIVE_ACCESS;
            // no transition cycles as the bank must be in transistion state
            pbank->ready += RDRAM_ACTIVE_ACCESS;
          } else {
            latency += RDRAM_ACTIVE_ACCESS;
            latency += RDRAM_STANDBY_TO_ACTIVE;

            pbank->activeCycles += RDRAM_ACTIVE_ACCESS;
            pbank->s2aCycles += RDRAM_STANDBY_TO_ACTIVE;
            pbank->ready = (currentCycle + latency);
          }
          break;

        case NAP:
          if(pbank->ready > currentCycle) {
            latency += RDRAM_ACTIVE_ACCESS;

            pbank->activeCycles += RDRAM_ACTIVE_ACCESS;
            // no transition cycles as the bank must be in transistion state
            pbank->ready += RDRAM_ACTIVE_ACCESS;
          } else {
            latency += RDRAM_ACTIVE_ACCESS;
            latency += RDRAM_NAP_TO_ACTIVE;

            pbank->activeCycles += RDRAM_ACTIVE_ACCESS;
            pbank->n2aCycles += RDRAM_NAP_TO_ACTIVE;
            pbank->ready = (currentCycle + latency);
          }
          break;

        case POWERDOWN:
          if(pbank->ready > currentCycle) {
            latency += RDRAM_ACTIVE_ACCESS;

            pbank->activeCycles += RDRAM_ACTIVE_ACCESS;
            // no transition cycles as the bank must be in transistion state
            pbank->ready += RDRAM_ACTIVE_ACCESS;
          } else {
            latency += RDRAM_ACTIVE_ACCESS;
            latency += RDRAM_POWERDOWN_TO_ACTIVE;

            pbank->activeCycles += RDRAM_ACTIVE_ACCESS;
            pbank->p2aCycles += RDRAM_POWERDOWN_TO_ACTIVE;
            pbank->ready = (currentCycle + latency);
          }
          break;

        default:
          assert(0);
          break;
      }



      delay = latency;

      return latency;
    }  // accessStatic


    /*
     * prints out statistics out to STDOUT
     */
    void puke() {
      cout << endl;
      cout << "RDRAM statistics:" << endl;
      cout << "Cycles elapsed: " << currentCycle << endl;
      cout << "Number of accesses: " << accesses << endl;
      cout << "Number of reads: " << reads << endl;
      cout << "Number of writes: " << writes << endl;

      int numberActiveBanks = 0;
      int numberStandbyBanks = 0;
      int numberNapBanks = 0;
      int numberPowerdownBanks = 0;

      long long activeCycles = 0;
      long long standbyCycles = 0;
      long long napCycles = 0;
      long long powerdownCycles = 0;

      for(int i=0; i<RDRAM_NUM_BANKS; i++) {

        switch(banks[i].state) {
          case ACTIVE:
            numberActiveBanks++;
            activeCycles += banks[i].activeCycles;
            standbyCycles += banks[i].standbyCycles;
            napCycles += banks[i].napCycles;
            powerdownCycles += banks[i].powerdownCycles;
            break;

          case STANDBY:
            numberStandbyBanks++;
            activeCycles += banks[i].activeCycles;
            standbyCycles += banks[i].standbyCycles;
            napCycles += banks[i].napCycles;
            powerdownCycles += banks[i].powerdownCycles;
            break;

          case NAP:
            numberNapBanks++;
            activeCycles += banks[i].activeCycles;
            standbyCycles += banks[i].standbyCycles;
            napCycles += banks[i].napCycles;
            powerdownCycles += banks[i].powerdownCycles;
            break;

          case POWERDOWN:
          default:
            numberPowerdownBanks++;
            activeCycles += banks[i].activeCycles;
            standbyCycles += banks[i].standbyCycles;
            napCycles += banks[i].napCycles;
            powerdownCycles += banks[i].powerdownCycles;
            break;

        }  // switch

      }  // for

      long long total_engy = RDRAM_ACTIVE_POWER*activeCycles +
                             RDRAM_STANDBY_POWER*standbyCycles +
                             RDRAM_NAP_POWER*napCycles +
                             RDRAM_POWERDOWN_POWER*powerdownCycles;

      cout << "Number of banks in ACTIVE state: " << numberActiveBanks << endl;
      cout << "Number of banks in STANDBY state: " << numberStandbyBanks << endl;
      cout << "Number of banks in NAP state: " << numberNapBanks << endl;
      cout << "Number of banks in POWERDOWN: " << numberPowerdownBanks << endl;
      cout << "Energy (nJ): " << total_engy << endl;
      cout << endl;
    }  // puke


    /*
     * doServe
     */
    inline
    int doServe() {
      currentCycle++;
/* 
      if(1 == outstanding) {
        assert(busy >= 0);

        if(busy > 0) {
          busy--;
        }

        if(0 == busy) {
          outstanding = 0;
          return 1;
        }
      }
 */
      return 0;
    }  // doServe


#ifdef USED  // used for debugging purpose only
    /*
     * accessDebug -- constant access latency for debugging purpose
     */
    int accessDebug(RdramPhysicalMemoryAddress paddr, RdramCommand cmd) {
      rdram_bank_t * pbank = RDRAM_ADDR2BANK(banks, paddr);

      if(cmd == RDRAM_READ) {
        reads++;
      } else {
        writes++;
      }
      accesses++;

      lastBank = pbank->id;
      pbank->lastAccessed = currentCycle;
      pbank->ready = (currentCycle + 6);

      outstanding = 1;
      busy = 6;
      return 6;
    }  // accessDebug


    /*
     * Print out bank state/status
     */
    void printStates() {
      cout << endl;
      cout << "RDRAM statistics:" << endl;
      cout << "Cycles elapsed: " << currentCycle << endl;
      cout << "Number of accesses: " << accesses << endl;
      cout << "Number of reads: " << reads << endl;
      cout << "Number of writes: " << writes << endl;

      int numberActiveBanks = 0;
      int numberStandbyBanks = 0;
      int numberNapBanks = 0;
      int numberPowerdownBanks = 0;
      for(int i=0; i<RDRAM_NUM_BANKS; i++) {
        long long idleInterval = (currentCycle - banks[i].lastAccessed);
        // has the bank been standing by or napping or off?
        if(idleInterval > RDRAM_THRESHOLD) {
          switch(idleInterval / RDRAM_THRESHOLD) {
            case 1:  // STANDBY STATE
              numberStandbyBanks++;
              banks[i].activeCycles += RDRAM_THRESHOLD;
              banks[i].standbyCycles += (idleInterval - RDRAM_THRESHOLD);

              break;
            case 2:  // NAP STATE
              numberNapBanks++;
              banks[i].activeCycles += RDRAM_THRESHOLD;
              banks[i].standbyCycles += RDRAM_THRESHOLD;
              banks[i].napCycles += (idleInterval - 2*RDRAM_THRESHOLD);

              break;
            case 3:  // POWERDOWN STATE
            default:
              numberPowerdownBanks++;
              banks[i].activeCycles += RDRAM_THRESHOLD;
              banks[i].standbyCycles += RDRAM_THRESHOLD;
              banks[i].napCycles += RDRAM_THRESHOLD;
              banks[i].powerdownCycles += (idleInterval - 3*RDRAM_THRESHOLD);

              break;
            }  // switch
         } else {
           numberActiveBanks++;
           banks[i].activeCycles += idleInterval;
         }
      }  // for

      cout << "Number of banks in ACTIVE state: " << numberActiveBanks << endl;
      cout << "Number of banks in STANDBY state: " << numberStandbyBanks << endl;
      cout << "Number of banks in NAP state: " << numberNapBanks << endl;
      cout << "Number of banks in POWERDOWN: " << numberPowerdownBanks << endl;
      cout << endl;

    }  // printStates
    
    
    /**
     * RDRAM bank access function -- only works if the banks are initialized
     * to ACTIVE state
     */
    int access(RdramPhysicalMemoryAddress paddr, RdramCommand cmd) {
      if(cmd == RDRAM_READ) {
        reads++;
      } else {
        writes++;
      }
      accesses++;

      rdram_bank_t * pbank = RDRAM_ADDR2BANK(banks, paddr);
      int latency = 0;
      long long idleInterval = (currentCycle - pbank->lastAccessed);

      // has the bank been standing by or napping or off?
      if(idleInterval > RDRAM_THRESHOLD) {
        switch(idleInterval / RDRAM_THRESHOLD) {
          case 1:  // STANDBY STATE
            latency += RDRAM_STANDBY_TO_ACTIVE;
            latency += RDRAM_ACTIVE_ACCESS;
            pbank->ready =
                (currentCycle + RDRAM_STANDBY_TO_ACTIVE + RDRAM_ACTIVE_ACCESS);

            pbank->activeCycles += RDRAM_THRESHOLD;
            pbank->standbyCycles += (idleInterval - RDRAM_THRESHOLD);

            break;
          case 2:  // NAP STATE
            latency += RDRAM_NAP_TO_ACTIVE;
            latency += RDRAM_ACTIVE_ACCESS;
            pbank->ready =
                (currentCycle + RDRAM_NAP_TO_ACTIVE + RDRAM_ACTIVE_ACCESS);

            pbank->activeCycles += RDRAM_THRESHOLD;
            pbank->standbyCycles += RDRAM_THRESHOLD;
            pbank->napCycles += (idleInterval - 2*RDRAM_THRESHOLD);

            break;
          case 3:  // POWERDOWN STATE
          default:
            latency += RDRAM_POWERDOWN_TO_ACTIVE;
            latency += RDRAM_ACTIVE_ACCESS;
            pbank->ready =
                (currentCycle + RDRAM_POWERDOWN_TO_ACTIVE + RDRAM_ACTIVE_ACCESS);

            pbank->activeCycles += RDRAM_THRESHOLD;
            pbank->standbyCycles += RDRAM_THRESHOLD;
            pbank->napCycles += RDRAM_THRESHOLD;
            pbank->powerdownCycles += (idleInterval - 3*RDRAM_THRESHOLD);

            break;
        }  // switch

      } else {  // bank in ACTIVE state
        if(currentCycle < pbank->ready) {
          latency += (pbank->ready - currentCycle + RDRAM_ACTIVE_ACCESS);
          pbank->ready += RDRAM_ACTIVE_ACCESS;
        } else {
          latency += RDRAM_ACTIVE_ACCESS;
          pbank->ready = (currentCycle + RDRAM_ACTIVE_ACCESS);
        }

      }

      lastBank = pbank->id;
      pbank->lastAccessed = currentCycle;

      outstanding = 1;
      busy = latency;

      return latency;
    }  // access
#endif


  };  // end class RdramBank


}  // end namespace Rdram
}  // end namespace Flexus

