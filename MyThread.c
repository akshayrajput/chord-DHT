#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include "MyThread.h"

#define SECOND 1
#define STACK_SIZE 4096

static int currentThread = -1;
static int nThreads = 0;
static TCB threadList[MAXTHREADS];
static int ran = 0;

//static int count_dead = MAXTHREADS;

static void wrapper();
void alarm_handler(int sig);
void initStatistics(struct statistics* stat,int id);
void clean();
void dispatch(int sig);
void yield();
void deleteThread(int threadID);
int createWithArgs(void *(*f)(void *), void *args);
int create(void (*f)(void));
int getID();
void run(int tid);
void suspend(int tid);
void resume(int tid);
void sleep(int secs);
void start(void);

//char stack1[STACK_SIZE];
//char stack2[STACK_SIZE];
//sigjmp_buf env[2];

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
        "rol    $0x11,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5 

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
        "rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#endif

void initStatistics(struct statistics* stat,int id)
{
	assert(stat != NULL);
	stat->ID = id;
	stat->state = DED;
	stat->burst = 0;
	stat->total_exec_time = 0;
	stat->total_slp_time = 0;
	stat->avg_time_quant = 0;
	stat->avg_wait_time =  0;
	stat->RedTimeTotal = 0;
	//stat->slptime = 0;

}

void clean()
{
    int count = 0;
    for(int i = (currentThread+1)%MAXTHREADS; count < MAXTHREADS; i = (i+1)%MAXTHREADS)
    {
        if(threadList[i].stat.state != DED)
        {
            threadList[i].stat.avg_wait_time = threadList[i].stat.RedTimeTotal / threadList[i].stat.burst;
            threadList[i].stat.avg_time_quant = threadList[i].stat.total_exec_time / threadList[i].stat.burst;
            printf("Thread ID: %d\nTotal execution time: %d\nTotal Sleep Time: %d\nTotal bursts: %d\nAverage waiting time: %f\nAverage Time Quanta: %f\n\n\n\n\n",i,threadList[i].stat.total_exec_time, threadList[i].stat.total_slp_time, threadList[i].stat.burst,threadList[i].stat.avg_wait_time,threadList[i].stat.avg_time_quant);
            threadList[i].stat.state = DED;
            count++;
        }
    }
    exit(0);
}

void dispatch(int sig)
{
  int count = 0;
  signal(SIGALRM,SIG_IGN);
  
 if(threadList[currentThread].stat.state != DED)
 { 
    int ret_val = sigsetjmp(threadList[currentThread].env,1);
    if (ret_val == 1) {
      return;
    }
  
    threadList[currentThread].stat.state = RED;
    threadList[currentThread].stat.total_exec_time += (clock() - threadList[currentThread].stat.RunTimeStart)/CLOCKS_PER_SEC;
    threadList[currentThread].stat.RedTimeStart = clock();
  }
  for(int i = (currentThread+1)%MAXTHREADS; count <= MAXTHREADS; i = (i+1)%MAXTHREADS)
        {
	        if(threadList[i].stat.state == RED)
            {
	            currentThread = i;
                break;
	        }
	        count++;
	     }
	     if(count > MAXTHREADS) exit(0);
	     
	     threadList[currentThread].stat.state = RUN;
	     threadList[currentThread].stat.RedTimeTotal += (clock() - threadList[currentThread].stat.RedTimeStart)/CLOCKS_PER_SEC;
	     threadList[currentThread].stat.RunTimeStart = clock();
         threadList[currentThread].stat.burst++;   
         signal(SIGALRM, alarm_handler);
         alarm(1);
         siglongjmp(threadList[currentThread].env,1);
}

void yield(){
  dispatch(SIGALRM);
}

void deleteThread(int threadID)
{
    nThreads --;
    
    threadList[threadID].stat.state = DED;
    printf("Thread with %d id is deleted\n",threadID);
}

static void wrapper()
{
    if(threadList[currentThread].tType == ARG)
    {
        threadList[currentThread].retVal = threadList[currentThread].f2(threadList[currentThread].args);
    }
    else
    {
        threadList[currentThread].f1();
    }
  printf("Thread %d exited\n",getID());
  deleteThread(currentThread);
  
  //currThread = NULL;//threadMAP[1];
 // if(count_dead < MAXTHREADS)
    signal(SIGALRM,alarm_handler);
    alarm(1);
    dispatch(SIGALRM);
}

int createWithArgs(void *(*f)(void *), void *args)
{
    int id=-1;
    if(ran==0)
    {
        for(int i = 0; i < MAXTHREADS; i++)
            initStatistics(&(threadList[i].stat),i);
        ran = 1;
    }
	for(int i = 0; i < MAXTHREADS; i++)
 	{	
 	    if(threadList[i].stat.state==DED)
			{
			    id = i;
			    break;
			}
	}
    if(id == -1)
	{
		fprintf(stderr, "Error Cannot allocate id\n");
		return id;
		
	}
    assert(id >= 0 && id < MAXTHREADS);
    
    //address_t sp, pc;
   //sp = (address_t)stack + 4096 - sizeof(address_t);
    //pc = (address_t)wrapper;
    //sigsetjmp(threadList[currentThread].env,1);
    threadList[id].f2 = f;
    threadList[id].args = args;
   // sigemptyset(&tcb->context->__saved_mask);
  
    
	threadList[id].stat.state = RED;  
	nThreads++;
	//count_dead--;
	return  id;
}


int create(void (*f)(void))
{
//TODO - update data structures 
	int id=-1;
	if(ran == 0)
	{
	    for(int i = 0; i < MAXTHREADS; i++)
            initStatistics(&(threadList[i].stat),i);
        ran = 1;
	}
	for(int i = 0; i < MAXTHREADS; i++)
 	{	
 	    if(threadList[i].stat.state==DED)
			{
			    id = i;
			    break;
			}
	}
	
	if(id==-1)
	{
		fprintf(stderr, "Error Cannot allocate id");
		return id;
		
	}
	assert(id >= 0 && id < MAXTHREADS);
	threadList[id].tType = NOARG;
	threadList[id].f1 = f;
	threadList[id].stat.state = NEW;  
	nThreads++;
	//count_dead--;
	return  id;
}

void alarm_handler(int sig)
{
	signal(SIGALRM, alarm_handler);
	dispatch(SIGALRM);
}



int getID()
{
	return currentThread;
}

void run(int tid)
{
	threadList[tid].stat.state = RED;
}

void suspend(int tid)
{
	threadList[tid].stat.state = SUS;
}

void resume(int tid)
{
	threadList[tid].stat.state = RED;
}

void sleep(int secs)
{
	threadList[currentThread].stat.state = SLP;
	//threadList[currentThread].stat.slptime = secs;
	//threadList[currentThread].stat.start_time_slp = clock();
	clock_t StartTime = clock();
    while(((clock()-StartTime)/CLOCKS_PER_SEC) <= secs);
    threadList[currentThread].stat.total_slp_time += secs;
	
	dispatch(SIGALRM);
	 
	
}


struct statistics* getStatus(int tid)
{
	struct statistics* result = &(threadList[tid].stat);
	return result;
}


void start(void)
{
    address_t sp, pc;
    for(int i = 0; i < MAXTHREADS; i++)
    {
        sp = (address_t)threadList[i].stack + STACK_SIZE - sizeof(address_t);
        pc = (address_t)wrapper;
        sigsetjmp(threadList[i].env, 1);
        (threadList[i].env->__jmpbuf)[JB_SP] = translate_address(sp);
        (threadList[i].env->__jmpbuf)[JB_PC] = translate_address(pc);
        sigemptyset(&(threadList[i].env)->__saved_mask);
        if(threadList[i].stat.state == NEW)   threadList[i].stat.state = RED; 
    }
    signal(SIGALRM,alarm_handler);
    alarm(1);
    while(1);
}
