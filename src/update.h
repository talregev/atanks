#ifndef UPDATE_HEADER_FILE__
#define UPDATE_HEADER_FILE__


#define UPDATE_STR_LENGTH 256


// rewritten struct to be used with C++11 threads.
struct update_data
{
	char* server_name         = nullptr;
	char* host_name           = nullptr;
	char* remote_file         = nullptr;
	char  update_string[1024];

	explicit update_data(const char* server_, const char* remote_,
	                     const char* host_);
	~update_data();

	void operator()();
};

#endif

