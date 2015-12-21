#ifndef	ENVIRONMENT_DEFINE
#define	ENVIRONMENT_DEFINE

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


#include "main.h"
#include "network.h"
#include "gfxData.h"
#include "text.h"


// As everything depends on environment.h, PLAYER, TANK and VIRTUAL_OBJECT
// Must be forwarded here, and included before the ENVIRONMENT definition
class VIRTUAL_OBJECT;
class TANK;
class PLAYER;


#ifndef HAS_DIRENT
#  if defined(ATANKS_IS_MSVC)
#    include "extern/dirent.h"
#  else
#    include <dirent.h>
#  endif // Linux
#  define HAS_DIRENT 1
#endif //HAS_DIRENT


#ifndef MAX_GRAVITY_DELAY
#  define GRAVITY_DELAY 200
#  define MAX_GRAVITY_DELAY 3
#endif

#define SPRING_CHANGE 1.25
#define BOUNCE_CHANGE 0.90

#define GET_R(x) ((x & 0xff0000) >> 16)
#define GET_G(x) ((x & 0x00ff00) >> 8)
#define GET_B(x) ( x & 0x0000ff)


// Something from externs.h can not be used via include
// due to circular dependencies.
extern int32_t GREY;
extern int32_t GREEN;

// Defined in sound.cpp:
extern int32_t MAX_VOLUME_FACTOR;


/** @class ENVIRONMENT
  * @brief Fixed values of the current environment the game takes place in.
  *
  * This class holds all values and the corresponding methods that define
  * the gaming environment.
  *
  * This means that all values in here must be set on game round start and
  * must not change until the game round ends.
  *
  * So basically this class consolidates everything set up with the options
  * menu and by the game round initialization.
  *
  * Everything that can change between the game round start and the game round
  * end has to be managed by GLOBALDATA.
**/
class ENVIRONMENT
{
public:

	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */
	explicit ENVIRONMENT ();
	~ENVIRONMENT ();


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */
	void     addGamePlayer      (PLAYER* player_);
	PLAYER*  createNewPlayer    (const char* player_name);
	void     creditWinners      (int32_t winner);
	void     decreaseVolume     ();
	void     deletePermPlayer   (PLAYER* player_);
	void     destroy            (); // Must be called before allegro shutdown
	void     find_config_dir    ();
	bool     find_data_dir      ();
	void     first_init         (); // Used for the first init after creation
	void     genItemsList       ();
	int32_t  getPlayerByName    (const char* player_name);
	void     increaseVolume     ();
	int32_t  ingamemenu         ();
	void     initialise         (); // Does a regular initialization
	bool     isItemAvailable    (int32_t itemNum);
	bool     loadBackgroundMusic();
	bool     loadBitmaps        ();
	bool     loadFonts          ();
	bool     loadGameFiles      ();
	void     load_from_file     (FILE *file);
	bool     loadSounds         ();
	void     load_text_files    ();
	void     newRound           ();
	void     removeGamePlayer   (PLAYER* player_);
	void     Reset_Options      ();
	bool     save_to_file       (FILE *file);
	bool     sendToClients      (const char* message); // send a short message to all network clients
	void     set_fps            (int32_t new_FPS);
	void     window_update      (int32_t x, int32_t y, int32_t w, int32_t h);


	/* ----------------------
	 * --- Public members ---
	 * ----------------------
	 */

	PLAYER**     allPlayers             = nullptr;
	int32_t      availableItems[THINGS];
	SAMPLE*      background_music       = nullptr;
	char**       bitmap_filenames       = nullptr;
	int32_t      boxedMode              = BM_OFF;
	BITMAP**     button                 = nullptr;
	bool         campaign_mode          = false;
	double       campaign_rounds        = 0.;   // 20% of the total round number
	bool         check_for_updates      = true;
	int32_t      colourDepth            = 0;
	int32_t      colourTheme            = CT_CRISPY;  // land and sky gradiant theme
	char         configDir[PATH_MAX + 1];
	int32_t      current_wallType       = 0;
	int32_t      custom_background      = 0;
	char         dataDir[PATH_MAX + 1];
	int32_t      debris_level           = 1;
	bool         detailedLandscape      = false;
	bool         detailedSky            = false;
	bool         ditherGradients        = true;
	bool         divide_money           = false;
	bool         do_box_wrap            = false;
	bool         drawBackground         = true;
	bool         dynamicMenuBg          = true;
	bool         fadingText             = false;
	int32_t      falling_dirt_balls     = 0;
	int32_t      fog                    = 0;
	int32_t      fontHeight             = 0;  // Fixed in ctor, no calls to text_height(font) needed.
	double       FPS_mod                = 0.; // Pre-calculated, used in many places.
	int32_t      frames_per_second      = 0;
	int32_t      full_screen            = FULL_SCREEN_FALSE;
	char         game_name[GAMENAMELEN + 1];
	sGfxData     gfxData;
	double       gravity                = 0.15;
	int32_t      halfHeight             = DEFAULT_SCREEN_HEIGHT / 2;
	int32_t      halfWidth              = DEFAULT_SCREEN_WIDTH / 2;
	double       interest               = 1.25;
	int32_t      itemtechLevel          = 5;
	bool         isBoxed                = false;
	bool         isGameLoaded           = true;
	int32_t      landSlideDelay         = MAX_GRAVITY_DELAY;
	int32_t      landSlideType          = SLIDE_GRAVITY;
	int32_t      landType               = LAND_RANDOM;
	eLanguages   language               = EL_ENGLISH;
	int32_t      lightning              = 0;
	bool         loadGame               = false;
	FONT*        main_font              = nullptr;
	int32_t      maxFireTime            = 0;
	int32_t      maxNumTanks            = 0;
	double       maxVelocity            = 0.;
	int32_t      max_screen_updates     = 64;
	int32_t      menuBeginY             = 0;
	int32_t      menuEndY               = 0;
	int32_t      meteors                = 0;
	BITMAP**     misc                   = nullptr;
	BITMAP**     missile                = nullptr;
	int32_t      mouseclock             = 0;
	bool         nameAboveTank          = true;
	bool         network_enabled        = false;
	int32_t      network_port           = DEFAULT_NETWORK_PORT;
	double       nextCampaignRound      = 0; // When AI players will be raised next
	int32_t      numAvailable           = 0;
	int32_t      numGamePlayers         = 0;
	int32_t      numHumanPlayers        = 0;
	int32_t      numPermanentPlayers    = 0;
	int32_t      number_of_bitmaps      = 0;
	bool         osMouse                = true; // whether we should use the OS or custom mouse
	bool         play_music             = true;
	PLAYER**     players                = nullptr;
	PLAYER*      playerOrder[MAXPLAYERS];
	uint32_t     rounds                 = 5;
	int32_t      satellite              = 0;
	uint32_t     saved_gameindex        = 0;
	const char** saved_game_list        = nullptr;
	uint32_t     saved_game_list_size   = 0;
	int32_t      scoreHitUnit           = 75;
	int32_t      scoreRoundWinBonus     = 10000;
	int32_t      scoreSelfHit           = 25;
	int32_t      scoreTeamHit           = 10;
	int32_t      scoreUnitDestroyBonus  = 5000;
	int32_t      scoreUnitSelfDestroy   = 2500;
	int32_t      screenHeight           = DEFAULT_SCREEN_HEIGHT;
	int32_t      screenWidth            = DEFAULT_SCREEN_WIDTH;
	double       sellpercent            = 0.80;
	char         server_name[129];
	char         server_port[129];
	bool         shadowedText           = true;
	bool         showAIFeedback         = true;
	bool         showFPS                = false;
	int32_t      skipComputerPlay       = SKIP_HUMANS_DEAD;
	BITMAP*      sky                    = nullptr;
	double       slope[360][2];
	int32_t      sound_driver           = SD_AUTODETECT;
	bool         sound_enabled          = true;
	SAMPLE**     sounds                 = nullptr;
	int32_t      startmoney             = 15000;
	BITMAP**     stock                  = nullptr;
	bool         swayingText            = true;
	BITMAP**     tank                   = nullptr;
	BITMAP**     tankgun                = nullptr;
	int32_t      temp_screenHeight      = 0; // 0 to detect command line arguments
	int32_t      temp_screenWidth       = 0; // versus loaded configuration.
	int32_t      time_to_fall           = 0; // amount of time dirt will hover
	BITMAP**     title                  = nullptr;
	int32_t      turntype               = TURN_RANDOM;
	double       viscosity              = 0.5;
	int32_t      violent_death          = 0;
	int32_t      voices                 = 0;
	int32_t      volley_delay           = 10; // Delay factor for volley shots, 5-50
	int32_t      volume_factor          = MAX_VOLUME_FACTOR;
	int32_t      wallColour             = GREEN;
	int32_t      wallType               = WALL_RUBBER;
	int32_t      weapontechLevel        = 5;
	BOX          window;
	int32_t      windstrength           = 8;
	int32_t      windvariation          = 1;

	// Text structures holding (translated) lines of in game text
	TEXTBLOCK* gloat        = nullptr;
	TEXTBLOCK* ingame       = nullptr;
	TEXTBLOCK* instructions = nullptr;
	TEXTBLOCK* panic        = nullptr;
	TEXTBLOCK* kamikaze     = nullptr;
	TEXTBLOCK* retaliation  = nullptr;
	TEXTBLOCK* revenge      = nullptr;
	TEXTBLOCK* suicide      = nullptr;
	TEXTBLOCK* war_quotes   = nullptr;


private:

	/* -----------------------
	 * --- Private methods ---
	 * -----------------------
	 */


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	DIR* music_dir = nullptr;
};


#define HAS_ENVIRONMENT 1

#endif

