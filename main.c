#include <stdio.h>
#include <unistd.h>


#define procNameLength 32
#define maxProcNum 30
#define policyName 256

#define debugInput

typedef struct process {
    char name[procNameLength];
    int t_ready;
    int t_exec;
    pid_t pid;
}Process;
char policy_For_schedule[policyName];
Process processes[maxProcNum];

int main(int argc, char* argv[]){
    scanf("%s",policy_For_schedule);
    int procNum;
    scanf("%d", &procNum);

    for(int i=0;i<procNum;i++){
        char procName[procNameLength];
        int readyTime;
        int endTime;
        scanf("%s%d%d", procName, &readyTime, &endTime);
    #ifdef debugInput
        printf("%s\n%d\n%d\n", procName, readyTime, endTime);
    #endif 
    }
}
