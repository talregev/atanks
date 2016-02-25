#include "aicore.h"
#include "player.h"
#include "tank.h"
#include "missile.h"
#include "beam.h"
#include "explosion.h"

#include <cassert>


/// @brief Maximum AI Level is the highest level being lucky, thus +1.
const int32_t maxAiLevel = DEADLY_PLAYER + 1;


/** @struct sItemListEntry
  * @brief doubly linked list element to organize the AIs item preferences.
**/
struct sItemListEntry
{
	int32_t         amount     = 0;       //!< Number of items in stock.
	bool            escape     = false;   //!< Set to true if "getting away" points are awarded.
	bool            kamikaze   = false;   //!< Set to true if self destruct points are awarded.
	sItemListEntry* next       = nullptr;
	int32_t         preference = 0;       //!< Shortcut to the players preferences.
	sItemListEntry* prev       = nullptr;
	int32_t         score      = 0;       //!< How likely the AI uses this item.
	bool            selectable = false;   //!< Some are not selectable, like parachutes.
	int32_t         type       = 0;       //!< The (enum) itemType of the item

	explicit sItemListEntry(sItemListEntry* prev_);
	~sItemListEntry();

	const char* getName() { return item[type].getName(); }
};


/** @struct sOppMemEntry
  * @brief doubly linked list element to organize the AIs opponent memory.
**/
struct sOppMemEntry
{
	bool          alive       = true;    //!< False if the tank is destroyed.
	int32_t       attempts    = 0;       //!< How often tried to hit this round.
	int32_t       buried_l    = 0;       //!< Buried level to the left.
	int32_t       buried_r    = 0;       //!< Buried level to the right.
	double        diffLife    = 0.;      //!< Difference to bots life value: (this - opp).
	double        distance    = 0.;      //!< Shortcut to the absolute distance between both tanks.
	int32_t       dmgDone     = 0;       //!< damage done in simulation to calculate hit score.
	sOpponent*    entry       = nullptr; //!< The AIs sOpponent memory (see players.h).
	bool          hasRepulse  = false;   //!< Whether or not the opponent has a repulsor shield up.
	sOppMemEntry* next        = nullptr;
	bool          is_buried   = 0;       //!< Whether buried_l+buried_r is greater than BURIED_LEVEL.
	bool          onSameTeam  = false;   //!< True if on the same team as the player.
	double        opLife      = 0.;      //!< Full opponents life, which is tank->sh + tank->l.
	double        opX         = 0;       //!< X-coordinate of the opponents tank.
	double        opY         = 0;       //!< Y-coordinate of the opponents tank.
	sOppMemEntry* prev        = nullptr;
	bool          revengeDone = false;   //!< Wether the score has already taken revenge into account.
	int32_t       score       = 0;       //!< How likely the AI attacks this opponent.
	double        team_mod    = 1.;      //!< Multiplier for the score according to which teams both belong to.

	explicit sOppMemEntry(sOppMemEntry* prev_);
	~sOppMemEntry();

	const char* getName() { return entry->opponent->getName(); }
};


/** @struct sWeapListEntry
  * @brief doubly linked list element to organize the AIs weapon preferences
**/
struct sWeapListEntry
{
	int32_t         amount     = 0;     //!< Number of weapons in stock.
	bool            blastOut   = false; //!< Set to true if blasting out points are awarded.
	int32_t         delay      = 0;     //!< Used to track delayed weapons.
	double          dmgCluster = 0.;    //!< Cluster full damage.
	double          dmgSingle  = 0.;    //!< Single shot damage.
	double          dmgSpread  = 0.;    //!< Spread full damage.
	bool            kamikaze   = false; //!< Set to true if self destruct points are awarded.
	sWeapListEntry* next       = nullptr;
	int32_t         preference = 0;     //!< Shortcut to the players preferences.
	sWeapListEntry* prev       = nullptr;
	int32_t         radius     = 0;     //!< Blast radius of the weapon.
	int32_t         score      = 0;     //!< How likely the AI uses this weapon.
	int32_t         spread     = 1;     //!< Checked weapon spread value. (See AICore::getMemory())
	int32_t         subMunCount= 0;     //!< Number of sub munition "bomblets"
	int32_t         subMunType = -1;    //!< Clusters and such have sub munition.
	int32_t         type       = 0;     //!< The (enum) weaponType of the weapon.

	explicit sWeapListEntry(sWeapListEntry* prev_);
	~sWeapListEntry();

	const char* getName() { return weapon[type].getName(); }
};


/// @brief Template swapper, the types just need prev/next pointers
template<typename T> void swap_entries(T* lhs, T* rhs)
{
	if (lhs && rhs && (lhs != rhs)) {
		// backup neighbourhood (and use as short cuts ;-) )
		T* l_next = lhs->next;
		T* l_prev = lhs->prev;
		T* r_next = rhs->next;
		T* r_prev = rhs->prev;

		// Insert rhs into lhs location
		if (l_next && (l_next != rhs)) l_next->prev = rhs;
		if (l_prev && (l_prev != rhs)) l_prev->next = rhs;

		// Insert lhs into rhs location
		if (r_next && (r_next != lhs)) r_next->prev = lhs;
		if (r_prev && (r_prev != lhs)) r_prev->next = lhs;

		// Move rhs to lhs location
		rhs->next = l_next == rhs ? lhs : l_next;
		rhs->prev = l_prev == rhs ? lhs : l_prev;

		// Move lhs to (former) rhs location
		lhs->next = r_next == lhs ? rhs : r_next;
		lhs->prev = r_prev == lhs ? rhs : r_prev;
	}
}


/// @brief Template sorter, the types need prev, next and score.
/// Sorting is done by score in descending order. If *head is sorted
/// down the list, it is set to the new first element.
template<typename T> void sort_entries(T** head)
{
	if (!head || !(*head) )
		return;

	bool sorted = false;

	while (!sorted) {

		// Yield on each iteration to not hog the CPUs
		if (!global.skippingComputerPlay)
			std::this_thread::yield();

		sorted = true;

		T* curr = *head;
		T* next = curr->next;

		while (next) {
			if (next->score > curr->score) {
				sorted = false;
				swap_entries(curr, next);
				if (*head == curr)
					*head = next;
			} else
				curr = next;
			next = curr->next;
		} // End of having next
	} // end of not sorted

#if defined(ATANKS_DEBUG_AIMING) || defined(ATANKS_DEBUG_EMOTIONS)
	DEBUG_LOG_AI("Memory Sorting", "Sorting results:", 0)
	T* curr = *head;
	int32_t nr = 1;
	while (curr) {
		if (curr->score > -10000) {
			DEBUG_LOG_AI("Memory Sorting", "% 3d: Score %5d (%s)",
						 nr++, curr->score, curr->getName())
			curr = curr->next;
		} else
			curr = nullptr;
	}
#endif // ATANKS_DEBUG_AIMING || ATANKS_DEBUG_EMOTIONS
}


/// @brief AICore default constructor
AICore::AICore() :
	textAllowed(ATOMIC_VAR_INIT(true))
{
	// As the opponent counts, and both weapons and items
	// list sizes are fixed, memory is reserved here.

	/// 1) Items
    for (int32_t i = 0; canWork && (i < ITEMS); ++i) {
		try {
			item_curr = new itEntry_t(item_last);
			if (!item_head)
				item_head = item_curr;
			item_last = item_curr;
		} catch (std::exception &e) {
			cerr << "Unable to reserve " << sizeof(itEntry_t);
			cerr << " bytes for AI item chain: " << e.what() << endl;

			destroy();
			canWork = false;
		}
    }

	/// 2) Opponents
    for (int32_t i = 0; canWork && (i < env.numGamePlayers); ++i) {
		try {
			mem_curr = new opEntry_t(mem_last);
			if (!mem_head)
				mem_head = mem_curr;
			mem_last = mem_curr;
		} catch (std::exception &e) {
			cerr << "Unable to reserve " << sizeof(opEntry_t);
			cerr << " bytes for AI memory chain: " << e.what() << endl;

			destroy();
			canWork = false;
		}
    }

	/// 3) Weapons
    for (int32_t i = 0; canWork && (i < WEAPONS); ++i) {
		try {
			weap_curr = new weEntry_t(weap_last);
			if (!weap_head)
				weap_head = weap_curr;
			weap_last = weap_curr;
		} catch (std::exception &e) {
			cerr << "Unable to reserve " << sizeof(weEntry_t);
			cerr << " bytes for AI weapon chain: " << e.what() << endl;

			destroy();
			canWork = false;
		}
    }

    // Stop if no work can be done
    isStopped = !canWork;

    DEBUG_LOG_AI("AICore", "Instance created", 0)
}


/// @brief AICore destructor
AICore::~AICore()
{
	if (isWorking) {
		if (!isStopped)
			this->stop();
		while (isWorking)
			std::this_thread::yield();
	}

	// Clean up memory chains:
	this->destroy();

	DEBUG_LOG_AI("AICore", "Instance destroyed", 0)
}


/// @brief return the currently active player or nullptr if not working
PLAYER* AICore::active_player() const
{
	if (isWorking)
		return player;
	return nullptr;
}


/** @brief aim the current selection
  * @param[in] is_last if set to true, the best result is accepted, no matter
  * what the outcome might be.
  * @return true if the aiming resulted in a usable hit.
**/
bool AICore::aim(bool is_last)
{
	plStage = PS_AIM;

	DEBUG_LOG_AIM(player->getName(), "Starting to aim %s at %s",
				weapon[weap_idx].getName(), mem_curr->entry->opponent->getName())

	int32_t attempt  = 0;

	// reset current values as there can be no guarantee that the
	// last selected combination works for the current weapon/opponent
	// selection.
	sanitizeCurr();
	hill_detected    = false;
	// Note: curr_overshoot is reset to MAX_OVERSHOOT in calcAttack() but
	// might have an actual traced value from calcBoxed(), so do not reset
	// it here again.


	// Reset aiming round memory
	best_score      = NEUTRAL_ROUND_SCORE;
	best_angle      = angle;
	best_power      = power;
	best_prime_hit  = false;
	best_overshoot  = MAX_OVERSHOOT;
	last_ang_mod    = 0;
	last_overshoot  = MAX_OVERSHOOT;
	last_pow_mod    = 0;
	last_reverted   = false;
	last_score      = 0;
	last_was_better = false;
	reached_x       = x;
	reached_y       = y;


	// loop until finished, forced off or ending unsuccessfully
	while (!isStopped && (++attempt <= findRngAttempts) ) {

		// Yield on each iteration to not hog the CPUs
		if (!global.skippingComputerPlay)
			std::this_thread::yield();

		int32_t hit_score      = 0;
		int32_t has_crashed    = 0;
		int32_t has_finished   = 0;

		// Modifications for this round:
		int32_t ang_mod =   1 + RAND_AI_1P; // [ 1;  7]
		int32_t pow_mod = (10 + RAND_AI_1P) // [10; 17]
						* curr_power / 100; // [10;340]

		DEBUG_LOG_AIM(player->getName(),
		              "[%d/%d] Angle % 3d, Power % 4d",
		              attempt, findRngAttempts,
		              GET_DISP_ANGLE(curr_angle), curr_power)

		// Lower ang_mod and pow_mod if the last overshoot isn't that high
		int32_t abs_last_overshoot = std::abs(last_overshoot);
		int32_t max_ang_mod        = abs_last_overshoot / (ai_level * 20) + 1;
		int32_t max_pow_mod        = abs_last_overshoot / (ai_level *  5) + 5;

		// max_pow_mod must be a power of five
		max_pow_mod += max_pow_mod % 5;

		if (ang_mod > max_ang_mod) {
			DEBUG_LOG_AIM(player->getName(),
			              " => ang_mod too high (%d / %d) ; reducing...",
			              ang_mod, max_ang_mod)
			ang_mod = max_ang_mod;
		}

		// pow_mod must be a power of five
		pow_mod += pow_mod % 5;

		if (pow_mod > max_pow_mod) {
			DEBUG_LOG_AIM(player->getName(),
			              " => pow_mod too high (%d / %d) ; reducing...",
			              pow_mod, max_pow_mod)
			pow_mod = max_pow_mod;
		}


		// See where we are going:
		traceWeapon(has_crashed, has_finished);
		hit_score = calcHitScore(is_last && needSuccess);


		// See whether this shot actually got nearer to the opponent.
		// (Note: Otherwise a better score just means less collateral damage!)
		bool is_nearer = std::abs(last_overshoot) >= std::abs(curr_overshoot);


		DEBUG_LOG_AIM(player->getName(),
		              "[%d/%d] => Score %d, Overshoot %d (%s [last: %d])",
		              attempt, findRngAttempts, hit_score, curr_overshoot,
		              is_nearer ? "Nearer" : "Farther", last_overshoot)


		// Note down a new best score:
		bool new_best_score = (hit_score > best_score);
		if ( ( new_best_score && curr_prime_hit)
		  || (!best_prime_hit && (new_best_score || curr_prime_hit) ) ) {
			// Note: Better score with prime hit, prime hit for the first time,
			//       or better score with prime never hit.
			DEBUG_LOG_AIM(player->getName(),
						  "[%d/%d] => New best score %d [%d best]",
						  attempt, findRngAttempts,
						  hit_score, best_score)

			sanitizeCurr();

			best_angle     = curr_angle;
			best_overshoot = curr_overshoot;
			best_power     = curr_power;
			best_prime_hit = curr_prime_hit;
			best_score     = hit_score;

			// The last modifications seem to have brought something
			ang_mod = (last_ang_mod + (SIGN(last_ang_mod) * ang_mod)) / 2;
			pow_mod = (last_pow_mod + pow_mod) / 2 * SIGN(best_overshoot);
		}

		// The outcome has to be checked:
		if (canWork && !isStopped) {

			/* The following situations can occur:
			 *
			 * A) The shot (or part of them) have not been finished.
			 *    In this case power must be reduced and the angle
			 *    has to be moved in a more neutral position if it
			 *    is too steep or flat.
			 * B) With steel or wrap wall the shot might crash into
			 *    a wrap ceiling or the wall and ceiling made of steel.
			 *    Generally this can only be fixed by lowering power and
			 *    getting away from 45° angles.
			 * C) The hit is nearer than the last one.
			 *    This is good. If it even has a positive hit_score,
			 *    it can be noted down as a new best hit.
			 * D) The hit is farther away. If the score is better,
			 *    the direction of modification seems correct.
			 *    Otherwise it might be better to revert those changes.
			 */

			// --- Situation A) The shot was not finished. ---
			//-------------------------------------------------
			if ( !has_finished // Pure situation A must be taken care of
			  || ( (has_finished < weap_curr->spread)
				&& ( (has_finished - RAND_AI_0P) < 0) ) ) {

				DEBUG_LOG_AIM(player->getName(),
								"[%d/%d] %d / %d finished, trying to correct",
								attempt, findRngAttempts,
								has_finished, weap_curr->spread)

				fixUnfinished(ang_mod, pow_mod);
				last_reverted   = (SIGN(last_ang_mod) != SIGN(ang_mod));
				last_was_better = false;
			} // End of having lost the shot prediction


			// --- Situation B) The shot(s) crashed       ---
			//------------------------------------------------
			else if ( (has_crashed == weap_curr->spread)
			       || ( (has_crashed > 0)
			         && ((has_crashed + RAND_AI_0P) >= weap_curr->spread)) ) {

				DEBUG_LOG_AIM(player->getName(),
								"[%d/%d] %d / %d crashed, trying to correct",
								attempt, findRngAttempts,
								has_crashed, weap_curr->spread)

				fixCrashed(ang_mod, pow_mod);

				// If there is a positive hit_score, halve it for every
				// shot that crashed:
				if (hit_score > 0) {
					for (int32_t i = 0; i < has_crashed; ++i) {
						if (RAND_AI_0P)
							hit_score /= 2;
					}
				}

				last_reverted   = (SIGN(last_ang_mod) != SIGN(ang_mod));
				last_was_better = false;
			}


			// --- Situation C) The shot is nearer to the target. ---
			//--------------------------------------------------------
			else if (is_nearer) {
				// Note: if this is a new best score, some adaptation has
				//       already been made above.
				if (hit_score < last_score) {
					DEBUG_LOG_AIM(player->getName(),
					              "[%d/%d] => Nearer but not a better score %d [%d best]",
					              attempt, findRngAttempts,
					              hit_score, best_score)

					// Just modify the new angle mod to mimic the last
					// with new values
					ang_mod = std::abs(ang_mod) * SIGN(last_ang_mod);

					last_was_better = true;
				} else
					last_was_better = false;

				last_reverted   = (SIGN(last_ang_mod) != SIGN(ang_mod));

				// pow_mod must have the opposite sign of the overshoot:
				pow_mod = std::abs(pow_mod) * SIGN(curr_overshoot) * -1;

				DEBUG_LOG_AIM(player->getName(),
							"[%d/%d] New angle mod %d, new power mod %d",
							attempt, findRngAttempts,
							ang_mod, pow_mod)
			} // End of having a nearer hit


			// --- Situation D) The shot hit farther away than the best. ---
			//---------------------------------------------------------------
			else {
				DEBUG_LOG_AIM(player->getName(),
				              "[%d/%d] Farther impact (%d curr, %d best)"
				              " [score %d]",
				              attempt, findRngAttempts, curr_overshoot,
				              best_overshoot, hit_score)

				fixOvershoot(ang_mod, pow_mod, hit_score);

				last_reverted = SIGN(ang_mod) != SIGN(last_ang_mod);

				DEBUG_LOG_AIM(player->getName(),
							"[%d/%d] New angle mod %d, new power mod %d",
							attempt, findRngAttempts,
							ang_mod, pow_mod)
			} // End of situation C


			// Try to fix 180° shots if no positive score was achieved:
			if ( (180 == curr_angle) && !ang_mod && (hit_score < 1) ) {
				ang_mod = SIGN(mem_curr->opX - x)
						* (RAND_AI_1P + 1)
						* -1.;
				DEBUG_LOG_AIM(player->getName(),
							  "Vertical shot detected, new angle mod %d",
							  ang_mod)
			}

			// Otherwise check if we actually reach a non-steel wall if the
			// shot was flipped.
			else if ( hasFlipped
			       && (std::abs(curr_overshoot) > std::abs(mem_curr->opX - x))
			       && (SIGN(reached_x - x) != SIGN(mem_curr->opX - x))) {
				DEBUG_LOG_AIM(player->getName(),
				              "Flip shot failed, flipping back from %d° to %d°",
				              GET_DISP_ANGLE(curr_angle),
				              GET_DISP_ANGLE(FLIP_ANGLE(curr_angle)))
				curr_angle = FLIP_ANGLE(curr_angle);
				hasFlipped = false;
			}

			// Power modification can be modified by a difference between
			// the overshoot and the actual modification according to
			// AI settings:
			// ----------------------------------------------------------
			double power_diff = (std::abs(curr_overshoot) - std::abs(pow_mod))
			                  / (std::abs(ang_mod) ? std::abs(ang_mod) : 1);

			if ( (curr_overshoot < MAX_OVERSHOOT)
			  && (power_diff > std::abs(pow_mod))
			  && (hit_score < 1) ) {
				DEBUG_LOG_AIM(player->getName(),
				              "Too low power mod difference %d"
				              " (overshoot %d, pow_mod %d)",
				              ROUND(power_diff), curr_overshoot, ROUND(pow_mod))

				pow_mod += power_diff * focusRate / 2. * SIGNd(pow_mod);

				if (std::abs(pow_mod) > max_pow_mod)
					pow_mod = max_pow_mod * SIGN(pow_mod);

				DEBUG_LOG_AIM(player->getName(),
				              std::abs(pow_mod) == max_pow_mod
				              ? "pow_mod %d at maximum!"
				              : "Hopefully fixed power mod: %d",
				              pow_mod)
			}


			// Make sure both modifications applied end in a sane results:
			// -----------------------------------------------------------

			// Test angle to the right
			if ( (curr_angle + ang_mod) < 90) {
				if (curr_angle > 90)
					// Just sanitize
					ang_mod = 90 - curr_angle;
				else
					// Pull up to try again from a very different view
					ang_mod = ROUNDu(60. * focusRate) // + [10;60]
					        - (RAND_AI_0P * 5);       // - [ 5;25]
			} // End of sanitizing angle right

			// Test angle to the left
			else if ( (curr_angle + ang_mod) > 270) {
				if (curr_angle < 270)
					// Just sanitize
					ang_mod = 270 - curr_angle;
				else
					// Pull up to try again from a very different view
					ang_mod = (-60. * focusRate) // - [10;60]
					        + (RAND_AI_0P * 5);  // + [ 0;25]
			} // End of sanitizing angle left


			// Test bottom power range
			if ( (curr_power + pow_mod) < MIN_POWER) {
				if (curr_power > MIN_POWER)
					// Just sanitize
					pow_mod = MIN_POWER - curr_power;
				else
					// Give more power to go somewhere else
					pow_mod = (900. * focusRate) // + [150;900]
					        - (RAND_AI_0P * 50); // - [  0;250]
			}

			// Test upper power range
			if ( (curr_power + pow_mod) > MAX_POWER) {
				if (curr_power < MAX_POWER)
					// Just sanitize
					pow_mod = MAX_POWER - curr_power;
				else
					// Give more power to go somewhere else
					pow_mod = (-900. * focusRate) // - [150;900]
					        + (RAND_AI_0P * 50);  // + [  0;250]
			}


			// Apply mods:
			curr_angle   += ang_mod;
			curr_power   += pow_mod;


			// Save current score, mods and overshot:
			last_ang_mod   = ang_mod;
			last_overshoot = curr_overshoot;
			last_pow_mod   = pow_mod;
			last_score     = hit_score;
		} // End of canWork and not isStopped


	} // end of aiming loop

	DEBUG_LOG_AIM(player->getName(),
				"Final score with angle %d, power %d : %d => %s%s",
				GET_DISP_ANGLE(best_angle), best_power, best_score,
				(best_score > 0) || (is_last && needSuccess) ? "Success!" : "Failure!",
				is_last && needSuccess ? " (is_last forced!)" : "")

	// If this was the last try and it did not reach the target having
	// a negative best score, assume that the path is blocked.
	// However, if a best setup is already known, revert to that.
	if ( (weap_idx < WEAPONS) && is_last && needSuccess
	  && (best_setup_score < 0)
	  && (best_round_score < 0)
	  && (best_score < 0)
	  && (best_overshoot < 0) // too short
	  && ( ( (reached_y > BOXED_TOP)                 // Not a ceiling crash,
	      && (-best_overshoot > weap_curr->radius) ) // but can't hit
	    || hill_detected ) /* if a hill was detected, it must be removed */ ) {
		bool free_tank = std::abs(reached_x - x) < weapon[RIOT_BLAST].radius;

		if (useFreeingTool(free_tank, is_last)) {
			needAim       = false;
			isBlocked     = true;
			hill_detected = true;

			if (free_tank)
				calcUnbury(is_last);
			else {
				// Write back best values
				curr_angle = best_angle;
				curr_power = best_power;

				// If this is a shot that got too short and a riot bomb
				// is chosen, flatten the angle to hit the mountain in between
				flattenCurrAng();

				// Now set the results:
				sanitizeCurr();
				angle      = curr_angle;
				power      = curr_power;
				DEBUG_LOG_AIM(player->getName(),
							  "Obstacle detected, trying to clear path using %s",
							  weap_idx < WEAPONS
							  ? weapon[weap_idx].getName()
							  : item[weap_idx - WEAPONS].getName())
			}
			return true;
		}
	}

	// Write back best values if this is a success:
	if (best_score > best_round_score) {
		best_round_score = best_score;
		curr_angle       = best_angle;
		curr_power       = best_power;
		sanitizeCurr();
	}


	return ( (best_round_score > 0) || (is_last && needSuccess) );
}


/// @brief Allow AICore to create FLOATTEXT instances
void AICore::allowText()
{
	textAllowed.store(true, ATOMIC_WRITE);
}


/** @brief calculate basic attack values or set up the last ones
  * @param[in] attempt If this equals findTgtAttempts, this method is forced to
  *            not check too harshly, so it always returns true.
  * @return true if the method came up with something sane.
**/
bool AICore::calcAttack(int32_t attempt)
{
	bool is_last   = ((attempt == findTgtAttempts) && needSuccess);
	plStage        = PS_CALCULATE;
	hasFlipped     = false;
	isBlocked      = false;
	needAim        = false;
	curr_overshoot = MAX_OVERSHOOT;
	offset_x       = 0;
	offset_y       = 0;

	// If an item is chosen over a weapon, nothing is to be done
	if (item_curr && !weap_curr) {
		// If an item is selected, write back the currently used
		// angle and power, nothing is to be changed now.
		curr_angle = angle;
		curr_power = power;
		return true;
	}

	assert(weap_curr
		&& "ERROR: weap_curr is nullptr in calcAttack() with no item chosen!");

	// If the currently chosen opponent is the one attacked
	// in the last round, simply copy back the old attack
	// values and be done
	if ( last_opp
	  && (last_opp->opponent != player) // don't repeat self destruct attempts
	  && (last_opp  == mem_curr->entry)
	  && (last_weap == weap_idx) ) {
		curr_angle = last_ang;
		curr_power = last_pow;

		if (weap_idx < WEAPONS)
			needAim = true;

		return true;
	}

	/* If the current target is different or there was no last target,
	 * a basic set of values must be generated.
	 *
	 * Outline:
	 * --------
	 * There are five possible scenarios:
	 * a) The tank is not buried (enough) and a laser is chosen:
	 *    -> a direct angle will do, make sure power is sane.
	 * b) The tank is buried and an appropriate tool is chosen:
	 *    -> fire tool at the most filled side or, if the difference is less
	 *       than the AI level, in the direction of the chosen opponent.
	 * c) Kamikaze
	 *    -> indicated by setting mem_curr to the own entry
	 *    -> if shaped weapon is chosen, fire 45° and power 150 to the side
	 *       where the terrain height is nearest to this tanks bottom.
	 *    -> if napalm is chosen, fire against the wind with power 100 - 300
	 *    -> otherwise fire 180° and power 200 + spread modification.
	 * d) Fire in non-boxed mode
	 *    -> normal calculation
	 * e) Fire in boxed mode
	 *    -> extended power-control after normal calculation
	 *    -> if the target can't be reached while staying below the ceiling,
	 *       check for an obstacle that can be removed and do so if found.
	 */

	DEBUG_LOG_AIM(player->getName(), "[%d / %d] Starting to aim at %s",
				attempt, findTgtAttempts, mem_curr->entry->opponent->getName())
	DEBUG_LOG_AIM(player->getName(), "Aim from %d/%d to %d/%d [distance %d/%d]",
				ROUND(x), ROUND(y),
				ROUND(mem_curr->opX), ROUND(mem_curr->opY),
				ROUND(mem_curr->opX - x), ROUND(mem_curr->opY - y))

	/* Case a) The tank is not buried (enough) and a laser is chosen
	 * ===================================================================
	 */
	if ( (buried < BURIED_LEVEL)
	  && (SML_LAZER <= weap_idx)
	  && (LRG_LAZER >= weap_idx) )
		return calcLaser(is_last);


	/* Case b) The tank is buried and an appropriate tool is chosen
	 * ===================================================================
	 * (This means that it must be checked whether this is an appropriate
	 *  tool or not. Here the method might fail if it isn't suitable.)
	 */
	if (buried >= BURIED_LEVEL)
		return calcUnbury(is_last);


	/* Case c) Kamikaze
	 * ===================================================================
	 */
	if (mem_curr->entry->opponent == player)
		return calcKamikaze(is_last);


	/* Case d) Fire in non-boxed mode
	 * ===================================================================
	 * This is always done, the boxed mode variant below simply checks the
	 * values and tries to adapt.
	 * The flipping is only allowed if the same opponent is tried again.
	 */
	bool result = calcStandard(is_last, (0 == (++mem_curr->attempts % 2)));


	/* Case e) Fire in boxed mode
	 * ===================================================================
	 *    -> extended power-control after normal calculation
	 *    -> if the target can't be reached while staying below the ceiling,
	 *       check for an obstacle that can be removed and do so if found.
	 */
	if (result && env.isBoxed && !isBlocked && needAim && (weap_idx < WEAPONS))
		result = calcBoxed(is_last);


	return result;
}


/** @brief Case e) Fire in boxed mode
  *
  * Note: calcAttack() has to make sure this method is only called if it is
  * appropriate. No further checks are made within this method.
  *
  * @param[in] is_last If this is set to true, the method is forced to succeed.
  * @return true if sane values could be found.
**/
bool AICore::calcBoxed(bool is_last)
{
	// Return at once if the bot "forgets" that there is a ceiling:
	if (!is_last && RAND_AI_1N)
		// With this even the useless bot has only a ~33% chance to forget...
		return true;

	bool    crashed   = true; // Assume the shot crashed in the ceiling
	bool    finished  = false;
	int32_t local_x   = x;
	int32_t local_y   = y;
	double  end_xv    = 0.;
	double  end_yv    = 0.;
	bool    can_mod_a = true;
	bool    can_mod_p = true;
	bool    top_wrap  = false; // Whether the shot wrapped through a wrap ceiling
	bool    can_dig   =   (weap_idx >= BURROWER)
	                   && (weap_idx <= PENETRATOR);

	// Cycle until the ceiling isn't hit any more.
	while ( canWork && !isStopped && crashed
		&& (can_mod_a || can_mod_p)
		&& traceShot(curr_angle, 0, finished, top_wrap, local_x, local_y,
	                 end_xv, end_yv)
		&& finished ) {

		// Yield on each iteration to not hog the CPUs
		if (!global.skippingComputerPlay)
			std::this_thread::yield();

		crashed = false;

		if ( (local_y <= BOXED_TOP) // crashed on top
		    // wrapped to bottom with dirt above is a ceiling crash, too.
		  || ( top_wrap && !can_dig // (Unless a penetrator trick shot is tried)
		    && (local_y > global.surface[local_x].load()) )
		    // Steel wall have additional wall crashes
		  || ( (WALL_STEEL == env.current_wallType)
		    && ( (local_x <= 2)
		      || (local_x >= (env.screenWidth - 3) ) ) ) ) {

			crashed = true;

			// Either reduce angle (-1), or power (1), or both (0)
			int32_t mod_mode = RAND_AI_1P ? (rand() % 2 ? -1 : 1) : 0;

			// Apply mods but do not reduce into nothingness
			if ( (mod_mode < 1) && can_mod_a) {
				if ( (curr_angle > 90) && (curr_angle < 270) )
					curr_angle += curr_angle < 180 ? -1 : 1;
				else
					can_mod_a = false;
			}
			if ( (mod_mode > -1) && can_mod_p) {
				if (curr_power > MIN_POWER)
					curr_power -= 5;
				else
					can_mod_p = false;
			}

			DEBUG_LOG_AIM(player->getName(),
			              "Ceiling crash! Reducing %s [%d°/%d]",
			              1 == mod_mode ? "power          " :
			              0 == mod_mode ? "angle and power" :
			                              "angle          ",
			              GET_DISP_ANGLE(curr_angle), curr_power)

		}
	} // End of crashed loop

	// If the last (not crashed) shot is finished but doesn't get to
	// the target, it is blocked. But this is only considered if
	// a) This is the last attempt and
	// b) This map has a wrap or steel ceiling and
	// c) There was no positive setup score already
	if ( finished && !crashed
	  && (best_setup_score <= 0)
	     // But do not bail out on first try!
	  && (best_setup_score > NEUTRAL_ROUND_SCORE)
	  && (weap_idx < WEAPONS)
	  && (curr_overshoot < 0) // too short
	  && is_last
	  && ( (WALL_STEEL == env.current_wallType)
	    || (WALL_WRAP  == env.current_wallType) )
	  && (-curr_overshoot > weap_curr->radius) // Can't hit
	  && (-curr_overshoot > (mem_curr->distance / 3 * 2)) ) {
		// Note: With big weapons an near opponents, the radius might
		// be larger than two thirds the distance, hence two checks.
		bool free_tank = FABSDISTANCE2(x, y, local_x, local_y)
		                 < weapon[RIOT_CHARGE].radius;

		if (useFreeingTool(free_tank, is_last)) {
			needAim   = false;
			isBlocked = true;
			if (free_tank)
				calcUnbury(is_last);
			else {
				// If a riot bomb is chosen, flatten the angle:
				flattenCurrAng();
				sanitizeCurr();
				angle     = curr_angle;
				power     = curr_power;
				DEBUG_LOG_AIM(player->getName(),
							  "Obstacle detected, trying to clear path using %s",
							  weap_idx < WEAPONS
							  ? weapon[weap_idx].getName()
							  : item[weap_idx - WEAPONS].getName())
			}
			return true;
		}

		// This did not work. (useFreeingTool always
		// succeeds if is_last is true)
		return false;
	}

	return (is_last || !crashed);
}


/** @brief calculate a hit score off the dmgDone values in the opponent memory
  *
  * This method cycles through the opponents memory, and sums up
  * the damage done with curr_weap to a total score according
  * to a) how much damage over the opponents health (aka overkill)
  * has been done and b) on which team they are compared to us.
  *
  * If the primary target was not hit, the score is ensured to be
  * negative, as collateral damage is discouraged. However, this is
  * only done if @a is_last is false, as collateral damage with a
  * total positive score is better than nothing on the very last attempt.
  *
  * @param[in] is_last if set to true then any score is accepted.
  * @return The accumulated score.
**/
int32_t AICore::calcHitScore(bool is_last)
{
	int32_t    hit_score    = 0;
	opEntry_t* opp          = mem_head;
	bool       can_overkill = true;
	eTeamTypes target_team  = mem_curr
	                        ? mem_curr->entry->opponent->team
	                        : TEAM_NEUTRAL;
	bool       tgt_team_hit = false;
	weaponType weapType     = static_cast<weaponType>(weap_curr->type);


	// Dirt weapons and the reducer can not overkill
	if ( ( (DIRT_BALL     <= weapType)
	    && (SUP_DIRT_BALL >= weapType) )
	  || (  REDUCER       == weapType) )
		can_overkill = false;


	while (opp) {

		// Yield on each iteration to not hog the CPUs
		if (!global.skippingComputerPlay)
			std::this_thread::yield();

		if (opp->dmgDone > 0) {
			int32_t overkill  = 0;
			bool    is_killed = false;
			bool    shock_hit = (isShocked && (opp->entry == shocker));
			double  self_mod  = opp->entry->opponent == player
								? (player->selfPreservation + 1.) * ai_level
								: 1.;
			// Note: team_mod is negative if same team, so self_mod must
			//       be positive, or self hits generate huge scores!

			// 1: Determine whether the opponent was killed.
			if (can_overkill && (opp->dmgDone >= opp->opLife) ) {
				overkill     = opp->dmgDone - opp->opLife;
				opp->dmgDone = opp->opLife;
				is_killed    = true;
			}

			DEBUG_LOG_AIM(player->getName(),
						  "Total damage against %10s: %d (%s, %d overkill)",
						  opp->entry->opponent->getName(), opp->dmgDone,
						  is_killed ? "KILLED" : "not killed", overkill)


			// 2: Add the simple damage to the score:
			hit_score += static_cast<double>(opp->dmgDone)
			           * opp->team_mod * self_mod
					   * (shock_hit ? ai_over_mod : 1.);


			// 3: Raise the score a bit if it is collateral damage on
			//    non-neutral team members of our target, but not our team.
			if ( (TEAM_NEUTRAL != target_team)
			  && (player->team != target_team)
			  && (opp->entry->opponent->team == target_team) ) {
				hit_score   *= 1. + ((player->defensive + 2.5) / 10.);
				tgt_team_hit = true;
			}


			// 4: If the opponent is killed, add a bonus to the score depending
			//    on team hit, self hit and whether the bot needs money or not.
			if (is_killed) {
				double kill_bonus = opp->dmgDone;

				if (needMoney)
					kill_bonus *= ai_type_mod;

				// Add some more for killing the shocker:
				if (shock_hit)
					kill_bonus += kill_bonus / ai_over_mod;

				hit_score += kill_bonus * opp->team_mod * self_mod;
			}


			// 5: Check for overkill and dock points for it.
			if (overkill > 0) {
				double over_score = overkill;

				// It is bad if the bot needs money, as it wastes expensive ammo
				if (needMoney)
					over_score *= ai_type_mod + .5;

				// On the other hand, if the bot hit the shocker, the overkill
				// isn't considered that bad, though.
				if (shock_hit)
					over_score /= 3. - ai_over_mod;

				// Generate a generally negative score:
				over_score = std::abs(over_score) * -1.;

				// The more aggressive, the less the reduction will be,
				// but only if it is neithe rus nor our team that got hit
				if (!opp->onSameTeam)
					over_score /= (player->defensive - 2.) * -1.;

				// Add a fraction of the overkill score
				hit_score += over_score / (10. - ai_level_d);
			} // End of overkill score
		} // end of having damage done
		opp = opp->next;
	} // End of looping opponents

	// If the primary target was not hit and this is not the last
	// attempt, make hit_score to be negative, unless the enemy
	// team is decimated. In the latter case the score is simply
	// reduced according to whether the bot needs money or not.
	if (!curr_prime_hit && !is_last && (hit_score > 0.)) {
		if (tgt_team_hit)
			hit_score /= (ai_type_mod + ai_level_d)
			             / (player->defensive + (needMoney ? 2.5 : 4.0));
		else
			hit_score = -1 * std::abs(hit_score);
	}

	return hit_score;
}


/** @brief calculate a score according to where the shot hit and
  * collateral damage done to friend and foe.
  *
  * Damage is recorded in the opponent memory dmgDone value.
  *
  * @param[in] hit_x x coordinate where the current selection hit.
  * @param[in] hit_y y coordinate where the current selection hit.
  * @param[in] weap_rad Calculated radius of the weapon.
  * @param[in] dmg Calculated damage of the weapon.
  * @param[in] weapType Type of the weapon.
  * @return The resulting score
**/
void AICore::calcHitDamage(int32_t hit_x, int32_t hit_y, double weap_rad,
                           double dmg, weaponType weapType)
{
	if ( (nullptr == weap_curr) // no weapon no score
	  || (0 == weap_rad) )      // no radius, no hit
		return;

	// If this has no damage it is either a dirt ball, a reducer
	// or a riot weapon.
	// As dirt balls and reducers must be evaluated, they get a fake
	// damage of their radius so a score can be generated.
	if (0 == dmg) {
		if ( ( (DIRT_BALL     <= weapType)
		    && (SUP_DIRT_BALL >= weapType) )
		  || (  REDUCER       == weapType) )
			dmg = weap_rad;
		else
			return;
	}

	// Napalm blobs have a much higher full damage output
	// than listed, as they do damage over time:
	if (NAPALM_JELLY == weapType)
		dmg *= static_cast<double>(EXPLOSIONFRAMES * weapon[NAPALM_JELLY].etime)
		       / ai_over_mod;

	// Now the score can be calculated
	opEntry_t* opp = mem_head;
	DEBUG_LOG_AIM(player->getName(), "Checking impact at %d x %d", hit_x, hit_y)

	while (opp) {

		// Yield on each iteration to not hog the CPUs
		if (!global.skippingComputerPlay)
			std::this_thread::yield();

		TANK* oppTank = opp->entry->opponent->tank;

		if (oppTank) {
			// Now calculate the score if in range
			double part_dmg = 0.;

			// For dirt balls, only whether the tank is in x-range counts
			// as dirt falls down
			if ( (DIRT_BALL     <= weapType)
			  && (SUP_DIRT_BALL >= weapType)
			  && (oppTank->x > (hit_x - weap_rad))
			  && (oppTank->x < (hit_x + weap_rad)) )
				part_dmg = dmg;
			else
				// All others need a normal check
				part_dmg = get_hit_damage(oppTank, weapType, hit_x, hit_y);

			if (part_dmg > 0.) {

				// REDUCER must be set, it is only checked as valid, yet:
				if (REDUCER == weapType)
					part_dmg = opp->entry->opponent->damageMultiplier * 25.;

				// Set hit damage according to primary or collateral damage
				if (opp == mem_curr) {
					curr_prime_hit = true;
					opp->dmgDone += ROUND(part_dmg);
					DEBUG_LOG_AIM(player->getName(),
								  "%10s in blast range : primary damage   : %4d, total %d",
								  opp->entry->opponent->getName(),
								  ROUND(part_dmg), opp->dmgDone)
				} else {
					opp->dmgDone += ROUND(part_dmg);
					DEBUG_LOG_AIM(player->getName(),
								  "%10s in blast range : collateral damage: %4d, total %d",
								  opp->entry->opponent->getName(),
								  ROUND(part_dmg), opp->dmgDone)
				}

			} // end of having damage delivered
		} // end of having a tank to consider

		opp = opp->next;
	}
}


/** @brief Case c) Kamikaze
  *
  * Note: calcAttack() has to make sure this method is only called if it is
  * appropriate. No further checks are made within this method.
  *
  * @param[in] is_last If this is set to true, the method is forced to succeed.
  * @return true if sane values could be found.
**/
bool AICore::calcKamikaze(bool is_last)
{
	DEBUG_LOG_AIM(player->getName(), "I have decided to go bye bye!", 0)

	// Is the selection sane?
	if (weap_curr->kamikaze) {

		bool is_good = true;

		// Only weapons need any angle/power adaptation
		if (weap_curr) {
			if ( (SHAPED_CHARGE <= weap_idx)
			  && (CUTTER        >= weap_idx) ) {

				// For this it is necessary to look at the terrain.
				// This does not make sense if there isn't a flat area
				// at either side of the tank with an even height.
				int32_t bottom = tank->getBottom();
				int32_t max_rad = weapon[weap_idx].radius / 20;

				// With a power of 150, the weapon can be hurled ~45 pixels.
				// So check that place plus some pixels around according
				// to ai_level.
				int32_t to_go  = 1 + ai_level + (RAND_AI_1P);
				int32_t dist   = 45 - (to_go / 2);
				int32_t diff_l = 0;
				int32_t diff_r = 0;
				int32_t xl     = x - dist;
				int32_t xr     = x + dist;

				for (int32_t i = 0; i < to_go; ++i) {
					--xl;
					++xr;

					// If any x is in/ beyond a non-wrap wall, screw it:
					// a - check left side
					if ( xl < 1 ) {
						if (WALL_WRAP == env.current_wallType)
							xl = env.screenWidth - 1 - (1 - std::abs(xl));
						else
							diff_l += env.screenHeight;
					} else
						diff_l += std::abs(global.surface[xl].load() - bottom);

					// b - check right side
					if ( xr > (env.screenWidth - 2) ) {
						if (WALL_WRAP == env.current_wallType)
							xr = 1 + (env.screenWidth - 1 - xr);
						else
							diff_r += env.screenHeight;
					} else
						diff_r += std::abs(global.surface[xr].load() - bottom);
				} // End of looping distance to_go

				// The average distance is taken. This ignores sudden peaks
				// and gaps that might stop/swallow the projectile, but
				// that should be okay.
				diff_l /= to_go;
				diff_r /= to_go;

				int32_t a_mod = rand() % (10 - ai_level)
							  * (rand() % 2 ? -1 : 1);
				int32_t p_mod = rand() % (20 - (2 * ai_level))
							  * (rand() % 2 ? -1 : 1);
				if ( (diff_l < diff_r) && (diff_l < max_rad) )
					curr_angle = 225 + a_mod;
				else if (diff_r < max_rad)
					curr_angle = 135 + a_mod;
				else
					is_good = false; // May need emergency plan below

				// Set power and write back values
				if (is_good)
					curr_power = 150 + p_mod;

				DEBUG_LOG_AIM(player->getName(),
				              "Firing %s at %d° with power %d%s",
				              weapon[weap_idx].getName(),
				              GET_DISP_ANGLE(curr_angle), curr_power,
				              is_good ? "!" : " will not work! Need a plan!")

			} else if ( (SML_NAPALM <= weap_idx)
					 && (LRG_NAPALM >= weap_idx) ) {

				// Adapt according to the wind:
				int32_t wind     = ROUND(global.wind);
				int32_t wind_mod = 10 + (std::abs(wind) * (1 + RAND_AI_0P));

				if (wind > 0)
					curr_angle = 225;
				else if (wind < 0)
					curr_angle = 135;
				else
					curr_angle = 180;

				curr_power = 100 + wind_mod;

				DEBUG_LOG_AIM(player->getName(),
				              "Firing %s at %d° with power %d (wind %d, wind_mod %d)",
				              weapon[weap_idx].getName(),
				              GET_DISP_ANGLE(curr_angle), curr_power,
				              wind, wind_mod)

			} else {
				int32_t spread_mod = 50 + (10 * (1 + RAND_AI_0P));
				int32_t spread     = weapon[weap_idx].spread; // shortcut
				curr_angle = 180;
				curr_power = 200 + ( (spread * spread_mod) / (2 - (spread % 2)));

				DEBUG_LOG_AIM(player->getName(),
				              "Firing %s at %d° with power %d",
				              weapon[weap_idx].getName(),
				              GET_DISP_ANGLE(curr_angle), curr_power)
			}
		}

		if (is_good) {
			sanitizeCurr();
			angle = curr_angle;
			power = curr_power;
			return true;
		}
	} // end of sane item/weapon selection

	// No, this selection does not make sense.
	// However, if this is a last try, it must work somehow.
	if ( is_last
	  && ( useItem(ITEM_FATAL_FURY)
	    || useItem(ITEM_DYING_WRATH)
	    || useItem(ITEM_VENGEANCE)
	    || useWeapon(DTH_HEAD)
	    || useWeapon(NUKE)
	    || useWeapon(SML_NUKE)
	    || useWeapon(LRG_MIS)
	    || useWeapon(MED_MIS)
	    || useWeapon(SML_MIS) ) ) {
		// Note: The trick is, that the first working selection
		// results in the if-statement to end, and SML_MIS always
		// works.
		curr_angle = 180;
		curr_power = 250;
		angle      = curr_angle;
		power      = curr_power;

		DEBUG_LOG_AIM(player->getName(),
					  "Emergency plan: Firing %s at %d° with power %d",
					  weapon[weap_idx].getName(),
					  GET_DISP_ANGLE(curr_angle), curr_power)

		return true;
	}

	// This can not work, and does not need to be forced.
	return false;
}


/** @brief Case a) The tank is not buried (enough) and a laser is chosen.
  *
  * Note: calcAttack() has to make sure this method is only called if it is
  * appropriate. No further checks are made within this method.
  *
  * @return True if the opponent can be hit, false if the shot is blocked or
  *         pumped into a wall or the ceiling.
**/
bool AICore::calcLaser(bool is_last)
{
	int32_t old_angle = curr_angle;
	int32_t old_power = curr_power;
	double  drift     = static_cast<double>(maxAiLevel + 2 - RAND_AI_0P)
	                  * errorMultiplier * (rand() % 2 ? -1. : 1.);

	curr_power = tank->p;
	curr_angle = GET_SAFE_ANGLE(mem_curr->opX - x, mem_curr->opY - y, drift);

	// Power doesn't matter, but the values must be sane nonetheless:
	sanitizeCurr();

	// Lets see where the laser ends:
	double start_x = 0;
	double start_y = 0;
	player->tank->getGuntop(curr_angle, start_x, start_y);

	BEAM mind_beam(player, start_x, start_y, curr_angle, weap_curr->type,
	               BT_MIND_SHOT);

	int32_t end_x = 0;
	int32_t end_y = 0;
	mind_beam.getEndPoint(end_x, end_y);

	// Generate a score for this
	curr_prime_hit = false;
	// Note: calcHitDamage() sets curr_prime_hit to true if we hit our target.

	// reset virtual damage on opponents.
	opEntry_t* opp = mem_head;
	while (opp) {
		opp->dmgDone = 0;
		opp = opp->next;
	}

	calcHitDamage(end_x, end_y, weapon[weap_curr->type].radius,
	              weap_curr->dmgSingle, static_cast<weaponType>(weap_curr->type));
	int32_t hit_score = calcHitScore(is_last && needSuccess);

	// If the target is behind a dirt wall, break up this attempt
	bool crashed = false;
	if ( !tank->shootClearance(curr_angle, mem_curr->distance, crashed)
	  || crashed) {

		// ...unless this is a forced success ...
		if (is_last)
			// at least reduce the score.
			hit_score = hit_score;
		else {
			curr_angle       = old_angle;
			curr_power       = old_power;
			needAim          = true; // for the next try or the closure
			DEBUG_LOG_AI(player->getName(), "Cancelling laser shot: %s",
			             crashed ? "Wrong angle" : "Not enough clearance")
			return false;
		}
	}

	// If a positive score was achieved, we are set
	if ( (hit_score > 0) || is_last) {
		// Write back values
		sanitizeCurr();
		angle            = curr_angle;
		power            = curr_power;
		if ( (hit_score > best_round_score) || is_last)
			best_round_score = std::abs(hit_score);

		DEBUG_LOG_AIM(player->getName(),
						"Firing %s at %d° with power %d",
						weapon[weap_curr->type].getName(),
						GET_DISP_ANGLE(curr_angle), curr_power)

		return true;
	}

	curr_angle       = old_angle;
	curr_power       = old_power;
	needAim          = true; // for the next try or the closure
	DEBUG_LOG_AI(player->getName(), "Cancelling laser shot: %d score too low",
	             hit_score)

	return false;
}


/** @brief calculate x and y offsets for weapons that need it.
  *
  * These offsets are stored in offset_x and offset_y, as they are needed
  * in multiple places.
  *
  * If the needed offset is off the screen, or makes no sense, the method
  * returns false. But if @a is_last is set to true, insane offsets are tried
  * to be fixed. The idea is, that the bot tries nevertheless out of pure
  * desperation.
  *
  * @param[in] is_last If set to true, the method never fails
  * @return true if sane offsets were found.
**/
bool AICore::calcOffset(bool is_last)
{
	bool result = true;

	offset_x = 0;
	offset_y = 0;

	/* Weapon type 1: Napalm bombs
	 * There are two situations to consider, the normal shot using wind
	 * and the under-run trick. The latter is evil and will almost always
	 * destroy any tank even with a small napalm bomb. Just place the
	 * bomb with tail wind under a tank. No shield can protect the tank
	 * and most jellies will burn into the hull. So this option must be
	 * limited.
	 * Otherwise the wind is what has to be taken into account plus the
	 * surroundings. If the wind side is above the opponent, more distance
	 * is needed than when the area is below the target tank.
	 */
	if ( (weap_idx >= SML_NAPALM) && (weap_idx <= LRG_NAPALM) ) {
		offset_x = global.wind * (ai_level + RAND_AI_1P) * (-1. - focusRate);

		// The farther away the opponent is, the more power is needed to
		// bring the package to the target. More impact power means a higher
		// initial velocity of the blobs, so the offset must be tweaked a bit.
		offset_x *= 1.
		          + ( mem_curr->distance
		            / static_cast<double>(env.screenWidth)
		            * focusRate);


		int32_t pos_x = ROUND(mem_curr->opX) + offset_x;


		// If the resulting x position is not on the screen,
		// the calculation already failed.
		if (pos_x < 2) {
			if (is_last) {
				// But this must be taken...
				pos_x    = 2;
				offset_x = pos_x - ROUND(mem_curr->opX);
			} else
				result = false;
		} else if (pos_x > (env.screenWidth - 2)) {
			if (is_last) {
				// The same...
				pos_x    = env.screenWidth - 2;
				offset_x = ROUND(mem_curr->opX) - pos_x;
			} else
				result = false;
		}

		// If the result is true, the area around pos_x must be checked
		// to determine the y offset and whether to adapt offset_x
		// any further or not.
		if (result) {
			bool    found = false;
			int32_t pos_y = global.surface[pos_x].load();

			if (pos_y < (mem_curr->opY - std::abs(offset_x) + ai_level)) {
				// This means more distance is needed. So we search for
				// the next valid x position that goes down again or that
				// doubles the x distance, whatever comes earlier.
				int32_t mov_x = SIGN(global.wind) * -1;
				int32_t max_x = pos_x + offset_x;

				if (max_x < 2)
					max_x = 2;
				if (max_x > (env.screenWidth - 2))
					max_x = (env.screenWidth - 2);

				for ( ; !found && (pos_x != max_x); pos_x += mov_x) {
					if (global.surface[pos_x].load() > pos_y) {
						pos_x -= mov_x; // One step back
						found  = true;
					}
				}
			} else if (pos_y > (mem_curr->opY + std::abs(offset_x) - ai_level)) {
				// Here check the area towards the enemy. But going nearer
				// than half the distance means an under-run, which is only
				// allowed after an additional check.
				int32_t mov_x = SIGN(global.wind);
				int32_t max_x = pos_x - (offset_x / (RAND_AI_0P ? 1 : 2));
				// Note: Yes, the RAND_AI_0P is the mentioned additional check. ;)
				int32_t max_y = mem_curr->entry->opponent->tank->getBottom();

				for ( ; !found && (pos_x != max_x); pos_x += mov_x) {
					if (global.surface[pos_x].load() <= (max_y - ai_level))
						found = true;
				}
			}

			// adapt offset_x and offset_y now:
			offset_x = pos_x - mem_curr->opX;
			offset_y = global.surface[pos_x].load() - mem_curr->opY;
		}
	} // End of handling napalm

	/* Weapon type 2: Shaped charges.
	 * These are easy. The shaped charge have an y radius of 1/20 of the
	 * x radius. Within this y radius of the centre no damage is done, so
	 * the area from this radius + 1 to + (ai_level * 2) is checked on
	 * either sides whether there is an y position that allows to actually
	 * catch the enemy in the blast. If not, this try is a failure.
	 * However, there is another trick shot here: Place a big shaped charge
	 * like the cutter at the right height behind a hill and blast the
	 * opponents tank through it.
	 */
	else if ( (weap_idx >= SHAPED_CHARGE) && (weap_idx <= CUTTER) ) {
		int32_t rad_y    = weapon[weap_idx].radius / 20;
		int32_t dist_x   = rad_y + RAND_AI_1P;
		int32_t max_dist = RAND_AI_0P
		                 ? (dist_x * 2) + RAND_AI_1P        // normal shot
		                 : weapon[weap_idx].radius * 2 / 3; // trick shot
		int32_t seek_y   = (mem_curr->opY
		                   + mem_curr->entry->opponent->tank->getBottom()) / 2;
		int32_t left_x  = mem_curr->opX - dist_x;
		int32_t right_x = mem_curr->opX + dist_x;
		int32_t left_y  = left_x > 2
						? std::abs(global.surface[left_x].load()) : 0;
		int32_t right_y = right_x < (env.screenWidth - 2)
						? std::abs(global.surface[right_x].load()) : 0;
		bool    go_left  = (mem_curr->opX > x); // Which side to prefer
		bool    found_l  = false;
		bool    found_r  = false;

		for ( ; !found_l && !found_r && (dist_x < max_dist); ++dist_x) {
			left_x  = mem_curr->opX - dist_x;
			right_x = mem_curr->opX + dist_x;
			left_y  = left_x > 2
			        ? std::abs(global.surface[left_x].load()) : 0;
			right_y = right_x < (env.screenWidth - 2)
			        ? std::abs(global.surface[right_x].load()) : 0;

			if (std::abs(left_y - seek_y) <= rad_y)
				found_l = true;
			if (std::abs(right_y - seek_y) <= rad_y)
				found_r = true;
		}

		// If both are valid, use what is preferred
		if (found_l && found_r) {
			if (go_left)
				found_r = false;
			else
				found_l = false;
		}

		// if none is found but this is the last_try, take the simple
		// distance to the preferred side
		if (!found_l && !found_r) {
			if (is_last) {
				if ( (go_left && ((mem_curr->opX - rad_y - 1) > 1))
				  || ((mem_curr->opX + rad_y + 1) > (env.screenWidth - 2)) ) {
					found_l = true;
					left_x  = mem_curr->opX - rad_y - 1;
					left_y  = global.surface[left_x].load();
				} else {
					found_r = true;
					right_x = mem_curr->opX + rad_y + 1;
					right_y = global.surface[right_x].load();
				}
			} else
				result = false;
		}

		// If something is found, set the real offsets
		if (found_l) {
			offset_x = left_x - mem_curr->opX;
			offset_y = left_y - mem_curr->opY;
		} else if (found_r) {
			offset_x = right_x - mem_curr->opX;
			offset_y = right_y - mem_curr->opY;
		}
	} // End of handling shaped charges

	/* Weapon type 3: Driller
	 * The driller must be placed above an enemy tank. This means it is
	 * only useful if the tank is buried, or the tank shall be sunk into
	 * the surface. However, there might be the possibility of an under
	 * shot if the tank is placed on a mountain side.
	 */
	else if (DRILLER == weap_idx) {
		int32_t rad_x    = weapon[weap_idx].radius / 20;
		int32_t pos_x    = ROUND(mem_curr->opX);
		int32_t pos_y    = global.surface[pos_x].load();
		int32_t max_dist = rad_x * 2 / 3;
		int32_t min_y    = mem_curr->opY - rad_x;
		int32_t max_y    = mem_curr->entry->opponent->tank->getBottom() + rad_x;
		bool    found_l  = false;
		bool    found_r  = false;

		// If the direct coordinates are already in order, do not search
		// further. Otherwise try to shift left and right.
		if ( (pos_y > min_y) && (pos_y < max_y) ) {
			for (int32_t off_x = 1
			    ; !found_l && !found_r && (off_x < max_dist)
			    ; ++off_x) {

				int32_t left_x  = pos_x - off_x;
				int32_t right_x = pos_x + off_x;
				int32_t left_y  = left_x > 1
				                ? global.surface[left_x].load() : mem_curr->opY;
				int32_t right_y = right_x < (env.screenWidth - 1)
				                ? global.surface[right_x].load() : mem_curr->opY;
				if ( (left_y < min_y) || (left_y > max_y) ) {
					found_l = true;
					pos_x   = left_x;
					pos_y   = left_y;
				} else if ( (right_y < min_y) || (right_y > max_y) ) {
					found_r = true;
					pos_x   = right_x;
					pos_y   = right_y;
				}
			}

			// If this did not succeed, but it is the last_shot or
			// the AI chooses to bury its opponent, use the opponents
			// coordinates.
			if (!found_l && !found_r) {
				if (is_last || (RAND_AI_0N)) {
					pos_x = mem_curr->opX;
					pos_y = mem_curr->opY;
				} else
					result = false;
			}
		} // end of searching a position to use

		// Set offsets if all is well
		if (result) {
			offset_x = pos_x - mem_curr->opX;
			offset_y = pos_y - mem_curr->opY;
		}
	} // End of handling drillers

	return result;
}


/** @brief Case d) Fire in non-boxed mode
  *
  * Note: calcAttack() has to make sure this method is only called if it is
  * appropriate. No further checks are made within this method.
  *
  * @param[in] is_last If this is set to true, the method is forced to succeed.
  * @param[in] allow_flip_shot If set to true, the bot is allowed to shoot in
  * the opposite direction. On steel walls, this parameter is ignored.
  * @return true if sane values could be found.
**/
bool AICore::calcStandard(bool is_last, bool allow_flip_shot)
{
	bool result = calcOffset(is_last);
	needAim     = true;


	// --- 1) Get a basic raw angle firing directly ---
	// ------------------------------------------------
	bool    wrapped  = false;
	double  opX      = mem_curr->opX + offset_x;
	double  opY      = mem_curr->opY + offset_y;
	// just some shortcuts
	double  dist_x   = opX - x;
	double  dist_y   = opY - y;
	int32_t scrWidth = env.screenWidth;

	// Do not start horizontally, this might happen quite often.
	// If the opponent is above, limit the angle to somewhere between
	// 60° and 75°. If it is below, limit angle between 20° and 35° and
	// limit the angle between 40° and 55° if ~equal.
	int32_t new_angle = GET_SAFE_ANGLE(dist_x, dist_y, 0);
	int32_t ang_limit = (focusRate * static_cast<double>(rand() % 16));

	if      (dist_y < -100) /* above */ ang_limit += 60;
	else if (dist_y >  100) /* below */ ang_limit += 20;
	else                    /* equal */ ang_limit += 40;

	// Apply limit:
	if (new_angle < ( 90 + ang_limit)) new_angle =  90 + ang_limit;
	if (new_angle > (270 - ang_limit)) new_angle = 270 - ang_limit;


	// --- 2) Modify the beginning angle according to focusRate ---
	// --- Keeping this more variable gives a larger range of   ---
	// --- starting points to go forth from.                    ---
	// ------------------------------------------------------------
	double angle_mod = (rand() % 13) * focusRate // useless: 0-2, deadly+1: 0-12
	                 * ((rand() % 2) ? -1. : 1.);
	while ( (std::abs(angle_mod > 0.))
	     && ( ( (new_angle > 180)
	         && ( ( (new_angle + angle_mod) < 190)
	           || ( (new_angle + angle_mod) > 260) ) )
	       || ( (new_angle < 180)
	         && ( ( (new_angle + angle_mod) > 170)
	           || ( (new_angle + angle_mod) < 100) ) ) ) ) {
		angle_mod /= 2.;
	}
	new_angle += angle_mod;


	// --- 3) If this is a wrap wall, check whether shooting ---
	// ---    through the wall is actually shorter.          ---
	// --- A note on allow_flip_shot: If shooting wrapped is ---
	// --- shorter, the AI will chose it more often the      ---
	// --- higher the AI level. (80% for a deadly bot)       ---
	// --- The flipping is then a possibility to shoot non-  ---
	// --- wrapped again.                                    ---
	// ---------------------------------------------------------
	if ( (WALL_WRAP == env.current_wallType) && RAND_AI_0P ) {
		int32_t wrapDist = opX > x
		                 ?  x + scrWidth - 3 - opX
		                 : (scrWidth - x - 3 + opX) * -1;

		if (std::abs(wrapDist) < std::abs(dist_x)) {
			wrapped    = true;
			hasFlipped = true;
			dist_x     = wrapDist;
			new_angle = FLIP_ANGLE(new_angle);

			DEBUG_LOG_AIM(player->getName(),
			              "Flipping through wrap wall at %d°",
			              GET_DISP_ANGLE(new_angle))
		}
	}


	// --- 4) Switch sides if possible and allowed ---
	// -----------------------------------------------
	if ( (WALL_STEEL != env.current_wallType)
	  && allow_flip_shot
	  && (rand() % ( (ai_level + 3) / 2)) ) {
		new_angle = FLIP_ANGLE(new_angle);

		// The result of this flip is different for each wall type
		if (WALL_RUBBER == env.current_wallType) {
			dist_x += opX > x
			        ? (x - 1.) + ( (x - 1.) / BOUNCE_CHANGE)
			        :    (scrWidth - opX - 2.)
			        + ((scrWidth - opX - 2.) / BOUNCE_CHANGE);
		} else if (WALL_SPRING == env.current_wallType) {
			dist_x += opX > x
			        ? (x - 1.) + ( (x - 1.) / SPRING_CHANGE)
			        :    (scrWidth - opX - 2.)
			          + ((scrWidth - opX - 2.) / SPRING_CHANGE);
		} else if (WALL_WRAP == env.current_wallType) {
			if (wrapped)
				// Shoot directly again
				dist_x = opX - x;
			else
				dist_x = opX > x
					   ?  x + scrWidth - 3 - opX
					   : (scrWidth - x - 3 + opX) * -1;
		}

		DEBUG_LOG_AIM(player->getName(),
		              "Flipping %s wall at %d°",
		              wrapped ? "back from" : "towards",
		              GET_DISP_ANGLE(new_angle))

		// wrap / unwrap
		wrapped    = !wrapped;
		hasFlipped = wrapped;
	}


	// --- 5) Adjust angle giving shooting clearance    ---
	// --- Here the clearance is either needed to reach ---
	// --- the next wall or half the distance to the    ---
	// --- selected opponent.                           ---
	// ----------------------------------------------------
	int32_t clearance = std::abs(dist_x / 2);
	int32_t old_angle = new_angle;
	int32_t max_drift = (ai_level + 1) / 2; // [1;3]
	bool    crashed   = false;

	if (wrapped)
		// wrapped shots need clearance to the wall away from the opponent:
		clearance = opX > x ? x - 2 : env.screenWidth - x - 2;

	while ( (new_angle < (180 - max_drift))
	     && !tank->shootClearance(new_angle, clearance, crashed)
	     && !crashed )
		++new_angle;
	while ( (new_angle > (180 + max_drift))
	     && !tank->shootClearance(new_angle, clearance, crashed)
	     && !crashed)
		--new_angle;


	// --- 6) Revert to half the distance between both angles ---
	// ---    if no full clearance is possible.               ---
	// --- An attempt to remove possible obstacles might be   ---
	// --- triggered here.                                    ---
	// ----------------------------------------------------------
	if ( ( new_angle >= (180 - max_drift) )
	  && ( new_angle <= (180 + max_drift) ) ) {
		new_angle = (new_angle + old_angle) / 2;

		// If this is the last chance, try to clear the obstacle.
		// Alternatively a bot with high pain sensitivity might chose
		// to remove the obstacle earlier. The idea here is, that the
		// bot does not want to "piss off" its opponent before the
		// obstacle is removed.
		// However, if there is already a setup with a positive
		// score, revert to that.
		if ( (best_setup_score <= 0)
		  && (is_last
		    || (  (rand() % (ai_level * 20))
		        < (player->painSensitivity * ai_level * 5) ) ) ) {
			/* Range is from Useless and pain resistant (0.1) to
			 * (Deadly + 1) and very pain sensitive: [max rand value]
			 * Useless    : 0.1 * 1 * 5 =  0.5 [ 20]
			 * Deadly + 1 : 3.0 * 6 * 5 = 90.0 [120]
			 */
			isBlocked  = true;
			result     = useFreeingTool(false, is_last);
			curr_angle = new_angle;

			// Try not to bomb the ceiling:
			if ( (curr_angle >= 150) && (curr_angle <= 210)) {
				flattenCurrAng();

				// Write back curr_ang, or it gets overwritten below.
				new_angle = curr_angle;
			}

			DEBUG_LOG_AIM(player->getName(),
						  "Obstacle detected, trying to clear path using %s",
						  weap_idx < WEAPONS
						  ? weapon[weap_idx].getName()
						  : item[weap_idx - WEAPONS].getName())
		} else
			// This did not work out
			result = false;
	} // End of being blocked


	// --- 7) Find necessary power                ---
	// --- This is just an estimation on possible ---
	// ---  "air time" of the projectile          ---
	// ----------------------------------------------
	if (result) {
		// As there is nothing that can fail now, the new
		// angle is the one to go with:
		curr_angle = new_angle;

		double slope_x = env.slope[curr_angle][0];
		double slope_y = env.slope[curr_angle][1];
		double rawTime = slope_x != 0.
		               ? dist_x / slope_x
		               : dist_x / 0.00000001; // 180° should be impossible though.

		// lower target, less power.
		// If the target is above, the projectile hits earlier than
		// on lower targets, where the projectile has to fall down there.
		double airTime = std::abs(rawTime) + (
		                 dist_y * slope_y * env.gravity
		                 * env.FPS_mod * 2.0);

		// Less airTime doesn't necessarily mean less power
		// Horizontal firing means more power needed even though
		// air time is minimised.
		curr_power = ROUNDu(std::sqrt(
						airTime * env.gravity * env.FPS_mod)
						* static_cast<double>(env.frames_per_second));

		// Power modification according to the bots focus rate
		// This helps having slightly different starting powers to
		// begin aiming with.
		curr_power += focusRate * static_cast<double>(rand() % 51)
		            * (rand() % 2 ? -1. : 1.);
		// With a focus rate of [0.166;1] this results in a modification
		// between [-8.3;8.3] and [-50;50].

		// Be sure power is in the valid range:
		if (curr_power > MAX_POWER) curr_power = MAX_POWER;
		if (curr_power < MIN_POWER) curr_power = MIN_POWER;

		// Power is only available in a stepping of five
		curr_power -= curr_power % 5;

		DEBUG_LOG_AIM(player->getName(),
		              "Firing %s at %d° with power %d",
		              weapon[weap_idx].getName(),
		              GET_DISP_ANGLE(curr_angle), curr_power)
	} // End of having a result

	// Write back values and stop aiming if this is a blocked path freeing attempt
	if (result && isBlocked) {
		sanitizeCurr();
		angle   = curr_angle;
		power   = curr_power;
		needAim = false;
	}


	return result;
}


/** @brief Case b) The tank is buried and an appropriate tool is chosen
  *
  * Note: This means that it must be checked whether this is an appropriate
  * tool or not. Here the method might fail if it isn't suitable.
  *
  * @param[in] is_last If this is set to true, the method is forced to succeed.
  * @return true if sane values could be found.
**/
bool AICore::calcUnbury(bool is_last)
{
	DEBUG_LOG_AIM(player->getName(), "I am buried! (%d >= %d)",
	              buried, BURIED_LEVEL)

	// Suitable tool?
	if ( ( (RIOT_BOMB  <= weap_curr->type)       // Clear freeing tool
	    && (RIOT_BLAST >= weap_curr->type) )     // that clears dirt
	  || ( ( (SHAPED_CHARGE > weap_curr->type)   // shaped charges can not
	      || (CUTTER        < weap_curr->type) ) // be used, and neither can
	    && (DRILLER != weap_curr->type)          // the driller, to self
		&& weap_curr->kamikaze) ) {              // destruct while buried

		// To not blast away an obstacle towards a wall with no
		// enemies behind it, count how many enemies are on each
		// side, first:
		opEntry_t* op       = mem_head;
		int32_t    op_left  = 0;
		int32_t    op_right = 0;
		while (op) {
			if ( op->alive && !op->onSameTeam ) {
				if (op->opX < tank->x)
					op_left++;
				else
					op_right++;
			}
			op = op->next;
		}

		// Determine starting values according to which side
		// is buried stronger, and where the opponents are:
		bool go_left = true;

		// It is better to go right instead of left if:
		// a) more enemies are on the right
		// b) the count is equal but the right side is more buried or
		// c) the current favourite target is on the right and the left
		//    is not that much more buried. (depends on AI level)
		if ( (op_right > op_left)                               // a)
		  || ( (op_right == op_left) && (buried_r > buried_l) ) // b)
		  || ( (mem_curr->opX > x)
		    && (std::abs(buried_r - buried_l) < ai_level) ) ) { // c)

			go_left = false;
		}

		// find a good starting angle where the obstacle begins:
		int32_t dist    = ai_level * (player->defensive + 3.) * 2;
		bool    crashed = false;
		curr_angle = 180;

		if (go_left) {
			while ( (curr_angle < 250)
			     && ( tank->shootClearance(curr_angle, dist, crashed)
			       || !crashed) )
				++curr_angle;
		} else {
			while ( (curr_angle > 110)
			     && ( tank->shootClearance(curr_angle, dist, crashed)
			       || !crashed) )
				--curr_angle;
		}

		// Add a variant to the angle:
		curr_angle += ( (rand() % 21) - 10) / ai_level;

		// If riot charges are used, 45° is the lower border.
		if ( (RIOT_BOMB  <= weap_curr->type)
		  && (RIOT_BLAST >= weap_curr->type) ) {
			if (curr_angle < 135) curr_angle = 135;
			if (curr_angle > 225) curr_angle = 225;
		}

		// Be sure current values are sane:
		sanitizeCurr();

		angle     = curr_angle;
		power     = curr_power;
		needAim   = false; // Already done here!
		isBlocked = true;

		DEBUG_LOG_AIM(player->getName(),
		              "Freeing myself using %s at %d° with power %d",
		              weapon[weap_idx].getName(),
		              GET_DISP_ANGLE(angle), power)

		return true;
	} // End of having selected an appropriate tool.

	// The only non-self-destruct way to use a weapon for freeing
	// one self is the shaped charge:
	if ( ( (SHAPED_CHARGE <= weap_curr->type)
	    && (CUTTER        >= weap_curr->type) )
	  || ( DRILLER == weap_curr->type ) ) {
		curr_angle = 180;
		curr_power = 10 + RAND_AI_0P;

		sanitizeCurr();

		angle     = curr_angle;
		power     = curr_power;
		needAim   = false; // Already done here!
		isBlocked = true;

		DEBUG_LOG_AIM(player->getName(),
		              "Freeing myself using %s at %d° with power %d",
		              weapon[weap_idx].getName(),
		              GET_DISP_ANGLE(angle), power)

		return true;
	}

	// emergency values if this is our last try:
	if (is_last) {
		if (!useFreeingTool(true, true))
			useWeapon(SML_MIS);
		curr_angle = buried_l > buried_r ? 200 : 100
		           + (rand() % 61);
		curr_power = 500 + (rand() % 501);

		sanitizeCurr();

		angle     = curr_angle;
		power     = curr_power;
		needAim   = false; // Already done here!
		isBlocked = true;

		DEBUG_LOG_AIM(player->getName(),
		              "(last!) Freeing myself using %s at %d° with power %d",
		              weapon[weap_idx].getName(),
		              GET_DISP_ANGLE(angle), power)

		return true;
	}

	// Otherwise this has failed

	DEBUG_LOG_AIM(player->getName(), "Nothing suitable selected (%s)",
	              item_curr
	              ? item[weap_idx - WEAPONS].getName()
	              : weap_curr
	                ? weapon[weap_idx].getName() : "NOTHING")

	return false;
}


/// @return false if the initialization of this instance failed
bool AICore::can_work() const
{
	return canWork;
}


/** @brief check the currently set item list and update its organization
  *
  * This checks every item compared to the currently selected
  * target and sets a score on usability. The list is then
  * sorted by score in descending order.
**/
void AICore::checkItemMem()
{
	item_curr = item_head;

	DEBUG_LOG_AIM(player->getName(), "Starting to check item memory", 0)

	while (item_curr) {

		// Yield on each iteration to not hog the CPUs
		if (!global.skippingComputerPlay)
			std::this_thread::yield();

		// Update score
		updateItemScore(item_curr);

		// Advance
		item_curr = item_curr->next;
	}

	// Eventually sort the list
	sort_entries(&item_head);
}


/** @brief check the currently set memory and update its organization
  *
  * This looks into each entry whether there is new damage for
  * this turn, and updates the score.
  * After the score updates, the list is reordered, so the entry
  * with the highest score becomes mem_head, and the list ends with
  * the lowest scored entry.
**/
void AICore::checkOppMem()
{
	mem_curr = mem_head;

	DEBUG_LOG_AIM(player->getName(), "Starting to check opponent memory", 0)

	while (mem_curr) {

		// Yield on each iteration to not hog the CPUs
		if (!global.skippingComputerPlay)
			std::this_thread::yield();

		// Update score
		updateOppScore(mem_curr);

		// Advance
		mem_curr = mem_curr->next;
	}

	// Do we have a new revengee?
	if (revengee && (player->revenge != revengee->opponent))
		player->revenge = revengee->opponent;

	// Not a single one?
	if (nullptr == revengee)
		player->revenge = nullptr;

	// Eventually sort the list
	sort_entries(&mem_head);
}


/** @brief check the currently set weapon list and update its organization
  *
  * This checks every weapon compared to the currently selected
  * target and sets a score on usability. The list is then
  * sorted by score in descending order.
**/
void AICore::checkWeapMem()
{
	weap_curr = weap_head;

	DEBUG_LOG_AIM(player->getName(), "Starting to check weapon memory", 0)

	while (weap_curr) {

		// Yield on each iteration to not hog the CPUs
		if (!global.skippingComputerPlay)
			std::this_thread::yield();

		// Update score
		updateWeapScore(weap_curr);

		// Advance
		weap_curr = weap_curr->next;
	}

	// Eventually sort the list
	sort_entries(&weap_head);
}


/// @brief destroy all memory chains
void AICore::destroy()
{
	while (item_head) {
		item_curr = item_head;
		item_head = item_curr->next;
		delete item_curr;
	}
	item_curr = nullptr;
	item_last = nullptr;

	while (mem_head) {
		mem_curr = mem_head;
		mem_head = mem_curr->next;
		delete mem_curr;
	}
	mem_curr = nullptr;
	mem_last = nullptr;

	while (weap_head) {
		weap_curr = weap_head;
		weap_head = weap_curr->next;
		delete weap_curr;
	}
	weap_curr = nullptr;
	weap_last = nullptr;
}


/// @brief Forbid AICore to create FLOATTEXT instances
void AICore::forbidText()
{
	textAllowed.store(false);
}


/// @brief Get a string describing the given AI @a level
const char* AICore::getLevelName  (int32_t level) const
{
	switch (level) {
		case 0:
			return "HUMAN (!!!)";
			break;
		case 1:
			return "Useless";
			break;
		case 2:
			return "Guesser";
			break;
		case 3:
			return "Range Finder";
			break;
		case 4:
			return "Targetter";
			break;
		case 5:
			return "Deadly";
			break;
		case 6:
			return "Deadly + 1";
			break;
		default:
			break;
	}
	return "OUT OF RANGE (!!!)";
}


/** @brief get the set players memory and check it
  *
  * This method fetches all sOpponent entries from the handled player,
  * and fills the item and weapon chains with the stock count and weapon
  * preferences.
  *
  * The scores are not calculated, and the list is not sorted.
  * Therefore checkOppMem() must be called first when the thread
  * starts in operator().
  *
  * @return true if the memory could be copied
**/
bool AICore::getMemory()
{
	assert(player         && "ERROR: getMemory() reached with nullptr player?");
	assert( tank          && "ERROR: getMemory() reached with nullptr tank?");
	assert(!tank->destroy && "ERROR: getMemory() reached with destroyed tank?");


	/// === 1) Copy item information ===

	int32_t idx  = 0;
	int32_t pref = 0;

	item_curr = item_head;
	item_last = nullptr;

	assert(item_head && "ERROR: getMemory() called without item memory set up!");

	if (!item_head)
		return false;

	do {

		// Yield on each iteration to not hog the CPUs
		if (!global.skippingComputerPlay)
			std::this_thread::yield();

		if ( -1 < (pref = player->getItemPref(idx) ) ) {
			item_curr->amount     = player->ni[idx];
			item_curr->preference = pref;
			item_curr->selectable = item[idx].selectable ? true : false;
			item_curr->type       = idx;

			// The kamikaze value is only pre-set to true for vengeance
			// items, all other must be determined if the bot really
			// chooses to self-destruct.
			if ( (item_curr->type >= ITEM_VENGEANCE)
			  && (item_curr->type <= ITEM_FATAL_FURY) )
				item_curr->kamikaze = true;
			else
				item_curr->kamikaze = false;

			// Advance current
			item_curr = item_curr->next;
		}
	} while ((++idx < ITEMS) && item_curr);


	/// === 2) Copy opponents information ===

	idx = 0;
	int32_t    jcnt = 0; // Jedi count
	int32_t    scnt = 0; // Sith count;
	double     dail = ai_level_d; // [d]ouble [ai]_[l]evel
	sOpponent* opp  = nullptr;

	mem_curr = mem_head;
	mem_last = nullptr;

	assert(mem_head && "ERROR: getMemory() called without opponents memory set up!");

	if (!mem_head)
		return false;

	do {

		// Yield on each iteration to not hog the CPUs
		if (!global.skippingComputerPlay)
			std::this_thread::yield();

		mem_curr->alive = false; // must be confirmed

		if ( (opp = player->getOppMem(idx) ) ) {
			TANK* oppTank = nullptr;

			mem_curr->attempts    = 0;
			mem_curr->entry       = opp;
			mem_curr->revengeDone = false;

			if (  opp->opponent
			  &&  opp->opponent->tank
			  && !opp->opponent->tank->destroy)
				oppTank = opp->opponent->tank;

			// The other values depend on whether an active tank was found:
			if (oppTank) {
				mem_curr->is_buried  = oppTank->howBuried(&mem_curr->buried_l,
				                                          &mem_curr->buried_r)
				                       > BURIED_LEVEL ? true : false;
				mem_curr->hasRepulse = oppTank->hasRepulsorActivated();
				mem_curr->opLife     = oppTank->l + oppTank->sh;
				mem_curr->opX        = oppTank->x;
				mem_curr->opY        = oppTank->y;
				mem_curr->diffLife   = currLife - mem_curr->opLife;
				mem_curr->distance   = FABSDISTANCE2(x, y, mem_curr->opX, mem_curr->opY);

				if (oppTank->l > 0)
					mem_curr->alive = true;

				// Is this the last one? then set as default best choice:
				if (last_opp == opp)
					best_setup_mem = mem_curr;
			} else {
				// Reset some values if there is no tank
				mem_curr->opLife   = 0.;
				mem_curr->diffLife = currLife;
				mem_curr->distance = env.screenWidth * env.screenHeight;
			}

			// Some calculations can be cut short if this is ourselves:
			if (opp->opponent == player) {
				mem_curr->onSameTeam = true;
				mem_curr->team_mod   = ( ( (player->painSensitivity  + 1.)   // [1;4]
				                         * (player->selfPreservation + 1.) ) // [1;4]
				                       + 2.) // [ 3  ; 18]
				                     / -1.;  // [-1.5; -9]
			} else {
				mem_curr->onSameTeam = ( (TEAM_NEUTRAL   != player->team)
				                      && (opp->opponent->team == player->team) );

				// team_mod is a multiplier reflecting the general behaviour
				// against the own and other teams.
				if ( TEAM_JEDI == player->team ) {
					// Jedi go strongly for Sith and protect their team
					if (mem_curr->onSameTeam)
						mem_curr->team_mod = (2. + dail) / -2.; // [-1.5; -4.]
					else if ( TEAM_SITH == opp->opponent->team ) {
						mem_curr->team_mod = 2. * ai_level;     // [2;12]
					} else
						mem_curr->team_mod = ai_level;          // [1; 6]
				} else if ( TEAM_SITH == player->team ) {
					// Sith go for everyone, slightly favouring Jedi and do
					// not care that much hitting their own team members.
					if (mem_curr->onSameTeam)
						mem_curr->team_mod = (2. + dail) / -3.; // [-1; -2.66]
					else if ( TEAM_JEDI == opp->opponent->team ) {
						mem_curr->team_mod = 1.25 * ai_level;   // [1.25;7.5]
					} else
						mem_curr->team_mod = ai_level;          // [1   ;6  ]
				} else {
					// Neutrals go slightly more for the teams, and less for
					// other neutrals. This is supposed to reflect the fact
					// that Jedi and Sith have friends with them helping them
					// out. Neutrals are all alone and considered less dangerous.
					if ( TEAM_NEUTRAL == opp->opponent->team )
						mem_curr->team_mod = 1. + (dail / 2.);  // => [1.5;4.]
					else {
						mem_curr->team_mod = .5 + dail;         // => [1.5;6.5]
						if (TEAM_JEDI == opp->opponent->team)
							++jcnt;
						else
							++scnt;
					}
				} // End of team_mod determination
			} // End of opponent handling

			// Advance current
			mem_curr = mem_curr->next;
		}
		++idx;
	} while (opp && mem_curr);

	// If this is a neutral player, it has counted jedi and sith. This is
	// done to raise the team_mod whenever any of these teams sport more
	// than one remaining tank.
	if ( (TEAM_NEUTRAL == player->team) && ( (jcnt > 1) || (scnt > 1) ) ) {
		double j_mod = dail / 10. * static_cast<double>(jcnt - 1);
		double s_mod = dail / 10. * static_cast<double>(scnt - 1);
		mem_curr = mem_head;

		while (mem_curr) {

			if ( (TEAM_JEDI == mem_curr->entry->opponent->team)
			  && (jcnt > 1) )
				mem_curr->team_mod += j_mod;
			else if ( (TEAM_SITH == mem_curr->entry->opponent->team)
			       && (scnt > 1) )
				mem_curr->team_mod += s_mod;

			mem_curr = mem_curr->next;
		}
	}

	/// === 3) Copy weapon information ===

	double dmgMod  = player->damageMultiplier;
	idx  = 0;
	pref = 0;

	weap_curr = weap_head;
	weap_last = nullptr;

	assert(weap_head && "ERROR: getMemory() called without weapon memory set up!");

	if (!weap_head)
		return false;

	// Reset blast values, they have to be found anew:
	blast_min = 0.;
	blast_med = 0.;
	blast_big = 0.;
	blast_max = 0.;

	do {

		// Yield on each iteration to not hog the CPUs
		if (!global.skippingComputerPlay)
			std::this_thread::yield();

		if ( -1 < (pref = player->getWeapPref(idx) ) ) {
			int32_t subMun = weapon[idx].submunition; // short-cut
			double  damage = weapon[idx].damage * dmgMod
			               * weapon[idx].getDelayDiv();

			// === Dirt weapons have a "damage" based on their radius ===
			if ( (DIRT_BALL         <= idx)
			  && (SMALL_DIRT_SPREAD >= idx) )
				damage = weapon[idx].radius * (1.5 + player->defensive);

			weap_curr->amount     = player->nm[idx];

			// Chain missiles and such have a delay of the same count they shoot
			// missiles. However, weapons without a delay get a value of 1 here,
			// so looping in traceWeapon() is much simpler.
			weap_curr->delay      = weapon[idx].delay > 0 ? weapon[idx].delay : 1;

			// To be able to track weapons that trigger other sub munitions,
			// their configuration must be remembered, too:
			weap_curr->subMunCount= weapon[idx].numSubmunitions;
			weap_curr->subMunType = subMun;
			// Non-spread weapons have their spread value set to 1. To catch
			// future cases where this might be different, the value is
			// stored and sanitized here:
			weap_curr->spread     = weapon[idx].spread > 0 ? weapon[idx].spread : 1;
			weap_curr->dmgCluster = weapon[idx].numSubmunitions > 0
			                      ? dmgMod
			                        * static_cast<double>(weapon[subMun].damage)
			                        * static_cast<double>(weapon[idx].numSubmunitions)
			                      : 0.;
			weap_curr->dmgSingle  = damage;
			weap_curr->dmgSpread  = damage
			                      * static_cast<double>(weap_curr->spread);
			weap_curr->preference = pref;
			weap_curr->radius     = weapon[idx].radius;
			weap_curr->type       = idx;

			// Save blast value for opponents score calculation
			double ds = weap_curr->dmgSingle;
			if (SML_MIS == idx) {
				if (blast_min < ds) blast_min = ds;
				if (blast_med < ds) blast_med = ds;
				if (blast_big < ds) blast_big = ds;
				if (blast_max < ds) blast_max = ds;
			} else if ( ( (MED_MIS == idx) || (LRG_MIS == idx) )
			       && (weap_curr->amount > 0) ) {
				if (blast_med < ds) blast_med = ds;
				if (blast_big < ds) blast_big = ds;
				if (blast_max < ds) blast_max = ds;
			} else if ( ( (SML_NUKE == idx) || (NUKE == idx) )
			       && (weap_curr->amount > 0) ) {
				if (blast_big < ds) blast_big = ds;
				if (blast_max < ds) blast_max = ds;
			} else if ( ( (DTH_HEAD == idx) )
			       && (weap_curr->amount > 0)
			       && (blast_max < ds) )
				blast_max = ds;

			// If this is the last weapon used, store as default best choice
			if (last_weap == idx)
				best_setup_weap = weap_curr;

			// Advance current
			weap_curr = weap_curr->next;
		}
	} while ((++idx < WEAPONS) && weap_curr);

	return true;
}


/// @brief flatten curr_ang if a riot bom is chosen to clear the path
/// Note: No checks about being blocked, call it when appropriate. Only
///       the weapon is checked against riot bombs.
void AICore::flattenCurrAng()
{
	if ( (RIOT_BOMB     <= weap_curr->type)
	  && (HVY_RIOT_BOMB >= weap_curr->type) ) {
		int32_t div = 1 + ( (RAND_AI_1P + 2) / 2); // [2;4]
		// Minimum : 1 + ( (0 + 2) / 2 ) = 1 + ( 2 / 2 ) = 1 + 1 = 2
		// Useless:  1 + ( (1 + 2) / 2 ) = 1 + ( 3 / 2 ) = 1 + 1 = 2
		// Deadly+1: 1 + ( (5 + 2) / 2 ) = 1 + ( 7 / 2 ) = 1 + 3 = 4
		if (curr_angle > 180)
			curr_angle += (270 - curr_angle) / div;
		else
			curr_angle -= (curr_angle - 90) / div;
	}
}


/// @brief adapt @a ang_mod and @a pow_mod when a shot using them crashed.
void AICore::fixCrashed (int32_t &ang_mod, int32_t &pow_mod)
{
	// Unless a hill was detected, the angle mod must not be greater than 1
	if (!hill_detected && (ang_mod > 1))
		ang_mod = 1;

	// Use a unified angle or there has to be an if/else for the same code
	int32_t fix_ang = curr_angle > 180 ? FLIP_ANGLE(curr_angle) : curr_angle;

	// The following rules must be applied:
	// 1) If boxed mode is on, the angle must not be higher than 150°
	//    or it has to be reduced more.
	// 2) Otherwise lower the angle further away from 45° (aka 135° here) if
	//    it is already low.
	//    If it isn't, it is raised anyway.
	// 3) If none of the above apply, assume the angle to be in order if it
	//    is between 130 and 140, which is 45° +/- 5°.
	if (env.isBoxed && (fix_ang > 150)) {             // 1)
		ang_mod *= -1 * RAND_AI_1P;
		// Do not overdo it:
		while (std::abs(ang_mod) > (ai_level + 2))
			ang_mod /= 2;
	} else if ( (fix_ang > 100) && (fix_ang <= 135) ) // 2)
		ang_mod *= -1;
	else if ( (fix_ang > 130) && (fix_ang <  140) )   // 3)
		ang_mod = 0; // None needed

	// now flip ang_mod if the angle was flipped:
	ang_mod *= curr_angle > 180 ? -1 : 1;

	// The power must be reduced if it is greater than the x distance.
	int32_t pow_diff = curr_power - std::abs(mem_curr->opX - x);
    if (pow_diff > 0) {
		pow_mod = std::abs(pow_mod) * -1;
		// And strengthen the power reduction more if
		// the power is more than 50% over the distance
		if ( pow_diff >= (std::abs(mem_curr->opX - x) / 2) )
			pow_mod *= 2 * RAND_AI_1P;
    }
}


/// @brief Try to adapt @a ang_mod and @a pow_mod according to the current
/// overshoot and where the best hit landed.
void AICore::fixOvershoot(int32_t& ang_mod, int32_t& pow_mod, int32_t hit_score)
{
	// Here are some more possible (sub) situations to consider:
	// 1) The current score is at least better than the last.
	//    This can happen if the shot does no longer hit team mates.
	//    The important situation is, if the overshoot is very small
	//    and a new best score is achieved. The bigger the weapon, the
	//    higher the probability that this might be the case.
	// 2) Both the current and the last overshoot were negative, the
	//    angle was optimized towards 45° and the power was raised.
	//    Having a worse overshoot then can happen if the gun was
	//    lowered and the shot crashes into the side of a hill or
	//    mountain.
	//    The angle must then be brought towards 180° more than the
	//    last angle modification brought it away from it.
	// 3) The current score is worse than the last score.
	//     a) The last score was better than the one before.
	//        The modifications might have been too strong, try
	//        values between the two.
	//     b) That was two worse tries in a row.
	//        The direction was wrong, and the last modifications
	//        must be reverted and strengthened by the current set
	//        mods.
	// 4) No last score or the same, just adapt the mods according to
	//    whether the shot was too short or too long.


	bool angle_was_optimized = false;
	if ( ( (curr_angle > 180) && (curr_angle <= 235) && (last_ang_mod > 0) )
	  || ( (curr_angle < 180) && (curr_angle >= 135) && (last_ang_mod < 0) ) )
		angle_was_optimized = true; // Optimized towards 45° on its side

	// reset hill detection if the current overshoot isn't short
	if ( (curr_overshoot > 0) || (hit_score > 0) )
		hill_detected = false;

	if (last_score && (hit_score > 0) && (hit_score > last_score)) {

		// 1) Better score with worse overshoot.
		// Here a best score might happen. But this is only noteworthy
		// if the hit_score is positive. Otherwise it would simply
		// mean that less damage was done (team and others) and that is
		// hardly anything to note down.
		// Note: if this is a new best score, some adaptation has
		//       already been made in aim().
		if (hit_score < best_score) {
			// Only adapt the signedness of the mods and note that
			// the aiming is not there, yet:
			DEBUG_LOG_AIM(player->getName(),
			              " => Better score %d [%d last]",
			              hit_score, last_score)
			ang_mod = std::abs(ang_mod) * SIGN(last_ang_mod);
			pow_mod = std::abs(pow_mod) * SIGN(curr_overshoot) * -1;
		}

		// At least better than the last
		last_was_better = true;
		hill_detected   = false;
	} else if ( (hit_score <= 0)
	         && (last_overshoot < 0)
	         && (curr_overshoot <= last_overshoot) // Keeps being too short
	         && angle_was_optimized) {

		// 2) Assume a hill in the path
		pow_mod = (std::abs(pow_mod) + std::abs(last_pow_mod)) / 2; // raise it
		ang_mod = static_cast<double>(std::abs(last_ang_mod) + std::abs(ang_mod))
		        * ai_over_mod * SIGNd(last_ang_mod) * -1.;
		// Note: This accumulates the last and the current angle modification,
		//       strengthens depending on AI level and ensures it has the
		//       opposite direction from the last modification.

		// Make sure the new ang_mod really gets the angle upwards:
		if ( SIGN(curr_angle - 180) == SIGN(ang_mod) )
			ang_mod *= -1;

		// Make sure the new ang_mod doesn't make the angle to flip over:
		if ( (curr_angle > 180) && ((curr_angle + ang_mod) <= 180) )
			ang_mod = 181 - curr_angle;
		if ( (curr_angle < 180) && ((curr_angle + ang_mod) >= 180) )
			ang_mod = curr_angle - 181;

		DEBUG_LOG_AIM(player->getName(),
		              "Assuming hill crash, reverting ang_mod to %d",
		              ang_mod)

		last_was_better = false; // false, so this change won't get directly
		// reverted again.
		hill_detected   = true;
	} else if (last_score && (last_score > hit_score) ) {
		// 3) Wrong direction!
		if (last_was_better) {

			DEBUG_LOG_AIM(player->getName(),
			              " => Worse score %d [%d was better]",
			              hit_score, last_score)

			// Try a mod in between the two
			if (std::abs(last_ang_mod) > 1)
				ang_mod = -1 * (last_ang_mod / 2);
			else
				ang_mod = -1 * SIGN(last_ang_mod);
			if (std::abs(last_pow_mod) > 9)
				pow_mod = -1 * (last_pow_mod / 2);
			else
				pow_mod = -5 * SIGN(last_pow_mod);
		} else {
			// b) Revert and go in the opposite direction:

			DEBUG_LOG_AIM(player->getName(),
			              " => Worse score %d [%d was worse]",
			              hit_score, last_score)

			// If the last was reverted already, strengthen the
			// move in the opposite direction
			if (last_reverted) {
				// First make positive and strengthen
				ang_mod = std::abs(ang_mod) * (RAND_AI_0P + 1);
				pow_mod = std::abs(pow_mod) * (RAND_AI_1P + 1);
			}

			// Then strengthen the last values by the current and revert:
			ang_mod = -1 * (  last_ang_mod
			                + (SIGN(last_ang_mod) * ang_mod) );
			pow_mod = -1 * (  last_pow_mod
			                + (SIGN(last_pow_mod) * pow_mod) );

			// Adapt by overshoot: (Yes, still necessary)
			if (SIGN(curr_overshoot) == SIGN(pow_mod))
				pow_mod *= -1;
		}

		last_was_better = false;
	} else {
		// 4) Just do the set mod according to overshoot

		DEBUG_LOG_AIM(player->getName(), "=> Same score %d, overshoot %d",
		              hit_score, curr_overshoot)

		// Put in some limits for the angle according to where the opponent is
		int32_t ang_limit = (focusRate * static_cast<double>(rand() % 16));
		int32_t dist_y    = mem_curr->opY - y;
		if      (dist_y < -100) /* above */ ang_limit += 60;
		else if (dist_y >  100) /* below */ ang_limit += 20;
		else                    /* equal */ ang_limit += 40;

		// If a hill was detected, do not modify angle more than 1
		if (hill_detected && (ang_mod > 1))
			ang_mod = 1;

		if ( ( (curr_angle <= 180) && (curr_angle > (90 + ang_limit)) )
		  || ( (curr_angle > (270 - ang_limit) ) ) )
			ang_mod *= -1;

		// Adapt pow_mod by overshoot
		pow_mod = std::abs(pow_mod) * SIGN(curr_overshoot) * -1;

		last_was_better = false;
	}
}


/// @brief adapt @a ang_mod and @a pow_mod when a shot using them did not
/// finish.
void AICore::fixUnfinished (int32_t &ang_mod, int32_t &pow_mod)
{
	// Put in some limits for the angle according to where the opponent is
	int32_t ang_limit = (focusRate * static_cast<double>(rand() % 16));
	int32_t dist_y    = mem_curr->opY - y;
	if      (dist_y < -100) /* above */ ang_limit += 60;
	else if (dist_y >  100) /* below */ ang_limit += 20;
	else                    /* equal */ ang_limit += 40;

	// If a hill was detected on the path, do not alter the angle
	// more than by 1
	if (hill_detected && (ang_mod > 1))
		ang_mod = 1;

	// If the angle is too steep to the right, or too flat
	// to the left, make ang_mod negative:
	// Note: If the shot is to the right, it is in the range
	// 90-180 and going down needs a negative mod.
	// If going to the left it needs a positive mod to go
	// down as it is in the range 180 - 270.

	if ( ( (curr_angle <= 180) && (curr_angle > (90 + ang_limit)) )
	  || ( (curr_angle > (270 - ang_limit) ) ) )
		ang_mod *= -1;

	// The power must be reduced if it is greater than twice the
	// x distance, but raised if less than the simple x distance.
	// If the power is too low, shots can quickly end up with too
	// many bounces if the wall is rubber or spring.
    int32_t dist_x = std::abs(mem_curr->opX - x);
    if (curr_power > (2 * dist_x))
		pow_mod *= -1;
	else if (curr_power > dist_x)
		pow_mod = 0; // Do not change
}


/// @return true once the operator() ends
bool AICore::hasExited() const
{
	return isFinished;
}


/// @brief initialize work with the current players data
bool AICore::initialize()
{
	DEBUG_LOG_AI(player->getName(), "Starting think work, setting up.", 0)

#if defined(ATANKS_IS_WINDOWS)
	// Here srand() is thread local according to MSDN.
	// This affects cygwin/mingw builds, too.
	// Thanks to billy Buerger for pointing this out!
	srand(time(nullptr));
#endif // Microsoft Windows Build

	/// === Step 1 : Copy relevant data ===
	ai_level      = static_cast<int32_t>(player->type);
	ai_level_d    = static_cast<double>(ai_level);
	ai_over_mod   = 1. + (ai_level_d / 10.); // [1.1;1.5]
	ai_type_mod   = (1. + ai_level_d) / 2.;  // [1.0;3.0]
	blast_min     = 0.;
	blast_med     = 0.;
	blast_big     = 0.;
	blast_max     = 0.;
	isShocked     = false;
	revengee      = nullptr;
	shocker       = nullptr;
	needSuccess   = true;
	needAim       = true;
	isBlocked     = false;
	hill_detected = false;

	// Data from player:
	needMoney = ((player->getMoneyToSave(false) - player->money) > 0);
	last_opp  = player->getOppMem(-1);

	// Data from tank:
	tank      = player->tank;
	if (tank && !tank->destroy) {
		angle     = tank->a;
		power     = tank->p;
		weap_idx  = tank->cw;
		buried    = tank->howBuried(&buried_l, &buried_r);
		currLife  = tank->l + tank->sh;
		maxLife   = tank->getMaxLife();
		x         = tank->x;
		y         = tank->y;
		last_ang  = 180;
		last_pow  = 1000;
		last_weap = 0;

		// Is there a last opponent?
		if (last_opp) {
			last_ang  = angle;
			last_pow  = power;
			last_weap = weap_idx;
			DEBUG_LOG_AI(player->getName(), "Last opponent was %s",
						 last_opp->opponent->getName())
		}

		// Select last weapon/item used
		if (weap_idx > WEAPONS)
			useItem(weap_idx);
		else
			useWeapon(weap_idx);
	} else
		return false;

	// reset calculation values
	curr_angle     = angle;
	curr_power     = power;

	// Reset setup values:
	best_round_score     = NEUTRAL_ROUND_SCORE;
	best_setup_angle     = angle;
	best_setup_damage    = 0;
	best_setup_item      = nullptr;
	best_setup_mem       = nullptr;
	best_setup_overshoot = MAX_OVERSHOOT;
	best_setup_power     = power;
	best_setup_score     = NEUTRAL_ROUND_SCORE;
	best_setup_weap      = nullptr;


	/// === Step 2: See whether this bot gets lucky ===
	if ((rand() % 100) < ai_level) {
		// So the useless bot has a 1% and the deadly bot a 5% chance
		int32_t raise = 1 + ( (5 - ai_level) / 2);
		/* Useless: 1 + ((5 - 1) / 2) =>  1 + (4 / 2) => 1 + 2 => 3
		 * Guesser: 1 + ((5 - 2) / 2) =>  1 + (3 / 2) => 1 + 1 => 2
		 * Ranger : 1 + ((5 - 3) / 2) =>  1 + (2 / 2) => 1 + 1 => 2
		 * Target : 1 + ((5 - 4) / 2) =>  1 + (1 / 2) => 1 + 0 => 1
		 * Deadly : 1 + ((5 - 5) / 2) =>  1 + (0 / 2) => 1 + 0 => 1
		 */
		DEBUG_LOG_AI(player->getName(),
					  "Lucky Turn: Raise from \"%s\" to \"%s\"",
					  getLevelName(ai_level),
					  getLevelName(ai_level + raise))
		ai_level     += raise;
		ai_level_d    = static_cast<double>(ai_level);
		ai_over_mod   = 1. + (ai_level_d / 10.); // [1.1;1.5]
		ai_type_mod   = (1. + ai_level_d) / 2.;  // [1.0;3.0]
		showFeedback("*lucky*", GREEN, -.8, TS_NO_SWAY, 100);
	}


	/// === Step 3 : Set stage and allow the work to be done ===
	if (tank && !tank->destroy && !isStopped && canWork && getMemory()) {
		// Note: Without the memory, no real work is possible.

		textAllowed.store(true);
		plStage     = PS_SELECT_TARGET;
		isWorking   = true;
		return true;
	}

	return false;
}


/// @brief Sanitize curr_angle and curr_power.
void AICore::sanitizeCurr()
{
	if (curr_angle <        90) curr_angle =        90;
	if (curr_angle >       270) curr_angle =       270;
	if (curr_power > MAX_POWER) curr_power = MAX_POWER;
	if (curr_power < MIN_POWER) curr_power = MIN_POWER;
	curr_power      -= curr_power % 5;
}


/// @brief show ai feedback if allowed and not skipping computer play.
/// Whenever a feedback message is shown, the AI sleeps for dur/10 + 1 ms.
void AICore::showFeedback(const char* const feedback, int32_t col, double yv,
                          eTextSway text_sway, int32_t dur)
{
	if (env.showAIFeedback && !global.skippingComputerPlay) {
		// Wait for the AI to be allowed to create texts
		while (!textAllowed.load(ATOMIC_READ))
			std::this_thread::yield();

		int32_t y_pos = y - (50 + (rand() % 21));
		new FLOATTEXT(feedback, x, y_pos, .0, yv, col, CENTRE, text_sway, dur, false);
		MSLEEP( (dur / 10) + 1 )
	}
}


/** @brief Select the next item to use on the current target.
  *
  * This method tries to determine the best item / weapon selection
  * to be used on the currently selected target (mem_curr).
  *
  * Some of the thinking depend on random numbers, so calling this
  * method twice on the same target might lead to different selections.
  *
  * This is wanted, so many tries on higher ai levels with a small
  * number of difficult to reach targets might eventually lead to
  * a sane result.
  *
  * The selected item / weapon is saved in item_curr or weap_curr. The
  * method makes sure that it is not the same as item_last / weap_last.
  * Please note, however, that if there is only one selectable item,
  * if the bot is out of stock of everything but small missiles for example,
  * then item_curr / weap_curr might end up the same as the last selections.
  *
  * The method returns true if the selection it ends up with makes sense.
  * If @a is_last is set to true, the method itself returns true, too.
  *
  * @param[in] is_last If set to true, the method will return true in any case.
  * @return true if the selection makes sense, or if @a is_last is set to true.
**/
bool AICore::selectItem(bool is_last)
{
	// Back up current selections
	item_last = item_curr;
	weap_last = weap_curr;

	// Advance to the next weapon
	bool has_weap = true;

	// If a best setup with primary target hit has been achieved
	// already, or if the bot is shocked, select a random weapon.
	// Otherwise do an ordered advance down the chain.
	if (best_setup_prime || isShocked) {
		int32_t weap_num = rand() % WEAPONS;
		weap_curr = weap_head;

		// If weap_last was not set, set it to head, too.
		if (!weap_last)
			weap_last = weap_curr;

		// Now rotate until the weapon is found.
		while (weap_num) {
			weap_curr = weap_curr->next;

			// Skip not available weapons, non-damage entries, the last weapon
			// and weapons with a negative score.
			while ( weap_curr
			     && ( (weap_curr->amount <= 0)
			       || (weap_curr->dmgSingle < 2.)
			       || (weap_curr->score <= 0)
			       || (weap_curr == weap_last)
			       // Reducer and dirt weapons are non-damage, too.
			       // they have a fake damage set, so filter them here.
			       || (REDUCER == weap_curr->type)
			       || ( (DIRT_BALL <= weap_curr->type)
			         && (SMALL_DIRT_SPREAD >= weap_curr->type) ) ) )
				weap_curr = weap_curr->next;

			// Rotate if the end was hit
			if (!weap_curr)
				weap_curr = weap_head;

			--weap_num;
		}
	} else {
		while ( has_weap
		    && ( !weap_curr
		      || (weap_curr == weap_last)
		      || (weap_curr->amount <= 0)
		      || (weap_curr->dmgSingle < 2.)
		      || (weap_curr->score <= 0)
		      || ( mem_curr->hasRepulse
		        && RAND_AI_1P
		        && (SML_NAPALM <= weap_curr->type)
		        && (LRG_NAPALM >= weap_curr->type)
		        && RAND_AI_1P ) ) ) {
			weap_curr = weap_curr ? weap_curr->next : weap_head;

			// If no weapon was selected at the start, weap_last is
			// now nullptr, but must be weap_head once weap_curr is
			// beyond head.
			if (!weap_last && (weap_curr != weap_head))
				weap_last = weap_head;

			// If this rotated once through everything, there is
			// only this one weapon left or a lot of tries are through:
			if (weap_last == weap_curr)
				has_weap = false;
		}
	} // end of ordered rotation

	// Use weap_head if there is no other weapon:
	if (!has_weap) {
		weap_curr = weap_head;
		weap_last = weap_curr;
	}

	// If the bot is shocked, the next weapon is selected,
	// someone in panic does not do much thinking any more
	if (isShocked) {
		item_curr = nullptr;
		if (nullptr == weap_curr)
			weap_curr = weap_head;

		weap_idx = weap_curr->type;

		DEBUG_LOG_EMO(player->getName(), "(SHOCKED) Quick selected %s",
		              weapon[weap_idx].getName())
		return true;
	}

	// Advance to the next item
	bool has_item = true;
	while ( has_item
	    && ( !item_curr
	      || (item_curr == item_last)
	      || (item_curr->amount <= 0)
	      || !item_curr->selectable
	      || (item_curr->score <= 0) ) ) {
		item_curr = item_curr ? item_curr->next : item_head;

		// If no item was selected at the start, item_last is
		// now nullptr, but must be item_head once item_curr is
		// beyond head.
		if (!item_last && (item_curr != item_head))
			item_last = item_head;

		// If this rotated once through everything, there is
		// only this one weapon left:
		if (item_last == item_curr)
			has_item = false;
	}

	// If no items are available, it has to be taken out of consideration:
	if (!has_item) {
		item_curr = nullptr;
		item_last = nullptr;
	}


	// Do not use items with a negative score, as those are items
	// that are unavailable.
	if (item_curr
	  && ( (item_curr->score < 0)
	    || (player->ni[item_curr->type] <= 0) ) )
		item_curr = nullptr;

	// Note: sub-optimal weapons ( too low damage ) can be negative.

	// Do not use self destruct items/weapons unless the
	// bot wants to self destruct
	if (mem_curr->opLife <= (currLife * 10.)) {
		if (item_curr && (item_curr->kamikaze))
			item_curr = nullptr;
		if (weap_curr && (weap_curr->kamikaze))
			weap_curr = nullptr;
	}

	// Do not use teleporters unless buried or blocked
	if ( item_curr
	  && (item_curr->type >= ITEM_TELEPORT)
	  && (item_curr->type <= ITEM_MASS_TELEPORT)
	  && !isBlocked
	  && !item_curr->escape)
		item_curr = nullptr;

	// Do not use riot bombs if the path is not blocked
	if ( weap_curr
	  && (weap_curr->type >= RIOT_BOMB)
	  && (weap_curr->type <= HVY_RIOT_BOMB)
	  && !isBlocked)
		weap_curr = nullptr;

	// If both are still set, take what has the higher score
	if (item_curr && weap_curr) {
        if (item_curr->score > weap_curr->score) {
			weap_curr = nullptr;
			weap_idx  = item_curr->type + WEAPONS;
		} else {
			item_curr = nullptr;
			weap_idx  = weap_curr->type;
		}
	} else if (item_curr)
		weap_idx = item_curr->type + WEAPONS;
	else if (weap_curr)
		weap_idx = weap_curr->type;
	else
		weap_idx = -1;

	if (!item_curr && !weap_curr && is_last)
		// If nothing is set but this is the last chance,
		// use the small missile as a fallback weapon
		useWeapon(SML_MIS);


	DEBUG_LOG_EMO(player->getName(), "Next selection: %s",
	              weap_curr
	              ? weapon[weap_idx].getName()
	              : item_curr
	                ? item[weap_idx - WEAPONS].getName()
	                : "NOTHING (fail)")

	return (item_curr || weap_curr);
}


/** @brief Select the next target to try  to hit or handle.
  *
  * This method selects and sets the current target. Basically it
  * just wanders down the memory chain as it is sorted by score already.
  *
  * The following additional rules (besides the ordering by score) apply:
  *   - If the bot is shocked, the shocker is always selected.
  *   - If a revenge is sought, that opponent is always selected if it
  *     is not currently selected or was the last one.
  *   - If @a is_last is set to true, the target with the highest score
  *     or that is sought revenge against is selected and the method
  *     returns true.
  *
  * Whenever the selection makes sense, the method returns true.
  *
  * @param[in] is_last If set to true, the method will return true in any case.
  * @return true if the selection makes sense, or if @a is_last is set to true.
**/
bool AICore::selectTarget(bool is_last)
{
	// Be quickly done if this bot is shocked
	if (isShocked) {

		// Is the shocker still there?
		if (shocker->opponent->tank && !shocker->opponent->tank->destroy) {
			mem_last = mem_curr;
			if (!mem_curr || (mem_curr->entry != shocker)) {
				mem_curr = mem_head;
				while (mem_curr && (mem_curr->entry != shocker))
					mem_curr = mem_curr->next;
			}

			DEBUG_LOG_EMO(player->getName(), "(SHOCKED) Targetting %s",
						  mem_curr ? mem_curr->entry->opponent->getName() : "NONE?")

			return (mem_curr->entry == shocker);
		} else {
			// Nope, gone with the wind.
			shocker   = nullptr;
			isShocked = false;
		}
	}

	// Preselect the revengee if not done already and there is one:
	// Note: Of course the revengee is forced if this is the very last try!
	if ( revengee
	  && ( is_last
		|| ( (!mem_curr || (mem_curr->entry != revengee))
		  && (!mem_last || (mem_last->entry != revengee)) ) ) ) {

		// is the revengee still alive?
		if (revengee->opponent->tank && !revengee->opponent->tank->destroy) {
			mem_last = mem_curr;
			mem_curr = mem_head;
			while (mem_curr && (mem_curr->entry != revengee))
				mem_curr = mem_curr->next;

			DEBUG_LOG_EMO(player->getName(), "(REVENGE) Targetting %s",
						  mem_curr ? mem_curr->entry->opponent->getName() : "NONE?")

			return (mem_curr->entry == revengee);
		} else
			// No longer relevant...
			revengee = nullptr;
	}

	// If nothing was preselected, a simple walk down the chain
	// is in order. (revengees must be skipped, though)
	// However, if this is the last_try, the primary target is always selected.
	sOppMemEntry* mem_old = mem_curr; // backup

	if (is_last || (nullptr == mem_curr))
		mem_curr = mem_head;

	// If the revengee is currently selected, the "walk" must continue
	// from the last opponent on, or the bot will have a flip between
	// the revengee and the first other opponent only.
	if (!is_last && revengee && mem_curr && (mem_curr->entry == revengee)) {
		if (mem_last)
			mem_curr = mem_last;
		else
			mem_curr = mem_head;
	}

	// Now walk down the list skipping the revengee if set
	if (!is_last) {
		while ( mem_curr
		    && ( (mem_curr == mem_old)
		      || (mem_curr == mem_last)
		      || (revengee && (mem_curr->entry == revengee))
		      || (mem_curr->entry->opponent == player)
		      || (nullptr == mem_curr->entry->opponent->tank)
		      || mem_curr->entry->opponent->tank->destroy) )
			mem_curr = mem_curr->next;
		mem_last = mem_old;
	}

	// If is_last is set, mem_curr must not be nullptr. Otherwise a
	// nullptr can happen if too few tanks are left.
	assert ( (!is_last || mem_curr) && "ERROR: Is last but nullptr curr!");

	DEBUG_LOG_EMO(player->getName(), "( normal) Targetting %s",
				  mem_curr ? mem_curr->entry->opponent->getName() : "nobody")

	return (nullptr != mem_curr);
}


/** @brief setup the basic attack values
  *
  * This method tries to find a sane target-weapon-combination.
  * The number of tries to do so is dictated by the AI level,
  * and if this is the very last targeting try, the method will
  * come up with the minimum possible combination.
  *
  * @param[in] is_last If set to true, something usable is forced to be set.
  * @return true if a viable combination was found, false otherwise.
**/
bool AICore::setupAttack(bool is_last, int32_t &opp_attempt, int32_t &weap_attempt)
{
	bool selectDone     = false;
	bool breakUp        = false;
	bool has_new_target = false;

	while (isWorking && !isStopped && !selectDone && !breakUp) {

		// Yield on each iteration to not hog the CPUs
		if (!global.skippingComputerPlay)
			std::this_thread::yield();

		// 1) Select a target
		//====================
		if (!weap_attempt || !mem_curr) {
			plStage = PS_SELECT_TARGET;

			DEBUG_LOG_AIM(player->getName(), "Selecting target, try %d / %d",
						  opp_attempt + 1, findOppAttempts)

			selectDone = selectTarget( (++opp_attempt == findOppAttempts)
			                        && is_last && needSuccess);
			if (selectDone && (mem_curr != mem_last)) {
				best_round_score = NEUTRAL_ROUND_SCORE; // New target!
				item_curr = nullptr;
				item_last = nullptr;
				weap_curr = nullptr;
				weap_last = nullptr;
				has_new_target = true;
			}
		} else {
			has_new_target = false;
			selectDone = true;
		}

		// 2) Select an item / weapon
		//============================
		if (selectDone) {
			plStage = PS_SELECT_WEAPON;

			/* --- Ensure dedicated lists and starting values --- */
			if (has_new_target) {
				checkWeapMem();
				checkItemMem();
				weap_idx  = -1;
			}

			/* --- Now do the selection --- */
			selectDone = false;
			while ( isWorking && !isStopped && !selectDone
			    && (weap_attempt < findWeapAttempts) ) {
				DEBUG_LOG_AIM(player->getName(), "Selecting item, try %d / %d",
							  weap_attempt + 1, findWeapAttempts)

				// Yield on each iteration to not hog the CPUs
				if (!global.skippingComputerPlay)
					std::this_thread::yield();

				selectDone = selectItem( (++weap_attempt == findWeapAttempts)
				                      && is_last && needSuccess);
			}

			// selectItem() must have set weap_idx properly:
			assert( ( !selectDone
			       || ( item_curr && (weap_idx == WEAPONS + item_curr->type) )
			       || ( weap_curr && (weap_idx ==           weap_curr->type) ) )
			     && "ERROR: Selection done but weap_idx does not"
			     && " contain the correct index!");

			// If this was the last attempt to select a weapon, and another
			// opponent selection is in order, reset weap_attempt so a new
			// opponent can be selected
			if ( (weap_attempt == findWeapAttempts)
			  && (opp_attempt  <  findOppAttempts) )
				weap_attempt = 0;
		} // end of item selection

		// break up this round if no selection was done but the
		// opponent selection has ended
		if (!selectDone
		  && (opp_attempt  >= findOppAttempts)
		  && (weap_attempt >= findWeapAttempts) )
			breakUp = true;
	} // end of basic setup cycle

	// If the selection was not done properly (not possible) but this
	// is the absolutely final round and there has been no good setup, yet,
	// an emergency plan is used:
	if (isWorking && !isStopped && is_last && !selectDone && needSuccess) {
		// Try to "get out" first:
		selectDone = useItem(ITEM_TELEPORT);

		if (!selectDone && !mem_curr->is_buried)
			selectDone = useItem(ITEM_SWAPPER);

		if (!selectDone)
			selectDone = useItem(ITEM_MASS_TELEPORT);

		// If this still isn't going anywhere, revert to first target with
		// small missiles, that might be useless now, but there is always
		// a next round.
		if (!selectDone) {
			item_curr  = nullptr;
			mem_curr   = mem_head;
			selectDone = useWeapon(SML_MIS);

			DEBUG_LOG_EMO(player->getName(),
			              "Last Try Selection: %s against %s",
			              weapon[SML_MIS].getName(),
			              mem_head->entry->opponent->getName())

			assert(selectDone && "ERROR: Not even small missile can be selected?");
		} else
			weap_curr = nullptr; // Emergency plan working.
	}


	// The last thing to consider is kamikaze.
	// While the appropriate items *are* self destruct devices, choosing
	// a weapon with kamikaze potential (and score) does not mean that the
	// bot *must* destroy themselves. So in that case an extra test is in order.
	if ( selectDone
	  && ( (item_curr && item_curr->kamikaze)
	    || (weap_curr && weap_curr->kamikaze) ) ) {
		bool self_destruct = true;

		// selfPreservation is a value in the interval [0;3]
		if (weap_curr && ( (rand() % 35) < (player->selfPreservation * 10.)))
			self_destruct = false;

		// Another "way out" is if a weapon shall be used but it does
		// not do enough damage:
		if (self_destruct && weap_curr && (weap_curr->dmgSingle < currLife))
			self_destruct = false;

		// do not self destruct if a good best setup was already found
		if (!needSuccess && best_setup_prime && (best_setup_score > 0))
			self_destruct = false;

		// If the bot still wants to self destruct, change mem_curr
		// to reflect this:
        if (self_destruct && (mem_curr->entry->opponent != player) ) {
			mem_last = mem_curr;
			mem_curr = mem_head;
			while (mem_curr->entry->opponent != player)
				mem_curr = mem_curr->next;

			// This must never fail:
			assert(mem_curr && "ERROR: Self not found in memory?");

			DEBUG_LOG_EMO(player->getName(),
			              "Chosen to self destruct using %s",
						  weap_curr
						  ? weapon[weap_idx].getName()
						  : item_curr
							? item[weap_idx - WEAPONS].getName()
							: "NOTHING (fail)")

        } else {
			// To chicken out, the small missile is chosen if this is
			// the last thing to consider
			if (is_last && needSuccess)
				useWeapon(SML_MIS);
			else
				selectDone = false;
        }
	}


	// If both attempts, selecting an opponent and a weapon, have reached
	// the maximum tries, they get reset, so the next full targeting cycle
	// is triggered:
	if ( !is_last && breakUp) {
		opp_attempt  = 0;
		weap_attempt = 0;
	}

	return selectDone;
}


/** @brief start the work on one player
  *
  * This method starts working on a new player. All relevant
  * data is fetched from @a player_ that must not be nullptr.
  *
  * If a job is running, no new one is started.
  * If @a player_ is not an AI player, it is not handled.
  *
  * @param[in] player Pointer to the player to handle.
  * @return true if a job was started, false otherwise.
  */
bool AICore::start(PLAYER* player_)
{
	if ( canWork && player_ && !isWorking && !isStopped
	  && (player_->type > HUMAN_PLAYER)
	  && (player_->type < NETWORK_CLIENT)
	  && (PS_AI_IS_IDLE == plStage) ) {

		DEBUG_LOG_AI(player_->getName(), "==============================", 0)
		DEBUG_LOG_AI(player_->getName(), " AICore started for %s", player_->getName())
		DEBUG_LOG_AI(player_->getName(), "------------------------------", 0)

		lguard_t guard(actionMutex);
		plStage   = PS_AI_INITIALIZE;
		player    = player_;
		isWorking = true;
		actionCondition.notify_one();

		return true;
	}

	return false;
}


/** @brief Retrieve the current status of the AI
  *
  * The arguments will not be changed if the AI is not working
  * on any player.
  *
  * @param[out] aItem Receives the currently selected weapon/item
  * @param[out] aAngle Receives the currently set angle
  * @param[out] aPower Receives the currently set power
  * @param[out] pl_stage Receives the current stage of the AI. This is always sent.
  * @return true if the AI is still working, false if it has finished.
  */
bool AICore::status(int32_t &aItem, int32_t &aAngle,
					int32_t &aPower, ePlayerStages &pl_stage)
{
	pl_stage = plStage;

	if (isWorking) {
		aItem  = weap_idx;
		aAngle = angle;
		aPower = power;
		return true;
	}

	return false;
}


/// @brief Tell the thread to stop even if it is not finished.
void AICore::stop()
{
	lguard_t guard(actionMutex);
	isStopped = true;
	actionCondition.notify_one();
}


/** @brief Trace the sub munition of a cluster type weapon
  *
  * Damage is recorded in the opponent memory dmgDone value.
  *
  * @param[in] subType The type of the sub munition. No checks done!
  * @param[in] subCount The number of sub munition parts.
  * @param[in] sub_x Trigger x coordinate.
  * @param[in] sub_y Trigger y coordinate.
  * @param[in] inh_xv Parent missile xv the moment it triggered.
  * @param[in] inh_yv Parent missile yv the moment it triggered.
**/
void AICore::traceCluster(int32_t subType, int32_t subCount,
                          int32_t sub_x, int32_t sub_y,
                          double inh_xv, double inh_yv)
{
	double    divergence   = weapon[weap_curr->type].divergence;
	double    speedVar     = weapon[weap_curr->type].speedVariation;
	double    spreadVar    = weapon[weap_curr->type].spreadVariation;
	WEAPON*   sub_weap     = &weapon[subType];
	double    divStep      = static_cast<double>(divergence)
	                       / static_cast<double>(subCount - 1);
	int32_t   startPoint   = divStep < 0. ? 0 : 180;
	int32_t   randStart    = rand () % 1000000;
	ePhysType subPhys      = PT_NORMAL;
	int32_t   startY       = sub_y - 20;
	int32_t   cl_overshoot = MAX_OVERSHOOT;
	int32_t   old_overshoot= curr_overshoot; // overshoot is only used for mirvs and funkies
	double    radius       = sub_weap->radius;
	double    sub_dmg      = sub_weap->damage * player->damageMultiplier;

	// If the weapon is fired into a ceiling, adapt starting y
	if (env.isBoxed
	  && (startY <= BOXED_TOP)
	  && ( (WALL_STEEL == env.current_wallType)
		|| ( (WALL_WRAP  == env.current_wallType)
		  && (!env.isBoxed || !env.do_box_wrap) ) ) )
		startY = MENUHEIGHT + 20;

	// Change physics of the sub munitions for the funky bomb
	if ( (weap_curr->type == FUNKY_BOMB) || (weap_curr->type == FUNKY_DEATH) )
		subPhys = PT_FUNKY_FLOAT;

	// If this is a steel wall hit, the start point angle needs
	// to be adapted. And erased if this is a ceiling hit
	if (WALL_STEEL == env.current_wallType) {
		if ( (CLUSTER <= weap_curr->type) && (SUP_CLUSTER >= weap_curr->type) ) {
			if ( x < 2 )
				startPoint -= divergence + 1 + (rand() % 10);
			else if ( x > (env.screenWidth - 3) )
				startPoint += divergence + 1 + (rand() % 10);
			else if ( y <= BOXED_TOP)
				startPoint = 0;
		} else if ( (SML_NAPALM <= weap_curr->type)
				 && (LRG_NAPALM >= weap_curr->type) ) {
			if ( x < 2 )
				startPoint -= 10 + rand() % 21;
			else if ( x > (env.screenWidth - 3) )
				startPoint += 10 + rand() % 21;
			else if ( y <= BOXED_TOP)
				startPoint = 0;
		} else if ( ( (SMALL_MIRV == weap_curr->type)
		           || (CLUSTER_MIRV == weap_curr->type) )
		         && ( y <= BOXED_TOP) ) {
			startPoint = 0;
			inh_yv     = std::abs(inh_yv);
		}
	}

	// The spread can be created!
	for (int32_t sc = 0; sc < subCount; ++sc) {
		double   speed        = weapon[weap_curr->type].launchSpeed;
		int32_t  newMissCount = sub_weap->countdown;
		int32_t  newMissAngle = ROUND(
					  (divStep * static_cast<double>(sc))
					+  static_cast<double>(startPoint)
					- (static_cast<double>(divergence) / 2.) );

		// trace hard, but yield per sub mun
		if (!global.skippingComputerPlay)
			std::this_thread::yield();

		// Manipulate angle if applicable
		if (speedVar > 0.)
			newMissAngle += ROUND(
						  static_cast<double>(divergence)
						* spreadVar
						* Noise(randStart + 1054 + sc) );

		// Be sure the angle is valid
		while (newMissAngle < 0)
			newMissAngle += 360;
		newMissAngle %= 360;

		// Manipulate number of submunition projectiles if applicable
		if (sub_weap->countVariation > 0) {
			newMissCount += ROUND(
						  static_cast<double>(sub_weap->countdown)
						* sub_weap->countVariation
						* Noise(randStart + 78689 + sc) );
			// This might go wrong, so be sure it doesn't
			if (newMissCount <= 0)
				newMissCount = 0;
		}

		// Manipulate launching speed if applicable
		if (speedVar > 0)
			speed += ROUND(speedVar * speed * Noise(randStart + 124786 + sc) );

		// Launch new submunition missile
		MISSILE mind_shot(player, sub_x, startY,
		                  env.slope[newMissAngle][0] * speed * env.FPS_mod + inh_xv,
		                  env.slope[newMissAngle][1] * speed * env.FPS_mod + inh_yv,
		                  subType, MT_MIND_SHOT, ai_level, 0);
		mind_shot.update_submun(subPhys, newMissCount);

		// Keep flying/rolling/digging/whatever until the missile hits something
		// or the number of bounces is too high for this bot to keep track.
		while (!mind_shot.destroy && (maxBounce >= mind_shot.bounced())) {
			mind_shot.applyPhysics();

			// Yield on each iteration to not hog the CPUs
			if (!global.skippingComputerPlay)
				std::this_thread::yield();
		}

		// If the missile is destroyed, the number of bounces is in order.
		if (mind_shot.destroy) {
			// The distance from the target must take both the direction
			// of the last movement of the mind shot and the positions of
			// both tanks into account:
			int32_t tank_dir = SIGN(mem_curr->opX - x);
			int32_t hit_dir  = SIGN(mem_curr->opX - mind_shot.x);
			int32_t shot_dir = mind_shot.direction();

			curr_overshoot = ABSDISTANCE2(mem_curr->opX + offset_x,
			                              mem_curr->opY + offset_y,
										  mind_shot.x,   mind_shot.y);

			if (tank_dir == shot_dir)
				curr_overshoot *= (tank_dir == hit_dir ? -1 :  1);
			else
				curr_overshoot *= (tank_dir == hit_dir ?  1 : -1);

			// Note it down if this mind shot is better than the best
			if (std::abs(curr_overshoot) < std::abs(cl_overshoot))
				cl_overshoot = curr_overshoot;

			// eventually add the score:
			calcHitDamage(mind_shot.x, mind_shot.y, radius, sub_dmg,
			              static_cast<weaponType>(subType));

		}
	} // End of looping submunitions

	// Write back best overshoot if this is a MIRV or funky weapon.
	// clusters and napalm weapons spread stuff out, there the hit
	// of the parent weapon is what counts.
	if ( (SMALL_MIRV   == weap_curr->type)
	  || (FUNKY_BOMB   == weap_curr->type)
	  || (FUNKY_DEATH  == weap_curr->type)
	  || (CLUSTER_MIRV == weap_curr->type) )
		curr_overshoot = cl_overshoot;
	else
		curr_overshoot = old_overshoot;
}


/** @brief Fire a mind shot and see where it goes.
  *
  * If the mind shot got destroyed, curr_overshoot is set to the distance of
  * mem_curr->opponent to reached_x_/reached_y_. Positive values mean "too far",
  * negative values mean the shot went "too short".
  *
  * @param[in]  trace_angle The angle to use, normally a spread variation of
  *             curr_angle.
  * @param[in]  delay_idx Index of this shot in a delayed shot. Used to simulate
  *             "bombing through" terrain.
  * @param[out] finished true if the mind shot got destroyed before reaching
  *             maxBounce wall bounces/wraps.
  * @param[out] top_wrapped Set to true if the the missile wrapped through a
  *             wrap wall ceiling.
  * @param[out] reached_x_ The x-coordinate on destruction. If the mind shot was
  *             cancelled before being destroyed, @a reached_x_ is not changed.
  * @param[out] reached_y_ The y-coordinate on destruction. If the mind shot was
  *             cancelled before being destroyed, @a reached_y_ is not changed.
  * @param[out] end_xv The x velocity the moment the projectile ends.
  * @param[out] end_yv The y velocity the moment the projectile ends.
  * @return true if all went well, false if any new() operator failed. If this
  *         method returns false, the AI can no longer work.
**/
bool AICore::traceShot(int32_t trace_angle, int32_t delay_idx,
                       bool &finished, bool &top_wrapped,
                       int32_t &reached_x_, int32_t &reached_y_,
                       double &end_xv, double &end_yv)
{
	double   top_x        = x;
	double   top_y        = y;
	double   vel_mod      = static_cast<double>(curr_power) * env.FPS_mod;
	double   vel_x        = env.slope[trace_angle][0] * vel_mod / 100.;
	int32_t  aim_dir      = SIGN(vel_x);
	double   vel_y        = env.slope[trace_angle][1] * vel_mod / 100.;
	bool     can_top_wrap = ( env.isBoxed
	                       && (WALL_WRAP == env.current_wallType)
	                       && env.do_box_wrap );
	double   old_yv       = 0;
	double   old_y        = 0;

	tank->getGuntop(trace_angle, top_x, top_y);

	MISSILE mind_shot(player, top_x, top_y, vel_x, vel_y, weap_idx,
	                  MT_MIND_SHOT, ai_level, delay_idx);

	// Adapt missile drag if the player has dimpled/slick projectiles
	if (player->ni[ITEM_DIMPLEP])
		mind_shot.drag *= item[ITEM_DIMPLEP].vals[0];
	else if (player->ni[ITEM_SLICKP])
		mind_shot.drag *= item[ITEM_SLICKP].vals[0];

	// Keep flying/rolling/digging/whatever until the missile hits something
	// or the number of bounces is too high for this bot to keep track.
	while (!mind_shot.destroy && (maxBounce >= mind_shot.bounced())) {
		if (can_top_wrap) {
			old_yv = vel_y;
			old_y  = mind_shot.y;
		}

		mind_shot.applyPhysics();

		if (can_top_wrap) {
			mind_shot.getVelocity(vel_x, vel_y);
			if ( ( (old_yv < 0.) && (vel_y < 0.) && (mind_shot.y > old_y) )
			  || ( (old_yv > 0.) && (vel_y > 0.) && (mind_shot.y < old_y) ) )
				top_wrapped = true;
		}

		if (!global.skippingComputerPlay)
			std::this_thread::yield();
	}

	// If the missile is destroyed, the number of bounces is in order.
	if (mind_shot.destroy) {
		mind_shot.getVelocity(end_xv, end_yv);
		finished      = true;
		reached_x_    = mind_shot.x;
		reached_y_    = mind_shot.y;

		// The distance from the target must take both the direction
		// of the last movement of the mind shot and the positions of
		// both tanks into account:
		int32_t tank_dir = SIGN(mem_curr->opX - x);
		int32_t hit_dir  = SIGN(mem_curr->opX - reached_x_);
		int32_t shot_dir = mind_shot.direction();

		// If the shot was flipped, and is therefore shooting into the opposite
		// direction of the target, the distance between the impact and the wall
		// shooting at must be added if the shot went away from the target.
		// However, this only matters if no bounces have been recorded, yet.
		int32_t flip_offset = 0;
		if ( hasFlipped && (aim_dir == shot_dir)
		  && (WALL_STEEL != env.current_wallType)
		  && (0 == mind_shot.bounced())) {
			if (reached_x_ < x)
				flip_offset = 2 * reached_x_;
			else
				flip_offset = 2 * (env.screenWidth - reached_x_);
		}

		curr_overshoot = ABSDISTANCE2(mem_curr->opX + offset_x + flip_offset,
		                              mem_curr->opY + offset_y,
		                              reached_x_, reached_y_);

		if (tank_dir == shot_dir)
			curr_overshoot *= (tank_dir == hit_dir ? -1 :  1);
		else
			curr_overshoot *= (tank_dir == hit_dir ?  1 : -1);
	} else {
		finished       = false;
		curr_overshoot = MAX_OVERSHOOT;
	}

	return true;
}


/** @brief trace all shots from weap_curr and fill in the arguments.
  *
  * Damage is recorded in the opponent memory dmgDone value.
  *
  * @return best overshoot.
**/
void AICore::traceWeapon(int32_t &has_crashed, int32_t &has_finished)
{
	assert(weap_curr && "ERROR: traceWeapon() with nullptr weap_curr?");

	int32_t trace_overshoot= MAX_OVERSHOOT;
	int32_t curr_reached_x = reached_x;
	int32_t curr_reached_y = reached_y;
	double  end_xv         = 0.;
	double  end_yv         = 0.;

	has_crashed    = 0;
	has_finished   = 0;
	curr_prime_hit = false;

	// reset virtual damage on opponents.
    opEntry_t* opp = mem_head;
    while (opp) {
		opp->dmgDone = 0;
		opp = opp->next;
    }

	// Loop by spread, weapons that do not spread have a value of 1.
	for (int32_t i = 0 ;
		 canWork && !isStopped && (i < weap_curr->spread) ;
		 ++i) {
		int32_t tr_a     = curr_angle
		                 + (  (SPREAD * i)
		                    - (SPREAD * (weap_curr->spread - 1) / 2) );
		bool    finished = false;
		bool    top_wrap = false;
		reached_x  = x;
		reached_y  = y;

		// Loop again by delay, weapons that have no delay default to 1 here.
		for (int32_t j = 0 ;
			 canWork && !isStopped && (j < weap_curr->delay) ;
			 ++j) {
			if (traceShot(tr_a, j, finished, top_wrap, curr_reached_x, curr_reached_y,
						  end_xv, end_yv)
			  && finished) {

				++has_finished;

				// Check whether the shot crashed
				if ( ( env.isBoxed
				    && ( (curr_reached_y <= BOXED_TOP ) // crashed on top
				      // wrapped to bottom with dirt above is a ceiling crash, too.
				      || ( top_wrap
				        && ( (weap_curr->type < BURROWER) || (weap_curr->type > PENETRATOR) )
				        && (curr_reached_y > global.surface[curr_reached_x].load()) ) ) )
				  // Steel wall have additional wall crashes
				  || ( (WALL_STEEL == env.current_wallType)
				    && (weap_curr->subMunCount < 1) // Clusters never crash
				    && ( (curr_reached_x <= 2)
				      || (curr_reached_x >= (env.screenWidth - 3) ) ) ) ) {

						++has_crashed;
				} // end of omni-crash-check

				if (weap_curr->subMunCount > 0) {
					double inh_xv = weapon[weap_curr->type].impartVelocity * end_xv;
					double inh_yv = weapon[weap_curr->type].impartVelocity * end_yv;
					// Trace the cluster parts and add their score:
					traceCluster(weap_curr->subMunType, weap_curr->subMunCount,
								 curr_reached_x, curr_reached_y, inh_xv, inh_yv);
				} else
					// Calculate the damage for the hit, crashes are handled later
					calcHitDamage(curr_reached_x, curr_reached_y,
								  static_cast<double>(weap_curr->radius),
								  weap_curr->dmgSingle,
								  static_cast<weaponType>(weap_curr->type));

				// Note best overshoot if any
				if (std::abs(curr_overshoot) < std::abs(trace_overshoot)) {
					trace_overshoot = curr_overshoot;
					reached_x       = curr_reached_x;
					reached_y       = curr_reached_y;
				}
			} // End of if traceShot
		} // End of delay loop
	} // End of spread loop

	// Write back best data found:
	curr_overshoot = trace_overshoot;
}


/// @brief Set a new score to an items entry
void AICore::updateItemScore(itEntry_t* pItem)
{
	/* There aren't many items that are actually usable.
	 * 1. Teleporters
	 *    These can be used to get out of a buried scenario.
	 *    Further they might be an alternative if the
	 *    targeted tank is far away.
	 * 2. Fan
	 *    This item has no real use for the AI but one:
	 *    If the enemy is behind a mountain and the AI has tail wind,
	 *    then it might be helpful to change the wind direction.
	 *    However, this does only make sense if not that many other
	 *    bots have their shot until this one gets its next try.
	 * 3. Self destruct devices
	 *    If this bots tanks is almost dead, and the preferred target
	 *    has a lot of health left, then trying to take them with us
	 *    using a big boom is somewhat compelling.
	 * 4. Fuel and rockets.
	 *    Fuel can be used to get away from a steep wall, rockets can
	 *    be used to get out of a steep canyon.
	 */


	// === Get out quickly if the chosen item is out of stock ===
	// ==========================================================
	if (0 == pItem->amount) {
		pItem->score = -50000;
		return;
	}


	// === Only evaluate items that are available ===
	// ==============================================
	if (!env.isItemAvailable(pItem->type + WEAPONS)) {
		pItem->score = -100000;
		return;
	}

	DEBUG_LOG_AI(player->getName(), "Evaluating score for %s",
	             item[pItem->type].getName())

	// reset helper boolean
	pItem->escape = false;


	/* -------------------------------------------------------------
	 * --- 1) Set score for freeing capabilities while buried    ---
	 * ------------------------------------------------------------- */
	double unbury_score = 0;
    if (buried > BURIED_LEVEL) {
		double bury_diff = buried - BURIED_LEVEL;
		double off_mod   = (player->defensive - 1.5) / -2.;

		if (ITEM_TELEPORT == pItem->type)
			unbury_score = bury_diff * ai_level * off_mod
			               // It is more valuable when the target is buried,
			               // as it is not desirable to swap with them
			             * (mem_curr->is_buried ? 3. : 1.);

		else if (ITEM_SWAPPER == pItem->type)
			unbury_score = bury_diff * ai_level * off_mod * 2.
			               // If the target is buried, the swapper is no good.
			             * (mem_curr->is_buried ? -50. : 2.);

		else if (ITEM_MASS_TELEPORT == pItem->type)
			unbury_score = bury_diff * ai_level * off_mod * 1.25;

		else if (ITEM_FAN == pItem->type)
			// at least the bot might think the usage is safe.
			unbury_score = bury_diff
			             * static_cast<double>(maxAiLevel + 1 - ai_level)
			             * off_mod / 2.;
		else
			// Everything else is useless
			unbury_score = -5000.;

		// "escape" tool ?
		if ( (ITEM_TELEPORT <= pItem->type)
		  && (ITEM_MASS_TELEPORT >= pItem->type) )
			pItem->escape = true;
    }


	/* -------------------------------------------------------------
	 * --- 2) The wind direction change score for fans           ---
	 * ------------------------------------------------------------- */
	double fan_score = 0.;
	if ( (ITEM_FAN == pItem->type)
	  // The fan can only be considered useful if the bot has tail wind:
	  && ( ( (mem_curr->opX > tank->x) && (global.wind > 0.) )
	    || ( (mem_curr->opX < tank->x) && (global.wind < 0.) ) ) ) {

        // First count how many other bots can have their turn until
        // this one will get its next chance:
        opEntry_t* check   = mem_head;
        int32_t    between = 0;
        while (check) {
			if ( (check->entry->opponent != player) && check->alive)
				++between;
			check = check->next;
        }

		// Now look whether there is a mountain in between:
		int32_t check_x   = ROUND(mem_curr->opX);
		int32_t checked   = 0;
		int32_t direction = SIGN(global.wind) * -1;
		int32_t range_x   = 10 * (ai_level + RAND_AI_0P);
		int32_t top_ledge = env.screenHeight;

		while ( (checked < range_x)
		     && (check_x > 1)
		     && (check_x < (env.screenWidth - 2)) ) {
			int32_t check_y = global.surface[check_x].load(ATOMIC_READ);
			if (check_y < top_ledge)
				top_ledge = check_y;
			check_x += direction;
		}

		// Now the score is a simple height difference modified by defensiveness
		// (The more defensive the player is, the more it is inclined to prepare
		//  the next attack instead of pissing them of with a weak shot.)
		fan_score = static_cast<double>(top_ledge - mem_curr->opY)
		          * (player->defensive + ai_level_d);

		// However, the score is multiplied again with the count of opponents
		// that will have their try until this one gets its next shot.
		fan_score *= fan_score > 0.
		           ? static_cast<double>(ai_level - between) // normal multiplier
		           : static_cast<double>(between); // The more bots, the more useless.

		// However, if the bot already used the fan in the last round,
		// do not repeat, no matter what:
		if (last_weap == (ITEM_FAN + WEAPONS)) {
			DEBUG_LOG_EMO(player->getName(),
			              "=> Reducing fan score %6.2lf to %62lf (no repeat)",
			              fan_score, std::abs(fan_score * ai_level) * -1.);
			fan_score = std::abs(fan_score * ai_level) * -1.;
		}
	} // End of calculating a fan score


	/* -------------------------------------------------------------
	 * --- 3) Set score for self destruct probability            ---
	 * ------------------------------------------------------------- */
    double selfde_score = 0.;
    if ( (mem_curr->opLife > (currLife * 10.))
	  || (isShocked && (mem_curr->opLife > (currLife *  5.))) ) {
		if ( (ITEM_VENGEANCE >= pItem->type)
		  && (ITEM_FATAL_FURY <= pItem->type) ) {
			selfde_score = static_cast<double>(pItem->type - ITEM_VENGEANCE + 1)
			             * mem_curr->diffLife / (player->selfPreservation + .5);
		}
    }


	/* -------------------------------------------------------------
	 * --- 4) The "useless" score, for not usable items          ---
	 * ------------------------------------------------------------- */
	double useless_score = 0.;
	if ( (ITEM_FATAL_FURY < pItem->type)
	  && (ITEM_ROCKET != pItem->type) )
		useless_score = -50000.;
	/// @todo : FUEL should be made available to the AI somehow.


	/* -------------------------------------------------------------
	 * --- 5) Sum up the score                                   ---
	 * --- This will be used for sorting the items list          ---
	 * ------------------------------------------------------------- */
	double pref_score = pItem->preference / static_cast<double>(ai_level * 10);

	double xScore = unbury_score + selfde_score + useless_score + fan_score;

	if (useless_score > -1.) {
		DEBUG_LOG_EMO(player->getName(), "  preference   : %6.2lf%s", pref_score,
					xScore > 1. ? "" : " (ignored)")
		DEBUG_LOG_EMO(player->getName(), "  unbury_score : %6.2lf", unbury_score)
		DEBUG_LOG_EMO(player->getName(), "  fan_score    : %6.2lf", fan_score)
		DEBUG_LOG_EMO(player->getName(), "  selfde_score : %6.2lf", selfde_score)
		DEBUG_LOG_EMO(player->getName(), "  useless_score: %6.2lf", useless_score)
	}

	// Only add preferences if there is any use for the item:
	if (xScore > 1.)
		xScore += pref_score;

	pItem->score = ROUND(xScore);

	DEBUG_LOG_EMO(player->getName(), "  Final Score  : %8d", pItem->score)
}


/// @brief Set a new score to an opponents entry
void AICore::updateOppScore(opEntry_t* pOpp)
{
    sOpponent* entry    = pOpp->entry;
	PLAYER*    opponent = entry->opponent;
	TANK*      oppTank  = opponent->tank;

	DEBUG_LOG_AI(player->getName(), "Evaluating score for %s", opponent->getName())

	/* Quickly handle dead tanks and the own entry */
	if ( (player == opponent) || !oppTank || oppTank->destroy ) {
		entry->damage_from += entry->damage_last;
		entry->damage_last  = 0;
		pOpp->score = (player == opponent) ? -2 : -1;

		DEBUG_LOG_AI(player->getName(), "%s%s",
		             player == opponent ? "" : opponent->getName(),
		             player == opponent
		             ? "Not evaluating myself!"
		             : " is dead and not selectable!")
		return;
	}


	/* -------------------------------------------------------------
	 * --- 1) Set up a fear value (if needed)                    ---
	 * --- This is used to possibly trigger actions that may not ---
	 * --- be wise but are imposed by a sudden surge of fear.    ---
	 * ------------------------------------------------------------- */
	double fear_damage = 0.;
	double fear_shock  = 0.;
	if (!pOpp->onSameTeam) {
		entry->fear_shock = 0.;

		// The higher the AI level, the more the taken over fear is
		// reduced. *But* the more defensive the player is, the less
		// it is reduced. (Even more on a lucky turn. ;-)
		// Ranges are from useless full defensive to deadly full offensive:
		// From: 2.5 + 1 - ( 1 + 1) => 3.5 - 2 => 1.5 (only one third reduced)
		// To  : 2.5 + 5 - (-1 + 1) => 7.5 - 0 => 7.5 (~87% taken off)
		entry->fear /= 2.5 + ai_level_d
		             - (player->defensive + 1.0);

		// Only add new fear if there was any damage
		if (entry->damage_last > 0) {
			fear_damage  = player->painSensitivity * entry->damage_last;
			entry->fear += player->selfPreservation;
		}

		// first fear check:
		// If the AI can not stand the pain, the damage done is multiplied
		// with the fear value. This does not trigger any action, yet, but
		// the score will go up a lot.
		fear_shock = entry->fear - static_cast<double>(RAND_AI_0P);
		if ( (fear_damage > 0.) && (fear_shock > 0.) ) {
			fear_damage *= entry->fear;
			DEBUG_LOG_EMO(player->getName(),
			              "%s caused fear shock %lf with damage %u",
			              opponent->getName(), fear_shock,
			              ROUNDu(fear_damage))
			// Is this the new shocker?
			if ( (nullptr == shocker)
			  || (fear_shock > shocker->fear_shock) ) {
				DEBUG_LOG_EMO(player->getName(),
				              "%s %s%s%s as new shocker", opponent->getName(),
				              shocker ? "replaces" : "set",
				              shocker ? " " : "",
				              shocker ? shocker->opponent->getName() : "")
				shocker = entry;
			}
		} // End of having a fear shock
		entry->fear_shock = fear_shock;
	} // end of fear value handling

	/* -------------------------------------------------------------
	 * --- 2) Check damage for whether revenge is called for     ---
	 * ------------------------------------------------------------- */
	double revenge_score = 0.;
	if ( (entry->damage_last > 0) && !pOpp->onSameTeam) {

		// First reduce the current damage accumulated. More for lower level bots.
		if (!pOpp->revengeDone) {
			DEBUG_LOG_EMO(player->getName(), "Current anger damage from %s: %d",
						 opponent->getName(), entry->revenge_dmg)

			entry->revenge_dmg /= 4.5 - ai_type_mod;

			DEBUG_LOG_EMO(player->getName(), " --> Anger cooled down to   : %d",
						 entry->revenge_dmg)

			// Add current damage
			entry->revenge_dmg += entry->damage_last;

			DEBUG_LOG_EMO(player->getName(), " --> Anger raised again to  : %d",
						 entry->revenge_dmg)

			// Revenge damage handled:
			pOpp->revengeDone = true;
		}

		// Now see whether a new act of vengeance is initiated:
		if ( (entry->revenge_dmg > (player->vengeanceThreshold * maxLife))
		  && ( (rand() % 100) <= player->vengeful) ) {

			// Okay, the potential is there...
			revenge_score = static_cast<double>(entry->damage_last * player->vengeful)
			              / 100.;
			if ( (nullptr == revengee)
			  || (entry->revenge_dmg > revengee->revenge_dmg) ) {
				// A new one!
				DEBUG_LOG_EMO(player->getName(), " --> [%d] %s %s%s for revenge!",
				             entry->revenge_dmg,
				             entry->opponent->getName(),
				             revengee ? "replaces " : "is set ",
				             revengee ? revengee->opponent->getName() : "")
				revengee = entry;
			}
		}
	} // end of revenge value handling


	/* -------------------------------------------------------------
	 * --- 3) Check opponents health compared to this tank       ---
	 * --- The more health they got, the more money can be made. ---
	 * --- On the other hand, the bigger the difference, the     ---
	 * --- more impressive they are.                             ---
	 * -------------------------------------------------------------
	 */
	double life_score = 0.;
	if (pOpp->diffLife < 0.) {
		// The opponent has more health. This might impress the bot:
		if ( (rand() % static_cast<int32_t>(DEADLY_PLAYER)) < ai_level) {
			// No, there is nothing impressive with that...
			life_score = (player->defensive - 3.) / 2. * pOpp->diffLife;
			// Note:
			// Full Defensive : (-1 - 3) / 2 * -x => -4 / 2 * -x => -2 * -x = 2 * x
			// Full Offensive : ( 1 - 3) / 2 * -x => -2 / 2 * -x => -1 * -x = 1 * x

			// If the bot needs money, the opponents health might be added:
			if (needMoney && RAND_AI_0P)
				life_score += pOpp->opLife;
		}
	} else
		// Add points for their weakness, more if the bot is offensive
		life_score = (player->defensive + 3.) / 2. * pOpp->diffLife;
		// Note:
		// Full Defensive : (-1 + 3) / 2 * x => 2 / 2 * x = 1 * x
		// Full Offensive : ( 1 + 3) / 2 * x => 4 / 2 * x = 2 * x


	/* -------------------------------------------------------------
	 * --- 4) Add points for distance                            ---
	 * --- The theory is, that weaker bots concentrate on nearer ---
	 * --- enemies first, while stronger bots do not mind.       ---
	 * ------------------------------------------------------------- */
	double dist_score = (static_cast<double>(env.halfWidth) - pOpp->distance)
	                  / ai_level_d;


	/* -------------------------------------------------------------
	 * --- 5) Add points the easier the target is to be killed.  ---
	 * --- The easier, and cheaper, the better. But even much    ---
	 * --- better if this bot needs money.                       ---
	 * ------------------------------------------------------------- */
	double vict_score = 0.;
	double vict_mod   = needMoney ? static_cast<double>(8 - ai_level) : 1.;
	if (pOpp->opLife < blast_max)
		vict_score += vict_mod * (blast_max - pOpp->opLife) * 1. * ai_type_mod;
	if (pOpp->opLife < blast_big)
		vict_score += vict_mod * (blast_big - pOpp->opLife) * 2. * ai_type_mod;
	if (pOpp->opLife < blast_med)
		vict_score += vict_mod * (blast_med - pOpp->opLife) * 4. * ai_type_mod;
	if (pOpp->opLife < blast_min)
		vict_score += vict_mod * (blast_min - pOpp->opLife) * 8. * ai_type_mod;


	/* --------------------------------------------------------------
	 * --- 6) Add or dock points regarding AI level               ---
	 * --- More powerful opponents are targeted preferably, while ---
	 * --- weaker ones are not considered to be such a threat.    ---
	 * --- Note: Human players are handled like deadly bots.      ---
	 * -------------------------------------------------------------- */
	double level_score = 0.;
	if (!pOpp->onSameTeam) {
        if ( (HUMAN_PLAYER     < opponent->type)
		  && (LAST_PLAYER_TYPE > opponent->type) )
			level_score = static_cast<double>(opponent->type - player->type);
		else
			level_score = static_cast<double>(DEADLY_PLAYER  - player->type);

		// The higher the self preservation, the more urgent deadlier
		// bots are targeted to get them down early.
		if (level_score > 0.)
			level_score *= player->selfPreservation + 1.;

		// The more defensive the bot is, the more does it want to target
		// weaker opponents to not aggravate the stronger ones
		if (level_score < 0.)
			level_score *= player->defensive + 2.;

		// The more health this bots tank has, the more prominent is this score
		level_score = std::abs(level_score) * currLife;
	}


	/* -------------------------------------------------------------
	 * --- 7) Add points for score difference                    ---
	 * --- Target the leading bots earlier, losing ones later.   ---
	 * ------------------------------------------------------------- */
	double win_score = pOpp->onSameTeam ? 0. :
	                   (opponent->score - player->score)
	                   * (player->selfPreservation + 1.)
	                   * (player->defensive + 2.)
	                   * static_cast<double>(ai_level + 1)
	                   * (static_cast<double>(pOpp->opLife) / 10.)
	                   / (player->painSensitivity + 0.5);
	// Note: The win_score is only used if positive.
	// 1 - Self preservation: Get rid of the winner as a threat soon.
	// 2 - Defensiveness : Even more if of the defensive type.
	// 3 - The smarter the more they do care.
	// 4 - Multiply with 10% of the opponents tank life
	// 5 - Pain Sensitivity: Can they stand the answer? ( If this value
	//     is lower than 0.5, they care so less, that the score is raised. Up
	//     to a doubling is possible - If they really feel no pain.)
	// Maximum score:
	// deadly + 1 (lucky turn), maximum defensiveness and self preservation, painless:
	// (4 * 3 * 7) / 0.5 = 84 / 0.5 = 168 points per opponent health point and
	// round win difference.


	/* -------------------------------------------------------------
	 * --- 8) Sum up the score                                   ---
	 * --- This will be used for sorting the opponents list      ---
	 * ------------------------------------------------------------- */
	double damage_score = entry->damage_from - entry->damage_to;
	double kill_score   = (entry->killed_me   - entry->killed_them) * maxLife;
	double last_score   = entry->damage_last * type_mod;

	DEBUG_LOG_EMO(player->getName(), "  team_mod     : %6.2lf", pOpp->team_mod)
	DEBUG_LOG_EMO(player->getName(), "  type_mod     : %6.2lf", type_mod)
	DEBUG_LOG_EMO(player->getName(), "  damage_score : %6.2lf", damage_score)
	DEBUG_LOG_EMO(player->getName(), "  kill_score   : %6.2lf", kill_score)
	DEBUG_LOG_EMO(player->getName(), "  last_score   : %6.2lf", last_score)
	DEBUG_LOG_EMO(player->getName(), "  fear_damage  : %6.2lf", fear_damage)
	DEBUG_LOG_EMO(player->getName(), "  revenge_score: %6.2lf", revenge_score)
	DEBUG_LOG_EMO(player->getName(), "  life_score   : %6.2lf", life_score)
	DEBUG_LOG_EMO(player->getName(), "  dist_score   : %6.2lf", dist_score)
	DEBUG_LOG_EMO(player->getName(), "  vict_score   : %6.2lf", vict_score)
	DEBUG_LOG_EMO(player->getName(), "  level_score  : %6.2lf", level_score)
	DEBUG_LOG_EMO(player->getName(), "  win_score    : %6.2lf", win_score)

	double xScore = ( damage_score  > 0. ? pOpp->team_mod * damage_score  : 0. )
	              + ( kill_score    > 0. ? pOpp->team_mod * kill_score    : 0. )
	              + ( last_score    > 0. ? pOpp->team_mod * last_score    : 0. )
	              + ( fear_damage   > 0. ? pOpp->team_mod * fear_damage   : 0. )
	              + ( fear_shock    > 0. ?    fear_shock * fear_damage   : 0. )
	              + ( revenge_score > 0. ? pOpp->team_mod * revenge_score : 0. )
	              + ( life_score    > 0. ? pOpp->team_mod * life_score    : 0. )
	              + ( vict_score    > 0. ? pOpp->team_mod * vict_score    : 0. )
	              + ( win_score     > 0. ? win_score                     : 0. )
	              + dist_score + level_score;
	pOpp->score = ROUND(xScore);

	DEBUG_LOG_EMO(player->getName(), "  Final Score  : %8d", pOpp->score)

	// --- clean up damage_last ---
	if (entry->damage_last) {
		entry->damage_from += entry->damage_last;
		entry->damage_last  = 0;
	}
}


/// @brief Set a new score to a weapons entry
void AICore::updateWeapScore(weEntry_t* pWeap)
{
	// As this is used a few dozen times, a shortcut to pWeap->type is nice:
	weaponType wType = pWeap ? static_cast<weaponType>(pWeap->type) : SML_MIS;


	// === Get out quickly if the chosen item is out of stock ===
	// ==========================================================
	if (0 == pWeap->amount) {
		pWeap->score = -50000;
		return;
	}


	// === Only evaluate items that are available ===
	// ==============================================
	if (!env.isItemAvailable(wType)) {
		pWeap->score = -100000;
		return;
	}

	DEBUG_LOG_AI(player->getName(), "Evaluating score for %s",
	             weapon[wType].getName())

	// reset boolean helpers
	pWeap->blastOut = false;
	pWeap->kamikaze = false;


	// === If no opponent is chosen (however this may happen) then ===
	// === the pure preferences count.                             ===
	// ===============================================================
	if (nullptr == mem_curr) {
		pWeap->score = pWeap->preference;
		DEBUG_LOG_AI(player->getName(), " -> Use preference %d", pWeap->preference)
		return;
	}


	// === If this is a laser, it will only be evaluated if the tank is ===
	// === not below this players tanks as it can not be reached then.  ===
	// ====================================================================
	if ( (SML_LAZER <= wType) && (LRG_LAZER >= wType)
	  && (mem_curr->opY > y) ) {
		pWeap->score = -45000;
		DEBUG_LOG_AI(player->getName(), " -> Target y %d is not reachable from %d",
		             ROUND(mem_curr->opY), ROUND(y))
		return;
	}


	// === If this is the percent bomb, reducer or theft bomb, its  ===
	// === damage must be adapted, as it depends on the selected    ===
	// === target and/or current capabilities.                      ===
	// ================================================================
	if (PERCENT_BOMB == wType) {
		pWeap->dmgCluster = 0.;
		pWeap->dmgSingle  = mem_curr->opLife / 2;
		pWeap->dmgSpread  = pWeap->dmgSingle;
	}

	// === The same applies to the reducer ===
	if (REDUCER == wType) {
		pWeap->dmgCluster = 0.;
		pWeap->dmgSingle  = (mem_curr->opLife / 2.)
		                  * (mem_curr->entry->opponent->damageMultiplier / 2.)
		                  * (player->painSensitivity + ai_over_mod)
		                  / (-1. * (player->defensive - 1.25 - ai_over_mod));
		pWeap->dmgSpread  = pWeap->dmgSingle;
	}

	// === And the theft bomb ===
	int32_t theft_size = static_cast<int32_t>(player->damageMultiplier * THEFT_AMOUNT);
	if (THEFT_BOMB == wType) {
		double steal_amount = std::min(mem_curr->entry->opponent->money, theft_size);
		pWeap->dmgCluster = 0.;
		pWeap->dmgSingle  = ROUND(steal_amount / ai_level_d);
		pWeap->dmgSpread  = pWeap->dmgSingle;
	}


	/* -------------------------------------------------------------
	 * --- 1) Set score for reaching the target health           ---
	 * ------------------------------------------------------------- */
	double weap_dmg    = 0.; // Filled here, used for splash score, too
	double dmg_diff    = 0.;
	double point_score = mem_curr->opLife;
	// If the bot is shocked, spread and cluster weapons get a bonus:
	double shock_bonus = isShocked
	                   ? static_cast<double>(maxAiLevel - ai_level) + ai_over_mod
	                   : 1.;

	if (pWeap->dmgCluster > 1.) {
		weap_dmg = pWeap->dmgCluster;
		dmg_diff = (weap_dmg / ai_over_mod) - point_score;
		if (dmg_diff > 0.)
			dmg_diff *= shock_bonus;
	} else if (pWeap->dmgSpread > (pWeap->dmgSingle + 0.25) ) {
		weap_dmg = pWeap->dmgSpread;
		dmg_diff = (weap_dmg / (ai_over_mod / 2.)) - point_score;
		if (dmg_diff > 0.)
			dmg_diff *= shock_bonus;
	} else if (pWeap->dmgSingle > 1.) {
		weap_dmg = pWeap->dmgSingle;
		dmg_diff = weap_dmg / ai_over_mod
		         - (THEFT_BOMB == wType
		             ? mem_curr->entry->opponent->money
		             : point_score);
	} else
		dmg_diff = -point_score;

	if (dmg_diff < 0.)
		// Too less damage
		point_score += dmg_diff;
	else if (dmg_diff > 0.)
		// Otherwise chop off a modified difference
		point_score -= dmg_diff
		             / ( -(player->defensive - 2.5) // 3.5 full offensive, 2.5 full defensive
		               * ai_over_mod ); // the higher the level, the more the reduction.

	// If this is a REDUCER, THEFT_BOMB or dirt weapon, and the fake damage is
	// higher than the target health, modify the score. The AI wants
	// to finish off the almost dead and not debuff them
	if ( (REDUCER == wType)
	  || (THEFT_BOMB == wType)
	  || ( (DIRT_BALL <= wType) && (SMALL_DIRT_SPREAD >= wType) ) ) {
		if (mem_curr->opLife <= blast_min)
			point_score = 0;
		else if (mem_curr->opLife <= blast_med)
			point_score /= static_cast<double>(ai_level + 3);
		else if (mem_curr->opLife <= blast_big)
			point_score /= static_cast<double>(ai_level + 1) / ai_over_mod;
		else if (dmg_diff > 0.)
			point_score /= ai_over_mod;
	}

	// If this is the theft bomb, but we do not need money urgently,
	// reduce the score
	if ( (THEFT_BOMB == wType) && !needMoney)
		point_score /= ai_level_d + ai_over_mod;


	/* -------------------------------------------------------------
	 * --- 2) check buried state, shaped charges and the driller ---
	 * ---    might still be usable.                             ---
	 * ------------------------------------------------------------- */
	double unbury_score = 0.;
	if (buried > BURIED_LEVEL) {
		// Shaped charges refer to the y coordinate
		if ( (SHAPED_CHARGE <= wType)
		  && (CUTTER        >= wType)
		  && (std::abs(mem_curr->opY - y) < (weapon[wType].radius / 20))
		  && (mem_curr->distance < weapon[wType].radius) )
			// This one is usable.
			unbury_score = pWeap->dmgSingle * ai_over_mod;

		// The driller is only usable in a vertical way:
		else if ( (DRILLER == wType)
		  && (std::abs(mem_curr->opX - x) < (weapon[wType].radius / 20))
		  && (mem_curr->distance < weapon[wType].radius) )
			unbury_score = pWeap->dmgSingle * ai_over_mod;

		// Riot bombs and charges are the ultimate tools, of course
		else if ( ( (RIOT_CHARGE <= wType) && (RIOT_BLAST    >= wType) )
				||( (RIOT_BOMB   <= wType) && (HVY_RIOT_BOMB >= wType) ) )
			unbury_score = ai_type_mod
			             * static_cast<double>(weapon[wType].radius)
			             * static_cast<double>(buried - BURIED_LEVEL + ai_level);

		// Everything else is (mostly) useless
		else {
			if (pWeap->dmgCluster > 1.)
				unbury_score -= ai_type_mod * pWeap->dmgCluster * pWeap->dmgSingle;
			else {
				unbury_score -= ai_type_mod * (pWeap->dmgSpread + pWeap->dmgSingle);
				// However, if the target is in range and a self hit would not
				// kill our own tank...
				if ( (mem_curr->distance < weapon[wType].radius)
				  && (currLife > pWeap->dmgSingle)
				  && (currLife > pWeap->dmgSpread) )
					unbury_score += ai_over_mod * pWeap->dmgSingle / player->selfPreservation;
			}
		} // End of "useless" weapons

		if (unbury_score > 1.)
			pWeap->blastOut = true;
	} // End of unbury score.

	// If not buried, riot weapons are useless:
	else if ( ( (RIOT_CHARGE <= wType) && (RIOT_BLAST    >= wType) )
	        ||( (RIOT_BOMB   <= wType) && (HVY_RIOT_BOMB >= wType) ) )
		unbury_score = -50000.;


	/* -------------------------------------------------------------
	 * --- 3) Panic score - If this bot has panicked, the more   ---
	 * ---    damage the better.                                 ---
	 * ------------------------------------------------------------- */
	double panic_score = 0.;
	if (isShocked && (mem_curr->entry == shocker) && (buried <= BURIED_LEVEL) ) {
		panic_score = pWeap->dmgCluster > 1.
		            ? pWeap->dmgCluster
		            : pWeap->dmgSpread;
		// If this is a debuffing weapon like reducer or percent bomb,
		// it is valued even higher.
		if (REDUCER == wType)
			panic_score += mem_curr->entry->opponent->damageMultiplier
			             * mem_curr->opLife * player->selfPreservation;
		else if (PERCENT_BOMB == wType)
			panic_score += pWeap->dmgSingle / player->selfPreservation;
		else if ((DIRT_BALL         <= wType)
			  && (SMALL_DIRT_SPREAD >= wType))
			panic_score += weapon[wType].radius
			             * weapon[wType].spread
			             * (1.5 + player->defensive);
		else if (THEFT_BOMB == wType)
			panic_score += pWeap->dmgSingle * player->selfPreservation;
	}


	/* ----------------------------------------------------------------------
	 * --- 4) Score for reaching buried opponents.                        ---
	 * ---    If an opponent is buried, burrowers and tremors are useful. ---
	 * ---------------------------------------------------------------------- */
	double dig_score = 0.;
	if ( mem_curr->is_buried
	  || ( (mem_curr->opX > x) && (mem_curr->buried_l >= BURIED_LEVEL_HALF) )
	  || ( (mem_curr->opX < x) && (mem_curr->buried_r >= BURIED_LEVEL_HALF) ) ) {

		// Chain weapons can push through dirt, but are bad when the own tank
		// is buried.
		if ( (CHAIN_GUN <= wType) && (JACK_HAMMER >= wType) )
			dig_score = pWeap->dmgSingle
			          * static_cast<double>(weapon[wType].getDelayDiv())
			          / (1.75 + player->defensive)
			          * (buried > BURIED_LEVEL ? -1. : 1.);

		// Burrowers can actually directly reach the target
		else if ( (BURROWER <= wType) && (PENETRATOR >= wType) )
			dig_score = pWeap->dmgSingle * ai_type_mod;

		// tremors are somewhat weak, but the do not only (possibly) reach
		// the target but remove dirt as well.
		else if ( (TREMOR <= wType) && (TECTONIC >= wType) )
			dig_score = (pWeap->dmgSingle + weapon[wType].radius)
			          * ai_over_mod * (2.1 + player->defensive);

		// Riot bombs are useful to undig an opponent as well.
		else if ( (RIOT_BOMB <= wType) && (HVY_RIOT_BOMB >= wType) )
			dig_score = weapon[wType].radius
			            // Note: pain sensitivity is used, as not doing any
			            //       damage won't trigger a vengeance reaction.
			          * (1. + player->defensive + player->painSensitivity);

		// remember that this is chosen for blasting out an opponent:
		if (dig_score > 1.)
			pWeap->blastOut = true;
	}


	/* --------------------------------------------------------------
	 * --- 5) Splash damage                                       ---
	 * --- Check all tanks whether they are in "splash range" and ---
	 * --- add or dock points according to the team_mod value of  ---
	 * --- the hit tanks. This score can be negative and is meant ---
	 * --- to help bots to decide against oversized weapons if    ---
	 * --- good working alternatives are present.                 ---
	 * -------------------------------------------------------------- */
	double splash_score = 0.;
	double money_made   = 0.; // build here, used below
	double money_cost   = 0.; // build here, used below
	if (buried <= BURIED_LEVEL) {
		opEntry_t* op    = mem_head;

		// Always assume a full direct hit:
		double xhit = mem_curr->opX;
		double yhit = mem_curr->opY;

		// The minimum in_rate depends on the defensive level. EXPLOSION takes
		// different values for the shaped weapons and tectonics. Further the
		// full rate limit is 10% damage. The bot does not calculate minimum
		// axis rates, and the full rate limit might become lower or higher than
		// this 10%. This is wanted as bots "only estimate".
		double rate_limit = (player->defensive + .75) / 10.;
		// result: Over-offensive Sith: (-1.25 + 0.75) / 10. => 0.5 / 10. =>  5%
		//         Over-defensive Jedi: ( 1.25 + 0.75) / 10. => 2.0 / 10. => 20%

		while (op) {

			// Do not evaluate the target, as it will get the hit anyway
			if (op == mem_curr) {
				op =op->next;
				continue;
			}

			// Yield on each iteration to not hog the CPUs
			if (!global.skippingComputerPlay)
				std::this_thread::yield();

			PLAYER* pl = op->entry->opponent;
			TANK*   lt = pl ? pl->tank : nullptr; // short cut

			if (!lt || lt->destroy || (op->opLife < 1.)) {
				// irrelevant
				op = op->next;
				continue;
			}

			double weap_rad  = weap_curr->radius;
			double xrad      =    DRILLER       == wType
			                 ? weap_rad / 20. : weap_rad;
			double yrad      = ( (SHAPED_CHARGE <= wType)
							  && (CUTTER        >= wType) )
			                 ? weap_rad / 20. : weap_rad;
			double in_rate_x = 0.;
			double in_rate_y = 0.;

			if (lt->isInEllipse(xhit, yhit, xrad, yrad, in_rate_x, in_rate_y)) {
				double in_rate = in_rate_x * in_rate_y;

				if (in_rate < rate_limit)
					in_rate = rate_limit;

				double score = std::min(weap_dmg * in_rate, op->opLife)
				             * op->team_mod;

				// Do not overdo positive scores
				if (score >= 0)
					// Note: That is [1.1;4.8]
					score /= ai_over_mod * static_cast<double>((ai_level + 1) / 2);

				splash_score += score;

				DEBUG_LOG_EMO(player->getName(),
				              "%s in splash range, %s %d points",
				              pl == player ? "I am" : pl->getName(),
				              score > 0 ? "add " : "dock",
				              std::abs(ROUND(score)))

				// Note down money made or cost:
				if (THEFT_BOMB == wType) {
						double theft_done = std::min(in_rate * theft_size,
						       static_cast<double>(op->entry->opponent->money));
					if (op->team_mod > 0.)
						money_made += theft_done;
					else if (lt != tank) // No effect on self!
						money_cost += theft_done / (10. - ai_level_d);
				} else {
					if (op->team_mod > 0.)
						money_made += ( std::min(weap_dmg * in_rate, op->opLife)
						              * static_cast<double>(env.scoreHitUnit) )
						            + ( (weap_dmg * in_rate) >= op->opLife
						                ? static_cast<double>(env.scoreUnitDestroyBonus)
						                : 0.);
					else if (lt == tank)
						money_cost += ( std::min(weap_dmg * in_rate, currLife)
						              * static_cast<double>(env.scoreSelfHit) )
						            + ( (weap_dmg * in_rate) >= currLife
						                ? static_cast<double>(env.scoreUnitSelfDestroy)
						                : 0.);
					else
						money_cost += ( std::min(weap_dmg * in_rate, op->opLife)
						              * static_cast<double>(env.scoreTeamHit) )
						            + ( (weap_dmg * in_rate) >= op->opLife
						                ? static_cast<double>(env.scoreUnitSelfDestroy)
						                : 0.);
				} // End of regular weapon check
			} // End of opponent in explosion

			op = op->next;
		} // End of looping opponents memory
	} // end of calculating splash damage score


	/* -------------------------------------------------------------
	 * --- 6) Kamikaze potential                                 ---
	 * --- If the bot decides to self destruct, it is important  ---
	 * --- to check what this weapon would do.                   ---
	 * ------------------------------------------------------------- */
    double selfde_score = 0.;
    if ( (mem_curr->opLife > (currLife * 10.))
	  || (isShocked && (mem_curr->opLife > (currLife *  5.))) ) {

		// Only some weapons are considered for a big boom bye bye
		if ( ( (SML_NUKE   <= pWeap->type) && (DTH_HEAD   >= pWeap->type) )
		  || ( (WIDE_BOY   == pWeap->type) || (CUTTER     == pWeap->type) )
		  || ( (MED_NAPALM == pWeap->type) || (LRG_NAPALM == pWeap->type) ) ) {
			double kRad = pWeap->radius;
			double kDmg = pWeap->dmgSingle;

			// for a kamikaze, the shaped weapons have to be shot somewhat to
			// the side, so extend the radius if this tank is not buried, or
			// reduce it to zero if it is.
			if ((WIDE_BOY == pWeap->type) || (CUTTER == pWeap->type)) {
				if (buried >= (BURIED_LEVEL / ai_level))
					kRad = 0.;
				else
					kRad += 50. * ai_over_mod;
			}

			// The same counts for the napalm, although it does not really have
			// a radius. This must be estimated according to the current wind.
			else if ((MED_NAPALM == pWeap->type) || (LRG_NAPALM == pWeap->type)) {
				if (buried >= (BURIED_LEVEL / ai_level))
					kRad = 0.;
				else {
					kRad = std::abs(global.wind / (env.windstrength / 4.)) + 1.;
					/* This produces the following multiplier: (with max wind = 8)
					 * wind = 0 : (0 / (8 / 4)) + 1 = (0 / 2) + 1 = = 1
					 * wind = 1 : (1 / (8 / 4)) + 1 = (1 / 2) + 1 = = 1.5
					 * wind = 4 : (4 / (8 / 4)) + 1 = (4 / 2) + 1 = = 3
					 * wind = 6 : (6 / (8 / 4)) + 1 = (6 / 2) + 1 = = 4
					 * wind = 8 : (8 / (8 / 4)) + 1 = (8 / 2) + 1 = = 5
					*/
					kRad *= weapon[pWeap->type].launchSpeed / ai_level;

					// Napalm is a cluster, but not everything will hit
					kDmg = pWeap->dmgCluster / ai_level_d;
				}
			}

			// Check against collateral damage unless shocked and the shocker is
			// in range
			bool tgt_in_range  = false;
			if ( isShocked
			  && (mem_curr->entry == shocker) // can this be false?
			  && (mem_curr->distance < kRad) ) {
				// No check, just do it
				selfde_score = (pWeap->dmgCluster + pWeap->dmgSingle) * ai_over_mod;
				tgt_in_range = true;
			} else {
				// Nope, be reasonable
				selfde_score        = kDmg;
				sOppMemEntry* check = mem_head;
				while (check) {

					// Yield on each iteration to not hog the CPUs
					if (!global.skippingComputerPlay)
						std::this_thread::yield();

					if ( (check->opLife > 0.) && (check->distance < kRad) ) {
						if (check->onSameTeam)
							selfde_score -= kDmg * ai_type_mod
							              * ( 1.25 + player->defensive);
						else
							selfde_score += kDmg * ai_over_mod * check->team_mod;

						// Award extra points if this is the current target
						if (mem_curr == check) {
							selfde_score += check->opLife * check->team_mod;
							tgt_in_range  = true;
						}
					}
					check = check->next;
                }
			} // End of checking done damage

			// Now, if the score is positive, this is a kamikaze choice:
			if ( (selfde_score > 0.) && tgt_in_range)
				pWeap->kamikaze = true;

		} else if (THEFT_BOMB == pWeap->type)
			// In such a situation a (non-aggravating!) theft might
			// be considered useful the more defensive and self preservative
			// a bot is.
			selfde_score = pWeap->dmgSingle * ai_type_mod
			             * (2. + player->defensive + player->selfPreservation);
		else
			// Unsuitable
			selfde_score -= pWeap->dmgSpread * ai_type_mod;
	  }


	/* -------------------------------------------------------------
	 * --- 7) Economic evaluation                                ---
	 * --- If the maximum damage bounty the weapon can generate  ---
	 * --- is lower than the weapon score, points are docked.    ---
	 * --- Generating more money than the weapon is worth adds   ---
	 * --- some bonus points.                                    ---
	 * ------------------------------------------------------------- */
	double eco_score  = 0.;
	double money_mod  = needMoney
	                  ? ai_type_mod
	                  : (static_cast<double>(RAND_AI_1P + 1) * 10.);
	double money_diff = money_made - money_cost;
	if (!isShocked) {
		// Note: Shocked bots do not care about money!
		if ( money_diff >= weapon[pWeap->type].cost)
			eco_score += (money_made / money_mod)
			           - (money_cost / money_mod);
		else
			eco_score -= money_diff / money_mod;
	}


	/* -------------------------------------------------------------
	 * --- 8) Sum up the score                                   ---
	 * --- This will be used for sorting the weapons list        ---
	 * ------------------------------------------------------------- */
	double pref_score = pWeap->preference / ai_level_d
	                  / (std::abs(dmg_diff) > 1. ? std::abs(dmg_diff) : 1.);
	                  // the further away, the less likely.

	DEBUG_LOG_EMO(player->getName(), "  preference   : %6.2lf", pref_score)
	DEBUG_LOG_EMO(player->getName(), "  point_score  : %6.2lf [diff %6.2lf]",
	              point_score, dmg_diff)
	DEBUG_LOG_EMO(player->getName(), "  unbury_score : %6.2lf", unbury_score)
	DEBUG_LOG_EMO(player->getName(), "  panic_score  : %6.2lf", panic_score)
	DEBUG_LOG_EMO(player->getName(), "  splash_score : %6.2lf", splash_score)
	DEBUG_LOG_EMO(player->getName(), "  selfde_score : %6.2lf", selfde_score)
	DEBUG_LOG_EMO(player->getName(), "  dig_score    : %6.2lf", dig_score)
	DEBUG_LOG_EMO(player->getName(), "  eco_score    : %6.2lf (M %6.2lf / C -%6.2lf / D %6.2lf)",
	              eco_score, money_made, money_cost, money_diff)

	double xScore = pref_score + point_score + unbury_score + panic_score
	              + splash_score + selfde_score + dig_score + eco_score;

	pWeap->score = ROUND(xScore);

	DEBUG_LOG_EMO(player->getName(), "  Final Score  : %8d", pWeap->score)
}


/** @brief Select a tool to free the tank or clear a path
  * @param[in] free_tank If set to true, a tool to free the tank is chosen,
  *            a tool to clear the path otherwise.
  * @param[in] is_last If set to true, an emergency selection is done to force
  *            this method to succeed.
  * @return true if the selection succeeded, false otherwise.
**/
bool AICore::useFreeingTool(bool free_tank, bool is_last)
{
	// If the current weapon is already used to blast out
	// an opponent, no other tool is needed.
	if (!free_tank && weap_curr && weap_curr->blastOut)
		return true;

	if ( ( free_tank
		&& ( useWeapon(RIOT_BLAST)
		  || useWeapon(RIOT_CHARGE) ) )
	  || useWeapon(HVY_RIOT_BOMB)
	  || useWeapon(RIOT_BOMB)
	  || (!free_tank
		&& ( useWeapon(CHAIN_GUN)
		  || useWeapon(DRILLER)
		  || useWeapon(CHAIN_MISSILE) ) )
	  || (free_tank
	    && ( useItem(ITEM_TELEPORT) // Note: No mass teleport here!
	      || ( !mem_curr->is_buried
	        && useItem(ITEM_SWAPPER) ) ) ) ) {

		DEBUG_LOG_AIM(player->getName(), "Selected %s to %s",
		              weap_idx < WEAPONS
		              ? weapon[weap_idx].getName()
		              : item[weap_idx - WEAPONS].getName(),
		              free_tank ? "free my tank" : "clear firing path")

		return true;
	}

	// If the "normal" selection is not possible (out of stock)
	// but this is the last try, the bot has to revert to standard
	// missiles. Expensive, but should work.
	else if ( is_last
	  && ( ( !free_tank
		  && ( useWeapon(SML_NUKE)
			|| useWeapon(LRG_MIS)
			|| useWeapon(MED_MIS) ) )
	    || useItem(ITEM_TELEPORT)
	    || ( useItem(ITEM_SWAPPER)
	      && !mem_curr->is_buried )
	    || useItem(ITEM_MASS_TELEPORT) // As a last resort this is okay.
		|| useWeapon(SML_MIS) ) ) {

		DEBUG_LOG_AIM(player->getName(), "(LAST) Selected %s to %s",
		              weap_idx < WEAPONS
		              ? weapon[weap_idx].getName()
		              : item[weap_idx - WEAPONS].getName(),
		              free_tank ? "free my tank" : "clear firing path")

		return true;
	}

	return is_last;
}


/// @brief explicitly select @a item_type, returns true if available and chosen.
bool AICore::useItem (itemType item_type)
{
	if (env.isItemAvailable(item_type) && (player->ni[item_type] > 0)) {
		item_curr = item_head;
		while (item_curr && (item_curr->type != item_type))
			item_curr = item_curr->next;

		if (item_curr && (item_curr->type == item_type)) {
			weap_idx  = WEAPONS + item_type;
			weap_curr = nullptr;
			return true;
		} else if (weap_curr)
			item_curr = nullptr;
	}

	return false;
}


/// @brief convenience function to use the full index as an integer to choose
/// an item. Full index means the value is beyond the WEAPONS constant.
bool AICore::useItem(int32_t item_index)
{
	if ( (item_index >= WEAPONS) && (item_index < THINGS) )
		return useItem(static_cast<itemType>(item_index - WEAPONS));
	return false;
}


/// @brief explicitly select @a weapon_type, returns true if available and chosen.
bool AICore::useWeapon (weaponType weap_type)
{
	if (env.isItemAvailable(weap_type) && (player->nm[weap_type] > 0)) {
		weap_curr = weap_head;
		while (weap_curr && (weap_curr->type != weap_type))
			weap_curr = weap_curr->next;

		if (weap_curr && (weap_curr->type == weap_type)) {
			weap_idx = weap_type;
			item_curr = nullptr;
			return true;
		} else if (item_curr)
			weap_curr = nullptr;
	}

	return false;
}


/// @brief convenience function to use the numeric index as an integer to choose
/// a weapon.
bool AICore::useWeapon(int32_t weap_index)
{
	if (weap_index < WEAPONS)
		return useWeapon(static_cast<weaponType>(weap_index));
	return false;
}


/// @brief Call this once the AI weapon is fired to signal the end of the
/// players turn
void AICore::weapon_fired()
{
	if (isWorking && !isStopped && (PS_FIRE == plStage)) {
		DEBUG_LOG_AI(player->getName(), "------------------------------", 0)
		DEBUG_LOG_AI(player->getName(), " Weapon fired for %s", player->getName())

		lguard_t guard(actionMutex);
		plStage = PS_CLEANUP;
		actionCondition.notify_one();
	}
}


/// @brief Core threading operator
void AICore::operator()()
{
	while (canWork && !isStopped) {

		// Go to sleep until the thread is woken up
		luniq_t actionLock(actionMutex);
		actionCondition.wait(actionLock, [this]{
			return (isWorking || isStopped);
		} );

		// If the thread is to be stopped, exit the loop
		if (isStopped)
			// Cleaner than "break", but only on a philosophical level... ;-)
			continue;

		if (!initialize()) {
			plStage   = PS_AI_IS_IDLE;
			isWorking = false;
			continue;
		}

		// --------------------------------------------------------------------
		// --- First update the foe list, only then a target can be picked- ---
		// --------------------------------------------------------------------
		checkOppMem();


		// -----------------------------------------------------------------
		// --- See whether the bot falls for a fear shock.               ---
		// --- If they are mortally afraid of a shocker, no other target ---
		// --- will be picked. It is fixed on that one then.             ---
		// -----------------------------------------------------------------
		if (shocker) {
			DEBUG_LOG_EMO(player->getName(),
			              "Terrified by %s (fear shock: %lf)",
			              shocker->opponent->getName(), shocker->fear_shock)
			double reshock = shocker->fear
			               - (static_cast<double>(RAND_AI_0P + 2) / 2.);
			if (reshock >= shocker->fear_shock) {
				isShocked = true;

				// Generate a nice message telling the world that we are in awe:
				if (!isStopped && !global.skippingComputerPlay) {
					const char* text = player->selectPanicPhrase(shocker->opponent);
					try {
						if (text) {
							// Wait for the AI to be allowed to create texts
							while (!textAllowed.load(ATOMIC_READ))
								std::this_thread::yield();

							// Now create the instance
							new FLOATTEXT(text,x, y - 30, .0, -.4, player->color,
							              CENTRE, TS_NO_SWAY, 150, false);
						}
					} catch (...) {
						perror ( "aicore.cpp: Failed to allocate memory for"
								 " panic text in operator().");
					}
					if (text)
						free(const_cast<char*>(text));
				}


				DEBUG_LOG_EMO(player->getName(),
				              "Shock confirmed with %lf over %lf",
				              reshock, shocker->fear_shock)
			} else {
				isShocked = false;
				DEBUG_LOG_EMO(player->getName(),
				              "Overcame shock with %lf under %lf",
				              reshock, shocker->fear_shock)
			}
		}


		// ---------------------------------------------------
		// --- Set basic behaviour values                  ---
		// --- Done here and not in initialize so the full ---
		// --- shock check is already done.                ---
		// ---------------------------------------------------
		findOppAttempts = ai_level + 2 - (isShocked ? ai_level / 2 : 0);
		findRngAttempts = (std::pow(ai_level + 1, 2) + 1)
		                / (isShocked ? ai_level + 1 : 1)
		                + (isShocked ? 0 : ai_level + 4);
		findTgtAttempts = ai_level
		                - (isShocked ? ai_level - 1 : 0)
		                + (isShocked ? 0 : 1);
		findWeapAttempts= ai_level * 2 / (isShocked ? ai_level : 1);
		focusRate       = ai_level_d * 2.
		                / static_cast<double>(maxAiLevel * 2);
		errorMultiplier = static_cast<double>(maxAiLevel + 1 - ai_level)
		                / static_cast<double>(findRngAttempts);
		maxBounce       = ROUNDu(std::pow(ai_level, 2) / 2.) + 2;
		/* The results should be [if shocked]:
		 * findOppAttempts : Useless   3   [2], Deadly + 1:  8    [4]
		 * findRngAttempts : Useless: 10   [2], Deadly + 1: 60    [7]
		 * findTgtAttempts : Useless:  2   [1], Deadly + 1:  7    [1]
		 * findWeapAttempts: Useless:  2   [1], Deadly + 1: 12    [2]
		 * focusRate       : Useless:  0.166,   Deadly + 1:  1.0
		 * errorMultiplier : Useless:  1.2 [3], Deadly + 1:  0.02 [0.14]
		 * maxBounce       : Useless:  3,       Deadly + 1: 20
		 */

		DEBUG_LOG_AI(player->getName(), "AI Level       : %d (%s)", ai_level,
					 getLevelName(ai_level))
		DEBUG_LOG_AI(player->getName(), "type_mod       : %4.3lf", type_mod)
		DEBUG_LOG_AI(player->getName(), "errorMultiplier: %4.3lf",
					 errorMultiplier)
		DEBUG_LOG_AI(player->getName(), "findOppAttempts: %d", findOppAttempts)
		DEBUG_LOG_AI(player->getName(), "findRngAttempts: %d", findRngAttempts)
		DEBUG_LOG_AI(player->getName(), "findTgtAttempts: %d", findTgtAttempts)
		DEBUG_LOG_AI(player->getName(), "focusRate      : %4.3lf", focusRate)
		DEBUG_LOG_AI(player->getName(), "maxBounce      : %d", maxBounce)
		DEBUG_LOG_AI(player->getName(), "needMoney      : %s",
					 needMoney ? "Yes" : "No")


		// ------------------------------------------------------------------
		// --- The full cycle of target selection, weapon/item selection, ---
		// --- setting up the basic combat values and targeting the       ---
		// --- selected weapon might need a few attempts. The higher the  ---
		// --- AI level, the more attempts the bot gets. If the maximum   ---
		// --- number of attempts is reached, all used methods are forced ---
		// --- to come up with a minimum result.                          ---
		// ------------------------------------------------------------------
		int32_t tgt_attempts  = 0;
		int32_t opp_attempts  = 0;
		int32_t weap_attempts = 0;
		bool    done          = false;

		while (canWork && isWorking && !isStopped
		    && (needAim || !isBlocked) // End if a free is needed
			&& (tgt_attempts < findTgtAttempts) ) {

			// Yield on each iteration to not hog the CPUs
			if (!global.skippingComputerPlay)
				std::this_thread::yield();

			// ----------------------------------------------------------
			// --- 1) Cycle target and item selection.                ---
			// --- Those are combined, because selecting a different  ---
			// --- target later might make the current item selection ---
			// --- less effective or even useless. Thus the item is   ---
			// --- chosen individually.                               ---
			// ----------------------------------------------------------
			if (!opp_attempts && !weap_attempts) {
				++tgt_attempts;
				mem_curr  = nullptr;
				DEBUG_LOG_AIM(player->getName(), "Starting setup %d / %d",
				              tgt_attempts, findTgtAttempts)
			}
			done = setupAttack(tgt_attempts == findTgtAttempts,
			                   opp_attempts, weap_attempts);

			// ----------------------------------------------------------
			// --- 2) Calculate basic attack values.                  ---
			// --- If the target and item selection is different than ---
			// --- in the last round, new basic values must be        ---
			// --- calculated. If the selections are what this player ---
			// --- had in the last round, this won't be needed. Just  ---
			// --- continue were we left off last round.              ---
			// ----------------------------------------------------------
			if (done)
				done = calcAttack(tgt_attempts);

			// ----------------------------------------------------------
			// --- 3) Aim the current selection                       ---
			// ----------------------------------------------------------
			if (done && needAim && !isBlocked)
				done = aim( (tgt_attempts == findTgtAttempts) && needSuccess );
			else  if (!needAim || isBlocked) {
				DEBUG_LOG_AIM(player->getName(), "No aiming done: %s, %s",
				              needAim   ? "Aiming needed"   : "Aiming NOT needed",
				              isBlocked ? "shot is blocked" : "Shot is NOT blocked")
			}


			// ------------------------------------------------------
			// --- 4) If this round was successful, check whether ---
			// ---    A new best setup is found                   ---
			// ------------------------------------------------------
			if (done) {
				// Reset opponent and weapon attempts if a positive score
				// was achieved and the AI has tried enough items or the
				// opponent selection is finished.
				if (best_round_score > 0) {
					if ( (weap_attempts > ai_level)
					  || (opp_attempts == findOppAttempts) ) {
						opp_attempts  = 0;
						weap_attempts = 0;
					}

					// Tweak the score if the primary target was hit:
					if (best_prime_hit) {

						// add the weapon and opponent score, so attacks, even
						// if they are not perfect, get emphasized if the preferred
						// setup is chosen:
						if (  mem_curr && revengee
						  && (player != mem_curr->entry->opponent) )
							best_round_score += mem_curr->score
											  / (revengee == mem_curr->entry
												 ? ai_level : ai_level * 10);
						if (weap_curr && (weap_curr->dmgSingle > 0) )
							best_round_score += weap_curr->score / (ai_level * 10);
					}
				} // End of having a best_round_score greater than zero

				// Note down best setup score and settings if better or
				// forced to succeed due to last attempt condition
				bool new_best_setup_score = (best_round_score > best_setup_score);
				if ( ( ( new_best_setup_score && best_prime_hit)
				    || ( !best_setup_prime
				      && (new_best_setup_score || best_prime_hit) ) )
				  || ( (NEUTRAL_ROUND_SCORE == best_setup_score)
					&& (tgt_attempts == findTgtAttempts)
				    && needSuccess) ) {
					best_setup_angle     = curr_angle;
					best_setup_damage    = mem_curr->dmgDone;
					best_setup_item      = item_curr;
					best_setup_mem       = mem_curr;
					best_setup_overshoot = best_overshoot;
					best_setup_power     = curr_power;
					best_setup_prime     = best_prime_hit;
					best_setup_weap      = weap_curr;
					DEBUG_LOG_AIM(player->getName(),
								"New best setup with angle %d, power %d using %s : (%d > %d)",
								GET_DISP_ANGLE(curr_angle), curr_power,
								weap_idx < WEAPONS
								? weapon[weap_idx].getName()
								: item[weap_idx - WEAPONS].getName(),
								best_round_score, best_setup_score)
					best_setup_score = best_round_score;

					// This targeting round is definitely over
					opp_attempts  = 0;
					weap_attempts = 0;

					if ( needSuccess
					  && best_setup_prime
					  && (best_setup_score > 0) )
						// There is no need to force anything any more:
						needSuccess = false;

					// Give feedback according to what has happened
					if ( (best_round_score > 0) && best_prime_hit )
						showFeedback("!!!", GREEN, -.5, TS_NO_SWAY, 150);
					else
						showFeedback("!", GREEN, -.6, TS_HORIZONTAL, 120);
				} else if (needSuccess)
					showFeedback("?", RED, -.7, TS_HORIZONTAL, 90);
			} // End of setup score handling
		} // End of full preparation cycle


		// ---------------------------------------------------------
		// --- If the revengee has been changed due to the score ---
		// --- considerations, write back the new victim:        ---
		// ---------------------------------------------------------
		if (revengee && (revengee->opponent != player->revenge)) {
			if (revengee->opponent != player)
				player->revenge = revengee->opponent;
			else
				revengee = nullptr;
		} else if (!revengee)
			player->revenge = nullptr;


		// --------------------------------------------
		// --- If no real setup could be found, see ---
		// --- whether a freeing attempt is needed. ---
		// --------------------------------------------
		if (!isStopped && !isShocked
		  && !isBlocked  // If these fail, aim() already has set up
		  && needAim     // a freeing attempt. Do not do it twice!
		  && best_setup_weap
		  && ( (best_setup_score < 0)
		    || ( (0 == best_setup_score ) && RAND_AI_0P) ) ) {
			DEBUG_LOG_AIM(player->getName(), "Best setup score %d too low!",
			              best_setup_score)

			// First, copy best noted data (if any)
			if (NEUTRAL_ROUND_SCORE != best_setup_score) {
				curr_angle = best_setup_angle;
				curr_power = best_setup_power;
			}

			// Now see whether to unbury or clear the path:
			if ( ( buried_l >= (BURIED_LEVEL_HALF / 2) )
			  || ( buried_r >= (BURIED_LEVEL_HALF / 2) ) ) {
				useFreeingTool(true, true);
				calcUnbury(true);
			} else {
				curr_angle = best_setup_angle;
				curr_power = best_setup_power;
				if (RAND_AI_0P
				  || hill_detected
				  || !best_setup_weap
				  || (best_setup_weap->spread > 1)
				  || (best_setup_weap->subMunCount > 0)
				  || (REDUCER == best_setup_weap->type)
				  || (PERCENT_BOMB == best_setup_weap->type) ) {
					useFreeingTool(false, true);

					// If this is a riot bomb, flatten the angle,
					// but only if the best overshoot (we took the
					// angle and power from its setup) was too long.
					// No use in firing a riot bomb behind the opponent
					if (best_setup_overshoot > 0)
						flattenCurrAng();
				} else {
					// In this case use the current weapon, but go a bit down
					// with the angle:
					int32_t ang_mod = 5 + RAND_AI_1P;

					if (curr_angle < 180) {
						curr_angle -= ang_mod;
						if (curr_angle < 95)
							curr_angle = 95;
					} else {
						curr_angle += ang_mod;
						if (curr_angle > 265)
							curr_angle = 265;
					}

					if ( (REDUCER      != best_setup_weap->type)
					  && (PERCENT_BOMB != best_setup_weap->type)
					  && ( (RIOT_BOMB  >  best_setup_weap->type)
					    || (RIOT_BLAST <  best_setup_weap->type) ) )
						useWeapon(best_setup_weap->type);
					else
						useWeapon(SML_MIS);
				} // end of using best setup weapon.
			}

			sanitizeCurr();
			angle           = curr_angle;
			power           = curr_power;
			needAim         = false;
			best_setup_weap = nullptr;
			isBlocked       = true;

			showFeedback("???", PURPLE, -.6, TS_NO_SWAY, 100);
		}


		// ----------------------------------------
		// --- Write back the best attack setup ---
		// ----------------------------------------
		if (!isStopped && needAim && !isBlocked ) {
			curr_angle = best_setup_angle;
			curr_power = best_setup_power;
			item_curr  = best_setup_weap ? nullptr : best_setup_item;
			mem_curr   = best_setup_mem;
			weap_curr  = item_curr ? nullptr : best_setup_weap;
			weap_idx   = weap_curr ? weap_curr->type
			           : item_curr ? item_curr->type + WEAPONS
			           : 0;

			sanitizeCurr();
			angle      = curr_angle;
			power      = curr_power;

			DEBUG_LOG_AIM(player->getName(),
						"Using best setup with angle %d, power %d using %s (Score %d)",
						GET_DISP_ANGLE(angle), power,
						weap_idx < WEAPONS
						? weapon[weap_idx].getName()
						: item[weap_idx - WEAPONS].getName(),
						best_setup_score)
		} else if (!isStopped) {
			// Note: Without aiming or when blocked, the setup memory
			//       was not used in some cases.
			curr_angle = angle;
			curr_power = power;
			sanitizeCurr();
			angle = curr_angle;
			power = curr_power;
		}


		// ---------------------------------------------------------
		// --- For the bot to yell out a retaliation phrase, the ---
		// --- following conditions must be true:                ---
		// --- 1) A weapon is chosen                             ---
		// --- 2) The primary target must be hit                 ---
		// --- 3 a) The target is the revengee and               ---
		// --- 3 b) the damage is at least 10% per AI level or   ---
		// --- 4 a) the target is not the revengee and           ---
		// --- 4 b) the damage is at least 20% per AI level      ---
		// ---------------------------------------------------------
		int32_t min_rev_dmg = best_setup_mem
		                    ? best_setup_mem->opLife * (ai_level - RAND_AI_0P) / 10
		                    : 0;
		int32_t min_oth_dmg = best_setup_mem
		                    ? best_setup_mem->opLife * (ai_level - RAND_AI_0P) /  5
		                    : 0;
		if ( !isStopped && !global.skippingComputerPlay             // allowed to issue texts
		  && weap_curr && needAim && !needSuccess                   // (1) targeting was successful
		  && best_setup_prime                                       // (2) primary target gets damage
		  && ( ( revengee && (revengee == best_setup_mem->entry)    // (3 a) revengee targeted
		      && (best_setup_damage >= min_rev_dmg) )               // (3 b) enough damage done
		    || ( (!revengee || (revengee != best_setup_mem->entry)) // (4 a) not the revengee
		      && (best_setup_damage >= min_oth_dmg) ) ) ) {         // (4 b) enough damage done
			const char* text = player->selectRetaliationPhrase();
			try {
				if (text) {
					// Wait for the AI to be allowed to create texts
					while (!textAllowed.load(ATOMIC_READ))
						std::this_thread::yield();

					// Now create the instance
					new FLOATTEXT(text,x, y - 30, .0, -.4, player->color,
					              CENTRE, TS_NO_SWAY, 150, false);
				}
			} catch (...) {
				perror ( "aicore.cpp: Failed to allocate memory for"
						 " retaliation text in operator().");
			}
			if (text)
				free(const_cast<char*>(text));
		}


		// -------------------------------------------------
		// --- Tell the world this tank is going bye bye ---
		// -------------------------------------------------
		if ( !isStopped
		  && (mem_curr->entry->opponent == player)
		  && !global.skippingComputerPlay) {
			try {
				// Wait for the AI to be allowed to create texts
				while (!textAllowed.load(ATOMIC_READ))
					std::this_thread::yield();

				// Now create it
				new FLOATTEXT (player->selectKamikazePhrase(),
				               x, y - 30, .0, -.4,
				               player->color, CENTRE, TS_NO_SWAY, 300, false);
			} catch (...) {
				perror ( "aicore.cpp: Failed allocating memory for"
				         " kamikazeText in operator().");
			}
		}


		// ---------------------------------------
		// --- Apply some "last second" errors ---
		// ---------------------------------------
		if (!isStopped && needAim && !isBlocked
		     // Assume that bots can 'fix' errors from the last round:
		  && ( (nullptr == mem_curr) || (mem_curr->entry != last_opp) )
		  && RAND_AI_1N) {
			int32_t ang_mod = maxAiLevel - ai_level + 2; // [ 2; 7]
			int32_t pow_mod = (ang_mod * 5) + 1;         // [11;36]
			double ang_err = rand() % ang_mod;           // [ 1; 6]
			double pow_err = rand() % pow_mod;           // [10;35]

			// Angles always go 'up', but never over the top
			if (angle > (180. + ang_err))
				curr_angle = angle - ang_err;
			else if (angle < (180. - ang_err))
				curr_angle = angle + ang_err;

			// Power error is always a raise
			curr_power = power + pow_err;

			sanitizeCurr();

			DEBUG_LOG_AIM(player->getName(),
					"Last second errors: Angle %d° -> %d°), Power: %d -> %d)",
					GET_DISP_ANGLE(angle), GET_DISP_ANGLE(curr_angle),
					power, curr_power)

			showFeedback("*fumble*", RED, -.8, TS_NO_SWAY, 100);

			angle = curr_angle;
			power = curr_power;

		}
		assert( (angle == curr_angle) && "ERROR: Finished but angle not set!");
		assert( (power == (curr_power - (curr_power % 5)))
				&& "ERROR: Finished but power not set!");
		assert( ( (weap_idx >= WEAPONS) || (0 == weapon[weap_idx].warhead) )
		     && "ERROR: Not usable warhead chosen!");
		assert( (weap_idx >= 0) && (weap_idx < THINGS)
		     && env.isItemAvailable(weap_idx)
		     && "ERROR: Unavailable or invalid weap_idx!");
		assert( ( (weap_idx >= WEAPONS) || (player->nm[weap_idx] > 0) )
		     && "ERROR: Weapon chosen that is out of stock!");
		assert( ( (weap_idx < WEAPONS) || (player->ni[weap_idx - WEAPONS] > 0) )
		     && "ERROR: Item chosen that is out of stock!");
		assert( ( (weap_idx < WEAPONS)
		       || ((weap_idx - WEAPONS) < ITEM_LGT_SHIELD)
		       || ((weap_idx - WEAPONS) == ITEM_FUEL)
		       || ((weap_idx - WEAPONS) == ITEM_ROCKET) )
		     && "ERROR: The chosen item is not usable!");


		// ---------------------------------------
		// --- Wait for the weapon to be fired ---
		// ---------------------------------------
		if (!isStopped) {
			plStage = PS_FIRE; // It can be fired now

			DEBUG_LOG_AI(player->getName(),
			             "Finished thinking, waiting to fire %s against %s",
			             weap_curr
			             ? weapon[weap_idx].getName()
			             : item[weap_idx - WEAPONS].getName(),
			             mem_curr
			             ? mem_curr->entry->opponent->getName()
			             : "Nobody")

			actionCondition.wait(actionLock, [this]{
				return (!canWork || !isWorking || isStopped
						|| (PS_CLEANUP == plStage) );
			} );
		}


		// --------------------------------------
		// --- Remember the current selection ---
		// --- (But only if it was hit)       ---
		// --------------------------------------
		if (!isStopped && best_setup_prime && (best_setup_mem == mem_curr) )
			player->setLastOpponent(mem_curr ? mem_curr->entry : nullptr);
		else
			player->setLastOpponent(nullptr);

		DEBUG_LOG_AI(player->getName(), "Cleaning up...", 0)

		// --------------------
		// ---   Clean up   ---
		// --------------------
		angle          = 180;
		power          = MAX_POWER / 2;
		curr_angle     = 180;
		curr_power     = MAX_POWER / 2;
		curr_overshoot = MAX_OVERSHOOT;
		weap_idx       = SML_MIS;
		blast_big      = 0.;
		blast_max      = 0.;
		blast_med      = 0.;
		blast_min      = 0.;
		textAllowed.store(false, ATOMIC_WRITE);

		// Note: There is no need to clean up the memory chain.
		// It is only created once, all players have the same
		// size, and the getMemory() method reuses an existing
		// one.
		player    = nullptr;
		tank      = nullptr;

		// Eventually signal that the work has finished.
		plStage   = PS_AI_IS_IDLE;
		isWorking = false;
	} // End of not being stopped

	isFinished = true;
}


/// =========================================
/// === Helper list entry implementations ===
/// =========================================


/// @brief explicit constructor adding the instance to the list
sItemListEntry::sItemListEntry(sItemListEntry* prev_) :
	prev(prev_)
{
	if (prev) {
		next = prev->next;
		prev->next = this;

		if (next)
			next->prev = this;
	}
}


/// @brief The destructor removes the element from the list
sItemListEntry::~sItemListEntry()
{
	if (prev) {
		prev->next = next;
		prev       = nullptr;
	}
	if (next) {
		next->prev = prev;
		next       = nullptr;
	}
}


/// @brief explicit constructor adding the instance to the list
sOppMemEntry::sOppMemEntry(sOppMemEntry* prev_) :
	prev(prev_)
{
	if (prev) {
		next = prev->next;
		prev->next = this;

		if (next)
			next->prev = this;
	}
}


/// @brief The destructor removes the element from the list
sOppMemEntry::~sOppMemEntry()
{
	if (prev) {
		prev->next = next;
		prev       = nullptr;
	}
	if (next) {
		next->prev = prev;
		next       = nullptr;
	}
	entry = nullptr;
}


/// @brief explicit constructor adding the instance to the list
sWeapListEntry::sWeapListEntry(sWeapListEntry* prev_) :
	prev(prev_)
{
	if (prev) {
		next = prev->next;
		prev->next = this;

		if (next)
			next->prev = this;
	}
}


/// @brief The destructor removes the element from the list
sWeapListEntry::~sWeapListEntry()
{
	if (prev) {
		prev->next = next;
		prev       = nullptr;
	}
	if (next) {
		next->prev = prev;
		next       = nullptr;
	}
}


