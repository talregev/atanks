#ifndef UPDATE_HEADER_FILE__
#define UPDATE_HEADER_FILE__


#ifdef NETWORK
#ifdef THREADS


#define UPDATE_STR_LENGTH 256


struct update_data
{
   char *server_name, *host_name;
   char *remote_file;
   char *update_string;
   char *current_version;
};



// this function kicks it all off
// returns the address of a string containing
// update notice. The string will have a notice
// if update is ready or be 0-length with no update.
// The function is threaded and may change the
// string after the function returns.
// The returning string is allocated and should
// be freed at the end of the calling process.
// On error, NULL is returned.
char *Check_For_Update(char *server_name, char *remote_file, char *host_name, char *current_version);



// The function/thread that checks for new updates.
void *Get_Latest_Version(void *data);


#endif
#endif

#endif

