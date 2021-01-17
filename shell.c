//Jithin Kallukalam Sojan 2017A7PS0163P 
//Samina Shiraj Mulani 2018A7PS0314P 

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX 100
#define MAXpaths 30
#define MAXProcesses 10
#define MAXCommandSize 50
#define MAXArgs 15
#define BUFFERInitSize 400
#define READFromPipe 100
// MAXProcesses is used to define the maximum processes that can be piplened in the command.

char commands[10][MAX];
int validity[10];
int currentIndex = 0;
int pids[10][MAXProcesses];
int statuses[10][MAXProcesses];
int numSubCommands[10];
int rotate = 0, first = 0;

void printChildStatus(int status);

static void sigHandlerINT(int sig){    
    int start = 0;
    if(rotate==1){
        start = currentIndex;
    }
    
    int current = (currentIndex+10-1)%10;
    int stop = start;
    int count = -1;
    printf("\n");
    printf("\n");
    if(first==0){
        printf("No previous commands.\n");
        return;
    }
    if(rotate==0){
        printf("Last %d commands:\n",currentIndex);
    }
    else{
        printf("Last %d commands:\n",10);
    }
    printf("\n");
    while(1){
        printf("Command %d:  ",count);
        printf("\x1B[32m");
        printf("%s\n",commands[current]);
        printf("\x1B[0m");
        if(!validity[current]){
            printf("\x1B[32m");
            printf("Invalid command.\n");
            printf("\x1B[0m");
        }
        else{
            printf("%-9s\t|%-9s\n","PID","STATUS");
            printf("________________|________________________________________________\n");
            for(int i = 0;i<numSubCommands[current];i++){
                printf("\x1B[32m");
                printf("%-9d\t",pids[current][i]);
                printf("\x1B[0m");
                printf("|");
                printf("\x1B[32m");
                printChildStatus(statuses[current][i]);
                printf("\x1B[0m");
                printf("\n");
            }
        }

        if(current==stop){
            break;
        }

        current = (current+10-1)%10;
        count--;
        printf("\n");
    }
    printf("\n");
}
static void sigHandlerQUIT(int sig){
    printf("Do you really want to exit(y/n):");
    fflush(stdout);

    char c[MAX];
    ssize_t size = read(0,&c,MAX);
    if(size!=0){
        c[size-1] = '\0';
    }
    else{
        return;
    }
    
    if(c[0]=='y'){
        exit(0);
    }
}

int trimSpaces(char* tok,int currentIndex,char subCommand[MAXProcesses][MAXCommandSize]);

int checkCommandPipelineType(char command[MAX]){
    char* cmdTypeCheck = strstr(command,"|||");
    if(cmdTypeCheck!=NULL){
        return 3;
    }
    cmdTypeCheck = strstr(command,"||");
    if(cmdTypeCheck){
        return 2;
    }
    cmdTypeCheck = strstr(command,"|");
    if(cmdTypeCheck){
        return 1;
    }
    return 0;
}

int checkValidity(char command[MAX],int cmdType,char subCommand[MAXProcesses][MAXCommandSize]){
    int currIndex = 0;
    char* check = NULL;
    char* start = command;
    char temp;
    char* end = start + strlen(command);
    if(cmdType==3){
        check = strstr(start,"|||");
        if(check==start){
            return -1;
        }

        temp = *check;
        *check = '\0';
        if(strstr(start,"||")!=NULL || strstr(start,"|")!=NULL || strstr(start,",")!=NULL)return -1;
        if(trimSpaces(start,currIndex,subCommand)==-1){
            return -1;
        }
        currIndex++;
        *check = temp;

        check = check+3;
        if(check==end){
            return -1;
        }

        if(strstr(check,"|||")!=NULL || strstr(check,"||")!=NULL || strstr(check,"|")!=NULL)return -1;
    }
    else if(cmdType==2){
        check = strstr(start,"||");
        if(check==start){
            return -1;
        }
        
        temp = *check;
        *check = '\0';
        if(strstr(start,"|")!=NULL || strstr(start,",")!=NULL)return -1;
        if(trimSpaces(start,currIndex,subCommand)==-1){
            return -1;
        }
        currIndex++;
        *check = temp;

        check = check+2;
        if(check==end){
            return -1;
        }

        if(strstr(check,"||")!=NULL || strstr(check,"|")!=NULL)return -1;

    }
    else if(cmdType==1){
        if(strstr(command,",")!=NULL)return -1;
        while((check=strstr(start,"|"))!=NULL){
            if(check==start){
                return -1;
            }

            temp = *check;
            *check = '\0';
            if(trimSpaces(start,currIndex,subCommand)==-1){
                return -1;
            }
            currIndex++;
            *check = temp;

            check = check+1;
            if(check==end){
                return -1;
            }

            start = check;
        }
        if(trimSpaces(start,currIndex,subCommand)==-1){
            return -1;
        }
        currIndex++;

    }
    else{
        if(strstr(start,",")!=NULL)return -1;
        if(trimSpaces(start,currIndex,subCommand)==-1){
            return -1;
        }
        currIndex++;
    }

    if(cmdType>=2){
        int count = 0;
        start = check;
        while((check = strstr(start,","))!=NULL){
            count++;
            if(start==check){
                return -1;
            }

            temp = *check;
            *check = '\0';
            if(trimSpaces(start,currIndex,subCommand)==-1){
                return -1;
            }
            currIndex++;
            *check = temp;

            check = check+1;
            if(check==end){
                return -1;
            }
            start = check;
        }
        if(count!=(cmdType-1)){
            return -1;
        }
        if(trimSpaces(start,currIndex,subCommand)==-1){
            return -1;
        }
        currIndex++;
    }
    return currIndex;
}

int trimSpaces(char* tok,int currentIndex,char subCommand[MAXProcesses][MAXCommandSize]){
    if(strlen(tok)==0){
        return -1;
    }
    while(*tok==' '||*tok=='\t'){
        tok+=1;
        if(*tok=='\0')return -1;
    }

    int length = strlen(tok);
    char* endPointer = tok+length-1;
    while(*endPointer==' ' || *endPointer == '\t')endPointer-=1;

    strncpy(subCommand[currentIndex],tok,endPointer-tok+1);
    subCommand[currentIndex][endPointer-tok+1] = '\0';
}

char* removeFirstSpace(char subString[]){
    char* occ1 = strstr(subString," ");
    char* occ2 = strstr(subString,"\t");

    if(!occ1 && !occ2)return NULL;
    char* start = NULL;
    if(!occ2 || (occ1 && (occ1<occ2))){
        *occ1 = '\0';
        start = occ1+1;
    }
    else{
        *occ2 = '\0';
        start = occ2+1;
    }

    while(*start==' ' || *start=='\t')start = start+1;
    return start;
}

void printChildStatus(int status){
    if(WIFEXITED(status)){
        printf("Normal Termination,exit status = %d",WEXITSTATUS(status));
    }
    else if(WIFSIGNALED(status)){
        printf("Abnormal Termination, signal number = %d",WTERMSIG(status));
    }
    else if(WIFSTOPPED(status)){
        printf("Child stopped, signal number = %d",WSTOPSIG(status));
    }
    else{
        printf("Child continued.");
    }
}

void printProcessInfo(char* subcmd, int ip,int op,int fdInp,int fdOp,int pid, int status){
    printf("\x1B[32m");
    printf("%-10s ",subcmd);
    printf("\t");
    printf("Pid: %-7d",pid);
    printf("\t");
    if(ip==0){
        printf("Input: %-15s","stdin");
    }
    else if(ip==1){
        printf("Input: %-8s%-7d","File fd",fdInp);
    }
    else{
        printf("Input: %-8s%-7d","Pipe fd",fdInp);
    }
    printf("\t");
    if(op==0){
        printf("Output: %-15s","stdout");
    }
    else if(op==1){
        printf("Output: %-8s%-7d","File fd",fdOp);
    }
    else{
        printf("Output: %-8s%-7d","Pipe fd",fdOp);
    }
    printf("\t");
    printf("Status: ");
    printChildStatus(status);
    printf("\n");
    printf("\x1B[0m");
}


int executeProcess(char subCmdPath[MAX],char* subCmdArgs[MAXArgs],int ip,int op,int fdInp,int fdOp,int pInp[2],int pOp[2],int *pid){
    *pid = fork();
    if(*pid>0){
        if(op==1){
            close(fdOp);
        }
        else if(op==2){
            close(pOp[1]);
        }

        if(ip==1){
            close(fdInp);
        }
        else if(ip==2){
            close(pInp[0]);
        }
        int status;
        waitpid(*pid,&status,0);
        return status;
    }
    else if(*pid==-1){
        perror("fork");
        exit(0);
    }
    else{
        sigset_t blockSet;
        sigemptyset(&blockSet);
        sigaddset(&blockSet,SIGINT);
        sigaddset(&blockSet,SIGQUIT);
        if(sigprocmask(SIG_BLOCK,&blockSet,NULL)==-1){
            printf("Error: Blocking SIGINT and SIGQUIT in child.\n");
            exit(0);
        }

        if(op!=0){
            close(1);
            if(op==1){
                dup(fdOp);
            }
            else{
                dup(pOp[1]);
                close(pOp[0]);
            }
        }
        if(ip!=0){
            close(0);
            if(ip==1){
                dup(fdInp);
            }
            else{
                //The write end of the pipe was closed by parent in previous execProcess() call.
                dup(pInp[0]);
            }
        }
        if(execv(subCmdPath,subCmdArgs)==-1){
            perror("execv");
            exit(0);
        }
    }
}

int searchForPathAndExecute(char subCommand[MAXProcesses][MAXCommandSize],int numSubCommands,int cmdType,int pids[10][MAXProcesses],int currentIndex,char paths[MAXpaths][MAX],int statuses[10][MAXProcesses],int countPath){
    char* pointerToExec[MAXProcesses];
    char* pointerToRest[MAXProcesses];
    char subCmdPaths[MAXProcesses][MAX];

    char* endPoint = NULL;

    for(int i = 0;i<numSubCommands;i++){
        pointerToExec[i] = subCommand[i];
        pointerToRest[i] = removeFirstSpace(subCommand[i]);

        int j = 0;
        while(j<countPath){
            endPoint = paths[j] + strlen(paths[j]);
            strcat(paths[j],"/");
            strcat(paths[j],pointerToExec[i]);
            if(access(paths[j],F_OK)==0){
                break;
            }

            *endPoint = '\0';
            j++;
        }
        if(j==countPath){
            printf("No executable corresponding to %s.\n",pointerToExec[i]);
            return -1;
        }
        else{
            strcpy(subCmdPaths[i],paths[j]);
            *endPoint = '\0';    
        }
    }

    int pipeInp[2];
    int pipeOp[2];

    char* interim;
    ssize_t size;
    ssize_t readSize;
    ssize_t totalSize;

    int inpArr[MAXProcesses];
    int opArr[MAXProcesses];
    int inpFdArr[MAXProcesses];
    int opFdArr[MAXProcesses];

    for(int i = 0;i<numSubCommands;i++){
        char* subCmdArgs[MAXArgs];

        int inp = 0;
        int op = 0;
        int append = 0;
        char fileInp[MAX];
        char fileOp[MAX];

        subCmdArgs[0] = pointerToExec[i];
        int currInd = 1;
        subCmdArgs[currInd] = pointerToRest[i];
        char* file = NULL;
        char* next = NULL;
        while(subCmdArgs[currInd]){
            if(subCmdArgs[currInd][0]=='<' || subCmdArgs[currInd][0]=='>'){
                if(subCmdArgs[currInd][0]=='>' && subCmdArgs[currInd][1]=='>'){
                    next = subCmdArgs[currInd]+2;
                    append = 1;
                }
                else next = subCmdArgs[currInd]+1;

                char temp = subCmdArgs[currInd][0];
                if(*next=='\0'){
                    printf("Redirection operation without file.\n");
                    return -1;
                }
                if(*next==' ' || *next=='\t'){
                    file = removeFirstSpace(subCmdArgs[currInd]);
                    if(!(('a'<=*file && *file<='z')||('A'<=*file && *file<='Z'))){
                        printf("Invalid redirection operator.\n");
                        return -1;
                    }
                    subCmdArgs[currInd] = removeFirstSpace(file);
                }
                else{
                    file = next;
                    if(!(('a'<=*file && *file<='z')||('A'<=*file && *file<='Z'))){
                        printf("Invalid redirection operator.\n");
                        return -1;
                    }
                    subCmdArgs[currInd] = removeFirstSpace(file);
                }
                if(temp=='<'){
                    inp = 1;
                    strcpy(fileInp,file);
                }
                else{
                    op = 1;
                    strcpy(fileOp,file);
                }
            }
            else{
                currInd++;
                subCmdArgs[currInd] = removeFirstSpace(subCmdArgs[currInd-1]);
            }
        }

        // All valid paths at this point.
        int fdInp;
        int fdOp;
        if(inp){
            fdInp = open(fileInp,O_RDONLY);
            if(fdInp==-1){
                printf("Error in opening file %s for input.\n",fileInp);
                return -1;
            }
        }
        if(op){
            if(append==0) fdOp = open(fileOp,O_RDONLY|O_WRONLY|O_TRUNC|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
            else fdOp = open(fileOp,O_WRONLY|O_APPEND|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
            if(fdOp==-1){
                printf("Error in opening file %s for output.\n",fileOp);
            }
        }

        if(cmdType==0){
            statuses[currentIndex][i] = executeProcess(subCmdPaths[i],subCmdArgs,inp,op,fdInp,fdOp,pipeInp,pipeOp,&pids[currentIndex][i]);
            if(inp) inpFdArr[i] = fdInp;
            if(op) opFdArr[i] = fdOp;
            inpArr[i] = inp;
            opArr[i] = op;

        }
        else if(cmdType==1){
            if(pipe(pipeOp)==-1){
                perror("pipe");
                exit(0);
            }
            if(i!=0){
                if(inp!=1){
                    //Read from pipe
                    inp = 2;
                }
                else{
                    close(pipeInp[0]);
                }
            }
            if(i!=numSubCommands-1){
                if(op!=1){
                    //Write into pipe.
                    op = 2;
                }
                else{
                    close(pipeOp[1]);
                }
            }

            statuses[currentIndex][i] = executeProcess(subCmdPaths[i],subCmdArgs,inp,op,fdInp,fdOp,pipeInp,pipeOp,&pids[currentIndex][i]);
            inpArr[i] = inp;
            opArr[i] = op;
            if(inp==1)inpFdArr[i] = fdInp;
            else if(inp==2)inpFdArr[i] = pipeInp[0];

            if(op==1)opFdArr[i] = fdOp;
            else if(op==2)opFdArr[i] = pipeOp[1];

            pipeInp[0] = pipeOp[0];
            pipeInp[1] = pipeInp[1];
        }
        else{
            if(i==0){
                if(pipe(pipeOp)==-1){
                    perror("pipe");
                    exit(0);
                }
                if(op!=1){
                    //Write into pipe.
                    op = 2;
                }
                else{
                    close(pipeOp[1]);
                }
                statuses[currentIndex][i] = executeProcess(subCmdPaths[i],subCmdArgs,inp,op,fdInp,fdOp,pipeInp,pipeOp,&pids[currentIndex][i]);
                pipeInp[0] = pipeOp[0];
                pipeInp[1] = pipeInp[1];

                //Read from pipe and then send to two/three processes.
                size = BUFFERInitSize;
                readSize = READFromPipe;
                totalSize = 0;
                interim = (char*)malloc(sizeof(char)*size);
                while(readSize==READFromPipe){
                    readSize = read(pipeInp[0],(void*)(interim+totalSize),READFromPipe);
                    totalSize+=readSize;
                    //If the write end of the pipe is closed, when we try to read, does it show EOF even if there is stuff written in the pipe?
                    if(totalSize==size){
                        size = size*2;
                        interim = (char*)realloc((void*)interim,sizeof(char)*size);
                    }
                }
                close(pipeInp[0]);
                inpArr[i] = inp;
                opArr[i] = op;
                if(inp==1)inpFdArr[i] = fdInp;

                if(op==1)opFdArr[i] = fdOp;
                else if(op==2)opFdArr[i] = pipeOp[1];
            }
            else{
                ssize_t writeSize;
                if(inp==0){
                    inp = 2;
                    if(pipe(pipeInp)==-1){
                        perror("pipe");
                        exit(0);
                    }
                    writeSize = write(pipeInp[1],(void*)interim,totalSize);
                    if(writeSize<totalSize){
                        printf("Error: IPC pipe produced error.\n");
                        return -2;
                    }
                    close(pipeInp[1]);
                }
                statuses[currentIndex][i] = executeProcess(subCmdPaths[i],subCmdArgs,inp,op,fdInp,fdOp,pipeInp,pipeOp,&pids[currentIndex][i]);
                inpArr[i] = inp;
                opArr[i] = op;
                if(inp==1)inpFdArr[i] = fdInp;
                else if(inp==2)inpFdArr[i] = pipeInp[0];

                if(op==1)opFdArr[i] = fdOp;
            }
        }
    }

    printf("\n\n");
    printf("Command Details:\n");
    for(int i = 0;i<numSubCommands;i++){
        if(i==0){
            printf("Process P%d:\n",1);
            printf("\t");
        }
        else if(cmdType>=2 && i>=0){
            printf("Process P%d pipelined into P%d:\n",1,i+1);
            printf("\t");
        }
        else{
            printf("Process P%d piplined into P%d:\n",i,i+1);
            printf("\t");
        }
        printProcessInfo(pointerToExec[i],inpArr[i],opArr[i],inpFdArr[i],opFdArr[i],pids[currentIndex][i],statuses[currentIndex][i]);
    }

    printf("\n\n");

    return 1;
}

int parseCommand(char commands[10][MAX], int currentIndex, int pids[10][MAXProcesses],char paths[MAXpaths][MAX],int statuses[10][MAXProcesses],int countPath){
    char command[MAX];
    strcpy(command,commands[currentIndex]);

    int cmdType = checkCommandPipelineType(command);
    char subCommand[MAXProcesses][MAXCommandSize];

    int numSubCommands = checkValidity(command,cmdType,subCommand);
    if(numSubCommands<0)return numSubCommands;

    int valid = searchForPathAndExecute(subCommand,numSubCommands,cmdType,pids,currentIndex,paths,statuses,countPath);
    if(valid<0)return valid;

    return numSubCommands;
}

int main(){
    
    for(int i = 0;i<10;i++){
        memset(commands[i],0,sizeof(char)*MAX);
    }

    char* s = getenv("PATH");
    if(s==NULL){
        printf("There is no path to search for files in.\n");
        exit(0);
    }

    char paths[MAXpaths][MAX];
    int pathIndex = 0;

    char* token = strtok(s,":");
    while(token){
        if(strlen(token)){
            strcpy(paths[pathIndex],token);
            pathIndex++;
        }
        token = strtok(NULL,":");
    }
    int countPath = pathIndex;

    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask,SIGINT);
    sigdelset(&mask,SIGQUIT);
    sigprocmask(SIG_SETMASK,&mask,NULL);

    if(signal(SIGINT,sigHandlerINT)==SIG_ERR){
        printf("Error: Setting handler for SIGINT.\n");
        exit(0);
    }

    if(signal(SIGQUIT,sigHandlerQUIT)==SIG_ERR){
        printf("Error: Setting handler for SIQUIT.\n");
        exit(0);
    }

    while(1){
        printf("\x1B[32m""~$ ""\x1B[0m");
        fflush(stdout);
        ssize_t readBytes = read(0,commands[currentIndex],MAX);
        if(readBytes!=0){
            commands[currentIndex][readBytes-1] = '\0';
        }
        else{
            commands[currentIndex][readBytes] = '\0';
        }
        printf("\n");
        
        int valid;
        valid = parseCommand(commands,currentIndex,pids,paths,statuses,countPath);
        if(valid==-1){
            printf("Invalid command.\n");
        }
        if(valid<0){
            validity[currentIndex] = 0;
        }
        else{
            validity[currentIndex] = 1;
        }
        if(valid>=0){
            numSubCommands[currentIndex] = valid;
        }

        first = 1;

        currentIndex = (currentIndex+1)%10;
        if(currentIndex==0){
            rotate = 1;
        }

    }
}