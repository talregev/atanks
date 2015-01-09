#ifndef GLOBALDATA_DEFINE
#define GLOBALDATA_DEFINE
/*
 * atanks - obliterate each other with oversize weapons
 * Copyright (C) 2003  Thomas Hudson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * */

#include <sys/types.h>
#include <dirent.h>

#include "main.h"
#include "text.h"

#define DEFAULT_SCREEN_WIDTH 800
#define DEFAULT_SCREEN_HEIGHT 600

#define LANGUAGE_ENGLISH 0.0
#define LANGUAGE_PORTUGUESE 1.0
#define LANGUAGE_FRENCH 2.0
#define LANGUAGE_GERMAN 3.0
#define LANGUAGE_SLOVAK 4.0
#define LANGUAGE_RUSSIAN 5.0
#define LANGUAGE_SPANISH 6.0
#define LANGUAGE_ITALIAN 7.0

#define COLOUR_THEME_REGULAR 0.0
#define COLOUR_THEME_CRISPY 1.0

#define GAME_NAME_LENGTH 64

#define VIOLENT_DEATH_OFF 0
#define VIOLENT_DEATH_LIGHT 1
#define VIOLENT_DEATH_MEDIUM 2
#define VIOLENT_DEATH_HEAVY 3

#define MAX_INTEREST_AMOUNT 100000
#define MAX_TEAM_AMOUNT 500000

#define DEMO_WAIT_TIME 60

#define SOUND_AUTODETECT 0
#define SOUND_OSS 1
#define SOUND_ESD 2
#define SOUND_ARTS 3
#define SOUND_ALSA 4
#define SOUND_JACK 5

#define FULL_SCREEN_EITHER 10.0
#define FULL_SCREEN_TRUE  1.0
#define FULL_SCREEN_FALSE 0.0

#define MAX_AI_TIME 60

#define ALL_SOCKETS -1


enum skipComputerPlayType
{
  SKIP_NONE, SKIP_HUMANS_DEAD// , SKIP_AUTOPLAY
};

class PLAYER;
class TANK;
class GLOBALDATA
  {
  private:
  DIR *music_dir;
  public:
    ~GLOBALDATA();
    int	WHITE, BLACK, PINK;
    double	slope[360][2];

    char	*dataDir;
    char    *configDir;
    BOX	*updates, *lastUpdates, window;
    int	updateCount, lastUpdatesCount;
    int	stopwindow;
    int	command;
    double  frames_per_second;

    PLAYER	**allPlayers;
    int	numPermanentPlayers;
    #ifdef THREADS
    pthread_rwlock_t* command_lock;
    #endif
    void wr_lock_command ();
    void unlock_command ();
    int get_command ();
    PLAYER	**players;
    int	numPlayers;
    int	numHumanPlayers;
    int	computerPlayersOnly;
    double	skipComputerPlay;	/* options requires doubles - grr */
    /* It's a lot simpler than having
     * special cases for each type */
    int	numTanks;
    int	maxNumTanks;
    TANK	*currTank;

    int	updateMenu;

    int	curland, cursky;
    int get_curland();
    void unlock_curland();
    void lock_curland();
    void destroy_curland_lock();
    void init_curland_lock();
    #ifdef THREADS
    pthread_mutex_t* curland_lock;
    #endif
    int	colourDepth;
    int	screenWidth, screenHeight;
    int menuBeginY, menuEndY;
    int	halfWidth, halfHeight;
    int     width_override, height_override;
    double  temp_screenWidth, temp_screenHeight;
    PLAYER *client_player;     // the index we use to know which one is the player on the client side
    gfxDataStruct	gfxData;
    // DATAFILE	*SOUND;
    ENVIRONMENT *env;

    // bool            full_screen;
    // int		cacheCirclesBG ;	// This is just a flag, so it need only be
						// 		an integer, not a double 
    void Reset_Options();

    /* Logically, these three variables should be ints.  However, converting
    them to ints (or even an enumerated type) would require some rewritting
    of the options function - and that's a lot of work. 2003.09.05 */
    /* Hence being double. 2004.01.05 */
    double ditherGradients;
    double detailedLandscape;
    double detailedSky;
    double os_mouse;          // whether we should use the OS or custom mouse
    double colour_theme;     // land and sky gradiant theme
    double sound_driver;

    /* All this money data; couldn't it be moved into some separate data
    structure or object */
    /* It could, but it's not a problem */
    double	startmoney;
    double	interest;
    double	scoreHitUnit;
    double	scoreSelfHit;
    double	scoreUnitDestroyBonus;
    double	scoreUnitSelfDestroy;
    double	scoreRoundWinBonus;
    double  sellpercent;
    double  divide_money;
    double  play_music;
    double full_screen;
    int show_scoreboard;
    char server_name[128], server_port[128];

    /* double? */
    /* double for options() reasons, no messing about with casting or
     * special cases. */
    double	turntype;
    double	rounds;
    int     currentround;
    double  sound;
    double  language;
    int     name_above_tank;
    int     tank_status_colour;
    char   *tank_status;
    char    game_name[GAME_NAME_LENGTH];
    double  load_game;
    double  campaign_mode;
    double  violent_death;
    double  saved_game_index;
    char    **saved_game_list;
    double  max_fire_time;
    bool close_button_pressed;
    char *update_string;
    double check_for_updates;
    bool demo_mode;
    double enable_network, listen_port;
    int draw_background;
    BITMAP **button, **misc, **missile, **stock, **tank, **tankgun, **title;
    SAMPLE **sounds;
    SAMPLE *background_music;
    FONT *unicode, *regular_font;
    TEXTBLOCK *war_quotes, *instructions, *ingame;
    TEXTBLOCK *gloat, *revenge, *retaliation, *suicide, *kamikaze;
    char *client_message;   // message sent from client to main menu


    GLOBALDATA ();
    void	initialise ();
    int     saveToFile_Text (FILE *file);
    int     loadFromFile_Text (FILE *file);
    int     loadFromFile(ifstream &my_file);
    void	addPlayer (PLAYER *player);
    void	removePlayer (PLAYER *player);
    PLAYER	*getNextPlayer (int *playerCount);
    PLAYER	*createNewPlayer (ENVIRONMENT *env);
    void	destroyPlayer (PLAYER *player);
    char    *Get_Config_Path();
    bool    Check_Time_Changed();      // check to see if one second has passed
    bool		bIsGameLoaded;
    bool		bIsBoxed;
    int			iHumanLessRounds;
    double	dMaxVelocity;
    int Load_Sounds();
    int Load_Bitmaps();
    SAMPLE *Load_Background_Music();
    int Load_Fonts();
    void Change_Font();
    void Update_Player_Menu();
    int Load_Text_Files();
    #ifdef NETWORK
    int Send_To_Clients(char *message);        // send a short message to all network clients
    #endif
#ifdef DEBUG_AIM_SHOW
    bool bASD;
#endif
    int Find_Data_Dir();
    double Calc_Max_Velocity();
    int Count_Humans();
    int Check_For_Winner();    // returns winner index or -1 for no winner
    void Credit_Winners(int winner);
  };

#endif
