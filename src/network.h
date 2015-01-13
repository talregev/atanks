#ifndef NETWORK_HEADER_FILE__
#define NETWORK_HEADER_FILE__

/*
This file will contain two sets of headers and data. One for dealing with queued message
and the other for handling server sockets. This queue will be in standard, platform-neutral
C++. However, the sockets will use Linux/UNIX/BSD specific code, which will have to be
updated to run on other operating systems.
-- Jesse
*/

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define MAX_MESSAGE_LENGTH 256

typedef struct message_struct
{
    char *text;
    int to;   // which client does the message go to? May not be used as most will go to everyone
    void *next;
} MESSAGE;

class MESSAGE_QUEUE
{
public:
    MESSAGE *first_message, *last_message;

    MESSAGE_QUEUE();
    ~MESSAGE_QUEUE();

    // add a message to the queue
    bool Add(char *some_text, int to);

    // pull the first message from the queue and erase it from the queue
    MESSAGE *Read();

    // read the next message in the queue without erasing it
    MESSAGE *Peek();

    MESSAGE *Read_To(int to);

    // erases the next message in the queue without reading it
    void Erase();

    // erase all messages in the queue
    void Erase_All();

};

typedef struct send_receive_struct
{
    int listening_port;
    int shut_down;
    void *global;
} SEND_RECEIVE_TYPE;


#ifdef NETWORK

#define MAX_CLIENTS 10
#define DEFAULT_LISTEN_PORT 25645

int Setup_Server_Socket(int port);

int Setup_Client_Socket(char *server, char *port);

int Accept_Incoming_Connection(int my_socket);

int Send_Message(MESSAGE *mess, int to_socket);

MESSAGE *Receive_Message(int from_socket);

void Clean_Up_Server_Socket(int my_socket);

void Clean_Up_Client_Socket(int my_socket);

int Check_For_Incoming_Data(int socket_number);

int Check_For_Errors(int socket_number);

void *Send_And_Receive(void *data_we_need);

#endif // NETWORK

#endif
