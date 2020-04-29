#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>


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

#define debugInput
#define sysGET_TIME 548
#define sysPRINTK 549

typedef struct process {
    char name[procNameLength];
    int readyTime;
    int executeTime;
    pid_t pid;
}Process;
char policy_For_schedule[policyName];
Process processes[maxProcNum];
char str_dmesg[maxTodemsg];

void printProcesses(int N){
    for(int i=0;i<N;i++){
        printf("%s\n%d\n%d\n", processes[i].name, processes[i].readyTime, processes[i].executeTime);
    }
}

int do_schedule(Process processes[], int processNum, int policy);
int process2cpu(int pid, int coreIdx);
int process_pick(int pid);
int main(int argc, char* argv[]){
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
    
    printf("policy: %d\n", policy);
    do_schedule(processes, procNum, policy);
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
int do_schedule(Process processes[], int processNum, int policy){
    //sorting the process with the ready time
    qsort(processes, processNum, sizeof(Process), cmp);
    #ifdef debugInput
    printProcesses(processNum);
    #endif
    //using two core machine
    //assign scheduling process to one core
    int scheduler_pid = getpid();
    int ret_process2cpu = process2cpu(scheduler_pid, schedulingCPU);
    printf("ret_process2cpu %d\n",ret_process2cpu);
    int ret_process_pick = process_pick(scheduler_pid);
    printf("ret_process_pick %d\n",ret_process_pick);
    //give scheduler higher priority

    int time_unit = 0;
    int runningIdx = -1;
    int finish_num = 0;

    while(1){
        // if there is a process running and finish
        if (runningIdx != -1 && processes[runningIdx].executeTime == 0){
            waitpid(processes[runningIdx].pid, NULL, 0);
            printf("%s %d\n", processes[runningIdx].name, processes[runningIdx].pid);
            fflush(stdout);

            finish_num += 1;
            runningIdx = -1;

            if(finish_num == processNum) break;
        }

        // if process is ready
        for (int i=0;i<processNum;i++){
            if(processes[i].readyTime == time_unit){
                // fork a process and start execute
                processes[i].pid = process_create(processes[i]);
                process_idle(processes[i].pid);
            }
        }
    }
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
        int scheduled_process_pid = getpid()
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
		printf("sched_setscheduler:pick\n");
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
