#include "optionscreens.h"
#include "player.h"
#include "files.h"
#include "sound.h"

// Helper functions to build the sub menus for the options screen
static void build_Physics (Menu &mPhysics,
                           int32_t menuMid, int32_t itemWidth, int32_t itemHeight,
                           int32_t itemPadding, int32_t itemY);
static void build_Weather (Menu &mWeather,
                           int32_t menuMid, int32_t itemWidth, int32_t itemHeight,
                           int32_t itemPadding, int32_t itemY);
static void build_Graphics(Menu &mGraphics,
                           int32_t menuMid, int32_t itemWidth, int32_t itemHeight,
                           int32_t itemPadding, int32_t itemY);
static void build_Money   (Menu &mMoney,
                           int32_t menuMid, int32_t itemWidth, int32_t itemHeight,
                           int32_t itemPadding, int32_t itemY);
static void build_Network (Menu &mNetwork,
                           int32_t menuMid, int32_t itemWidth, int32_t itemHeight,
                           int32_t itemPadding, int32_t itemY);
static void build_Sound   (Menu &mSound,
                           int32_t menuMid, int32_t itemWidth, int32_t itemHeight,
                           int32_t itemPadding, int32_t itemY);

// Helper action function to do direct language changes
#define LANG_SWITCH_TRIGGER 0x0BadCafe
int32_t switch_language(eLanguages* lang, int32_t val);


/** @brief draw the Menu Background
  *
  * Draws a 600x400 centred box, fills it with some random lines or circles.
  * Someday, we should make this more generic; have it take the box dimensions
  * as an input parameter.
**/
void drawMenuBackground (eBackgroundTypes backType, int32_t tOffset, int32_t numItems)
{
	rectfill (global.canvas, env.halfWidth - 300, env.menuBeginY, // 100,
	          env.halfWidth + 300, env.menuEndY, // env.screenHeight - 100,
	          makecol (0, 79, 0));
	rect     (global.canvas, env.halfWidth - 300, env.menuBeginY, // 100,
	          env.halfWidth + 300, env.menuEndY, // env.screenHeight - 100,
	          makecol (128, 255, 128));

	if (BACKGROUND_BLANK == backType)
		return;

	drawing_mode (DRAW_MODE_TRANS, NULL, 0, 0);
	global.current_drawing_mode = DRAW_MODE_TRANS;
	set_trans_blender (0, 0, 0, 15);

	for (int32_t tCount = 0; tCount < numItems; tCount++) {
		int32_t radius = static_cast<int32_t>( (perlin1DPoint (1.0, 5,
		                                   (tOffset * 0.0333) + tCount + 423346,
		                                     0.5, 8) + 1.1) * 20); // [0.1;2.1]
		int32_t xpos = env.halfWidth
		             + static_cast<int32_t>(perlin1DPoint (1.0, 3,
		                                   (tOffset * 0.0166) + tCount + 232662,
		                                     0.3, 3) * (299 - radius));
		int32_t ypos = env.halfHeight
		             + static_cast<int32_t>(perlin1DPoint (1.0, 2,
		                                   (tOffset * 0.0175) + tCount + 42397,
		                                     0.3, 3)
		                                   * (env.halfHeight - env.menuBeginY
		                                      - radius - 1));
		switch (backType) {
			case BACKGROUND_CIRCLE:
				circlefill (global.canvas, xpos, ypos, radius, LIME_GREEN);
				break;
			case BACKGROUND_LINE:
				rectfill (global.canvas, xpos - radius / 2, env.menuBeginY + 1,
				          xpos + radius / 2, env.menuEndY - 1, LIME_GREEN);
				break;
			case BACKGROUND_SQUARE:
				rectfill (global.canvas, xpos - radius, ypos - radius,
				          xpos + radius, ypos + radius, LIME_GREEN);
				break;
			case BACKGROUND_BLANK:
			default:
				break;
		}

	}

	solid_mode();
	global.current_drawing_mode = DRAW_MODE_SOLID;
}


/// @brief Show a screen listing all players allowing to create new and edit existing ones.
void editPlayers ()
{
	/// @todo : Currently the width is fixed on 600. This should be made
	/// more dynamic like the height. Although the height is fixed, too...
	/// However, there is much to do to get an adaptable and good looking menu...

	int32_t optionsRetVal = 0;
	int32_t menuMid       = 300;
	int32_t itemHeight    = env.fontHeight + 2;
	int32_t itemPadding   = 2;
	int32_t itemY         = (itemHeight + itemPadding) * 2;
	int32_t btnHeight     = env.misc[7]->h + itemPadding;
	int32_t menuHeight    = env.menuEndY - env.menuBeginY; // Raw height
	int32_t plListHeight  = menuHeight - itemY             // Top area reserved for the title
	                      - btnHeight - itemPadding - 2;   // Bottom area reserved for buttons
	// "Select Players"
	Menu menu(MC_PLAYERS, env.halfWidth - menuMid, env.menuBeginY);

	// "Create New"
	PLAYER* player_new = nullptr;
	int32_t first_idx = menu.addMenu(&player_new, new_player, 1,
	                                 menuMid - 53, itemY,
	                                 100, itemHeight, itemPadding);
	itemY += itemHeight + itemPadding;

	// Add one entry per player
	// One Menu per player:
	// Add one edit option per player
	int32_t last_idx  = first_idx;
	int32_t max_width = 100;
	plListHeight -= itemY; // this is left.

	// First loop to determine maximum name width
	for (int32_t num = 0; num < env.numPermanentPlayers; num++) {
		int32_t xLen = text_length(font, env.allPlayers[num]->getName());
		if (xLen > max_width)
			max_width = xLen;
	}

	// Now really add them
	for (int32_t num = 0; num < env.numPermanentPlayers; num++)
		last_idx = menu.addMenu(&env.allPlayers[num], edit_player, -1,
		                        0, 0, max_width + 15, itemHeight, itemPadding);
	// last_idx is one too high now
	last_idx--;

	// Distribute the player list:
	menu.distribute(first_idx, last_idx, menuMid * 2, plListHeight, itemY, false);

	// Add "back" button
	menu.addButton( 2, nullptr, KEY_ESC,
	                env.misc[7], nullptr, env.misc[8], false,
	                menuMid - (env.misc[7]->w / 2),
	                menuHeight - btnHeight - itemPadding - 2,
	                0, 0, 2);

	while ( (KEY_ESC != optionsRetVal)
	     && !global.isCloseBtnPressed() ) {
		optionsRetVal = menu();

		// Was an edit confirmed?
		if (PE_CONFIRM_EDIT & optionsRetVal)
			menu.redraw(optionsRetVal ^ PE_CONFIRM_EDIT, true);
		else if (PE_CONFIRM_DEL & optionsRetVal) {
			// delete the menu entry
			int32_t num = optionsRetVal ^ PE_CONFIRM_DEL;
			menu.delete_entry(num + first_idx);
			--last_idx;

			// The player has to be deleted, too:
			PLAYER* to_delete = env.allPlayers[num];
			env.deletePermPlayer(to_delete);

			// redistribute the remaining:
			menu.distribute(first_idx, last_idx, menuMid * 2, plListHeight, itemY, true);

			optionsRetVal = 0;
		} else if (PE_CONFIRM_NEW & optionsRetVal) {
			if (player_new) {
				// Add a menu entry for the new player:
				menu.addMenu(&env.allPlayers[env.numPermanentPlayers - 1],
				             edit_player, -1, 0, 0, max_width + 15, itemHeight,
				             itemPadding);
				// Move the new menu entry up, the "back" button is in the way
				menu.move_entry(last_idx + 2, last_idx + 1);

				// And re-distribute
				menu.distribute(first_idx, ++last_idx, menuMid * 2,
				                plListHeight, itemY, true);
			}
			optionsRetVal = 0;
		}
	}
}


/// @brief The main options menu
void optionsMenu ()
{
	/// @todo : Currently the width is fixed on 600. This should be made
	/// more dynamic like the height. Although the height is fixed, too...
	/// However, there is much to do to get an adaptable and good looking menu...

	int32_t optionsRetCode = 0;
	int32_t menuMid        = 300;
	int32_t itemWidth      = 210;
	int32_t itemHeight     = env.fontHeight + 2;
	int32_t itemPadding    = 2;
	int32_t itemFullHeight = itemHeight + itemPadding;
	int32_t itemY          = itemFullHeight * 3;
	int32_t btnHeight      = env.misc[7]->h + itemPadding;
	int32_t menuHeight     = env.menuEndY - env.menuBeginY; // Raw height
	int32_t menuLeft       = env.halfWidth - menuMid;
	int32_t menuTop        = env.menuBeginY;
	int32_t idx            = 1;

	Menu mMain    (MC_MAIN,     menuLeft, menuTop);
	Menu mPhysics (MC_PHYSICS,  menuLeft, menuTop);
	Menu mWeather (MC_WEATHER,  menuLeft, menuTop);
	Menu mGraphics(MC_GRAPHICS, menuLeft, menuTop);
	Menu mMoney   (MC_FINANCE,  menuLeft, menuTop);
	Menu mNetwork (MC_NETWORK,  menuLeft, menuTop);
	Menu mSound   (MC_SOUND,    menuLeft, menuTop);

	// As the sub menus must be attached to the main options menu,
	// they have to be build first, before the main menu can be built.
	build_Sound   (mSound,    menuMid, itemWidth, itemHeight, itemPadding, itemY);
	build_Network (mNetwork,  menuMid, itemWidth, itemHeight, itemPadding, itemY);
	build_Money   (mMoney,    menuMid, itemWidth, itemHeight, itemPadding, itemY);
	build_Graphics(mGraphics, menuMid, itemWidth, itemHeight, itemPadding, itemY);
	build_Weather (mWeather,  menuMid, itemWidth, itemHeight, itemPadding, itemY);
	build_Physics (mPhysics,  menuMid, itemWidth, itemHeight, itemPadding, itemY);

	// Now the main options screen can be built:

	// "Reset All"
	Menu mReset(MC_RESET, menuLeft, menuTop);
	mReset.addButton( 1, nullptr, RO_RESET,
	                  env.misc[7], nullptr, env.misc[8], false,
	                  menuMid + 50,
	                  menuHeight- btnHeight - 6, 0, 0, itemPadding);
	mReset.addButton( 2, nullptr, RO_BACK,
	                  env.misc[7], nullptr, env.misc[8], false,
	                  menuMid - env.misc[7]->w - 50,
	                  menuHeight- btnHeight - 6, 0, 0, itemPadding);

	// "Reset options"
	mMain.addMenu   ( &mReset, idx++, RED, menuMid - (env.misc[7]->w / 2), itemY,
	                  150, itemFullHeight, itemPadding);
	itemY += btnHeight + itemPadding;

	// "Physics"
	mMain.addMenu(&mPhysics, idx++, WHITE, menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Weather"
	mMain.addMenu(&mWeather, idx++, WHITE, menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Graphics"
	mMain.addMenu(&mGraphics, idx++, WHITE, menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Money"
	mMain.addMenu(&mMoney, idx++, WHITE, menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Network"
	mMain.addMenu(&mNetwork, idx++, WHITE, menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Sound"
	mMain.addMenu(&mSound, idx++, WHITE, menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Weapon Tech Level"
	mMain.addValue(&env.weapontechLevel, idx++, WHITE, 0, 5, 1, "%d",
	               menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Item Tech Level"
	mMain.addValue(&env.itemtechLevel, idx++, WHITE, 0, 5, 1, "%d",
	               menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Landscape"
	mMain.addValue(&env.landType, idx++, nullptr, WHITE,
	               TC_LANDTYPE, static_cast<int32_t>(LAND_NONE),
	               menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Turn Order"
	mMain.addValue(&env.turntype, idx++, nullptr, WHITE,
	               TC_TURNTYPE, static_cast<int32_t>(TURN_SIMUL),
	               menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Skip AI-only play"
	mMain.addValue(&env.skipComputerPlay, idx++, nullptr, WHITE,
	               TC_SKIPTYPE, static_cast<int32_t>(SKIP_HUMANS_DEAD),
	               menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Show FPS"
	mMain.addValue(&env.showFPS, idx++, nullptr, WHITE,
	               TC_OFFON, 1,
	               menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Language"
	mMain.addValue(&env.language, switch_language, idx++, nullptr, WHITE,
	               TC_LANGUAGE, static_cast<int32_t>(EL_ITALIAN),
	               menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);

	// "Back"
	mMain.addButton(idx, nullptr, KEY_ESC,
	                env.misc[7], nullptr, env.misc[8], false,
	                menuMid - (env.misc[7]->w / 2),
	                menuHeight - btnHeight - itemPadding - 2,
	                0, 0, 2);

	// Safe the current language, if a new one is selected,
	// the text files must be reloaded.
	eLanguages cur_lang = env.language;

	while (0 == optionsRetCode) {
		int32_t old_fps = env.frames_per_second;

		optionsRetCode = mMain();

		if (RO_RESET == optionsRetCode) {
			env.Reset_Options();
			optionsRetCode = 0;
		} else if (RO_BACK == optionsRetCode)
			optionsRetCode = 0;
		else if (LANG_SWITCH_TRIGGER == optionsRetCode) {
			mMain.setLanguage(true);
			optionsRetCode = 0;
		}

		// If no exit code is set, redraw the menu:
		if (0 == optionsRetCode)
			mMain.redrawAll(true);

		// Update pre-calculated values if FPS has been changed:
		if (old_fps != env.frames_per_second)
			env.set_fps(0); // 0 triggers re-calculation only
	} // End of menu loop

	// Did the language change?
	if (env.language != cur_lang) {
		env.load_text_files();
		Load_Weapons_Text();
	}
}


/// @brief Show a screen that shows the preparations to create a new game.
int32_t selectPlayers ()
{
	/// @todo : Currently the width is fixed on 600. This should be made
	/// more dynamic like the height. Although the height is fixed, too...
	/// However, there is much to do to get an adaptable and good looking menu...

	int32_t optionsRetVal  = 0;
	int32_t menuMid        = 300;
	int32_t itemWidth      = 120;
	int32_t itemHeight     = env.fontHeight + 4;
	int32_t itemPadding    = 2;
	int32_t itemFullHeight = itemHeight + itemPadding;
	int32_t itemY          = itemFullHeight * 2;
	int32_t btnHeight      = env.misc[7]->h + itemPadding;
	int32_t menuHeight     = env.menuEndY - env.menuBeginY; // Raw height
	int32_t plListHeight   = menuHeight - itemY           // Top area reserved for the title
	                       - btnHeight - itemPadding - 2; // Bottom area reserved for buttons
	int32_t idx            = 1;

	uint32_t number_saved_games = 0;
	dirent** saved_game_names;
	char**   game_list = NULL;

	// Use new menu system:
	// "Select Players"
	Menu menu(MC_PLAY, env.halfWidth - menuMid, env.menuBeginY);

	// "Rounds"
	menu.addValue(&env.rounds, idx++, BLACK, 1, MAX_ROUNDS, 1, "%u",
	              menuMid - (itemWidth / 2), itemY, itemWidth, itemHeight,
	              itemPadding);
	itemY += itemFullHeight + 2;

	// "New Game Name"
	strncpy(env.game_name, "New Game", GAMENAMELEN);
	menu.addText(env.game_name, idx++, GAMENAMELEN, BLACK, "%s",
	             menuMid - (itemWidth / 2), itemY, itemWidth, itemHeight,
	             itemPadding);
	itemY += itemFullHeight + 2;

	// find saved games
	saved_game_names = Find_Saved_Games(number_saved_games);

	if ( ( saved_game_names ) && ( number_saved_games ) ) {

		// Extend the global list if it is too small
		if (env.saved_game_list_size <= number_saved_games) {
			// Note: The number of saved games in ENVRIONMENT must be
			// one entry larger, as the text array for the option menu
			// has to be terminated by a null pointer.
			game_list = (char**)realloc(env.saved_game_list,
			                            sizeof(char*) * (number_saved_games + 1));
			if (game_list) {
				// The old names must be freed and the new initialized
				for (uint32_t i = 0; i <= number_saved_games ; ++i) {
					if ( ( i < env.saved_game_list_size ) && game_list[i] )
						free(game_list[i]);
					game_list[i] = nullptr;
				}

				env.saved_game_list = const_cast<const char**>(game_list);
				env.saved_game_list_size = number_saved_games + 1;
			} else {
				cerr << "ERROR extending saved game list from ";
				cerr << env.saved_game_list_size << " entries to ";
				cerr << number_saved_games << " entries." << endl;
				free (saved_game_names);
				return KEY_ESC;
			}
		} else
			game_list = const_cast<char**>(env.saved_game_list);
		// End of preparing space for the saved game list

		// Copy the found names (Without the extra entry of course)
		for (uint32_t i = 0; i < number_saved_games; ++i) {

			// If the name is already set, free it, it was strdup'd
			if ( game_list[i] )
				free(game_list[i]);
			game_list[i] = strdup(saved_game_names[i]->d_name);

			// clear trailing extension
			if ( strchr(game_list[i], '.') )
				strchr(game_list[i], '.')[0] = '\0';
		}

		// set up menu for selecting saved games
		// "or Load Game"
		env.saved_gameindex = 0;
		menu.addValue(&env.saved_gameindex, idx++, env.saved_game_list,
		              BLACK, TC_FREETEXT, number_saved_games - 1,
		              menuMid - (itemWidth / 2), itemY, itemWidth, itemHeight,
					  itemPadding);
		itemY += itemFullHeight + 4; // Next two options need more height

		// "Load Game"
		menu.addToggle(&env.loadGame, idx++, WHITE,
					menuMid - 125, itemY, 100, itemHeight + 5, itemPadding);
		// itemY stays, "Campaign" is on the same row.
	} // End of having saved games

	// If no saved games are there, idx must be advanced nevertheless or
	// the texts go mixed up:
	else
		idx += 2;


	// "Campaign"
	// Note: And save the result, it is the first player index
	int32_t first_idx = menu.addToggle(&env.campaign_mode, idx++, WHITE,
	                                   menuMid + 25, itemY, 100, itemHeight + 5,
	                                   itemPadding);
	itemY += itemFullHeight + 7; // ET_TOGGLE needs more height

	// Add one entry per player
	int32_t last_idx  = first_idx;
	int32_t max_width = 100;
	plListHeight -= itemY; // this is left.

	// First loop to determine maximum name width
	for (int32_t num = 0; num < env.numPermanentPlayers; num++) {
		int32_t xLen = text_length(font, env.allPlayers[num]->getName())
		             + (2 * itemPadding);
		if (xLen > max_width)
			max_width = xLen;
	}

	// Now really add them
	for (int32_t num = 0; num < env.numPermanentPlayers; num++)
		last_idx = menu.addToggle(&env.allPlayers[num],
		                          0, 0, max_width + 21, itemHeight, itemPadding);

	// last_idx is one too high now.
	last_idx--;


	// Distribute the player list:
	menu.distribute(first_idx, last_idx, menuMid * 2, plListHeight, itemY, false);

	// The "Okay" and "Back" buttons have their own texts to be translated
	menu.addButton( idx + 1, nullptr, KEY_ESC,
	                env.misc[7], nullptr, env.misc[8], false,
	                menuMid - env.misc[7]->w - 25,
	                menuHeight - btnHeight - itemPadding - 2,
	                0, 0, 2);
	menu.addButton( idx, nullptr, KEY_ENTER,
	                env.misc[7], nullptr, env.misc[8], false,
	                menuMid + 25,
	                menuHeight - btnHeight - itemPadding - 2,
	                0, 0, 2);

	// Let the menu take over user input until the game is either started,
	// or the user opts out to the main menu.
	// Idea for the future:
	// Start the menu as a thread and do some fancy stuff with the background
	// while we are waiting.
	do {
		optionsRetVal = menu();

		if ( env.loadGame ) {
			if ( env.saved_game_list
			  && ( env.saved_gameindex < env.saved_game_list_size )
			  && env.saved_game_list[env.saved_gameindex][0]) {

				strncpy(env.game_name,
				        env.saved_game_list[env.saved_gameindex],
				        GAMENAMELEN);
			}
		}

		if (optionsRetVal == KEY_ENTER) {
			if (env.loadGame) {
				// A set game shall be loaded
				if (Check_For_Saved_Game())
					optionsRetVal = MRC_Load_Game;
				else {
					optionsRetVal = 0;
					errorMessage = env.ingame->Get_Line(39);
					errorX  = env.halfWidth - text_length(font, errorMessage) / 2;
					errorY = env.menuBeginY + itemFullHeight;
				}
			} else {
				// Start a new game
				int32_t playerCount = 0;
				env.numGamePlayers  = 0;

				// Add selected players to the game:
				for (int z = 0; z < env.numPermanentPlayers; z++) {
					if (env.allPlayers[z]->selected) {
						env.addGamePlayer(env.allPlayers[z]);
						playerCount++;
					}
				}

				// Check selected players
				if ((playerCount < 2) || (playerCount > MAXPLAYERS)) {
					if (playerCount < 2)
						errorMessage = env.ingame->Get_Line(8);
					else if (playerCount > MAXPLAYERS)
						errorMessage = env.ingame->Get_Line(9);

					errorX = env.halfWidth - text_length(font, errorMessage) / 2;
					errorY = env.menuBeginY + itemFullHeight;
					optionsRetVal = 0;
				} else
					optionsRetVal = MRC_Play_Game;
			} // End of loading versus starting anew
		} // End of KEY_ENTER result

		// zero means an error occured.
		// keep running the loop until ESC is pressed or a non-zero value appears
	} while ((KEY_ESC       != optionsRetVal)
	      && (MRC_Play_Game != optionsRetVal)
	      && (MRC_Load_Game != optionsRetVal)
	      && !global.isCloseBtnPressed() );

	if ( (KEY_ESC == optionsRetVal)
	  || global.isCloseBtnPressed() )
		optionsRetVal = MRC_Esc_Menu;

	if (saved_game_names) {
		for (uint32_t i = 0; i < number_saved_games; ++i) {
			if ( saved_game_names[i] ) {

#if defined(ATANKS_IS_WINDOWS)
				// Under windows d_name is strdup'd
				if ( saved_game_names[i]->d_name )
					free(saved_game_names[i]->d_name);
#endif // Windows

				free(saved_game_names[i]);
			}
		}
		free(saved_game_names);
	}

	return optionsRetVal;
}


// Helper functions to build the sub menus for the options screen
static void build_Physics(Menu &mPhysics,
                          int32_t menuMid, int32_t itemWidth, int32_t itemHeight,
                          int32_t itemPadding, int32_t itemY)
{
	int32_t itemFullHeight = itemHeight + itemPadding;
	int32_t btnHeight      = env.misc[7]->h + itemPadding;
	int32_t menuHeight     = env.menuEndY - env.menuBeginY; // Raw height
	int32_t idx            = 1;

	assert( (0 == mPhysics.count()) && "ERROR: mPhysics already built?");

	if (mPhysics.count())
		return; // Don't build twice!

	// "Gravity"
	mPhysics.addValue(&env.gravity, idx++, WHITE, .025, .325, .025, "%5.3f",
	                  menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Viscosity",
	mPhysics.addValue(&env.viscosity, idx++, WHITE, .25, 2., .25, "%3.2f",
	                  menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Land Slide"
	mPhysics.addValue(&env.landSlideType, idx++, nullptr, WHITE,
	                  TC_LANDSLIDE, static_cast<int32_t>(SLIDE_CARTOON),
	                  menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Land Slide Delay",
	mPhysics.addValue(&env.landSlideDelay, idx++, WHITE, 1, 5, 1, "%d",
	                  menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;


	// "Wall Type"
	mPhysics.addValue(&env.wallType, idx++, nullptr, WHITE,
	                  TC_WALLTYPE, static_cast<int32_t>(WALL_RANDOM),
	                  menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;


	// "Boxed Mode",
	mPhysics.addValue(&env.boxedMode, idx++, nullptr, WHITE,
	                  TC_OFFONRANDOM, static_cast<int32_t>(BM_RANDOM),
	                  menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;


	// "Boxed Ceiling Wrapping",
	mPhysics.addValue(&env.do_box_wrap, idx++, nullptr, WHITE,
	                  TC_OFFON, static_cast<int32_t>(BM_ON),
	                  menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;


	// "Violent Death"
	mPhysics.addValue(&env.violent_death, idx++, nullptr, WHITE,
	                  TC_LIGHTNING, static_cast<int32_t>(VD_HEAVY),
	                  menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;


	// "Timed Shots"
	mPhysics.addValue(&env.maxFireTime, idx++, WHITE, 0, 180, 5, "%d",
	                  menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Volley Delay"
	mPhysics.addValue(&env.volley_delay, idx++, WHITE, 5, 50, 1, "%d",
	                  menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Explosion Debris"
	mPhysics.addValue(&env.debris_level, idx++, nullptr, WHITE,
	                  TC_LIGHTNING, 3,
	                  menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);

	// "Back" Button
	mPhysics.addButton(idx, nullptr, KEY_ESC,
	                   env.misc[7], nullptr, env.misc[8], false,
	                   menuMid - (env.misc[7]->w / 2),
	                   menuHeight - btnHeight - itemPadding - 2,
	                   0, 0, 2);
}


static void build_Weather(Menu &mWeather,
                          int32_t menuMid, int32_t itemWidth, int32_t itemHeight,
                          int32_t itemPadding, int32_t itemY)
{
	int32_t itemFullHeight = itemHeight + itemPadding;
	int32_t btnHeight      = env.misc[7]->h + itemPadding;
	int32_t menuHeight     = env.menuEndY - env.menuBeginY; // Raw height
	int32_t idx            = 1;

	assert( (0 == mWeather.count()) && "ERROR: mWeather already built?");

	if (mWeather.count())
		return; // Don't build twice!


	// "Meteor Showers"
	mWeather.addValue(&env.meteors, idx++, nullptr, WHITE,
	                  TC_METEOR, 3,
	                  menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Lightning"
	mWeather.addValue(&env.lightning, idx++, nullptr, WHITE,
	                  TC_LIGHTNING, 3,
	                  menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Falling Dirt"
	mWeather.addValue(&env.falling_dirt_balls, idx++, nullptr, WHITE,
	                  TC_METEOR, 3,
	                  menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Laser Satellite"
	mWeather.addValue(&env.satellite, idx++, nullptr, WHITE,
	                  TC_SATELLITE, static_cast<int32_t>(SL_SUPER),
	                  menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Fog"
	mWeather.addValue(&env.fog, idx++, nullptr, WHITE,
	                  TC_OFFON, 1,
	                  menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;


	// "Max Wind Strength"
	mWeather.addValue(&env.windstrength, idx++, WHITE, 0, 100, 5, "%d",
	                  menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;


	// "Wind Variation"
	mWeather.addValue(&env.windvariation, idx++, WHITE, 0, 100, 3, "%d",
	                  menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);

	// "Back"
	mWeather.addButton(idx, nullptr, KEY_ESC,
	                   env.misc[7], nullptr, env.misc[8], false,
	                   menuMid - (env.misc[7]->w / 2),
	                   menuHeight - btnHeight - itemPadding - 2,
	                   0, 0, 2);
}


static void build_Graphics(Menu &mGraphics,
                           int32_t menuMid, int32_t itemWidth, int32_t itemHeight,
                           int32_t itemPadding, int32_t itemY)
{
	int32_t itemFullHeight = itemHeight + itemPadding;
	int32_t btnHeight      = env.misc[7]->h + itemPadding;
	int32_t menuHeight     = env.menuEndY - env.menuBeginY; // Raw height
	int32_t idx            = 1;

	assert( (0 == mGraphics.count()) && "ERROR: mGraphics already built?");

	if (mGraphics.count())
		return; // Don't build twice!

	// "Full Screen"
	mGraphics.addValue(&env.full_screen, idx++, nullptr, WHITE,
	                   TC_OFFON, 1,
	                   menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Dithering"
	mGraphics.addValue(&env.ditherGradients, idx++, nullptr, WHITE,
	                   TC_OFFON, 1,
	                   menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Detailed Land"
	mGraphics.addValue(&env.detailedLandscape, idx++, nullptr, WHITE,
	                   TC_OFFON, 1,
	                   menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Detailed Sky"
	mGraphics.addValue(&env.detailedSky, idx++, nullptr, WHITE,
	                   TC_OFFON, 1,
	                   menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Fading Text"
	mGraphics.addValue(&env.fadingText, idx++, nullptr, WHITE,
	                   TC_OFFON, 1,
	                   menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Shadowed Text"
	mGraphics.addValue(&env.shadowedText, idx++, nullptr, WHITE,
	                   TC_OFFON, 1,
	                   menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Swaying Text"
	mGraphics.addValue(&env.swayingText, idx++, nullptr, WHITE,
	                   TC_OFFON, 1,
	                   menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Colour Theme"
	mGraphics.addValue(&env.colourTheme, idx++, nullptr, WHITE,
	                   TC_COLOUR, static_cast<int32_t>(CT_CRISPY),
	                   menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Screen Width"
	mGraphics.addValue(&env.temp_screenWidth, idx++, WHITE, 800, 2000, 100, "%d",
	                   menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Screen Height"
	mGraphics.addValue(&env.temp_screenHeight, idx++, WHITE, 600, 1400, 100, "%d",
	                   menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Mouse Pointer"
	mGraphics.addValue(&env.osMouse, idx++, nullptr, WHITE,
	                   TC_MOUSE, 1,
	                   menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Game Speed"
	mGraphics.addValue(&env.frames_per_second, idx++, WHITE, 30, 1000, 5, "%d",
	                   menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Custom Background"
	mGraphics.addValue(&env.custom_background, idx++, nullptr, WHITE,
	                   TC_OFFON, 1,
	                   menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Show AI Feedback"
	mGraphics.addValue(&env.showAIFeedback, idx++, nullptr, WHITE,
	                   TC_OFFON, 1,
	                   menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Dynamic Menu Background"
	mGraphics.addValue(&env.dynamicMenuBg, idx++, nullptr, WHITE,
	                   TC_OFFON, 1,
	                   menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);


	// "Back"
	mGraphics.addButton(idx, nullptr, KEY_ESC,
	                env.misc[7], nullptr, env.misc[8], false,
	                menuMid - (env.misc[7]->w / 2),
	                menuHeight - btnHeight - itemPadding - 2,
	                0, 0, 2);
}


static void build_Money(Menu &mMoney,
                        int32_t menuMid, int32_t itemWidth, int32_t itemHeight,
                        int32_t itemPadding, int32_t itemY)
{
	int32_t itemFullHeight = itemHeight + itemPadding;
	int32_t btnHeight      = env.misc[7]->h + itemPadding;
	int32_t menuHeight     = env.menuEndY - env.menuBeginY; // Raw height
	int32_t idx            = 1;

	assert( (0 == mMoney.count()) && "ERROR: mMoney already built?");

	if (mMoney.count())
		return; // Don't build twice!

	// "Starting Money"
	mMoney.addValue(&env.startmoney, idx++, WHITE, 0, 200000, 5000, "%d",
	                menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Interest Rate"
	mMoney.addValue(&env.interest, idx++, WHITE, 1., 1.5, .05, "%3.2f",
	                menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Round Win Bonus"
	mMoney.addValue(&env.scoreRoundWinBonus, idx++, WHITE, 0, 50000, 5000, "%d",
	                menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Damage Bounty"
	mMoney.addValue(&env.scoreHitUnit, idx++, WHITE, 0, 500, 25, "%d",
	                menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Self-Damage Penalty"
	mMoney.addValue(&env.scoreSelfHit, idx++, WHITE, 0, 5000, 25, "%d",
	                menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Team-Damage Penalty"
	mMoney.addValue(&env.scoreTeamHit, idx++, WHITE, 0, 5000, 10, "%d",
	                menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Tank Destruction Bonus"
	mMoney.addValue(&env.scoreUnitDestroyBonus, idx++, WHITE, 0, 20000, 2500, "%d",
	                menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Tank Self-Destruction Penalty"
	mMoney.addValue(&env.scoreUnitSelfDestroy, idx++, WHITE, 0, 20000, 2500, "%d",
	                menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Item Sell Multiplier"
	mMoney.addValue(&env.sellpercent, idx++, WHITE, 0., 1., .1, "%2.2f",
	                menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Teams Share"
	mMoney.addValue(&env.divide_money, idx++, nullptr, WHITE,
	                TC_OFFON, 1,
	                menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);

	// "Back"
	mMoney.addButton(idx, nullptr, KEY_ESC,
	                env.misc[7], nullptr, env.misc[8], false,
	                menuMid - (env.misc[7]->w / 2),
	                menuHeight - btnHeight - itemPadding - 2,
	                0, 0, 2);
}


static void build_Network(Menu &mNetwork,
                          int32_t menuMid, int32_t itemWidth, int32_t itemHeight,
                          int32_t itemPadding, int32_t itemY)
{
	int32_t itemFullHeight = itemHeight + itemPadding;
	int32_t btnHeight      = env.misc[7]->h + itemPadding;
	int32_t menuHeight     = env.menuEndY - env.menuBeginY; // Raw height
	int32_t idx            = 1;

	assert( (0 == mNetwork.count()) && "ERROR: mNetwork already built?");

	if (mNetwork.count())
		return; // Don't build twice!


	// "Check Updates"
	mNetwork.addValue(&env.check_for_updates, idx++, nullptr, WHITE,
	                  TC_OFFON, 1,
	                  menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Networking"
	mNetwork.addValue(&env.network_enabled, idx++, nullptr, WHITE,
	                  TC_OFFON, 1,
	                  menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Listen Port"
	mNetwork.addValue(&env.network_port, idx++, WHITE, 10645, 64645, 1000, "%d",
	                  menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Server Address"
	mNetwork.addText(env.server_name, idx++, 127, WHITE, "%s",
	                 menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Server Port"
	mNetwork.addText(env.server_port, idx++, 127, WHITE, "%s",
	                 menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);

	// "Back"
	mNetwork.addButton(idx, nullptr, KEY_ESC,
	                env.misc[7], nullptr, env.misc[8], false,
	                menuMid - (env.misc[7]->w / 2),
	                menuHeight - btnHeight - itemPadding - 2,
	                0, 0, 2);
}


static void build_Sound(Menu &mSound,
                        int32_t menuMid, int32_t itemWidth, int32_t itemHeight,
                        int32_t itemPadding, int32_t itemY)
{
	int32_t itemFullHeight = itemHeight + itemPadding;
	int32_t btnHeight      = env.misc[7]->h + itemPadding;
	int32_t menuHeight     = env.menuEndY - env.menuBeginY; // Raw height
	int32_t idx            = 1;

	assert( (0 == mSound.count()) && "ERROR: mSound already built?");

	if (mSound.count())
		return; // Don't build twice!

	// "All Sound"
	mSound.addValue(&env.sound_enabled, idx++, nullptr, WHITE,
	                TC_OFFON, 1,
	                menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Sound Driver"
	mSound.addValue(&env.sound_driver, idx++, nullptr, WHITE,
	                TC_SOUNDDRIVER, static_cast<int32_t>(SD_JACK),
	                menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Music"
	mSound.addValue(&env.play_music, idx++, nullptr, WHITE,
	                TC_OFFON, 1,
	                menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Volume Faktor"
	mSound.addValue(&env.volume_factor, idx++, WHITE, 0, MAX_VOLUME_FACTOR, 1, "%d",
	                menuMid - 50, itemY, itemWidth, itemHeight, itemPadding);

	// "Back"
	mSound.addButton(idx, nullptr, KEY_ESC,
	                env.misc[7], nullptr, env.misc[8], false,
	                menuMid - (env.misc[7]->w / 2),
	                menuHeight - btnHeight - itemPadding - 2,
	                0, 0, 2);
}


/// @brief Switch language helper function
/// Note: The real use of this function is, that it generates a return
///       code, so optionsMenu() can react on the language change. ;-)
int32_t switch_language(eLanguages* lang, int32_t val)
{
    eLanguages old_lang = *lang;

    if (val > 0)
		++(*lang);
	else if (val < 0)
		--(*lang);

	if (*lang != old_lang)
		return LANG_SWITCH_TRIGGER;
	return 0;
}

