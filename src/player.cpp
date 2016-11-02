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


#include "environment.h"
#include "globaldata.h"
#include "player.h"
#include "tank.h"
#include "menu.h"
#include "files.h"
#include "floattext.h"
#include "network.h"
#include "missile.h"
#include "aicore.h"

#include <cassert>

/// Static helper values to help with keyboard controls:
static bool ctrlUsedUp        = false;
static bool has_ctrl_pressed  = false;
static bool has_shift_pressed = false;

/// @brief default ctor
PLAYER::PLAYER() :
	sdi_has_fired(ATOMIC_VAR_INIT(false))
{
	// Do a memset initialization thanks to VC++
	memset(ni,           0, sizeof(int32_t) * ITEMS);
	memset(nm,           0, sizeof(int32_t) * WEAPONS);
	memset(currPref,     0, sizeof(int32_t) * THINGS);
	memset(desired,      0, sizeof(int32_t) * THINGS);
	memset(saveMoneyFor, 0, sizeof(int32_t) * THINGS);
	memset(name,         0, sizeof(char)    * NAME_LEN);
	memset(weapPref,     0, sizeof(int32_t) * THINGS);

	strncpy(name, "New Player", NAME_LEN);

	// 25% of time set to perplay weapon preferences
	preftype = (rand () % 4) ? ALWAYS_PREF : PERPLAY_PREF;

	/* Generate a set of preferences now. The reason is:
	 * If the player is a PERPLAY_PREF type player, no preferences
	 * are loaded from the atanks configuration file. But if the user
	 * changes the type to ALWAYS_PREF and starts a new game, no new
	 * preferences are generated. So then these are used, which is
	 * safe enough.
	 */
	generatePreferences();

	switch (rand() % 4) {
		case 0: // === red type ===
			color = makecol(200 + (rand() % 56), rand() % 25, rand() % 25);
		break;
		case 1: // === green type ===
			color = makecol( rand() % 25, 200 + (rand() % 56), rand() % 25);
		break;
		case 2: // === blue type ===
			color = makecol( rand() % 25, rand() % 25, 200 + (rand() % 56) );
		break;
		case 3:
		default: // === violet type ===
			color = makecol( 200 + (rand() % 56), rand() % 25, 200 + (rand() % 56));
		break;
	}

}


/// @brief default dtor
PLAYER::~PLAYER ()
{
	if (tank) {
		delete(tank);
		tank = nullptr;
	}

	if (opponents) {
		delete [] opponents;
		opponents = nullptr;
	}
}


/// @brief update currPrefs array with considering needs and stock amounts
void PLAYER::boostPrefences(bool boostArmour, bool boostAmps, bool boostWeapons)
{
	int32_t ai_level = static_cast<int32_t>(type);

	for (int32_t i = 1; i < THINGS; ++i) {
		double pref = currPref[i];

		// boost preferences if wanted:
		if (boostArmour
		  && ( (WEAPONS + ITEM_ARMOUR  ) <= i)
		  && ( (WEAPONS + ITEM_PLASTEEL) >= i) )
			pref *= 1. + ((1. + static_cast<double>(RAND_AI_0P)) / 10.);

		if (boostAmps
		  && ( (WEAPONS + ITEM_INTENSITY_AMP) <= i)
		  && ( (WEAPONS + ITEM_VIOLENT_FORCE) >= i) )
			pref *= 1. + ((1. + static_cast<double>(RAND_AI_0P)) / 10.);

		if (boostWeapons && i && (i < WEAPONS))
			pref *= 1. + ((1. + static_cast<double>(RAND_AI_1P)) / 10.);

		// Lower weapon preferences if there are enough in stock already
		if (i && (i < WEAPONS)) {
			double cur_amount = nm[i] / weapon[i].getDelayDiv();
			double one_amount = weapon[i].amt / weapon[i].getDelayDiv();
			double max_amount = one_amount * ai_level;
			double div_amount = cur_amount - max_amount;

			// - cur_amount is the total amount of single shots. getDelayDiv()
			// is used, because it simply returns the number of shots fired by
			// delayed weapons, while it returns always 1 for the other weapons.
			// - one_amount - The number of nm[i] that is gotten by buying one
			//                unit.
			// - max_amount - below this no reduction or sale is considered.
			// - div_amount - if positive, the bot has enough in stock.
			//              - if larger than one_amount, selling the excess
			//                amount is considered.

			if (div_amount >= 1.) {
				pref /= div_amount;
				DEBUG_LOG_FIN(name,
							  "Lower %s pref (%d in stock) %d -> %d",
							  weapon[i].getName(),
							  ROUND(cur_amount), currPref[i], ROUND(pref))

				if (env.sellpercent > 0.01) {

					// saleable are the units considered to be sold:
					int32_t saleable = (div_amount - RAND_AI_1P) / one_amount;

					if (saleable > 0) {
						money += ROUNDu(weapon[i].cost * env.sellpercent)
							   * saleable;
						nm[i] -= weapon[i].amt * saleable;
						DEBUG_LOG_FIN(name,
									  "Sold %d %s for $%s",
									  saleable,
									  weapon[i].getName(),
									  Add_Comma(ROUNDu( weapon[i].cost
													  * env.sellpercent)
													  * saleable))
					}
				} // end of selling allowed
			} // end of having enough in stock
		} // end of watching weapons

		// Lower item preferences if there are enough in stock already
		if (i >= WEAPONS) {
			int32_t j = i - WEAPONS;
			if ( (j < ITEM_ARMOUR) || (j > ITEM_VIOLENT_FORCE) ) {
				double cur_amount = ni[j];
				double one_amount = item[j].amt;
				double max_amount = one_amount * ai_level;

				// Note: The values are the same as above.

				// Repair kit and SDI are limited differently
				if (ITEM_REPAIRKIT == j)
					max_amount *= painSensitivity + defensive + 2.;
				else if (ITEM_SDI == j)
					max_amount *= defensive + 2. + (ai_level / 2.);

				double div_amount = cur_amount - max_amount;

				if (div_amount >= 1.) {
					pref /= div_amount;
					DEBUG_LOG_FIN(name,
								  "Lower %s pref (%d in stock) %d -> %d",
								  item[j].getName(),
								  ROUND(cur_amount), currPref[i],
								  ROUND(pref))

					if ( (env.sellpercent > 0.01) && (j < ITEM_VENGEANCE) ) {

						// Note: Armour and Amps are not considered here!
						int32_t saleable = (div_amount - RAND_AI_1P) / one_amount;

						if (saleable > 0) {
							money += ROUNDu(item[j].cost * env.sellpercent)
								   * saleable;
							ni[j] -= item[j].amt * saleable;
							DEBUG_LOG_FIN(name,
										  "Sold %d %s for $%s",
										  saleable,
										  item[j].getName(),
										  Add_Comma(ROUNDu( item[j].cost
														  * env.sellpercent)
														  * saleable))
						}
					} // end of selling allowed
				} // End of having enough in stock
			} // End of item type limitation
		} // End of being in items range

		// Write back preferences:
		currPref[i] = ROUND(pref);
	}
}


/** @brief Buy item with index @a itemindex
  * An item has been selected, this function merely buys it. It
  * first does checks to make sure the item can be bought.
  * The function returns true if we successfully bought the item or
  * false if we could not get it for some reason.
**/
bool PLAYER::buy_item(int32_t itemindex, int32_t max_boost)
{
	bool bought  = true;

	if (itemindex < WEAPONS) {
		// The three things to test:
		// 1: Enough money?
		// 2: Space free in stock?
		// 3: Tech level not too high?
		if ( (money >= weapon[itemindex].cost)
		  && (nm[itemindex] < MAX_ITEMS_IN_STOCK)
		  && (weapon[itemindex].techLevel <= env.weapontechLevel) ) {
			money         -= weapon[itemindex].cost;
			nm[itemindex] += weapon[itemindex].amt;

			// don't allow more than MAX_ITEMS_IN_STOCK
			if (nm[itemindex] > MAX_ITEMS_IN_STOCK)
				nm[itemindex] = MAX_ITEMS_IN_STOCK;
		} else
			bought = false;
	} // end of buying a weapon

	else {

		// Items need an additional check:
		// The purchase of boost items is limited by
		// both AI type and overall boost level.
		// The same applies to shields

		int32_t ai_level = static_cast<int32_t>(type);
		int32_t itemNum  = itemindex - WEAPONS;
		bool    isBoost  = ( (itemNum >= ITEM_ARMOUR)
		                  && (itemNum <= ITEM_VIOLENT_FORCE) );
		bool    isShield = ( (itemNum >= ITEM_LGT_SHIELD)
		                  && (itemNum <= ITEM_HVY_REPULSOR_SHIELD) );

		if ( (money > item[itemNum].cost)
		  && (ni[itemNum] < MAX_ITEMS_IN_STOCK)
		  && env.isItemAvailable(itemNum)
		  && ( (HUMAN_PLAYER == type )
			|| !(isBoost || isShield)
			|| ( isBoost
			  && (ai_level > boostBought)
			  && (getBoostValue() < max_boost) )
			|| ( isShield
			  && (ai_level > shieldBought) ) ) ) {
			money -= item[itemNum].cost;
			ni[itemNum] += item[itemNum].amt;

			// Count it if it was a boost item
			if (isBoost)
				boostBought++; // Okay, take it!

			// Count it if it was a shield
			if (isShield)
				shieldBought++; // Okay, same procedure

			// don't allow more than MAX_ITEMS_IN_STOCK
			if (ni[itemNum] > MAX_ITEMS_IN_STOCK)
				ni[itemNum] = MAX_ITEMS_IN_STOCK;
		} else
			bought = false;
	}

	return bought;
}


/// @brief call this after loading a game to ensure backwards compatibility
void PLAYER::checkOppMem()
{
	if (!oppCount)
		newGame();
}


/// @brief Have the AI choosing something to buy.
int32_t PLAYER::chooseItemToBuy (int32_t max_boost, int32_t &last_idx)
{

	// Do not do this if there is no money:
	if (money < 1000)
		return -1;

	// Possibly pre-select an item by checking the current situation:
	int32_t currItem = computerSelectPreBuyItem (max_boost);

	// Be done already if the pre-selection provided a "must have"
	if ( (currItem > 0) && buy_item(currItem, max_boost) )
		return currItem;

	// Loop through the wish list and try to buy something.
	// The more of the item is in the inventory, the less likely
	// it is that the AI tries to buy the item. If the inventory
	// is empty, the chance is 50% for a useless bot and 88% for
	// a deadly bot.
	// There do not need to be any further modifications, the
	// preferences are already tweaked by defensiveness and
	// AI level.
	int32_t ai_level = static_cast<int32_t>(type);
	for ( ; last_idx < THINGS ; ++last_idx) {

		currItem = desired[last_idx];

		// Skip small missile, restart instead
		if (!currItem) {
			last_idx = 0;
			// Note: Small missiles are sorted to the end of the list.
			continue;
		}

		// Skip unaffordable items
		if ( ( (currItem <  WEAPONS)
		    && (weapon[currItem].cost > money) )
		  || ( (currItem >= WEAPONS)
		    && (item[currItem - WEAPONS].cost > money) ) )
			continue;

		// Now take the chance
		int32_t amount  = currItem < WEAPONS
		                ? weapon[currItem].amt
		                : item[currItem - WEAPONS].amt;
		int32_t maxAmt  = amount * ai_level;
		double  currAmt = currItem < WEAPONS
		                ? nm[currItem]
		                : ni[currItem - WEAPONS];
		double  newAmt  = currAmt + amount;
		double  amtMod  = currAmt > 0. ? newAmt / currAmt : 2.;
		// Note: The more items there are already, the more amtMod
		// will go down near 1.0, from a maximum of 2.0.

		int32_t chance = ROUNDu(amtMod * (static_cast<double>(ai_level) - .5));
		/* Results:
		 * Useless : 1 * (1 - 0.5) = 1 * (0.5) = 0.5 => 50% (rounded to 1)
		 * Useless : 2 * (1 - 0.5) = 2 * (0.5) = 1   => 50%
		 * Deadly  : 1 * (5 - 0.5) = 1 * (4.5) = 4.5 => 80% (rounded to 5)
		 * Deadly  : 2 * (5 - 0.5) = 2 * (4.5) = 9   => 87.5%
		 */

		// Bots have far higher limits for repair units and SDIs:
		if ( (ITEM_SDI       == (currItem - WEAPONS))
		  || (ITEM_REPAIRKIT == (currItem - WEAPONS)) )
			maxAmt *= 10;

		/* The bot tries to buy the item if:
		 * a) It does not have any and the chance is taken, or
		 * b) It has equal or more than the weapons amount times its level and
		 *    a negative (low) chance against its ai_level is taken.
		 */
		if ( ( ( (currAmt <  maxAmt) && (rand() % (chance + 1)) ) /* Scenario a) */
		    || ( (currAmt >= maxAmt) && RAND_AI_0N ) )            /* Scenario b) */
			 && buy_item(currItem, max_boost) ) {

			// Advance index to not buy the same item over and over again
			if (RAND_AI_1P)
				++last_idx;

			return currItem;
		}
	}

	return -1;
}


eControl PLAYER::computerControls (AICore* aicore, bool allow_fire)
{
	// Don't act at all when in scoreboard or endgame stage
	if (STAGE_SCOREBOARD <= global.stage)
		return CONTROL_NONE;

	int32_t       ai_weap    = tank ? tank->cw : SML_MIS;
	int32_t       ai_angle   = 0;
	int32_t       ai_power   = 0;
	ePlayerStages ai_stage   = PS_STAGE_COUNT;
	bool          is_working = aicore->status(ai_weap, ai_angle, ai_power, ai_stage);

	// If the AI is working with a different player or the AI is dead, return
    if ( !aicore->can_work()
	  || (is_working && (this != aicore->active_player())) )
		return CONTROL_NONE;

	// Do not try to start the AI if this players tank is
	// about to be destroyed or while it is moving.
	if (!tank || tank->destroy || tank->isFlying() || (tank->l < 1) )
		return CONTROL_NONE;

	tank->requireUpdate ();

	/* Start the AI for this player if:
	 * 1) The AI is idle and
	 * 2) the game is in aiming stage and
	 * 3) this player still has a tank that (checked above)
	 * 4) is not about to get destroyed and (checked above)
	 * 5) stands still on the ground.       (checked above)
	 */
	if ( (PS_AI_IS_IDLE == ai_stage)
	  && (STAGE_AIM     == global.stage) ) {

		if (aicore->start(this))
			return CONTROL_NONE;
		else {
			cerr << "FATAL: Can not start idle AI with this player!" << endl;
			global.set_command(GLOBAL_COMMAND_MENU);
			return CONTROL_QUIT;
		}
	}

	// Return at once if the AI is still initializing or cleaning up
	else if ( (PS_AI_INITIALIZE == ai_stage)
	       || (PS_CLEANUP       == ai_stage) )
		return CONTROL_NONE;

	// If the AI is not working (yet), return
	if (!is_working)
		return CONTROL_NONE;

	// Now, being here, the ai is working on this very player.

	// Copy stage now, as it is this players stage as well
	plStage = ai_stage;

	// Sanitize AI values:
	// Note: None of these should ever kick in!
	assert( (ai_angle >=        90) && "ERROR: AI set too low angle!");
	assert( (ai_angle <=       270) && "ERROR: AI set too high angle!");
	assert( (ai_power >=         0) && "ERROR: AI set too low power!");
	assert( (ai_power <= MAX_POWER) && "ERROR: AI set too high power!");
	assert( (0 == (ai_power % 5)  ) && "ERROR: AI set non mod 5 power!");
	assert( (   (ai_weap < 0) /* unset ! */
	       || ( (ai_weap <  WEAPONS) && nm[ai_weap          ] > 0)
	       || ( (ai_weap >= WEAPONS) && ni[ai_weap - WEAPONS] > 0) )
	     && "ERROR: AI set weapon that has a zero stock!" );
	if (ai_angle <        90) ai_angle = 90;
	if (ai_angle >       270) ai_angle = 270;
	if (ai_power <         0) ai_power = 0;
	if (ai_power > MAX_POWER) ai_power = MAX_POWER;
	ai_power -= ai_power % 5;
	if ( ( (ai_weap <  WEAPONS) && nm[ai_weap          ] <= 0)
	  || ( (ai_weap >= WEAPONS) && ni[ai_weap - WEAPONS] <= 0) )
		ai_weap = tank->cw;

	// Only put anything on the screen if this is the firing stage
	if (PS_FIRE == plStage) {

		// If there is a difference between the AI selection and the
		// display, transport the values on the screen.
		if ( (ai_angle != tank->a)
		  || (ai_power != tank->p)
		  || (ai_weap  != tank->cw) ) {
			global.updateMenu = true;

			if (global.skippingComputerPlay) {
				// When skipping, the values are simply copied:
				tank->a  = ai_angle;
				tank->p  = ai_power;
				tank->cw = ai_weap;
			} else {
				// Transfer angle:
				if (ai_angle > tank->a)
					++tank->a;
				else if (ai_angle < tank->a)
					--tank->a;

				// Transfer power:
				if (ai_power > tank->p)
					tank->p += 5;
				else if (ai_power < tank->p)
					tank->p -= 5;

				// Transfer weapon information:
				if (ai_weap != tank->cw) {
					changed_weapon = false;

					int32_t cw_mod = tank->cw < ai_weap ? 1 : -1;
					tank->cw += cw_mod;

					// Skip unusable items and those that are
					// out of stock
					while ( ( !env.isItemAvailable(tank->cw)
					       || ( (tank->cw <  WEAPONS) && !nm[tank->cw] )
					       || ( (tank->cw >= WEAPONS) && !ni[tank->cw - WEAPONS] ) )
					     && (tank->cw != ai_weap) )
						tank->cw += cw_mod;
				}
			} // End of regular transfer
		} // End of transferring difference

		// otherwise fire the current weapon if this is allowed
		else if (allow_fire) {
			aicore->weapon_fired();
			global.updateMenu = 1;

			if (type == VERY_PART_TIME_BOT)
				type = NETWORK_CLIENT;

			gloating = false;
			plStage  = PS_CLEANUP;
			return CONTROL_FIRE;
		}
	} // End of handling firing stage

	// If the AI wants to move their tank, do so:
	else if (PS_MOVE_LEFT == plStage) {
		if (tank->moveTank(DIR_LEFT)) {
			aicore->hasMoved(DIR_LEFT);
			return CONTROL_OTHER;
		} else
			aicore->hasMoved(0); // No movement possible
	} else if (PS_MOVE_RIGHT == plStage) {
		if (tank->moveTank(DIR_RIGHT)) {
			aicore->hasMoved(DIR_RIGHT);
			return CONTROL_OTHER;
		} else
			aicore->hasMoved(0); // No movement possible
	}

	return CONTROL_NONE;
}


int32_t PLAYER::computerSelectPreBuyItem (int32_t max_boost)
{
	int32_t max_level = static_cast<int32_t>(DEADLY_PLAYER);
	int32_t ai_level  = static_cast<int32_t>(type);
	double  mood      = 1. + defensive + (
	                    (static_cast<double>(rand())
	                  / (static_cast<double>(RAND_MAX) / 2.)) );
	// mood is 0.0 <= x <= 4.0

	/*	Prior buying anything else, a 5 step system takes place:
	 * 1.: Parachutes (if gravity is on)
	 * 2.: Minimum weapon probability (aka 5 medium and 3 large missiles)
	 * 3.: The most expensive item from the saveMoneyFor list
	 * 4.: Armour/Amps
	 * 5.: "Tools" to free themselves like Riot Blasts
	 * 6.: Shields, if enough money is there
	 * 7.: Fuel, everybody shall have at least 100 units.
	 * 8.: if all is set, look for dimpled/slick projectiles!
	 */


	// Step 1: Check for parachutes (if the bot remembers to check)
	if ( ( (type >= RANGEFINDER_PLAYER)
		|| RAND_AI_1P )
	  && (env.landSlideType > SLIDE_NONE)
	  && (ni[ITEM_PARACHUTE] < 10)
	  && (money > item[ITEM_PARACHUTE].cost) ) {

		DEBUG_LOG_FIN(name, "Pre-selecting Parachute", 0)
		return (WEAPONS + ITEM_PARACHUTE);
	}


    // Step 2: Minimum range of damaging weapons:
    // To be fair, this is always done and never forgotten.
	if ( (nm[LRG_MIS] < 3) && (money >= weapon[LRG_MIS].cost) ) {

		DEBUG_LOG_FIN(name, "Pre-selecting Large Missile", 0)
		return LRG_MIS;
	}

	if ( (nm[MED_MIS] < 5) && (money >= weapon[MED_MIS].cost) ) {

		DEBUG_LOG_FIN(name, "Pre-selecting Medium Missile", 0)
		return MED_MIS;
	}


	// Step 3: Check weapons/items currently saving money for:
	int32_t saved_cost = 0;
	int32_t saved_item = 0;
	for (int32_t i = 0; i < THINGS; ++i) {
		if ( (saveMoneyFor[i] > 0)
		  && (saveMoneyFor[i] < money)
		  && (saveMoneyFor[i] > saved_cost) ) {
			saved_cost = saveMoneyFor[i];
			saved_item = i;
		}
	}
	// Got one?
	if (saved_item > 0) {
		DEBUG_LOG_FIN(name, "Finally got enough money for %s!",
		              saved_item < WEAPONS
		                ? weapon[saved_item].getName()
		                : item[saved_item - WEAPONS].getName())
		// Take it out from the wish list:
		saveMoneyFor[saved_item] = 0;
		return saved_item;
	}


	// Step 4: Check for Armour / Amps (if the bot remembers to check)
	if ( (boostBought < ai_level) && RAND_AI_1P ) {
		int32_t boost_value = getBoostValue();
		int32_t boost_limit = max_boost - boost_value;
		int32_t armour_val  = getArmourValue();
		int32_t amp_val     = getAmpValue();


		if ( (boost_limit > (max_level - ai_level + 1))
		  && (!needDamage || RAND_AI_0P) ) {

			DEBUG_LOG_FIN(name,
						"Pre-Check: Max Boost %d, Armour %d, Amp %d, Limit %d",
						max_boost, armour_val, amp_val, boost_limit);

			// See which is preferred:
			boost_limit = max_boost - (2 * (DEADLY_PLAYER + 1)) + RAND_AI_0P;

			if ( boost_value < boost_limit) {
				if ( (amp_val    - defensive)
				   < (armour_val + defensive) ) {
					needAmp    = true;
					needArmour = false;
				} else {
					needAmp    = false;
					needArmour = true;
				}
			}


			// Prefer armour if the mood is defensive and armour isn't too
			// far ahead from the amps:
			if ( needArmour || ( (mood >= 2.0)  && (armour_val <= amp_val) ) ) {
				// The player is in a defensive mood or armour has fallen behind
				// If we have 25% more money than the plasteel cost, buy it,
				// else the armour will do. If the armour is far behind, no
				// money is spared.
				if ( (money >= (item[ITEM_PLASTEEL].cost * 1.25) )
				  || ( ( (armour_val < (amp_val * 0.5)) || !armour_val)
					&& (money > item[ITEM_PLASTEEL].cost) ) ) {

					DEBUG_LOG_FIN(name, "Pre-selecting Plasteel Plating", 0)
					return (WEAPONS + ITEM_PLASTEEL);

				}

				if ( (money >= (item[ITEM_ARMOUR].cost * 2.0))
				  && (ni[ITEM_ARMOUR] < ni[ITEM_PLASTEEL])
				  && (mood >= 3.5) ) {

					DEBUG_LOG_FIN(name, "Pre-selecting Armour", 0)
					return (WEAPONS + ITEM_ARMOUR);
				}
			} // End of armour check

			// Otherwise go for a shining new amp:
			if ( needAmp || ( (mood <= 2.0)  && (amp_val <= armour_val) ) ) {
				// The player is in a offensive mood or amps have fallen behind
				// If we have 25% more money than the violent force cost, buy
				// it, else the normal amp will do.
				// If the amps have fallen behind too much, no money is spared.
				if ( (money >= (item[ITEM_VIOLENT_FORCE].cost * 1.5) )
				  || ( ( (amp_val < (armour_val * 0.5) ) || !amp_val)
					&& (money > item[ITEM_VIOLENT_FORCE].cost) ) ) {

					DEBUG_LOG_FIN(name, "Pre-selecting Violent Force", 0)
					return (WEAPONS + ITEM_VIOLENT_FORCE);

				}

				if ( (money >= (item[ITEM_INTENSITY_AMP].cost * 1.75))
				  && (ni[ITEM_INTENSITY_AMP] < ni[ITEM_VIOLENT_FORCE])
				  && (mood < 1.0) ) {

					DEBUG_LOG_FIN(name, "Pre-selecting Intensity Amp", 0)
					return (WEAPONS + ITEM_INTENSITY_AMP);
				}
			} // End of amp check
		} // End of being allowed to by boost items
	} // end of step 3


	// 5.: Freeing tools
	if (RAND_AI_0P) {
		if ( !nm[HVY_RIOT_BOMB] || !nm[RIOT_BOMB] || (mood < 2.) ) {

			// More offensive in this round, check for riot bombs
			if ( (nm[HVY_RIOT_BOMB] < 2)
			  && (money >= weapon[HVY_RIOT_BOMB].cost) ) {

				DEBUG_LOG_FIN(name, "Pre-selecting Heavy Riot Bomb", 0)
				return HVY_RIOT_BOMB;
			}

			if ( (nm[RIOT_BOMB] < 5)
			  && (money >= weapon[RIOT_BOMB].cost) ) {

				DEBUG_LOG_FIN(name, "Pre-selecting Riot Bomb", 0)
				return RIOT_BOMB;
			}
		} else {
			// In a defensive mood the charges are checked
			if ( (nm[RIOT_BLAST] < 2)
			  && (money >= weapon[RIOT_BLAST].cost) ) {

				DEBUG_LOG_FIN(name, "Pre-selecting Riot Blast", 0)
				return RIOT_BLAST;
			}

			if ( (nm[RIOT_CHARGE] < 5)
			  && (money >= weapon[RIOT_CHARGE].cost) ) {

				DEBUG_LOG_FIN(name, "Pre-selecting Riot Charge", 0)
				return RIOT_CHARGE;
			}
		}
	} // End of step 4


	// 6.: Shields
	if ((shieldBought < ai_level) && RAND_AI_1P) {
		if (mood <= 1.5) {

			// offensive type, go through reflectors
			if ( (   ni[ITEM_LGT_REPULSOR_SHIELD]
			      <= (item[ITEM_LGT_REPULSOR_SHIELD].amt * ai_level) )
			  && (money >= (item[ITEM_LGT_REPULSOR_SHIELD].cost * 2.0)) ) {

				DEBUG_LOG_FIN(name, "Pre-selecting Light Repulsor Shield", 0)
				return (WEAPONS + ITEM_LGT_REPULSOR_SHIELD);
			}

			if ( (   ni[ITEM_MED_REPULSOR_SHIELD]
			      <= (item[ITEM_MED_REPULSOR_SHIELD].amt * ai_level) )
			  && (money >= (item[ITEM_MED_REPULSOR_SHIELD].cost * 1.75)) ) {

				DEBUG_LOG_FIN(name, "Pre-selecting Medium Repulsor Shield", 0)
				return (WEAPONS + ITEM_MED_REPULSOR_SHIELD);
			}

			if ( (   ni[ITEM_HVY_REPULSOR_SHIELD]
			      <= (item[ITEM_HVY_REPULSOR_SHIELD].amt * ai_level) )
			  && (money >= (item[ITEM_HVY_REPULSOR_SHIELD].cost * 1.5)) ) {

				DEBUG_LOG_FIN(name, "Pre-selecting Heavy Repulsor Shield", 0)
				return (WEAPONS + ITEM_HVY_REPULSOR_SHIELD);
			}
		} // End of offensive mood

		if (mood >= 2.5) {

			// defensive type, go through hard shields
			if ( (   ni[ITEM_LGT_SHIELD]
			      <= (item[ITEM_LGT_SHIELD].amt * ai_level) )
			  && (money >= (item[ITEM_LGT_SHIELD].cost * 2.0)) ) {

				DEBUG_LOG_FIN(name, "Pre-selecting Light Shield", 0)
				return (WEAPONS + ITEM_LGT_SHIELD);
			}

			if ( (   ni[ITEM_MED_SHIELD]
			      <= (item[ITEM_MED_SHIELD].amt * ai_level) )
			  && (money >= (item[ITEM_MED_SHIELD].cost * 1.75)) ) {

				DEBUG_LOG_FIN(name, "Pre-selecting Medium Shield", 0)
				return (WEAPONS + ITEM_MED_SHIELD);
			}

			if ( (   ni[ITEM_HVY_SHIELD]
			      <= (item[ITEM_HVY_SHIELD].amt * ai_level) )
			  && (money >= (item[ITEM_HVY_SHIELD].cost * 1.5)) ) {

				DEBUG_LOG_FIN(name, "Pre-selecting Heavy Shield", 0)
				return (WEAPONS + ITEM_HVY_SHIELD);
			}
		} // End of defensive mood
	} // End of step 5


	// Step 7: Fuel
	if ( (ni[ITEM_FUEL] < 100)
	  && (money >= item[ITEM_FUEL].cost) ) {
		DEBUG_LOG_FIN(name, "Pre-selecting Fuel", 0)
		return (WEAPONS + ITEM_FUEL);
	}


    // Step 8: Slick / Dimpled Projectiles
    if ( (ni[ITEM_SLICKP] + ni[ITEM_DIMPLEP]) < 100 ) {

		if ( (ni[ITEM_DIMPLEP] < 50)
		  && (money >= item[ITEM_DIMPLEP].cost) ) {

			DEBUG_LOG_FIN(name, "Pre-selecting Dimpled Projectiles", 0)
			return (WEAPONS + ITEM_DIMPLEP);
		}

		if ( (ni[ITEM_SLICKP] < 50)
		  && (money >= item[ITEM_SLICKP].cost) ) {

			DEBUG_LOG_FIN(name, "Pre-selecting Slick Projectiles", 0)
			return (WEAPONS + ITEM_SLICKP);
		}
    }

	return -1;
}


eControl PLAYER::controlTank (AICore* aicore, bool allow_fire)
{
	// Handle User input, this is read for providing the ingame menu
	// even when no human player is active. Otherwise the player would
	// not be able to enter the ingame menu whenever an AI player is
	// active.
	k = 0;
	K = 0;
	if (key_shifts & KB_CTRL_FLAG)  has_ctrl_pressed  = true;
	else                            has_ctrl_pressed  = false;
	if (key_shifts & KB_SHIFT_FLAG) has_shift_pressed = true;
	else                            has_shift_pressed = false;
	if (keypressed() ) {
		k = readkey ();
		K = k >> 8;

		// Enter ingame menu?
		if ( (KEY_ESC == K) || (KEY_P == K) ) {
			int32_t mm = env.ingamemenu ();

			global.make_update (0, 0, env.screenWidth, env.screenHeight);
			global.make_bgupdate (0, 0, env.screenWidth, env.screenHeight);

			switch (mm) {
				case 1:
					// Main Menu
					global.set_command(GLOBAL_COMMAND_MENU);
					return CONTROL_QUIT;
				case 2:
					// Quit the game
					global.set_command(GLOBAL_COMMAND_QUIT);
					return CONTROL_QUIT;
				case 3:
					// Skip AI
					if (STAGE_SCOREBOARD > global.stage)
						return CONTROL_SKIP;
				default:
					break;
			}
		}

		// check for number key being pressed
		if ( (K >= KEY_0) && (K <= KEY_9) ) {
			int32_t value = K - KEY_0;
			K = 0;

			// make sure the value is within range
			if ( (value < env.numGamePlayers)
			  && env.players[value] ) {

				TANK *my_tank = env.players[value]->tank;

				if (my_tank) {
					snprintf(global.tank_status, 127,
							 "%s: %d + %d -- Team: %s",
							 env.players[value]->name,
							 my_tank->l,
							 my_tank->sh,
							 env.players[value]->getTeamName() );
					global.tank_status_colour = env.players[value]->color;
					global.updateMenu = 1;
				} else
					memset(global.tank_status, 0, sizeof(char) * 128);
			}
		} // end of check status keys

		// Check for scorboard toggle key
		if ( (K == KEY_TILDE) || (K == KEY_SLASH) ) {
			K = 0;
			global.showScoreBoard = !global.showScoreBoard;
			if (!global.showScoreBoard) {
				// erase it:
				global.make_update  (0, MENUHEIGHT, 300,
							 (env.maxNumTanks + 1) * env.fontHeight );
				global.make_bgupdate(0, MENUHEIGHT, 300,
							 (env.maxNumTanks + 1) * env.fontHeight );
			}
		}

		// Handle volume control
		if (KEY_V == K) {
			K = 0;
			if (has_shift_pressed)
				env.increaseVolume();
			else
				env.decreaseVolume();
		}
	} // End of handling all time possible key presses.


	if (key[KEY_F1]) {
		int32_t nr = 0;
		do {
			snprintf(path_buf, PATH_MAX, "screenshot_%03d.bmp", ++nr);
		} while (!access(path_buf, F_OK));

		if (nr < 1000)
			save_bmp(path_buf, global.canvas, nullptr);
	}


	if (has_ctrl_pressed && ctrlUsedUp) {
		if ( !(key[KEY_LEFT]
			|| key[KEY_RIGHT]
			|| key[KEY_UP]
			|| key[KEY_DOWN]
			|| key[KEY_PGUP]
			|| key[KEY_PGDN]
			|| key[KEY_A]
			|| key[KEY_D]
			//additional control
			|| key[KEY_W]
			|| key[KEY_S]
			|| key[KEY_R]
			|| key[KEY_F]) )
			ctrlUsedUp = false;
	} else
		ctrlUsedUp = false;


	// A) HUMAN
	if ( (HUMAN_PLAYER == type) || !tank)
		return humanControls (aicore);

	// B) Network Client
#ifdef NETWORK
	else if (type == NETWORK_CLIENT)
		return executeNetCmd(true, aicore);
#endif // NETWORK

	// C) AI Player
	else if (global.stage == STAGE_AIM)
		return computerControls(aicore, allow_fire);

	return CONTROL_NONE;
}


void PLAYER::drawIndicator(int32_t x, int32_t y, int32_t h)
{
	if (HUMAN_PLAYER == type) {
		int32_t radius = ROUND(static_cast<double>(h) / 2.) - 2;
		circlefill (global.canvas, x + radius + 2, y + radius + 2, radius,
						makecol (200, 100, 255));
		circle     (global.canvas, x + radius + 2, y + radius + 2, radius,
						BLACK);
	} else {
		rectfill (global.canvas, x, y + 2, x + 15, y + h - 1, BLACK);
		for (int32_t i = 0; i < type; ++i)
			rectfill(global.canvas,
					x + (3 * i) + 1, y + 3,
					x + (3 * i) + 2, y + h - 2,
					makecol (100, 255, 100));
	}
}


#ifdef NETWORK
// This function gets called during a round when a networked
// player gets to act. The function checks to see if anything
// is in the net_command variable. If there is, it handles
// the request. If not, the function returns.
//
// We should have some time keeping in here before this goes live
// to avoid hanging the game.
eControl PLAYER::executeNetCmd(bool my_turn, AICore* aicore)
{
   char buffer[NET_COMMAND_SIZE];
   static int playerindex = -1;
   static int fire_delay = 0, net_delay = 0;
	int32_t towrite, written;

   if (my_turn)
   {
       fire_delay++;
       // if enough time has passed, we give up and turn control over to the AI
       if (fire_delay >= NET_DELAY)
       {
          type = VERY_PART_TIME_BOT;
          fire_delay = 0;
          return computerControls(aicore, true);
       }
   }

   if (! net_command[0])
   {
           net_delay++;
           /*
           if (my_delay >= NET_DELAY)
           {
              my_delay = 0;
              setComputerValues();
              type = VERY_PART_TIME_BOT;
              strcpy(buffer, "PING");
              write(server_socket, buffer, strlen(buffer));
              return computerControls();
           }
           else*/
           if (net_delay >= NET_DELAY_SHORT)
               // prompt the client to respond
				SAFE_WRITE(server_socket, "%s", "PING")
           return CONTROL_NONE;
   }     // we did not get a command to process

   else
     net_delay = 0;       // we got something, so reset timer

   if (! strncmp(net_command, "VERSION", 7) )
		SAFE_WRITE(server_socket, "SERVERVERSION %s", VERSION)
   else if (! strncmp(net_command, "CLOSE", 5) )
   {
       close(server_socket);
       type = DEADLY_PLAYER;
   }
   else if (! strncmp(net_command, "BOXED", 5) )
   {
      char buffer[32];
		SAFE_WRITE(server_socket, "BOXED %d", env.isBoxed ? 1 : 0);
   }
   else if (! strncmp(net_command, "GOSSIP", 6) )
   {
      snprintf(global.tank_status, 127, "%s", & (net_command[7]) );
      global.updateMenu = TRUE;
   }
   else if (! strncmp(net_command, "HEALTH", 6) )
   {
       int tankindex;
       char buffer[64];

       sscanf( &(net_command[7]), "%d", &tankindex );
       if ( (tankindex >= 0) && (tankindex < env.numGamePlayers) )
       {
           if (env.players[tankindex]->tank)
				SAFE_WRITE(server_socket, "HEALTH %d %d %d %d", tankindex,
						env.players[tankindex]->tank->l,
						env.players[tankindex]->tank->sh,
						env.players[tankindex]->tank->sht)
       }

   }
   else if (! strncmp(net_command, "ITEM", 4) )
   {
       char buffer[32];
       int itemindex;
       sscanf( &(net_command[5]), "%d", &itemindex);
       if ( (itemindex >= 0) && (itemindex < ITEMS) )
			SAFE_WRITE(server_socket, "ITEM %d %d", itemindex, ni[itemindex])
   }
   else if (! strncmp(net_command, "MOVE", 4) )
   {
       if (! my_turn) return CONTROL_NONE;
       if (tank)
       {
           if (strstr(net_command, "LEFT") )
              tank->moveTank(DIR_LEFT);
           else
              tank->moveTank(DIR_RIGHT);
           global.updateMenu = 1;
       }
   }
   else if (! strncmp(net_command, "FIRE", 4) )
   {
       int angle = 180, power = 1000, item = 0;
       if (! my_turn) return CONTROL_NONE;
       sscanf( & (net_command[5]), "%d %d %d", &item, &angle, &power);
       fire_delay = 0;
       if (tank)
       {
           if (item >= THINGS) item = 0;
           tank->cw = item;
           if (item < WEAPONS)
           {
              if (nm[tank->cw] < 1)
                 tank->cw = 0;
           }
           else   // item
           {
              if ( ni[tank->cw - WEAPONS] < 1)
                  tank->cw = 0;
           }
           tank->a = angle;
           if (tank->a > 270) tank->a = 270;
           else if (tank->a < 90) tank->a = 90;
           tank->p = power;
           if (tank->p > 2000) tank->p = 2000;
           else if (tank->p < 0) tank->p = 0;
           gloating = false;
           net_command[0] = '\0';
           return CONTROL_FIRE;
       }
   }

   // find out which player this is
   else if (! strncmp(net_command, "WHOAMI", 6) )
   {
       bool found = false;
       char buffer[128];

       while ( (playerindex < env.numGamePlayers) && (! found) )
       {
           if ( env.players[playerindex] == this )
           {
               found = true;
				SAFE_WRITE(server_socket, "YOUARE %d", playerindex)
           }
           else
              playerindex++;
       }
       // check to see if something went very wrong
       if (! found)
			SAFE_WRITE(server_socket, "YOUARE %d", -1)
   }
   // return wind speed
   else if (! strncmp(net_command, "WIND", 4) )
   {
      char buffer[64];
		SAFE_WRITE(server_socket, "WIND %f", global.wind)
   }

   // find out how many players we have
   else if (! strncmp(net_command, "NUMPLAYERS", 10) )
   {
      char buffer[32];
		SAFE_WRITE(server_socket, "NUMPLAYERS %d", env.numGamePlayers)
   }
   else if (! strncmp(net_command, "PLAYERNAME", 10) )
   {
      int my_number;
      char buffer[128];
      sscanf( &(net_command[11]), "%d", &my_number);
      if ( (my_number >= 0) && (my_number < env.numGamePlayers) )
      		SAFE_WRITE(server_socket, "PLAYERNAME %d %s", my_number,
						env.players[my_number]->getName())
	}

   // how many rounds are we playing
   else if (! strncmp(net_command, "ROUNDS", 6) )
   {
       char buffer[64];
		SAFE_WRITE(server_socket, "ROUNDS %d %d", env.rounds,
					global.currentround);
   }
   // send back the position of each tank
   else if (! strncmp(net_command, "TANKPOSITION", 12) )
   {
      char buffer[64];
      int count;

      sscanf( &(net_command[13]), "%d", &count);
      if ( (count >= 0) && (count < env.numGamePlayers)
		&& (env.players[count]->tank) )
			SAFE_WRITE(server_socket, "TANKPOSITION %d %d %d", count,
						(int) env.players[count]->tank->x,
						(int) env.players[count]->tank->y)
   }

   // send back the surface height of the dirt
   else if (! strncmp(net_command, "SURFACE", 7) )
   {
		char buffer[32];
		int x;

		sscanf( &(net_command[8]), "%d", &x);
		if ( (x >= 0) && (x < env.screenWidth) )
#if defined(ATANKS_IS_BSD)
			SAFE_WRITE(server_socket, "SURFACE %d %d", x,
			           global.surface[x].load())
#else
			SAFE_WRITE(server_socket, "SURFACE %d %ld", x,
			           global.surface[x].load())
#endif // BSD
	}
   else if (! strncmp(net_command, "SCREEN", 6) )
   {
      char buffer[64];
		SAFE_WRITE(server_socket, "SCREEN %d %d",
					env.screenWidth, env.screenHeight)
   }
   else if (! strncmp(net_command, "TEAMS", 5) )
   {
      int count;
      char buffer[32];

      sscanf( &(net_command[6]), "%d", &count);
      if ( (count < env.numGamePlayers) && (count >= 0) )
			SAFE_WRITE(server_socket, "TEAM %d %d", count,
						(int) env.players[count]->team)
   }
   else if (! strncmp(net_command, "WALLTYPE", 8) )
   {
      char buffer[32];
		SAFE_WRITE(server_socket, "WALLTYPE %d", env.current_wallType)
   }
   else if (! strncmp(net_command, "WEAPON", 6) )
   {
      char buffer[32];
      int weapon_number;
      sscanf( &(net_command[7]), "%d", &weapon_number);
      if ( (weapon_number >= 0) && (weapon_number < WEAPONS) )
		SAFE_WRITE(server_socket, "WEAPON %d %d", weapon_number, nm[weapon_number])
   }

   net_command[0] = '\0';

   return CONTROL_NONE;
}
#endif // NETWORK


void PLAYER::exitShop()
{
	double tmpDM = (ni[ITEM_INTENSITY_AMP] * item[ITEM_INTENSITY_AMP].vals[0])
	             + (ni[ITEM_VIOLENT_FORCE] * item[ITEM_VIOLENT_FORCE].vals[0]);

	damageMultiplier = 1.0;

	if (tmpDM > 0)
		damageMultiplier += std::pow(tmpDM, 0.6);

	// All players need small missiles:
	if (nm[SML_MIS] < 100)
		nm[SML_MIS] += 100 + (rand() % 100); // + [100;199]
	if (nm[SML_MIS] < 250)
		nm[SML_MIS] +=  50 + (rand() %  50); // + [ 50; 99]
}


/// @brief fill the list of desired items and return the number of damaging weapons
int32_t PLAYER::generateDesiredList()
{
	int32_t result = 0;

	memset(desired, 0, sizeof(int32_t) * THINGS);

	for (int32_t i = 1; i < THINGS; ++i) {
		if (env.isItemAvailable(i)) {
			desired[i]  = i;
			currPref[i] = weapPref[i];

			// No negative prefs:
			if (currPref[i] < 0)
				currPref[i] = 0;

			// Count weapons:
			if ( (i < WEAPONS) && nm[i] )
				++result;
		} else
			desired[i] = 0;
		// Unavailable items will not be inserted and
		// the slot is filled with a 0 (small missile)
		// that will be sorted away.
	}

	return result;
}


void PLAYER::generatePreferences()
{
	double  baseProb    = static_cast<double>(MAX_WEAP_PROBABILITY) / 2.;
	int32_t currItem    = 0;
	double  worth       = 0.;
	bool    isWarhead   = false;
	int32_t maxWeapPref = 0;
	int32_t maxItemPref = 0;
	double  ai_rate     = static_cast<double>(type) / 2. + .5;

	/* --------------------------------------
	 * --- Generate basic characteristics ---
	 * --------------------------------------
	 */
	defensive          = (static_cast<double>(rand() % 10001) / 5000.)
	                   - 1.; // [-1;+1]
	vengeful           = 1 + (rand () % 100); // [1;100]
	vengeanceThreshold = 0.05
	                   + (static_cast<double>(rand () % 901)
	                      / 1000.); // [0.05;0.95]
	selfPreservation   = static_cast<double>(rand () % 3001) / 1000; // [0;3]
	painSensitivity    = static_cast<double>(rand () % 3001) / 1000; // [0;3]

	// Now 'defensive' can be modified by team:
	if (team == TEAM_JEDI) {
		defensive += static_cast<double>(rand() % 501) / 1000.;
		if (defensive > 1.25)
			defensive = 1.25; // + 1.25 is Super Defensive
	}
	else if (team == TEAM_SITH) {
		defensive -= static_cast<double>(rand() % 501) / 1000.;
		if (defensive < -1.25)
			defensive = -1.25; // - 1.25 is Super Aggressive
	}

	/* --------------------------------------------
	 * --- Generate weapon and item preferences ---
	 * --------------------------------------------
	 */
	if (strcmp(name, "New Player")) {
		DEBUG_LOG_EMO(name, "Generating preferences (defensive %lf)", defensive)
		DEBUG_LOG_EMO(name, "---------------------------------------", 0)
	}

	weapPref[0] = 0; // small missiles are always zero!

	for (int32_t i = 1; i < THINGS; ++i) {
		worth     = baseProb / -2.;
		isWarhead = false;

		if (i < WEAPONS) {
			// Talking about weapons
			currItem = i;
			if (weapon[i].warhead
			  || ( (currItem >= SML_METEOR)
				&& (currItem <= LRG_LIGHTNING)))
				isWarhead = true;
				// Warheads are ignored, this way naturals
				// are taken out automatically.
			else {

				int32_t warheads = weapon[currItem].spread;

				// === 1. Damage: ===
				//--------------------
				if (weapon[currItem].numSubmunitions > 0) {

					warheads = weapon[currItem].numSubmunitions;

					// Use the total damage for clusters
					worth = weapon[weapon[currItem].submunition].damage * warheads;

					if ( ( (currItem >= SML_NAPALM) && (currItem <= LRG_NAPALM))
					  || ( (currItem >= FUNKY_BOMB) && (currItem <= FUNKY_DEATH)) )
						worth /= (defensive + 2. + ai_rate) / 2.;
						// These weapons are too unpredictable to be counted full.
						// But a true offensive useless bot divides only by 1.0
						// (so not all all, they do not mind) and a true
						// defensive deadly bot divides by 2.5

					// Napalm Jellies doe damage over time. So their worth
					// has to reflect that.
					if ( (currItem >= SML_NAPALM) && (currItem <= LRG_NAPALM))
						worth *= static_cast<double>(EXPLOSIONFRAMES) / 2.
						       / static_cast<double>(type);

					if (worth > baseProb)
						// Or Large Napalm will always be everybody favourite
						worth = baseProb;
				} else
					// Otherwise use spread value with damage. For non-spread
					// weapons this value is always 1.
					worth = weapon[currItem].damage * (warheads * 2)
					      * weapon[currItem].getDelayDiv();
					// Note: warheads are counted twice, because otherwise spread
					//       weapons get a by far too low score!

				// 1 Damage is worth 0.5%o of the base probability.
				worth *= baseProb * 0.0005;

				// === 2. Defensiveness multiplier ===
				//-------------------------------------
				// As said above, defensive players avoid spread/cluster
				// weapons that are too unpredictable. Thus they rate
				// non-spreads higher:
				if (warheads == 1)
					worth *= (defensive + 1.5) * ai_rate;

				// === 3. Dirt weapons ===
				//-------------------------
				// Dirt balls and such weapons do no damage and have to be rated
				// by defensiveness value. Further more the higher the self
				// preservation value of a bot, the more likely they will try
				// to bury main damage dealers for one or two rounds of bought
				// silence.
				if ( (currItem >= DIRT_BALL) && (currItem <= SMALL_DIRT_SPREAD) )
					worth = warheads * weapon[currItem].radius
					      * ai_rate * (defensive + 2.)
					      * selfPreservation;

				// === 4. Debuff weapons ===
				//---------------------------
				// These are the opposite of dirt weapons, they are for the
				// offensive type with high self preservation.
				// Note: Although the percent bomb is not a de-buff weapon,
				// it can hardly be rated any other way, as it has no set yield.
				if ( (currItem >= PERCENT_BOMB) && (currItem <= REDUCER) )
					worth = 300. * ai_rate
					      * -(defensive - 2.)
					      * selfPreservation;
				// Note: The theft bomb is a debuff weapon with extra benefits. ;-)
				if (THEFT_BOMB == currItem)
					worth = (150. + vengeful) * ai_rate
					      * ( (selfPreservation + 2.) / 2.)
					      * (std::abs(defensive) + 1.0);

				// === 5. Shaped weapons are deadly but limited ===
				//--------------------------------------------------
				if ( ( (currItem >= SHAPED_CHARGE) && (currItem <= CUTTER) )
				  || ( DRILLER == currItem ) )
					worth *= 1.0 - ( ((2. * ai_rate) + (defensive * 5.0)) / 20.0);
					// useless, full offensive: * 1.15
					// deadly, full defensive : * 0.45

				// === 6. Rollers and penetrators ===
				//------------------------------------
				// These are modified by type, as they *are* useful
				if ( ( (currItem >= SML_ROLLER) && (currItem <= DTH_ROLLER) )
				  || ( (currItem >= BURROWER)   && (currItem <= PENETRATOR) ) )
					worth *= 1.0 + (ai_rate / 5.) + (defensive / 2.);

				// === 7. Tectonics need to be raised! ===
				//-----------------------------------------
				// These are nice to damage multiple buried enemies where
				// penetrators can only reach one.
				if ( (currItem >= TREMOR) && (currItem <= TECTONIC) )
					worth *= 2.0 + (ai_rate / 5.) + (defensive / 3.);

				// finally dWorth must not be greater than the 3/4 of MAX_WEAPON_PROBABILITY
				if (worth > (MAX_WEAP_PROBABILITY * 0.75))
					worth =  MAX_WEAP_PROBABILITY * 0.75;
			} // End of "not a warhead"
		} else {
			// Talking about items
			currItem = i - WEAPONS;

			/* Theory:
			 * The more offensive a bot is, the more likely they go for
			 * damage amps and repulsor shields.
			 * The more defensive they are, the more likely they go for
			 * armour and hard shields.
			 */

			switch (currItem) {
				case ITEM_TELEPORT:
					worth = (defensive - 1.5) * (baseProb / -5.00) * selfPreservation;
					break;
				case ITEM_SWAPPER:
					worth = (defensive - 1.5) * (baseProb / -3.75) * selfPreservation;
					break;
				case ITEM_MASS_TELEPORT:
					worth = (defensive - 1.5) * (baseProb / -1.50) * selfPreservation;
					break;
				case ITEM_FAN:
					worth = 0.0; // useless things!
					break;
				case ITEM_VENGEANCE:
				case ITEM_DYING_WRATH:
				case ITEM_FATAL_FURY:
					worth = (defensive + 1.5)
					      *  static_cast<double>(weapon[(int)item[currItem].vals[0]].damage)
					      *  item[currItem].vals[1];
					break;
				case ITEM_ARMOUR:
				case ITEM_PLASTEEL:
					worth = baseProb * ( item[currItem].vals[0] / item[ITEM_PLASTEEL].vals[0])
					      * ( defensive + 1.25 );
					break;
				case ITEM_LGT_SHIELD:
				case ITEM_MED_SHIELD:
				case ITEM_HVY_SHIELD:
					worth = baseProb * ( item[currItem].vals[0] / item[ITEM_HVY_SHIELD].vals[0])
					      * ( defensive + 1.25 );
					break;
				case ITEM_INTENSITY_AMP:
				case ITEM_VIOLENT_FORCE:
					worth = baseProb * ( item[currItem].vals[0] / item[ITEM_VIOLENT_FORCE].vals[0])
					      * ( (-1. * defensive) + 1.25 );
					break;
				case ITEM_LGT_REPULSOR_SHIELD:
				case ITEM_MED_REPULSOR_SHIELD:
				case ITEM_HVY_REPULSOR_SHIELD:
					worth = baseProb * ( item[currItem].vals[0] / item[ITEM_HVY_REPULSOR_SHIELD].vals[0])
					      * ( (-1. * defensive) + 1.25 );
					break;
				case ITEM_REPAIRKIT:
					worth = (baseProb / 12. * ai_rate)
					      * (defensive + 2.25 + (selfPreservation / 2.));
					break;
				case ITEM_PARACHUTE:
					worth = (baseProb / 10. * ai_rate)
					      * ( (defensive + 1.5) / 1.5);
					// Parachutes *are* popular! :)
					break;
				case ITEM_SLICKP:
					worth = baseProb / 25. * ai_rate;
					break;
				case ITEM_DIMPLEP:
					worth = baseProb / 15. * ai_rate;
					break;
				case ITEM_FUEL:
					worth = baseProb / 30. * ai_rate;
					isWarhead = true;  // Yes, it's a lie. ;-)
					break;
				case ITEM_ROCKET:
					worth     = -5000; // Bots don't use rockets
					isWarhead = true;  // The cake is a lie!
					break;
				case ITEM_SDI:
					worth = (baseProb / 13. * ai_rate)
					      * ( (defensive + 2.25 + selfPreservation) / 1.25);
					break;
				default:
					cerr << "Error: Unhandled item " << currItem;
					cerr << "       in generatePreferences()!" << endl;
					worth = baseProb / ai_rate;
			}


			// worth must not be greater than the half of MAX_WEAPON_PROBABILITY
			if (worth > (MAX_WEAP_PROBABILITY / 2))
				worth =  MAX_WEAP_PROBABILITY / 2;
		}

		// Boost the tiny ones:
		if (worth < (MAX_WEAP_PROBABILITY / 25.0))
			worth =  MAX_WEAP_PROBABILITY / 25.0; // Which is very very little...
		if (worth < (MAX_WEAP_PROBABILITY / 8))
			// allow to double (more or less)
			worth += static_cast<double>(rand() % static_cast<int32_t>(std::abs(worth)));

		// But don't overdo either:
		if (worth >  MAX_WEAP_PROBABILITY)
			worth =  MAX_WEAP_PROBABILITY;

		if (isWarhead)
			weapPref[i] = 0;	// It will not get any slot!
		else
			weapPref[i] = ROUND(worth);

		// Count statistical values
		if ((i < WEAPONS) && (weapPref[i] > maxWeapPref))
			maxWeapPref = weapPref[i];
		if ((i >= WEAPONS) && (weapPref[i] > maxItemPref))
			maxItemPref = weapPref[i];

		if (strcmp(name, "New Player")) {
			DEBUG_LOG_EMO(name, "%23s (%6s): %5d",
					i < WEAPONS ? weapon[i].getName() : item[i - WEAPONS].getName(),
					i < WEAPONS ? "weapon" : "item",
					weapPref[i])
		}
	} // end of looping THINGS

	// If the maximum preferences are too low, they have to be augmented
	if (maxWeapPref < MAX_WEAP_PROBABILITY) {
		worth = static_cast<double>(MAX_WEAP_PROBABILITY)
		      / static_cast<double>(maxWeapPref);

		for (int32_t i = 1; i < WEAPONS; ++i) {
			if (weapPref[i] > (MAX_WEAP_PROBABILITY / 100.0)) {
				weapPref[i] = ROUND(worth * weapPref[i]);
				if (strcmp(name, "New Player")) {
					DEBUG_LOG_EMO(name, "%23s (%6s) amplified to: %5d",
							weapon[i].getName(), "weapon", weapPref[i])
				}
			}
		}
	}

	if (maxItemPref < (MAX_WEAP_PROBABILITY * 0.75) ) {
		worth = static_cast<double>(MAX_WEAP_PROBABILITY) * 0.75
		      / static_cast<double>(maxItemPref);

		for (int32_t i = WEAPONS; i < THINGS; ++i) {
			if (weapPref[i] > (MAX_WEAP_PROBABILITY / 100.0)) {
				weapPref[i] = ROUND(worth * weapPref[i]);
				if (strcmp(name, "New Player")) {
					DEBUG_LOG_EMO(name, "%23s (%6s) amplified to: %5d",
							item[i - WEAPONS].getName(), "item", weapPref[i])
				}
			}
		}
	}

	if (strcmp(name, "New Player"))
		DEBUG_LOG_EMO(name, "=======================================", 0)
}


int PLAYER::getAmpValue()
{
	double amp_val = ni[ITEM_INTENSITY_AMP] * item[ITEM_INTENSITY_AMP].vals[0];
	double vio_val = ni[ITEM_VIOLENT_FORCE] * item[ITEM_VIOLENT_FORCE].vals[0];
	return ROUNDu(  (amp_val + vio_val)
	              / static_cast<double>(item[ITEM_VIOLENT_FORCE].vals[0]));
}


int PLAYER::getArmourValue()
{
	double arm_val = ni[ITEM_ARMOUR]   * item[ITEM_ARMOUR].vals[0];
	double pla_val = ni[ITEM_PLASTEEL] * item[ITEM_PLASTEEL].vals[0];
	return ROUNDu(  (arm_val + pla_val)
	              / static_cast<double>(item[ITEM_PLASTEEL].vals[0]) );
}


int PLAYER::getBoostValue()
{
	return (getAmpValue() + getArmourValue());
}


/// @brief return the item preference of item @a idx or -1 if @a idx is out of
/// range
/// Note: This uses the static weapPref instead of the adapted currPref,
///       because it is used by AICore for point calculation.
int32_t PLAYER::getItemPref(int32_t idx)
{
	if ( (idx > -1) && (idx < ITEMS) )
		return weapPref[WEAPONS + idx];
	return -1;
}


int32_t PLAYER::getMoneyToSave(bool first_look)
{
	// If this is the first look in a shopping round,
	// the list of items to save money for must be built:
	if (first_look) {
		int32_t avgPref     = 0;
		int32_t prefCount   = 0;
		int32_t prefLimit   = 0;
		memset(saveMoneyFor, 0, sizeof(int32_t) * THINGS);

		// if the preferences are exceptionally low, a div by 0
		// might occur, so it has to be made dynamic:
		for (int32_t i = 0; i < THINGS; ++i) {
			if (currPref[i] > prefLimit) {
				prefLimit += currPref[i];
				prefCount++;
			}
		}

		prefLimit /= prefCount ? prefCount : 1;
		prefCount  = 0;

		// Now it is guaranteed, that prefCount and avgPref
		// will result in values > 0.
		for (int32_t i = 0; i < THINGS; ++i) {
			if (currPref[i] > prefLimit) {
				prefCount++;
				avgPref += currPref[i];
			}
		}

		// Complete the average preference of the most valuable weapons:
		avgPref /= prefCount ? prefCount : 1;

		// Now go through the list and add everything above the
		// average into the save money list, if the amount in stock
		// is too low:
		for (int32_t i = 0; i < THINGS; ++i) {
			int32_t j = i - WEAPONS; // short cut
			if ( (currPref[i] > avgPref)
			  && ( ( (i <  WEAPONS) && (nm[i] < weapon[i].amt) )
			    || ( (j == ITEM_VIOLENT_FORCE) && needAmp)
			    || ( (j == ITEM_PLASTEEL)      && needArmour) ) ) {
				saveMoneyFor[i] = i <  WEAPONS ? weapon[i].cost : item[j].cost;
				DEBUG_LOG_FIN(name, " => Save money for %s!",
				              i <  WEAPONS
				                ? weapon[i].getName()
				                : item[j].getName())
			} // End of having a big enough preference
		} // End of looping THINGS
	} // End of building safe-for-list


	// moneyToSafe can be easily generated (and regenerated)
	// by walking through the list of things currently saved
	// money for:
	double  moneyToSave = 0.;
	double  wanted      = 0.;
	double  max_cost    = 0.; // The most expensive item is counted twice
	for (int32_t i = 0; i < THINGS; ++i) {
		int32_t j = i - WEAPONS; // short cut

		if (saveMoneyFor[i] > 0) {
			// Still needed?
			if ( ( (i <  WEAPONS) && (nm[i] < weapon[i].amt) )
			  || ( (j == ITEM_VIOLENT_FORCE) && needAmp)
			  || ( (j == ITEM_PLASTEEL)      && needArmour) ) {
			  	moneyToSave += saveMoneyFor[i];
			  	wanted      += 1.;
				if (saveMoneyFor[i] > max_cost)
					max_cost = saveMoneyFor[i];
			} else
				// nope...
				saveMoneyFor[i] = 0;
		}
	}

	// If anything is wanted, the most expensive item is counted twice.
	// This is done so the bots do not consider having enough money
	// too early, just like humans would.
	if (max_cost > 1.) {
		moneyToSave += max_cost;
		wanted      += 1.;

		// The average money to save modified by the player type
		// is the result:
		moneyToSave = (moneyToSave / wanted)
		            * (1. + (static_cast<double>(LAST_PLAYER_TYPE - type) / 10.));
	}

	/* Results for Armageddon only @ 100k credits:
	 * (wanted is 1 in this test case)
	 * Useless: 100,000 * (1 + ( (6 - 1) / 10)) = 100,000 * 1.5 = 150,000
	 * Deadly : 100,000 * (1 + ( (6 - 5) / 10)) = 100,000 * 1.1 = 110,000
	 */

	// Whenever moneyToSave is less than the money owned, boostBought is reset
	if (first_look && (money > ROUND(moneyToSave)) ) {
		boostBought  = 0; // Let's go!
		shieldBought = 0;
	}

	return ROUND(moneyToSave);
}


// return the player name
const char* PLAYER::getName () const
{
	return name;
}


// This function checks for incoming data from a client.
// If data is coming in, we put the incoming data in the net_command
// variable. If the socket connection is broken, then we will
// close the socket and hand control over to the AI.
bool PLAYER::getNetCmd()
{
#ifdef NETWORK
	if (Check_For_Incoming_Data(server_socket)) {
		// we have something coming down the pipe
		memset(net_command, '\0', NET_COMMAND_SIZE);    // clear buffer
		size_t status = read(server_socket, net_command, NET_COMMAND_SIZE);
		if (! status) {
			// connection is broken
			close(server_socket);
			type = DEADLY_PLAYER;
			printf("%s lost network connection. Returning control to AI.\n", name);
			return false;
		} else {
			// we got data
			net_command[NET_COMMAND_SIZE - 1] = '\0';
			Trim_Newline(net_command);
		}
	}
#endif // NETWORK
	return true;
}


/** @brief Get one entry of the opponent memory or the last one attacked
  * @param[in] idx Index of the opponent memory to get, or -1 to get the last attacked.
**/
sOpponent* PLAYER::getOppMem(int32_t idx)
{
	// regular memory
	if ( (idx > -1) && (idx < oppCount) )
		return &opponents[idx];

	// or the last attacked
	else if (-1 == idx)
		return last_opponent;

	// or invalid.
	return nullptr;
}


// returns a static string to the player's team name
const char* PLAYER::getTeamName() const
{
	static char team_name[9] = { 0 };

	switch (team) {
		case TEAM_JEDI:
			snprintf(team_name, 8, "%s", "Jedi");
			break;
		case TEAM_NEUTRAL:
			snprintf(team_name, 8, "%s", "Neutral");
			break;
		case TEAM_SITH:
			snprintf(team_name, 8, "%s", "Sith");
			break;
		case TEAM_COUNT:
		default:
			snprintf(team_name, 8, "%s", "* N/A *");
			break;
	}

	return team_name;
}


/// @brief return the weapon preference of weapon @a idx or -1 if @a idx is out
/// of range.
/// Note: This uses the static weapPref instead of the adapted currPref,
///       because it is used by AICore for point calculation.
int32_t PLAYER::getWeapPref(int32_t idx)
{
	if ( (idx > -1) && (idx < WEAPONS) )
		return weapPref[idx];
	return -1;
}


eControl PLAYER::humanControls (AICore* aicore)
{
	bool     moved  = false;
	eControl status = CONTROL_NONE;

	// Keyboard control in aim stage
	if ( (global.stage == STAGE_AIM) && tank) {
		if ( (key[KEY_LEFT] || key[KEY_A])
		  && !ctrlUsedUp && (tank->a < 270) ) {
			if (has_shift_pressed)
				tank->a = std::min(tank->a + 5, 270);
			else
				tank->a++;
			global.updateMenu = 1;
			if (has_ctrl_pressed)
				ctrlUsedUp = true;
		}

		if ( (key[KEY_RIGHT] || key[KEY_D])
		  && !ctrlUsedUp && (tank->a > 90) ) {
			if (has_shift_pressed)
				tank->a = std::max(tank->a - 5, 90);
			else
				tank->a--;
			global.updateMenu = 1;
			if (has_ctrl_pressed)
				ctrlUsedUp = true;
		}

		if ( (key[KEY_DOWN] || key[KEY_S])
		  && !ctrlUsedUp && (tank->p > 0) ) {
			if (has_shift_pressed)
				tank->p = std::max(tank->p - 25, 0);
			else
				tank->p -= 5;
			global.updateMenu = 1;
			if (has_ctrl_pressed)
				ctrlUsedUp = true;
		}

		if ( (key[KEY_UP] || key[KEY_W])
		  && !ctrlUsedUp && (tank->p < MAX_POWER) ) {
			if (has_shift_pressed)
				tank->p = std::min(tank->p + 25, MAX_POWER);
			else
				tank->p += 5;
			global.updateMenu = 1;
			if (has_ctrl_pressed)
				ctrlUsedUp = true;
		}

		if ( (key[KEY_PGUP] || key[KEY_R])
		  && !ctrlUsedUp && (tank->p < MAX_POWER) ) {
			tank->p += 100;
			if (tank->p > MAX_POWER)
				tank->p = MAX_POWER;
			global.updateMenu = 1;
			if (has_ctrl_pressed)
				ctrlUsedUp = true;
		}

		if ( (key[KEY_PGDN] || key[KEY_F])
		  && !ctrlUsedUp && (tank->p > 0) ) {
			tank->p -= 100;
			if (tank->p < 0)
				tank->p = 0;
			global.updateMenu = 1;
			if (has_ctrl_pressed)
				ctrlUsedUp = true;
		}
	}

	// See whether there is a new key press
    if (! k) {
		if ( keypressed() ) {
			k = readkey();
			K = k >> 8;
		}
    }

	// If anything is newly there, make it happen
	if (K) {
		status = CONTROL_OTHER;

		if ( (global.stage == STAGE_AIM) && tank) {
			if (K == KEY_N) {
				tank->a = 180;
				global.updateMenu = 1;
				K = 0;
			}

			if ( (K == KEY_TAB) || (K == KEY_C) ) {
				global.updateMenu = 1;
				bool done = false;
				while (!done) {
					if (++tank->cw >= THINGS)
						tank->cw = 0;

					if ( ( (tank->cw < WEAPONS)
						&& tank->player->nm[tank->cw])
					  || ( (tank->cw >= WEAPONS)
						&& item[tank->cw - WEAPONS].selectable
						&& tank->player->ni[tank->cw - WEAPONS] ) )
						done = true;
				}
				changed_weapon = false;
				K = 0;
			}

			if ( (K == KEY_BACKSPACE) || (K == KEY_Z) ) {
				global.updateMenu = 1;
				bool done = false;
				while (!done) {
					if (--tank->cw < 0)
						tank->cw = THINGS - 1;

					if ( ( (tank->cw < WEAPONS)
						&& tank->player->nm[tank->cw])
					  || ( (tank->cw >= WEAPONS)
						&& item[tank->cw - WEAPONS].selectable
						&& tank->player->ni[tank->cw - WEAPONS] ) )
						done = true;
				}
				changed_weapon = false;
				K = 0;
			}

			// put the tank under computer control
			if (K == KEY_F10) {
				type = PART_TIME_BOT;
				K = 0;
				return (computerControls(aicore, false));
			}

			// move the tank
			if ( (K == KEY_COMMA) || (K == KEY_H) )
				moved = tank->moveTank(DIR_LEFT);
			if ( (K == KEY_STOP) || (K == KEY_J) )
				moved = tank->moveTank(DIR_RIGHT);

			if (moved) {
				global.updateMenu = 1;
				K = 0;
			}

			// Fire Weapon
			if ( (K == KEY_SPACE)
			  && ( ( (tank->cw <  WEAPONS)
				  && (tank->player->nm[tank->cw]))
				|| ( (tank->cw >= WEAPONS)
				  && (tank->player->ni[tank->cw - WEAPONS]) ) ) ) {

				gloating = false;
				status = CONTROL_FIRE;
				changed_weapon = false;
				K = 0;
			}
		} // end of being in aim satge and having a tank
	} // End of havig a key

	return status;
}


void PLAYER::initialise (bool loaded_game)
{
	// Initialize basic values if this is not loaded
	if (!loaded_game) {
		memset(nm, 0, sizeof(int32_t) * WEAPONS);
		memset(ni, 0, sizeof(int32_t) * ITEMS);

		ni[ITEM_FUEL] = 100; // Supply some initial fuel

		kills  = 0;
		killed = 0;
		score  = 0;
	}

	last_opponent = nullptr;
	tank          = nullptr;
}


/// @brief read player data from a dump file.
bool PLAYER::load_from_file (FILE *file)
{
	if (!file)
		return false;

	char  line[MAX_CONFIG_LINE  + 1] = { 0 };
	char  field[MAX_CONFIG_LINE + 1] = { 0 };
	char  value[MAX_CONFIG_LINE + 1] = { 0 };
	char* result                     = nullptr;

	setlocale(LC_NUMERIC, "C");

	// read until we hit line "*PLAYER*" or "***" or EOF
	do {
		result = fgets(line, MAX_CONFIG_LINE, file);
		if ( !result
		  || !strncmp(line, "***", 3) )
			// eof OR end of record
			return false;
	} while ( strncmp(line, "*PLAYER*", 8) );

	bool  done = false;

	while (result && !done) {
		// read a line
		memset(line, '\0', MAX_CONFIG_LINE);
		if ( ( result = fgets(line, MAX_CONFIG_LINE, file) ) ) {

			// if we hit end of the record, stop
			if (! strncmp(line, "***", 3) ) {
				done = true;
				continue; // This exits the loop as well
			}

			// strip newline character
			size_t line_length = strlen(line);
			while ( line[line_length - 1] == '\n') {
				line[line_length - 1] = '\0';
				line_length--;
			}

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

			// check which field we have and process value
			if (!strcasecmp(field, "NAME") )
				strncpy(name, value, NAME_LEN);
			else if (!strcasecmp(field, "COLOR") ) {
				sscanf(value, "%d", &color);
			} else if (!strcasecmp(field, "DEFENSIVE") )
				sscanf(value, "%lf", &defensive);
			else if (!strcasecmp(field, "PAINSENSITIVITY") )
				sscanf(value, "%lf", &painSensitivity);
			else if (!strcasecmp(field, "PLAYED") )
				sscanf(value, "%u", &played);
			else if (!strcasecmp(field, "PREFTYPE") ) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				if ( (val >= 0) && (val <= ALWAYS_PREF))
					preftype = static_cast<playerPrefType>(val);
			} else if (!strcasecmp(field, "SELFPRESERVATION") )
				sscanf(value, "%lf", &selfPreservation);
			else if (!strcasecmp(field, "TANK_BITMAP") )
				sscanf(value, "%d", &tankbitmap);
			else if (!strcasecmp(field, "TEAM") ) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				if ( (val >= 0) && (val <= TEAM_JEDI) )
					team = static_cast<eTeamTypes>(val);
			} else if (!strcasecmp(field, "TYPE") ) {
				int32_t val = 0;
				sscanf(value, "%d", &val);

				if ( (val >= HUMAN_PLAYER) && (val <= LAST_PLAYER_TYPE))
					type = static_cast<playerType>(val);

				// make sure previous human players are restored as humans
				if (type == PART_TIME_BOT)
					type = HUMAN_PLAYER;

			} else if (!strcasecmp(field, "TYPESAVED") ) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				if ( (val >= HUMAN_PLAYER) && (val <= LAST_PLAYER_TYPE)) {
					type_saved = static_cast<playerType>(val);
					if (type_saved > HUMAN_PLAYER)
						type = type_saved;
				}
			} else if (!strcasecmp(field, "VENGEANCETHRESHOLD") ) {
				sscanf(value, "%lf", &vengeanceThreshold);
				// fix old configs
				if (vengeanceThreshold < 0.05)
					vengeanceThreshold = 0.05
									   + (static_cast<double>(rand () % 901)
										  / 1000.); // [0.05;0.95]
				if (vengeanceThreshold > 0.95)
					vengeanceThreshold = 0.95;
			} else if (!strcasecmp(field, "VENGEFUL") ) {
				sscanf(value, "%d", &vengeful);
				// fix old configs
				if (vengeful < 1)
					vengeful = 1 + (rand () % 100); // [1;100]
				if (vengeful > 100)
					vengeful = 100;
			} else if (!strcasecmp(field, "WON") )
				sscanf(value, "%u", &won);
			else if (!strcasecmp(field, "WEAPONPREFERENCES") ) {
				int32_t wp_index = -1;
				int32_t wp_value = -1;
				sscanf(value, "%d %d", &wp_index, &wp_value);
				if ( (wp_index < THINGS) && (wp_index >= 0) )
					weapPref[wp_index] = wp_value;
			} // end of valid data line
		} // end of if we read a line properly
	} // end of while not done

	return true;
}


/** @brief Load player data from @a file which has the @a file_version.
  *
  * Version additions that are not found in earlier versions:
  * <ul>
  * <li>Version 65 : THEFT_BOMB
  * </ul>
  *
  * Version changes from earlier versions:
  * <ul>
  * <li>Version 65 : FUEL needs preferences
  * </ul>
  *
**/
void PLAYER::load_game_data(FILE* file, int32_t file_version)
{
	if (!file)
		return;

	char  line[MAX_CONFIG_LINE  + 1] = { 0 };
	char  field[MAX_CONFIG_LINE + 1] = { 0 };
	char  value[MAX_CONFIG_LINE + 1] = { 0 };
	char* result                     = nullptr;
	bool  done                       = false;
	bool  has_pref_loaded            = false;

	setlocale(LC_NUMERIC, "C");


	do {
		// read a line
		memset(line, '\0', MAX_CONFIG_LINE);
		if ( ( result = fgets(line, MAX_CONFIG_LINE, file) ) ) {

			// if we hit end of the record, stop
			if (! strncmp(line, "***", 3) ) {
				done = true;
				continue; // This exits the loop as well
			}

			// strip newline character
			size_t line_length = strlen(line);
			while ( line[line_length - 1] == '\n') {
				line[line_length - 1] = '\0';
				line_length--;
			}

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

			// check which field we have and process value
			if (!strcasecmp(field, "DEFENSIVE") )
				sscanf(value, "%lf", &defensive);
			else if (!strcasecmp(field, "PAINSENSITIVITY") )
				sscanf(value, "%lf", &painSensitivity);
			else if (!strcasecmp(field, "KILLED") )
                    sscanf(value, "%d", &killed );
			else if (!strcasecmp(field, "KILLS") )
                    sscanf(value, "%d", &kills );
			else if (!strcasecmp(field, "MONEY") )
                    sscanf(value, "%d", &money );
			else if (!strcasecmp(field, "SCORE") )
                    sscanf(value, "%d", &score );
			else if (!strcasecmp(field, "SELFPRESERVATION") )
				sscanf(value, "%lf", &selfPreservation);
			else if (!strcasecmp(field, "TYPE") ) {
				int32_t val = 0;
				sscanf(value, "%d", &val );
				if ( (val >= HUMAN_PLAYER) && (val < LAST_PLAYER_TYPE) )
					type = static_cast<playerType>(val);
			} else if (!strcasecmp(field, "TYPESAVED") ) {
				int32_t val = 0;
				sscanf(value, "%d", &val );
				if ( (val >= HUMAN_PLAYER) && (val < LAST_PLAYER_TYPE) )
					type_saved = static_cast<playerType>(val);
			} else if (!strcasecmp(field, "VENGEANCETHRESHOLD") ) {
				sscanf(value, "%lf", &vengeanceThreshold);
				// fix old configs
				if (vengeanceThreshold < 0.05)
					vengeanceThreshold = 0.05
									   + (static_cast<double>(rand () % 901)
										  / 1000.); // [0.05;0.95]
				if (vengeanceThreshold > 0.95)
					vengeanceThreshold = 0.95;
			} else if (!strcasecmp(field, "VENGEFUL") ) {
				sscanf(value, "%d", &vengeful);
				// fix old configs
				if (vengeful < 1)
					vengeful = 1 + (rand () % 100); // [1;100]
				if (vengeful > 100)
					vengeful = 100;
			}

			// Preferences - saved if "PERPLAY_PREF" - type player.
			else if (!strcasecmp(field, "WEAPONPREFERENCES")) {
				int32_t prf_idx = -1;
				int32_t prf_val = -1;
				sscanf(value, "%d %d", &prf_idx, &prf_val);
				if ( (prf_idx > -1) && (prf_idx < THINGS) ) {

					/* === Version Checks for new weapons / items === */

					if ( (file_version < 65) && (prf_idx >= THEFT_BOMB) ) {
						if (THEFT_BOMB == prf_idx) {
							// Generate a value
							weapPref[THEFT_BOMB] = (150. + vengeful)
							                     * (static_cast<double>(type) / 2. + .5)
						                         * ( (selfPreservation + 2.) / 2.)
						                         * (std::abs(defensive) + 1.0);
							DEBUG_LOG_EMO(name, "New preference for %s : %5d",
							              weapon[THEFT_BOMB].getName(),
							              weapPref[THEFT_BOMB])
						}
						++prf_idx; // Skip new index value
					} // End of version 65 THEFT_BOMB

					/* === Version Checks for changed weapons / items === */

					if ( (file_version < 65) ) {
						if ( (ITEM_FUEL == (prf_idx - WEAPONS))
						  && (prf_val < 1) ) {
							// Generate a value
							prf_val = static_cast<double>(MAX_WEAP_PROBABILITY)
							        / 60.
							        * ( static_cast<double>(type) / 2. + .5);

							DEBUG_LOG_EMO(name, "Changed preference for %s : %5d",
							              item[ITEM_FUEL].getName(),
							              prf_val)
						}
					} // End of version 65 ITEM_FUEL

					/* === Store data === */
					// (If someone edited the save game, the index might
					//  be too larger now, so check again to be safe!)
					if (prf_idx < THINGS)
						weapPref[prf_idx] = prf_val;

					// separate very old from new save games
					has_pref_loaded   = true;
				}
			}

			// Inventory of the weapons
			else if (!strcasecmp(field, "WEAPON")) {
				int32_t weap_idx = -1;
				int32_t weap_val = -1;
				sscanf(value, "%d %d", &weap_idx, &weap_val);
				if ( (weap_idx > -1) && (weap_idx < WEAPONS) ) {

					/* === Version Checks for new weapons === */

					if ( (file_version < 65) && (weap_idx >= THEFT_BOMB) )
						++weap_idx; // Skip new index value

					/* === Store data === */
					// (If someone edited the save game, the index might
					//  be too larger now, so check again to be safe!)
					if (weap_idx < WEAPONS)
						nm[weap_idx] = weap_val;
				}
			}

			// Inventory of the items
			else if (!strcasecmp(field, "ITEM")) {
				int32_t item_idx = -1;
				int32_t item_val = -1;
				sscanf(value, "%d %d", &item_idx, &item_val);
				if ( (item_idx > -1) && (item_idx < ITEMS) ) {

					/* === Version Checks for new weapons === */
					// Currently there are no new items.

					ni[item_idx] = item_val;
				}
			}

			// Opponents Memory
			else if (!strcasecmp(field, "OPPCOUNT") ) {
				int32_t safed_count = 0;
				sscanf(value, "%d", &safed_count );

				// prepare the memory
				if (opponents) {
					delete [] opponents;
					opponents = nullptr;
				}

				if (safed_count) {
					try {
						oppCount  = safed_count;
						opponents = new opp_t[oppCount];
					} catch (std::exception &e) {
						cerr << "ERROR: Unable to allocate ";
						cerr << (sizeof(opp_t) * oppCount);
						cerr << " bytes for opponents array!" << endl;
						cerr << "ERROR: " << e.what() << endl;
						oppCount = 0;
					}
				} else
					oppCount = 0;
			} // end of oppcount handling
			else if (!strcasecmp(field, "OPPMEM_INDX") ) {
				int32_t opp_idx = -1;
				int32_t opp_val = -1;
				sscanf(value, "%d %d", &opp_idx, &opp_val);
				if ( (opp_idx > -1) && (opp_idx < oppCount) ) {
					opponents[opp_idx].index    = opp_val;
					opponents[opp_idx].opponent = env.allPlayers[opp_val];
				}
			} else if (!strcasecmp(field, "OPPMEM_DDEA") ) {
				int32_t opp_idx = -1;
				int32_t opp_val = -1;
				sscanf(value, "%d %d", &opp_idx, &opp_val);
				if ( (opp_idx > -1) && (opp_idx < oppCount) )
					opponents[opp_idx].damage_from = opp_val;
			} else if (!strcasecmp(field, "OPPMEM_DDON") ) {
				int32_t opp_idx = -1;
				int32_t opp_val = -1;
				sscanf(value, "%d %d", &opp_idx, &opp_val);
				if ( (opp_idx > -1) && (opp_idx < oppCount) )
					opponents[opp_idx].damage_to = opp_val;
			} else if (!strcasecmp(field, "OPPMEM_FEAR") ) {
				int32_t opp_idx = -1;
				double  opp_val = 0.;
				sscanf(value, "%d %lf", &opp_idx, &opp_val);
				if ( (opp_idx > -1) && (opp_idx < oppCount) )
					opponents[opp_idx].fear = opp_val;
			} else if (!strcasecmp(field, "OPPMEM_KIME") ) {
				int32_t opp_idx = -1;
				int32_t opp_val = -1;
				sscanf(value, "%d %d", &opp_idx, &opp_val);
				if ( (opp_idx > -1) && (opp_idx < oppCount) )
					opponents[opp_idx].killed_me = opp_val;
			} else if (!strcasecmp(field, "OPPMEM_KITH") ) {
				int32_t opp_idx = -1;
				int32_t opp_val = -1;
				sscanf(value, "%d %d", &opp_idx, &opp_val);
				if ( (opp_idx > -1) && (opp_idx < oppCount) )
					opponents[opp_idx].killed_them = opp_val;
			}


		} // End of having a line
	} while (result && !done);
	// End of reading player section


	// For backwards compatibility the preferences must be generated
	// if this is a PERPLAY type player but no preferences got saved.
	// This might be the case for very old save games.
	if (!has_pref_loaded
	  && (PERPLAY_PREF == preftype)
	  && (type != HUMAN_PLAYER) )
		generatePreferences();
}


/// @brief reserve memory for the opponents array and fill it
void PLAYER::newGame()
{
	if (env.numGamePlayers) {

		if (opponents) {
			delete [] opponents;
			opponents = nullptr;
		}

		try {
			oppCount  = env.numGamePlayers;
			opponents = new opp_t[oppCount];
		} catch (std::exception &e) {
			cerr << "ERROR: Unable to allocate " << (sizeof(opp_t) * oppCount);
			cerr << " bytes for opponents array!" << endl;
			cerr << "ERROR: " << e.what() << endl;
			oppCount  = 0;
		}
	}

	if (oppCount) {
		for (int32_t i = 0; i < oppCount; ++i) {
			opponents[i].opponent = env.players[i];
			opponents[i].index    = env.players[i]->index;
		}
	}
}


// run this at the beginning of each turn
void PLAYER::newRound()
{
	// if the player is under computer control, give it back to the player
	if ( type == PART_TIME_BOT )
		type = HUMAN_PLAYER;

	if (!tank) {
		try {
			tank = new TANK();
			tank->player = this;
		} catch (std::exception &e) {
			cerr << "FATAL: Error allocating memory for TANK in player.cpp:";
			cerr << __LINE__ << " : " << e.what() << endl;
			global.set_command(GLOBAL_COMMAND_QUIT);
		}
	}
	// tank->newRound() doesn't need to be called, because
	// the game loop will do that on tank placement.

	// if we are playing in a campaign, raise the AI level for every 20% played
	// rounds, so that useless players become deadly at 80% played rounds
	if (env.campaign_mode
	  && (global.currentround < env.nextCampaignRound)
	  && (type > HUMAN_PLAYER)
	  && (type < DEADLY_PLAYER) )
		++type;

	// reset some basic values
	changed_weapon    = false;
	time_left_to_fire = env.maxFireTime;
	skip_me           = false;
	last_shield_used  = 0;

	// Save damage from opponents if there was some not processed.
	// Although this would be done automatically once the AI takes
	// this player over the next time, lingering damage from the
	// last round can lead to panic actions and/or revenge actions
	// against players, who haven't fired, yet.
	// Noting the damage will raise the probability, but only once
	// the opponent had their first shot.
	for (int32_t i = 0; i < oppCount; ++i) {
		if (opponents[i].damage_last > 0) {
			opponents[i].damage_from += opponents[i].damage_last;
			opponents[i].damage_last  = 0;
		}
	}
}


void PLAYER::noteDamageFrom(PLAYER* opponent, int32_t damage, bool destroyed)
{
	if (opponent) {
		int32_t idx       = oppCount;
		int32_t max_score = 0;

		for (int32_t i = 0; i < oppCount; ++i) {
			if (opponents[i].revenge_dmg > max_score)
				max_score = opponents[i].revenge_dmg;
			if (opponents[i].opponent == opponent)
				idx = i;
		}

		if ( idx < oppCount ) {
			opponents[idx].damage_last += damage;
			if (destroyed)
				opponents[idx].killed_me++;

			// If this one has the new top score and is not
			// the current revenge player, get a message out
			int32_t rev_dmg = opponents[idx].damage_last
			                + opponents[idx].revenge_dmg;

			if ( (opponents[idx].opponent != this)
			  && (opponents[idx].opponent != revenge)
			  && (rev_dmg > (vengeanceThreshold * tank->getMaxLife()))
			  && (rev_dmg > max_score) ) {

				revenge = opponents[idx].opponent;

				if (!global.skippingComputerPlay ) {
					try {
						new FLOATTEXT(selectRevengePhrase(),
						              tank->x, tank->y - 30, .0, -.4, color,
						              CENTRE, TS_NO_SWAY, 300, false);
					} catch (...) {
						perror ( "player.cpp: Failed to allocate memory for"
								 " revenge text in noteDamageFrom().");
					}
				}
			}
		} // end of having the opponent
	} // end of having any opponent
}


void PLAYER::noteDamageTo(PLAYER* opponent, int32_t damage, bool destroyed)
{
	if (opponent) {
		int32_t idx = 0;

		while ((idx < oppCount) && (opponent != opponents[idx].opponent))
			++idx;

		if ( idx < oppCount ) {
			opponents[idx].damage_to += damage;
			if (destroyed)
				opponents[idx].killed_them++;
		}
	}
}


// if we have some shield strength at the end of the round, then
// reclaim this shield back into our inventory
void PLAYER::reclaimShield()
{
    if (tank && last_shield_used && (tank->sh > 0))
		ni[last_shield_used] += 1;
	last_shield_used = 0;
}


// This function takes one off the player's time to fire.
// If the player runs out of time, the function returns true.
// If the player has time left, or no time clock is being used,
// then the function returns false.
bool PLAYER::reduceClock()
{
	if (! time_left_to_fire)
		// not using clock
		return false;

	if (0 == --time_left_to_fire) {
		time_left_to_fire = env.maxFireTime;
		return true;
	}

	return false;
}


/// @brief save game relevant data to @a file
void PLAYER::save_game_data(FILE* file)
{
	fprintf(file, "KILLED=%d\n", killed);
	fprintf(file, "KILLS=%d\n", kills);
	fprintf(file, "MONEY=%d\n", money);
	fprintf(file, "SCORE=%d\n", score);
	fprintf(file, "TYPE=%d\n", type);
	fprintf(file, "TYPESAVED=%d\n", type_saved);

	// Preferences, needed for "PERPLAY_PREF" - players
	if ( (PERPLAY_PREF == preftype) && (HUMAN_PLAYER != type) ) {
		// Note: "ALWAYS_PREF" - players do not need this here, but in
		// save_to_file(), as the preferences are generated only once.
		fprintf(file, "DEFENSIVE=%lf\n", defensive);
		fprintf(file, "PAINSENSITIVITY=%lf\n", painSensitivity);
		fprintf(file, "SELFPRESERVATION=%lf\n", selfPreservation);
		fprintf(file, "VENGEANCETHRESHOLD=%lf\n", vengeanceThreshold);
		fprintf(file, "VENGEFUL=%d\n", vengeful);
		for (int32_t i = 0; i < THINGS; ++i)
			fprintf (file, "WEAPONPREFERENCES=%d %d\n", i, weapPref[i]);
	}

	// Inventory of the weapons
	for (int32_t i = 0; i < WEAPONS; ++i)
		fprintf(file, "WEAPON=%d %d\n", i, nm[i]);

	// Inventory of the items
	for (int32_t i = 0; i < ITEMS; ++i)
		fprintf(file, "ITEM=%d %d\n", i, ni[i]);

	// Opponents memory
	fprintf(file, "OPPCOUNT=%d\n", oppCount);
	for (int32_t i = 0; i < oppCount; ++i) {
		int32_t idx = opponents[i].index; // Just a shortcut

		// Save damage from last turn if any is still there:
		if (opponents[i].damage_last > 0) {
			opponents[i].damage_from += opponents[i].damage_last;
			opponents[i].damage_last  = 0;
		}
		fprintf(file, "OPPMEM_INDX=%d %d\n",  i, idx);
		fprintf(file, "OPPMEM_DDEA=%d %d\n",  i, opponents[i].damage_from);
		fprintf(file, "OPPMEM_DDON=%d %d\n",  i, opponents[i].damage_to);
		fprintf(file, "OPPMEM_FEAR=%d %lf\n", i, opponents[i].fear);
		fprintf(file, "OPPMEM_KIME=%d %d\n",  i, opponents[i].killed_me);
		fprintf(file, "OPPMEM_KITH=%d %d\n",  i, opponents[i].killed_them);
	}

	fprintf(file, "***\n");
}


/// @brief dump full player data to @a file
void PLAYER::save_to_file (FILE *file)
{
	if (! file)
		return;

	// start section with "*PLAYER*"
	fprintf (file, "*PLAYER*\n");
	fprintf (file, "NAME=%s\n", name); // Set first for easier debugging
	fprintf (file, "COLOR=%d\n", color);
	fprintf (file, "DEFENSIVE=%lf\n", defensive);
	fprintf (file, "PAINSENSITIVITY=%lf\n", painSensitivity);
	fprintf (file, "PLAYED=%u\n", played);
	fprintf (file, "PREFTYPE=%d\n", preftype);
	fprintf (file, "SELFPRESERVATION=%lf\n", selfPreservation);
	fprintf (file, "TANK_BITMAP=%d\n", tankbitmap);
	fprintf (file, "TEAM=%d\n", team);
	fprintf (file, "TYPE=%d\n", type);
	fprintf (file, "TYPESAVED=%d\n", type_saved);
	fprintf (file, "VENGEANCETHRESHOLD=%lf\n", vengeanceThreshold);
	fprintf (file, "VENGEFUL=%d\n", vengeful);
	fprintf (file, "WON=%u\n", won);

	// Preferences, needed for "ALWAYS_PREF" - players
	if (ALWAYS_PREF == preftype) {
		// Note: "PERPLAY_PREF" - players do not need this here, but in
		// save_game_data(), as the preferences are different in each game.
		for (int32_t i = 0; i < THINGS; ++i)
			fprintf (file, "WEAPONPREFERENCES=%d %d\n", i, weapPref[i]);
	}

	fprintf (file, "***\n");
}


const char* PLAYER::selectGloatPhrase ()
{
	return env.gloat->Get_Random_Line();
}


/// @return a constructed panic phrase which must be freed!
const char *PLAYER::selectPanicPhrase (PLAYER* shocker)
{
	if (! shocker)
		return nullptr;

	const char* line  = env.panic->Get_Random_Line();
	size_t      tLen  = strlen(shocker->getName()) + strlen(line);
	char*       pText = (char *)calloc(tLen + 1, sizeof (char));

	if (!pText)
		return nullptr;

	snprintf(pText, tLen, line, shocker->getName());

	return pText;
}


const char *PLAYER::selectKamikazePhrase ()
{
	return env.kamikaze->Get_Random_Line();
}


/// @return a constructed retaliation phrase which must be freed!
const char *PLAYER::selectRetaliationPhrase ()
{
	if (! revenge)
		return nullptr;

	const char* line  = env.retaliation->Get_Random_Line();
	const char* rname = revenge->getName();
	size_t      tLen  = strlen(rname) + 4 + strlen(line);
	char*       pText = (char *)calloc(tLen + 1, sizeof (char));

	if (pText)
		atanks_snprintf(pText, tLen, "%s%s !!!", line, rname);

	return pText;
}


const char* PLAYER::selectRevengePhrase ()
{
	return env.revenge->Get_Random_Line();
}


const char *PLAYER::selectSuicidePhrase ()
{
	return env.suicide->Get_Random_Line();
}


/// @brief store @a last_opp to be remembered as the current/last target
void PLAYER::setLastOpponent(sOpponent* last_opp)
{
	last_opponent = last_opp;
}


void PLAYER::setName (const char* name_)
{
	if (!name_ || strncmp(name, name_, NAME_LEN - 1)) {
		memset(name, 0, NAME_LEN);
		if (name_)
			strncpy (name, name_, NAME_LEN - 1);
	}
}


/// @brief fill in the list of desired items and update their preferences
void PLAYER::updatePreferences(int32_t max_boost, int32_t max_score)
{
	// 1.: Fill cart and preference array.
	// The preferences are copied, as they might get boosted this round
	int32_t weapons_in_stock = generateDesiredList();
	int32_t ai_level         = static_cast<int32_t>(type);

	// 2.: Amplify wish list by current boost and score situation
	needAmp    = false;
	needArmour = false;
	needDamage = false;

	// Check whether boosting armour / amps is wanted:
	if (getBoostValue() < (max_boost / ai_level)) {
		// Yes. which ?
		if (defensive < 0.) {
			DEBUG_LOG_FIN(name, "updPref: Need to boost amps    (%d / %d)",
						  getBoostValue(), max_boost / ai_level)
			needAmp   = true; // Try to come back with more damage output
		} else {
			DEBUG_LOG_FIN(name, "updPref: Need to boost armour  (%d / %d)",
						  getBoostValue(), max_boost / ai_level)
			needArmour = true; // Try to come back with more endurance
		}
	}

	// Fallen behind? Need more weapons?
	if ( (score <= (max_score / (ai_level + 1)))
	  && (weapons_in_stock < (2 * ai_level)) ) {
		DEBUG_LOG_FIN(name, "updPref: Need to boost weapons (%d / %d)",
					  score, max_score / (ai_level + 1))
		needDamage = true;
	}


	// 3.: Boost preferences if wanted and lower weapon/item
	//     preferences if there are enough in stock already.
	//     Further note down items to sell.
	boostPrefences(needArmour, needAmp, needDamage);


	// 4.: Sort these items by preferences
	bool isSorted = false;
	while (!isSorted) {
		isSorted = true;

		for (int32_t i = 1; i < THINGS; ++i) {
			int32_t idx_l  = desired[i - 1];
			int32_t idx_r  = desired[i];

			if ( (currPref[idx_l] < currPref[idx_r])
				// sort SML_MIS to the back, too
			  || ( (0 == idx_l) && idx_r) ) {
				isSorted       = false;
				desired[i]     = idx_l;
				desired[i - 1] = idx_r;
			}
		}
	}

#ifdef ATANKS_DEBUG_FINANCE
	// Get out the top twenty
	for (int32_t i = 0; i < THINGS; ++i) {
		DEBUG_LOG_FIN(name, "%2d. preference: %6d - %s",
					  i + 1, currPref[desired[i]],
					  desired[i] < WEAPONS
					  ? weapon[desired[i]].getName()
					  : item[desired[i] - WEAPONS].getName())
	}
#endif // ATANKS_DEBUG_FINANCE
}


/// @brief mini ctor to pacify Visual C++
PLAYER_mini::PLAYER_mini()
{
	memset(name, 0, sizeof(char) * NAME_LEN);
	strncpy(name, "New Player", NAME_LEN);
}


/// @brief backup a players editable data
void PLAYER_mini::copy_from(PLAYER* source)
{
	if (source) {
		assert( (source->index > -1) && "INDEX ERROR on PLAYER!");
		color       = source->color;
		index       = source->index;
		strncpy(name, source->getName(), NAME_LEN);
		played      = source->played;
		player      = source;
		preftype    = source->preftype;
		tankbitmap = source->tankbitmap;
		team        = source->team;
		type        = source->type;
		won         = source->won;
	}
}


/// @brief copy backed up values back to the source player
void PLAYER_mini::write_back(PLAYER* target)
{
	if (target)
		player = target;
	if (player) {
		player->color = color;
		player->setName(name);
		// played is read only.
		player->preftype = preftype;
		player->tankbitmap = tankbitmap;
		player->team = team;
		player->type = type;
		// won is read only.
	}
}



/// @brief action function to display the edit player screen
int32_t edit_player(PLAYER** target, int32_t)
{
	int32_t result = 0;

	assert(target && "ERROR: target must be set");
	assert(*target && "ERROR: target must point to something valid!");

	if (!target || !(*target))
		return -1;

	int32_t menuMid        = 300;
	int32_t itemLeft       = menuMid - 75;
	int32_t itemHeight     = env.fontHeight + 2;
	int32_t itemPadding    = 2;
	int32_t itemFullHeight = itemHeight + itemPadding;
	int32_t itemY          = itemFullHeight * 3;
	int32_t btnHeight      = env.misc[7]->h + itemPadding;
	int32_t menuHeight     = env.menuEndY - env.menuBeginY; // Raw height


	// Use "Mini-Player" struct to be able to cancel player editing
	PLAYER_mini player_bak;
	player_bak.copy_from(*target);

	// The "Are you sure" screen when deleting a player
	Menu areyousure(MC_AREYOUSURE,
					env.halfWidth - menuMid, env.menuBeginY);
	areyousure.addButton( 1, nullptr, PE_CONFIRM_DEL,
						env.misc[7], nullptr, env.misc[8], false,
						menuMid + 50,
						menuHeight- btnHeight - 6, 0, 0, itemPadding);
	areyousure.addButton( 2, nullptr, PE_BACK,
						env.misc[7], nullptr, env.misc[8], false,
						menuMid - env.misc[7]->w - 50,
						menuHeight- btnHeight - 6, 0, 0, itemPadding);

	// The menu, but with the player name as title
	Menu menu(MC_PLAYER, env.halfWidth - menuMid, env.menuBeginY);
	menu.setTitle(player_bak.name, false);

	// "Name"
	menu.addText(player_bak.name, 1, NAME_LEN, player_bak.color, "%s",
				itemLeft, itemY, 150, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Colour"
	menu.addColor(&player_bak.color, 2, itemLeft, itemY, 150, 50, 25, itemPadding);
	itemY += 50 + itemPadding;

	// "Type"
	menu.addValue(&player_bak.type, 3, nullptr, BLACK,
				TC_PLAYERTYPE, static_cast<int32_t>(DEADLY_PLAYER),
				itemLeft, itemY, 150, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Team"
	menu.addValue(&player_bak.team, 4, nullptr, BLACK,
				TC_PLAYERTEAM, static_cast<int32_t>(TEAM_JEDI),
				itemLeft, itemY, 150, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Generate Pref"
	menu.addValue(&player_bak.preftype, 5, nullptr, BLACK,
				TC_PLAYERPREF, static_cast<int32_t>(ALWAYS_PREF),
				itemLeft, itemY, 150, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Played"
	menu.addText(&player_bak.played, 6, BLACK, "% 8u",
				itemLeft, itemY, 150, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Won"
	menu.addText(&player_bak.won, 7, BLACK, "% 8u",
				itemLeft, itemY, 150, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Tank Type"
	menu.addValue(&player_bak.tankbitmap, 8, nullptr, BLACK,
				TC_TANKTYPE, static_cast<int32_t>(TT_MINI),
				itemLeft, itemY, 150, 35, itemPadding,
				display_tank_desc);
	itemY += 35 + itemPadding;

	// "Delete This Player"
	menu.addMenu(&areyousure, 9, RED, itemLeft, itemY, 150, itemFullHeight, itemPadding);

	// "Okay" and "Back"
	menu.addButton(10, nullptr, PE_CONFIRM_EDIT, env.misc[7], nullptr,
						env.misc[8], false,
						menuMid + 50,
						menuHeight- btnHeight - 6, 0, 0, itemPadding);
	menu.addButton(11, nullptr, PE_BACK, env.misc[7], nullptr,
						env.misc[8], false,
						menuMid - env.misc[7]->w - 50,
						menuHeight- btnHeight - 6, 0, 0, itemPadding);

	result = menu();

	// If the editing is confirmed, the backup must be written back
	if (PE_CONFIRM_EDIT & result)
		player_bak.write_back();

	// If the player shall be deleted, the player index must be added
	if (PE_CONFIRM_DEL & result)
		result |= player_bak.index;

	return result;
}

static PLAYER_mini player_new; //!< Used by new_player to keep previous settings

/// @brief action function to display the edit player screen
int32_t new_player(PLAYER** target, int32_t)
{
	int32_t result = 0;

	assert(target && "ERROR: target must be set");
	assert((nullptr == *target)
		  && "ERROR: *target must nullptr!");

	if (!target || *target)
		return -1;

	int32_t menuMid        = 300;
	int32_t itemLeft       = menuMid - 75;
	int32_t itemHeight     = env.fontHeight + 2;
	int32_t itemPadding    = 2;
	int32_t itemFullHeight = itemHeight + itemPadding;
	int32_t itemY          = itemFullHeight * 3;
	int32_t btnHeight      = env.misc[7]->h + itemPadding;
	int32_t menuHeight     = env.menuEndY - env.menuBeginY; // Raw height

	// The menu, with title from the menu class
	Menu menu(MC_PLAYER, env.halfWidth - menuMid, env.menuBeginY);

	// "Name"
	menu.addText(player_new.name, 1, NAME_LEN, player_new.color, "%s",
				itemLeft, itemY, 150, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Colour"
	menu.addColor(&player_new.color, 2, itemLeft, itemY, 150, 50, 25, itemPadding);
	itemY += 50 + itemPadding;

	// "Type"
	menu.addValue(&player_new.type, 3, nullptr, BLACK,
				TC_PLAYERTYPE, static_cast<int32_t>(DEADLY_PLAYER),
				itemLeft, itemY, 150, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Team"
	menu.addValue(&player_new.team, 4, nullptr, BLACK,
				TC_PLAYERTEAM, static_cast<int32_t>(TEAM_JEDI),
				itemLeft, itemY, 150, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Generate Pref"
	menu.addValue(&player_new.preftype, 5, nullptr, BLACK,
				TC_PLAYERPREF, static_cast<int32_t>(ALWAYS_PREF),
				itemLeft, itemY, 150, itemHeight, itemPadding);
	itemY += itemFullHeight;

	// "Played" and "Won" do not make sense here

	// "Tank Type"
	menu.addValue(&player_new.tankbitmap, 8, nullptr, BLACK,
				TC_TANKTYPE, static_cast<int32_t>(TT_MINI),
				itemLeft, itemY, 150, 35, itemPadding,
				display_tank_desc);
	itemY += 35 + itemPadding;

	// "Delete This Player" is surely not needed

	// "Okay" and "Back"
	menu.addButton(10, nullptr, PE_CONFIRM_NEW, env.misc[7], nullptr,
						env.misc[8], false,
						menuMid + 50,
						menuHeight- btnHeight - 6, 0, 0, itemPadding);
	menu.addButton(11, nullptr, PE_BACK, env.misc[7], nullptr,
						env.misc[8], false,
						menuMid - env.misc[7]->w - 50,
						menuHeight- btnHeight - 6, 0, 0, itemPadding);

	while (!result) {
		char existsMessage[200];
		result = menu();

		// If the player is to be created, two things must happen.
		// First, ensure that the name is unique
		// Second, create the real player
		if (PE_CONFIRM_NEW & result) {
            if (-1 == env.getPlayerByName(player_new.name)) {
				*target = env.createNewPlayer(player_new.name);
				if (*target)
					player_new.write_back(*target);
            } else {
				snprintf(existsMessage, 199, "The player \"%s\" already exists!",
						player_new.name);
				errorMessage = existsMessage;
				errorX = env.halfWidth - text_length(font, errorMessage) / 2;
				errorY = env.menuBeginY + itemFullHeight;
				result = 0;
            }
		}
	} // End of !result

	return result;
}
