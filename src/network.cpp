#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef NETWORK
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#endif

#include "network.h"
#include "player.h"
#include "globaldata.h"

// init the object
MESSAGE_QUEUE::MESSAGE_QUEUE()
{
    first_message = last_message = NULL;
}

// do clean up on all messages
MESSAGE_QUEUE::~MESSAGE_QUEUE()
{
    Erase_All();
}

// add a new message to the queue
// Returns true on success and false is an error occures
bool MESSAGE_QUEUE::Add(char *some_text, int to)
{
    MESSAGE *new_message;

    if (!some_text)
        return false;

    new_message = (MESSAGE *) calloc(1, sizeof(MESSAGE));
    if (!new_message)
        return false;
    new_message->text = (char *) calloc(strlen(some_text), sizeof(char));
    if (!new_message->text)
    {
        free(new_message);
        return false;
    }

    // fill the message structure
    strcpy(new_message->text, some_text);
    new_message->to = to;
    // next is already cleared by calloc

    // empty line, create new line
    if (!first_message)
    {
        first_message = new_message;
        last_message = new_message;
    }

    else // we have a line, add it to the end
    {
        last_message->next = new_message;
        last_message = new_message;
    }
    return true;
}

// retrieve a message and erase it from the queue
// returns a message on success and NULL on failure
MESSAGE *MESSAGE_QUEUE::Read()
{
    MESSAGE *my_message;

    my_message = Peek();       // grab next message
    if (my_message)
        Erase();            // clear it from the queue

    return my_message;
}

// returns a message from the queue without removing it from the line
// Returns a message on success or a NULL on failure
MESSAGE *MESSAGE_QUEUE::Peek()
{
    MESSAGE *my_message;

    // see if there is a message to get
    if ((!first_message) || (!first_message->text))
        return NULL;

    // we want to make a copy of the message, not just pass back a pointer
    my_message = (MESSAGE *) calloc(1, sizeof(MESSAGE));
    if (!my_message)
        return NULL;
    my_message->text = (char *) calloc(strlen(first_message->text), sizeof(char));
    if (!my_message->text)
    {
        free(my_message);
        return NULL;
    }

    // we have an empty message. Now fill it
    strcpy(my_message->text, first_message->text);
    my_message->to = first_message->to;

    return my_message;
}

// Finds the next message destined "to" a given player
// This function returns the first message it finds and
// erases it. The message is returned on success or a
// NULL is returned if no message is found.
MESSAGE *MESSAGE_QUEUE::Read_To(int my_to)
{
    MESSAGE *current, *previous = NULL, *my_message = NULL;
    bool found = false;

    // search for matching to field
    current = first_message;
    while ((current) && (!found))
    {
        if (current->to == my_to)
            found = true;
        else
        {
            previous = current;
            current = (MESSAGE *) current->next;
        }
    }

    if (!found)
        return NULL;

    // found match, create a copy and erase the original
    my_message = (MESSAGE *) calloc(1, sizeof(MESSAGE));
    if (!my_message)
        return NULL;

    my_message->text = (char *) calloc(strlen(current->text), sizeof(char));
    if (!my_message->text)
    {
        free(my_message);
        return NULL;
    }

    my_message->to = current->to;
    strcpy(my_message->text, current->text);
    if (previous)
        previous->next = current->next;
    else
        first_message = (MESSAGE *) current->next;

    if (last_message == current)
        last_message = previous;

    free(current->text);
    free(current);
    return my_message;
}

// Erases the next message in the line without returning anything
void MESSAGE_QUEUE::Erase()
{
    MESSAGE *next_in_line;

    if (first_message)
    {
        next_in_line = (MESSAGE *) first_message->next;
        // clean up
        if (first_message->text)
        {
            free(first_message->text);
            first_message->text = NULL;
        }
        free(first_message);

        // see if we are at the end of the list
        if (last_message == first_message)
            last_message = next_in_line;

        first_message = next_in_line;
    }

}

// This function erases all messages in the queue.
void MESSAGE_QUEUE::Erase_All()
{
    MESSAGE *current, *coming_up;

    current = first_message;
    while (current)
    {
        coming_up = (MESSAGE *) current->next;
        if (current->text)
            free(current->text);
        free(current);
        current = coming_up;
    }

    first_message = last_message = NULL;
}

//
// Past this point we have socket functions.
//
//
#ifdef NETWORK

// Create a socket for listening. Returns a listening socket
// on success or -1 on failure.
int Setup_Server_Socket(int port)
{
    int listensocket;
    struct sockaddr_in myaddr;

    memset(&myaddr, 0, sizeof(myaddr));
    listensocket = socket(AF_INET, SOCK_STREAM, 0);
    myaddr.sin_port = htons(port);
    myaddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(listensocket, (struct sockaddr *) &myaddr, sizeof(myaddr)) < 0)
    {
        printf("Bind failed: %s\n", strerror(errno));
        return -1;
    }
    if (listen(listensocket, 5))
    {
        printf("Listen failed: %s\n", strerror(errno));
        return -1;
    }

    return listensocket;
}

// Connect to a remote server. Returns -1 on failure or
// a socket (int) on success.
int Setup_Client_Socket(char *server_name, char *port)
{
    int socket_num, port_number;
    struct sockaddr_in server_address;
    struct hostent *server;

    sscanf(port, "%d", &port_number);
    socket_num = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_num < 0)
        return -1;
    server = gethostbyname(server_name);
    if (!server)
        return -1;
    memset((char *) &server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    memcpy((char *) &server_address.sin_addr.s_addr, (char *) server->h_addr, server->h_length);
    server_address.sin_port = htons(port_number);

    // try to connect
    if (connect(socket_num, (sockaddr *)&server_address, sizeof(server_address)) < 0)
        return -1;

    return socket_num;
}

// Accepts an incoming connection request. Returns the
// new socket connection on success or -1 on failure.
int Accept_Incoming_Connection(int my_socket)
{
    int new_connection;
    socklen_t my_length;
    struct sockaddr new_address;

    my_length = sizeof(new_address);
    new_connection = accept(my_socket, (struct sockaddr *) &new_address, &my_length);
    return new_connection;
}

// Take whatever is in the message and send it
// to a socket.
// Returns a negative number on failure. Zero and
// positive numbers indicate success.
int Send_Message(MESSAGE *mess, int to_socket)
{
    int status;
    char buffer[MAX_MESSAGE_LENGTH];

    strncpy(buffer, mess->text, MAX_MESSAGE_LENGTH);
    buffer[MAX_MESSAGE_LENGTH - 1] = '\0';
    status = write(to_socket, buffer, strlen(buffer));
    return status;
}

// Read data from a socket and put it in a message
// Returns the message on success and NULL on failure.
// Note: the "to" field of the message is not set.
MESSAGE *Receive_Message(int from_socket)
{
    MESSAGE *my_message;
    int bytes_read;
    char buffer[MAX_MESSAGE_LENGTH];

    my_message = (MESSAGE *) calloc(1, sizeof(MESSAGE));
    if (!my_message)
        return NULL;

    memset(buffer, '\0', MAX_MESSAGE_LENGTH);
    bytes_read = read(from_socket, buffer, MAX_MESSAGE_LENGTH);
    if (bytes_read > 0)
    {
        buffer[MAX_MESSAGE_LENGTH - 1] = '\0';
        my_message->text = (char *) calloc(strlen(buffer), sizeof(char));
        if (!my_message->text)
        {
            free(my_message);
            return NULL;
        }
        strcpy(my_message->text, buffer);
        return my_message;
    }
    else   // error while reading
    {
        free(my_message);
        return NULL;
    }

}

void Clean_Up_Server_Socket(int my_socket)
{
    close(my_socket);
}

void Clean_Up_Client_Socket(int my_socket)
{
    close(my_socket);
}

// This function checks the socket to see if there is data ready to
// be read. If there is data ready, then the function returns TRUE. If
// an error occures, the function returns -1. If there is no data
// and no error, the function returns FALSE.
int Check_For_Incoming_Data(int socket_number)
{
    fd_set rfds;
    struct timeval tv;
    int retval;

    FD_ZERO(&rfds);
    FD_SET(socket_number, &rfds);

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    retval = select(socket_number + 1, &rfds, NULL, NULL, &tv);

    if (retval == -1)
        return -1;

    else if (retval)
        return TRUE;

    else
        return FALSE;

    return TRUE;
}

// This function checks the passed socket for errors. If the socket
// is error-free, the function returns FALSE. If an exception
// has occured, the function returns TRUE.
int Check_For_Errors(int socket_number)
{
    fd_set exds;
    struct timeval tv;
    int expval;

    FD_ZERO(&exds);
    FD_SET(socket_number, &exds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    expval = select(socket_number + 1, NULL, NULL, &exds, &tv);

    if (expval == -1)
        return TRUE;
    else if (expval)
        return TRUE;
    else
        return FALSE;
}

// This function will probably be called as a separate thread at the
// start of the game. It will set up a listening port, accept
// incoming connections and manage them. That is, they will be
// passed on to AI players. 
void *Send_And_Receive(void *all_the_data)
{
    SEND_RECEIVE_TYPE *send_receive_data = (SEND_RECEIVE_TYPE *) all_the_data;
    int server_socket, new_socket;
    int counter;
    GLOBALDATA *global = (GLOBALDATA *) send_receive_data->global;
    bool found;

    // set up listening socket
    server_socket = Setup_Server_Socket(send_receive_data->listening_port);
    if (server_socket == -1)
    {
        printf("Error creating listening socket.\n");
        pthread_exit(NULL);
    }

    while (!send_receive_data->shut_down)
    {
        // check for incoming connections
        int status = Check_For_Incoming_Data(server_socket);
        if (status == TRUE) // it could be either -1 (ERROR) or 0 (FALSE)
        {
            new_socket = Accept_Incoming_Connection(server_socket);
            printf("Accepted connection.\n");
            // give connection to AI player
            found = false;
            counter = 0;
            while ((!found) && (counter < global->numPlayers))
            {
                if ( (global->players[counter]->type >= USELESS_PLAYER)
                     && (global->players[counter]->type <= DEADLY_PLAYER) )
                {
                    found = true;
                    global->players[counter]->server_socket = new_socket;
                    global->players[counter]->previous_type = global->players[counter]->type;
                    global->players[counter]->type = NETWORK_CLIENT;
                    printf("Assigned connection to %s\n", global->players[counter]->getName());
                }
                else
                    counter++;
            }
            // in case we did not find a match
            if (!found)
            {
                printf("Unable to assign new connection to player.\n");
                write(new_socket, "NOROOM", strlen("NOROOM"));
                close(new_socket);
            }
        }

        // consider resting for a moment?
        usleep(10000);
    }

    // clean up everything
    printf("Cleaning up networking\n");
    Clean_Up_Server_Socket(server_socket);
    counter = 0;
    while (counter < global->numPlayers)
    {
        if (global->players[counter]->type == NETWORK_CLIENT)
        {
            write(global->players[counter]->server_socket, "CLOSE", strlen("CLOSE"));
            close(global->players[counter]->server_socket);
        }
        counter++;
    }

    pthread_exit(NULL);
}

#endif
