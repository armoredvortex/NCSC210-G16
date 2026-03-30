#include "kernel/types.h"
#include "user/user.h"

int main(){
    int key = 123; 
    int qid;
    while((qid=msgget(key++)) != -1){
        printf("------------------\n");
        printf("Message Queue ID: %d\n", qid);
        char* msg = "Hello";
        if(sendmsg(qid,0,msg, strlen(msg)+1) < 0){
            printf("Failed to send message\n");
        } else{
            printf("Message sent\n");
        }
        printf("------------------\n");
    }

    exit(0);
}