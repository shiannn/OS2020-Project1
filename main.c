#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/types.h> 
#include <sys/wait.h>

#define procNameLength 32
#define maxProcNum 30
#define policyName 256
#define maxTodemsg 200

#define FIFO 1
#define RR 2
#define SJF 3
#define PSJF 4
#define schedulingCPU 0
#define childCPU 1

#define UNIT_TIME() { volatile unsigned long i;for (i = 0; i < 1000000UL; i++);}                                       \

//#define debugInput
#define sysGET_TIME 548
#define sysPRINTK 549

typedef struct process {
    char name[procNameLength];
    int readyTime;
    int executeTime;
    pid_t pid;
}Process;

typedef struct Queue{
    Process processArray[maxProcNum];
    int size;
    int st, ed;
}Queue;

char policy_For_schedule[policyName];
Process processes[maxProcNum];

Process processQueue[maxProcNum];

char str_dmesg[maxTodemsg];

int last_context_switch_time;
int time_unit = 0;
int runningIdx = -1;
	
int finish_num = 0;

void printProcesses(int N){
    for(int i=0;i<N;i++){
        printf("%s\n%d\n%d\n", processes[i].name, processes[i].readyTime, processes[i].executeTime);
    }
}

int do_schedule(Process processes[], Queue* qPtr, int processNum, int policy);
int process2cpu(int pid, int coreIdx);
int process_pick(int pid);
int process_idle(int pid);
int process_create(Process process);

void enqueue(Queue* qPtr, Process item);
Process dequeue(Queue* qPtr);
Queue CreateQueue();
Process head(Queue* qPtr);

int main2(int argc, char* argv[]){
    Queue q = CreateQueue();
    Process item;
    strcpy(item.name, "abcde");
    enqueue(&q, item);
    printf("%d\n",q.size);
    Process ret = dequeue(&q);
    printf("%s\n", ret.name);
    printf("%d\n",q.size);
}

int main(int argc, char* argv[]){
    Queue q = CreateQueue();

    scanf("%s",policy_For_schedule);
    int procNum;
    scanf("%d", &procNum);

    for(int i=0;i<procNum;i++){
        char procName[procNameLength];
        int readyTime;
        int endTime;
        scanf("%s%d%d", procName, &readyTime, &endTime);
        strcpy(processes[i].name, procName);
        processes[i].readyTime = readyTime;
        processes[i].executeTime = endTime;
    }

    int policy;
    if(strcmp(policy_For_schedule, "FIFO")==0){
        policy = FIFO;
    }
    else if(strcmp(policy_For_schedule, "RR")==0){
        policy = RR;
    }
    else if(strcmp(policy_For_schedule, "SJF")==0){
        policy = SJF;
    }
    else if(strcmp(policy_For_schedule, "PSJF")==0){
        policy = PSJF;
    }
    else{
        printf("policy error!!!\n");
    }
    
    //printf("policy: %d\n", policy);
    do_schedule(processes, &q, procNum, policy);
    return 0;
}
int cmp(const void *a, const void *b){
    Process *aptr = (Process *)a;
    Process *bptr = (Process *)b;
    if(aptr->readyTime > bptr->readyTime){
        return 1;
    }
    else if(aptr->readyTime < bptr->readyTime){
        return -1;
    }
    else{
        return 0;
    }
}

int select_next(Process processes[], Queue* qPtr,  int processNum, int policy){
    if(runningIdx != -1){
        switch(policy){
            case SJF: case FIFO:{
                return runningIdx;
            }
        }
    }
    int retProcessIdx = -1;
    switch(policy){
        case PSJF: case SJF:{
            for(int i=0;i<processNum;i++){
                if(processes[i].pid==-1 || processes[i].executeTime==0){
                    continue;
                }
                if(retProcessIdx==-1 || (processes[i].executeTime < processes[retProcessIdx].executeTime)){
                    retProcessIdx = i;
                }
            }
            break;
        }
        case FIFO:{
            for(int i=0;i<processNum;i++){
                if(processes[i].pid == -1 || processes[i].executeTime == 0){
                    continue;
                }
                if(retProcessIdx == -1 || (processes[i].readyTime < processes[retProcessIdx].readyTime)){
                    retProcessIdx = i;
                }
            }
            break;
        }
        case RR:{
            /*
            if(runningIdx == -1){
                for(int i=0;i<processNum;i++){
                    if(processQueue[i].pid != -1 && processQueue[i].executeTime > 0){
                        retProcessIdx = i;
                        break;
                    }
                }
            }
            else if((time_unit - last_context_switch_time) % 500 == 0){
                retProcessIdx = (runningIdx + 1) % processNum;
                while(processQueue[retProcessIdx].pid == -1 || processQueue[retProcessIdx].executeTime == 0){
                    retProcessIdx = (retProcessIdx + 1) % processNum;
                }
            }
            else{
                retProcessIdx = runningIdx;
            }
            break;
            */
            
            //queue ready 時才會 insert
            //finish 則 pop 掉
            //context switch 則 pop 再 push
            if(runningIdx == -1){
                /*
                for(int i=0;i<processNum;i++){
                    if(processes[i].pid != -1 && processes[i].executeTime > 0){
                        retProcessIdx = i;
                        break;
                    }
                }
                */
                //head of queue
                if(qPtr -> size > 0){
                    retProcessIdx = head(qPtr);
                }
                // if empty then retProcessIdx still -1
            }
            else if((time_unit - last_context_switch_time) % 500 == 0){
                /*
                retProcessIdx = (runningIdx + 1) % processNum;
                while(processes[retProcessIdx].pid == -1 || processes[retProcessIdx].executeTime == 0){
                    retProcessIdx = (retProcessIdx + 1) % processNum;
                }
                */
                //dequeue and enqueue
                if(qPtr -> size > 0){
                    Process to_be_switch = dequeue(qPtr);
                    enqueue(qPtr, to_be_switch);
                    //new head is the next
                    retProcessIdx = head(qPtr);
                }
            }
            else{
                //something is running and not yet context switch
                retProcessIdx = runningIdx;
            }
            break;
            
        }
    }
    return retProcessIdx;
}

int do_schedule(struct process proc[],Queue* qPtr, int nproc, int policy) {
	qsort(proc, nproc, sizeof(struct process), cmp);

	/* Initial pid = -1 imply not ready */
	for (int i = 0; i < nproc; i++)
		proc[i].pid = -1;

	/* Set single core prevent from preemption */
	process2cpu(getpid(), schedulingCPU);
	
	/* Set high priority to scheduler */
	process_pick(getpid());
	
	/* Initial scheduler */
	time_unit = 0;
	runningIdx = -1;
	finish_num = 0;
	
	while(1) {
		//fprintf(stderr, "Current time: %d\n", ntime);

		/* Check if running process finish */
		if (runningIdx != -1 && proc[runningIdx].executeTime == 0) {
		
			//kill(running, SIGKILL);
			waitpid(proc[runningIdx].pid, NULL, 0);
			printf("%s %d\n", proc[runningIdx].name, proc[runningIdx].pid);
			fflush(stdout);
			
			runningIdx = -1;
			finish_num++;

			/* All process finish */
			if (finish_num == nproc)
				break;
		}

		/* Check if process ready and execute */
		for (int i = 0; i < nproc; i++) {
			if (proc[i].readyTime == time_unit) {
                // create put in array
                int tempPid = process_create(proc[i]);
				proc[i].pid = tempPid;

                // create in queue
                enqueue(qPtr, proc[i]);
                

				process_idle(proc[i].pid);
			}

		}

		/* Select next running process */
		int next = select_next(proc, qPtr, nproc, policy);
		if (next != -1) {
			/* Context switch */
			if (runningIdx != next) {
				process_pick(proc[next].pid);
				process_idle(proc[runningIdx].pid);
				runningIdx = next;
				last_context_switch_time = time_unit;
			}
		}

		/* Run an unit of time */
		UNIT_TIME();
		if (runningIdx != -1)
			proc[runningIdx].executeTime--;
		time_unit++;
	}

	return 0;
}

int process_idle(int pid){
    struct sched_param schedule_parameter;
    schedule_parameter.sched_priority = 0;
    int ret = sched_setscheduler(pid, SCHED_IDLE, &schedule_parameter);
    if (ret < 0){
        printf("sched_setscheduler idle error!!!\n");
        return -1;
    }
    return ret;
}

int process_create(Process process){
    int pid = fork();
    if(pid<0){
        printf("fork error!!!\n");
        return -1;
    }
    else if(pid==0){
        unsigned long stSecond, stnSecond;
        syscall(sysGET_TIME, &stSecond, &stnSecond);

        for (int i=0;i < process.executeTime; i++){
            //execute executeTime TIME UNITS
            //the for loop from TA
            UNIT_TIME();
        }
        unsigned long edSecond, ednSecond;
        syscall(sysGET_TIME, &edSecond, &ednSecond);
        int scheduled_process_pid = getpid();
        sprintf(str_dmesg, "[Project1] %d %lu.%09lu %lu.%09lu\n", scheduled_process_pid, stSecond, stnSecond, edSecond, ednSecond);
        syscall(sysPRINTK, str_dmesg);
        exit(0);
    }
    else{
        //scheduler
        //assign the child process to another core
        //fork will return child process pid to parent
        process2cpu(pid, childCPU);
        return pid;
    }
}
int process_pick(int pid){
    struct sched_param schedule_parameter;

    /* SCHED_OTHER should set priority to 0 */
	schedule_parameter.sched_priority = 0;

    int ret = sched_setscheduler(pid, SCHED_OTHER, &schedule_parameter);
	
	if (ret < 0) {
		printf("sched_setscheduler:pick error\n");
		return -1;
	}

	return ret;
}
int process2cpu(int pid, int coreIdx){
    if(coreIdx > sizeof(cpu_set_t)){
        printf("exceed core bound!!!\n");
        return -1;
    }
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(coreIdx, &mask);

    int ret = sched_setaffinity(pid, sizeof(mask), &mask);
    if(ret < 0 ){
        printf("sched_setaffinity error!!!\n");
        exit(1);
    }
    
    return 0;
}
