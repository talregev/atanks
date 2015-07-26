#pragma once
#ifndef ATANKS_SRC_AICORE_H_INCLUDED
#define ATANKS_SRC_AICORE_H_INCLUDED

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
 *
 */

/** @file aicore.h
  * @brief Home of the AICore class
  *
  * This class substituted the old AI code in the PLAYER class.
  *
  * While the old AI worked pretty well in most situations, it did
  * lag the game on more intense calculations.
  * Further there were some situations (like peaks in the way and
  * such things) that the old AI could not handle.
  * It could have been fixed, but would have lagged the game even more.
  *
  * By outsourcing the AI to its own class and providing an
  * operator()(), the AI calculations could be put in a background
  * thread. This resulted in a much smoother gaming experience.
  *
  * Further the AI is now easier to debug, to maintain and to extend
  * than it was possible with the old AI code.
**/


#include "player_types.h"
#include "globaltypes.h"
#include "floattext.h"

#include <condition_variable>

// A few often needed randomization shortcuts
#define RAND_AI_0P       (rand() % ai_level)
#define RAND_AI_0N (0 == (rand() % ai_level))
#define RAND_AI_1P       (rand() % (ai_level + 1))
#define RAND_AI_1N (0 == (rand() % (ai_level + 1)))

// Init and check for round scores
#define NEUTRAL_ROUND_SCORE -1000000

#ifndef HAS_PLAYER
class PLAYER;
struct sOpponent;
#endif // HAS_PLAYER
class TANK;

// These are restricted to aicore.cpp, as
// they are of no use anywhere else. - sed
struct sItemListEntry;
struct sOppMemEntry;
struct sWeapListEntry;


/** @class AICore
  * @brief Core AI, written to be used as a background thread.
  *
  * The AI operator() is the main function that does all the work.
  *
  * If you need to edit something in that work flow, please try to
  * keep the documentation up-to-date.
  *
  * Work Flow:
  *
  * Initialization in initialize():
  * --------------------------------
  * needSuccess : This is set to true and later set to false if a full
  *               targeting round resulted in a primary target hit, or the full
  *               score is greater than zero.
  *
  * Initialization in operator():
  * --------------------------------
  * findTgtAttempts : Targetting attempts. Number of full cycles of finding an
  *                   attack setup.
  *                   Counted using tgt_attempts in the operator() loop.
  * findOppAttempts : Opponent attempts. How many opponents are tried per
  *                   targeting attempt.
  *                   Counted using opp_attempts in the operator() loop.
  * findWeapAttempts: Weapon attempts. How many weapons / items are tried per
  *                   opponent attempt.
  *                   Counted using weap_attempts in the operator() loop.
  * findRngAttempts : Range find attempts. How many corrections to the aiming
  *                   are done per weapon attempt.
  *                   Counted using rng_attempts in aim() loop.
  * As the full number of aiming can be up to Tgt*Opp*Weap*Rng attempts, the
  * current targeting attempt is finished once a new best attack plan is found.
  *
  * Cycle in operator():
  * --------------------------------
  *   1  If both opp_attempts and weap_attempts are 0, advance tgt_attempt. All
  *      three selections, mem, weap and item, are set to nullptr here.
  *
  *   2  setupAttack() - is_last is true when tgt_attempts equals findTgtAttempts.
  *                    - opp_attempts and weap_attempts are submitted as references.
  *      Here the basic setup is done by selecting an opponent and a suitable item
  *      or weapon to use against them.
  *     2.1  selectTarget() - Select a new target if no target is selected or
  *          weap_attempts is 0. The latter is true after weap_attempts reaches
  *          findWeapAttempts. item and weap are set to nullptr before selecting
  *          a new target.
  *       2.1.1  If shocked, the shocker is forced to be selected.
  *       2.1.2  If the current or last target weren't a set revengee, preselect them.
  *       2.1.3  Otherwise select the next known opponent that is not the revengee.
  *     2.2  Otherwise the current target is kept and mem_last is set to mem_curr.
  *     2.3  If a new target was selected, set weap and item to nullptr and generate
  *          new score values for all weapons and items. The scores depend in many
  *          places on the selected target, so the lists must be rebuild.
  *     2.4  selectItem() - This is cycled until it returns true or weap_attempts
  *          equals findWeapAttempts. is_last is true if the current weap_attempts
  *          value equals findWeapAttempts, is_last from setupAttack() is true, and
  *          the member needSuccess is true.
  *       2.4.1  Remember last item and weapon selection in weap_last and item_last.
  *       2.4.2  If a setup with hit primary target is already known, select a random
  *              weapon. Otherwise do an ordered cycle by weapon score.
  *       2.4.3  If shocked, that's it. No other selection done. weap_curr forced to
  *              be valid, then true is returned.
  *       2.4.4  cycle ordered through the items.
  *       2.4.5  Block items with negative scores, teleporters unless buried and
  *              riot weapons unless blocked.
  *       2.4.6  If both weapon and item are set, take the one with the higehr score.
  *       2.4.7  If this all failed on a last attempt, force select small missile.
  *     2.5  If this was the last weapon selection attempts, but there are opponent
  *          selection tries left, reset weap_attempts to zero so a new target is
  *          selected in the next cycle in setupAttack().
  *     2.6  End the cycle if either both opponent and item selection succeeded, or
  *          if both opp_attempts and weap_attempts equal their maximum number of
  *          tries.
  *     2.7  If this did not work out but is_last is true, try to get somewhere else
  *          using teleporters, or, if that is not possible, use a small missile
  *          against the highest rated opponent.
  *     2.8  If a self destruct item or weapon is selected, do three extra checks to
  *          "chicken out". If not, set mem_curr to the player itself.
  *     2.9  If this wasn't an is_last forced attempt and both opponent and item
  *          selection attempts reached their limits, reset both so a new targetting
  *          cycle is triggered in operator().
  *   3  calcAttack() - the current tgt_attempts is submitted.
  *      Here the attack is calculated according to the selected opponent and item.
  *      is_last is true if this is the last tgt_attempt and needSuccess is true.
  *     3.1  Reset isBlocked and needAim to false, as a new calculation is possible.
  *     3.2  If an item is selected, simply copy current angle and power to
  *          curr_angle and curr_power and retuurn true.
  *     3.3  If the current opponent and weapon are the same as in the last game
  *          turn, simply copy back the angle and power used then. With weapons, set
  *          needAim to true. Then return true.
  *     3.4  calcLaser() - Selected for lasers if the tank is not buried.
  *                      - is_last is submitted.
  *       3.4.1  Backup curr_angle and curr_power in case the shot gets cancelled.
  *       3.4.2  Do a mind shot to see where the laser ends.
  *       3.4.3  Calculate the hit damage and score of the first end point
  *       3.4.4  Cancel the attack if the shot is blocked by a dirt wall.
  *              Set needAim to true (isBlocked is not changed) and return false.
  *       3.4.5  Cancel the attack if the hit score is not positive, another
  *              tank than the target is hit, then.
  *              Set needAim to true (isBlocked is not changed) and return false.
  *       3.4.6  If this succeeded, write back angle and power and store the hit_score
  *              in best_round_score.
  *     3.5  calcUnbury() - Selected if the tank is buried.
  *                       - is_last is submitted.
  *       3.5.1  Select and setup a suitable tool to free the tank.
  *     3.6  calcKamikaze() - Selected if mem_curr is the player themselves.
  *       3.6.1  Make sure the selections are in order and make sense.
  *     3.7  calcStandard() - Selected in every other situation.
  *                         - is_last is submitted. allow_flip is true when the
  *                           current attempt to aim at this target is even.
  *       3.7.1  Set needAim to true and find a raw angle to begin with.
  *       3.7.2  If shooting through a wrap wall is shorter, flip the shot.
  *       3.7.3  If allow_flip is true and the wall is not steel, flip the shot.
  *       3.7.4  Adjust the angle giving shooting clearance.
  *       3.7.5  If the resulting angle is too high, and no positive setup score
  *              has been found, yet, trigger obstacle removal if this is the last
  *              attempt. isBlocked is set to true, then.
  *       3.7.6  If the angle finding was successful, calculate a raw power to begin
  *              aiming with.
  *       3.7.7  If the shot is blocked on is_last, write back angle/power and set
  *              needAim to be false.
  *     3.8  calcBoxed() - Used after calcStandard() if boxed mode is activated,
  *                        the shot is not blocked, yet, and a weapon is chosen.
  *                      - is_last is submitted.
  *       3.8.1  Reduce angle and/or power a bit as long as the shot crashes into
  *              the ceiling.
  *       3.8.2  If on a last_attempt, the shot does not crash and does not reach
  *              target by at least 2/3 the distance, try to clear the path.
  *              isBlocked is set to true, and needAim to false, then.
  *   4  aim() - true is submitted if tgt_attempts has reached findTgtAttempts
  *              and needSuccess is still true.
  *     4.1  return at once if needAim is false or isBlocked is true.
  *     4.2  Try to optimize angle and power until findRngAttempts are used.
  *     4.3  If on is_last no positive score is achieved and no best setup
  *          with a positive score was found, and the best overshoot does not
  *          reach the target by at least 2/3 the distance, try to free the
  *          firing path. isBlocked is set to true and needAim to false then.
  *     4.4  If a positive score was achieved or this is the last attempt without
  *          being blocked, write back best score, angle and power.
  *   5  If the best round score is positive, and at least ai_level weapons have
  *      been tried or all opponent attempts are used, reset opp_attempts and
  *      weap_attempts to end this tgt_attempt.
  *   6  If the best round score is positive and the primary target was hit,
  *      add the opponent score, divided by ten times the ai_level or the single
  *      ai_level if hitting the revengee to the score. Further add the weapon
  *      if damaging divided by ten times the ai_level.
  *   7  If a new best setup score with primary target hit, or if this is the last
  *      attempt and needSuccess is still true, store the current setting in the
  *      best_setup_* members.
  *      If the score is positive or the primary target was hit, needSuccess
  *      is set to false.
  *   8  If the best score is below zero while needAim is still true and isBlocked
  *      is still false and the bot is not shocked, start a freeing attempt.
  *      needAim is the set to false and isBlocked to true.
  *   9  If needAim is still true and isBlocked still false, write back best_setup_*
  *      members to the current setup.
**/
class AICore
{
public:
	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	explicit AICore();
	~AICore();

	// No copying, no assignment
	AICore(const AICore&) = delete;
	AICore &operator=(const AICore&) = delete;


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	PLAYER* active_player() const;
	void    allowText    ();
	bool    can_work     () const;
	void    forbidText   ();
	bool    hasExited    () const;
	bool    start        (PLAYER* player_);
	bool    status       (int32_t &aItem, int32_t &aAngle,
	                      int32_t &aPower, ePlayerStages &pl_stage);
	void    stop         ();
	void    weapon_fired ();

	void    operator()   ();


	/* ----------------------
	 * --- Public members ---
	 * ----------------------
	 */

private:

	typedef ePlayerStages             plStage_t;
	typedef sItemListEntry            itEntry_t;
	typedef sOppMemEntry              opEntry_t;
	typedef sWeapListEntry            weEntry_t;
	typedef std::mutex                mutex_t;
	typedef std::condition_variable   condv_t;
	typedef std::lock_guard<mutex_t>  lguard_t;
	typedef std::unique_lock<mutex_t> luniq_t;


	/* -----------------------
	 * --- Private methods ---
	 * -----------------------
	 */

	bool        aim            (bool is_last);
	bool        calcAttack     (int32_t attempt);
	bool        calcBoxed      (bool is_last);
	void        calcHitDamage  (int32_t hit_x, int32_t hit_y, double weap_rad,
	                            double dmg, weaponType weapType);
	int32_t     calcHitScore   (weaponType weapType, bool is_last);
	bool        calcKamikaze   (bool is_last);
	bool        calcLaser      (bool is_last);
	bool        calcOffset     (bool is_last);
	bool        calcStandard   (bool is_last, bool allow_flip_shot);
	bool        calcUnbury     (bool is_last);
	void        checkItemMem   ();
	void        checkOppMem    ();
	void        checkWeapMem   ();
	void        destroy        ();
	void        fixCrashed     (int32_t &ang_mod, int32_t &pow_mod);
	void        fixOvershoot   (int32_t &ang_mod, int32_t &pow_mod,
	                            int32_t hit_score);
	void        fixUnfinished  (int32_t &ang_mod, int32_t &pow_mod);
	const char* getLevelName   (int32_t level) const;
	bool        getMemory      ();
	void        noteBestScore  (int32_t &ang_mod, int32_t &pow_mod,
	                            int32_t hit_score);
	bool        initialize     ();
	void        sanitizeCurr   ();
	bool        selectItem     (bool is_last);
	bool        selectTarget   (bool is_last);
	bool        setupAttack    (bool is_last, int32_t &opp_attempt,
	                            int32_t &weap_attempt);
	void        showFeedback   (const char* const feedback, int32_t col,
	                            double yv, eTextSway text_sway, int32_t dur);
	void        traceCluster   (int32_t subType, int32_t subCount,
	                            int32_t sub_x, int32_t sub_y,
	                            double inh_xv, double inh_yv);
	bool        traceShot      (int32_t trace_angle,
	                            bool &finished, bool &top_wrapped,
	                            int32_t &reached_x, int32_t &reached_y,
	                            double &end_xv, double &end_yv);
	void        traceWeapon    (int32_t &reached_x, int32_t &reached_y,
	                            int32_t &has_crashed, int32_t &has_finished);
	void        updateItemScore(itEntry_t* pItem);
	void        updateOppScore (opEntry_t* pOpp);
	void        updateWeapScore(weEntry_t* pWeap);
	bool        useFreeingTool (bool free_tank, bool is_last);
	bool        useItem        (itemType item_type);
	bool        useItem        (int32_t item_index);
	bool        useWeapon      (weaponType weap_type);
	bool        useWeapon      (int32_t weap_index);


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	// Internal values
	mutex_t    actionMutex;
	condv_t    actionCondition;
	volatile
	bool       canWork          = true;
	int32_t    curr_angle       = 90;    //!< The angle that is currently tested
	int32_t    curr_overshoot   = 0;     //!< Current calculated distance of hit versus opponent
	int32_t    curr_power       = 0;     //!< The power that is currently tested
	bool       curr_prime_hit   = false; //!< Whether the primary target was hit.
	double     errorMultiplier  = 0.;    //!< Default error reduction according to AI level
	int32_t    findOppAttempts  = 0;     //!< Number of attempts to select a suitable opponent
	int32_t    findRngAttempts  = 0;     //!< Number of attempts to aim the current selection
	int32_t    findTgtAttempts  = 0;     //!< Number of attempts to come up with an attack plan
	int32_t    findWeapAttempts = 0;     //!< Number of attempts to find a suitable item/weapon
	double     focusRate        = 0.;    //!< How good a bot can focus on a specific task
	bool       isBlocked        = false; //!< Set to true if a shot can't get through
	volatile
	bool       isFinished       = false; //!< Set to true when operator() ends
	bool       isShocked        = false;
	volatile
	bool       isStopped        = false;
	volatile
	bool       isWorking        = false;
	int32_t    maxBounce        = 0;     //!< How many wall bounces/wraps can be calculated
	bool       needAim          = false; //!< true if this is a standard shot
	bool       needSuccess      = true;  //!< true unless a best score is achieved
	int32_t    offset_x         = 0;
	int32_t    offset_y         = 0;
	int32_t    overshoot        = 0;     //!< Overshoot value of currently best angle and power
	plStage_t  plStage          = PS_AI_IS_IDLE;
	sOpponent* revengee         = nullptr; //!< If set, it is tried first as a target
	sOpponent* shocker          = nullptr; //!< The current fear shock winner
	abool_t    textAllowed;                //!< Is new FLOATTEXT allowed?
	int32_t    weap_idx         = SML_MIS;

	// Values taken from the player and their tank
	int32_t    ai_level  = 0;       //!< To not having to cast from player type.
	double     ai_overmod= 0.;      //!< modifier for overkills and similar
	int32_t    angle     = 90;      //!< The currently determined best angle
	double     blast_min = 0.;      //!< Damage done by small missile
	double     blast_med = 0.;      //!< Damage done by medium or large missile
	double     blast_big = 0.;      //!< Damage done by small nuke or nuke
	double     blast_max = 0.;      //!< Damage done by death head
	int32_t    buried    = 0;       //!< Full buried level
	int32_t    buried_l  = 0;       //!< left side buried level
	int32_t    buried_r  = 0;       //!< right side buried level
	double     currLife  = 0.;
	itEntry_t* item_curr = nullptr; //!< Currently selected entry
	itEntry_t* item_head = nullptr; //!< Last selected entry
	itEntry_t* item_last = nullptr; //!< Entry with highest score
	int32_t    last_ang  = 0;       //!< Angle used in last round
	sOpponent* last_opp  = nullptr; //!< The opponent attacked in the last round
	int32_t    last_pow  = 0;       //!< Power used in last round
	int32_t    last_weap = 0;       //!< weapon used in the last round
	int32_t    maxLife   = 100;
	opEntry_t* mem_curr  = nullptr; //!< Currently selected entry
	opEntry_t* mem_head  = nullptr; //!< Last selected entry
	opEntry_t* mem_last  = nullptr; //!< Entry with highest score
	bool       needMoney = false;   //!< Might alter some decisions
	PLAYER*    player    = nullptr;
	int32_t    power     = 0;       //!< The currently determined best power
	TANK*      tank      = nullptr;
	double     type_mod  = 1.;
	weEntry_t* weap_curr = nullptr; //!< Currently selected entry
	weEntry_t* weap_head = nullptr; //!< Last selected entry
	weEntry_t* weap_last = nullptr; //!< Entry with highest score
	double     x         = 0.;
	double     y         = 0.;

	// Values to remember settings and situations over aiming rounds
	int32_t    best_angle      = 0;
	int32_t    best_power      = 0;
	int32_t    best_score      = NEUTRAL_ROUND_SCORE;
	bool       hill_detected   = false;
	int32_t    last_ang_mod    = 0;
	int32_t    last_overshoot  = MAX_OVERSHOOT;
	int32_t    last_pow_mod    = 0;
	bool       last_reverted   = false;
	int32_t    last_score      = 0;
	bool       last_was_better = false;
	int32_t    reached_x       = x;
	int32_t    reached_y       = y;

	// Values used to memorize the best setup in a round
	int32_t    best_round_score = 0;
	int32_t    best_setup_angle = 0;
	itEntry_t* best_setup_item  = nullptr;
	opEntry_t* best_setup_mem   = nullptr;
	int32_t    best_setup_power = 0;
	bool       best_setup_prime = false;
	int32_t    best_setup_score = 0;
	weEntry_t* best_setup_weap  = nullptr;
};

#endif // ATANKS_SRC_AICORE_H_INCLUDED

