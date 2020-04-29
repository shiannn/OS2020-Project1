#include <sys/types.h> 
#define maxProcesses 30
#define procNameLength 32

typedef struct process {
    char name[procNameLength];
    int readyTime;
    int executeTime;
    pid_t pid;
}Process;

typedef struct Queue{
    Process processArray[maxProcesses];
    int size;
    int st, ed;
}Queue;

Queue CreateQueue(){
    Queue q;
    q.size = 0;
    q.st = 0;
    q.ed = 0;
    return q;
}

void enqueue(Queue* qPtr, Process item){
    qPtr -> processArray[qPtr-> ed] = item;
    qPtr -> size += 1;
    qPtr-> ed = (qPtr-> ed+1)%maxProcesses;
    return;
}

Process dequeue(Queue* qPtr){
    Process ret = qPtr -> processArray[qPtr-> st];
    qPtr -> size -= 1;
    qPtr-> st = (qPtr-> st+1)%maxProcesses;
    return ret;
}