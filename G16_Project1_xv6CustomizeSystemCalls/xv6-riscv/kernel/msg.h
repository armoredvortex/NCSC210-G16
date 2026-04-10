#define MAX_MSG 10 // max number of messages
#define MAX_Q 10   // max number of queues
#define MAX_MSG_LEN 128 // max length of message

struct message {
    int  type;
    char data[128];
    int len;
    int valid;
};

struct msgqueue {
    int key;
    int used;

    struct message msg[MAX_MSG]; // array of messages
    int count;        // no.of messages in queue

    // message queue changed circular queue to linear arr implementation 
    // int            front;        // front idx of queue
    // int            rear;         // rear idx of queue
    
};

void msginit(); // for msg queues initialization (definition in msg.c)
