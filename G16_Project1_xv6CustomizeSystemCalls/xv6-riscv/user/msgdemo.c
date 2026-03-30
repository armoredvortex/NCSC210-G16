#include "kernel/types.h"
#include "user/user.h"

int main(){
    int key = 123; 
    int qid;
    while((qid=msgget(key++)) != -1){
        printf("Message Queue ID: %d\n", qid);
    }
    if(qid == -1){
        printf("Failed to create message queue\n");
        exit(1);
    }
    exit(0);
}