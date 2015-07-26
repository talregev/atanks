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

struct MESSAGE
{
  char *text;
  int to;   // which client does the message go to? May not be used as most will go to everyone
  void *next;
};



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



struct SEND_RECEIVE_TYPE
{
    int listening_port;
    int shut_down;
};


#define MAX_CLIENTS 10
#define DEFAULT_NETWORK_PORT 25645

#ifdef NETWORK


/// Wrapper for safe socket writes with return value check.
/// A char buffer named "buffer" must be available to put the message in.
/// The two size_t values "towrite" and "written" must be at least declared.
/// All three variables will be overwritten.
#define SAFE_WRITE(sock_, fmt_, ...) { \
	sprintf(buffer, fmt_, __VA_ARGS__); \
	towrite = strlen(buffer); \
	written = write(sock_, buffer, towrite); \
	if (written < towrite) \
		fprintf(stderr, "%s:%d: Warning: Only %d/%d bytes sent to server\n", \
				__FILE__, __LINE__, written, towrite); \
}

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

#else
	#define SAFE_WRITE(sock_, fmt_, ...) { }
#endif



#endif

