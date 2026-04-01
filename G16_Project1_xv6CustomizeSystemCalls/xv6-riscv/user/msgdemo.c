#include "kernel/types.h"
#include "user/user.h"

int main(){
    int key = 123; 
    int qid;
    
    qid = msgget(key); 
    if(qid == -1){
        printf("failed to create message queue: msgget failed\n");
    }

    int pid = fork();
    if(pid > 0){
        //parent 
        wait(0);
        char* buff = (char*)malloc(100); 
        if(recvmsg(qid, 1, buff, 100) == -1){
            printf("failed to receive message from child: msgrcv failed\n");
        } else{
            printf("message received from child: %s\n", buff);
        }
    } else{
        //child 
        char* msg = "hello parent, how are you";
        if(sendmsg(qid, 1,msg,strlen(msg)+1) == -1){
            printf("failed to send message to parent: msgsnd failed\n");
        } else{
            printf("message sent: from child\n");
        }
    }
    exit(0);
}