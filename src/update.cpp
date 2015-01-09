#ifdef NETWORK
#ifdef THREADS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#include "update.h"






char *Check_For_Update(char *server_name, char *remote_file, char *host_name, char *current_version)
{
    struct update_data *my_update;
    pthread_t update_thread;

    my_update = (struct update_data *) calloc(1, sizeof(struct update_data));
    if (!my_update)
       return NULL;
    my_update->server_name = server_name;
    my_update->remote_file = remote_file;
    my_update->host_name = host_name;
    my_update->current_version = current_version;
    my_update->update_string = (char *) calloc(UPDATE_STR_LENGTH, sizeof(char));
    if (! my_update->update_string)
       return NULL;

    pthread_create( &update_thread, NULL, Get_Latest_Version, (void *) my_update);
    return my_update->update_string;
}



void *Get_Latest_Version(void *new_data)
{
   struct update_data *my_data = (struct update_data *) new_data;
   // set up socket
   int socket_num, port_number = 80;
   struct sockaddr_in server_address;
   struct hostent *server;
   char buffer[1024];
   char *found = NULL;
   int got_bytes;
   double this_version, web_version;


   socket_num = socket(AF_INET, SOCK_STREAM, 0);
   if (socket_num < 0)
      pthread_exit(NULL);
   server = gethostbyname(my_data->server_name);
   if (! server)
      pthread_exit(NULL);
   bzero((char *) &server_address, sizeof(server_address));
   server_address.sin_family = AF_INET;
   bcopy((char *) server->h_addr,
         (char *) &server_address.sin_addr.s_addr,
         server->h_length);
   server_address.sin_port = htons(port_number);

   // try to connect
   if ( connect(socket_num, (sockaddr *)&server_address, sizeof(server_address)) < 0)
      pthread_exit(NULL);


   // get HTTP data
   snprintf(buffer, 1024, "GET /%s HTTP/1.1\nHost: %s\n\n", my_data->remote_file, my_data->host_name);
   write(socket_num, buffer, strlen(buffer));
   got_bytes = read(socket_num, buffer, 1024);

   // search for version number in return data
   if (got_bytes > 1)
      found = strstr(buffer, "Version: ");
   while ( (got_bytes > 1) && (! found) )
   {
      got_bytes = read(socket_num, buffer, 1024);
      if (got_bytes > 1)
         found = strstr(buffer, "Version: ");
   }

   // compare version number
   if (found)
   {
      found += 9;
      found[5] = '\0';
      sscanf(found, "%lf", &web_version);
      sscanf(my_data->current_version, "%lf", &this_version);
      if (web_version > this_version)
         sprintf(my_data->update_string, "A new version, %2.1lf, is ready for download.", web_version);
   }

   close(socket_num);
   pthread_exit(NULL);
}

#endif
#endif
