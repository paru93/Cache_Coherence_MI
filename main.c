#include <stdio.h>
#include <stdlib.h>
//#include "includes.h"
#define CACHELATENCY 1
#define MAINMEMLATENCY 20
#define MAXPROCS 2
#define CACHELINESIZE 256//For testing
#define MAINLINESIZE 1024
int index_bits=0;
typedef struct
{
	char proc_id;
	unsigned char index;
	int addr;
	char rw;
	char data;
	int tag;
}CacheCtrl_t;
unsigned char  read,write,readfrom_cache,readfrom_main_mem,writeback_main_mem;
int miscount=0,hitcount=0,memcycles=0;
typedef enum
{
	I = 0,
	M = 1
}MI_t;

typedef enum
{
	WRITE,
	READ
}memOpr_t;

typedef enum
{
	CACHE_EMPTY,
	FOUND_IN_CACHE,
	NOTFOUND_IN_CACHE
}CacheDataFind_t;

typedef struct
{
    char proc_id;
    int addr;
    char RW;
    int data;
} upReq_t;

typedef struct
{
    MI_t mi;
	char data;
	int tag;
}CacheMem_t;
static CacheMem_t  cacheMem[MAXPROCS][CACHELINESIZE];	// initialize the cache memory
int total_memory_latency=0;

unsigned int LOG2( unsigned int x )
{
  unsigned int ans = 0 ;
  while( x>>=1 ) ans++;
  return ans ;
}
void MICtrllr(CacheCtrl_t *cc,unsigned char *writeback_main_mem,char Proc_ID)
{
    MI_t nextState;

	//Snoop
	if(Proc_ID!=cc->proc_id)
	{
		switch(cacheMem[Proc_ID-1][cc->index].mi)
		{
			case M: // This case will not arise because in invalid the Read/Write flags are set before entering
                    // this loop
					//when there is a writeback from snooping processor cache, tags match with the request hence the Invalidate the block
					if(*writeback_main_mem==1)
                    {
                        nextState=I;
                    }
                    else
                    {
                        nextState=M;
                    }
					break;

            case I:	nextState = I;
					break;

		}
	}
	else
    {
        switch(cacheMem[Proc_ID-1][cc->index].mi)
		{
			case M: nextState=M;
                    break;

            case I:	nextState = M;
					break;

        }

    }
	cacheMem[Proc_ID-1][cc->index].mi = nextState;
}


void cacheMemAddrCheck(CacheCtrl_t *cc,unsigned char *readfrom_main_mem, unsigned char *writeback_main_mem,char Proc_ID)
{
    // Clear the signals which are used to indicate the other cache or memory that the data was not found
    *readfrom_main_mem = 0;
    *writeback_main_mem = 0;


    // check if the cache location is empty or contains an invalid address/tag
    //For processor requesting the data
	if(cc->proc_id==Proc_ID)
	{

		if(cc->tag != cacheMem[Proc_ID-1][cc->index].tag || (cacheMem[Proc_ID-1][cc->index].mi==I))
		{
            // according to read/write operation the respective signal is asserted
			 miscount++;
			//when the cache has different tag at same address, evict and read from read new data at requested memory location
			if((cacheMem[Proc_ID-1][cc->index].mi==I))
            {
                *writeback_main_mem=0;

            }
			*readfrom_main_mem=1;
			//memcycles+=2*MAINMEMLATENCY;
		}
		else
		{
			//Tags match hence read from cache
			hitcount++;
			memcycles+=CACHELATENCY;
		}
	}
    else    //snooping processor
    {
        //*memRd = 0;     // no need to go to memory for reading the data
        //*Hit = 1;       // found in the cache location
		if(cc->tag == cacheMem[Proc_ID-1][cc->index].tag)
		{
			//respond to invalidate by writing the data back to main memory
			*writeback_main_mem = 1;
            // according to read/write operation the respective signal is asserted
		}

    }
}
unsigned char CacheReadWrite(CacheCtrl_t *cc,char Proc_ID)
{
    if(cc->rw)
    {
        return (cacheMem[Proc_ID-1][cc->index].data);
    }
    else
    {
        cacheMem[Proc_ID-1][cc->index].data=cc->data;
        return (cc->data);
    }
}


void SharedMem(unsigned char RW,CacheCtrl_t *cc,char Proc_ID)
{
	static unsigned char mem[MAINLINESIZE] = {0};
	//static unsigned char cnt = 0;	// count for the delay
	//unsigned short delaycnt = 0;
	int eviction_address=0;

	//if(cnt < 5)
	//  for(delaycnt = 0; delaycnt < 1000000; delaycnt++);
	//else
    //{
		if(RW)		// for read operation
		{
		    printf("%x",cc->tag);
			cacheMem[Proc_ID-1][cc->index].data=mem[cc->tag];		// return the data requested from teh mem
            printf("cacheMem_containts=%d    data mem=%d\n",cacheMem[Proc_ID-1][cc->index].data,mem[cc->tag]);
		}
		else		// for write operation
		{

			//eviction_address=((cacheMem[Proc_ID-1][cc->index].tag)<<8) | (cc->index);
			//eviction_address=((cacheMem[Proc_ID-1][cc->index].tag)) | (cc->index);
			eviction_address=((cacheMem[Proc_ID-1][cc->index].tag));

			printf("eviction_address=%d\n",eviction_address);
			mem[eviction_address]=cacheMem[Proc_ID-1][cc->index].data;// write the data in the memory
			printf("cacheMem_containts=%d    data mem=%d\n",cacheMem[Proc_ID-1][cc->index].data,mem[eviction_address]);
		}
	//}


}
void cacheMem_initialize()
{
    int no_proc,cache_lines;
    for(no_proc=0;no_proc<MAXPROCS;no_proc++)
    {
        for(cache_lines=0;cache_lines<CACHELINESIZE;cache_lines++)
        {
            cacheMem[no_proc][cache_lines].data=0x0;
            cacheMem[no_proc][cache_lines].mi=I;
            cacheMem[no_proc][cache_lines].tag=0x0;
        }

    }

}

int main()
{
    CacheCtrl_t cc;
    int snoop_id;
    unsigned char RW;
    cacheMem_initialize();
    cc.data=0x44;
    cc.index=2;
    cc.proc_id=2;
    cc.rw=0;
    cc.tag=0x55;
   // CacheMem_t CacheMem;
    cacheMem[cc.proc_id-1][cc.index].data=0x23;
    cacheMem[cc.proc_id-1][cc.index].mi=I;
    cacheMem[cc.proc_id-1][cc.index].tag=0x55;

    cacheMem[cc.proc_id-2][cc.index].data=0x34;
    cacheMem[cc.proc_id-2][cc.index].mi=M;
    cacheMem[cc.proc_id-2][cc.index].tag=0x54;
   //printf("data=%d  mi=%d   tag=%d\n",cacheMem[cc.proc_id-1][cc.index].data,cacheMem[cc.proc_id-1][cc.index].mi,cacheMem[cc.proc_id-1][cc.index].tag);

    snoop_id=(cc.proc_id==1)?2:1;
    cacheMemAddrCheck(&cc,&readfrom_main_mem,&writeback_main_mem,snoop_id);
    printf("readfrom_main_mem=%d    writeback_main_mem=%d\n",readfrom_main_mem,writeback_main_mem);

    //if after snooper's response if tags matched, writeback and read from main memory
    if(writeback_main_mem==1)
    {
        RW=0;
        SharedMem(RW,&cc,snoop_id);
    }

    //Update the snooper's MI state
    MICtrllr(&cc,&writeback_main_mem,snoop_id);
    //printf("Cache snoop=%x  mi=%d   tag=%x\n",cacheMem[snoop_id][cc.index].data,cacheMem[snoop_id][cc.index].mi,cacheMem[snoop_id][cc.index].tag);
    writeback_main_mem=0;
    cacheMemAddrCheck(&cc,&readfrom_main_mem,&writeback_main_mem,cc.proc_id);


   // printf("Cache snoop=%x  mi=%d   tag=%x\n",cacheMem[snoop_id][cc.index].data,cacheMem[snoop_id][cc.index].mi,cacheMem[snoop_id][cc.index].tag);

    if(writeback_main_mem==1 || readfrom_main_mem==1)
    {
        //write followed by read
        //When tags don't match
        if(writeback_main_mem==1)
        {
           RW=0;
           SharedMem(RW,&cc,cc.proc_id);
        }
        RW=1;
        SharedMem(RW,&cc,cc.proc_id);
    }
    CacheReadWrite(&cc,cc.proc_id);
   // printf("Cache snoop=%x  mi=%d   tag=%x\n",cacheMem[snoop_id][cc.index].data,cacheMem[snoop_id][cc.index].mi,cacheMem[snoop_id][cc.index].tag);
    MICtrllr(&cc,&writeback_main_mem,cc.proc_id);
    printf("Cache req has data=%x  mi=%d   tag=%x\n",cacheMem[cc.proc_id-1][cc.index].data,cacheMem[cc.proc_id-1][cc.index].mi,cacheMem[cc.proc_id-1][cc.index].tag);
    printf("Cache snoop has data=%x  mi=%d   tag=%x\n",cacheMem[snoop_id-1][cc.index].data,cacheMem[snoop_id-1][cc.index].mi,cacheMem[snoop_id-1][cc.index].tag);
    //Check the tag o
    //if()
    return 0;
}
