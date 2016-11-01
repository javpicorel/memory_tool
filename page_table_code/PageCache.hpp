#ifndef FAST_PAGE_CACHE
#define FAST_PAGE_CACHE

#include <map>
#include <fstream>
#include "CacheStats.hpp"
#include "common.hpp"

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

#define LRU_BASELINE

typedef unsigned char state_t;

unsigned long allOnes(unsigned int region_size){
    switch(region_size){
        case 128:  return 0x3; //2^2 -1;
        case 256:  return 0xF; //2^4 -1;
        case 512:  return 0xFF; //2^8 -1;
        case 1024: return 0xFFFF; //2^16 -1;
        case 2048: return 0xFFFFFFFF; //2^32 -1;
        case 4096: return 0xFFFFFFFFFFFFFFFF; //2^64 -1;
        default :
            assert(false);
            return 0;
    }
}

//for pages
const state_t kInvalid = 0x0;
const state_t kValid = 0x1;
//FIXME add dirty??

//for blocks
const state_t kTouched = 0x1;
const state_t kDirty = 0x2;
const state_t kInvalidated = 0x4;
const state_t kDowngraded = 0x8;
const state_t kTouchedRead = 0x10;

//operations
const int kRead = 0;
const int kWrite=1;

struct TAGEntry{
    unsigned long address;   // full triggering address
    unsigned long tag;       // 
    unsigned int  lru_time;
    state_t pageState;
    unsigned short pid;
};

//helper functions


//returns last $bits bits of $number in the reverse order
unsigned long reverse(unsigned long number, int bits){
	unsigned long result = 0;
	for(int i=0; i<bits; i++) {
		result = result <<1;
		if(number % 2 == 1) result++;
		number = number >>1;
	}
	return result;
}

unsigned countOnes(unsigned long number) {
	unsigned int result = 0;
	while (number>0) {
		if(number%2 ==1) result++;
		number = number>>1;
	}
	return result;
}


class WayPredictor{
public:
 unsigned int SIZE;
 unsigned int VAULTS;
 unsigned int foldCount;

 virtual unsigned hash(unsigned long key){
    key=key/VAULTS;
    unsigned int c, hashed=0;
    /*
    int foldCount=0;
    switch(SIZE){
     case 1024:
        foldCount=1; //2x10bit
     case 512:
        foldCount=1; //2x9bit
     case 256:
        foldCount=2; //2x8bit
        break;
     case 128:
        foldCount=2; //2x7bit
        break;
     case 64:
        foldCount=3; //3x6bit
        break;
     case 32:
        foldCount=3; //3x5bit
        break;
     case 16:
        foldCount=4; //4x4bit
        break;
     case 8:
        foldCount=5; //5x3bit
        break;
     case 4:
        foldCount=8; //7x2bit
        break;
     case 2:
        foldCount=15; //15x1bit
        break;
     case 1:
        return 0;
        break;
      default: 
        exit(-1);
    }
    */
    for(unsigned int i=0; i<foldCount; i++){
        c= key % SIZE;
        key/= SIZE;
        hashed^= c;
     }
   return hashed;
  }

 WayPredictor(unsigned int aNumVault, unsigned int aNumWP, unsigned int aFoldCount){
    SIZE = aNumWP;
    VAULTS = aNumVault;
    table = (unsigned int *) malloc(sizeof(unsigned int) * SIZE);
    memset(table, 0, sizeof(unsigned int) * SIZE);
    foldCount= aFoldCount;
 }

  unsigned get(unsigned long key){
    return table[hash(key)];
  }

  void update(unsigned long key, unsigned way){
    unsigned int index=hash(key);
    table[index]=way;
  }

  void writeWayPredictor(gzFile& output){
    gzwrite(output, &(table[0]), SIZE*sizeof(unsigned int));
  }

  void readWayPredictor(gzFile& input){
    gzread(input, &(table[0]), SIZE*sizeof(unsigned int));
  }

  virtual ~WayPredictor(){};

protected:
 unsigned int *table;
};


class PageCache{
	CacheStats* theStats;

	unsigned int region_size; //in bytes
	unsigned int onChipMemorySize; //in MB
	unsigned int associativity;

  unsigned int numVault;
  unsigned int numWP;

	unsigned long numOfOnChipPages; //number of page frames on the chip
	unsigned long numOfSets;
	unsigned int blocksPerPage;

	TAGEntry *cachedTable; 
  unsigned int workload;

  WayPredictor **wayPredictor;

  unsigned int hash(unsigned long address1){ //returns 0..numOfOnChipPages
    return (address1/region_size)%numOfSets;
  } 

  unsigned long computeTag(unsigned long address){
    return address/region_size;
  }

  public:

	PageCache(unsigned int region_size1,  unsigned onChipMemorySize1, unsigned associativity1, CacheStats* stats1,  unsigned wkld1, unsigned int aNumVault, unsigned int aNumWP) {
		initialize(region_size1,  onChipMemorySize1, associativity1, wkld1, aNumVault, aNumWP);
		theStats=stats1; 
	}
 
	void setStats(CacheStats* stats){
		theStats=stats;
	}

	void initialize(unsigned int region_size1,  unsigned int onChipMemorySize1, unsigned int associativity1, unsigned wkld, unsigned int aNumVault, unsigned long aNumWP){

    //wayPredictor = (WayPredictor **) malloc(sizeof(WayPredictor *) * aNumVault);
		//for(unsigned int i=0; i< aNumVault; i++) 
    //  wayPredictor[i]=new WayPredictor(aNumVault, aNumWP);

		workload=wkld;
		region_size = region_size1;
		onChipMemorySize = onChipMemorySize1;
		associativity = associativity1;
    blocksPerPage =region_size/BLOCK_SIZE;
    numVault = aNumVault;
    numWP = aNumWP;

    numOfOnChipPages=(onChipMemorySize*1024/region_size)*1024; //MB

    if(associativity == 0) associativity = numOfOnChipPages; //Fully associative (TODO: Too slow, need another implementation for full associativity)
      numOfSets = numOfOnChipPages / associativity;

    unsigned int theFoldCount = 0;
    unsigned long numOfSetsVault = numOfSets / aNumVault;

    if (LOG2(aNumWP) != 0){ //Make sure the number of WP entries is not zero, otherwise there is nothing to fold
      if (LOG2(numOfSetsVault) % LOG2(aNumWP) == 0){ //Division between the vault sets and WP entries is even
        theFoldCount = LOG2(numOfSetsVault)/LOG2(aNumWP);
      }else{
        theFoldCount = LOG2(numOfSetsVault)/LOG2(aNumWP) + 1; //Division between sets and WP entries is not even
      }
    }

    wayPredictor = (WayPredictor **) malloc(sizeof(WayPredictor *) * aNumVault);
		for(unsigned int i=0; i< aNumVault; i++) 
      wayPredictor[i]=new WayPredictor(aNumVault, aNumWP, theFoldCount);

		cachedTable=new TAGEntry[numOfOnChipPages]; 

		for(unsigned k=0; k<numOfSets; k++) 
		  for(unsigned l=0; l<associativity; l++) {
  			unsigned i=k*associativity+l;	

			  cachedTable[i].tag= 0L;
			  cachedTable[i].address=0L;
			  cachedTable[i].pageState=kInvalid;
 			  cachedTable[i].lru_time =l;
 			  cachedTable[i].pid = 0;
		}
  
    std::cout<<"=======================================================" << std::endl;
	  printf("region_size %uB\n",region_size);
	  printf("onChipMemorySize %uMB\n",onChipMemorySize);
	  printf("numOfOnChipPages %lu\n",numOfOnChipPages);
	  printf("numOfSets %lu\n",numOfSets);
	  printf("associativity %u\n",associativity);
	  printf("numVaults %u\n", numVault);
	  printf("perWPNumEntries %u\n", numWP);
    std::cout<<"=======================================================" << std::endl;
	}

  void fixPredictor(unsigned long key, unsigned int way){ 
	    unsigned myVault=hash(key) % numVault;
      wayPredictor[myVault]->update(key/region_size, way);
	}

  unsigned int predictWay(unsigned long key){
    unsigned myVault=hash(key) % numVault;
	  return wayPredictor[myVault]->get(key/region_size);
  }
       
	void clearPage(unsigned long address, unsigned int way){
		unsigned long page = address / region_size;
		unsigned long index=hash(address);
		unsigned int ind=index*associativity+way;
		cachedTable[ind].tag=0L;
		cachedTable[ind].address=0L;
		cachedTable[ind].pageState=kInvalid;
		cachedTable[ind].pageState=0;
	}

  void touchAnAddress(unsigned long address, unsigned int way, int op, unsigned short pid){
	  unsigned long tag = computeTag(address);
	  unsigned long index = hash(address);
	  unsigned long ind =index*associativity+way; 
	  DBG_Assert(cachedTable[ind].pageState==kValid,(<<"Accessing an invalid page"));
	  DBG_Assert(cachedTable[ind].tag==tag,(<<"Tag mismatch!"));
	  DBG_Assert(cachedTable[ind].pid==pid,(<<"Pid mismatch!"));
  }

  void insert(unsigned long address, state_t state, unsigned int way,  unsigned int operation, unsigned short pid){
	  fixPredictor(address, way);
	  unsigned long tag = computeTag(address);
	  unsigned long index = hash(address);
	  (theStats->thePageInserts_stat)++;
	  unsigned long ind= index*associativity+way;
	  cachedTable[ind].tag = tag;
	  cachedTable[ind].pageState = state;
	  cachedTable[ind].pid = pid;

    //LRU update
    unsigned old_time=cachedTable[ind].lru_time;
    cachedTable[ind].lru_time = 0;
    for (unsigned i=0; i<associativity; i++)
		  if((i!=way) && (cachedTable[index*associativity+i].lru_time<old_time)) 
        cachedTable[index*associativity+i].lru_time++;

	    cachedTable[ind].address=address;

  }

	const state_t& lookup(unsigned long address, unsigned int &way, unsigned short pid){
		way = 0;
		unsigned long tag = computeTag(address);
		unsigned long index = hash(address);
		for (unsigned int i=0; i < associativity; i++)
	  	  if((cachedTable[index*associativity+i].pageState!=kInvalid) && (cachedTable[index*associativity+i].tag == tag) && (cachedTable[index*associativity+i].pid == pid)){
			    way = i;
			    return cachedTable[index*associativity+i].pageState;
	 	    }
		return kInvalid;
	}

	state_t findVictim(unsigned long address, unsigned int &way,unsigned long &toWriteBack){
		unsigned long index = hash(address);
		int toReplace = -1;

 		bool foundInvalid=false; 

	 	for (unsigned int i=0; i <associativity; i++)
	    if(cachedTable[index*associativity+i].pageState==kInvalid) toReplace = i;
                
    foundInvalid=true; //there might be an invalid one

	  if(toReplace<0) {
		  foundInvalid=false;
		  toReplace=0;

		  for(unsigned int i=1; i<associativity; i++)
		    if(cachedTable[index*associativity+i].lru_time>cachedTable[index*associativity+toReplace].lru_time) toReplace=i;
		}
                
		way = toReplace;
		unsigned long ind=index*associativity+way;
	  toWriteBack = cachedTable[ind].address;

		if(cachedTable[ind].pageState==kInvalid) 
      return kInvalid;

    state_t returnState = cachedTable[ind].pageState; 
    clearPage(address, way);

    return returnState;
  }

  void applyPolicy(unsigned long address, unsigned int way){
    unsigned long page = address / region_size;
    unsigned long index = hash(address);
	  unsigned int old_time = cachedTable[index*associativity+way].lru_time;
	  unsigned long ind=index*associativity+way;

#ifdef LRU_BASELINE
    for(unsigned int i=0; i<associativity; i++)
      if(cachedTable[index*associativity+i].lru_time<old_time) cachedTable[index*associativity+i].lru_time++;
        cachedTable[index*associativity+way].lru_time=0;
#endif
  }

  virtual ~PageCache(){
    delete [] cachedTable;
	  for(unsigned int i= 0; i< numVault; i++){
      delete wayPredictor[i];        	
    }
  }


    
}; //class



#endif //FAST_PAGE_CACHE

