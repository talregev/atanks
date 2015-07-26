#ifndef CLIENT_HEADER_FILE__
#define CLIENT_HEADER_FILE__

#ifdef NETWORK

#define CLIENT_VERSION 1
#define CLIENT_SCREEN 2
#define CLIENT_WIND 3
#define CLIENT_NUMPLAYERS 4
#define CLIENT_WHOAMI 5
#define CLIENT_WEAPONS 6
#define CLIENT_ITEMS 7
#define CLIENT_ROUNDS 8
#define CLIENT_NAME 9
#define CLIENT_WALL_TYPE 10
#define CLIENT_BOXED 11
#define CLIENT_TEAMS 12
#define CLIENT_SURFACE 13
#define CLIENT_TANK_POSITION 14
#define CLIENT_TANK_HEALTH 15
#define CLIENT_PLAYING 16

#define CLIENT_UP 1
#define CLIENT_DOWN 2
#define CLIENT_LEFT 3
#define CLIENT_RIGHT 4
#define CYCLE_BACK 10
#define CYCLE_FORWARD 20

#define CLIENT_ARGS 6

#endif

#define CLIENT_ERROR_VERSION 1
#define CLIENT_ERROR_SCREENSIZE 2
#define CLIENT_ERROR_DISCONNECT 3


// This function takes some data from the server
// and tries to figure out what to do with it.
// The game stage is returned.
int Parse_Client_Data(char *buffer);


// Draws a background
void Create_Sky();

// Sends fire command to the server
// Message must be in format "FIRE item angle power"
int Client_Fire(PLAYER *my_player, int my_socket);
int Client_Power(PLAYER *my_player, int up_or_down);
int Client_Angle(PLAYER *my_player, int left_or_right);
int Client_Cycle_Weapon(PLAYER *my_player, int forward_or_back);

// Take an error code and return a string with readable info.
// The returning string should NOT be freed after use.
// Note: This is nowhere used. ( REMOVEME ??? )
const char* Explain_Error(int32_t error_code);

int Game_Client(int socket_number);

#endif

