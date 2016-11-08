//#include <core/stats.hpp>
//#include "common.hpp"
//#include "CacheStats.hpp"
//#include "PageCache.hpp"
#include <zlib.h>
#include <fstream>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <iostream>

#include <unordered_map>
#include <list>

#define LOG2(x)         \
  ((x)==1 ? 0 :         \
  ((x)==2 ? 1 :         \
  ((x)==4 ? 2 :         \
  ((x)==8 ? 3 :         \
  ((x)==16 ? 4 :        \
  ((x)==32 ? 5 :        \
  ((x)==64 ? 6 :        \
  ((x)==128 ? 7 :       \
  ((x)==256 ? 8 :       \
  ((x)==512 ? 9 :       \
  ((x)==1024 ? 10 :     \
  ((x)==2048 ? 11 :     \
  ((x)==4096 ? 12 :     \
  ((x)==8192 ? 13 :     \
  ((x)==16384 ? 14 :    \
  ((x)==32768 ? 15 :    \
  ((x)==65536 ? 16 :    \
  ((x)==131072 ? 17 :   \
  ((x)==262144 ? 18 :   \
  ((x)==524288 ? 19 :   \
  ((x)==1048576 ? 20 :  \
  ((x)==2097152 ? 21 :  \
  ((x)==4194304 ? 22 :  \
  ((x)==8388608 ? 23 :  \
  ((x)==16777216 ? 24 : \
  ((x)==33554432 ? 25 : \
  ((x)==67108864 ? 26 : \
  ((x)==134217728 ? 27 : \
  ((x)==268435456 ? 28 : \
  ((x)==536870912 ? 29 : \
  ((x)==1073741824 ? 30 : \
  ((x)==2147483648 ? 31 : \
  ((x)==4294967296 ? 32 : \
  ((x)==8589934592 ? 33 : \
  ((x)==17179869184 ? 34 : \
  ((x)==34359738368 ? 35 : -0xffff))))))))))))))))))))))))))))))))))))

  #define EXTRACT_BITS(VA,STARTBIT,NBITS) ((VA >> STARTBIT) & ((1ULL << NBITS) - 1)) //Extract set bits

/////////////////////////////////////
//Memory map (single chip)
/////////////////////////////////////
// Direct-mapped
// PFN(21) OFFSET (12)
// RAS(12) Bank(4) Vault (5) CAS (12)
/////////////////////////////////////
// Set associative (2-Way)
// PFN (20) OFFFSET (12)
// RAS(11) Bank(4) Vault(5) RAS(0) CAS(11)
/////////////////////////////////////

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

struct DataItem {
   int data;   
   long long int key;
   unsigned short pid;
};

uint64_t size; //Number of sets
uint64_t region_size;
bool scrambling;

struct DataItem** hashArray; 
struct DataItem* dummyItem;
struct DataItem* item;

//Stats
unsigned long long int inserts = 0;
unsigned long long int cold = 0;
unsigned long long int accesses = 0;
unsigned long long int hits = 0;
unsigned long long int misses = 0;

//////////////////////////////////////////////////////////////////////////////////////
unsigned long long int computeVPN(unsigned long long int key) {
  //return key/region_size;
  return key >> 12;
  //return key;
}

unsigned long long int hashCode(unsigned long long int key, unsigned short pid) {
  unsigned long long int tmp = key ^ pid;

  return tmp % size;
}

//////////////////////////////////////////////////////////////////////////////////////
typedef std::pair<long long int, int> key_value_pair_t;
typedef std::list<key_value_pair_t>::iterator list_iterator_t;
uint64_t max_size; //Associativity:

std::list<key_value_pair_t> **cache_items_list_array;
std::unordered_map<long long int, list_iterator_t> **cache_items_map_array;

void put(long long int key, int data, unsigned short pid){
  unsigned long long int setIndex;
  if (scrambling){
    setIndex = hashCode(key, pid);
  }
  else{
    setIndex = hashCode(key, 0);
  }
  
  //printf("Key: %llx, Index: %lld\n", key, setIndex);

  auto it = cache_items_map_array[setIndex]->find(key);

  //printf("Find!\n");

  cache_items_list_array[setIndex]->push_front(key_value_pair_t(key,data));

  //printf("Push front!\n");

  if (it != cache_items_map_array[setIndex]->end()){ //The element was in the set
    //printf("Element was in the set!\n");
    cache_items_list_array[setIndex]->erase(it->second);
    cache_items_map_array[setIndex]->erase(it);
  }

  //printf("Before map!\n");
  (*(cache_items_map_array[setIndex]))[key] = cache_items_list_array[setIndex]->begin();
  //printf("After map!\n");

  if (cache_items_map_array[setIndex]->size() > max_size){
    //printf("Replace!\n");
    auto last = cache_items_list_array[setIndex]->end();
    last--;
    cache_items_map_array[setIndex]->erase(last->first);
    cache_items_list_array[setIndex]->pop_back();
  }else{
    cold++;
  }

}

bool get(long long int key, unsigned short pid) {
  unsigned long long int setIndex;
  if (scrambling){
    setIndex = hashCode(key, pid);
  }
  else{
    setIndex = hashCode(key, 0);
  }
  //unsigned long long int setIndex = hashCode(key, 0);
  //printf("Key: %llx, Index: %lld\n", key, setIndex);
 
  auto it = cache_items_map_array[setIndex]->find(key);

  if (it == cache_items_map_array[setIndex]->end()){
    return false;
  } else {
    cache_items_list_array[setIndex]->splice(cache_items_list_array[setIndex]->begin(), *(cache_items_list_array[setIndex]), it->second);
    return true;
  }
}

bool exists(long long int key){
  unsigned long long int setIndex = hashCode(key, 0);

  //printf("Exists - Key: %llx, Index: %lld, size: %lld, asso: %lld\n", key, setIndex, size, max_size);

  return cache_items_map_array[setIndex]->find(key) != cache_items_map_array[setIndex]->end();
}

void display() {
   unsigned int i = 0;
	
   for(i = 0; i<size; i++) {
	
      if(hashArray[i] != NULL)
         printf(" (%lld,%d,%d)\n",hashArray[i]->key,hashArray[i]->data,hashArray[i]->pid);
      else
         printf(" ~~ \n");
   }
	
   printf("\n");
}

void displayStats() {
  unsigned long long int inUse = 0;

  std::cout<<"=====================================================" << std::endl;
  printf("Inserts: %llu\n", inserts);
  printf("Cold: %llu\n", cold);
  printf("Searches: %llu\n", accesses);
  printf("Hits: %llu\n", hits);
  printf("Misses: %llu\n", misses);
  printf("Miss ratio: %f\n", 100*((float)misses)/(accesses));
  printf("Misses (w/o cold): %llu\n", misses-cold);
  printf("Miss ratio (w/o cold): %f\n", 100*((float)misses-cold)/(accesses));
  std::cout<<"=====================================================" << std::endl;
}

void printUsage(std::string s){
 unsigned int numWorkloads = sizeof(workloads)/sizeof(workloads[0]);

 std::cout<<s<<std::endl;
 std::cout<<"Usage: Fast3DCacheImpl m w (p) (i) (o) (t) (r) (v) (vs) (s) (np) (f)"<< std::endl;
 std::cout<<"    m - vault size in MB"<< std::endl;
 std::cout<<"    w - workload (0-" << numWorkloads - 1 << ")" << std::endl;

 for(unsigned int i=0; i < numWorkloads; i++)  
   std::cout<<"        " << i << " - " << workloads[i] << std::endl;

 std::cout<<"    p - page size in bytes (default 4096)" << std::endl;
 std::cout<<"    a - associativity (default 1)" << std::endl;
 std::cout<<"    i - core id (default all cores)" << std::endl;
 std::cout<<"    o - number of operations to execute (default all operations in trace)" << std::endl;
 std::cout<<"    t - path traces (default /tmp)" << std::endl;
 std::cout<<"    r - path results (default /tmp)" << std::endl;
 std::cout<<"    s - size of the workload trace (default 8)" << std::endl;
 std::cout<<"    np - number of workload traces (default 1)" << std::endl;
 std::cout<<"    h - ASID scrambling (default 0)" << std::endl;
}

bool parseArgs(int argc, char ** argv, unsigned &memorySize, unsigned &workload, unsigned &regionSize, unsigned long long int &associativity, short &idx, long long int &operations, char *tpath, char *rpath, unsigned short &traceSize, unsigned short &numTraces, bool &src){

  if(argc < 3){ 
    printUsage("Not enough arguments\n"); 
    return false;
  }

  memorySize = atoi(argv[1]);
  workload=atoi(argv[2]);

  if (argc >= 4) 
    regionSize=atoi(argv[3]);
  else 
    regionSize = 4096;

  if (memorySize < 1 || memorySize > 32768){
    printUsage("Memory Size < 1 or > 32768MB"); 
    return false;
  }

  if ((regionSize>4096) || (regionSize ==1)){
    printUsage("Region Size 1 or more than 4096B"); 
    return false;
  }

  if (argc >= 5)
    associativity= atoi(argv[4]);
  else
    associativity= 1;

  if (argc >= 6) 
    idx= atoi(argv[5]);
  else idx = -1;

  if (argc >= 7) 
    operations = (long long int) strtoull(argv[6], NULL, 10);
  else operations = -1;

  if (argc >= 8){
    strcpy(tpath, argv[7]);
  }else{
    strcpy(tpath, "/tmp/");
  }

  if (argc >= 9){
    strcpy(rpath, argv[8]);
  }else{
    strcpy(rpath, "/tmp/");
  }

  if (argc >= 10) 
    traceSize= (unsigned short) atoi(argv[9]);
  else traceSize = 8;

  if (argc >= 11) 
    numTraces= (unsigned short) atoi(argv[10]);
  else numTraces = 1;

  if (numTraces > 256){
    printUsage("Number of traces < 1 or > 256"); 
    return false;
  }

  if (argc >= 12){
    src= atoi(argv[11]);
  }else{
    src= 0;
  }

  if (!((src == 0) || (src == 1))){
    printUsage("Address scrambling = 1 or = 0"); 
    return false;
  }

  return true;
}

int main(int argc, char ** argv){
  unsigned workload, regionSize;
  unsigned int memorySize;
  short idx;
  long long int max_ops;
  char trace_path[80];
  char result_path[80];
  unsigned int vault, vaultID;
  unsigned short traceSize;
  unsigned short numTraces;
  unsigned short factor;
  unsigned short pids[256];
  unsigned long long int associativity;

  if(!parseArgs(argc, argv, memorySize, workload, regionSize, associativity, idx, max_ops, trace_path, result_path, traceSize, numTraces, scrambling)){
    return false;
  }

  //Size of the page table
  size = ((unsigned long long int) memorySize*1024*1024)/regionSize/associativity;
  //size = memorySize/associativity; //sets
  max_size = associativity; //assoc
  region_size = regionSize;

  std::cout<<"======================Information======================" << std::endl;
  std::cout<<"Thread: " << idx << std::endl;
  std::cout<<"Max Operations: " << max_ops << std::endl;
  std::cout<<"Trace Path: " << trace_path << std::endl;
  std::cout<<"Result Path: " << result_path << std::endl;
  std::cout<<"Workload: " << workloads[workload] << std::endl;
  std::cout<<"Trace Size: " << traceSize << std::endl;
  std::cout<<"Number of traces: " << numTraces << std::endl;
  std::cout<<"Scrambling: " << scrambling << std::endl;
  std::cout<<"=========================Memory========================" << std::endl;
  std::cout<<"Number of entries: " << memorySize << std::endl;
  std::cout<<"Number of associativity: " << associativity << std::endl;
  std::cout<<"Number of sets: " << size << std::endl;
  std::cout<<"=======================================================" << std::endl;

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
      return false;
  }

  cache_items_list_array = (std::list<key_value_pair_t> **) malloc(sizeof(std::list<key_value_pair_t> *) * size);
  cache_items_map_array = (std::unordered_map<long long int, list_iterator_t> **) malloc(sizeof(std::unordered_map<long long int, list_iterator_t> **) * size);

  for (unsigned long long int i=0; i < size; i++){
    cache_items_list_array[i] = new std::list<key_value_pair_t>;
    cache_items_map_array[i] = new std::unordered_map<long long int, list_iterator_t>;
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
    return false;
  }

  for (int i=0; i < numTraces; i++){
    pids[i] = rand() % 65536; // 16-bit ASID

    printf("PID[%d]=%d\n", i, pids[i]);
    //pids[i] = 0;
    //pids[i] = i;
  }

  while(!gzeof(input)){ //Memory Accesses

     if ((max_ops != -1) && (ops > (unsigned long long int) max_ops))
       break;

     int code=gzread(input, &message, sizeof(compressedRecord));
     //int code = 1;

     //message.address = rand() % (memorySize*2);

     if(code <= 0) {
	     std::cout<< "Error, code=" << code << std::endl;
       int err=0;
       std::cout << gzerror(input, &err) << std::endl;
	     std::cout << "gzerror code=" << err <<std::endl;
       break;
     } 

     ops++;
    
     if ((ops % 10000000) == 0){
     //if ((ops % 10000) == 0){
       printf("Number of ops %llu...\n", (unsigned long long) ops);
     }

     for (unsigned short pid = 0; pid < numTraces; pid++){ //Number of processes we are simulating

       //printf("Address: %llx, VPN: %llx, PID: %x, PID+VPN: %llx\n", message.address, computeVPN(message.address), pids[pid], computeVPN((((unsigned long long int) pids[pid]) << 48) ^ message.address));
       
       //bool hit = exists(computeVPN(message.address));
       bool hit = get( computeVPN( (((unsigned long long int) pids[pid]) << 48)  ^ message.address ) , pids[pid]);
       ++accesses;


       if (!hit){
         //put(computeVPN(message.address), 0);
         put( computeVPN( (((unsigned long long int) pids[pid]) << 48)  ^ message.address ), 0, pids[pid]);

         inserts++;

         misses++;
       }else{
         hits++;
       }
     }
  }

  printf("Ops count: %llu\n", (unsigned long long int) ops);
 
  displayStats();
  //display();
  gzclose(input);
  return true;
}

