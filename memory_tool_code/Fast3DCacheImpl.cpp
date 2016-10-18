#include <core/stats.hpp>
#include "common.hpp"
#include "CacheStats.hpp"
#include "PageCache.hpp"
#include <zlib.h>
#include <fstream>

namespace Flexus {
namespace Core {
  void Break() {}
}
}

struct compressedRecord{
 char operation;
 uint64_t address;
 uint32_t index;
};

char* workloads[] =     {"memcached",       
                         "rocksdb",       
                         "mysql",     
                         "tpch", 
                         "tpcds", 
                         "cassandra", 
                         "neo4j", 
                         "test", 
                         "memcached-filtered", 
                         "rocksdb-filtered", 
                         "mysql-filtered", 
                         "tpch-filtered", 
                         "tpcds-filtered", 
                         "cassandra-filtered", 
                         "neo4j-filtered", 
                         "test-filtered"};

char* workloadPaths[] = {"pinatrace-memcached",     
                         "pinatrace-rocksdb", 
                         "pinatrace-mysql", 
                         "pinatrace-tpch", 
                         "pinatrace-tpcds", 
                         "pinatrace-cassandra",
                         "pinatrace-neo4j", 
                         "pinatrace-test",
                         "pinatrace-filtered-memcached", 
                         "pinatrace-filtered-rocksdb", 
                         "pinatrace-filtered-mysql", 
                         "pinatrace-filtered-tpch", 
                         "pinatrace-filtered-tpcds",
                         "pinatrace-filtered-cassandra", 
                         "pinatrace-filtered-neo4j", 
                         "pinatrace-filtered-test"};

//Scale factor
#define S8 8
#define S16 16
#define S32 32
#define S64 64

class Fast3DCache{

  unsigned int region_size;
  std::string file;
  unsigned int workload; 
  unsigned int cacheSize;
  unsigned int region;
  unsigned int associativity;	
  std::string result_path;
  unsigned int numberVault;
  unsigned int numberWpEntries;
public:
  PageCache  *theCache; //Memory structure
  CacheStats *theStats; //Memory stats structure
 
  // Initialization
  void initialize(unsigned wkld, unsigned  reg_size,  unsigned  cache_size, unsigned assoc, std::string const & rpath, unsigned int vault, unsigned int wp){
          workload =wkld;  
          file.clear();
          file=workloads[workload];
          result_path.clear();
          result_path=rpath;
          region=0;
          region_size = reg_size;
          cacheSize=cache_size;
          associativity=assoc;
          numberVault = vault;
          numberWpEntries = wp;
          theStats = new CacheStats(file,"region_0", cacheSize, region_size,  assoc, rpath);         
          theCache = new PageCache(region_size,  cacheSize, assoc, theStats,  workload, numberVault, numberWpEntries);
  }

  Fast3DCache (unsigned wkld, unsigned regSize, unsigned onChipMemorySize, unsigned associativity, std::string const & rpath, unsigned int vault, unsigned int wp){
     initialize(wkld, regSize,  onChipMemorySize, associativity, rpath, vault, wp); 
  }

  void printStatistics()
  {
    theStats->printStatistics();
  }
 
  void newRegion(){
    delete theStats;
    region++;
    string str="region_";
    string s1= boost::lexical_cast<string>(region);
    str.append(s1);
    theStats=new CacheStats(file, str, cacheSize, region_size, associativity, result_path);
    theCache->setStats(theStats); 
  }

  void saveStats(){
    printStatistics();
    newRegion();
  }   

  PageCache* getCache() { return theCache;}

  void forwardMessage(compressedRecord &msg){}  

  unsigned int replace( compressedRecord &message){
     unsigned int way;
     unsigned long toReplace;
     state_t victim_state = theCache->findVictim(message.address, way, toReplace);
     if(victim_state ==kInvalid) theStats->theColdMisses_stat++;
     return way;
  }

  // Ports
  void fromL2(compressedRecord &message, short &idx) {
    unsigned int way = 0;
    unsigned int op = kRead;
    bool page_in_cache = false;

    if (!(((uint32_t) idx == message.index) || (idx == -1))){
      return;
    }

    state_t state= theCache->lookup(message.address, way);  

    if(state == kValid){
      page_in_cache = true; //page hit       
    }

    /*handling cache*/ 
    if(page_in_cache){
       unsigned predictedWay=theCache->predictWay(message.address);

      if(predictedWay==way)
        (theStats->wayPredictionHits)++;
      else{
        (theStats->wayPredictionMisses)++;
        theCache->fixPredictor(message.address, way);
      }
    }

    if(page_in_cache){ //Hit
      if(message.operation == 'R'){
        op = kRead;
        theStats->theReadRequests_stat++;
		    theStats->theReadHits_stat++;
	    }else if(message.operation  == 'W' ){
        op = kWrite;
        theStats->theWriteRequests_stat++;
		    theStats->theWriteHits_stat++;
	    }

      theCache->touchAnAddress(message.address, way, op); 
      theCache->applyPolicy(message.address, way);	
    }else{ //Miss
       if(message.operation == 'R'){
         theStats->theReadRequests_stat++;
         theStats->theReadMisses_stat++;

         way = replace(message);
         theCache->insert(message.address, kValid, way, READ);
       }else if(message.operation=='W'){  
         theStats->theWriteRequests_stat++;
         theStats->theWriteMisses_stat++;
         way = replace(message);
         theCache->insert(message.address, kValid, way, WRITE);
       } 
    }

  }

  ~Fast3DCache(){
    delete theCache;
    delete theStats;
  }

 std::string& myFile(){
  return file;
 }

};

void printUsage(string s){
 unsigned int numWorkloads = sizeof(workloads)/sizeof(workloads[0]);

 cout<<s<<endl;
 cout<<"Usage: Fast3DCacheImpl m w (p) (a) (i) (o) (v) (wp) (s)"<< std::endl;
 cout<<"    m - memory size in MB"<< std::endl;
 cout<<"    w - workload (0-" << numWorkloads - 1 << ")" << std::endl;

 for(unsigned int i=0; i < numWorkloads; i++)  
   cout<<"        " << i << " - " << workloads[i] << std::endl;

 cout<<"    p - page size in bytes (default 4096)" << std::endl;
 cout<<"    a - associativity (default direct mapped)" << std::endl;
 cout<<"    i - core id (default all cores)" << std::endl;
 cout<<"    o - number of operations to execute (default all operations in trace)" << std::endl;
 cout<<"    t - path traces (default /tmp)" << std::endl;
 cout<<"    r - path results (default /tmp)" << std::endl;
 cout<<"    v - number of vaults (default 64)" << std::endl;
 cout<<"    wp - number of wp entries (default 1024)" << std::endl;
 cout<<"    s - size of the workload trace (default 8)" << std::endl;
}

bool parseArgs(int argc, char ** argv, unsigned &cacheSize, unsigned &workload, unsigned &regionSize, unsigned &assoc, short &idx, long long int &operations, char **tpath, char **rpath, unsigned int &vault, unsigned int &wp, unsigned short &traceSize){

  if(argc < 3){ 
    printUsage("Not enough args"); 
    return false;
  }

  cacheSize = atoi(argv[1]);
  workload=atoi(argv[2]);

  if (argc >= 4) 
    regionSize=atoi(argv[3]);
  else 
    regionSize = 4096;

  if (argc >= 5) 
    assoc=atoi(argv[4]);
  else 
    assoc=1;

  if (cacheSize < 1 || cacheSize > 32768){
    printUsage("Memory Size < 1 or > 32768MB "); 
    return false;
  }

  if ((regionSize>4096) || (regionSize ==1)){
    printUsage("Region Size 1 or more than 4096B"); 
    return false;
  }

  if (argc >= 6) 
    idx= atoi(argv[5]);
  else idx = -1;

  if (argc >= 7) 
    operations = (long long int) strtoull(argv[6], NULL, 10);
  else operations = -1;

  if (argc >= 8){
    *tpath = (char *) malloc(strlen(argv[7] + 1));
    strcpy(*tpath, argv[7]);
  }else{
    *tpath = (char *) malloc(strlen("/tmp/") + 1);
    strcpy(*tpath, "/tmp/");
  }

  if (argc >= 9){
    *rpath = (char *) malloc(strlen(argv[8] + 1));
    strcpy(*rpath, argv[8]);
  }else{
    *rpath = (char *) malloc(strlen("/tmp/") + 1);
    strcpy(*rpath, "/tmp/");
  }

  if (argc >= 10) 
    vault= (unsigned int) atoi(argv[9]);
  else vault = 64;

  if (argc >= 11) 
    wp= (unsigned int) atoi(argv[10]);
  else wp = 1024;

  if (argc >= 12) 
    traceSize= (unsigned short) atoi(argv[11]);
  else traceSize = 8;

  return true;
}

int main(int argc, char ** argv){
  unsigned int workload, cacheSize, regionSize, associativity;
  short idx;
  long long int max_ops;
  char *trace_path;
  char *result_path;
  unsigned int vault, wp;
  unsigned short traceSize;

  if(!parseArgs(argc, argv, cacheSize, workload, regionSize, associativity, idx, max_ops, &trace_path, &result_path, vault, wp, traceSize)){
    return false;
  }

  std::cout<<"======================Information======================" << std::endl;
  std::cout<<"Thread: " << idx << std::endl;
  std::cout<<"Max Operations: " << max_ops << std::endl;
  std::cout<<"Trace Path: " << trace_path << std::endl;
  std::cout<<"Result Path: " << result_path << std::endl;
  std::cout<<"Workload: " << workloads[workload] << std::endl;
  std::cout<<"Trace Size: " << traceSize << std::endl;
  std::cout<<"=======================================================" << std::endl;

  Fast3DCache* component = new Fast3DCache(workload,  regionSize,    cacheSize,   associativity, result_path, vault, wp);
  std::string str(trace_path);
  str.append(workloadPaths[workload]);

  switch(traceSize){
    case S8:
      str.append("-8gb");
      break;
    case S16:
      str.append("-16gb");
      break;
    case S32:
      str.append("-32gb");
      break;
    case S64:
      str.append("-64gb");
      break;
    default:
      printf("Error: Trace size not 8/16/32/64\n");
      delete component;
      return false;
  }

  str.append(".out");
  char* cstr = new char [ str.size() + 1];
  strcpy (cstr, str.c_str());
  gzFile input = gzopen (cstr, "rb");
  compressedRecord message;
  uint64_t ops = 0;

  std::cout<<"Input file : " << str << std::endl;
  std::cout<<"=======================================================" << std::endl;

  if (input == Z_NULL){
    printf("Error: Cannot open trace file for %s.\n", cstr);
    gzclose(input);
    delete component;
    return false;
  }

  while(!gzeof(input)){ //Memory Accesses

     if ((max_ops != -1) && (ops > (unsigned long long int) max_ops))
       break;

     int code=gzread(input, &message, sizeof(compressedRecord));

     if(code <= 0) {
	     std::cout<< "Error, code=" << code << endl;
       int err=0;
       std::cout << gzerror(input, &err) << endl;
	     std::cout << "gzerror code=" << err <<endl;
       break;
     } 

     ops++;
     
     //printf("Number of ops %llu...\n", (unsigned long long) ops);
 
     component->fromL2(message, idx);
  }

  printf("Ops count: %llu\n", (unsigned long long int) ops);
 
  component->saveStats(); 
  gzclose(input);
  delete component;
  return true;
}

