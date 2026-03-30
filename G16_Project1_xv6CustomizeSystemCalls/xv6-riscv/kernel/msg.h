#define MAX_MSG 10 // max number of messages
#define MAX_Q 10   // max number of queues
#define MAX_MSG_LEN 128 // max length of message

struct message {
    int  type;
    char data[128];
};

struct msgqueue {
    int key;
    int used;

    struct message msg[MAX_MSG]; // array of messages
    int            front;        // front idx of queue
    int            rear;         // rear idx of queue
    int            count;        // no.of messages in queue
};

void msginit(); // for msg queues initialization (definition in msg.c)
