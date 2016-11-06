#include <core/stats.hpp>
#include "common.hpp"
#include "CacheStats.hpp"
#include "PageCache.hpp"
#include <zlib.h>
#include <fstream>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

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

uint64_t size; //Size of the page table
uint64_t region_size;
uint64_t numVaults;

struct DataItem** hashArray; 
struct DataItem* dummyItem;
struct DataItem* item;

//Stats
unsigned long long int inserts = 0;
unsigned long long int deletes = 0;
unsigned long long int conflicts = 0;
unsigned long long int retries = 0;
unsigned long long int searches = 0;
unsigned long long int hits = 0;
unsigned long long int misses = 0;
unsigned long long int removesFailed = 0;

unsigned long long int hashCode(unsigned long long int key) {
/*
   unsigned long long int key_aux = key;
   unsigned long long int c;
   unsigned long long int hash= 0;

   c = key_aux % size;
   key_aux = key_aux / size;
   hash = hash ^ c;
   c = key_aux % size;
   key_aux = key_aux / size;
   hash = hash ^ c;
   c = key_aux % size;
   key_aux = key_aux / size;
   hash = hash ^ c;

   return hash % size;
*/
  // unsigned long long int aux = EXTRACT_BITS(key, 19, 29);
   
 //  return (key ^ aux) % size;
   
   
  return key % size;

}
/*
unsigned long long int hashCode(unsigned long long int key) {
  key = (~key) + (key << 21); // key = (key << 21) - key - 1;
  key = key ^ (key >> 24);
  key = (key + (key << 3)) + (key << 8); // key * 265
  key = key ^ (key >> 14);
  key = (key + (key << 2)) + (key << 4); // key * 21
  key = key ^ (key >> 28);
  key = key + (key << 31);
  return key % size;
}
*/
unsigned long long int computeVPN(unsigned long long int key) {
  return key/region_size;
  //return key;
}

unsigned long long int computeVault(unsigned long long int key) {
   return key % numVaults;
}

//struct DataItem *search(unsigned long long int key) {
struct DataItem *search(unsigned long long int key, unsigned short pid) {
   unsigned short conflicts_tmp = 0;
   unsigned long long int retries_tmp = 0;
   unsigned long long counter=0;

   //get the hash 
   unsigned long long int hashIndex = hashCode(key);  

   ++searches;

   if((hashArray[hashIndex] != NULL) && (((hashArray[hashIndex]->key != (long long int) key)) || (hashArray[hashIndex]->pid != pid))){
     //++conflicts;
     ++conflicts_tmp;
     //printf("Conflict in %llu\n", hashIndex);
   }
	
   //move in array until an empty 
   while((hashArray[hashIndex] != NULL) && (counter < size)) {
      //printf("Index: %llu\n", hashIndex);
	
      if((hashArray[hashIndex]->key == (long long int) key) && (hashArray[hashIndex]->pid == pid)){
         ++hits;
         conflicts += conflicts_tmp;
         retries += retries_tmp;
         return hashArray[hashIndex]; 
      }
			
      //go to next cell
      ++hashIndex;

      ++counter;

      //account for the retry
      //++retries;
      ++retries_tmp;
		
      //wrap around the table
      hashIndex %= size;
   }      

   conflicts += conflicts_tmp;
   retries += retries_tmp;
	
   ++misses;
   return NULL;        
}

struct DataItem *get_first_key(long long int key){
   unsigned long long int hashIndex = hashCode(key);  
   unsigned long long int counter = 0;

   //move in array until an empty or deleted cell
   //while(hashArray[hashIndex] != NULL) {
   while(counter < size) {

      if ((hashArray[hashIndex] != NULL) && (hashArray[hashIndex]->key != -1)){
        return hashArray[hashIndex];
      }

      //go to next cell
      ++hashIndex;
		
      //wrap around the table
      hashIndex %= size;

      counter++;
   }

   return NULL;
}

struct DataItem *get_last_key(long long int key){
   unsigned long long int hashIndex = hashCode(key);  
   unsigned long long int counter = 0;

   //if ((hashArray[hashIndex] != NULL) && (hashArray[hashIndex]->key != -1)){
   //   return hashArray[hashIndex];
   //}


   //move in array until an empty or deleted cell
   //while(hashArray[hashIndex] != NULL) {
   while(counter < size) {

      //if ((hashArray[hashIndex] != NULL) && (hashArray[hashIndex]->key != -1)){
      if ((hashArray[hashIndex] == NULL) || (hashArray[hashIndex]->key == -1)){
        if (counter == 0){
          return hashArray[hashIndex];
        }else{
          if (hashIndex != 0){
            return hashArray[(hashIndex - 1)];
          }else{
            return hashArray[size - 1];
          }
        }
      }

      //go to next cell
      ++hashIndex;
		
      //wrap around the table
      hashIndex %= size;

      counter++;
   }

   return NULL;
}

bool replace(long long int key, int data, unsigned short pid) {

//   printf("Enter!\n");

   //struct DataItem *item = (struct DataItem*) malloc(sizeof(struct DataItem));
   struct DataItem *item;
   //item->data = data;  
   //item->key = (long long int) key;
   //item->pid = pid;

   //get the hash 
   unsigned long long int hashIndex = hashCode(key);

   //++inserts;

   //move in array until an empty or deleted cell
   while(hashArray[hashIndex] == NULL) {
      //go to next cell
      ++hashIndex;
		
      //wrap around the table
      hashIndex %= size;
   }

   if (hashArray[hashIndex]->key != -1){
     item= hashArray[hashIndex];
     item->data = data;  
     item->key = key;
     item->pid = pid;
     return true;
   }else{
     return false;
   }

/*
   if ((hashArray[hashIndex] != NULL) && (hashArray[hashIndex]->key != -1)){
     printf("True!\n");
     item= hashArray[hashIndex];

     item->data = data;  
     item->key = key;
     item->pid = pid;
     return true;
   }else{
     //printf("False!\n");
     return false;
   }
   */
}

void insert(long long int key, int data, unsigned short pid) {

   struct DataItem *item = (struct DataItem*) malloc(sizeof(struct DataItem));
   item->data = data;  
   item->key = (long long int) key;
   item->pid = pid;

   //get the hash 
   unsigned long long int hashIndex = hashCode(key);

   ++inserts;

   //move in array until an empty or deleted cell
   while(hashArray[hashIndex] != NULL && hashArray[hashIndex]->key != -1) {
      //go to next cell
      ++hashIndex;
		
      //wrap around the table
      hashIndex %= size;
   }
	
   hashArray[hashIndex] = item;
}

struct DataItem* erase(struct DataItem* item) {
   long long int key = item->key;
   unsigned short pid = item->pid;

   //get the hash 
   unsigned long long int hashIndex = hashCode(key);

   //++deletes;

   //move in array until an empty
   while(hashArray[hashIndex] != NULL) {
	
      if((hashArray[hashIndex]->key == key) && (hashArray[hashIndex]->pid == pid)) {
         struct DataItem* temp = hashArray[hashIndex]; 
			
         ++deletes;
         //assign a dummy item at deleted position
         hashArray[hashIndex] = dummyItem; 
         return temp;
      }
		
      //go to next cell
      ++hashIndex;
		
      //wrap around the table
      hashIndex %= size;
   }      
	
   return NULL;        
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

  printf("Enter!\n");

  for (unsigned long long int i = 0; i < size; i++){
    //printf("Index: %llu\n", i);
    if ((hashArray[i] != NULL) && (hashArray[i]->key != -1)){
      inUse++;
    }
  }
  printf("Exit!\n");

  std::cout<<"=====================================================" << std::endl;
  printf("Inserts: %llu\n", inserts);
  printf("Deletes: %llu\n", deletes);
  printf("Searches: %llu\n", searches);
  printf("Conflicts: %llu\n", conflicts);
  printf("Retries: %llu\n", retries);
  if (conflicts != 0)
    printf("Avg retry: %llu\n", retries/conflicts);
  if (searches != 0)
    printf("Avg probes: %f\n", ((float) (searches + retries))/searches);
  printf("Hits: %llu\n", hits);
  printf("Misses: %llu\n", misses);
  printf("Hit ratio: %f\n", 100*((float)hits)/(hits+misses));
  if (size != 0)
    printf("Load factor: %f\n", 100*((float)inUse)/size);
  if (searches != 0)
    printf("Fraction of conflicts: %f\n", 100*((float)conflicts)/searches);
  printf("Failed removes: %llu\n", removesFailed);
  std::cout<<"=====================================================" << std::endl;
}

void printUsage(string s){
 unsigned int numWorkloads = sizeof(workloads)/sizeof(workloads[0]);

 cout<<s<<endl;
 cout<<"Usage: Fast3DCacheImpl m w (p) (i) (o) (t) (r) (v) (vs) (s) (np) (f)"<< std::endl;
 cout<<"    m - vault size in MB"<< std::endl;
 cout<<"    w - workload (0-" << numWorkloads - 1 << ")" << std::endl;

 for(unsigned int i=0; i < numWorkloads; i++)  
   cout<<"        " << i << " - " << workloads[i] << std::endl;

 cout<<"    p - page size in bytes (default 4096)" << std::endl;
 cout<<"    i - core id (default all cores)" << std::endl;
 cout<<"    o - number of operations to execute (default all operations in trace)" << std::endl;
 cout<<"    t - path traces (default /tmp)" << std::endl;
 cout<<"    r - path results (default /tmp)" << std::endl;
 cout<<"    v - number of vaults (default 64)" << std::endl;
 cout<<"    vi - vault id (default 0)" << std::endl;
 cout<<"    s - size of the workload trace (default 8)" << std::endl;
 cout<<"    np - number of workload traces (default 1)" << std::endl;
 cout<<"    f - factor of page table compared to the vault size (default 1)" << std::endl;
}

bool parseArgs(int argc, char ** argv, unsigned &vaultSize, unsigned &workload, unsigned &regionSize, short &idx, long long int &operations, char *tpath, char *rpath, unsigned int &vault, unsigned int &vaultID, unsigned short &traceSize, unsigned short &numTraces, unsigned short &factor){

  if(argc < 3){ 
    printUsage("Not enough args"); 
    return false;
  }

  vaultSize = atoi(argv[1]);
  workload=atoi(argv[2]);

  if (argc >= 4) 
    regionSize=atoi(argv[3]);
  else 
    regionSize = 4096;

  if (vaultSize < 1 || vaultSize > 32768){
    printUsage("Memory Size < 1 or > 32768MB "); 
    return false;
  }

  if ((regionSize>4096) || (regionSize ==1)){
    printUsage("Region Size 1 or more than 4096B"); 
    return false;
  }

  if (argc >= 5) 
    idx= atoi(argv[4]);
  else idx = -1;

  if (argc >= 6) 
    operations = (long long int) strtoull(argv[5], NULL, 10);
  else operations = -1;

  if (argc >= 7){
    strcpy(tpath, argv[6]);
  }else{
    strcpy(tpath, "/tmp/");
  }

  if (argc >= 8){
    strcpy(rpath, argv[7]);
  }else{
    strcpy(rpath, "/tmp/");
  }

  if (argc >= 9) 
    vault= (unsigned int) atoi(argv[8]);
  else vault = 64;

  if (argc >= 10) 
    vaultID= (unsigned int) atoi(argv[9]);
  else vaultID = 0;

  if (argc >= 11) 
    traceSize= (unsigned short) atoi(argv[10]);
  else traceSize = 8;

  if (argc >= 12) 
    numTraces= (unsigned short) atoi(argv[11]);
  else numTraces = 1;

  if (numTraces > 256){
    printUsage("Number of traces < 1 or > 256"); 
    return false;
  }

  if (argc >= 13) 
    factor= (unsigned short) atoi(argv[12]);
  else factor = 1;

  return true;
}

int main(int argc, char ** argv){
  unsigned int workload, vaultSize, regionSize;
  short idx;
  long long int max_ops;
  char trace_path[80];
  char result_path[80];
  unsigned int vault, vaultID;
  unsigned short traceSize;
  unsigned short numTraces;
  unsigned short factor;
  unsigned short pids[256];

  if(!parseArgs(argc, argv, vaultSize, workload, regionSize, idx, max_ops, trace_path, result_path, vault, vaultID, traceSize, numTraces, factor)){
    return false;
  }

  //Size of the page table
  size = ((vaultSize*1024*1024)/regionSize)*factor;
  //size = vaultSize*factor;
  region_size = regionSize;
  numVaults = vault;

  std::cout<<"======================Information======================" << std::endl;
  std::cout<<"Thread: " << idx << std::endl;
  std::cout<<"Max Operations: " << max_ops << std::endl;
  std::cout<<"Trace Path: " << trace_path << std::endl;
  std::cout<<"Result Path: " << result_path << std::endl;
  std::cout<<"Workload: " << workloads[workload] << std::endl;
  std::cout<<"Trace Size: " << traceSize << std::endl;
  std::cout<<"Number of traces: " << numTraces << std::endl;
  std::cout<<"======================PageTable======================" << std::endl;
  std::cout<<"Number of entries: " << size << std::endl;
  std::cout<<"Number of vaults: " << vault << std::endl;
  std::cout<<"VaultID: " << vaultID << std::endl;
  std::cout<<"Factor page table: " << factor << std::endl;
  std::cout<<"=====================================================" << std::endl;

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

  //Initialize Inverted page table
  hashArray = (struct DataItem**) malloc(sizeof(struct DataItem*) * size); 

  if (hashArray == NULL){
    printf("Error: Malloc for the page table failed\n");
    exit(-1);
  }

  for (unsigned long long i=0; i < size; i++){
    hashArray[i] = NULL;
  }

  dummyItem = (struct DataItem*) malloc(sizeof(struct DataItem));

  if (hashArray == NULL){
    printf("Error: Malloc for the dummy item failed\n");
    exit(-1);
  }

  dummyItem->data = -1;
  dummyItem->key = -1;

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
    //pids[i] = 0;
  }

  while(!gzeof(input)){ //Memory Accesses

     if ((max_ops != -1) && (ops > (unsigned long long int) max_ops))
       break;

     int code=gzread(input, &message, sizeof(compressedRecord));
     //int code = 1;

     //message.address = rand() % (size);

     if(code <= 0) {
	     std::cout<< "Error, code=" << code << endl;
       int err=0;
       std::cout << gzerror(input, &err) << endl;
	     std::cout << "gzerror code=" << err <<endl;
       break;
     } 

     ops++;
    
     if ((ops % 10000000) == 0){
     //if ((ops % 10000) == 0){
       printf("Number of ops %llu..., load factor: %f, factor limit: %f\n", (unsigned long long) ops, ((float) (inserts-deletes))/size, ((float) 1)/factor);
     }

     //if (computeVault(computeVPN(message.address)) != vaultID){ //Skip if the address does not belong to the vault
     //  continue;
     //}

     for (unsigned short pid = 0; pid < numTraces; pid++){ //Number of processes we are simulating
       if (computeVault(computeVPN(message.address) ^ pids[pid]) != vaultID){ //Skip if the address does not belong to the vault
         continue;
       }
       
       item = search(computeVPN(message.address) ^ pids[pid], pids[pid]);
       //item = search(computeVPN(message.address), pid);
       //item = search(computeVPN(message.address | ((unsigned long long int) pid << 48)), pid);

       //printf("Address: %llx, VPN: %llx, Hash: %llu\n", (unsigned long long int) message.address, (unsigned long long int) computeVPN(message.address), (unsigned long long int) hashCode(message.address));
       //printf("Address: %llx, VPN: %llx, pid: %d, VPN+PID: %llx, Hash: %llu\n", (unsigned long long int) message.address, (unsigned long long int) computeVPN(message.address), pid, (unsigned long long int) computeVPN(message.address | ((unsigned long long int) pid << 48)), (unsigned long long int) hashCode(computeVPN(message.address | ((unsigned long long int) pid << 48))));
       //printf("Address: %llx, VPN: %llx, pid: %d, VPN XOR ASID: %llx, Hash: %llu, HashXOR: %llu\n", (unsigned long long int) message.address, (unsigned long long int) computeVPN(message.address), pids[pid], (unsigned long long int) computeVPN(message.address) ^ pids[pid], (unsigned long long int) hashCode(computeVPN(message.address)), (unsigned long long int) hashCode(computeVPN(message.address) ^ pids[pid]));
       //printf("Address: %llx, VPN: %llu, pid: %d, VPN XOR ASID: %llx, Hash: %llu, HashXOR: %llu\n", (unsigned long long int) message.address, (unsigned long long int) computeVPN(message.address), pids[pid], (unsigned long long int) computeVPN(message.address) ^ pids[pid], (unsigned long long int) hashCode(computeVPN(message.address)), (unsigned long long int) hashCode(computeVPN(message.address) ^ pids[pid]));

       if (item == NULL){

         if ((((float) (inserts - deletes))/size) >= (((float) 1)/factor)){
           bool replaced = replace(computeVPN(message.address) ^ pids[pid], 0, pids[pid]);

           if (!replaced){
             ++removesFailed;
           }

         /*
           struct DataItem *tmp_item = get_first_key(computeVPN(message.address) ^ pids[pid]);
           //struct DataItem *tmp_item = get_last_key(computeVPN(message.address) ^ pids[pid]);
           //struct DataItem *tmp_item = get_first_key(rand() % (size));

           if (tmp_item != NULL){
             erase(tmp_item);
           }else{
             ++removesFailed;
           }
*/
           /*
           bool replaced = replace(computeVPN(message.address) ^ pids[pid], 0, pids[pid]);

           if (!replaced){
             struct DataItem *tmp_item = get_first_key(rand() % (size));
             //struct DataItem *tmp_item = get_first_key(computeVPN(message.address) ^ pids[pid]);

             if (tmp_item != NULL){
               erase(tmp_item);
             }else{
               ++removesFailed;
             }
             
             insert(computeVPN(message.address) ^ pids[pid], 0, pids[pid]);
           }
           */
           //insert(computeVPN(message.address) ^ pids[pid], 0, pids[pid]);
         }else{

           insert(computeVPN(message.address) ^ pids[pid], 0, pids[pid]);
         }
       }
     }
  }

  printf("Ops count: %llu\n", (unsigned long long int) ops);
 
  displayStats();
  //display();
  gzclose(input);
  return true;
}

