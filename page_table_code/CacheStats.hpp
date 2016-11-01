#ifndef FLEXUS_FAST3DCACHE_CACHESTATS_HPP_INCLUDED
#define FLEXUS_FAST3DCACHE_CACHESTATS_HPP_INCLUDED

#include <core/stats.hpp>
#include <iostream>
#include <fstream>
#include "Histogram.hpp"
#include <zlib.h>


#define BLOCK_SIZE 64
#define MAX_BLOCKS_PER_REGION 64

using namespace Flexus::SharedTypes;
using namespace Flexus;
using namespace Stat;
using namespace std;

const unsigned int FETCH=0;
const unsigned int READ=1;
const unsigned int WRITE=2;

static bool ini = false;


struct CacheStats {
    std::string file;
    std::string region;
    std::string str;
    
    unsigned int  cacheSize;
    unsigned int  region_size;  
    unsigned assoc;
    
    unsigned long long theColdMisses_stat; 
    unsigned long long theReadRequests_stat;
    unsigned long long theWriteRequests_stat;
    unsigned long long theReadMisses_stat;
    unsigned long long theWriteMisses_stat;
    unsigned long long theReadHits_stat;
    unsigned long long theWriteHits_stat;
    unsigned long long thePageInserts_stat;
    unsigned long long wayPredictionHits;
    unsigned long long wayPredictionMisses;

    CacheStats(std::string const & theName, std::string const &reg, unsigned cacheSize, unsigned regionSize, unsigned assoc, std::string const & rpath)
    { 
        region.clear();
        region.append(reg);
        
        region_size = regionSize;
        cacheSize=cacheSize;
        file.clear();
        file.append(theName);
        file.append("_");
        file.append(region);
        assoc=assoc;

        if (!ini){

          str.clear();
          str.append(rpath);

          str.append(file);
          str.append("_");
          string s1 = boost::lexical_cast<string>(region_size);
          str.append(s1);
          str.append("B_page_");
          string s2 = boost::lexical_cast<string>(cacheSize);
          str.append(s2);
          str.append("MB_Memory_"); 
          str.append(boost::lexical_cast<string>(assoc));
          str.append("ways");
          std::cout<<"stat file= "<<str<<std::endl;       
          char* cstr = new char [str.size()+1];
          strcpy (cstr, str.c_str());
          ofstream output;
          output.open(cstr, ios::out);
          output.close();

          ini = true;
        }

        theColdMisses_stat = 0;
        theReadRequests_stat = 0;
        theWriteRequests_stat = 0;
        theReadMisses_stat = 0;
        theWriteMisses_stat = 0;
        theReadHits_stat = 0;
        theWriteHits_stat = 0;
        thePageInserts_stat = 0;
	      wayPredictionHits=0;
       	wayPredictionMisses=0;
    }

    void update() {
    }

    void printStatistics(){

        char* cstr = new char [str.size()+1];
        strcpy (cstr, str.c_str());
        ofstream output;
        output.open(cstr, ios::app);
        output<<"theColdMisses =  "<< theColdMisses_stat<<std::endl;
        output<<"theReadRequests =  "<< theReadRequests_stat<<std::endl;
        output<<"theWriteRequests = "<< theWriteRequests_stat<<std::endl;
        output<<"theReadMisses  = "<< theReadMisses_stat<<std::endl;
        output<<"theWriteMisses  = "<< theWriteMisses_stat<<std::endl;
        output<<"theReadHits = "<<theReadHits_stat<<std::endl;
        output<<"theWriteHits = "<<theWriteHits_stat<<std::endl;
        output<<"thePageInserts  = "<< thePageInserts_stat<<std::endl;
        output<<"wayPredictionHits = "<<wayPredictionHits<<std::endl;
        output<<"wayPredictionMisses = "<<wayPredictionMisses<<std::endl;
        output<<"wayPredictio HRatio = "<<100.0*wayPredictionHits/(wayPredictionMisses+wayPredictionHits)<<std::endl;
        output<<"Cache Miss Ratio = "<<((float)(theReadMisses_stat + theWriteMisses_stat)/(theReadRequests_stat + theWriteRequests_stat))<<std::endl;
        output<<"theMisses (-Cold) = "<<(theReadMisses_stat + theWriteMisses_stat - theColdMisses_stat)<<std::endl;
        output<<"Cache Miss Ratio (-Cold) = "<<((float)(theReadMisses_stat + theWriteMisses_stat - theColdMisses_stat)/(theReadRequests_stat + theWriteRequests_stat))<<std::endl;
    }
};


#endif /* FLEXUS_FAST3DCACHE_CACHESTATS_HPP_INCLUDED */
