#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cassert>

// basically all UNIX-like OSes should use stat
#ifndef WIN32
#  include <sys/stat.h>
#endif

#include "player.h"
#include "files.h"
#include "text.h"


// They are filled here, declaring them here prevents the linker
// from 'optimizing' them away.
WEAPON  weapon[WEAPONS];
WEAPON  naturals[NATURALS];
ITEM    item[ITEMS];



/* Update: Instead of using constantly allocated/deallocated
 * char strings for file path generation, one string buffer
 * used everywhere that does not need *alloc/free is faster
 * and a lot more secure.
 * - Sven
 */
char path_buf[PATH_MAX + 1];
// Note: Any consumer of this buffer has to include files.h or
//       to define an extern for it.


/** @brief Save the current game in progress
  * This function saves the game in progress.
  * All data is saved in a text file for flexibility.
  * @return true on success and false on failure.
**/
bool Save_Game()
{
	snprintf(path_buf, PATH_MAX, "%s/%s.sav", env.configDir, env.game_name);

	FILE* game_file = fopen(path_buf, "w");
	if (!game_file)
		return false;

	// write file version information
	fprintf(game_file, "VERSION\n");
	fprintf(game_file, "FILE_VERSION=%d\n", game_version);
	fprintf(game_file, "***\n");

	// write global data
	fprintf(game_file, "GLOBAL\n");
	fprintf(game_file, "CURRENTROUND=%d\n", global.currentround + 1);
	// Note: When the game is saved, the round already has been decreased. Thus,
	// to not loose rounds when saving a game, returning to main and load it
	// again, this has to be increased here.
	fprintf(game_file, "SCOREBOARD=%d\n", global.showScoreBoard ? 1 : 0);
	fprintf(game_file, "***\n");

	// write environment data
	fprintf(game_file, "ENVIRONMENT\n");
	fprintf(game_file, "CAMPAIGNMODE=%d\n", env.campaign_mode ? 1 : 0);
	fprintf(game_file, "CAMPAIGNROUNDS=%lf\n", env.campaign_rounds);
	fprintf(game_file, "NEXTCAMPROUND=%lf\n", env.nextCampaignRound);
	fprintf(game_file, "ROUNDS=%u\n", env.rounds);
	fprintf(game_file, "***\n");

	// write player data
	fprintf(game_file, "PLAYERS\n");
	for (int32_t i = 0; i < env.numGamePlayers; ++i) {
		PLAYER* my_player = env.players[i];

		if (my_player->index > -1) {
			// Note: This line is needed to know which player to load
			fprintf(game_file, "PLAYERNUMBER=%d\n", my_player->index);
			my_player->save_game_data(game_file);
		}
	}

	fprintf(game_file, "***EOF***\n");

	fclose(game_file);

	/* atanks always saved the current configuration in a file
	 * alongside the save game, but this environment file was
	 * never read. It makes no sense anyway, as it would change
	 * the current settings without re-loading the main config
	 * file.
	 * However, I find this very good for testing reasons, and
	 * if a game becomes boring, why not allow the player to
	 * enable some weather effects before loading a game?
	 * So it won't be added that the environment file is loaded,
	 * but if an old one is laying around, we should delete our
	 * own garbage:
	 */
	snprintf(path_buf, PATH_MAX, "%s/%s.txt",
	         env.configDir, env.game_name);
	if (!access(path_buf, F_OK) && !access(path_buf, W_OK))
		unlink(path_buf);

	return true;
}



/*
This function attempts to load a saved
game.
The function returns TRUE on success and
FALSE if an error occurs.
-- Jesse
*/
bool Load_Game()
{
	char    line[ MAX_CONFIG_LINE + 1] = { 0 };
	char    field[MAX_CONFIG_LINE + 1] = { 0 };
	char    value[MAX_CONFIG_LINE + 1] = { 0 };
	char*   result                     = nullptr;
	int32_t player_count               = 0;
	int32_t line_num                   = 0;
	int32_t player_idx                 = -1;
	bool    done                       = false;
	int32_t file_version               = 0;

	// Be sure that numbers are understood right:
	const char* cur_lc_numeric = setlocale(LC_NUMERIC, "C");

	// Ensure backward compatibility:
	env.nextCampaignRound = -1.;
	env.campaign_rounds   = -1.;

	// Open game file
	snprintf(path_buf, PATH_MAX, "%s/%s.sav", env.configDir, env.game_name);
	FILE* game_file = fopen(path_buf, "r");
	if (!game_file)
		return false;

	// Now read until the file is finished loading
	eSaveGameStage stage = SGS_NONE;
	do {
		// read a line
		memset(line, 0, MAX_CONFIG_LINE);
		if ( ( result = fgets(line, MAX_CONFIG_LINE, game_file) ) ) {
			++line_num;

			// if we hit end of the file, stop
			if (! strncmp(line, "***EOF***", 9) ) {
				done = true;
				continue; // This exits the loop as well
			}

			// strip newline character
			size_t line_length = strlen(line);
			while ( line[line_length - 1] == '\n') {
				line[line_length - 1] = '\0';
				line_length--;
			}

			// check to see if we found a new stage
			if (!strcasecmp(line, "GLOBAL") )
				stage = SGS_GLOBAL;
			else if (!strcasecmp(line, "ENVIRONMENT") )
				stage = SGS_ENVIRONMENT;
			else if (!strcasecmp(line, "PLAYERS") ) {
				// Here the file version must be known, or it is not set.
				// Inform the user if an upgrade is needed
				if (game_version > file_version)
					fprintf(stdout, "Game \"%s\" needs to be upgraded"
					        " from version %2.1f to version %2.1f\n",
					        env.game_name,
					        static_cast<float>(file_version) / 10.0,
					        static_cast<float>(game_version) / 10.0);

				stage = SGS_PLAYERS;
			} else if (!strcasecmp(line, "VERSION") )
				stage = SGS_VERSION;
			else {
				// Not a new stage, keep loading.

				// find equal sign
				size_t equal_position = 1;
				while ( ( equal_position < line_length )
					 && ( line[equal_position] != '='  ) )
					equal_position++;

				// make sure the equal sign position is valid
				if (line[equal_position] != '=')
					continue; // Go to next line

				// separate field from value
				memset(field, '\0', MAX_CONFIG_LINE);
				memset(value, '\0', MAX_CONFIG_LINE);
				strncpy(field, line, equal_position);
				strncpy(value, &( line[equal_position + 1] ), MAX_CONFIG_LINE);

				switch (stage) {
					case SGS_ENVIRONMENT:
						if (!strcasecmp(field, "CAMPAIGNMODE") ) {
							int32_t cm = 0;
							sscanf(value, "%d", &cm);
							env.campaign_mode = !cm ? false : true;
						} else if (!strcasecmp(field, "CAMPAIGNROUNDS") )
							sscanf(value, "%lf", &env.campaign_rounds);
						else if (!strcasecmp(field, "ROUNDS") )
							sscanf(value, "%u", &env.rounds);
						else if (!strcasecmp(field, "NEXTCAMPROUND") )
							sscanf(value, "%lf", &env.nextCampaignRound);
						else {
							cerr << path_buf << ":" << line_num << " : Ignored line\n";
							cerr << "The line \"" << line << "\"";
							cerr << " is ignored, it does not belong to ENV" << endl;
						}

						break;
					case SGS_GLOBAL:
						if (!strcasecmp(field, "CURRENTROUND") )
							sscanf(value, "%u", &global.currentround);

						// The following are kept for backwards compatibility:

						else if (!strcasecmp(field, "NEXTCAMPROUND") )
							sscanf(value, "%lf", &env.nextCampaignRound);
						else if (!strcasecmp(field, "CAMPAIGNMODE") ) {
							int32_t cm = 0;
							sscanf(value, "%d", &cm);
							env.campaign_mode = !cm ? false : true;
						} else if (!strcasecmp(field, "ROUNDS") )
							sscanf(value, "%u", &env.rounds);
						else if (!strcasecmp(field, "SCOREBOARD") ) {
							int32_t enabled = 0;
							sscanf(value, "%d", &enabled);
							global.showScoreBoard = (enabled != 0);
						} else {
							cerr << path_buf << ":" << line_num << " : Ignored line\n";
							cerr << "The line \"" << line << "\"";
							cerr << " is ignored, it does not belong to GLOBAL" << endl;
						}

						break;
					case SGS_PLAYERS:
						if (!strcasecmp(field, "PLAYERNUMBER") ) {
							sscanf(value, "%d", &player_idx);
							if ( (player_idx > -1)
							  && (player_idx < env.numPermanentPlayers)
							  && (player_count < MAXPLAYERS) ) {
								env.addGamePlayer( env.allPlayers[player_idx] );
								env.players[player_count++]->initialise(true);
							} else
								player_idx = -1;
						}

						// Now let the player load itself
						if (player_idx > -1)
							env.allPlayers[player_idx]->load_game_data(game_file, file_version);
						else {
							cerr << path_buf << ":" << line_num << " : Ignored line\n";
							cerr << "The line \"" << line << "\"";
							cerr << " is ignored, as player idx is " << player_idx << endl;
						}

						break;
					case SGS_VERSION:
						if (!strcasecmp(field, "FILE_VERSION") )
							sscanf(value, "%d", &file_version);
						else {
							cerr << path_buf << ":" << line_num << " : Ignored line\n";
							cerr << "The line \"" << line << "\"";
							cerr << " is ignored, it does not belong to VERSION" << endl;
						}

						break;
					case SGS_NONE:
					default:
						cerr << path_buf << ":" << line_num << " : Wrong line\n";
						cerr << "The line \"" << line << "\"";
						cerr << " does not belong  to any stage!" << endl;
						break;
				} // end of switching stage
			} // End of loading line / player record
		} // end of having read a line
	} while (result && !done);
	// End of being not done

	fclose(game_file);

	// See if the campaign state values have to be recalculated
	// because an old save game was loaded.
	if (env.campaign_rounds < 0.)
		env.campaign_rounds = static_cast<double>(env.rounds) / 5.;
	if (env.nextCampaignRound < 0.) {
		env.nextCampaignRound = static_cast<double>(env.rounds)
		                      - env.campaign_rounds;
		while (global.currentround < env.nextCampaignRound)
			env.nextCampaignRound -= env.campaign_rounds;
	}

	// To ensure backwards compatibility, all players have to check
	// their opponents memory. Old save files do not provide any.
    for (int32_t i = 0; i < env.numGamePlayers; ++i)
		env.players[i]->checkOppMem();

	// Revert locale settings
	if (cur_lc_numeric)
		setlocale(LC_NUMERIC, cur_lc_numeric);
	else
		setlocale(LC_NUMERIC, "");

	return true;
}



/*
Check to see if a saved game exists with the given name.
*/
bool Check_For_Saved_Game()
{
	snprintf(path_buf, PATH_MAX, "%s/%s.sav", env.configDir, env.game_name);

	if (!access(path_buf, R_OK))
		return true;

	return false;
}



/** @brief Copy atanks config to safe location.
  *
  * This function copies the atanks config file from the HOME_DIR folder to
  * HOME_DIR/.atanks
  * If the .atanks folder does not exist, this function will create it.
  *
  * @return true on success, false otherwise
*/
bool Copy_Config_File()
{
	static char xHere[2]            = ".";
	FILE* source_file               = nullptr;
	FILE* dest_file                 = nullptr;
	char  source_path[PATH_MAX + 1] = { 0 };
	char  dest_path[PATH_MAX + 1]   = { 0 };
	char  buffer[PATH_MAX + 1]      = { 0 };

	// check to see if the config file has already been copied
	snprintf(dest_path, PATH_MAX, "%s/atanks-config.txt", env.configDir);
	if (!access(dest_path, R_OK | W_OK))
		return true;

	char*  my_home_folder = getenv(HOME_DIR);

	// figure out where home is
	if (! my_home_folder)
		my_home_folder = xHere;


	// file not copied yet, create the required directory
	snprintf(buffer, PATH_MAX, "%s/.atanks", my_home_folder);
#ifdef ATANKS_IS_WINDOWS
	int32_t mkdir_status = mkdir(buffer);
#else
	int32_t mkdir_status = mkdir(buffer, 0700);
#endif // ATANKS_IS_WINDOWS

	if (mkdir_status == -1) {
		printf( "Error occured. Unable to create sub directory.\n");
		return false;
	}

	// check to make sure we have a source file
	snprintf(source_path, PATH_MAX, "%s/.atanks-config.txt", my_home_folder);
	source_file = fopen(source_path, "r");
	if (! source_file)
		return true;

	// we already have an open source file, create destination file
	dest_file = fopen(dest_path, "wb");
	if (! dest_file) {
		printf( "Unable to create destination file.\n");
		fclose(source_file);
		return false;
	}

	// we have open files, let's copy
	size_t file_status = fread(buffer, 1, PATH_MAX, source_file);
	while (file_status) {
		file_status = fwrite(buffer, 1, PATH_MAX, dest_file);
		file_status = fread(buffer, 1, PATH_MAX, source_file);
	}

	fclose(source_file);
	fclose(dest_file);
	return true;
}



/** @brief Make sure we have a music folder
  * @return true on success or false if an error occures.
**/
bool Create_Music_Folder()
{
	snprintf(path_buf, PATH_MAX, "%s/music", env.configDir);
	DIR* music_folder = opendir(path_buf);

	if (! music_folder) {
#ifdef ATANKS_IS_WINDOWS
		if (mkdir(path_buf))
#else
		if (mkdir(path_buf, 0700))
#endif // ATANKS_IS_WINDOWS
			return false;
	} else
		// it already exists
		closedir(music_folder);

	return true;
}


/*
Scroll text in a box
*/
void scrollTextList (TEXTBLOCK* lines)
{
	int32_t spacing  = 2;
	int32_t tOffset  = (RAND_MAX / 4) + (rand() % (RAND_MAX / 4));
	int32_t numItems = (rand() % 100) + 20;
	int32_t key      = 0;
	bool    done     = false;
	bool    moving   = true;

	eBackgroundTypes bgType = env.dynamicMenuBg
	                        ? static_cast<eBackgroundTypes>(rand() % BACKGROUND_COUNT)
	                        : BACKGROUND_BLANK;

	drawMenuBackground (bgType, tOffset, numItems);
	quickChange(true);

	int32_t clip_l = env.halfWidth - 299;
	int32_t clip_t = env.menuBeginY + 1;
	int32_t clip_b = env.menuEndY - 1;
	int32_t clip_r = env.halfWidth + 299;
	set_clip_rect(global.canvas, clip_l, clip_t, clip_r, clip_b);

	int32_t scrollOffset = clip_b - env.halfHeight - 14;
	flush_inputs ();
	SHOW_MOUSE(nullptr)

	do {
		if (global.isCloseBtnPressed())
			done = true;

		if (++tOffset >= INT_MAX)
			tOffset = 0;
		if (moving)
			scrollOffset--;

		if (scrollOffset < -(env.halfHeight - 100 + lines->Lines() * 30) )
			scrollOffset = env.halfHeight - 100;
		else if (scrollOffset > (clip_b - env.halfHeight - 14))
			scrollOffset = clip_b - env.halfHeight - 14;

		drawMenuBackground (bgType, tOffset, numItems);
		lines->Render_Lines(scrollOffset, spacing, clip_t, clip_b);
		global.make_update(env.halfWidth - 300, env.menuBeginY,
							601, env.screenHeight - 2 * env.menuBeginY);
		global.do_updates();
		LINUX_REST;

		if ( keypressed() ) {
			key = (readkey()) >> 8;
			switch (key) {
				case KEY_ESC:
					done = true;
					break;
				case KEY_SPACE:
					moving = true;
					break;
				case KEY_UP:
					scrollOffset += 2;
					moving = false;
					break;
				case KEY_DOWN:
					scrollOffset -= 2;
					moving = false;
					break;
			}      // end of switch
		}       // end of key pressed

		if (mouse_b)
			done = true;
	} while (!done);

	SHOW_MOUSE(global.canvas)
	set_clip_rect (global.canvas, 0, 0, (env.screenWidth-1), (env.screenHeight-1));
	flush_inputs ();
}


/* Flush key buffer and waits for button releases */
void flush_inputs()
{
	do { std::this_thread::yield(); } while (mouse_b);
	clear_keybuf();
}


// This file loads weapons, naturals and items
// from a text file
// Returns TRUE on success and FALSE on failure
bool Load_Weapons_Text()
{
	// Be sure that numbers are understood right:
	const char* cur_lc_numeric = setlocale(LC_NUMERIC, "C");

	// get path name
	if (env.language == EL_ENGLISH)
		snprintf(path_buf, PATH_MAX, "%s/text/weapons.txt", env.dataDir);
	else if (env.language == EL_PORTUGUESE)
		snprintf(path_buf, PATH_MAX, "%s/text/weapons.pt_BR.txt", env.dataDir);
	else if (env.language == EL_FRENCH)
		snprintf(path_buf, PATH_MAX, "%s/text/weapons_fr.txt", env.dataDir);
	else if (env.language == EL_GERMAN)
		snprintf(path_buf, PATH_MAX, "%s/text/weapons_de.txt", env.dataDir);
	else if (env.language == EL_SLOVAK)
		snprintf(path_buf, PATH_MAX, "%s/text/weapons_sk.txt", env.dataDir);
	else if (env.language == EL_RUSSIAN)
		snprintf(path_buf, PATH_MAX, "%s/text/weapons_ru.txt", env.dataDir);
	else if (env.language == EL_SPANISH)
		snprintf(path_buf, PATH_MAX, "%s/text/weapons_ES.txt", env.dataDir);
	else if (env.language == EL_ITALIAN)
		snprintf(path_buf, PATH_MAX, "%s/text/weapons_it.txt", env.dataDir);

	// open file
	FILE* wfile = fopen(path_buf, "r");

	if (! wfile) {
		printf( "Unable to open weapons file. (%s)\n", path_buf);
		return false;
	}

	// read line
	char       line[512]     = { 0 };
	char*      status        = fgets(line, 512, wfile);
	eFileStage file_stage    = FS_WEAPONS; // weapons, naturals, items
	eDataStage data_stage    = DS_NAME;    // name, description, data
	int32_t    item_count    = 0;
	int32_t    weapon_count  = 0;
	int32_t    natural_count = 0;

	while (status) {
		// clear end of line
		if ( strchr(line, '\n') )
			strchr(line, '\n')[0] = '\0';
		if ( strchr(line, '\r') )
			strchr(line, '\r')[0] = '\0';

		// skip # and empty lines
		if ( (! (line[0] == '#') ) && ( strlen(line) > 2 ) ) {

			// check for header
			if (!strcasecmp(line, "*WEAPONS*") ) {
				file_stage = FS_WEAPONS;
				data_stage = DS_NAME;
			} else if (!strcasecmp(line, "*NATURALS*") ) {
				file_stage = FS_NATURALS;
				data_stage = DS_NAME;
			} else if (!strcasecmp(line, "*ITEMS*") ) {
				file_stage = FS_ITEMS;
				data_stage = DS_NAME;
			}

			// not a special line, let's read some data
			else {
				// =============
				// == Weapons ==
				// =============
				if ( (FS_WEAPONS == file_stage) && (weapon_count < WEAPONS) ) {
					if (DS_NAME == data_stage)
						weapon[weapon_count].setName(line);
					else if (DS_DESC == data_stage)
						weapon[weapon_count].setDesc(line);
					else if (DS_DATA == data_stage) {
						sscanf(line, "%d %d %lf %lf %d %d %d %d %d %d %d %d %d %d %d %d %lf %d %lf %lf %lf %d %lf",
						       &(weapon[weapon_count].cost),
						       &(weapon[weapon_count].amt),
						       &(weapon[weapon_count].mass),
						       &(weapon[weapon_count].drag),
						       &(weapon[weapon_count].radius),
						       &(weapon[weapon_count].sound),
						       &(weapon[weapon_count].etime),
						       &(weapon[weapon_count].damage),
						       &(weapon[weapon_count].picpoint),
						       &(weapon[weapon_count].spread),
						       &(weapon[weapon_count].delay),
						       &(weapon[weapon_count].noimpact),
						       &(weapon[weapon_count].techLevel),
						       &(weapon[weapon_count].warhead),
						       &(weapon[weapon_count].numSubmunitions),
						       &(weapon[weapon_count].submunition),
						       &(weapon[weapon_count].impartVelocity),
						       &(weapon[weapon_count].divergence),
						       &(weapon[weapon_count].spreadVariation),
						       &(weapon[weapon_count].launchSpeed),
						       &(weapon[weapon_count].speedVariation),
						       &(weapon[weapon_count].countdown),
						       &(weapon[weapon_count].countVariation) );
					}

					// Advance data stage
					++data_stage;
					if ( (DS_NAME == data_stage) // flipped
					  || ( (DS_DATA    == data_stage)
					    && (EL_ENGLISH != env.language)) ) {
						data_stage = DS_NAME;
						weapon_count++;
					}
				}       // end of weapon section

				// ==============
				// == Naturals ==
				// ==============
				else if ( (FS_NATURALS == file_stage) && (natural_count < NATURALS) ) {
					if (DS_NAME == data_stage)
						naturals[natural_count].setName(line);
					else if (DS_DESC == data_stage)
						naturals[natural_count].setDesc(line);
					else if (DS_DATA == data_stage) {
						sscanf(line, "%d %d %lf %lf %d %d %d %d %d %d %d %d %d %d %d %d %lf %d %lf %lf %lf %d %lf",
						       &(naturals[natural_count].cost),
						       &(naturals[natural_count].amt),
						       &(naturals[natural_count].mass),
						       &(naturals[natural_count].drag),
						       &(naturals[natural_count].radius),
						       &(naturals[natural_count].sound),
						       &(naturals[natural_count].etime),
						       &(naturals[natural_count].damage),
						       &(naturals[natural_count].picpoint),
						       &(naturals[natural_count].spread),
						       &(naturals[natural_count].delay),
						       &(naturals[natural_count].noimpact),
						       &(naturals[natural_count].techLevel),
						       &(naturals[natural_count].warhead),
						       &(naturals[natural_count].numSubmunitions),
						       &(naturals[natural_count].submunition),
						       &(naturals[natural_count].impartVelocity),
						       &(naturals[natural_count].divergence),
						       &(naturals[natural_count].spreadVariation),
						       &(naturals[natural_count].launchSpeed),
						       &(naturals[natural_count].speedVariation),
						       &(naturals[natural_count].countdown),
						       &(naturals[natural_count].countVariation) );
					}

					// Advance data stage
					++data_stage;
					if ( (DS_NAME == data_stage) // flipped
					  || ( (DS_DATA    == data_stage)
					    && (EL_ENGLISH != env.language)) ) {
						data_stage = DS_NAME;
						natural_count++;
					}

				}       // end of naturals

				// ==============
				// == Items ==
				// ==============
				else if ( (FS_ITEMS == file_stage) && (item_count < ITEMS) ) {
					if (DS_NAME == data_stage)
						item[item_count].setName(line);
					else if (DS_DESC == data_stage)
						item[item_count].setDesc(line);
					else if (DS_DATA == data_stage) {
						sscanf(line, "%d %d %d %d %d %lf %lf %lf %lf %lf %lf",
						       &(item[item_count].cost),
						       &(item[item_count].amt),
						       &(item[item_count].selectable),
						       &(item[item_count].techLevel),
						       &(item[item_count].sound),
						       &(item[item_count].vals[0]),
						       &(item[item_count].vals[1]),
						       &(item[item_count].vals[2]),
						       &(item[item_count].vals[3]),
						       &(item[item_count].vals[4]),
						       &(item[item_count].vals[5]) );
					}

					// Advance data stage
					++data_stage;
					if ( (DS_NAME == data_stage) // flipped
					  || ( (DS_DATA    == data_stage)
					    && (EL_ENGLISH != env.language)) ) {
						data_stage = DS_NAME;
						item_count++;
					}
				}         // end of items
			}       // end of reading data from a valid line
		}     // end of valid line

		// read in data
		status = fgets(line, 512, wfile);
	} // end while(status)


	// close file
	if (wfile)
		fclose(wfile);


	// Revert locale settings
	if (cur_lc_numeric)
		setlocale(LC_NUMERIC, cur_lc_numeric);
	else
		setlocale(LC_NUMERIC, "");

	return true;
}




/*
Filter out files that do not have .sav in the name.
*/
#ifndef MACOSX
int Filter_File( const struct dirent *my_file )
#else
int Filter_File( struct dirent *my_file )
#endif
{
	if ( strstr(my_file->d_name, ".sav") )
		return true;
	else
		return false;
}

/*
This function finds a list of saved games on your profile.
On error, NULL is returned. If all goes well, a list of file names
are returned.
After use, the return value *must* be freed.
*/
#if defined(ATANKS_IS_LINUX)
dirent** Find_Saved_Games(uint32_t &num_files_found)
{
	dirent** my_list = nullptr;
	int32_t  status  = 0;

	status = scandir(env.configDir, &my_list, Filter_File, alphasort);
	if (status < 0) {
		printf("Error trying to find saved games.\n");
		return nullptr;
	}

	num_files_found = status;

	return my_list;
}
#endif //Linux


/*
This function hunts for saved games. If games are found, the
function returns an array of filenames. If an error occures
or no files are found, NULL is returned.
*/
#if defined(ATANKS_IS_WINDOWS)
dirent** Find_Saved_Games(uint32_t &num_files_found)
{
	dirent** my_list    = (dirent**)calloc(256, sizeof(dirent*));
	dirent*  one_file   = nullptr;
	uint32_t file_count = 0;
	DIR*     game_dir   = nullptr;

	if (!my_list)
		return nullptr;

	game_dir = opendir(env.configDir);
	if (!game_dir) {
		free(my_list);
		return nullptr;
	}

	while ( (one_file = readdir(game_dir)) && (file_count < 256) ) {
		// check to see if this is a save game file
		if ( strstr( one_file->d_name, ".sav" ) ) {
			my_list[file_count] = (dirent*)calloc(1, sizeof(dirent) );
			if ( my_list[file_count] ) {
				memcpy(my_list[file_count], one_file, sizeof(dirent));
				my_list[file_count]->d_name = strdup(one_file->d_name);
				file_count++;
			}
		}
	}

	closedir(game_dir);
	num_files_found = file_count;
	return my_list;
}
#endif // ATANKS_IS_WINDOWS


/*
 * This function searches for bitmap files (.bmp) in the config folder.
 * The function returns an array of bitmap file names. If no files
 * are found, or an error occures, then NULL is returned.
 * */
char** Find_Bitmaps(int32_t* bitmaps_found)
{
	char** my_list;
	struct dirent* one_file;
	int32_t file_count = 0;
	DIR* game_dir;

	my_list = (char**)calloc(256, sizeof(char*));
	if (! my_list)
		return nullptr;


	game_dir = opendir(env.configDir);
	if (! game_dir) {
		free(my_list);
		return nullptr;
	}

	one_file = readdir(game_dir);
	while ( (one_file) && (file_count < 256) ) {
		// check to see if this is a save game file
#ifdef ATANKS_IS_LINUX
		if ( strcasestr( one_file->d_name, ".bmp" ) ) {
#else
		if ( (strstr(one_file->d_name, ".bmp"))
		  || (strstr(one_file->d_name, ".BMP")) ) {
#endif // ATANKS_IS_LINUX
			size_t nLen = strlen(env.configDir)
			            + strlen(one_file->d_name) + 16;
			my_list[file_count] = (char*)calloc(nLen + 1, sizeof(char) );
			if ( my_list[file_count] ) {
				snprintf(my_list[file_count], nLen, "%s/%s", env.configDir, one_file->d_name);
				file_count++;
			}
		}

		one_file = readdir(game_dir);
	}

	closedir(game_dir);
	*bitmaps_found = file_count;
	if (file_count < 1) {
		free(my_list);
		my_list = nullptr;
	}

	return my_list;
}

