#ifndef MYTHREAD_H

#define MYTHREAD_H
#include"allstdlib.h"
#include<setjmp.h>
#define MAXTHREADS 100

typedef void *(*f_WithArgs)(void *);
enum STATES{DED=0,RUN,RED,SLP,SUS, NEW};
enum BOOLEAN {FALSE=0,TRUE};
enum TYPE {ARG = 0, NOARG};

struct statistics
{
	int ID;
	enum STATES state;
	int burst;
	int total_exec_time;
	int total_slp_time;
	float avg_time_quant;
 	float avg_wait_time;
 	int RedTimeTotal;
 	clock_t RunTimeStart;
 	clock_t RedTimeStart;
 	//int slptime;
 	//clock_t start_time_slp;
};

typedef struct tcb
{
	jmp_buf env;
	char stack[4096]; //the thread stack
	struct statistics stat;
	enum TYPE tType;//thread type determining arg/noarg
	void (*f1)(void);//thread routine no arg type
	f_WithArgs f2;//thread routine arg type
	void* args;//thread args
	void* retVal;
	
}TCB; //the thread control block

int create(void (*f)(void));//update dses of the control
int createWithArgs(void *(*f)(void *), void *arg);
int getID(void);//return RThreadID
void dispatch(int);//the scheduler func, sig == signalID?
void start(void);//start executing the threads - possibly a master thread???
void run(int threadID);//put thread status = RED?
void suspend(int threadID);//put thread status = SUS?
void resume(int threadID);//put thread status = RED?
void yield(void);
void initStatistics(struct statistics* stat,int id);
void deleteThread(int threadID);
void sleep(int sec);// put thread status = SLP?
struct statistics* getStatus(int threadID);//print the fields of statistics??
int createWithArgs(void*(*f)(void*),void* arg);
void clean(void);//stops the master thread?
void JOIN(int threadID);//bonus
void* GetThreadResult(int threadID);//bonus
#endif
