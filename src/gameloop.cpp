#include <cstdio>
#include <thread>
#include <condition_variable>
#include <cassert>

#include "main.h"
#include "files.h"
#include "satellite.h"
#include "update.h"
#include "network.h"
#include "land.h"
#include "clock.h"

#include "floattext.h"
#include "tank.h"
#include "explosion.h"
#include "beam.h"
#include "missile.h"
#include "decor.h"
#include "teleport.h"
#include "sky.h"
#include "sound.h"

#include "gameloop.h"
#include "player.h"
#include "aicore.h"

#include "shop.h"

/// Forwarding for function arguments
class ObjectUpdater;

/// === Helper functions ===
static inline bool   advance_tank        ();
static inline void   change_wind_strength();
static inline void   check_fps           (ObjectUpdater* upd);
static inline void   check_overtime      (AICore &aicore);
static inline void   check_skiptime      ();
static inline void   clear_voices        ();
static inline void   check_winner        ();
static inline double colorDistance       (int32_t col1, int32_t col2);
static inline void   delete_destroyed    (AICore &aicore);
static inline void   do_naturals         ();
static inline void   draw_FPS_Counter    ();
static inline void   draw_objects        (AICore &aicore);
static inline void   draw_eor_scoreboard (); // The [e]nd-[o]f-[r]ound score board
static inline void   draw_mini_scoreboard(); // The ingame mini score board
              void   draw_top_bar        ();
static inline bool   explode_tanks       ();
static inline void   fire_weapon         ();
static inline void   graph_bar           (int32_t x, int32_t y, int32_t col,
                                   int32_t actual, int32_t max);
static inline void   graph_bar_center    (int32_t x, int32_t y, int32_t col,
                                   int32_t actual, int32_t max);
static inline void   init_new_round      ();
static inline bool   manage_input        (AICore &aicore);
static inline void   set_level_settings  (LevelCreator* lcr);
static inline void   set_tank_settings   ();
static inline void   update_display      ();
static inline void   update_objects      (ObjectUpdater* upd);


/// === Static helper values ===
static int32_t AI_time_change  = 0;
static TANK*   curr_tank       = nullptr;
static bool    death_substitute= false;
static bool    fire            = false;
static int32_t FPS_counter     = 0;
static int32_t FPS_last        = 0;
static int32_t FPS_pos         = 0;
static int32_t game_us_needed  = 0;
volatile
static abool_t has_action      = ATOMIC_VAR_INIT(false);
volatile
static abool_t has_deco        = ATOMIC_VAR_INIT(false);
volatile
static abool_t has_explosion   = ATOMIC_VAR_INIT(false);
static int32_t human_players   = 0;
static TANK*   next_tank       = nullptr;
static bool    order_wrapped   = false;
static int32_t score_name_pos  = 0;
static int32_t score_money_pos = 0;
volatile
static bool    second_passed   = false;
volatile
static bool    show_frame      = true;
static int32_t skip_health     = 0;
static int32_t smkIdx          = -1;
static bool    update_screen   = true;
static int32_t us_per_frame    = 0;
volatile
static int32_t winner          = WINNER_NO_WIN;


// Note: Here mutexes must be used, condition_variable
// can not use anything else.
std::mutex              updMutex;
std::condition_variable updCondition;


/// Helper Class to multi-thread object updating
class ObjectUpdater
{
	eClasses class_    = CLASS_COUNT;
	abool_t  doExit;
	abool_t  doStart;
	abool_t  isDone;
	abool_t  isExited;
	int32_t  force_age = 0; // Only used for CLASS_DECOR_SMOKE to force more aging

public:

	explicit ObjectUpdater() :
		doExit  (ATOMIC_VAR_INIT(false)),
		doStart (ATOMIC_VAR_INIT(false)),
		isDone  (ATOMIC_VAR_INIT(false)),
		isExited(ATOMIC_VAR_INIT(false))
	{ /* nothing to see here */ }

	/// @brief the main thread handler.
	void operator()()
	{
		vobj_t* next_obj = nullptr;
		vobj_t* obj      = nullptr;
		TANK*   tmp_tank = nullptr;


		// The thread is valid until someone tells it to exit
		while (!doExit.load()) {

			// Sleep until called
			std::unique_lock<std::mutex> updLock(updMutex);
			updCondition.wait(updLock, [this]{
				return (doStart.load(ATOMIC_READ) || doExit.load(ATOMIC_READ));
			} );

			// Early quit if this is a call to do so:
			if (doExit.load())
				continue;

			// If this is the TANK class, yield once if
			// there is no known explosion, yet.
			if ((CLASS_TANK == class_) && (false == has_explosion.load()) )
				// Note: No argument to load(), use the most strict default!
				std::this_thread::yield();


			// Okay, do the updating for this class:
			global.getHeadOfClass(static_cast<eClasses>(class_), &obj);

			// If this is the floating text class, lock it, or AI
			// feedback might lead to data races.
			if (CLASS_FLOATTEXT == class_)
				global.lockClass(class_);

			while (obj) {

				// Explosions must be known at once:
				if ( (false == has_explosion.load(ATOMIC_READ))
				  && (CLASS_EXPLOSION == class_) ) {
					has_explosion.store(true);
					has_action.store(true);
				}

				// Make sure we know when stuff is happening!
				if ( (false == has_action.load(ATOMIC_READ))
				  && ( ( ( (CLASS_BEAM      == class_)
				        || (CLASS_MISSILE   == class_) )
				      && static_cast<PHYSICAL_OBJECT*>(obj)->isWeapon() )
				    || (CLASS_TELEPORT  == class_) ) )
					has_action.store(true);

				obj->getNext(&next_obj);

				// Trigger Explosion progress
				if (CLASS_EXPLOSION == class_)
					static_cast<EXPLOSION*>(obj)->explode();

				// Apply forced smoke ageing
				if ( (CLASS_DECOR_SMOKE == class_) && force_age)
					static_cast<DECOR*>(obj)->force_aging(force_age);

				// Do tank special handling
				if (CLASS_TANK == class_) {
					tmp_tank = static_cast<TANK*>(obj);

					if (!tmp_tank->destroy) {
						// Activate next volley shot if applicable
						if (tmp_tank->fire_another_shot) {
							if (! (tmp_tank->fire_another_shot % env.volley_delay) ) {
								has_action.store(true);
								tmp_tank->activateCurrentSelection();
							}
							tmp_tank->fire_another_shot--;
						}

						// Move and possibly apply pending damage
						tmp_tank->applyPhysics();
						tmp_tank->resetFlashDamage();
						if (tmp_tank->isFlying())
							has_action.store(true);
					}


					// If the tank is still alive, adjust its chess-style clock
					if ( !tmp_tank->destroy
					  && (env.maxFireTime > 0)
					  && (tmp_tank     == curr_tank)
					  && (HUMAN_PLAYER == tmp_tank->player->type)
					  && (STAGE_AIM    == global.stage)
					  && second_passed
					  && tmp_tank->player->reduceClock() ) {
						tmp_tank->player->skip_me = true;
						tmp_tank  = nullptr;
						fire      = false;
						curr_tank = next_tank ? next_tank : global.get_next_tank(&order_wrapped);
						next_tank = nullptr;
						global.set_curr_tank(curr_tank);
					}

					tmp_tank = nullptr;
				} else
					// All others need to apply physics
					obj->applyPhysics();

				obj = next_obj;
			} // End of looping objects of one class

			// If this is the floating text class, unlock it again.
			if (CLASS_FLOATTEXT == class_)
				global.unlockClass(class_);

			// All done
			isDone.store( true,  ATOMIC_WRITE);
			doStart.store(false, ATOMIC_WRITE);
		} // End of while not exiting

		isExited.store(true, ATOMIC_WRITE);
	}

	void finish     ()                 { doExit.store(true); }
	bool hasDone    () const           { return isDone.load(ATOMIC_READ); }
	bool hasExited  () const           { return isExited.load(ATOMIC_READ); }
	void setClass   (eClasses aClass_) { class_ = aClass_; }
	void setForceAge(int32_t ageing_)  { force_age = ageing_; }
	void start      ()                 { isDone.store(false, ATOMIC_WRITE);
	                                     doStart.store(true, ATOMIC_WRITE); }
};


/// The main game loop. Everything happens here.
void game ()
{
	volatile
	bool       done             = false;
	volatile
	int32_t    round_end_count  = 0;
	SATELLITE* satellite        = nullptr;
	const
	int32_t    EndOfRoundFrames = env.frames_per_second * WAIT_AT_END_OF_ROUND;
	AICore     aicore;

	// Check whether the AI Core is in any state to do work:
	if (!aicore.can_work()) {
		perror("The AI core could not be initialized");
		global.set_command(GLOBAL_COMMAND_QUIT);
		return;
	}

	// Initialize the new round
	init_new_round();

	// Only prepare the game if the player did not close
	// the window in the buy screen
	if ( (global.get_command() == GLOBAL_COMMAND_QUIT)
	  || (global.isCloseBtnPressed()) )
		return;

	// Now that everybody has done their shopping, initialize the tanks
	set_tank_settings();

	// Create the AI Core thread
	std::thread aithread(std::ref(aicore));

	// Create one updater thread per object class:
	ObjectUpdater updater[CLASS_COUNT];
	std::thread*  threads[CLASS_COUNT];

	// Set each updater to an individual class, but note the index
	// of the SMOKE. The smoke decoration index is used to force faster
	// ageing when rendering a frame takes too long.
	for (int32_t class_ = 0; class_ < CLASS_COUNT; ++class_) {
		if (CLASS_DECOR_SMOKE == class_)
			smkIdx = class_;

		updater[class_].setClass(static_cast<eClasses>(class_));
		threads[class_] = new std::thread(std::ref(updater[class_]));
	}

	// create satellite
	if (env.satellite)
		satellite = new SATELLITE();

	// get some mood music
	play_music();

	// Final round preparation
	global.AI_clock             = -1;
	global.skippingComputerPlay = false;
	curr_tank                   = global.order[0];
	next_tank                   = nullptr;
	death_substitute            = false;
	global.set_curr_tank(curr_tank);

	WIN_CLOCK_INIT
	game_us_reset();


	/* ==============================================================
	 * ==== init stuff complete. Get down to playing             ====
	 * ==============================================================
	 */

	while ( !done && (global.get_command() != GLOBAL_COMMAND_QUIT) ) {

		// No action and no explosions, yet.
		has_action.store(false);
		has_explosion.store(false);

		// For the chess-style clock and the AI clock in skipping computer
		// play it is necessary to know when a second has passed.
		second_passed = global.check_time_changed();


		// Check overtime and do frame display flipping
		if (global.skippingComputerPlay) {
			if (winner != WINNER_DRAW)
				check_overtime(aicore);

			// End skipping play if the game is over:
			if (global.stage >= STAGE_SCOREBOARD) {
				global.skippingComputerPlay = false;
				show_frame = true;
			}
		}


		// free used voices if possible.
		clear_voices();


		// Check whether the system is good with the amount of work
		// for the last frame:
		check_fps(updater);


		// Update all objects
		update_objects(updater);


		// Move that flying saucer
		if (satellite)
			satellite->move();


		// move land
		global.slideLand();


		// Delete everything that was destroyed
		delete_destroyed(aicore);


		// Drop some naturals if applicable
		if ( (false == has_action.load(ATOMIC_READ))
		  && !global.skippingComputerPlay
		  && (winner == WINNER_NO_WIN) ) {
			do_naturals();

			if (satellite)
				satellite->shoot();
		}

		// Now update / prepare the display for drawing
		update_display();


		// draw top bar
		if (global.updateMenu)
			draw_top_bar();


		// draw all this cool stuff
		draw_objects(aicore);


		if (satellite)
			satellite->draw();


		// If requested, show the mini scoreboard
		if (global.showScoreBoard)
			draw_mini_scoreboard();


		// If wanted, an FPS Counter is shown
		if (env.showFPS)
			draw_FPS_Counter();

		// Show the end of round scoreboard if it is needed
		if ( !global.skippingComputerPlay
		  && (STAGE_SCOREBOARD == global.stage)
		  && (global.get_command() != GLOBAL_COMMAND_QUIT) )
			draw_eor_scoreboard();


		// Now update what has been drawn
		if (show_frame) {
			// Draw custom mouse cursor
			SHOW_MOUSE(global.canvas);
			global.do_updates();
		}

		// Let bots do their thinking and perform synchronized input reaction
		if (curr_tank || (STAGE_SCOREBOARD > global.stage))
			done = manage_input(aicore);


		// Fire selected stuff (if any) and check skipping time
		if (fire && (STAGE_AIM == global.stage)) {
			fire_weapon();

			if (global.skippingComputerPlay && order_wrapped)
				check_skiptime();
		}

#ifdef NETWORK
		// check for input from network
		for (int32_t i = 0; i < env.numGamePlayers; ++i) {
			if (env.players[i]->type == NETWORK_CLIENT) {
				env.players[i]->getNetCmd();
				env.players[i]->executeNetCmd(false, &aicore);
			}
		}
#endif // NETWORK


		// Advance to next tank ?
		if (!advance_tank()
		  && (STAGE_ENDGAME > global.stage) )
			// In this case at least check for exploding tanks.
			explode_tanks();


		// check for winner
		if ( (false == has_explosion.load(ATOMIC_READ))
		  && (global.stage < STAGE_ENDGAME) )
			check_winner();


		// manage the end of the round
		if (global.stage == STAGE_SCOREBOARD) {
			if ( !explode_tanks()
			  && (false == has_explosion.load())
			  && (false == has_action.load())
			  && ( (++round_end_count >= EndOfRoundFrames)
			    || (WINNER_DRAW == winner) ) ) {
				if (false == has_deco.load(ATOMIC_READ)) {
					done = true;
					global.stage = STAGE_ENDGAME;
				}
			} else if ( has_explosion.load() )
				// recheck for winner
				check_winner();
			else if ( has_action.load() )
				round_end_count = 0;
		}


		// Possibly enter AI skipping mode
		if ( !human_players && env.skipComputerPlay
		  && !global.skippingComputerPlay
		  && (false == has_action.load())
		  && (global.numTanks > 1)
		  && (STAGE_SCOREBOARD > global.stage)
		  && (WINNER_NO_WIN == winner) ) {
			global.skippingComputerPlay = true;
			global.AI_clock = 0;
		}


		// Quit if the close button was pressed
		if (global.isCloseBtnPressed())
			done = true;
	}

	/* ==========================
	 * ==== end of game loop ====
	 * ==========================
	 */


	// Stop the AI thread
	aicore.stop();

	// Stop the updater threads:
	for (int32_t class_ = 0; class_ < CLASS_COUNT; ++class_)
		updater[class_].finish();
	updMutex.lock();
	updCondition.notify_all(); // <-- That's hilariously simple, isn't it?
	updMutex.unlock();

	// Wait and join all threads
	bool has_thread = true;
	bool has_aicore = true;

	while (has_thread) {

		has_thread = false;

		// AI Core:
		if (has_aicore) {
			if (aicore.hasExited()) {
				aithread.join();
				has_aicore = false;
			} else
				has_thread = true;
		}

		// Object updaters:
		for (int32_t class_ = 0; class_ < CLASS_COUNT; ++class_) {
			if (threads[class_]) {
				if (updater[class_].hasExited()) {
					threads[class_]->join();
					delete threads[class_];
					threads[class_] = nullptr;
				} else
					has_thread = true;
			}
		}
		if (has_thread)
			std::this_thread::yield();
	} // end of waiting for all threads to join


	// Show end of round score board and credit winner(s)
	if ((global.get_command() != GLOBAL_COMMAND_QUIT))
		draw_eor_scoreboard();

	// Ensure full window clipping rectangle
	set_clip_rect(global.canvas, 0, 0, env.window.w, env.window.h);

    // clean up

    if (satellite)
       delete satellite;

	// remove existing tanks etc
	for (int32_t i = 0; i < env.numGamePlayers; ++i) {
		if (env.players[i]->tank) {
			env.players[i]->reclaimShield();
			delete env.players[i]->tank;
		}
	}

	WIN_CLOCK_REMOVE
}



///  ==========================
///   === Helper functions ===
///  ==========================


static inline bool advance_tank()
{
	/* The whole finding of a next tank to have their shot is needed in
	 * two situations during the fire stage:
	 * a) When all action has ceased, because then the turn is over.
	 * b) The current tanks has exploded, a substitute must be found.
	 */

	if ( ( (false == has_action.load(ATOMIC_READ))
		&& (STAGE_FIRE == global.stage) )
	  || ( (global.stage < STAGE_SCOREBOARD)
		&& (!curr_tank || !curr_tank->player || curr_tank->destroy) ) ) {

		bool need_update = false;

		if ( (STAGE_FIRE == global.stage)
		  && !explode_tanks()
		  && (false == has_explosion.load(ATOMIC_READ))
		  && (false == has_action.load(ATOMIC_READ)) ) {
			// Note: has_action is checked last, so dead tanks can
			//       explode even if others are still falling/flying
			//       around.

			// Normal tank advancement after firing stage
			global.stage  = STAGE_AIM;
			order_wrapped = false;
			change_wind_strength();

			if ( !death_substitute ) {
				if ( next_tank && !next_tank->destroy
				  && (next_tank != curr_tank))
					curr_tank = next_tank;
				else
					curr_tank = global.get_next_tank(&order_wrapped);
			}

			death_substitute = false;
			next_tank        = nullptr;
			need_update      = true;

			// Activate shields and repair the tank.
			if (curr_tank) {
				curr_tank->reactivate_shield();
				curr_tank->repair();
			}
		} else if (!curr_tank || !curr_tank->player || curr_tank->destroy) {
			// We need a death substitute
			if (next_tank && !next_tank->destroy)
				curr_tank = next_tank;
			else
				curr_tank = global.get_next_tank(&order_wrapped);
			death_substitute = true;
			next_tank        = nullptr;
			need_update      = true;
		}

		if (need_update) {
			update_screen     = true;
			fire              = false;
			global.updateMenu = true;
			global.set_curr_tank(curr_tank);

			return true;
		}
	} // End of checking for tank advancement

	return false;
}


static inline void change_wind_strength ()
{
	if (!env.windvariation || !env.windstrength)
		return;
	else {
		global.wind = global.lastwind
		          + static_cast<double>(rand () % (env.windvariation * 100)) / 100
		          - static_cast<double>(env.windvariation) / 2.;
		if (global.wind > (env.windstrength / 2))
			global.wind = static_cast<double>(env.windstrength) / 2.;
		else if (global.wind < (-env.windstrength / 2))
			global.wind = static_cast<double>(env.windstrength) / -2.;

		global.lastwind = global.wind;
	}

	// make sure game clients have up to date wind data
#  ifdef NETWORK
	char buffer[64];
	sprintf(buffer, "WIND %f", global.wind);
	env.sendToClients(buffer);
#  endif // NETWORK
}


// See whether decorations must be reduced or time can be wasted
// to achieve the set FPS.
static inline void check_fps(ObjectUpdater* upd)
{
	if ( !global.skippingComputerPlay ) {
		game_us_needed = game_us_get();
		int32_t us_unused = us_per_frame - game_us_needed;

		if ( us_unused < 500 ) {
			// Stop adding decoration:
			if (!global.hasTooMuchDeco)
				global.hasTooMuchDeco = true;

			// If more us where needed than available, there is already
			// too much deco on the screen.
			// All smoke is forwarded in aging now, so they would get
			// deleted sooner.
			if (us_unused < 0) {
				int32_t agemod = (us_unused / -1000) + 1;

				// Don't age more than 5 frames:
				if (agemod > 5)
					agemod = 5;
				upd[smkIdx].setForceAge(agemod);
			}
		} else if ( global.hasTooMuchDeco && (us_unused > 1000) ) {
			// (Re-)enable deco
			global.hasTooMuchDeco = false;

			// Normal smoke ageing
			upd[smkIdx].setForceAge(0);
		}

		// Sleep what is unused
		if ( us_unused > 0 ) {
			USLEEP(us_unused)
		}

		// Add what has been used until now:
		game_us_needed += game_us_get();
	}
}


// Check whether the AI time is up and force a draw if it is.
// This method does not check whether it is needed and must not
// be called if global.skippingComputerPlay is false!
static inline void check_overtime(AICore &aicore)
{
	// Check every second whether the AI clock
	// should be changed:
	if (second_passed) {
		global.AI_clock  += SIGN(AI_time_change);
		AI_time_change    = 0;
		global.updateMenu = true;
	}

	// Skip every other frame
	show_frame = !show_frame;

	// Kill all tanks if skipping time is over
	if ( (global.AI_clock >= MAX_AI_TIME)
	  && (winner == WINNER_NO_WIN) ) {

		// Stop the ai first:
		aicore.stop();
        while (!aicore.hasExited())
			std::this_thread::yield();

		// in over-time, kill all tanks
		TANK* tank = nullptr;
		global.getHeadOfClass(CLASS_TANK, &tank);
		while (tank) {
			// reclaim shield. This is fair, because technically
			// the bots survive the battle. The tank destruction
			// is only "for the effect".
			// And if they bought vengeance items, they'll loose
			// one now. Expensive enough.
			if (tank->player)
				tank->player->reclaimShield();
			tank->addDamage(nullptr, tank->sh + tank->l + 1);
			tank->applyDamage();
			tank->resetFlashDamage();
			tank->getNext(&tank);
		}

		global.skippingComputerPlay = false;
		show_frame                  = true;
		global.stage                = STAGE_SCOREBOARD;
		winner                      = WINNER_DRAW;

		// All action is false, or they won't explode
		has_action.store(   false, ATOMIC_WRITE);
		has_explosion.store(false, ATOMIC_WRITE);

		// Now make them go bye bye
		explode_tanks();
	}
}


static inline void check_skiptime()
{
	// Check whether to reset the AI_clock
	int32_t cur_health = 0;
	int32_t bots_alive = 0;

	for (int32_t i = 0; i < env.numGamePlayers; ++i) {
		if ( (env.players[i])
		  && (env.players[i]->tank) ) {
			cur_health += env.players[i]->tank->l
						+ env.players[i]->tank->sh;
			++bots_alive;
		}
	}

	int32_t health_delta = skip_health - cur_health;

	if (!skip_health || (health_delta < (bots_alive * 5)))
		// No (real) damage done, raise AI_Clock
		++AI_time_change;
	else if (health_delta > (bots_alive * 25))
		// Lots of damage, halve the clock
		global.AI_clock /= 2;
	else if (global.AI_clock && (health_delta > (bots_alive * 10)) )
		// Moderate damage, decrease the AI_clock
		--AI_time_change;
		// No else, it would be a little damage and that means
		// do not change the AI_clock at all.
	skip_health = cur_health;
}


static inline void clear_voices()
{
	// Assume one voice per 2 ms to be usable again
	/// @todo : This must be substituted by a real direct
	///         control over the voices used. The auto-mixing
	///         of allegro 4 is just too inefficient.
	/// However, this way it is a lot better than without any control...
	if (game_us_needed >= 1000)
		global.used_voices -= game_us_needed / 500;
	else
		--global.used_voices;
	if (global.used_voices < 0)
		global.used_voices = 0;
}


static inline void check_winner()
{
	bool    all_jedi       = true;
	bool    all_sith       = true;
	int32_t player_count   = 0;
	int32_t last_alive     = -1;

	for (int32_t i = 0; i < env.numGamePlayers; ++i) {
		TANK* tank = env.players[i]->tank;
		if (  tank && tank->l && !tank->destroy && tank->player ) {
			eTeamTypes team = tank->player->team;
			if (TEAM_SITH != team) all_sith = false;
			if (TEAM_JEDI != team) all_jedi = false;

			last_alive = i;
			player_count++;
		}
	}

	if      (!player_count)     winner = WINNER_DRAW;
	else if (all_jedi)          winner = WINNER_JEDI;
	else if (all_sith)          winner = WINNER_SITH;
	else if (1 == player_count) winner = last_alive;
	else                        winner = WINNER_NO_WIN;

	// End skipping play if a winner is known:
	if (WINNER_NO_WIN != winner) {
		if (global.stage < STAGE_SCOREBOARD)
			global.stage = STAGE_SCOREBOARD;
		global.skippingComputerPlay = false;
		show_frame = true;
	}
}


/*****************************************************************************
 * colorDistance
 *
 * Treat two color values as 3D vectors of the form <r,g,b>.
 * Compute the scalar size of the difference between the two vectors.
 * *****************************************************************************/
double colorDistance (int32_t col1, int32_t col2)
{
	int32_t r1 = getr (col1);
	int32_t g1 = getg (col1);
	int32_t b1 = getb (col1);
	int32_t r2 = getr (col2);
	int32_t g2 = getg (col2);
	int32_t b2 = getb (col2);

	// Treat the colour-cube as a space
	return FABSDISTANCE3(r1, g1, b1, r2, g2, b2);
}


static inline void delete_destroyed(AICore &aicore)
{
	vobj_t* next_obj = nullptr;
	vobj_t* obj      = nullptr;

	// do not create new FLOATTEXT instance while deletion is in progress
	aicore.forbidText();

	// Now loop classes and delete destroyed objects
	for (int32_t class_ = 0; class_ < CLASS_COUNT; ++class_) {
		// Skip tank class, the tanks must be deleted in their special
		// function explode_tanks() as there is more to do.
		if (CLASS_TANK == class_)
			continue;

		eClasses e_class = static_cast<eClasses>(class_);

		global.getHeadOfClass(e_class, &obj);
		global.lockClass(e_class);

		while(obj) {
			obj->getNext(&next_obj);

			// Update object if it is destroyed
			if (obj->destroy) {
				obj->requireUpdate();
				obj->update();

				// For deleting the object, the class must be unlocked,
				// or we'll hit a deadlock with global.removeObject().
				global.unlockClass(e_class);
				delete obj;
				global.lockClass(e_class);
			}
			obj = next_obj;
		} // End of looping objects of one class

		// Finished:
		global.unlockClass(e_class);
	} // End of looping classes

	// Eventually re-allow AICore to create FLOATTEXT instances again
	aicore.allowText();
}


void do_naturals()
{
	if (global.naturals_activated >= 5)
		return;

	if (env.lightning) {
		int32_t chance = (600 / env.lightning) + 100;

		if (!(rand () % chance)) {
			try {
				new BEAM (nullptr, 1 + (rand () % (env.screenWidth - 2)),
				          MENUHEIGHT + (env.isBoxed ? 1 : 0),
				          ((rand () % 160) + (360 - 80)) % 360,
				          SML_LIGHTNING + (rand () % env.lightning), BT_NATURAL);
				global.naturals_activated++;
				return;
			} catch (...) { /* can't do anything here */ }
		}
	}      // end of lightning

	// only create meteors and dirt balls if we are not in aim mode on simul turn type
	if ( (env.turntype == TURN_SIMUL) && (global.stage == STAGE_AIM) )
		return;

	if (env.meteors) {
		int32_t chance = (600 / env.meteors) + 100;

		if (!(rand () % chance)) {
			int32_t ca = ((rand () % 160) + (360 - 80)) % 360;
			double  mxv = env.slope[ca][0] * 5;
			double  myv = env.slope[ca][1] * 5;

			try {
				new MISSILE(nullptr, 1 + (rand () % (env.screenWidth - 2)),
				            MENUHEIGHT + (env.isBoxed ? 1 : 0), mxv, myv,
				            SML_METEOR + (rand () % env.meteors), MT_NATURAL,
				            1, 0);
				global.naturals_activated++;
				return;
			} catch (...) { /* can't do anything here */ }
		}
	}

	if (env.falling_dirt_balls) {
		int32_t chance = (600 / env.falling_dirt_balls) + 100;

		if (! (rand() % chance) ) {
			int ca = ((rand() % 100) + (360 - 80) ) % 360;
			double mxv = env.slope[ca][0] * 5;
			double myv = env.slope[ca][1] * 5;

			try {
				new MISSILE(nullptr, 1 + (rand () % (env.screenWidth - 2)),
				            MENUHEIGHT + (env.isBoxed ? 1 : 0), mxv, myv,
				            DIRT_BALL + ( rand() % env.falling_dirt_balls),
				            MT_NATURAL, 1, 0);
				global.naturals_activated++;
			} catch (...) { /* can't do anything here */ }
		}
	}
}


static inline void draw_FPS_Counter()
{
	++FPS_counter;

	if (second_passed) {
		FPS_last    = FPS_counter;
		FPS_counter = 0;
	}

	int32_t fc = BLUE;
	int32_t bc = GREY;
	if (FPS_last < (env.frames_per_second - 5)) {
		fc = DARK_RED;
		bc = DARK_GREEN;
	} else if (FPS_last > (env.frames_per_second + 5)) {
		fc = TURQUOISE;
		bc = SILVER;
	}

	textprintf_ex (global.canvas, font, FPS_pos + 1, MENUHEIGHT + 11,
				   bc, -1, "% 4d FPS", FPS_last);
	textprintf_ex (global.canvas, font, FPS_pos,     MENUHEIGHT + 10,
				   fc, -1, "% 4d FPS", FPS_last);

	global.make_update(FPS_pos -  1, MENUHEIGHT +  5,
					   FPS_pos + 60, MENUHEIGHT + 20);

}


static inline void draw_objects(AICore &aicore)
{
	vobj_t* obj = nullptr;

	has_deco.store(false, ATOMIC_WRITE);
	set_clip_rect (global.canvas, 0, MENUHEIGHT,
				   (env.screenWidth-1), (env.screenHeight-1));

	// do not create new FLOATTEXT instance while drawing is in progress
	aicore.forbidText();

	for (int32_t class_ = 0; class_ < CLASS_COUNT; ++class_) {

		global.getHeadOfClass(static_cast<eClasses>(class_), &obj);
		while(obj) {

			if (show_frame) {
				obj->draw();
				obj->update();
			}

			if ( (false == has_deco.load(ATOMIC_READ))
			  && ( (CLASS_DECOR_DIRT  == class_)
			    || (CLASS_DECOR_SMOKE == class_) ) )
				has_deco.store(true, ATOMIC_WRITE);

			obj->getNext(&obj);
		} // End of looping objects

	} // End of looping classes

	// Eventually re-allow AICore to create FLOATTEXT instances again
	aicore.allowText();
}


/// @brief the ingame mini score board
static inline void draw_mini_scoreboard()
{
	int32_t line = MENUHEIGHT + 2;

	for (int i = 0; i < env.maxNumTanks; ++i) {
		PLAYER *player = env.playerOrder[i];

		assert(player && "ERROR: player in playerOrder is nullptr!");

		if (player) {
			int32_t color = player->color;
			const char*   money = Add_Comma(player->money);
			const char*   name  = player->getName();
			const char*   team  = player->getTeamName();
			int32_t       mid_y = line + (env.fontHeight / 2) + 1;

			// Strike through dead players (BLACK background *before* the name)
			if (!player->tank || player->tank->destroy)
				hline(global.canvas, 17, mid_y + 1, 276, BLACK);

			// Display team
			textprintf_ex (global.canvas, font,  16, line + 1, BLACK, -1,
			               "(%-7s)", team);
			textprintf_ex (global.canvas, font,  15, line,     color, -1,
			               "(%-7s)", team);

			// Display player indicator
			player->drawIndicator(score_name_pos - env.fontHeight - 2,
			                      line + 1, env.fontHeight - 3);

			// Display name
			textprintf_ex (global.canvas, font, score_name_pos + 1, line + 1,
			               BLACK, -1, "%s", name);
			textprintf_ex (global.canvas, font, score_name_pos, line,
			               color, -1, "%s", name);

			// Display money
			textprintf_ex (global.canvas, font, score_money_pos + 1, line + 1,
			               BLACK, -1, "$%s", money);
			textprintf_ex (global.canvas, font, score_money_pos, line,
			               color, -1, "$%s", money);

			// Draw an arrow indicating the current player
			if (curr_tank == player->tank) {
				hline(global.canvas, 4, mid_y,     12, color);
				hline(global.canvas, 5, mid_y + 1, 13, BLACK);
			}

			// Strike through dead players (color)
			if (!player->tank || player->tank->destroy)
				hline(global.canvas, 16, mid_y, 275, color);

			line += env.fontHeight;
		}
	}

	global.make_update(0, MENUHEIGHT, 300, line);
}


/// @brief This method draws the top bar with all current information
void draw_top_bar ()
{
	TANK*         tank          = global.get_curr_tank();
	PLAYER*       player        = tank   ? tank->player              : nullptr;
	const char*   name          = player ? player->getName ()        : nullptr;
	const char*   team_name     = player ? player->getTeamName()     : nullptr;
	int32_t       color         = player ? player->color             : BLACK;
	int32_t       time_to_fire  = player ? player->time_left_to_fire : 0;
	int32_t       y1            =  0;
	int32_t       y2            = 13;
	int32_t       y3            = 26;
	static
	int32_t change_colour = RED;

	// Copy empty top bar background
	global.updateMenu = false;

	// copy backdrop:
	set_clip_rect(global.canvas, 0, 0, env.screenWidth - 1, MENUHEIGHT - 1);
	blit (env.gfxData.topbar, global.canvas, 0, 0, 0, 0, env.screenWidth, MENUHEIGHT);

	// Fill in player info if possible :
	if (player) {
		// name is first, as always
		textout_ex   (global.canvas, font, name, 2, y1 + 1,
		              GetShadeColor(color, true, PINK), -1);
		textout_ex   (global.canvas, font, name, 1, y1,
		              color, -1);
		textprintf_ex(global.canvas, font, 1, y2, BLACK, -1,
		              "%s", env.ingame->Get_Line(18));

		// Display set angle. 0 is directly left, 180 points directly right
		graph_bar_center (50, y2 + 4, color, -(tank->a - 180) / 2, 180 / 2);
		textprintf_ex(global.canvas, font, 150, y2, BLACK, -1,
		              "%d", GET_DISP_ANGLE(tank->a));

		// Display set power
		graph_bar (50, y3 + 4, color, (tank->p) / (MAX_POWER/90), 90);
		textprintf_ex(global.canvas, font,   1, y3, BLACK, -1,
		              "%s", env.ingame->Get_Line(19));
		textprintf_ex(global.canvas, font, 150, y3, BLACK, -1,
		              "%d", tank->p);

		// Display the team name
		textprintf_ex(global.canvas, font, 200, y3, BLACK, -1,
		              "%s: %s", env.ingame->Get_Line(20), team_name);

		// Display weapon if chosen
		if (tank->cw < WEAPONS) {
			int32_t       amt = tank->player->nm[tank->cw]
			                  / weapon[tank->cw].getDelayDiv();
			int32_t col = BLACK;

			// Forcibly changed weapons (previous out of ammo) flash red/white
			// on each update.
			if (tank->player->changed_weapon) {
				col = change_colour;

				if (RED == change_colour)
					change_colour = WHITE;
				else
					change_colour = RED;
			}
			textprintf_ex(global.canvas, font, 180, y1, col, -1,
			              "%s: %d", weapon[tank->cw].getName(), amt);
		} else
			textprintf_ex(global.canvas, font, 180, y1, BLACK, -1,
			              "%s: %d", item[tank->cw - WEAPONS].getName(),
			              tank->player->ni[tank->cw - WEAPONS]);

		// Show the weapon / item icon
		draw_sprite(global.canvas, env.stock[ (tank->cw > 0) ? tank->cw : 1], 700, 1);

		// Eventually print out money, fuel and power
		textprintf_ex(global.canvas, font, 386, y1, BLACK, -1,
		              "$%s", Add_Comma(tank->player->money));
		textprintf_ex(global.canvas, font, 386, y2, BLACK, -1,
		              "%s: %d", env.ingame->Get_Line(21),
		              tank->player->ni[ITEM_FUEL]);
		textprintf_ex(global.canvas, font, 386, y3, BLACK, -1,
		              "%s: %.2f", "Power",
		              tank->player->damageMultiplier);
	} // End of displaying player info


	// Display round information
	textprintf_ex(global.canvas, font, 500,  y1, BLACK, -1,
	              "%s %d/%d", env.ingame->Get_Line(12),
	              env.rounds - global.currentround, env.rounds);

	// If a tank status is set, display it
	if (global.tank_status[0])
		textprintf_ex(global.canvas, font, 350, y3, global.tank_status_colour,
		              -1, "%s", global.tank_status);

	// Show the wind blowing (if configured)
	if (env.windstrength > 0) {
		textprintf_ex(global.canvas, font, 500, y2, BLACK, -1,
		              "%s", env.ingame->Get_Line(22));

		int32_t wcol1 = global.wind > 0 ? 1 : 0;
		int32_t wcol2 = global.wind < 0 ? 1 : 0;

		rect    (global.canvas, 540,                          y2 +  4,
		                        542 + (env.windstrength * 4), y2 + 12, BLACK);
		rectfill(global.canvas, 541 + (env.windstrength * 2), y2 +  5,
		    541 + (global.wind * 4) + (env.windstrength * 2), y2 + 11,
		         makecol(200 * wcol1, 200 * wcol2, 0) );
	}

	// Print AI Skip time or chess style clock if set
	if ( (global.AI_clock > -1) && (global.AI_clock <= MAX_AI_TIME) )
		textprintf_ex(global.canvas, font, 500, y3, BLACK, -1,
		              "AI Time: %d", MAX_AI_TIME - global.AI_clock);
	else if (env.maxFireTime)
		textprintf_ex(global.canvas, font, 500, y3, BLACK, -1,
		              "Time: %d", time_to_fire);

	// Update and be done
	global.stopwindow = 1;
	global.make_update (0, 0, env.screenWidth, MENUHEIGHT);
	global.stopwindow = 0;
}


/// Let all tanks explode that are destroyed
/// @return true if at least one tank goes bye bye
static inline bool explode_tanks()
{
	// return if something is exploding already
	if (has_explosion.load(ATOMIC_READ))
		return true; // true, because an explosion is present.

	TANK* tank       = nullptr;
	TANK* tmp        = nullptr;
	bool  res        = false;
	bool  tanks_left = false;

	// Check how many tanks are still alive and whether they are from
	// different teams
	bool all_jedi       = true;
	bool all_sith       = true;
	bool all_jedi_alive = true;
	bool all_sith_alive = true;
	bool do_explode     = false;

	global.getHeadOfClass(CLASS_TANK, &tank);

	while (tank) {
		// Look for teams for any tanks including exploding ones
		if (tank->player && (TEAM_JEDI != tank->player->team) )
			all_jedi = false;
		if (tank->player && (TEAM_SITH != tank->player->team) )
			all_sith = false;

		// Look for alive tanks
		if ( (tank->l > 0) && !tank->destroy) {
			tanks_left = true;

			// Note down if alive tanks are from other teams
			if (tank->player && (TEAM_JEDI != tank->player->team) )
				all_jedi_alive = false;
			if (tank->player && (TEAM_SITH != tank->player->team) )
				all_sith_alive = false;

		} else
			do_explode = true;

		tank->getNext(&tank);
	}

	// Return if no tank is about to explode:
	if (!do_explode)
		return false;

	// If tanks are left that are only jedi or sith, vengeance is disallowed:
	bool allow_vengeance = (tanks_left && !all_jedi && !all_sith);

	// Now explode what has to go
	global.getHeadOfClass(CLASS_TANK, &tank);
	while (tank) {

		tank->getNext(&tmp);

		// If the tank is now destroyed, let it explode
		if (tank->destroy) {

			/* Second level of vengeance allowance:
			 * If, for example, three tanks are left, one neutral, and two of
			 * the same team. And the neutral and one team tank are about to
			 * explode. Then the team tank must not trigger vengeance, because
			 * their team already won. However, the neutral tank might surely
			 * detonate in a firestorm and maybe force a draw.
			 */
			bool do_vengeance = allow_vengeance;
			if ( do_vengeance
			  && ( (all_jedi_alive && (TEAM_JEDI == tank->player->team))
			    || (all_sith_alive && (TEAM_SITH == tank->player->team)) ) )
				do_vengeance = false;

			tank->explode(do_vengeance);

			has_action.store(   true, ATOMIC_WRITE); // For sure.
			has_explosion.store(true, ATOMIC_WRITE); // It'll go now.
			res = true;

			// count human player reduction
			if ( (tank->player)
			  && ( (HUMAN_PLAYER   == tank->player->type)
				|| (NETWORK_CLIENT == tank->player->type) ) )
				--human_players;

			// Now the tank has to be removed, but take care
			// of the current and next tank if they are this
			if (curr_tank == tank)
				curr_tank = nullptr;
			if (next_tank == tank)
				next_tank = nullptr;

			// Remove from order array
			global.removeTank(tank);

			delete tank;
			tank = nullptr;
		} // End of handling tank destruction

		tank = tmp;
		tmp  = nullptr;
	}

	return res;
}


/// @brief Wrapper to automatically trigger firing in simultaneous mode, too.
static inline void fire_weapon()
{
	assert( (STAGE_AIM == global.stage)
	  && " ERROR: fire_weapon() called, but not STAGE_AIM!");
	global.stage = STAGE_FIRE;

	if (curr_tank && !curr_tank->destroy) {
		has_action.store(true);
		curr_tank->simActivateCurrentSelection();
	}

	// Have everything launched in simultaneous mode
	if (TURN_SIMUL == env.turntype) {
		TANK*   tank      = nullptr;

		global.getHeadOfClass(CLASS_TANK, &tank);
		while (tank) {
			if (tank->player->skip_me)
				tank->player->skip_me = false;
			else {
				has_action.store(true);
				tank->activateCurrentSelection();
			}
			tank->player->time_left_to_fire = env.maxFireTime;
			tank->getNext(&tank);
		}

		assert( (STAGE_FIRE == global.stage)
			 && "ERROR: global.stage changed illegally!");
	}

	fire = false;
}


// Draws indication bar
static inline void graph_bar(int32_t x, int32_t y, int32_t col, int32_t actual,
                      int32_t max_)
{
	rect     (global.canvas, x,     y,     x + max_ + 2,   y + 8, BLACK);
	rectfill (global.canvas, x + 1, y + 1, x + 1 + actual, y + 7, col);
}


// Draws indication bar - centered
static inline void graph_bar_center(int32_t x, int32_t y, int32_t col,
                             int32_t actual, int32_t max_)
{
	rect     (global.canvas, x, y,
	          x + max_ + 2,              y + 8, BLACK);
	rectfill (global.canvas, x + 1 + max_ / 2, y + 1,
	          x + 1 + actual + max_ / 2, y + 7, col);
}


// do new round preparations
static inline void init_new_round()
{
	// First env,
	srand(time(NULL));
	env.newRound();

	// then the players in case the campaign mode rise kicks in
	for (int32_t i = 0; i < env.numGamePlayers; ++i)
		env.players[i]->newRound();

	// finally global, so campaign mode round is changed after the players.
	global.newRound();

	// clear floating text
	FLOATTEXT* txt = nullptr;
	global.getHeadOfClass(CLASS_FLOATTEXT, &txt);
	while (txt) {
		txt->newRound();
		txt->getNext(&txt);
	}

	// Initialize the static inline global values, so no old data from a previous
	// run is carried over. (Unless this is wanted, of course)
	AI_time_change  = 0;
	curr_tank       = nullptr;
	fire            = false;
	human_players   = 0;
	us_per_frame    = 1000000 / env.frames_per_second;
	next_tank       = nullptr;
	order_wrapped   = false;
	second_passed   = false;
	show_frame      = true;
	skip_health     = 0;
	update_screen   = true;
	winner          = WINNER_NO_WIN;
	game_us_reset();

	// everyone gets to buy stuff. While the (human)
	// player(s) are at it, generate the next level
	// in the background.
	LevelCreator lvlCreator;
	std::thread gen_land_thread(std::ref(lvlCreator));
	shop(&lvlCreator);
	gen_land_thread.join();

	// End each players shopping and count the number of human players
	for (int32_t i = 0; i < env.numGamePlayers; ++i) {
		env.players[i]->exitShop();
		if ( (env.players[i]->type == HUMAN_PLAYER)
		  || (env.players[i]->type == NETWORK_CLIENT) )
			human_players++;
	}

	// set wind
	if (env.windstrength)
		global.wind = (rand() % env.windstrength) - (env.windstrength / 2);
	else
		global.wind = 0;
	global.lastwind = global.wind;

	// finalize preparation
	fi = 1;
	global.stage = STAGE_AIM;
	global.updateMenu = true;
	env.window = BOX(0, 0, env.screenWidth - 1, env.screenHeight - 1);
}


/// @brief Wrapper to combine both human input and AI actions.
static inline bool manage_input(AICore &aicore)
{
	bool done = false;

	if ( curr_tank && curr_tank->player ) {
		global.updateMenu = false;
		PLAYER* player    = curr_tank->player;
		bool    can_fire  = !( has_action.load(ATOMIC_READ)
		                    || has_explosion.load(ATOMIC_READ) );
		int32_t result    = player->controlTank(&aicore, can_fire);

		if (CONTROL_QUIT == result)
			done = true;
		else if (CONTROL_FIRE == result) {
			has_action.store(true);
			next_tank = global.get_next_tank(&order_wrapped);

			if (order_wrapped || (env.turntype != TURN_SIMUL) )
				fire = true;
		} else if ( (CONTROL_SKIP == result)
		         && !human_players
		         && env.skipComputerPlay
		         && !global.skippingComputerPlay
		         && (STAGE_SCOREBOARD > global.stage)
		         && (WINNER_NO_WIN == winner) ) {
			global.skippingComputerPlay = true;
			global.AI_clock = 0;
		}

		update_screen = false;
		if (result)
			global.updateMenu = true;
	}

	return done;
}


/** @brief Set up the next level to play
  *
  * This must work in parallel with the shop(), so any drawing must
  * lock the land, do the drawing and unlock it again.
**/
static inline void set_level_settings(LevelCreator* lcr)
{
#if defined(ATANKS_IS_WINDOWS)
	// Here srand() is thread local according to MSDN.
	// This affects cygwin/mingw builds, too.
	// Thanks to billy Buerger for pointing this out!
	srand(time(nullptr));
#endif // Microsoft Windows Build

	//  -------------------------
	// ===  Choosing colours   ===
	//=============================
	lcr->working_on(1);

	// Choose first gradients for sky and land

	// First the land:
	if (lcr->can_work()) {
		global.curland = (rand () % LANDS)
							+ (CT_CRISPY == env.colourTheme ? LANDS : 0);
		if (!env.gfxData.land_gradient_strips[global.curland]) {

			global.lockLand();

			env.gfxData.land_gradient_strips[global.curland]
				= create_gradient_strip (land_gradients[global.curland],
				                         (env.screenHeight - MENUHEIGHT));

			global.unlockLand();
		}
	}

	// Then the sky
	if (lcr->can_work()) {
		global.cursky = (rand () % SKIES)
		              + (CT_CRISPY == env.colourTheme ? SKIES : 0);

		if (!env.gfxData.sky_gradient_strips[global.cursky]) {
			global.lockLand();

			env.gfxData.sky_gradient_strips[global.cursky]
				= create_gradient_strip (sky_gradients[global.cursky],
				                         (env.screenHeight - MENUHEIGHT));

			global.unlockLand();
		}
	}
	BITMAP* sky_gradient_strip = env.gfxData.sky_gradient_strips[global.cursky];

	//  -------------------------
	// === Rendering Landscape ===
	//=============================
	lcr->working_on(2);
	if (lcr->can_work())
		generate_land (lcr, rand() % env.screenWidth, env.screenHeight);


	//  -------------------------
	// ===    Check Colours    ===
	//=============================
	if (lcr->can_work()) {
		int32_t peak_height = env.screenHeight;
		for (int32_t z = 0; lcr->can_work() && (z < env.screenWidth); ++z) {
			if (peak_height > global.surface[z].load())
				peak_height = global.surface[z].load(ATOMIC_READ);
		}

		int32_t min_dist    = 128; // start with this colour distance wanted
		int32_t max_tries   = 16;  // These many tries before lowering the distance
		int32_t cur_try     = 1;
		int32_t bottom      = env.screenHeight - MENUHEIGHT;
		int32_t max_y       = std::min(env.screenHeight - peak_height,
		                               env.screenHeight - MENUHEIGHT);
		bool    has_colours = false;

		while (!has_colours && lcr->can_work()) {
			has_colours = true;

			for (int32_t y = 2; lcr->can_work() && has_colours && (y < max_y); ++y) {
				if (colorDistance(
						getpixel(sky_gradient_strip,  0, bottom - y),
						getpixel(global.terrain, 0, env.screenHeight - y) )
						< min_dist)
					has_colours = false;
			}

			if (!has_colours && lcr->can_work()) {
				// Create new strip:
				global.cursky = (rand () % SKIES)
								   + (CT_CRISPY == env.colourTheme ? SKIES : 0);

				if (!env.gfxData.sky_gradient_strips[global.cursky]) {
					global.lockLand();

					env.gfxData.sky_gradient_strips[global.cursky]
						= create_gradient_strip (sky_gradients[global.cursky],
												(env.screenHeight - MENUHEIGHT));

					global.unlockLand();
				}
				sky_gradient_strip = env.gfxData.sky_gradient_strips[global.cursky];

				// Advance try and check:
				if (++cur_try > max_tries) {
					cur_try   = 1;
					min_dist /= 2;

					// Break if min_dist is reduced to 1:
					if (min_dist < 2)
						has_colours = true;
				} // end of advancing tries
			} // end of handling wrong colours
		} // End of searching suitable sky colours
	} // end of thread not to be killed

	//  -------------------------
	// ===    Rendering Sky    ===
	//=============================
	lcr->working_on(3);

	if (lcr->can_work()) {
		if (env.sky
		  && ( (env.sky->w  != env.screenWidth)
		    || (env.sky->h != (env.screenHeight - MENUHEIGHT) ) ) ) {
			destroy_bitmap(env.sky);
			env.sky = nullptr;
		}

		// see if we want a custom background
		if (env.custom_background && env.bitmap_filenames) {
			global.lockLand();
			if (env.sky)
				destroy_bitmap(env.sky);
			env.sky = load_bitmap(env.bitmap_filenames[
			                      rand() % env.number_of_bitmaps ], nullptr);
			global.unlockLand();
		}

		// if we do not have a custom background (or do not want one) create a new background
		if (!env.custom_background || !env.sky) {
			global.lockLand();

			if (!env.sky)
				env.sky = create_bitmap(env.screenWidth,
				                           env.screenHeight - MENUHEIGHT);

			global.unlockLand();

			generate_sky (lcr, sky_gradients[global.cursky],
							(env.ditherGradients ? GENSKY_DITHERGRAD : 0 ) |
							(env.detailedSky ? GENSKY_DETAILED : 0 )  );
		}
	}

	lcr->working_on(4);
}


/// @brief tank placement and player ordering
static inline void set_tank_settings()
{
	//  -------------------------
	// ===   Tank Placement    ===
	//=============================

	// Distribute tanks over the landscape
	abool_t taken[MAXPLAYERS] = { ATOMIC_VAR_INIT(false) };
	int32_t middle            = global.numTanks / 2;

	global.getHeadOfClass(CLASS_TANK, &curr_tank);
	while (curr_tank) {
		int32_t x = rand () % global.numTanks;
		while (taken[x]){
			bool go_up = x < middle ? true : false;
			while (taken[x] && (x > 0) && (x < (global.numTanks - 1)) )
				x += go_up ? 1 : -1;
			if (taken[x])
				x = rand () % global.numTanks;
		}

		/* Note: this is a lot faster than the previous approach, because
		 * the chance to fail if only one place is left is very high, while
		 * the chance to go into the direction of the last free spot is 50%.
		 */

		if (!taken[x]) {
			taken[x] = true;

			int32_t tx = (x + 1) * (env.screenWidth / (global.numTanks + 1));
			int32_t ty = env.screenHeight - global.surface[tx].load();

			curr_tank->newRound (tx, ty);
			curr_tank->getNext(&curr_tank);
		}
	}

	//  --------------------------
	// === Determine Tank Order ===
	//==============================

	for (int32_t z = 0; z < MAXPLAYERS; z++) {
		global.order[z]    = nullptr;
		env.playerOrder[z] = nullptr;
	}
	env.maxNumTanks = global.numTanks;

	// Distribute tanks in the order array
	int32_t place = 0;
	global.getHeadOfClass(CLASS_TANK, &curr_tank);
	while (curr_tank) {
		global.order[place++] = curr_tank;
		curr_tank->getNext(&curr_tank);
	}

	// Mix up the order if it is wanted to be randomized
	if ( (env.turntype == TURN_RANDOM)
	  || (env.turntype == TURN_SIMUL)) {
		for (int32_t index = 0; index < env.maxNumTanks; ++index) {
			for (int32_t round = 0; round < middle; ++round) {
				int32_t target = rand () % global.numTanks;
				if (target != index) {
					TANK* tmp_tank       = global.order[index];
					global.order[index]  = global.order[target];
					global.order[target] = tmp_tank;
				}
			}
		}
	}

	// Otherwise sort the order array
	else {
		bool sorted = false;
		while (!sorted) {
			sorted = true;
			for (int32_t index = 0; index < env.maxNumTanks - 1; ++index) {
				bool swap = false;
				if (env.turntype == TURN_HIGH) {
					if (global.order[index    ]->player->score
					  < global.order[index + 1]->player->score)
					swap = true;
				} else if (env.turntype == TURN_LOW) {
					if (global.order[index    ]->player->score
					  > global.order[index + 1]->player->score)
					swap = true;
				}
				if (swap) {
					TANK *tempTank = global.order[index];
					global.order[index] = global.order[index + 1];
					global.order[index + 1] = tempTank;
					sorted = false;
				}
			}
		}
	}

	// Create the ordered list of players
	// Since tanks get deleted as they are destroyed, we loose the information
	// about their order, but the player order list will be complete through the
	// entire round.
	int32_t max_name_len = 0;
	int32_t max_team_len = 0;

	for (int i = 0; i < env.maxNumTanks; ++i) {
		env.playerOrder[i] = global.order[i]->player;

		int32_t name_len = text_length(font, env.playerOrder[i]->getName());
		int32_t team_len = text_length(font, env.playerOrder[i]->getTeamName());

		if (name_len > max_name_len)
			max_name_len = name_len;
		if (team_len > max_team_len)
			max_team_len = team_len;

		// Reset tank flash damage and activate their first shields:
		if (env.playerOrder[i]->tank) {
			env.playerOrder[i]->tank->resetFlashDamage();
			env.playerOrder[i]->tank->reactivate_shield();
		}
	}

	// Set name and money position according to the maximum lengths
	score_name_pos  = 16 + (2 * env.fontHeight) + max_team_len;
	score_money_pos = score_name_pos + (2 * env.fontHeight) + max_name_len;

	// FPS is shown on the right top corner:
	FPS_pos = env.screenWidth - text_length(font, "XXXX FPS ");
}


/// @brief the [e]nd [o]f [r]ound score board
/// If the stage is STAGE_ENDGAME, credits and points are awarded and the
/// function waits for user input before it returns
static inline void draw_eor_scoreboard()
{
	// Clear key buffer
	if (STAGE_ENDGAME == global.stage) {
		while ( keypressed() )
			readkey();
	}

	// check to see if we have a winner or we just got out early
	if ( (winner != WINNER_NO_WIN)
	  && !global.demo_mode
	  && !global.isCloseBtnPressed() ) {

		// Re-Check for winner - This might have changed due to
		// dying wrath devices that went off - should not happen. Really.
		if (STAGE_ENDGAME == global.stage) {
			check_winner();

			// Eventually credit winner(s)
			env.creditWinners(winner);
		}

		// Now the scores can be displayed
		int32_t lh = 14; // The line height. If you need to change it, do it here.
		int32_t pd = 10; // Padding. How much space to the board border.

		// Find out longest player name and score length do determine the
		// score board size and score entry positions
		char    head_name[5]   = "Name";
		char    head_score[30] = { 0 };
		snprintf(head_score, 29, " %6s %6s %6s %6s", "Kills", "Killed", "Diff", "Won");

		int32_t namLen      = text_length(font, head_name);
		int32_t scoLen      = text_length(font, head_score);

		for (int32_t z = 0; z < env.numGamePlayers; z++) {
			int32_t curLen = text_length(font, env.players[z]->getName());
			if (curLen > namLen)
				namLen = curLen;
			char scoTxt[30] = { 0 };
			snprintf(scoTxt, 29, " %6d %6d %6d %6d",
						env.players[z]->kills,
						env.players[z]->killed,
						env.players[z]->killed - env.players[z]->kills,
						env.players[z]->score);
			curLen = text_length(font, scoTxt);
			if (curLen > scoLen)
				scoLen = curLen;
		}

		// Now calculate the dimensions of our score board.
		int32_t w = namLen + scoLen + (2 * pd);
		int32_t h = ((env.numGamePlayers + 4) * lh) + (2 * pd);
		int32_t x = env.halfWidth  - (w / 2);
		int32_t y = env.halfHeight - (h / 2);

		// Draw the background and the border
		global.make_update (x, y, w, h);

		rectfill(global.canvas, x, y, x + w, y + h, BLACK);
		rect    (global.canvas, x, y, x + w, y + h, WHITE);
		rect    (global.canvas, x + 1, y + 1, x + w - 1, y + h - 1, GREY);

		// Show a hint to press a key if this is the endgame board
		if (STAGE_ENDGAME == global.stage) {
			global.make_update (x, y + h, w, env.fontHeight + 6);
			textout_centre_ex(global.canvas, font, "Press any key to exit",
			                  x + (w / 2) + 2, y + h + 6, BLACK, -1);
			textout_centre_ex(global.canvas, font, "Press any key to exit",
			                  x + (w / 2),     y + h + 4, SILVER, -1);
		}

		// Add the padding now, or it must be summed in everywhere!
		x += pd;
		y += pd;
		w -= 2 * pd;
		h -= 2 * pd;

		// First title line, the winner
		if (winner == WINNER_JEDI)
			textout_centre_ex (global.canvas, font, "Jedi Win!",
								env.halfWidth, y, WHITE, -1);
		else if (winner == WINNER_SITH)
			textout_centre_ex (global.canvas, font, "Sith Win!",
								env.halfWidth, y, WHITE, -1);
		else if (winner == WINNER_DRAW)
			textout_centre_ex (global.canvas, font, "Draw",
								env.halfWidth, y, WHITE, -1);
		else
			textprintf_centre_ex (global.canvas, font,
									env.halfWidth, y,
									env.players[winner]->color, -1, "%s: %s",
									env.ingame->Get_Line(47),
									env.players[winner]->getName());

		// Second title line: The score is to follow. (Is this needed?)
		textout_right_ex (global.canvas, font, env.ingame->Get_Line(50),
							env.halfWidth, y + (2 * lh), WHITE, -1);

		// to make the following easier, skip the three used lines
		// (two titles, one blank)
		y += 3 * lh;

		// Third title line, the score board header
		int32_t scoStart = x + namLen;
		int32_t scoWidth = scoLen / 4;

		textout_ex    (global.canvas, font, "Name", x, y, WHITE, -1);
		textprintf_right_ex (global.canvas, font, scoStart + (1 * scoWidth),
							y, GREEN, -1, " %6s", "Kills");
		textprintf_right_ex (global.canvas, font, scoStart + (2 * scoWidth),
							y,   RED, -1, " %6s", "Killed");
		textprintf_right_ex (global.canvas, font, scoStart + (3 * scoWidth),
							y, WHITE, -1, " %6s", "Diff");
		textprintf_right_ex (global.canvas, font, scoStart + (4 * scoWidth),
							y, WHITE, -1, " %6s", "Won");


		// Create the score order.
		// The scores are ordered by score->diff->kills->killed->name
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

		// If this is the end-board, display here and wait for user input:
		if (STAGE_ENDGAME == global.stage) {
			global.do_updates();

			// Wait until a key is pressed
			while (!keypressed() && !mouse_b)
				LINUX_REST;

			// Clear key buffer
			while ( keypressed() )
				readkey();
		}

		// Clean up
		delete [] score_array;
	} // End of handling winner display
}


/** @brief sort players by scores.
  *
  * The return value is the pointer to the allocated array, users
  * must use its prev() pointer to find the head entry.
  *
  * @return a pointer to the scores array. This must be deleted.
**/
sScore* sort_scores()
{
	sScore* scores     = new sScore[env.numGamePlayers];
	sScore* score_head = scores;
	sScore* score_tail = scores;
	sScore* curr       = nullptr;

	for (int32_t z = 0; z < env.numGamePlayers; z++) {
		curr          = score_head;
		scores[z]     = *(env.players[z]);
		scores[z].idx = z; // The game index is needed.

		// Walk to find a lower score:
		while (curr
				&& (curr->score > scores[z].score))
			curr = curr->next;

		// Walk to find a lower diff:
		while (curr
				&& (curr->score == scores[z].score)
				&& (curr->diff   > scores[z].diff))
			curr = curr->next;

		// Walk to find a lower kills value:
		while (curr
				&& (curr->score == scores[z].score)
				&& (curr->diff  == scores[z].diff)
				&& (curr->kills  > scores[z].kills))
			curr = curr->next;

		// Walk to find a higher killed value:
		while (curr
				&& (curr->score == scores[z].score)
				&& (curr->diff  == scores[z].diff)
				&& (curr->kills == scores[z].kills)
				&& (curr->killed < scores[z].killed))
			curr = curr->next;

		// Walk to find a higher name value:
		while (curr
				&& (curr->score  == scores[z].score)
				&& (curr->diff   == scores[z].diff)
				&& (curr->kills  == scores[z].kills)
				&& (curr->killed == scores[z].killed)
				&& (strcmp(curr->name, scores[z].name) < 0))
			curr = curr->next;

		// If there is a curr, sort the new score before it.
		if (curr && (curr != &scores[z])) {
			scores[z].prev = curr->prev;
			scores[z].next = curr;
			if (scores[z].prev)
				scores[z].prev->next = &scores[z];
			curr->prev = &scores[z];
			if (score_head == curr)
				score_head = &scores[z];
		}

		// Otherwise this is the new tail:
		else if (score_tail != &scores[z]) {
			scores[z].prev   = score_tail;
			score_tail->next = &scores[z];
			score_tail       = &scores[z];
		}
	} // End of sorting scores

	return scores;
}


static inline void update_display()
{
	if (show_frame) {
		// do not show custom mouse cursor while drawing
		SHOW_MOUSE(nullptr)

		set_clip_rect(global.canvas, 0, 0,
		              env.screenWidth - 1, env.screenHeight - 1);

		if (update_screen) {
			update_screen = false;
			global.make_fullUpdate();
			global.updateMenu = true;
		}
		global.replace_canvas();
	}
}




static inline void update_objects(ObjectUpdater* upd)
{
	// Start all updater threads
	for (int32_t class_ = 0; class_ < CLASS_COUNT; ++class_)
		upd[class_].start();

	// Wakeup all at once:
	updMutex.lock();
	updCondition.notify_all();
	updMutex.unlock();

	// Wait for the threads to finish
	bool has_thread = true;
	while (has_thread) {
		has_thread = false;
		for (int32_t class_ = 0; class_ < CLASS_COUNT; ++class_) {
			if (!upd[class_].hasDone())
				has_thread = true;
		}
		if (has_thread)
			std::this_thread::yield();
	}

	// Reset SDI shot status on all tanks
	TANK* lt = nullptr;
	global.getHeadOfClass(CLASS_TANK, &lt);
	while (lt) {
		if (lt->player)
			lt->player->sdi_has_fired.store(false, ATOMIC_WRITE);
		lt->getNext(&lt);
	}
}


/// Level Creator Methods implementation
LevelCreator::LevelCreator()
{
	for (int32_t i = 0; i < 4; ++i)
		in_progress[i] = false;
}


/// The operator is just a wrapper.
void LevelCreator::operator()()
{
	fiVal = 1;
	set_level_settings(this);
	fiVal = 0;
}

void LevelCreator::add_fi()
{
	fiLock.lock();
	++fiVal;
	fiLock.unlock();
}

bool LevelCreator::can_work() const
{
	return !i_shall_die;
}

void LevelCreator::die_now()
{
	i_shall_die = true;
}

bool LevelCreator::has_progress()
{
	fiLock.lock();
	bool result = (fiVal > 0);
	fiVal = 0;
	fiLock.unlock();

	return result;
}

bool LevelCreator::is_finished() const
{
	return in_progress[3];
}

void LevelCreator::print_state() const
{
	if (in_progress[0]) {
		draw_sprite (global.canvas, env.misc[1], env.halfWidth - 120, env.halfHeight + 115);
		textout_centre_ex(global.canvas, font, env.ingame->Get_Line(42),
							env.halfWidth, env.halfHeight + 116, WHITE, -1);
		global.make_update(env.halfWidth - 120, env.halfHeight + 115,
		                   env.misc[1]->w, env.misc[1]->h);
	}

	if (in_progress[1]) {
		draw_sprite(global.canvas, env.misc[1], env.halfWidth - 120, env.halfHeight + 155);
		textout_centre_ex(global.canvas, font, env.ingame->Get_Line(44),
							env.halfWidth, env.halfHeight + 156, WHITE, -1);
		global.make_update(env.halfWidth - 120, env.halfHeight + 155,
			env.misc[1]->w, env.misc[1]->h);
	}


	if (in_progress[2]) {
		draw_sprite(global.canvas, env.misc[1], env.halfWidth - 120, env.halfHeight + 195);
		textout_centre_ex(global.canvas, font, env.ingame->Get_Line(43),
							env.halfWidth, env.halfHeight + 196, WHITE, -1);
		global.make_update(env.halfWidth - 120, env.halfHeight + 195,
			env.misc[1]->w, env.misc[1]->h);
	}
}

/// @brief Tell the LevelCreator that it does not need to yield any more
void LevelCreator::work_alone()
{
	i_must_yield = false;
}


void LevelCreator::working_on(int32_t what)
{
	if ( (what > 0) && (what < 5) ) {
		add_fi();
		in_progress[what - 1] = true;
	}
}


/// @brief yield if it is not working alone
void LevelCreator::yield()
{
	if (i_must_yield)
		std::this_thread::yield();
}
