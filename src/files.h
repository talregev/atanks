#ifndef FILE_HANDLING_HEADER_
#define FILE_HANDLING_HEADER_

#define MAX_CONFIG_LINE 128
#define MAX_INSULT_LINE 256

#define NO_STAGE 0
#define GLOBAL_STAGE 1
#define ENVIRONMENT_STAGE 2
#define PLAYER_STAGE 3

#include <dirent.h>
#include "globaldata.h"
#include "environment.h"
#include "text.h"


int Save_Game(GLOBALDATA *global, ENVIRONMENT *env);
int Load_Game(GLOBALDATA *global, ENVIRONMENT *env); 
int Check_For_Saved_Game(GLOBALDATA *global);
/*
Copy the atanks config file from HOME_DIR to
HOME_DIR/.atanks
*/
int Copy_Config_File(GLOBALDATA *global);

// Make sure there is a music folder in .atanks
int Create_Music_Folder(GLOBALDATA *global);

void renderTextLines(GLOBALDATA *global, ENVIRONMENT *env, TEXTBLOCK *lines, int scrollOffset,
                     const FONT* fnt, const int spacing );

void scrollTextList(GLOBALDATA *global, ENVIRONMENT *env, TEXTBLOCK *lines);

int draw_circlesBG(GLOBALDATA *global, BITMAP *dest, int x, int y, int width, int height, bool image);

void drawMenuBackground(GLOBALDATA *global, ENVIRONMENT *env, int itemType, int tOffset, int numItems);

void flush_inputs();

int Load_Weapons_Text(GLOBALDATA *global);

// char *Get_Random_Quote(GLOBALDATA *global);

#ifdef MACOSX
int Filter_File( struct dirent *my_file );
#else
int Filter_File( const struct dirent *my_file );
#endif

struct dirent ** Find_Saved_Games(GLOBALDATA *global, int *num_files_found);

char ** Find_Bitmaps(GLOBALDATA *global, int *bitmaps_found);

BITMAP *create_gradient_strip(const gradient *gradient, int length);

int gradientColorPoint(const gradient *grad, double length, double line);

double colorDistance(int col1, int col2);

// This function removes all weapon, natural and item data.
// Should be called before re-loading weapon file.
void Clear_Weapons();

// Draw a tank bitmap on the screen at the given location'
int Display_Tank_Bitmap(ENVIRONMENT *env, int xpos, int ypos, const void *image_number);

// cause natural events to happen
void doNaturals(GLOBALDATA *global, ENVIRONMENT *env);

// give people the chance to buy items
bool buystuff(GLOBALDATA *global, ENVIRONMENT *env);

// display the bar at the top of the game screen
void drawTopBar(GLOBALDATA *global, ENVIRONMENT *env, BITMAP *dest);

// display the scoreboard in the corner of the game screen
void drawScoreboard(GLOBALDATA *global, ENVIRONMENT *env, BITMAP *dest, TANK *curTank);

int slideLand(GLOBALDATA *global, ENVIRONMENT *env);

void set_level_settings(GLOBALDATA *global, ENVIRONMENT *env);

void showRoundEndScoresAt(GLOBALDATA *global, ENVIRONMENT *env, BITMAP *bitmap, int x, int y, int winner);

#endif
