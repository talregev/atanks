#define ATANKS_SRC_ATANKS_CPP 1

/*
 * atanks - obliterate each other with oversize weapons
 * Copyright (C) 2002,2003  Thomas Hudson,Juraj Michalek
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

#include "debug.h"
#include "globals.h"
#include "optionscreens.h"
#include "player.h"
#include "button.h"
#include "files.h"
#include "update.h"
#include "tank.h"
#include "beam.h"
#include "missile.h"
#include "gameloop.h"
#include "clock.h"


#ifdef NETWORK
# include <thread>
# include "client.h"
#endif


#define HELP_REQUESTED -100
#define SWITCH_HELP "-h"
#define SWITCH_FULL_SCREEN "-fs"
#define SWITCH_WINDOWED "--windowed"
#define SWITCH_NOSOUND "--nosound"
#define SWITCH_DATADIR "--datadir"
#define SWITCH_CONFIGDIR "-c"
#define SWITCH_NO_CONFIG "--noconfig"


/*****************************
*** static local variables ***
*****************************/
static bool        allow_network    = true;
static char        fullPath[PATH_MAX + 1] = { 0 };
static eFullScreen full_screen      = FULL_SCREEN_EITHER;
static bool        load_config_file = true;
static int32_t     screen_mode      = GFX_AUTODETECT_WINDOWED;
#ifdef NETWORK
static int32_t     client_socket    = -1;
#endif // NETWORK


/*************************
*** External variables ***
*************************/
extern WEAPON  weapon[WEAPONS];    // from files.cpp
extern WEAPON  naturals[NATURALS]; // from files.cpp
extern ITEM    item[ITEMS];        // from files.cpp


/*****************************
*** static local functions ***
*****************************/
static void    Change_Settings(bool old_sound, int32_t old_itech, int32_t old_wtech);
static void    close_button_handler(void);
static void    createConfig();
static void    credits();
static
const  char*   do_winner();
static void    endgame_cleanup();
       void    init_mouse_cursor();
static void    init_game_settings();
static void    initialisePlayers();
static bool    loadConfig();
static bool    loadPlayers(FILE* file);
static int32_t menu();
static void    newgame();
static int32_t parse_args(int32_t argc, char** argv);
static void    play_demo();
static void    play_local();
static void    play_networked();
static void    print_text_help();
static void    print_text_initmsg();
static bool    Save_Game_Settings(const char* path);
static void    show_options();
static void    title();


/*****************************
*** external functions     ***
*****************************/
void draw_simple_bg(bool drawImage);  // from shop.cpp
void quickChange   (bool clearerror); // from shop.cpp


/*******************************
*** Function implementations ***
*******************************/

/** @brief Take care of changed settings.
  *
  * This function detects changes to some environment settings and, if a
  * change has happened, makes the required changes to the game environment.
**/
static void Change_Settings(bool old_sound, int32_t old_itech, int32_t old_wtech)
{
	// first, check for a change in the sound settings
	if (old_sound != env.sound_enabled) {
		if (env.sound_enabled) {
			if (detect_digi_driver(DIGI_AUTODETECT)) {
				if (install_sound (DIGI_AUTODETECT, MIDI_NONE, NULL) < 0)
					fprintf (stderr, "install_sound: failed turning on sound\n");
			} else
				fprintf (stderr, "detect_digi_driver found no sound device\n");
		} else
			remove_sound();
	} // End of sound checking

	// Check for tech level changes
	if ( (old_itech != env.itemtechLevel)
	  || (old_wtech != env.weapontechLevel) )
		env.genItemsList();
}


/** @brief Close Button Handler
  *
  * This function catches the close command, usually given by the user pressing
  * the close window button. We'll try to clean-up.
**/
static void close_button_handler(void)
{
	global.pressCloseButton();
}


/// @brief Show the credits file in a text box
static void credits ()
{
	snprintf (path_buf, PATH_MAX, "%s/credits.txt", env.dataDir);

	TEXTBLOCK my_text(path_buf);
	scrollTextList (&my_text);
}


/// @brief create a fresh new config if loading was prohibited or failed
static void createConfig()
{
	env.numPermanentPlayers = 0;

	// Override full screen settings from command line
	if ( (full_screen == FULL_SCREEN_TRUE)
	  || (full_screen == FULL_SCREEN_FALSE) )
		env.full_screen = full_screen;

	// Determine basic screen settings
	env.temp_screenWidth  = env.screenWidth;
	env.temp_screenHeight = env.screenHeight;
	env.halfWidth         = env.screenWidth / 2;
	env.halfHeight        = env.screenHeight / 2;
	env.menuBeginY        = (env.screenHeight - 400) / 2;
	if (env.menuBeginY < 0)
		env.menuBeginY = 0;
	env.menuEndY = env.screenHeight - env.menuBeginY;

	// Perform game initialization
	init_game_settings ();
	env.load_text_files();
	init_mouse_cursor();

	// At least one human player must be created
	PLAYER *tempPlayer      = nullptr;
	int32_t tempRes         = PE_BACK; // ePlayerEdit, player_types.h
	char    noHumanMsg[200] = { 0 };

	while (!(tempRes & PE_CONFIRM_NEW)) {
		tempRes = new_player(&tempPlayer, 0);

		if (tempPlayer) {
			// Error case 1: The created player is an AI player
			if (HUMAN_PLAYER != tempPlayer->type) {
				snprintf(noHumanMsg, 199,
				         "The player \"%s\" is no human player!",
				         tempPlayer->getName());
				errorMessage = noHumanMsg;
				errorX       = env.halfWidth - text_length(font, errorMessage) / 2;
				errorY       = env.menuBeginY + 15;
				tempPlayer   = nullptr; // It is saved already
				tempRes      = PE_BACK;
			}
		} else {
			// error case 2: No player was created at all
			strncpy(noHumanMsg, "Please create at least one human player!", 199);
			errorMessage = noHumanMsg;
			errorX       = env.halfWidth - text_length(font, errorMessage) / 2;
			errorY       = env.menuBeginY + 15;
			tempRes = PE_BACK;
		}
	} // End of force-creating a human player

	// Default AI player names
	const char* const defaultNames[] = {
		"Caesar",
		"Alex",
		"Hatshepsut",
		"Patton",
		"Napoleon",
		"Attila",
		"Catherine",
		"Hannibal",
		"Stalin",
		"Mao"
	};

	for (int32_t i = 0; i < 10; ++i) {
		tempPlayer = env.createNewPlayer(defaultNames[i]);
		tempPlayer->type = static_cast<playerType>(rand () % (LAST_PLAYER_TYPE - 1) + 1);
		tempPlayer->generatePreferences();
	}
}


/// @brief Draw the endgame screen and return the winner name
/// or nullptr if no winner was found. The returned text is static
/// and must *NOT* be freed.
static const char* do_winner()
{
	static char return_string[257] = { 0 };

	// Get the dimensions of the score board and the texts right:
	int32_t lh = env.fontHeight + 3; // The line height.
	int32_t pd = 10; // Padding. How much space to the board border.

	// Find out longest player name and score length do determine the
	// score board size and score entry positions
	char    head_name[5]   = "Name";
	char    head_value[6]  = "Value";
	char    head_score[30] = { 0 };
	snprintf(head_score, 29, " %6s %6s %6s %6s", "Kills",
	         "Killed", "Diff", "Won");

	int32_t namLen = text_length(font, head_name);
	int32_t valLen = text_length(font, head_value);
	int32_t scoLen = text_length(font, head_score);

	// While checking for the winner, determine the real lengths needed
	int32_t  idx_jedi    = -1; // Jedi Player with the highest score
	int32_t  idx_neutral = -1; // Neutral player with the highest score
	int32_t  idx_sith    = -1; // SitH Player with the highest score
	int32_t  idx_winner  = -1; // Player with the highest score
	int32_t  maxdiff     = INT32_MIN;
	int32_t  maxscore    = -1;
	int32_t  maxkills    = -1;
	int32_t  minkilled   = INT32_MAX;
	bool     multiwinner = false;
	PLAYER** players     = env.players; // short cut
	int32_t  pl_money[MAXPLAYERS] = { 0 };

	for (int32_t z = 0; z < env.numGamePlayers; z++) {

		// Check the length of the name
		int32_t curLen = text_length(font, players[z]->getName());
		if (curLen > namLen)
			namLen = curLen;

		// Sum up weapons worth
		for (int32_t j = 0; j < WEAPONS; ++j) {
			if (weapon[j].amt && players[z]->nm[j])
				pl_money[z] += (weapon[j].cost / weapon[j].amt) * players[z]->nm[j];
		}

		// Sum up items worth
		for (int32_t j = 0; j < ITEMS; ++j) {
			if (item[j].amt && players[z]->ni[j])
				pl_money[z] += (item[j].cost / item[j].amt) * players[z]->ni[j];
		}

		// Check value length
		char valTxt[16] = { 0 };
		snprintf(valTxt, 15, " %14s", Add_Comma(pl_money[z]));
		curLen = text_length(font, valTxt);
		if (curLen > valLen)
			valLen = curLen;

		// Check the length of the score
		char    scoTxt[30] = { 0 };
		int32_t kill_diff  = players[z]->kills - players[z]->killed;
		snprintf(scoTxt, 29, " %6d %6d %6d %6d",
					players[z]->kills,
					players[z]->killed,
					kill_diff,
					players[z]->score);
		curLen = text_length(font, scoTxt);
		if (curLen > scoLen)
			scoLen = curLen;

		// Determine whether a new winner or a draw situation is found
		if ( (players[z]->score  == maxscore)
		  && (players[z]->kills  == maxkills)
		  && (players[z]->killed == minkilled) ) {
			multiwinner = true;
			if (TEAM_NEUTRAL == players[z]->team)
				idx_neutral = z;
		} else if ( (players[z]->score > maxscore)
		         || ( (players[z]->score == maxscore)
		           && (kill_diff > maxdiff) )
		         || ( (players[z]->score == maxscore)
		           && (kill_diff == maxdiff)
		           && (players[z]->kills > maxkills) ) ) {
			// Note: killed doesn't need to be checked. the same
			// amount of kills with less killed score would mean
			// a better diff anyway.
			maxdiff     = kill_diff;
			maxkills    = players[z]->kills;
			maxscore    = players[z]->score;
			minkilled   = players[z]->killed;
			idx_winner  = z;
			multiwinner = false;
			if (TEAM_NEUTRAL == players[z]->team)
				idx_neutral = z;
        }

		if (TEAM_JEDI == players[z]->team)
			idx_jedi = z;
		if (TEAM_SITH == players[z]->team)
			idx_sith = z;
	} // end of checking players

	// Now calculate the dimensions of our score board.
	int32_t w  = namLen + valLen + scoLen + (2 * pd);
	int32_t h  = ((env.numGamePlayers + 4) * lh) + (2 * pd);
	int32_t x  = env.halfWidth  - (w / 2);
	int32_t y  = env.halfHeight - (h / 2);
	int32_t qy = y + h + pd;
	BOX     qarea(x + pd, qy, w - (2 * pd), env.screenHeight - pd - qy);

	//stop mouse during drawing
	SHOW_MOUSE(nullptr)

	global.make_update (x, y - pd - env.misc[9]->h, w, h + pd + env.misc[9]->h);

	// Draw the winning bitmap, the background and the border
	draw_simple_bg(false);
	draw_sprite(global.canvas, env.misc[9],
	            env.halfWidth  - (env.misc[9]->w / 2),
	            y - env.misc[9]->h - pd);
	rectfill(global.canvas, x, y, x + w, y + h, BLACK);
	rect    (global.canvas, x, y, x + w, y + h, WHITE);
	rect    (global.canvas, x + 1, y + 1, x + w - 1, y + h - 1, GREY);

	// Add the padding now, or it must be summed in everywhere!
	x += pd;
	y += pd;
	w -= 2 * pd;
	h -= 2 * pd;

	// Draw winner names and info about all players
	if (multiwinner) {
		// check for team win
		if ( TEAM_JEDI == players[idx_winner]->team ) {
			if ( ( (idx_sith >= 0)
			    && (players[idx_sith]->score    == players[idx_winner]->score) )
			  || ( (idx_neutral >= 0)
			    && (players[idx_neutral]->score == players[idx_winner]->score) ) )
			snprintf(return_string, 256, "%s", env.ingame->Get_Line(48));
          else
			snprintf(return_string, 256, "%s", env.ingame->Get_Line(45));
		} else if ( TEAM_SITH == players[idx_winner]->team ) {
			if ( ( (idx_jedi >= 0)
			    && (players[idx_jedi]->score    == players[idx_winner]->score) )
			  || ( (idx_neutral >= 0)
			    && (players[idx_neutral]->score == players[idx_winner]->score) ) )
			snprintf(return_string, 256, "%s", env.ingame->Get_Line(48));
          else
			snprintf(return_string, 256, "%s", env.ingame->Get_Line(46));
		} else
			snprintf(return_string, 256, "%s", env.ingame->Get_Line(48));
	} else {
		if ( TEAM_JEDI == players[idx_winner]->team )
			snprintf(return_string, 256, "%s", env.ingame->Get_Line(45));
		else if (TEAM_SITH == players[idx_winner]->team)
			snprintf(return_string, 256, "%s", env.ingame->Get_Line(46));
		else
			snprintf(return_string, 256, "%s: %s", env.ingame->Get_Line(47),
					 players[idx_winner]->getName() );
	}

	// Print the title lines
	textprintf_centre_ex(global.canvas, font, env.halfWidth, y,
	                     players[idx_winner]->color, -1, "%s", return_string);

	// to make the following easier, skip the two used lines
	// (The title and one blank)
	y += 2 * lh;

	// Second title line, the score board header
	int32_t valStart = x + namLen;
	int32_t scoStart = valStart + valLen;
	int32_t scoWidth = scoLen / 4;

	textout_ex    (global.canvas, font, "Name", x, y, WHITE, -1);
	textprintf_right_ex (global.canvas, font, valStart + valLen, y, WHITE,
	                     -1, " %14s", "$ Value");
	textprintf_right_ex (global.canvas, font, scoStart + (1 * scoWidth),
						y, GREEN, -1, " %6s", "Kills");
	textprintf_right_ex (global.canvas, font, scoStart + (2 * scoWidth),
						y,   RED, -1, " %6s", "Killed");
	textprintf_right_ex (global.canvas, font, scoStart + (3 * scoWidth),
						y, WHITE, -1, " %6s", "Diff");
	textprintf_right_ex (global.canvas, font, scoStart + (4 * scoWidth),
						y, WHITE, -1, " %6s", "Won");

	// Now get the score list:
	sScore* score_array = sort_scores();

	// And get the head entry:
	sScore* score = score_array;
	while (score->prev)
		score = score->prev;


	// Eventually the player scores can be displayed:
	// (again skip the previous line for easier reading/doing below)
	y   += lh;
	int32_t z = 0;
	while (score) {
		textout_ex    (global.canvas, font, score->name,
						x, y + (z * lh), score->color, -1);
		textprintf_right_ex (global.canvas, font, valStart + valLen,
							y + (z * lh), WHITE,
							 -1, " %14s", Add_Comma(pl_money[score->idx]));
		textprintf_right_ex (global.canvas, font, scoStart + (1 * scoWidth),
							y + (z * lh), GREEN, -1, " %6d", score->kills);
		textprintf_right_ex (global.canvas, font, scoStart + (2 * scoWidth),
							y + (z * lh),   RED, -1, " %6d", score->killed);
		textprintf_right_ex (global.canvas, font, scoStart + (3 * scoWidth),
							y + (z * lh), score->diff < 0 ? RED : GREEN, -1,
							" %6d", score->diff);
		textprintf_right_ex (global.canvas, font, scoStart + (4 * scoWidth),
							y + (z * lh), WHITE, -1, " %6d", score->score);
		++z;
		score = score->next;
	}
	global.do_updates();

	// Add a war quote:
	const char* quote = env.war_quotes->Get_Random_Line();
	if (quote)
		draw_text_in_box(&qarea, quote, false);

	// Clean up
	delete [] score_array;

	return return_string;
}


static void endgame_cleanup()
{
	while (env.numGamePlayers > 0) {
		if (env.players[0]->tank) {
			delete env.players[0]->tank;
			env.players[0]->tank = nullptr;
		}

		// make sure networked clients say good-bye and return
		// to old AI level
		if (env.players[0]->type >= NETWORK_CLIENT)
			env.players[0]->type =
				env.players[0]->previous_type;

		env.removeGamePlayer(env.players[0]);
	}

	global.clear_objects();
}


#if defined(ATANKS_IS_MSVC) && defined(ATANKS_DEBUG)
// this removes the keyboard and mouse handler on break events,
// hopefully making both operational in Visual Studio again if
// a crash was caught. It does not work on breakpoints though,
// that doesn't trigger the Handler.
BOOL WINAPI ctrlHandler(DWORD CtrlType)
{
	if ( ( CTRL_BREAK_EVENT == CtrlType )
		|| ( CTRL_C_EVENT == CtrlType ) ) {
		remove_mouse();
		remove_keyboard();
	}

	return FALSE;
}
#endif // Microsoft Visual C++ and Debug


void init_mouse_cursor()
{
	if (env.osMouse )
		show_os_cursor(MOUSE_CURSOR_ARROW);
	else {
		set_mouse_sprite (env.misc[0]);
		set_mouse_sprite_focus (0, 0);
	}
}


static void init_game_settings()
{
	if (env.full_screen == FULL_SCREEN_TRUE)
		env.osMouse = false;

	int32_t status = allegro_init();

	if (status) {
		fprintf(stderr, "Unable to start Allegro.\nStatus %d", status);
		exit(1);
	}

	// Be sure no vsync is used:
	const char* no_vsync = get_config_string("graphics", "disable_vsync", "no");
	if ( strcasecmp("yes", no_vsync) )
		set_config_string("graphics", "disable_vsync", "yes");

	set_window_title( "Atomic Tanks V" VERSION);

	// Before we get started make sure, that if we are using full
	// screen mode, we have to ignore width and height settings.
	if (env.full_screen == FULL_SCREEN_TRUE) {
		status = get_desktop_resolution(&env.screenWidth, &env.screenHeight);
		if (status < 0) {
			env.screenWidth = 800;
			env.screenHeight = 600;
		}
		screen_mode = GFX_AUTODETECT_FULLSCREEN;
	}

	// check for X pressed on the window bar
	LOCK_FUNCTION(close_button_handler);
	set_close_button_callback(close_button_handler);

	// Ensure sane colour depth
	if (! env.colourDepth)
		env.colourDepth = desktop_color_depth();

	if ( (env.colourDepth != 16) && (env.colourDepth != 32) )
		env.colourDepth = 16;
	set_color_depth (env.colourDepth);

	// Now the screen mode can be set
	if (set_gfx_mode(screen_mode, env.screenWidth, env.screenHeight, 0, 0) < 0) {
		perror( "set_gfx_mode");

		status = set_gfx_mode(screen_mode, 800, 600, 0, 0);

		if ( status < 0 )
			exit(1);
		env.screenWidth = 800;
		env.screenHeight = 600;
    }
	enable_triple_buffer();

	env.halfWidth = env.screenWidth / 2;
	env.halfHeight = env.screenHeight / 2;

#ifdef ATANKS_IS_MSVC
# if defined(ATANKS_DEBUG)
	SetConsoleCtrlHandler(ctrlHandler, TRUE);
#endif // DEBUG
	if ( env.full_screen == FULL_SCREEN_TRUE )
		set_display_switch_mode(SWITCH_BACKAMNESIA);
	else
		set_display_switch_mode(SWITCH_BACKGROUND);
#endif // ATANKS_IS_WINDOWS

	if (install_keyboard () < 0) {
		perror ( "install_keyboard failed");
		exit (1);
	}

	if (install_mouse () < 0)
		perror ( "install_mouse failed");

	// check to see if we want sound
	if (env.sound_enabled) {
		int32_t sound_type = DIGI_AUTODETECT;

#       ifdef ATANKS_IS_LINUX
		switch ( env.sound_driver ) {
			case SD_OSS: sound_type = DIGI_OSS; break;
			case SD_ESD: sound_type = DIGI_ESD; break;
			case SD_ARTS: sound_type = DIGI_ARTS; break;
			case SD_ALSA: sound_type = DIGI_ALSA; break;
			case SD_JACK: sound_type = DIGI_JACK; break;
			default: sound_type = DIGI_AUTODETECT; break;
		}
#         ifdef UBUNTU
			if (DIGI_AUTODETECT == sound_type)
				sound_type = DIGI_OSS;
#         endif // UBUNTU
#       endif // ATANKS_IS_LINUX

		int32_t channels = detect_digi_driver(sound_type);

		if (!channels && (DIGI_AUTODETECT != sound_type)) {
			sound_type = DIGI_AUTODETECT;
			channels   = detect_digi_driver(sound_type);
		}

		if (channels) {
			env.voices = channels >  64 ?  32
			           : channels >  32 ?  16
			           : channels >  16 ?   8
			           : channels;

			int32_t snd_installed = -1;

			while ( (env.voices > 1) && (0 > snd_installed)) {
				DEBUG_LOG("Sound Init", "Reserving %d / %d voices", env.voices, channels)
				reserve_voices(env.voices, 0);
				snd_installed = install_sound(sound_type, DIGI_NONE, nullptr);

				// Instead of failing directly, reduces voices first
				if (-1 == snd_installed) {
					DEBUG_LOG("Sound Init", "Too many voices, halving...", 0)
					env.voices /= 2;
				}
			}

			// Now display an error message if it was not possible to succeed
			if (0 > snd_installed) {
				fprintf (stderr, "install_sound: failed initialising sound\n");
				fprintf (stderr, "Please try selecting a different Sound Driver"
							     " from the Options menu.\n");
			} else {
				int32_t set_voices = get_mixer_voices();
				DEBUG_LOG("Sound Init", "Mixer has %d voices", set_voices)

				if (set_voices < env.voices)
					env.voices = set_voices;

				// Set the mixer quality:
				int32_t mixq = get_mixer_quality();
				if (mixq < 2) {
					DEBUG_LOG("Sound Init", "Raising mixer quality from %d to 2",
								mixq)
					set_mixer_quality(2);
				}

			}
		} else
			fprintf (stderr, "detect_digi_driver detected no sound device\n");
	} // End of sound initialization

  	// Colour initialization, must be done here when allegro is initialized.
	BLACK       = makecol (0x00, 0x00, 0x00);
	BLUE        = makecol (0x00, 0x00, 0xff);
	DARK_GREEN  = makecol (0x00, 0x50, 0x00);
	DARK_GREY   = makecol (0x40, 0x40, 0x40);
	DARK_RED    = makecol (0x80, 0x00, 0x00);
	GOLD        = makecol (0xaf, 0xaf, 0x00);
	GREY        = makecol (0x80, 0x80, 0x80);
	GREEN       = makecol (0x00, 0xff, 0x00);
	LIGHT_GREEN = makecol (0x80, 0xff, 0x80);
	LIME_GREEN  = makecol (0xc8, 0xff, 0xc8);
	ORANGE      = makecol (0xfa, 0x96, 0x00);
	PINK        = makecol (0xff, 0x00, 0xff);
	PURPLE      = makecol (0xc8, 0x00, 0xc8);
	RED         = makecol (0xff, 0x00, 0x00);
	SILVER      = makecol (0xc0, 0xc0, 0xc0);
	TURQUOISE   = makecol (0x96, 0xc8, 0xff);
	WHITE       = makecol (0xff, 0xff, 0xff);
	YELLOW      = makecol (0xff, 0xff, 0x00);

	// Start preparing environment
	env.first_init();    // *MUST* be done before GLOBALDATA or
	global.first_init(); // max_screen_updates is not correct!

	// Prepare remaining environment
	clear_to_color (global.canvas, BLACK);
	env.loadBitmaps();
	title();
	env.loadSounds();

	init_mouse_cursor();

	env.loadFonts();

	env.window.x = 0;
	env.window.y = 0;
	env.window.w = 0;
	env.window.h = 0;

	for (int32_t z = 0; z < env.max_screen_updates; z++) {
		global.updates[z].x = 0;
		global.updates[z].y = 0;
		global.updates[z].w = 0;
		global.updates[z].h = 0;
	}

}


static void initialisePlayers()
{
	for (int32_t z = 0; z < env.numGamePlayers; ++z) {
		env.players[z]->money = env.startmoney;
		env.players[z]->score = 0;
		if ( (HUMAN_PLAYER != env.players[z]->type)
		  && (PERPLAY_PREF == env.players[z]->preftype))
			env.players[z]->generatePreferences();
		env.players[z]->initialise(false);
		env.players[z]->type_saved = env.players[z]->type;
	}
}


static bool loadConfig()
{
	bool result = false;

	if (load_config_file) {
		snprintf(fullPath, PATH_MAX, "%s/atanks-config.txt", env.configDir);

		FILE* old_config_file = fopen(fullPath, "r");

		if (old_config_file) {
			env.load_from_file(old_config_file);

			// over-ride full screen setting with command line
			if ( (full_screen == FULL_SCREEN_TRUE)
			  || (full_screen == FULL_SCREEN_FALSE) )
				env.full_screen = full_screen;

			// Initialize after loading
			init_game_settings();

			// Load texts first ...
			env.load_text_files();

			// ...then the players last
			result = loadPlayers(old_config_file);

			fclose(old_config_file);
		}
	} // End of loading old config file

	return result;
}


static bool loadPlayers(FILE* file)
{
	int32_t max_pl = env.numPermanentPlayers;

	if (env.allPlayers) {
		for (int32_t i = 0; i < env.numPermanentPlayers; ++i) {
			if (env.allPlayers[i])
				delete env.allPlayers[i];
			env.allPlayers[i]  = nullptr;
		}
		free(env.allPlayers);
		env.allPlayers = nullptr;
	}

	env.allPlayers = (PLAYER **)malloc(sizeof(PLAYER*) * max_pl);

	if (!env.allPlayers) {
		fprintf(stderr, "%s:%d : Failed to allocate memory for allPlayers\n",
				__FILE__, __LINE__);
		return false;
	}

	for (int32_t i = 0; i < max_pl; ++i)
		env.allPlayers[i] = nullptr;

	int32_t pl_count = 0;
	bool    status   = true;

	while (status) {
		PLAYER* player_new = nullptr;
		try {
			player_new = new PLAYER();
		} catch (std::exception &e) {
			fprintf(stderr, "%s:%d : Failed to allocate memory for player (%s)\n",
					__FILE__, __LINE__, e.what());
			status = false;
		}

		if (status)
			status = player_new->load_from_file(file);

		if (status) {
			player_new->index = pl_count;
			env.allPlayers[pl_count++] = player_new;
			if (pl_count == max_pl) {
				max_pl += 5;
				env.allPlayers = (PLAYER**)realloc(env.allPlayers,
				                                   sizeof(PLAYER *) * max_pl);
				for (int32_t i = pl_count; i < max_pl; ++i)
					env.allPlayers[i] = nullptr;
			}
		} else if (player_new)
			delete player_new;
	} // end of while status

	env.numPermanentPlayers = pl_count;

	return true;
}


static int32_t menu()
{
	int32_t result     = SIG_OK;
	int32_t shift_menu = env.halfHeight < 240 ? 240 - env.halfHeight : 0;
	int32_t move_btn   = env.button[0]->w / 2;
	int32_t bn         = env.language == EL_RUSSIAN ? MENUBUTTONS * 2 : 0;

	BUTTON but_play(env.halfWidth - move_btn,
	                env.halfHeight - 235 + shift_menu,
	                env.button[bn], env.button[bn], env.button[bn + 1]);
	bn += 2;
	BUTTON but_help(env.halfWidth - move_btn,
	                env.halfHeight - 185 + shift_menu,
	                env.button[bn], env.button[bn], env.button[bn + 1]);
	bn += 2;
	BUTTON but_options(env.halfWidth - move_btn,
	                   env.halfHeight - 135 + shift_menu,
	                   env.button[bn], env.button[bn], env.button[bn + 1]);
	bn += 2;
	BUTTON but_players(env.halfWidth - move_btn,
	                   env.halfHeight - 85 + shift_menu,
	                   env.button[bn], env.button[bn], env.button[bn + 1]);
	bn += 2;
	BUTTON but_credits(env.halfWidth - move_btn,
	                   env.halfHeight - 35 + shift_menu,
	                   env.button[bn], env.button[bn], env.button[bn + 1]);
	bn += 2;
	BUTTON but_quit(env.halfWidth - move_btn,
	                env.halfHeight + 65 + shift_menu,
	                env.button[bn], env.button[bn], env.button[bn + 1]);
	bn += 2;
	BUTTON but_network(env.halfWidth - move_btn,
	                   env.halfHeight + 15 + shift_menu,
	                   env.button[bn], env.button[bn], env.button[bn + 1]);

	BUTTON *button[MENUBUTTONS] = { &but_play,    &but_help,    &but_options,
	                                &but_players, &but_credits, &but_network,
	                                &but_quit };

	// Initialization of the menu
	global.stopwindow = true;
	fi = 1;
	lx = 0;
	ly = 0;
	k  = 0;
	K  = 0;

	bool    done         = false;
	int32_t seconds_idle = 0;
	int32_t btn_over     = -1;
	int32_t currentindex = 0;
	int32_t oldindex     = 0;
	int32_t maxindex     = MENUBUTTONS;
	int32_t lastmouse_x  = 0;
	int32_t lastmouse_y  = 0;

	// Clear key buffer and erase mouse button presses
	while ( keypressed() )
		readkey();
	mouse_b = 0;

	// Enable first background drawing:
	bool need_draw = true;
	draw_simple_bg(true);
	global.make_fullUpdate();

	while ( !done && (SIG_OK == result) ) {

		// Extra loop to divide the handling and the drawing
		while (!done && !need_draw) {
			// Count seconds for demo mode to start after its wait time
			if ( global.check_time_changed() ) {
				if (++seconds_idle > DEMO_WAIT_TIME) {
					done = true;
					global.set_command(GLOBAL_COMMAND_DEMO);
				}
			}

			// Detect mouse movement for custom cursors
			if (!env.osMouse
			  && ( (lastmouse_x != mouse_x)
			    || (lastmouse_y != mouse_y) ) ) {
				lastmouse_x = mouse_x;
				lastmouse_y = mouse_y;
				need_draw   = true;
			}

			// See where the mouse is
			for (int32_t z = 0; z < MENUBUTTONS; z++) {
				if (button[z]->isMouseOver()) {
					if ( (btn_over > -1) && (btn_over != z) ) {
						button[z]->draw();
						need_draw = true;
					}

					btn_over = z;
					break;
				}
			}

			// Handle mouse click
			if (mouse_b & 1) {
				for (int32_t z = 0; z < MENUBUTTONS; z++) {
					if (button[z]->isPressed ()) {
						need_draw = true;
						done      = true;
						if (z == 0)
							global.set_command(GLOBAL_COMMAND_PLAY);
						else if (z == 1)
							global.set_command(GLOBAL_COMMAND_HELP);
						else if (z == 2)
							global.set_command(GLOBAL_COMMAND_OPTIONS);
						else if (z == 3)
							global.set_command(GLOBAL_COMMAND_PLAYERS);
						else if (z == 4)
							global.set_command(GLOBAL_COMMAND_CREDITS);
						else if (z == 5)
							global.set_command(GLOBAL_COMMAND_NETWORK);
						else if (z == 6) {
							global.set_command(GLOBAL_COMMAND_QUIT);
							result = SIG_QUIT_GAME;
						}
					}
				}
			} // End of mouse button pressed

			// check for key press
			if ( keypressed() ) {
				k = readkey();
				K = k >> 8;
				fi = 2;
			}

			// Move selection down
			if ( ( K == KEY_DOWN ) || (K == KEY_S) ) {
				if (++currentindex >= maxindex)
					currentindex = 0;
				need_draw = true;
			}

			// Move selection up
			else if ( (K == KEY_UP) || (K == KEY_W) ) {
				if (--currentindex < 0)
					currentindex = maxindex - 1;
				need_draw = true;
			}

			// Activate selection
			else if ( (KEY_ENTER     == K)
				   || (KEY_ENTER_PAD == K)
				   || (KEY_SPACE     == K) ) {
				need_draw = true;
				done      = true;
				if (currentindex == 0)
					global.set_command(GLOBAL_COMMAND_PLAY);
				else if (currentindex == 1)
					global.set_command(GLOBAL_COMMAND_HELP);
				else if (currentindex == 2)
					global.set_command(GLOBAL_COMMAND_OPTIONS);
				else if (currentindex == 3)
					global.set_command(GLOBAL_COMMAND_PLAYERS);
				else if (currentindex == 4)
					global.set_command(GLOBAL_COMMAND_CREDITS);
				else if (currentindex == 5)
					global.set_command(GLOBAL_COMMAND_NETWORK);
				else if (currentindex == 6)
					global.set_command(GLOBAL_COMMAND_QUIT);
			}

			// Quick keys to exit and handle close button of the window
			else if ( (KEY_Q      == K)
				   || (KEY_ESC == K)) {
				done   = true;
				result = SIG_QUIT_GAME;
			}

			// Erase key presses
			K = 0;

			// Print out update info if any
			if ( (global.update_string) && (global.update_string[0]) ) {
				textout_centre_ex (global.canvas, font, global.update_string,
								   env.halfWidth    - 20, env.screenHeight - 50,
								   WHITE, -1);
				global.make_update(50, 450, 300, 50);
				need_draw = true;
			}

			// Print out client messages
			if (global.client_message) {
				textout_centre_ex(global.canvas, font, global.client_message,
								  env.halfWidth    - 20, env.screenHeight - 25,
								  WHITE, -1);
				global.make_update(50, 450, 300, 100);
				need_draw = true;
			}

			// Sleep a bit if nothing happened
			if (!done && !need_draw)
				LINUX_SLEEP
		} // End of while not needing to draw


		// flip to front if needed
		if (need_draw) {

			// Draw the buttons
			SHOW_MOUSE(nullptr)
			draw_simple_bg(true);

			for (int32_t z = 0; z < MENUBUTTONS; z++) {
				button[z]->draw();

				// draw a rectangle around the selected button
				if ( (z == currentindex) || (z == oldindex) ) {
					int32_t left   = env.halfWidth - move_btn - 6;
					int32_t top    = env.halfHeight - 241 + (50 * currentindex)
					               + shift_menu;
					int32_t right  = env.halfWidth + move_btn + 5;
					int32_t bottom = env.halfHeight - 192 + (50 * currentindex)
					               + shift_menu;
					global.make_update(left, top, right - left, bottom - top);
					if (z == currentindex)
						rect(global.canvas, left, top, right, bottom, YELLOW);
				}
			} // end of looping buttons

			// Show non-OS mouse
			SHOW_MOUSE(global.canvas)

			global.do_updates();
			need_draw = false;
			oldindex  = currentindex;
		} // End of if need_draw
	} // End of menu loop

	clear_keybuf ();

	return result;
}


static void newgame()
{
	env.initialise ();
	global.initialise ();

	// if a game should be loaded, try it or deny loading of the game
	if ( (env.loadGame) && (!Load_Game()) )
		env.loadGame = false;

	// Now check back whether to load a game
	if (!env.loadGame)
		initialisePlayers ();

	// There must not be any tanks!
	TANK* tank      = nullptr;
	TANK* next_tank = nullptr;
	global.getHeadOfClass(CLASS_TANK, &tank);
	while (tank) {
		tank->getNext(&next_tank);
		tank->player = nullptr;
		delete tank;
		tank         = next_tank;
	}

	// This is always true here, as a newly started game is handled like a loaded one:
	env.isGameLoaded = true;
}


/// @brief parse arguments and return EXIT_SUCCESS on success or EXIT_FAILURE
/// if anything went wrong. If all is well but help was requested, return
/// EXIT_HELP_SHOWN
static int32_t parse_args(int32_t argc, char** argv)
{
	for (int32_t c = 1; c < argc; ++c)  {
		bool        has_value = true;
		std::string arg(argv[c]);

		if ( (arg == SWITCH_HELP) || (arg == "--help") ) {
			print_text_help();
			return HELP_REQUESTED;
		} else if (arg == SWITCH_FULL_SCREEN) {
			screen_mode = GFX_AUTODETECT_FULLSCREEN;
			full_screen = FULL_SCREEN_TRUE;
		} else if (arg == SWITCH_WINDOWED) {
			screen_mode = GFX_AUTODETECT_WINDOWED;
			full_screen = FULL_SCREEN_FALSE;
		} else if ( (arg == "-d") || (arg == "--depth") ) {
			if ( (c < (argc - 1)) && (argv[c + 1][0] != '-') ) {
				std::string next_arg(argv[++c]);
				int32_t     val = strtol (next_arg.c_str(), nullptr, 10);

				if ( (16 == val) || (32 == val) )
					env.colourDepth = val;
				else {
					cerr << "ERROR: Invalid graphics depth!\n"
					     << "       Only 16 or 32 bit are supported!" << endl;
					return EXIT_FAILURE;
				}
			} else
				has_value = false;
		} else if ( (arg == "-w") || (arg == "--width") ) {
			if ( (c < (argc - 1)) && (argv[c + 1][0] != '-') ) {
				std::string next_arg(argv[++c]);
				int32_t     val = strtol (next_arg.c_str(), nullptr, 10);

				if ( 512 <= val ) {
					env.screenWidth      = val;
					env.halfWidth        = env.screenWidth / 2;
					env.temp_screenWidth = env.screenWidth;
				} else {
					cerr << "ERROR: Width too small (minimum 512)\n" << endl;
					return EXIT_FAILURE;
				}
			} else
				has_value = false;
		} else if ((arg == "-t") || (arg == "--tall") || (arg == "--height")) {
			if ( (c < (argc - 1)) && (argv[c + 1][0] != '-') ) {
				std::string next_arg(argv[++c]);
				int32_t     val = strtol (next_arg.c_str(), nullptr, 10);

				if ( 320 <= val ) {
					env.screenHeight      = val;
					env.halfHeight        = env.screenHeight / 2;
					env.temp_screenHeight = env.screenHeight;
				} else {
					cerr << "ERROR: Height too small (minimum 320)\n" << endl;
					return EXIT_FAILURE;
				}
			} else
				has_value = false;
		} else if (arg == "--datadir") {
			if ( (c < (argc - 1)) && (argv[c + 1][0] != '-') ) {
				std::string next_arg(argv[++c]);

				if ( next_arg.length() <= PATH_MAX )
					strncpy(env.dataDir, next_arg.c_str(), PATH_MAX);
				else {
					cerr << "ERROR: Datadir path too long:\n"
					     << "\"" << next_arg << "\"\n\n"
					     << "Maximum length:" << PATH_MAX << " characters"
					     << endl;
					return EXIT_FAILURE;
				}
			} else
				has_value = false;
		} else if (arg == "-c") {
			if ( (c < (argc - 1)) && (argv[c + 1][0] != '-') ) {
				std::string next_arg(argv[++c]);

				if ( next_arg.length() <= PATH_MAX )
					strncpy(env.configDir, next_arg.c_str(), PATH_MAX);
				else {
					cerr << "ERROR: Configuration path too long:\n"
					     << "\"" << next_arg << "\"\n\n"
					     << "Maximum length:" << PATH_MAX << " characters"
					     << endl;
					return EXIT_FAILURE;
				}
			} else
				has_value = false;
		} else if (arg == "--noconfig")
			load_config_file = false;
		else if (arg == "--nosound")
			env.sound_enabled = false;
		else if (arg == "--noname")
			env.nameAboveTank = false;
		else if (arg == "--nonetwork")
			allow_network = false;
		else if (arg == "--nobackground")
			env.drawBackground = false;
		else if (arg == "--nothread")
			cout << "--nothread is deprecated and will be ignored." << endl;
		else if (arg == "--thread")
			cout << "--thread is deprecated and will be ignored." << endl;

		// If a required argument is missing, print out a message
		if (!has_value) {
			cerr << "ERROR: Missing argument for " << arg << endl;
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}


static void play_demo()
{
	int32_t old_skip   = env.skipComputerPlay;
	int32_t old_rounds = env.rounds;
	bool    old_music  = env.play_music;

	global.demo_mode = true;
	env.loadGame     = false;
	env.play_music   = false;

	env.rounds          = (rand() % 101) + (rand() % 101) + 50;
	global.currentround = env.rounds - (rand() % env.rounds);

	// Be sure to have at least 10 rounds left
	if (global.currentround < 10)
		global.currentround = 10;

	// And at least 10 rounds must have been played, or it'll be a bit boring
	if (global.currentround > (env.rounds - 10))
		global.currentround =  env.rounds - 10;

	env.skipComputerPlay = SKIP_NONE;

	// set up a bunch of players (non-human, less than 10)
	int32_t playerCount = 0;
	env.numGamePlayers  = 0;
	for (int32_t i = 0; i < env.numPermanentPlayers; ++i) {
		if ( (env.allPlayers[i]->type > HUMAN_PLAYER)
		  && (i < MAXPLAYERS) ) {
			env.addGamePlayer(env.allPlayers[i]);
			playerCount++;
		}
	}

	newgame();

	for (int32_t i = 0; i < env.numGamePlayers; ++i) {
		env.players[i]->newGame();

		// give them money to spend:
		env.players[i]->money += static_cast<int32_t>(env.players[i]->type)
							   * 25000 * (env.rounds - global.currentround);
	}

	while ( (global.currentround > 0) && (!global.isCloseBtnPressed()) ) {
		game();
		if ( (global.get_command() == GLOBAL_COMMAND_QUIT)
		  || (global.get_command() == GLOBAL_COMMAND_MENU) )
			break;
	}

	endgame_cleanup();
	global.demo_mode     = false;
	env.skipComputerPlay = old_skip;
	env.play_music       = old_music;
	env.rounds           = old_rounds;
}


static void play_local()
{
	if (selectPlayers() != MRC_Esc_Menu) {

		// make sure the game has a name
		if (!env.game_name[0])
			strncpy(env.game_name, env.ingame->Get_Line(53), GAMENAMELEN);

		newgame ();

		if (!env.loadGame) {
			global.currentround = env.rounds;
			for (int32_t i = 0; i < env.numGamePlayers; ++i)
				env.players[i]->newGame();
		}

		// play the game for the selected number of rounds
		while ( (global.currentround > 0) && (!global.isCloseBtnPressed()) ) {
			game (); // play a round

			if (env.background_music) {
				stop_sample(env.background_music);
			}

			if (global.isCloseBtnPressed())
				global.set_command(GLOBAL_COMMAND_QUIT);

			// if user selected to quit or return to main menu during game play
			if ( (global.get_command() == GLOBAL_COMMAND_QUIT)
			  || (global.get_command() == GLOBAL_COMMAND_MENU) ) {
				env.sendToClients("CLOSE");
				break;
			}

			if (global.currentround != 0)
				// end of the round
				env.sendToClients("ROUNDEND");
		}

		// only show winner if finished all rounds and not broken off the last
		// round by exiting or quitting
		if ( (global.currentround == 0)
		  && (global.get_command() == GLOBAL_COMMAND_PLAY) ) {
			char        buffer[256] = { 0 };
			const char* winner      = do_winner();

			if (winner)
				snprintf(buffer, 255, "GAMEEND The game went to %s.", winner);
			else
				strncpy(buffer, "GAMEEND", 255);

			env.sendToClients(buffer);

			// Do fade and wait for user keypress
			quickChange(true);
			readkey ();

			for (int i = 0; i < env.numGamePlayers; i++)
				env.players[i]->type = env.players[i]->type_saved;
		}
		endgame_cleanup ();
	} // end of start new game
}


static void play_networked()
{
#ifdef NETWORK
	client_socket = Setup_Client_Socket(env.server_name, env.server_port);
	if (client_socket >= 0) {
		bool keep_playing = true;
		cout << "Ready to play networked" << endl;

		while (keep_playing)
			keep_playing = Game_Client(client_socket);

		Clean_Up_Client_Socket(client_socket);
	} else
		cerr << "ERROR: Unable to connect to server " << env.server_name
		     << ", port " << env.server_port << endl;
#else
		char noNetworkMsg[200] = { 0 };
		snprintf(noNetworkMsg, 199,
		         "This version of Atanks is not compiled to"
		         " handle network games.");
		errorMessage = noNetworkMsg;
		errorX       = env.halfWidth - text_length(font, errorMessage) / 2;
		errorY       = env.menuBeginY + 15;
		cerr << "ERROR: " << noNetworkMsg << endl;
#endif

}


static void print_text_help()
{
	cout << "-h  --help           Show this help screen\n"
	     << "-fs                  Full screen\n"
	     << "    --windowed       Run in a window\n"
	     << "-w  --width <width>  Specify the screen width in pixels\n"
	     << "-t  --tall  <height> Specify the screen height in pixels\n"
	     << "                     Adjust the screen size at your own risk\n"
	     << "                     (default is 800x600)\n"
	     << "-d  --depth <depth>  Colour depths, currently either 16 or 32 bit\n"
	     << "    --datadir <path> Path to the data directory\n"
	     << "-c <path>            Path to config and saved game directory\n"
	     << "    --noconfig       Do not load game settings.\n"
	     << "    --nosound        Disable sound\n"
	     << "    --noname         Do not show player name above tank\n"
	     << "    --nonetwork      Disable network connections.\n"
	     << "    --nobackground   Do not display the green menu background."
	     << endl;
}


static void print_text_initmsg()
{
	cout << "Atomic Tanks Version " << VERSION << " (-h for help)\n"
	     << "Authors: Tom Hudson        (rewrite, additions, improvements)\n"
	     << "         Stevante Software (original design)\n"
	     << "         Kota543 Software  (fixes and updates)\n"
	     << "         Jesse Smith       (additions, fixes and updates)\n"
		 << "         Sven Eden         (ai rewrite, additions, fixes and updates)\n\n"
		 << endl;
}


/*
This function calls the functions which save data to a text file.
The function requires the global data, environment and the path to
the config file name.
The function returns TRUE on success and FALSE on failure.
-- Jesse
*/
static bool Save_Game_Settings(const char* path)
{
	FILE* file = fopen(path, "w");
	if (!file) {
		perror ( "Error trying to open text file for writing.\n");
		return false;
	}

	env.save_to_file (file);

	for (int32_t i = 0; i < env.numPermanentPlayers; ++i)
		env.allPlayers[i]->save_to_file(file);

	fclose (file);
	return true;
}


static void show_options()
{
	// save old settings
	bool    temp_sound = env.sound_enabled;
	int32_t temp_itech = env.itemtechLevel;
	int32_t temp_wtech = env.weapontechLevel;

	optionsMenu();

	if (!Save_Game_Settings(fullPath))
		cerr << "atanks.cpp:" << __LINE__
		     << " Failed to save game settings from " << __FUNCTION__ << endl;

	// check for changes to settings
	Change_Settings(temp_sound, temp_itech, temp_wtech);
}


static void title()
{
	SHOW_MOUSE(nullptr)
	blit (env.title[0], screen, 0, 0,
	      env.halfWidth  - (env.title[0]->w / 2),
	      env.halfHeight - (env.title[0]->h / 2),
	      env.title[0]->w, env.title[0]->h);
	clear_keybuf ();
}


int32_t main (int32_t argc, char** argv)
{
	print_text_initmsg();

	// Parse arguments and exit early if needed
	int32_t result = parse_args(argc, argv);

	if (EXIT_FAILURE == result)
		return EXIT_FAILURE;
	if (HELP_REQUESTED == result)
		return EXIT_SUCCESS;

	// try to find data dir
	if (! env.find_data_dir() ) {
		cerr << "ERROR: Could not find data dir." << endl;
		return EXIT_FAILURE;
	}

	// Initialize random number generation
	srand (time(nullptr));

	// Set the game version global
#ifdef VERSION
	{
		double this_version = 0.;
		sscanf(VERSION, "%lf", &this_version);
		game_version = static_cast<int32_t>(this_version * 10);
	}
#endif // VERSION

	// try to find config dir
	env.find_config_dir();

	// load or create a configuration
	if (!loadConfig())
		createConfig();

	// Load game files
	if (!env.loadGameFiles())
		return EXIT_FAILURE; // message already out

	// new networking area
#ifdef NETWORK
	SEND_RECEIVE_TYPE* send_receive   = nullptr;
	std::thread*       network_thread = nullptr;

	// Create the update checker thread:
	update_data updateData("projects.sourceforge.net", "version.txt",
	                       "atanks.sourceforge.net");

	std::thread updateThread(std::ref(updateData));
	if (env.check_for_updates)
		global.update_string = updateData.update_string;

	// Initialize network if allowed and wanted
	if ( env.network_enabled && allow_network ) {
		send_receive = (SEND_RECEIVE_TYPE*)calloc(1, sizeof(SEND_RECEIVE_TYPE));
		if (!send_receive)
			cerr << "ERROR: Could not create networking data." << endl;
	}

	// If a SEND_RECEIVE_TYPE instance was created, start the networking thread
	if (send_receive) {
		send_receive->listening_port = env.network_port;

		// quit option already cleared by calloc call
		network_thread = new std::thread(Send_And_Receive, send_receive);
	}
#endif // NETWORK

	/* ===============================
	 * === The real main main loop ===
	 * ===============================
	 */
	do {

		//show the main menu
		global.set_command(GLOBAL_COMMAND_MENU);
		int32_t signal = menu ();

		// Ensure a clean client message
		if (global.client_message) {
			free(const_cast<char*>(global.client_message));
			global.client_message = nullptr;
		}

		// did the user signal to quit the game?
		if ( (signal == SIG_QUIT_GAME)
		  || global.isCloseBtnPressed() )
			global.set_command(GLOBAL_COMMAND_QUIT);

		// determine which menu item is selected
		switch (global.get_command()) {
			case GLOBAL_COMMAND_HELP:
				scrollTextList (env.instructions);
				break;
			case GLOBAL_COMMAND_OPTIONS:
				show_options();
			break;
			case GLOBAL_COMMAND_PLAYERS:
				editPlayers();
				break;
			case GLOBAL_COMMAND_CREDITS:
				credits();
				break;
			case GLOBAL_COMMAND_QUIT:
				// Already handled by while condition below
				break;
			case GLOBAL_COMMAND_NETWORK:
				play_networked();
				break;
			case GLOBAL_COMMAND_DEMO:
				play_demo();
				break;
			default:
				//must have commanded to play game
				play_local();
			break;
		} // end of menu switch
	} while ( (global.get_command() != GLOBAL_COMMAND_QUIT) );

	// print out if there is an update
	if ( (global.update_string) && (global.update_string[0]) ) {
		cout << global.update_string << endl;
		global.update_string = nullptr;
	}

	// Clean up network stuff
#ifdef NETWORK
	if (send_receive) {
		send_receive->shut_down = TRUE;
		LINUX_REST;
		network_thread->join();
		delete network_thread;
		free(send_receive);
	}
	updateThread.join();
#endif // NETWORK

	if (! Save_Game_Settings(fullPath)) {
		// This is a very critical issue, but as we are ending here, we just report it
		cerr << "atanks.cpp: Failed to save game settings from atanks::main()!" << endl;
		result = EXIT_FAILURE;
    }

	env.destroy();
	global.destroy();

	allegro_exit();

	cout << "See http://atanks.sourceforge.net for the latest news and downloads." << endl;

	return result;
}
END_OF_MAIN()
