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

#define FLEXUS_BEGIN_COMPONENT NetworkTest
#include FLEXUS_BEGIN_COMPONENT_IMPLEMENTATION()

#include <time.h>
#include <boost/random.hpp>

  #define DBG_DefineCategories NetworkTest, Test
  #define DBG_SetDefaultOps AddCat(NetworkTest | Test)
  #include DBG_Control()


namespace nNetworkTest {

using namespace Flexus;
using namespace Core;
using namespace SharedTypes;

using boost::intrusive_ptr;
using boost::minstd_rand;

#define NUM_NODES HACK_WIDTH
#define NUM_VC 3

typedef long long SumType;
typedef long long * SumType_p;


/*
FLEXUS_COMPONENT_CONFIGURATION_TEMPLATE(NetworkTestConfiguration,
    FLEXUS_PARAMETER( NumCycles, long, "Number of cycles", "nc", 1000 )
    FLEXUS_PARAMETER( NumMessages, long, "Number of messages per cycle", "nm", 3 )
);
*/

    ParameterDefinition NumCycles_def_
       (  "NumCycles"
       ,  "Number of cycles"
       ,  "nc"
       );
    typedef Parameter
       < NumCycles_def_
       , long
       > NumCycles_param;
    ParameterDefinition NumMessages_def_
       (  "NumMessages"
       ,  "Number of messages per cycle"
       ,  "nm"
       );
    typedef Parameter
       < NumMessages_def_
       , long
       > NumMessages_param;

    struct NetCfgStruct {
      std::string theConfigName_;
      long NumCycles;
      long NumMessages;
    }

    template < class Param0 = use_default, class Param1 = use_default >
    struct NetworkTestConfiguration {
        NetCfgStruct theCfg;

        typedef mpl::list <
          std::pair< typename Param0::tag, Param0 >,
          std::pair< typename Param1::tag, Param1 >
        > ArgList;

        typedef NetworkTestConfiguration<Param0, Param1> type;
        std::string name() { return theConfigName_; }
        NetworkTestConfiguration(const std::string & aConfigName) :
            theConfigName_(aConfigName)
            , NumCycles(theCfg, "1000")
            , NumMessages(theCfg, "3")
            {}

        typedef typename resolve < ArgList, NumCycles_param, NumCycles_param:: Static<1000> ::selected >::type NumCycles_t;
        NumCycles_t NumCycles;
        typedef typename resolve < ArgList, NumMessages_param, NumMessages_param:: Static<3> ::selected >::type NumMessages_t;
        NumMessages_t NumMessages;
    };


template <class Configuration>
class NetworkTestComponent : public FlexusComponentBase< NetworkTestComponent, Configuration> {
  typedef FlexusComponentBase<nNetworkTest::NetworkTestComponent, Configuration> base;
  typedef typename base::cfg_t cfg_t;
  static std::string componentType() { return "nNetworkTest::NetworkTestComponent" ; }
  public:
  using base::flexusIndex;
  using base::flexusWidth;
  using base::name;
  using base::statName;
  using base::cfg;
  private:
  typedef typename base::self self;

 public:
   NetworkTestComponent( cfg_t & aCfg, index_t anIndex, index_t aWidth )
    : base( aCfg, anIndex, aWidth, Flexus::Dbg::tSeverity::Severity(FLEXUS_internal_COMP_DEBUG_SEV) )
    {}

  // Initialization
  void initialize() {
    int ii, jj;

    currCycle = 0;
    baseRand.seed(time(0));
    randNode = new boost::uniform_smallint<int> (0, NUM_NODES-1);
    randVc = new  boost::uniform_smallint<int> ( 0, NUM_VC-1);
    randValue = new boost::uniform_int<int> (0, 1<<30);

    sendSums = new SumType_p[NUM_NODES];
    recvSums = new SumType_p[NUM_NODES];
    for(ii = 0; ii < NUM_NODES; ii++) {
      sendSums[ii] = new SumType[NUM_VC];
      recvSums[ii] = new SumType[NUM_VC];
      for(jj = 0; jj < NUM_VC; jj++) {
        sendSums[ii][jj] = 0;
        recvSums[ii][jj] = 0;
      }
    }
  }

  ~NetworkTestComponent() {
    int ii, jj;
    for(ii = 0; ii < NUM_NODES; ii++) {
      for(jj = 0; jj < NUM_VC; jj++) {
        if(sendSums[ii][jj] != recvSums[ii][jj]) {
          std::cout << "sums didn't match\n";
        }
      }
    }
    std::cout << "done comparing sums\n";
  }

  // Ports
  struct ToNics : public PushOutputPortArray<NetworkTransport,NUM_NODES> { };

  struct FromNics : public PushInputPortArray<NetworkTransport,NUM_NODES>, AlwaysAvailable {
    typedef FLEXUS_IO_LIST_EMPTY Inputs;
    typedef FLEXUS_IO_LIST_EMPTY Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void push(self& aNetTest, index_t anIndex, NetworkTransport transport) {
      aNetTest.countRecv(transport, int(anIndex));
    }
  };

  //Drive Interfaces
  struct NetTestDrive {
    FLEXUS_DRIVE( NetTestDrive ) ;

    typedef FLEXUS_IO_LIST(1, Availability<ToNics> ) Inputs;
    typedef FLEXUS_IO_LIST(1, Value<ToNics> ) Outputs;

    FLEXUS_WIRING_TEMPLATE
    static void doCycle(self & aNetTest) {
      if(aNetTest.currCycle < aNetTest.cfg.NumCycles.value) {
        aNetTest.sendMsgs<FLEXUS_PASS_WIRING> ();
      }
      (aNetTest.currCycle)++;
    }
  };

  //Declare the list of Drive interfaces
  typedef FLEXUS_DRIVE_LIST(1, NetTestDrive) DriveInterfaces;

private:

  void countRecv(NetworkTransport & transport, int dest) {
    intrusive_ptr<NetworkMessage> msg = transport[NetworkMessageTag];
    recvSums[dest][msg->vc] += transport[ProtocolMessageTag]->value;
  }

  FLEXUS_WIRING_TEMPLATE
  void sendMsgs() {
    // send the request number of messages, randomly distributed
    // (sources, destinations, and virtual channels)
    int ii;
    for(ii = 0; ii < cfg.NumMessages.value; ii++) {
      int src = (*randNode)(baseRand);
      int dest = (*randNode)(baseRand);
      if(src != dest) {
        intrusive_ptr<NetworkMessage> msg(new NetworkMessage());
        msg->src = src;
        msg->dest = dest;
        msg->vc = (*randVc)(baseRand);

        intrusive_ptr<ProtocolMessage> val(new ProtocolMessage());
        val->value = (*randValue)(baseRand);

        NetworkTransport transport;
        transport.set(NetworkMessageTag, msg);
        transport.set(ProtocolMessageTag, val);
        FLEXUS_CHANNEL_ARRAY(*this, ToNics, src) << transport;

        sendSums[dest][msg->vc] += val->value;
      }
    }
  }

  // Tallies of cumulative sends and receives for each node and virtual channel
  SumType **sendSums;
  SumType **recvSums;

  // Random generators
  minstd_rand baseRand;
  boost::uniform_smallint<int> *randNode;
  boost::uniform_smallint<int> *randVc;
  boost::uniform_int<int> *randValue;

  // Current cycle number
  long currCycle;

};

} // namespace nNetworkTest

#undef NUM_NODES
#undef NUM_VC

#include FLEXUS_END_COMPONENT_IMPLEMENTATION()
#define FLEXUS_END_COMPONENT NetworkTest

  #define DBG_Reset
  #include DBG_Control()
