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
#include "environment.h"
#include "globaldata.h"
#include "missile.h"
#include "tank.h"
#include "files.h"
#include "sound.h"
#include "player.h"

#include <cassert>

ENVIRONMENT::ENVIRONMENT ()
{
	// Unfortunately Visual C++ can not initialize arrays using an initialization list
	// although it is part of C++11. Gcc and clang do it fine btw...
	memset(availableItems, 0, sizeof(int32_t) * THINGS);
	memset(configDir,      0, sizeof(char)    * (PATH_MAX + 1));
	memset(dataDir,        0, sizeof(char)    * (PATH_MAX + 1));
	memset(game_name,      0, sizeof(char)    * GAMENAMELEN);
	memset(playerOrder,    0, sizeof(PLAYER*) * MAXPLAYERS);
	memset(server_name,    0, sizeof(char)    * 129);
	memset(server_port,    0, sizeof(char)    * 129);
	memset(slope,          0, sizeof(double)  * 720);



	set_fps(60); // rock solid default.

	fontHeight = 10; // Initial value

	strncpy(server_name, "127.0.0.1", 127);
	strncpy(server_port, "25645", 127);

	// Reserve space for the players array:
	// Note: The allPlayers array is dynamically (re-)allocated while loading
	//       stored players from the configuration.
	if ( (players = (PLAYER **) calloc( MAXPLAYERS, sizeof(PLAYER *) ) ) == nullptr)
		perror ( "environment.cpp: Failed allocating memory for players");

	// sin/cos short-cuts, With only 1° granularity the arrays are always
	// faster than live calculations.
	for (int32_t i = 0; i < 360; i++) {
		slope[i][0] = std::sin(DEG2RAD(i));
		slope[i][1] = std::cos(DEG2RAD(i));
	}
}


/** @brief default dtor
 * Cleanly remove created objects
 **/
ENVIRONMENT::~ENVIRONMENT()
{
	this->destroy();
}


/// @brief add a player to the players[] array that will take part in the next game
void ENVIRONMENT::addGamePlayer(PLAYER* player_)
{
	if (player_ && (numGamePlayers < MAXPLAYERS) ) {

		// Ensure the player isn't already there:
		for (int32_t i = 0; i < numGamePlayers; ++i) {
			if (player_ == players[i])
				return;
		}

		players[numGamePlayers++] = player_;

		if (HUMAN_PLAYER == player_->type)
			numHumanPlayers++;
	}
}


/// @brief create a new player or return nullptr if an error occurred
PLAYER* ENVIRONMENT::createNewPlayer (const char* player_name)
{
	PLAYER** reallocatedPlayers = nullptr;
	PLAYER*  player             = nullptr;

	assert (player_name && "ERROR: player_name is nullptr!");

	if (nullptr == player_name)
		return nullptr;

	if (getPlayerByName(player_name) > -1)
		return nullptr;

	reallocatedPlayers = (PLAYER**)realloc (allPlayers,
	                              sizeof (PLAYER*) * (numPermanentPlayers + 1));

	if (reallocatedPlayers)
		allPlayers = reallocatedPlayers;
	else
		perror ("environment.cpp: Failed allocating memory for reallocatedPlayers in ENVIRONMENT::createNewPlayer");

	player = new PLAYER ();
	if (!player)
		perror ("environment.cpp: Failed allocating memory for player in ENVIRONMENT::createNewPlayer");

	player->index = numPermanentPlayers;
	player->setName(player_name);
	allPlayers[numPermanentPlayers++] = player;

	return player;
}


/// @brief This function gives credits, score and money to the winner(s).
void ENVIRONMENT::creditWinners(int32_t winner)
{
	if (winner == WINNER_DRAW)    // no winner
		return;

	int32_t team_members = 0;

	if (winner == WINNER_JEDI) {
		for (int32_t i = 0; i < numGamePlayers; ++i) {
			if (TEAM_JEDI == players[i]->team) {
				players[i]->score++;
				players[i]->won++;
				team_members++;
			}
		}
	} else if (winner == WINNER_SITH) {
		for (int32_t i = 0; i < numGamePlayers; ++i) {
			if (TEAM_SITH == players[i]->team) {
				players[i]->score++;
				players[i]->won++;
				team_members++;
			}
		}
	} else if (winner < WINNER_NO_WIN) {
		players[winner]->score++;
		players[winner]->won++;
		players[winner]->money += scoreRoundWinBonus;
	}

	// team gets their money too
	if (team_members) {
		int32_t team_bonus = scoreRoundWinBonus / team_members;
		for (int32_t i = 0; i < numGamePlayers; ++i) {
			if ( ((winner == WINNER_JEDI) && (players[i]->team == TEAM_JEDI) )
			  || ((winner == WINNER_SITH) && (players[i]->team == TEAM_SITH) ) )
				players[i]->money += team_bonus;
		}
	}
}


void ENVIRONMENT::decreaseVolume()
{
	if (volume_factor > 0)
		--volume_factor;
}


/// @brief Remove one of the players, then gone for good.
void ENVIRONMENT::deletePermPlayer (PLAYER* player_)
{
	int32_t toCount = 0;

	for (int32_t fromCount = 0; fromCount < numPermanentPlayers; fromCount++) {
		if (allPlayers[fromCount] != player_) {
			if (allPlayers[toCount] != allPlayers[fromCount]) {
				allPlayers[toCount]  = allPlayers[fromCount];
				allPlayers[toCount]->index = toCount;
			}
			toCount++;
		}
	}
	numPermanentPlayers--;

	delete player_;
}


/** @brief Free all allocated memory.
  *
  * Important: This MUST be called *before* allegro shuts down!
**/
void ENVIRONMENT::destroy()
{
	if (sky) {
		destroy_bitmap(sky);
		sky = nullptr;
	}

	if (bitmap_filenames) {
		for (int32_t count = 0; count < number_of_bitmaps; ++count) {
			if (bitmap_filenames[count])
				free(bitmap_filenames[count]);
		}
		free(bitmap_filenames);
		bitmap_filenames = nullptr;
	}

	if (saved_game_list_size && saved_game_list) {
		for (uint32_t i = 0; i < saved_game_list_size; ++i) {
			if (saved_game_list[i])
				free (const_cast<char*>(saved_game_list[i]));
			saved_game_list[i] = nullptr;
		}
		free (saved_game_list);
		saved_game_list = nullptr;
		saved_game_list_size = 0;
	}

	if (music_dir) {
		closedir(music_dir);
		music_dir = nullptr;
	}

	if (background_music) {
		destroy_sample(background_music);
		background_music = nullptr;
	}

	if (sounds) {
		int32_t index = 0;
		while (sounds[index])
			destroy_sample(sounds[index++]);
		free(sounds);
		sounds = nullptr;
	}

	if (title) {
		int32_t index = 0;
		while (title[index])
			destroy_bitmap(title[index++]);
		free(title);
		title = nullptr;
	}

	if (button) {
		int32_t index = 0;
		while (button[index])
			destroy_bitmap(button[index++]);
		free(button);
		button = nullptr;
	}

	if (misc) {
		int32_t index = 0;
		while (misc[index])
			destroy_bitmap(misc[index++]);
		free(misc);
		misc = nullptr;
	}

	if (missile) {
		int32_t index = 0;
		while (missile[index])
			destroy_bitmap(missile[index++]);
		free(missile);
		missile = nullptr;
	}

	if (stock) {
		int32_t index = 0;
		while (stock[index])
			destroy_bitmap(stock[index++]);
		free(stock);
		stock = nullptr;
	}

	if (tank) {
		int32_t index = 0;
		while (tank[index])
			destroy_bitmap(tank[index++]);
		free(tank);
		tank = nullptr;
	}

	if (tankgun) {
		int32_t index = 0;
		while (tankgun[index])
			destroy_bitmap(tankgun[index++]);
		free(tankgun);
		tankgun = nullptr;
	}

	if (gloat)        { delete    gloat;        gloat        = nullptr; }
	if (ingame)       { delete    ingame;       ingame       = nullptr; }
	if (instructions) { delete    instructions; instructions = nullptr; }
	if (panic)        { delete    panic;        panic        = nullptr; }
	if (kamikaze)     { delete    kamikaze;     kamikaze     = nullptr; }
	if (retaliation)  { delete    retaliation;  retaliation  = nullptr; }
	if (revenge)      { delete    revenge;      revenge      = nullptr; }
	if (suicide)      { delete    suicide;      suicide      = nullptr; }
	if (war_quotes)   { delete    war_quotes;   war_quotes   = nullptr; }

	if (allPlayers) {
		for (int32_t i = 0; i < numPermanentPlayers; ++i) {
			if (allPlayers[i])
				delete allPlayers[i];
			allPlayers[i]  = nullptr;
		}
		free(allPlayers);
		allPlayers = nullptr;
	}

	if (players) {
		for (int32_t i = 0; i < MAXPLAYERS; ++i)
			players[i] = nullptr;
		free(players);
		players    = nullptr;
	}

	if (main_font) {
		destroy_font(main_font);
		main_font = nullptr;
	}

	gfxData.destroy();
}


/// @brief Sets configDir to the path to the config directory used by atanks
void ENVIRONMENT::find_config_dir()
{
	// If no config dir was given on the command line, try to find a valid one
	if (!configDir[0]) {
		// figure out file name
		char* homedir = getenv(HOME_DIR);
		snprintf(configDir, PATH_MAX, "%s/.atanks", homedir ? homedir : ".");

		// copy the file over, if we did not yet
		if (!Copy_Config_File()) {
			// If it did not work, look whether the directory already exists:
			DIR * pDestDir = opendir(env.configDir);
			if (!pDestDir)
				cerr << "ERROR: An error has occurred trying to set up"
				     << " Atomic Tanks folders." << endl;
			else {
				closedir(pDestDir);
				pDestDir = nullptr;
			}
		}
	} // end of no config file on the command line
}


/// @brief Sets dataDir to the path 'unicode.dat' can be found in.
bool ENVIRONMENT::find_data_dir()
{

	// If the datadir set by command line options, try that first
	if (dataDir[0]) {
		snprintf(path_buf, PATH_MAX, "%s/%s", DATA_DIR, "unicode.dat");
		if (!access(path_buf, R_OK))
			return true;
		else {
			cerr << "ERROR: The given datadir \"" << dataDir << "\""
			     << " is invalid!" << endl;
			memset(dataDir, 0, sizeof(char) * (PATH_MAX + 1));
		}
	}

	// Try the set directory from the build
	snprintf(path_buf, PATH_MAX, "%s/%s", DATA_DIR, "unicode.dat");
	if (!access(path_buf, R_OK))
		strncpy(dataDir, DATA_DIR, PATH_MAX);
	else {
        // This was not successful, try the current directory if not tried, yet.

        if (strncmp(DATA_DIR, ".", 1) && strncmp(DATA_DIR, "./", 2)) {
			strncpy(path_buf, "./unicode.dat", PATH_MAX);

			// Try again and reset if unsuccessful
			if (!access(path_buf, R_OK))
				strncpy(dataDir, ".", PATH_MAX);
        }
	}

	// If dataDir is set, now, this was a success.
	if (strlen(dataDir))
		return true;

	return false;
}


/// @brief Must be called before GLOBALDATA::first_init() is called!
void ENVIRONMENT::first_init()
{
	// Determine maximum number of updates before doing
	// a full update:
	max_screen_updates = ROUND(std::sqrt(ROUND(screenWidth / 8)
	                                   * ROUND(screenHeight / 8)));
	/* This is:
	 *  800 x  600 => sqrt(100 *  75) => sqrt( 7500) =  87
	 * 1280 x 1024 => sqrt(160 *  28) => sqrt(20480) = 143
	 * 1600 x  900 => sqrt(200 * 113) => sqrt(22600) = 150
	 * 1920 x 1080 => sqrt(240 * 135) => sqrt(32400) = 180
	 */

	// Get memory ...
	if (!sky)
		sky = create_bitmap (screenWidth, screenHeight - MENUHEIGHT);
	if (!sky) {
		cout << "Failed to create sky bitmap: " << allegro_error << endl;
		exit(1);
	}

	initialise ();

	menuBeginY = (screenHeight - 400) / 2;
	if (menuBeginY < 0)
		menuBeginY = 0;
	menuEndY = screenHeight - menuBeginY;

	gfxData.first_init();
}


/// @brief Fill availableItems array with everything buyable with current settings.
void ENVIRONMENT::genItemsList ()
{
	int32_t slot = 0;
	for (int32_t i = 0; i < THINGS; ++i) {
		if (isItemAvailable(i))
			availableItems[slot++] = i;
	}
	numAvailable = slot;
}


/// @brief return the index of the player with @a player_name or -1 if not found
int32_t ENVIRONMENT::getPlayerByName(const char* player_name)
{
	int32_t result = -1;

	assert (player_name && "ERROR: player_name is nullptr!");

	if (nullptr == player_name)
		return result;

	for (int32_t i = 0; (-1 == result) && (i < numPermanentPlayers); ++i) {
		if (!strcmp(player_name, allPlayers[i]->getName()))
			result = i;
	}

	return result;
}


void ENVIRONMENT::increaseVolume()
{
	if (volume_factor < MAX_VOLUME_FACTOR)
		++volume_factor;
}


int32_t ENVIRONMENT::ingamemenu ()
{
	int32_t     pressed   = -1;
	bool        need_draw = true;
	int32_t     button[INGAMEBUTTONS];
	bool        updatew[INGAMEBUTTONS];
	const char* buttext[INGAMEBUTTONS] = {
		ingame->Get_Line(69),
		ingame->Get_Line(70),
		ingame->Get_Line(71),
		ingame->Get_Line(72),
	};

	// Set/calculate button size and positions
	int32_t b_width  = 150;
	int32_t b_height = 20;
	int32_t b_space  = 5;
	int32_t b_half_w = b_width / 2;
	int32_t b_left   = halfWidth - b_half_w;
	int32_t b_right  = halfWidth + b_half_w - 1;

	int32_t d_width  = 200;
	int32_t d_height = ((INGAMEBUTTONS + 2) * b_height)
	                 + ((INGAMEBUTTONS + 1) * b_space);
	int32_t d_half_w = d_width / 2;
	int32_t d_half_h = d_height / 2;

	int32_t d_left   = halfWidth  - d_half_w;
	int32_t d_right  = halfWidth  + d_half_w - 1;
	int32_t d_top    = halfHeight - d_half_h;
	int32_t d_bottom = halfHeight + d_half_h - 1;

	// store last mouse coordinates for movement detection
	int32_t lastMouse_x     = 0;
	int32_t lastMouse_y     = 0;

	// Calculate button y values and set all button status to 0
	int32_t y = -d_half_h + b_height + b_space;

	for (int32_t i = 0; i < INGAMEBUTTONS; ++i) {
		updatew[i] = false;
		button[i]  = y;
		y += b_height + b_space;
	}

	SHOW_MOUSE(nullptr);
	k = 0;
	K = 0;

	global.make_update (d_left, d_top, d_width, d_height);
	rectfill (global.canvas, d_left, d_top, d_right, d_bottom, GREY);
	rect     (global.canvas, d_left, d_top, d_right, d_bottom, BLACK);

	while (-1 == pressed) {
		LINUX_REST;
		if (keypressed ()) {
			k = readkey ();
			K = k >> 8;
		}

		// look for keyboard exit
		if ( ( K == KEY_ESC)
		  || ( K == KEY_P) ) {
			pressed = -2;
			continue;
		}

		// Check mouse movement
		if ( !env.osMouse
		  && ( (lastMouse_x != mouse_x)
		    || (lastMouse_y != mouse_y) ) ) {
			lastMouse_x = mouse_x;
			lastMouse_y = mouse_y;
			need_draw = true;
		}

		if (mouse_b & 1) {
			bool is_hit = false;
			for (int32_t i = 0; !is_hit && (i < INGAMEBUTTONS); ++i) {
				if ( (mouse_x >= b_left)
				  && (mouse_x <  b_right)
				  && (mouse_y >= (button[i] + halfHeight))
				  && (mouse_y <  (button[i] + b_height + halfHeight)) ) {

					is_hit = true;

					if (pressed > -1)
						updatew[pressed] = true;
					pressed    = i;
					updatew[i] = true;
				}
			}

			if (!is_hit) {
				if (pressed > -1)
					updatew[pressed] = true;
				pressed = -1;
			}
		}

		// Update buttons
		for (int32_t i = 0; i < INGAMEBUTTONS; ++i) {
			if (updatew[i]) {
				updatew[i] = false;
				global.make_update (b_left, halfHeight + button[i],
				                         b_width, b_height);
			}
		}

		// Draw buttons
		if (need_draw) {
			SHOW_MOUSE(nullptr)

			for (int32_t i = 0; i < INGAMEBUTTONS; ++i) {
				draw_sprite (global.canvas, misc[(pressed == i) ? 8 : 7],
							 b_left, halfHeight + button[i]);
				textout_centre_ex (global.canvas, font, buttext[i], halfWidth,
								   halfHeight + button[i] + 1, WHITE, -1);
			}

			// Update non-OS mouse movements
			SHOW_MOUSE(global.canvas)

			global.do_updates ();
			need_draw = false;
		}
	} // end of menu loop

	return pressed;
}


void ENVIRONMENT::initialise ()
{
	campaign_rounds = static_cast<double>(rounds) / 5.;
	if (campaign_rounds < 1.)
		campaign_rounds = 1.;

	nextCampaignRound = static_cast<double>(rounds) - campaign_rounds;
}


/// @return true if the items tech level is not too high and if it is not a warhead.
bool ENVIRONMENT::isItemAvailable (int32_t itemNum)
{
	if (itemNum < WEAPONS) {
		if ( (weapon[itemNum].warhead)
		  || (weapon[itemNum].techLevel > weapontechLevel) )
			return false;
	} else if (item[itemNum - WEAPONS].techLevel > itemtechLevel)
		return false;
	return true;
}


/*
This function loads environment settings from a text
file. The function returns TRUE on success and FALSE if
any erors are encountered.
-- Jesse
*/
/// @todo : This should be changed to streams. It's C++ and we have formatted
/// text files, so formatted input/output should be far more efficient in
/// maintenance.
void ENVIRONMENT::load_from_file (FILE *file)
{
	char  line[MAX_CONFIG_LINE  + 1] = { 0 };
	char  field[MAX_CONFIG_LINE + 1] = { 0 };
	char  value[MAX_CONFIG_LINE + 1] = { 0 };
	char* result                     = nullptr;
	bool  done                       = false;
	bool  sound_bookmark = sound_enabled; // To disable by command line

	// read until we hit line "*ENV*" or "***" or EOF
	do {
		result = fgets(line, MAX_CONFIG_LINE, file);
		if (!result || !strncmp(line, "***", 3) )
			// eof or end of record
			return;
		else if (!strncmp(line, "*GLOBAL*", 8)) {
			// Old style config/save file
			rewind(file);
			global.load_from_file(file);
		}
    } while ( strncmp(line, "*ENV*", 5) );
	// read until we hit new record

	while ( (result) && (!done) ) {
		// read a line
		memset(line, 0, MAX_CONFIG_LINE);
		result = fgets(line, MAX_CONFIG_LINE, file);

		// if we hit end of the record, stop
		if (! strncmp(line, "***", 3) )
			done = true;

		if (result && !done) {

			// strip newline character
			int32_t line_length = strlen(line);
			while ( line[line_length - 1] == '\n') {
				line[line_length - 1] = '\0';
				line_length--;
			}

			// find equal sign
			int32_t equal_position = 1;
			while ( ( equal_position < line_length )
				 && ( line[equal_position] != '='  ) )
				equal_position++;

			// make sure the equal sign position is valid
			if (line[equal_position] != '=')
				continue; // Go to next line

			// seperate field from value
			memset(field, '\0', MAX_CONFIG_LINE);
			memset(value, '\0', MAX_CONFIG_LINE);
			strncpy(field, line, equal_position);
			strncpy(value, & (line[equal_position + 1]), 127);

			// check for fields and values
			if (!strcasecmp(field, "acceleratedai")) {
				sscanf(value, "%d", &skipComputerPlay);
				if (skipComputerPlay > SKIP_HUMANS_DEAD)
					skipComputerPlay = SKIP_HUMANS_DEAD;
			} else if (!strcasecmp(field, "checkupdates")) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				check_for_updates = val > 0 ? true : false;
			} else if (!strcasecmp(field, "colourtheme") ) {
				sscanf(value, "%d", &colourTheme);
				if (colourTheme < CT_REGULAR) colourTheme = CT_REGULAR;
				if (colourTheme > CT_CRISPY)  colourTheme = CT_CRISPY;
			} else if (!strcasecmp(field, "debrislevel") )
				sscanf(value, "%d", &debris_level);
			else if (!strcasecmp(field, "detailedland")) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				detailedLandscape = val > 0 ? true : false;
			} else if (!strcasecmp(field, "detailedsky")) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				detailedSky = val > 0 ? true : false;
			} else if (!strcasecmp(field, "dither")) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				ditherGradients = val > 0 ? true : false;
			} else if (!strcasecmp(field, "dividemoney") ) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				divide_money = val > 0 ? true : false;
			} else if (!strcasecmp(field, "doboxwrap") ) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				do_box_wrap = val > 0 ? true : false;
			} else if (!strcasecmp(field, "dynamicmenubg") ) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				dynamicMenuBg = val > 0 ? true : false;
			} else if (!strcasecmp(field, "frames") ) {
				int32_t new_fps = 0;
				sscanf(value, "%d", &new_fps);
				set_fps(new_fps);
			} else if (!strcasecmp(field, "fullscreen"))
				sscanf(value, "%d", &full_screen);
			else if (!strcasecmp(field, "interest"))
				sscanf(value, "%lf", &interest);
			else if (!strcasecmp(field, "language") ) {
				uint32_t stored_lang = 0;
				sscanf(value, "%u", &stored_lang);
				language = static_cast<eLanguages>(stored_lang);
			} else if (!strcasecmp(field, "maxfiretime") )
				sscanf(value, "%d", &maxFireTime);
			else if (!strcasecmp(field, "networking")) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				network_enabled = val > 0 ? true : false;
			} else if (!strcasecmp(field, "networkport"))
				sscanf(value, "%d", &network_port);
			else if (!strcasecmp(field, "numpermanentplayers"))
				sscanf(value, "%d", &numPermanentPlayers);
			else if (!strcasecmp(field, "osmouse")) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				osMouse = val > 0 ? true : false;
			} else if (!strcasecmp(field, "playmusic")) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				play_music = val > 0 ? true : false;
			} else if (!strcasecmp(field, "rounds") )
				sscanf(value, "%u", &rounds);
			else if (!strcasecmp(field, "scoreboard")) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				global.showScoreBoard = val > 0 ? true : false;
			} else if (!strcasecmp(field, "scorehitunit"))
				sscanf(value, "%d", &scoreHitUnit);
			else if (!strcasecmp(field, "scoreroundwinbonus"))
				sscanf(value, "%d", &scoreRoundWinBonus);
			else if (!strcasecmp(field, "scoreselfhit"))
				sscanf(value, "%d", &scoreSelfHit);
			else if (!strcasecmp(field, "scoreteamhit"))
				sscanf(value, "%d", &scoreTeamHit);
			else if (!strcasecmp(field, "scoreunitdestroybonus"))
				sscanf(value, "%d", &scoreUnitDestroyBonus);
			else if (!strcasecmp(field, "scoreunitselfdestroy"))
				sscanf(value, "%d", &scoreUnitSelfDestroy);
			else if (!strcasecmp(field, "sellpercent"))
				sscanf(value, "%lf", &sellpercent);
#ifdef NETWORK
			else if ( !strcasecmp(field, "servername") )
				sscanf(value, "%*[']%[^']%*[']", server_name);
			else if ( !strcasecmp(field, "serverport") )
				sscanf(value, "%*[']%[^']%*[']", server_port);
#endif // NETWORK
			else if ( !strcasecmp(field, "showaifeedback") ) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				showAIFeedback = val > 0 ? true : false;
			} else if ( !strcasecmp(field, "showfps") ) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				showFPS = val > 0 ? true : false;
			} else if (!strcasecmp(field, "soundenabled")) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				sound_enabled = val > 0 ? true : false;
			} else if (!strcasecmp(field, "sounddriver"))
				sscanf(value, "%d", &sound_driver);
			else if (!strcasecmp(field, "startmoney"))
				sscanf(value, "%d", &startmoney);
			else if (!strcasecmp(field, "turntype"))
				sscanf(value, "%d", &turntype);
			else if (!strcasecmp(field, "violentdeath") )
				sscanf(value, "%d", &violent_death);
			else if (!strcasecmp(field, "windstrength") )
				sscanf(value, "%d", &windstrength);
			else if (!strcasecmp(field, "windvariation") )
				sscanf(value, "%d", &windvariation);
			else if (!strcasecmp(field, "viscosity") ) {
				sscanf(value, "%lf", &viscosity);
			if (viscosity < 0.25)
				viscosity = 0.5;
			} else if (!strcasecmp(field, "gravity") ) {
				sscanf(value, "%lf", &gravity);
				if (gravity < 0.025)
					gravity = 0.15;
			} else if (!strcasecmp(field, "techlevel")) {
				sscanf(value, "%d", &weapontechLevel);
				itemtechLevel = weapontechLevel;    // for backward compatibility
			} else if (!strcasecmp(field, "weapontechlevel") )
				sscanf(value, "%d", &weapontechLevel);
			else if (!strcasecmp(field, "itemtechlevel") )
				sscanf(value, "%d", &itemtechLevel);
			else if (!strcasecmp(field, "meteors"))
				sscanf(value, "%d", &meteors);
			else if (!strcasecmp(field, "lightning") )
				sscanf(value, "%d", &lightning);
			else if (!strcasecmp(field, "satellite") )
				sscanf(value, "%d", &satellite);
			else if (!strcasecmp(field, "fog") )
				sscanf(value, "%d", &fog);
			else if (!strcasecmp(field, "landtype"))
				sscanf(value, "%d", &landType);
			else if (!strcasecmp(field, "landslidetype"))
				sscanf(value, "%d", &landSlideType);
			else if (!strcasecmp(field, "walltype"))
				sscanf(value, "%d", &wallType);
			else if (!strcasecmp(field, "boxmode"))
				sscanf(value, "%d", &boxedMode);
			else if (!strcasecmp(field, "textfade")) {
				int32_t res = 0;
				sscanf(value, "%d", &res);
				fadingText = res ? true : false;
			} else if (!strcasecmp(field, "textshadow")) {
				int32_t res = 0;
				sscanf(value, "%d", &res);
				shadowedText = res ? true : false;
			} else if (!strcasecmp(field, "textsway")) {
				int32_t res = 0;
				sscanf(value, "%d", &res);
				swayingText = res ? true : false;
			} else if (!strcasecmp(field, "landslidedelay"))
				sscanf(value, "%d", &landSlideDelay);
			else if (!strcasecmp(field, "fallingdirtballs") ) {
				sscanf(value, "%d", &falling_dirt_balls);
				if (falling_dirt_balls < 0) falling_dirt_balls = 0;
				if (falling_dirt_balls > 3) falling_dirt_balls = 3;
			} else if (!strcasecmp(field, "custombackground") )
				sscanf(value, "%d", &custom_background);
			else if (!strcasecmp(field, "volumefactor") )
				sscanf(value, "%d", &volume_factor);
			else if (!strcasecmp(field, "volleydelay") )
				sscanf(value, "%d", &volley_delay);
			else if (!strcasecmp(field, "screenwidth"))
				sscanf(value, "%d", &screenWidth);
			else if (!strcasecmp(field, "screenheight"))
				sscanf(value, "%d", &screenHeight);
		}     // end of read a line properly
	}     // end of while not done

	// If values were set on the command line, override
	// configuration values:
	if (temp_screenHeight)
		screenHeight = temp_screenHeight;
	else
		temp_screenHeight = screenHeight;
	if (temp_screenWidth)
		screenWidth = temp_screenWidth;
	else
		temp_screenWidth = screenWidth;

	// The resolution must not be below 800x600:
	if (screenHeight < 600)
		screenHeight = 600;
	if (screenWidth  < 800)
		screenWidth  = 800;

	// The screen resolution values must be copied back into
	// the temp variables, which are then used by the menu,
	// or changing the resolution will make the menu exit crash.
	// The resolution is set only once on game start, so
	// these changes go into temp and are stored back from them.
	temp_screenHeight = screenHeight;
	temp_screenWidth  = screenWidth;

	if (! sound_bookmark)
		sound_enabled = false;

	halfWidth = screenWidth / 2;
	halfHeight = screenHeight / 2;

	menuBeginY = (screenHeight - 400) / 2;
	if (menuBeginY < 0) menuBeginY = 0;
	menuEndY = screenHeight - menuBeginY;

	return;
}


#define LOAD_TEXT_BLOCK(var, file) try { \
	snprintf(path_buf, PATH_MAX, "%s/text/%s%s", dataDir, file, suffix); \
	TEXTBLOCK* new_##var = new TEXTBLOCK(path_buf); \
	if (var) \
		delete var; \
	var = new_##var; \
} catch (...) { }

/** @brief load text files according to set language
  * This function loads all needed text files, based on
  * language, into memory. If a previous text was loaded, it is
  * removed from memory first.
**/
void ENVIRONMENT::load_text_files()
{
	char suffix[12] = { 0 };

	switch (language) {
		case EL_FRENCH:
			strncpy(suffix, "_fr.txt", 11);
			snprintf(path_buf, PATH_MAX, "%s/text/war_quotes.txt", dataDir);
			break;
		case EL_GERMAN:
			strncpy(suffix, "_de.txt", 11);
			snprintf(path_buf, PATH_MAX, "%s/text/war_quotes.txt", dataDir);
			break;
		case EL_ITALIAN:
			strncpy(suffix, "_it.txt", 11);
			snprintf(path_buf, PATH_MAX, "%s/text/war_quotes_it.txt", dataDir);
			break;
		case EL_PORTUGUESE:
			strncpy(suffix, ".pt_BR.txt", 11);
			snprintf(path_buf, PATH_MAX, "%s/text/war_quotes.txt", dataDir);
			break;
		case EL_RUSSIAN:
			strncpy(suffix, "_ru.txt", 11);
			snprintf(path_buf, PATH_MAX, "%s/text/war_quotes_ru.txt", dataDir);
			break;
		case EL_SLOVAK:
			strncpy(suffix, "_sk.txt", 11);
			snprintf(path_buf, PATH_MAX, "%s/text/war_quotes.txt", dataDir);
			break;
		case EL_SPANISH:
			strncpy(suffix, "_ES.txt", 11);
			snprintf(path_buf, PATH_MAX, "%s/text/war_quotes_ES.txt", dataDir);
			break;
		case EL_ENGLISH:
		default:
			strncpy(suffix, ".txt", 11);       // default to english
			snprintf(path_buf, PATH_MAX, "%s/text/war_quotes.txt", dataDir);
			break;
	}

	try {
		TEXTBLOCK* new_war_quotes = new TEXTBLOCK(path_buf);
		if (war_quotes)
			delete war_quotes;
		war_quotes = new_war_quotes;
	} catch (...) { /* can't do anything helpful here anyway */ }


	LOAD_TEXT_BLOCK(gloat, "gloat")
	LOAD_TEXT_BLOCK(ingame, "ingame")
	LOAD_TEXT_BLOCK(instructions, "instr")
	LOAD_TEXT_BLOCK(panic, "panic")
	LOAD_TEXT_BLOCK(kamikaze, "kamikaze")
	LOAD_TEXT_BLOCK(retaliation, "retaliation")
	LOAD_TEXT_BLOCK(revenge, "revenge")
	LOAD_TEXT_BLOCK(suicide, "suicide")
}


/** @brief load a background music file.
  *
  * This function loads a music file (if there is one available.)
  * If a current sample is set, it will be released.
  *
  * @return true if a sample is loaded, false otherwise.
**/
bool ENVIRONMENT::loadBackgroundMusic()
{
	static bool isSecondTry = false;

	// see if we should bother
	if (!play_music)
		return false;

	SAMPLE* newStream    = nullptr;
	dirent*               folder_entry = nullptr;


	// see if we have the music folder open
	if (! music_dir) {
		snprintf(path_buf, PATH_MAX, "%s/music", configDir);
		music_dir = opendir(path_buf);
		if (!music_dir)
			return false;
	}


    // at this point we should have an open music folder
    // the music folder is closed by global's deconstructor
    // search for files ending in .wav
	folder_entry = readdir(music_dir);
	while (folder_entry && !newStream) {
		// we have something, see if it is a wav file
		if ( strstr(folder_entry->d_name, ".wav") ) {
			snprintf(path_buf, PATH_MAX, "%s/music/%s",
						configDir, folder_entry->d_name);
			newStream = load_sample(path_buf);
		}
		if (!newStream)
			folder_entry = readdir(music_dir);
	}

	if (!folder_entry)  {
		// hit end of folder
		closedir(music_dir);
		music_dir = nullptr;

		// If there is a current background music file loaded, then the
		// directory is just gone through completely. In that case a
		// recursive call re-opens the directory and starts anew.
		if (!isSecondTry && background_music) {
			isSecondTry = true;
			return loadBackgroundMusic();
		} else {
			// Otherwise there is either an error or there are no
			// files in that directory
			if (background_music) {
				// Okay, this is odd.
				destroy_sample(background_music);
				background_music = nullptr;
			}
			play_music = false;
			return false;
		}
	}

	if (background_music)
		destroy_sample(background_music);
	background_music = newStream;
	isSecondTry      = false;

	return true;
}


/*
 * This function loads all the bitmaps needed by the game.
 * Bitmaps are found in a series of sub-folders under the
 * data directory. The function returns true on success and
 * false if an error occurs.
*/
bool ENVIRONMENT::loadBitmaps()
{
	int32_t  file_group    = 0;
	char     sub_folder[9] = { 0 };
	BITMAP*  newbitmap     = nullptr;
	BITMAP** bitmap_array  = nullptr;

	while (file_group < 7) {
		// set the folder we're looking at
		switch (file_group) {
			case 0: strncpy(sub_folder, "title",   8); break;
			case 1: strncpy(sub_folder, "button",  8); break;
			case 2: strncpy(sub_folder, "misc",    8); break;
			case 3: strncpy(sub_folder, "missile", 8); break;
			case 4: strncpy(sub_folder, "stock",   8); break;
			case 5: strncpy(sub_folder, "tank",    8); break;
			case 6: strncpy(sub_folder, "tankgun", 8); break;
		}

		// set up empty array
		int32_t array_size = 10;
		bitmap_array = (BITMAP **) calloc(10, sizeof(BITMAP *) );
		if (! bitmap_array) {
			printf("Ran out of memory, loading bitmaps.\n");
			return false;
		}

		// search for files
		int32_t file_count = 0;
		snprintf(path_buf, PATH_MAX, "%s/%s/%d.bmp", dataDir,
				sub_folder, file_count);
		while ( !access(path_buf, F_OK | R_OK) && bitmap_array ) {
			newbitmap = load_bitmap(path_buf, nullptr);
			if (! newbitmap)
				printf("An error occured loading bitmap %s\n", path_buf);

			// Crop tank bitmaps for unification
			if (newbitmap && (5 == file_group)) {
				int32_t left   = 0;
				int32_t right  = newbitmap->w;
				int32_t top    = 0;
				int32_t bottom = newbitmap->h;

				// Find real left edge
				bool hasPix = false;
				while (!hasPix && (left < right)) {
					for (int32_t y = top; !hasPix && (y < bottom); ++y) {
						if (PINK != getpixel(newbitmap, left, y))
							hasPix = true;
					}
					if (!hasPix)
						++left;
				}

				// Find real right edge
				hasPix = false;
				while (!hasPix && (right > left)) {
					for (int32_t y = top; !hasPix && (y < bottom); ++y) {
						if (PINK != getpixel(newbitmap, right, y))
							hasPix = true;
					}
					if (!hasPix)
						--right;
				}

				// Find real top edge
				hasPix = false;
				while (!hasPix && (top < bottom)) {
					for (int32_t x = left; !hasPix && (x < right); ++x) {
						if (PINK != getpixel(newbitmap, x, top))
							hasPix = true;
					}
					if (!hasPix)
						++top;
				}

				// Find real bottom edge
				hasPix = false;
				while (!hasPix && (bottom > top)) {
					for (int32_t x = left; !hasPix && (x < right); ++x) {
						if (PINK != getpixel(newbitmap, x, bottom))
							hasPix = true;
					}
					if (!hasPix)
						--bottom;
				}

				// Now create the real bitmap
				bitmap_array[file_count] = create_bitmap(right - left,
															bottom - top);
				blit(newbitmap, bitmap_array[file_count], left, top, 0, 0,
						right - left, bottom - top);

				destroy_bitmap(newbitmap);
			} // End of cropping tank bitmap

			// otherwise just copy the bitmap pointer
			else {
				bitmap_array[file_count] = newbitmap;
			}

			file_count++;

			// make sure array is large enough
			if ( file_count >= array_size) {
				array_size += 10;
				bitmap_array = (BITMAP **) realloc(bitmap_array,
									sizeof(BITMAP *) * (array_size + 1) );
			if (! bitmap_array) {
				printf("Unable to increase array size while loading bitmaps.\n");
				return false;
			} else
				memset(bitmap_array + file_count, 0,
						sizeof(BITMAP*) * (array_size - file_count));
		}

			// get next file
			snprintf(path_buf, PATH_MAX, "%s/%s/%d.bmp", dataDir,
						sub_folder, file_count);
		}

		// save the new array
		switch (file_group) {
			case 0: title   = bitmap_array; break;
			case 1: button  = bitmap_array; break;
			case 2: misc    = bitmap_array; break;
			case 3: missile = bitmap_array; break;
			case 4: stock   = bitmap_array; break;
			case 5: tank    = bitmap_array; break;
			case 6: tankgun = bitmap_array; break;
		}

		file_group++;
	}

	return true;
}


// This file loads in extra fonts the game requires.
// Fonts should be stored in the datafolder. On
// success the function returns true. When an
// error occurs, it returns false.
bool ENVIRONMENT::loadFonts()
{
	snprintf(path_buf, PATH_MAX, "%s/unicode.dat", dataDir);

	main_font = load_font(path_buf, nullptr, nullptr);

	if (main_font)
		font = main_font;
	else
		printf("Unable to load font %s\n", path_buf);

	// Store font height
	if (main_font)
		fontHeight = text_height(main_font);

	return main_font ? true : false;
}


/// @brief collection of all game text file loadings.
bool ENVIRONMENT::loadGameFiles()
{
	// Before the (language specific) weapons texts can be loaded,
	// the english one must be pre-loaded to get the weapons data.
	// all other files only hold the texts.
	bool status = true;
	if (EL_ENGLISH != language) {
        eLanguages cur_lang = language;
        language = EL_ENGLISH;
		status   = Load_Weapons_Text();
        language = cur_lang;
	}

	if (status)
		status = Load_Weapons_Text();

	if (!status) {
		cerr << "ERROR: An error occurred trying to read weapons file." << endl;
		return status;
	}
	// Note: If english is chosen, the first load is not done.
	//       If english is not chosen, the first load pre-loads english
	//       Thus the second load is always necessary.


	bitmap_filenames = Find_Bitmaps(&number_of_bitmaps);

	// If no bitmaps where found, a custom background is futile.
	if ( custom_background
	  && !bitmap_filenames)
		custom_background = 0;

	Create_Music_Folder();
	genItemsList ();

	return status;
}


/** @brief load all needed sounds
  * This function loads all sounds from the data folder and saves them
  * in an array.
  * @return true on success or false if an error happens.
**/
bool ENVIRONMENT::loadSounds()
{
	SAMPLE *temp_sample = nullptr;

	// allocate space for sound samples
	sounds = (SAMPLE **) calloc(SND_COUNT, sizeof(SAMPLE *) );
	if (! sounds) {
		printf("Unable to create sound array.\n");
		return false;
	}

	// read from directory
	for (int32_t i = 0; i < SND_COUNT; ++i) {
		snprintf(path_buf, PATH_MAX, "%s/sound/%02d.wav", dataDir, i);
		if (!access(path_buf, R_OK)) {
			temp_sample = load_sample(path_buf);
			if (temp_sample)
				sounds[i] = temp_sample;
			else
				fprintf(stderr, "An error occured loading sound file %s\n", path_buf);
		}
		// No else, because the sound enum has free slots.
	}

   return true;
}


void ENVIRONMENT::newRound ()
{
	// set wall type
	if (wallType == WALL_RANDOM)
		current_wallType = rand() % 4;
	else
		current_wallType = wallType;

	time_to_fall = (rand() & landSlideDelay) + 1;

	// Set boxed mode
	if (BM_RANDOM == boxedMode) {
		if (rand() % 2)
			isBoxed = true;
		else
			isBoxed = false;
	} else if (BM_ON == boxedMode)
		isBoxed = true;
	else
		isBoxed = false;

	// Set wall colour
    switch (current_wallType)
    {
       case WALL_RUBBER:
          wallColour = makecol(0, 255, 0); break;
       case WALL_STEEL:
          wallColour = makecol(255, 0, 0); break;
       case WALL_SPRING:
          wallColour = makecol(0, 0, 255); break;
       case WALL_WRAP:
          wallColour = makecol(255, 255, 0); break;
    }

	// Init player array
	for (int32_t i = 0; i < MAXPLAYERS; ++i)
		playerOrder[i] = nullptr;
}


void ENVIRONMENT::removeGamePlayer(PLAYER* player_)
{
	int32_t fromCount = 0;
	int32_t toCount = -1;

	if (HUMAN_PLAYER == player_->type)
		numHumanPlayers--;

	while (fromCount < numGamePlayers) {
		if (player_ != players[fromCount]) {
			if ((toCount >= 0) && (fromCount > toCount)) {
				players[toCount]    = players[fromCount];
				players[fromCount]  = nullptr;
				toCount++;
			}
		} else
			// Position found, now move the remaining players down!
			toCount = fromCount;
		fromCount++;
	}
	numGamePlayers--;
}


/*
 * This function puts all the of the environment settings back
 * to their default values. These are settings which get written
 * to the config file.
 * -- Jesse
 *  */
void ENVIRONMENT::Reset_Options()
{
	boxedMode             = BM_OFF;
	check_for_updates     = true;
	colourTheme           = CT_CRISPY;
	custom_background     = 0;
	debris_level          = 1;
	detailedLandscape     = false;
	detailedSky           = false;
	ditherGradients       = true;
	divide_money          = false;
	fadingText            = false;
	falling_dirt_balls    = 0;
	fog                   = 0;
	set_fps(60);
	gravity               = 0.15;
	interest              = 1.25;
	itemtechLevel         = 5;
	landSlideDelay        = MAX_GRAVITY_DELAY;
	landSlideType         = SLIDE_GRAVITY;
	landType              = LAND_RANDOM;
	language              = EL_ENGLISH;
	lightning             = 0;
	maxFireTime           = 0;
	meteors               = 0;
	network_enabled       = false;
	network_port          = DEFAULT_NETWORK_PORT;
	osMouse               = true;
	play_music            = 1.0;
	satellite             = 0;
	scoreHitUnit          = 75;
	scoreRoundWinBonus    = 10000;
	scoreSelfHit          = 25;
	scoreTeamHit          = 10;
	scoreUnitDestroyBonus = 5000;
	scoreUnitSelfDestroy  = 0;
	sellpercent           = 0.80;
	shadowedText          = true;
	skipComputerPlay      = SKIP_HUMANS_DEAD;
	sound_driver          = SD_AUTODETECT;
	sound_enabled         = true;
	startmoney            = 15000;
	swayingText           = true;
	temp_screenHeight     = DEFAULT_SCREEN_HEIGHT;
	temp_screenWidth      = DEFAULT_SCREEN_WIDTH;
	turntype              = TURN_RANDOM;
	viscosity             = 0.5;
	violent_death         = 0;
	volley_delay          = 10;
	volume_factor         = MAX_VOLUME_FACTOR;
	wallType              = WALL_RUBBER;
	weapontechLevel       = 5;
	windstrength          = 8;
	windvariation         = 1;

	strncpy(server_name, "127.0.0.1", 127);
	strncpy(server_port, "25645", 127);
}


/** @brief Save environment settings to a text file
  *
  * This function saves the environment settings to a text file. Each line has
  * the format name=value.\n
  *
  * @return true on success and false on failure.
*/
bool ENVIRONMENT::save_to_file (FILE *file)
{
	if (!file)
		return false;

	fprintf (file, "*ENV*\n");

	fprintf (file, "ACCELERATEDAI=%d\n", skipComputerPlay);
	fprintf (file, "BOXMODE=%d\n", boxedMode);
	fprintf (file, "CHECKUPDATES=%d\n", check_for_updates ? 1 : 0);
	fprintf (file, "COLOURTHEME=%d\n", colourTheme);
	fprintf (file, "CUSTOMBACKGROUND=%d\n", custom_background);
	fprintf (file, "DEBRISLEVEL=%d\n", debris_level);
	fprintf (file, "DETAILEDLAND=%d\n", detailedLandscape ? 1 : 0);
	fprintf (file, "DETAILEDSKY=%d\n", detailedSky ? 1 : 0);
	fprintf (file, "DITHER=%d\n", ditherGradients ? 1 : 0);
	fprintf (file, "DIVIDEMONEY=%d\n", divide_money);
	fprintf (file, "DOBOXWRAP=%d\n", do_box_wrap);
	fprintf (file, "DYNAMICMENUBG=%d\n", dynamicMenuBg ? 1 : 0);
	fprintf (file, "FALLINGDIRTBALLS=%d\n", falling_dirt_balls);
	fprintf (file, "FOG=%d\n", fog);
	fprintf (file, "FRAMES=%d\n", frames_per_second);
	fprintf (file, "FULLSCREEN=%d\n", full_screen);
	fprintf (file, "GRAVITY=%f\n", gravity);
	fprintf (file, "INTEREST=%f\n", interest);
	fprintf (file, "ITEMTECHLEVEL=%d\n", itemtechLevel);
	fprintf (file, "LANDSLIDEDELAY=%d\n", landSlideDelay);
	fprintf (file, "LANDSLIDETYPE=%d\n", landSlideType);
	fprintf (file, "LANDTYPE=%d\n", landType);
	fprintf (file, "LANGUAGE=%u\n", static_cast<uint32_t>(language));
	fprintf (file, "LIGHTNING=%d\n", lightning);
	fprintf (file, "MAXFIRETIME=%d\n", maxFireTime);
	fprintf (file, "METEORS=%d\n", meteors);
	fprintf (file, "NETWORKING=%d\n", network_enabled ? 1 : 0);
	fprintf (file, "NETWORKPORT=%d\n", network_port);
	fprintf (file, "NUMPERMANENTPLAYERS=%d\n", numPermanentPlayers);
	fprintf (file, "OSMOUSE=%d\n", osMouse);
	fprintf (file, "PLAYMUSIC=%d\n", play_music ? 1 : 0);
	fprintf (file, "ROUNDS=%d\n", rounds);
	fprintf (file, "SATELLITE=%d\n", satellite);
	fprintf (file, "SCOREBOARD=%d\n", global.showScoreBoard ? 1 : 0);
	fprintf (file, "SCOREHITUNIT=%d\n", scoreHitUnit);
	fprintf (file, "SCOREROUNDWINBONUS=%d\n", scoreRoundWinBonus);
	fprintf (file, "SCORESELFHIT=%d\n", scoreSelfHit);
	fprintf (file, "SCORETEAMHIT=%d\n", scoreTeamHit);
	fprintf (file, "SCOREUNITDESTROYBONUS=%d\n", scoreUnitDestroyBonus);
	fprintf (file, "SCOREUNITSELFDESTROY=%d\n", scoreUnitSelfDestroy);
	fprintf (file, "SCREENHEIGHT=%d\n", temp_screenHeight);
	fprintf (file, "SCREENWIDTH=%d\n", temp_screenWidth);
	fprintf (file, "SELLPERCENT=%f\n", sellpercent);
	fprintf (file, "SERVERNAME='%s'\n", server_name);
	fprintf (file, "SERVERPORT='%s'\n", server_port);
	fprintf (file, "SHOWAIFEEDBACK=%d\n", showAIFeedback ? 1 : 0);
	fprintf (file, "SHOWFPS=%d\n", showFPS ? 1 : 0);
	fprintf (file, "SOUNDDRIVER=%d\n", sound_driver);
	fprintf (file, "SOUNDENABLED=%d\n", sound_enabled ? 1 : 0);
	fprintf (file, "STARTMONEY=%d\n", startmoney);
	fprintf (file, "TEXTFADE=%d\n", fadingText ? 1 : 0);
	fprintf (file, "TEXTSHADOW=%d\n", shadowedText ? 1 : 0);
	fprintf (file, "TEXTSWAY=%d\n", swayingText ? 1 : 0);
	fprintf (file, "TURNTYPE=%d\n", turntype);
	fprintf (file, "VISCOSITY=%f\n", viscosity);
	fprintf (file, "VIOLENTDEATH=%d\n", violent_death);
	fprintf (file, "VOLLEYDELAY=%d\n", volley_delay);
	fprintf (file, "VOLUMEFACTOR=%d\n", volume_factor);
	fprintf (file, "WALLTYPE=%d\n", wallType);
	fprintf (file, "WEAPONTECHLEVEL=%d\n", weapontechLevel);
	fprintf (file, "WINDSTRENGTH=%d\n", windstrength);
	fprintf (file, "WINDVARIATION=%d\n", windvariation);
	fprintf (file, "***\n");

	return true;
}


/// @brief This function sends a message to all connected game clients.
/// @return true on success or false if the message could not be sent
bool ENVIRONMENT::sendToClients(const char* message)
{
	if (! message) return false;

#ifdef NETWORK
	int32_t written = 0;
	int32_t message_length = strlen(message);

	for (int32_t index = 0; index < numGamePlayers; index++) {
		if ( (players[index]) && (players[index]->type == NETWORK_CLIENT) ) {
			written = write(players[index]->server_socket,
							message, message_length);
			if (written < message_length)
				fprintf(stderr, "%s:%d: Warning: Only %d/%d bytes sent to player %d\n",
						__FILE__, __LINE__, written, message_length, index);
		}
	} // done all players
#endif // NETWORK
	return true;
}



/// @brief set new frames per second if valid and calculate dependent values.
void ENVIRONMENT::set_fps(int32_t new_FPS)
{
	if (!new_FPS || ((new_FPS > 0) && (new_FPS != frames_per_second)) ) {
		if (new_FPS)
			frames_per_second = new_FPS;
		FPS_mod           = 100. / static_cast<double>(frames_per_second);
		maxVelocity       = static_cast<double>(MAX_POWER) * FPS_mod / 100.;
	}
}


void ENVIRONMENT::window_update(int32_t x, int32_t y, int32_t w, int32_t h)
{
	if (x < window.x)
		window.x = x;
	if (y < window.y)
		window.y = y;
	if (x + w > window.w)
		window.w = (x + w) - 1;
	if (y + h > window.h)
		window.h = (y + h) - 1;
	if (window.x < 0)
		window.x = 0;
	if (window.y < MENUHEIGHT)
		window.y = MENUHEIGHT;
	if (window.w > (screenWidth-1))
		window.w = (screenWidth-1);
	if (window.h > (screenHeight-1))
		window.h = (screenHeight-1);
}
