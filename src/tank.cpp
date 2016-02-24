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


#include "floattext.h"
#include "explosion.h"
#include "teleport.h"
#include "missile.h"
#include "player.h"
#include "beam.h"
#include "tank.h"
#include "sound.h"

#include <cassert> // For some checks if _DEBUG is defined.
#include <mutex>   // Needed for lock_guard

TANK::TANK () :
	PHYSICAL_OBJECT(false),
	healthText(nullptr, -1, -1, 0., 0., WHITE,     CENTRE, TS_NO_SWAY, -1, true),
	nameText(  nullptr, -1, -1, 0., 0., WHITE,     CENTRE, TS_NO_SWAY, -1, true),
	shieldText(nullptr, -1, -1, 0., 0., TURQUOISE, CENTRE, TS_NO_SWAY, -1, true)
{
	// The shield phase delta depends on currently set FPS
	shld_delta /= static_cast<double>(env.frames_per_second);

	setTextPositions(false);

	drag  = 0.5;
	mass  = 3000;
	a    += rand () % 180;

	// Add to the chain:
	global.addObject(this);
	global.numTanks++;
}


/*
The destructor only removes the tank and cleans up after it.
Any tank destruction (with big badass explosions, vengeance and stuff) is done
in TANK::explode().
*/
TANK::~TANK ()
{
	if (player) {
		player->tank = nullptr;
		player       = nullptr;
	}
	creditTo = nullptr;

	global.numTanks--;
	if (global.get_curr_tank() == this)
		global.set_curr_tank(nullptr);

	// Take out of the chain:
	global.removeObject(this);
}


/// @brief Set texts to vertical bounce and initialize text positions
void TANK::activate()
{
	shieldText.set_sway_type(TS_VERTICAL);
	healthText.set_sway_type(TS_VERTICAL);
	nameText.set_sway_type(TS_VERTICAL);

	setTextPositions(true);
}


/// @brief activate (aka "fire" or "shoot") whatever is selected right now.
/// Use this whenever a weapon is fired without the player hitting the trigger.
/// For the trigger slamming use simActivateCurrentSelection().
void TANK::activateCurrentSelection()
{
	// avoid firing weapons on exit in Windows
	if ( (global.get_command() == GLOBAL_COMMAND_QUIT)
	  || (global.get_command() == GLOBAL_COMMAND_MENU) )
		return;

	// This must not be called outside fire stage.
	assert( (STAGE_FIRE == global.stage)
			&& "ERROR: TANK::activateCurrentSelection() called outside STAGE_FIRE!");
	if (STAGE_FIRE != global.stage)
		return;

	// remove status from top bar at next redraw
	if (global.tank_status)
		global.tank_status[0] = 0;

	// reduce time to fall, but reset if already done
	if (--env.time_to_fall < 0)
		env.time_to_fall = (rand() % env.landSlideDelay) + 1;

	/** ==============================
	  * === Case 1 : Fire a weapon ===
	  * ==============================
	**/
	if (cw < WEAPONS) {
		player->changed_weapon = false;
		if (cw)
			player->nm[cw]--;

		// --- Ballistics ---
		//--------------------
		if (cw < BALLISTICS) {
			play_fire_sound(cw, x, 255, 1000);

			// Loop over spread, that covers everything
			for (int32_t z = 0; z < weapon[cw].spread; ++z) {
				int32_t ca = a + ((SPREAD * z) - (SPREAD * (weapon[cw].spread - 1) / 2));
				double  dp = static_cast<double>(p); // Just a shortcut against further casts

				// power must not be zero if this is a riot charge/blast.
				// If it were, the calculation of the triangle goes nuts and
				// sends the charge downwards, regardless of the set angle.
				/// @todo : Fix the calculation so this extra check becomes obsolete
				if ( (dp < 100.) && (RIOT_CHARGE <= cw) && (RIOT_BLAST >= cw) )
					dp = 100.;

				double mxv = env.slope[ca][0] * dp * env.FPS_mod / 100.;
				double myv = env.slope[ca][1] * dp * env.FPS_mod / 100.;

				try {
					MISSILE* newmis = new MISSILE(player,
					                   x + (env.slope[ca][0] * turr_off_x),
									   y + (env.slope[ca][1] * turr_off_x),
									   mxv, myv, cw, MT_WEAPON, 1, 0);

					// set up / check volley
					if (weapon[cw].delay && (0 == fire_another_shot) )
						fire_another_shot = weapon[cw].delay * env.volley_delay;

					// Adapt missile drag if the player has dimpled/slick projectiles
					if (player->ni[ITEM_DIMPLEP]) {
						player->ni[ITEM_DIMPLEP]--;
						newmis->drag *= item[ITEM_DIMPLEP].vals[0];
					} else if (player->ni[ITEM_SLICKP]) {
						player->ni[ITEM_SLICKP]--;
						newmis->drag *= item[ITEM_SLICKP].vals[0];
					}
				} catch (...)  {
					perror ( "tank.cpp: Failed to allocate memory for new"
							 " missile in TANK::activateCurrentSelection()");
				}

			}
		} // End of ballistics

		// --- Beam weapons ---
		//----------------------
		else {
			try {
				new BEAM (player, x + (env.slope[a][0] * turr_off_x),
                                  y + (env.slope[a][1] * turr_off_x),
                                  a, cw, BT_WEAPON);
			} catch (...) {
                  perror ( "tank.cpp: Failed to allocate memory for new"
						   " beam in TANK::activateCurrentSelection()");
			}
		}
    } // End of weapons

	/** =================================
	  * === Case 2 : Activate an item ===
	  * =================================
	**/
	else {
		int32_t ci = cw - WEAPONS; // [c]urrent [i]tem

		// If this is not a vengeance item, take it out of the inventory.
		// Note: Vengeance items are reduced in explode() if triggered.
		if ( (ITEM_VENGEANCE > ci) || (ITEM_FATAL_FURY < ci) )
			player->ni[ci]--;

		// --- Teleport ---
		//-----------------
		if (ITEM_TELEPORT == ci) {
			int32_t right  = env.screenWidth  - (tank_dia * 2);
			int32_t bottom = env.screenHeight - (tank_dia * 2) - MENUHEIGHT;
			int32_t new_x  = (rand() % right ) + tank_dia;
			int32_t new_y  = (rand() % bottom) + tank_dia + MENUHEIGHT;

			// Be sure the tank does not end up too high in the sky
			// or too deeply buried.
			int32_t surf_y = global.surface[new_x].load(ATOMIC_READ);
			if (new_y < (surf_y - 150) )
				new_y = surf_y - 150;
			else if (new_y > (surf_y + 100) )
				new_y = surf_y + 100;

			try {
				new TELEPORT (this, new_x, new_y, tank_dia, 120, ci);
				addDamage(player, 0.); // Fall is a self hit.
				isTeleported = true;
			} catch(...) {
				perror ( "tank.cpp: Failed to allocate memory for teleport"
						 " in TANK::activateCurrentSelection()");
			}
		}

		// --- Swapper ---
		//-----------------
		else if (ITEM_SWAPPER == ci) {
			TANK* other = nullptr;

			while (!other) {
				global.getHeadOfClass(CLASS_TANK, &other);

				// If there are only two tanks, just take the other
				if (2 == global.numTanks) {
					if (other == this)
						other->getNext(&other);
				} else {
					// Otherwise select one by random
					int32_t rtn = rand() % (global.numTanks - 1);
					while (rtn--)
						other->getNext(&other);

					// If the selection ended up with this tank, chose the next one
					if (other == this)
						other->getNext(&other);
				}

				assert(other && "ERROR: Swapper selected nullptr!");
				assert( (other != this) && "ERROR: Swapper selected this as other!");

				if (other == this)
					other = nullptr;
			} // End of selecting other

			this->addDamage( player, 0.); // Own falling damage
			this->isTeleported = true;
			other->addDamage(player, 0.); // Their falling damage
			other->isTeleported = true;

			try {
				// create a teleport object for this tank
				new TELEPORT (this, other->x, other->y, tank_dia, 120, ci);
				// create a teleport object for the other tank
				new TELEPORT (other, x, y, other->tank_dia, 120, ci);
			} catch (...) {
				perror ( "tank.cpp: Failed to allocate memory for swap teleports"
						 " in TANK::activateCurrentSelection()");
			}
		}

		// --- Mass Teleport ---
		//-----------------------
		else if (ITEM_MASS_TELEPORT == ci) {
			int32_t right  = env.screenWidth  - (tank_dia * 2);
			int32_t bottom = env.screenHeight - (tank_dia * 2) - MENUHEIGHT;
			TANK*   lt     = nullptr;

			global.getHeadOfClass(CLASS_TANK, &lt);
			while ( lt ) {
				int32_t new_x  = (rand() % right ) + tank_dia;
				int32_t new_y  = (rand() % bottom) + tank_dia + MENUHEIGHT;

				// Like with the normal teleport, ensure a sane y value.
				int32_t surf_y = global.surface[new_x].load(ATOMIC_READ);
				if (new_y < (surf_y - 150) )
					new_y = surf_y - 150;
				else if (new_y > (surf_y + 100) )
					new_y = surf_y + 100;
				try {
					new TELEPORT (lt, new_x, new_y, lt->tank_dia, 120, ci);
					lt->addDamage(player, 0.); // They fall, we earn. Cool.
					lt->isTeleported = true;

				} catch(...) {
					perror ( "tank.cpp: Failed to allocate memory for teleport"
							 " in TANK::activateCurrentSelection()");
				}
				lt->getNext(&lt);
			}
		}

		// --- Rocket ("I beliiiieve Ay Can Flaaaaaaaayy") ---
		//-----------------------------------------------------
		else if (ITEM_ROCKET == ci) {
			yv = -10;
			y -=  10;
			if (a < 180)
				xv += 0.3;
			else if (a > 180)
				xv -= 0.3;
			// If this leads to falling damage, make sure it is a self hit:
			addDamage(player, 0.);
			isTeleported = true;
			applyPhysics();
		}

		// --- Fan (aka "The most useless item there is") ---
		//----------------------------------------------------
		else if (ITEM_FAN == ci) {
			play_fire_sound(ITEM_FAN + WEAPONS, x, 255, 1000);

			if (a < 180)
				// move wind to the right
				global.wind += (p / 20);
			else
				// wind to the left
				global.wind -= (p / 20);

			// make sure wind is not too strong
			if (global.wind < (-env.windstrength / 2) )
				global.wind = -env.windstrength / 2;
			else if (global.wind > (env.windstrength / 2) )
				global.wind = env.windstrength / 2;

			global.lastwind = global.wind;
		}
		// --- Vengeance (BOOM BABY!) ---
		//--------------------------------
		else if ( (ITEM_VENGEANCE <= ci) && (ITEM_FATAL_FURY >= ci) ) {
			// Just preparation. The tank explodes, and the vengeance goes
			// off automatically as selected. ;-)
			this->player->reclaimShield();
			this->addDamage(nullptr, l + sh + repair_rate + 1);
			this->applyDamage();
		}
	} // End of items

	player->time_left_to_fire = env.maxFireTime;
}


/// @brief adds damage, sets creditTo and handles pending damage
void TANK::addDamage(PLAYER* damageFrom, double damage_)
{
	// Clear pending damage if the 'deliverer' changes
	if (damageFrom != creditTo) {
		applyDamage();
		damage = 0.;

		// Update creditTo first
		creditTo   = damageFrom;
		newDamager = true;
	}

	assert( (damage_ >= 0.) && "ERROR: Negative damage?");

	// Raise damage
	if (damage_ > 0.)
		damage += damage_;
}


void TANK::applyDamage ()
{
	// Only *one* call to applyDamage() at any time!
	std::lock_guard<CSpinLock> apply_damage_lock(damage_lock);

	// Before taking any action, damage must be at least 1
	if (destroy || (damage < 1.)) {
		newDamager = false;
		return;
	}

	// See if the pending damage destroys the tank:
	int32_t full_damage = ROUND(damage);

	if (full_damage >= (sh + l) ) {
		destroy     = true;
		damage      = sh + l; // Don't overdo
		full_damage = damage;
		player->killed++;
	}

	// Damage is applied to the shield, and negative shield later
	// added to life. This saves some if/then/else constructs
	int32_t old_sh = sh; // For repulsor shield blast through
	sh -= full_damage;

	/* -----------------------------------------------------------
	 * --- If a damage dealer is set, they must be awarded     ---
	 * --- their reward or, in case of self/team hit, penalty. ---
	 * -----------------------------------------------------------
	 */
	if (creditTo) {
		int32_t award    = full_damage;
		bool    self_hit = creditTo == player;
		bool    team_hit =  !self_hit
		                 && (TEAM_NEUTRAL != creditTo->team)
		                 && (player->team == creditTo->team);

		// Award kill point if no suicide
		if (destroy && !self_hit)
			creditTo->kills++;

		// note damage in own and opponents memory
		player->noteDamageFrom(creditTo, full_damage, destroy);
		creditTo->noteDamageTo(player,   full_damage, destroy);

		// The award must be adapted to the situation
		award *= self_hit ? env.scoreSelfHit :
		         team_hit ? env.scoreTeamHit :
		         env.scoreHitUnit;

		// The kill bonus is only applied if this is no team kill
		if (destroy && !team_hit)
			award += self_hit
			         ? env.scoreUnitSelfDestroy
			         : env.scoreUnitDestroyBonus;

		// Money can not go negative.
		if ((self_hit || team_hit) && (award > creditTo->money))
			award = creditTo->money;

		// If there is an award now, get the money text out
		// and the register to ring.
		if (award > 0) {
			if (creditTo->tank && !global.skippingComputerPlay) {
				static char the_money[16] = { 0x0 };
				snprintf(the_money, 15, "%s$%s",
						(team_hit || self_hit) ? "-" : "",
						Add_Comma( award ) );
				// show how much the shooter gets
				try {
					new FLOATTEXT(the_money,
							creditTo->tank->x, creditTo->tank->y - 30,
							.0, -.5, team_hit ? PURPLE : self_hit ? RED : GREEN,
							CENTRE,
							env.swayingText ? TS_HORIZONTAL : TS_NO_SWAY,
							200, false);
					if (global.stage < STAGE_SCOREBOARD)
						global.updateMenu = true;
				} catch (...) {
					perror ( "tank.cpp: Failed allocating memory for money text"
							 " in applyDamage().");
				}
			}
			creditTo->money += ( (team_hit || self_hit) ? -1 : 1) * award;
		} // End of applying damage award

		// If the tank is destroyed, and it was neither self nor team hit,
		// the damager might spawn a gloating message
		if ( destroy && !creditTo->gloating && !team_hit && !self_hit
		  && creditTo->tank && !creditTo->tank->destroy
		  && !global.skippingComputerPlay) {

			creditTo->gloating = true;

			try {
				new FLOATTEXT(creditTo->selectGloatPhrase(),
				              creditTo->tank->x, creditTo->tank->y - 30,
				              .0, -.4, creditTo->color, CENTRE, TS_NO_SWAY,
				              200, false);
			} catch (...) {
				perror ( "tank.cpp: Failed to allocate memory for"
						 " gloating text in applyDamage().");
			}
		} // End of spawning gloating text

		// Issue a suicide message if the player applied for a darwin award
		if (self_hit && destroy && !global.skippingComputerPlay) {
			try {
				new FLOATTEXT(player->selectSuicidePhrase(),
				              x, y - 30, .0, -.4, player->color,
				              CENTRE, TS_NO_SWAY, 300, false);
			} catch (...) {
				perror ( "tank.cpp: Failed allocate memory for suicide"
						 " text in applyDamage().");
			}
		}
	} // End of handling damager texts and awards

	// -----------------------------------
	// --- Eventually apply the damage ---
	// -----------------------------------
	int32_t old_life = l;
	if (destroy) {
		sh = 0;
		l  = 0;
		repulsion = 0;
	} else {
		if ( (ITEM_LGT_REPULSOR_SHIELD <= sht)
		  && (ITEM_HVY_REPULSOR_SHIELD >= sht) ) {
			// Repulsor shields do not take the full damage but
			// let half of it blasted through.
			int32_t sh_dmg = full_damage / 2;
			int32_t t_dmg  = (full_damage / 2) + (full_damage % 2);
			if (sh_dmg >= old_sh) {
				t_dmg += sh_dmg - old_sh;
				sh_dmg = old_sh;
			}
			if (t_dmg > l)
				t_dmg = l;

			sh = old_sh - sh_dmg;
			l -= t_dmg;

		}

		// normal shield-is-empty-check:
		if (sh < 0.5) {
			l += sh; // For non-repulsor shields.
			sh = 0;
			repulsion = 0;
		}
	}

	// --------------------------------
	// --- Display the damage value ---
	// --------------------------------
	if (full_damage > 0) {
		static char buf[10] = { 0x0 };
		flashdamage = 1;

		if (!global.skippingComputerPlay) {
			snprintf (buf, 9, "%d", full_damage);
			try {
				new FLOATTEXT(buf, x, y - 30, .0, -.3, RED, CENTRE,
				              env.swayingText ? TS_HORIZONTAL : TS_NO_SWAY,
				              300, false);
			} catch (...) {
				perror ( "tank.cpp: Failed to allocate memory for damage"
						 " text in applyDamage().");
			}
		}

		// If shield remains, the shield text has to be regenerated
		if (sh > 0) {
			snprintf (buf, 9, "%d", sh);
			shieldText.set_text (buf);
		} else
			shieldText.set_text (nullptr);

		// If life points were taken, the life text is to be regenerated
		if (old_life != l) {
			snprintf (buf, 9, "%d", l);
			healthText.set_text (buf);
		}
	} // End of having damage

  damage = 0.; // all applied
}


// Thanks to the rockets, tanks can 'fly', and thanks to ... uhm ...
// everything, tanks might fall down.
void TANK::applyPhysics ()
{
	// Do nothing if this tank was destroyed
	if (destroy)
		return;

	double old_y = y;

	// Special handling when rocketing upwards first:
	if ( yv < 0. ) {
		// Although activating a rocket instantly pushes the tank
		// upwards, it stops there if the tank is buried
		if ( howBuried(nullptr, nullptr) ) {
			xv = 0;
			yv = 0;
		}

		/// @todo : Is this really a problem?
		// make sure the tank does not leave the screen when flying
		else if ( (y < tank_off_y) )
			yv = 0;

		// Otherwise apply rocket x movement
		else
			x += xv * 4.;
	} // End of special rocketing handling

	// General movement is only applied while no damage flashes.
	// Note: This means that damage application halts all movement.
	if (flashdamage)
		++flashdamage; // Frame counted
	else {
		bool          on_tank = tank_on_tank();
		int32_t       bottom  = env.screenHeight - tank_off_y; // shortcut
		int32_t pix_col = getpixel(global.terrain, x, y + tank_off_y - tank_sag);

		// Hitting a wall only bounces the tank.
		if ( ((x + xv) < 1) || ((x + xv) > (env.screenWidth - 1)) )
			xv *= -1.;

		// Check whether a previous fall just ends:
		if ( (yv > 0.) && ( (y >= bottom) || (PINK != pix_col) || on_tank ) ) {
			addDamage(creditTo, yv * 10.);
			if (isTeleported)
				isTeleported = false;

			// 10 points of damage are 'free' when falling
			// Note: This negates any damage when parachuting as well.
			if (damage >= 10.)
				damage -= 10.;
			else
				damage  = 0.;

			// Stop movement
			xv = 0.;
			yv = 0.;

			// fix y if the tank was fast enough to push through the floor
			if (y > bottom)
				y = bottom;

			// Reset falling delay and apply damage at once
			delay_fall = env.landSlideDelay * 100;
			applyDamage();
		} // End of fall stop

		// Check whether the tank currently is falling
		else if ( (y < bottom) && (PINK == pix_col) && !on_tank
				&& (env.landSlideType > SLIDE_NONE) ) {

			// If this is set to cartoon falling, decrease delay and exit.
			if ( (SLIDE_CARTOON == env.landSlideType)
			  && (--delay_fall > 0) )
				return;

			yv += env.gravity * env.FPS_mod;

			// Check for parachute opening
			if (para) {
				if (para < 3)
					++para;

				// With a parachute, wind can blow the tank away
				xv += (global.wind - xv) / mass
				    * (drag + 0.35) * env.viscosity;

				// Limit yv, we have a parachute!
				if (yv > 0.5 )
					yv = 0.5;
			} else {
				// If we have parachutes, deploy one:
				if ((player->ni[ITEM_PARACHUTE]) && (yv >= 1.0)) {
					para = 1;
					player->ni[ITEM_PARACHUTE]--;
				}
			}

			x += xv;
			y += yv;
		} // End of fall start / continue

		// Nothing? Then make sure no parachute is shown
		else
			para = 0;

		// If there is no damage flashing, apply what is there
		if (!flashdamage) {
			applyDamage();
		}

		requireUpdate();
	} // End of regular movement

	setTextPositions(old_y != y);
}


/// @brief Test if the current weapon is available. Find another one,
/// preferably stronger, if the current is empty.
void TANK::check_weapon()
{
	if ( (cw < 0) || (cw > WEAPONS) )
		cw = 0;

	if (player && !player->nm[cw] ) {
		// Try upwards first:
		int32_t old_cw = cw++;
		for ( ; (cw < WEAPONS) && !player->nm[cw] ; ++cw) ;

		// If this wasn't successful, go downwards:
		if (WEAPONS == cw) {
			cw = old_cw - 1;
			for ( ; (cw > 0) && !player->nm[cw] ; --cw) ;
		}

		// This ends up with Small Missile if no other weapons are available.
		player->changed_weapon = true;
	}
}


/// @brief Deactivate vertical bounce and reset text positions
void TANK::deactivate()
{
	shieldText.set_sway_type(TS_NO_SWAY);
	healthText.set_sway_type(TS_NO_SWAY);
	nameText.set_sway_type(TS_NO_SWAY);

	setTextPositions(true);
}


void TANK::draw()
{
	// check for foggy weather
	if ( ( env.fog ) && ( global.get_curr_tank() != this ) ) {
		addUpdateArea (x - tank_off_x - 3, y - 25, 35, 46);
		requireUpdate ();
		return;
	}

	// get bitmap for tank
	if ( (use_tankbitmap < 0) || (use_turretbitmap < 0) ) {
		setBitmap();

		assert ((use_tankbitmap >= 0) && (use_turretbitmap >= 0)
			&& "ERROR: Unable to set tank/turret bitmap!");

		// No bitmap, no fun
		if ( (use_tankbitmap < 0) || (use_turretbitmap < 0) )
			return;
	}

	// Put a coloured rectangle below the tank.
	rectfill (global.canvas, x - tank_off_x, y + tank_off_y,
	          x + tank_off_x, y + tank_off_y + 2,
	          this->player->color);

	// Draw shield first if any
	if (sh > 0) {
		// Make sure the values are set. (Client may not have set them)
		assert((BLACK != shld_col_outer) && shld_thickness
				&& "ERROR: Did client() forget to set shield values?");

		if (BLACK == shld_col_outer)
			shld_col_outer = makecol(item[sht].vals[SHIELD_RED],
			                         item[sht].vals[SHIELD_GREEN],
			                         item[sht].vals[SHIELD_BLUE]);
		if (!shld_thickness)
			shld_thickness = item[sht].vals[SHIELD_THICKNESS];

		// Adapt shield phase:
		double str_mod = static_cast<double>(sh) / item[sht].vals[SHIELD_ENERGY];
		// The weaker the shield, the faster the phase
		shld_phase += shld_delta / str_mod;
		// Don't overdo
		while (shld_phase > 360.)
			shld_phase -= 360.;

		// Set basic values
		int32_t move_x  = ROUNDu(x);
		int32_t move_y  = ROUNDu(y) - turr_off_y + shld_rad_y;
		int32_t rad_x   = shld_rad_x;
		int32_t rad_y   = shld_rad_y;
		int32_t half_th = shld_thickness / 2;

		// Whether it is done over x or y depends on the type of shield
		if (sht < ITEM_LGT_REPULSOR_SHIELD) {
			rad_x  += static_cast<int32_t>(env.slope[ROUNDu(shld_phase)][0] * 3.);
			rad_y  += half_th;
			move_x -= half_th;
		} else {
			rad_x  += half_th;
			rad_y  += shld_phase * 6. / 360.;
			move_y -= half_th;
		}

		drawing_mode(DRAW_MODE_TRANS, nullptr, 0, 0);
		global.current_drawing_mode = DRAW_MODE_TRANS;
		set_trans_blender (0, 0, 0, 50);
		ellipsefill (global.canvas, move_x, move_y, rad_x, rad_y, shld_col_inner);
		set_trans_blender (0, 0, 0, ROUND(200. * str_mod) + 25);

		if (sht < ITEM_LGT_REPULSOR_SHIELD) {
			for (int32_t i = 0; i < shld_thickness; ++i)
				ellipse (global.canvas, move_x + i, move_y, rad_x, rad_y, shld_col_outer);
		} else {
			for (int32_t i = 0; i < shld_thickness; ++i)
				ellipse (global.canvas, move_x, move_y + i, rad_x, rad_y, shld_col_outer);
		}

		drawing_mode(DRAW_MODE_SOLID, nullptr, 0, 0);
		global.current_drawing_mode = DRAW_MODE_SOLID;
		setUpdateArea (move_x - shld_thickness - rad_x,
		               move_y - shld_thickness - rad_y,
		               (rad_x + shld_thickness) * 2,
		               (rad_y + shld_thickness) * 2);
	} // End of drawing shield

	// Without a shield, the update area can be smaller
	else
		setUpdateArea (x - turr_off_x - 1, y - turr_off_x - 1,
		               (turr_off_x * 2) + 2, tank_off_y + turr_off_x + 20);

	// Now draw the tank sprite
	draw_sprite   (global.canvas, env.tank[use_tankbitmap], x - tank_off_x, y);
	rotate_sprite (global.canvas, env.tankgun[use_turretbitmap],
					x - turr_off_x, y - turr_off_y,
					itofix( (90 - a) * 256 / 360) );

	// when using rockets, show flame
	if (yv < 0)
		/// @todo : This looks silly. We sure can do better, can't we?
		rectfill(global.canvas, x - tank_off_x, y + tank_off_y,
		               x + tank_off_x, y + tank_off_y + 10, ORANGE);

	// Eventually draw the parachute
	if (para) {
		draw_sprite (global.canvas, env.tank[para], x - tank_off_x - 3, y - 25);
		addUpdateArea (x - tank_off_x - 3, y - 25, 35, 66);
	}

	setTextPositions(false);
	requireUpdate ();
}


/// @brief Create explosion and sound if a tank is destroyed. If available
/// and/or set, stage a violent death.
void TANK::explode (bool allow_vengeance)
{
	if (!destroy)
		return;

	// Note: player->revenge and revenge texts are handled in applyDamage()

	try {
		new EXPLOSION (player, x, y, 0., env.screenHeight / 10., MED_MIS, false);
	} catch (...) {
		perror ( "tank.cpp: Failed to allocate memory for explosion"
		         " in TANK::explode()");
	}

	play_explosion_sound(MED_MIS, x, 255, 1000);

	// Violent death can only trigger if there is a player.
	// Note: There should be, always, so assert as well. Calling this method
	// after player was set to nullptr is a bug.
	assert (player && "ERROR: explode() called with nullptr player!");
	if (nullptr == player)
		return;

#ifdef NETWORK
	int32_t     playerindex = 0;
	bool        found       = false;
	static char buffer[15]  = { 0x0 };

	// get the player index
	while ( (playerindex < env.numGamePlayers) && (!found) ) {
		if (  env.players[playerindex]
		  && (env.players[playerindex]->tank == this) )
			found = true;
		else
			playerindex++;
	}

	// we should have found a match and now we send it to all clients
	if (found) {
		snprintf(buffer, 14, "REMOVETANK %d", playerindex);
		env.sendToClients(buffer);
	}
#endif // NETWORK

	// If vengeance is not allowed, break up here.
	if (!allow_vengeance)
		return;

	// If violent death is enabled to be automatically,
	// possibly sponsor one for the player unless they
	// already have something better.
	// But only if it is not the first 3 rounds.
	if (env.violent_death && ( (env.rounds - global.currentround) > 3) ) {
		int32_t ri = rand() % VIOLENT_CHANCE;

		// Limit ri to the value of violent_death.
		// This makes it less probable to trigger anything on lower settings.
		if ( (ri <= VD_HEAVY) && (ri > env.violent_death) )
			ri = env.violent_death;

		// The entry is, that the player either has no fatal fury
		// or is about to get one sponsored.
		if (ri && (!player->ni[ITEM_FATAL_FURY] || (VD_HEAVY == ri)) ) {
			if (VD_HEAVY == ri)
				++player->ni[ITEM_FATAL_FURY];
			else if (!player->ni[ITEM_DYING_WRATH] || (VD_MEDIUM == ri)) {
				if (VD_MEDIUM == ri)
					++player->ni[ITEM_DYING_WRATH];
				else if (VD_LIGHT == ri)
					++player->ni[ITEM_VENGEANCE];
			}
		} // end of applying sponsorship to big boom
	} // End of violent death sponsorship

	// If the player has something to trigger in their last moment,
	// do so now:
	int32_t numLaunch = 0;
	int32_t min_power = 0;
	int32_t del_power = 0;

	if (player->ni[ITEM_FATAL_FURY] > 0) {
		numLaunch = item[ITEM_FATAL_FURY].vals[SELFD_NUMBER];
		cw        = item[ITEM_FATAL_FURY].vals[SELFD_TYPE];
		player->nm[cw] += numLaunch;
		player->ni[ITEM_FATAL_FURY]--;
		min_power = MAX_POWER / 3;
		del_power = MAX_POWER / 3;
	} else if (player->ni[ITEM_DYING_WRATH] > 0) {
		numLaunch = item[ITEM_DYING_WRATH].vals[SELFD_NUMBER];
		cw        = item[ITEM_DYING_WRATH].vals[SELFD_TYPE];
		player->nm[cw] += numLaunch;
		player->ni[ITEM_DYING_WRATH]--;
		min_power = MAX_POWER / 4;
		del_power = MAX_POWER / 3;
	} else if (player->ni[ITEM_VENGEANCE] > 0) {
		numLaunch = item[ITEM_VENGEANCE].vals[SELFD_NUMBER];
		cw        = item[ITEM_VENGEANCE].vals[SELFD_TYPE];
		player->nm[cw] += numLaunch;
		player->ni[ITEM_VENGEANCE]--;
		min_power = MAX_POWER / 5;
		del_power = MAX_POWER / 3;
	}

	// If there is anything to launch, do it
	if (numLaunch) {
		// Expensive equipment like this should come with a certain quality.
		// The most important detail (right after actually going off and not
		// being a dud) is that the bucks won't be blasted the wrong way.
		TANK* tank    = nullptr;
		int32_t med_x = 0;
		int32_t tanks = 0;

		global.getHeadOfClass(CLASS_TANK, &tank);

		while (tank) {
			if ( (tank != this) && !tank->destroy
			  && ( (TEAM_NEUTRAL == player->team)
			    || (player->team != tank->player->team) ) ) {
				++tanks;
				med_x += tank->x;
			}
			tank->getNext(&tank);
		}

		// Get the medium x position of all tanks (or the middle of the
		// screen if this was the last tank... for the firework...)
		if (tanks)
			med_x /= tanks;
		else
			med_x = env.halfWidth;

		int32_t start_a = 45;
		int32_t mod_a   = 90;

		if (med_x < (x - 50) ) {
			start_a = 30;
			mod_a   = 55;
		} else if (med_x > (x + 50) ) {
			start_a = 95;
			mod_a   = 55;
		}

		// Before the violent death is applied, halve the players
		// damage multiplier:
		assert(player && "ERROR: explode Tank without player?");
		player->damageMultiplier = player->damageMultiplier > 1.
		                         ? 1. + ((player->damageMultiplier - 1.) / 2.)
		                         : .75;

		// Now go for it!
		int32_t cur_stage = global.stage;
		global.stage = STAGE_FIRE;
		for (int32_t i = numLaunch; i > 0; --i) {
			a = 180 - (start_a + (rand() % mod_a) - 90);
			p = min_power + (rand () % del_power);
			activateCurrentSelection ();
		}
		global.stage = cur_stage;
	}
}


/// @return The tanks bottom coordinate as used in collision detection.
int32_t TANK::getBottom()
{
	return y + tank_off_y - tank_sag;
}


/// @return The calculated tank diameter
double TANK::getDiameter()
{
	return tank_dia;
}


/// Sets @a top_x and @a top_y to the coordinates of the cannon tip
void TANK::getGuntop(int32_t angle_, double &top_x, double &top_y)
{
	top_x = x + (env.slope[angle_][0] * turr_off_x);
	top_y = y + (env.slope[angle_][1] * turr_off_y);

	// top_y must not be lower than tank y. (Lower means greater in value)
	if (top_y > y)
		top_y = y;
}


/// @return the current maxLife value
int32_t TANK::getMaxLife()
{
	return maxLife;
}


/// @brief return true if a repulsor shield is up and running
bool TANK::hasRepulsorActivated()
{
	return (repulsion != 0);
}


/// @brief return the number of pixels a tanks canon is buried
/// If @a left and or @a right are given, they will receive the buried
/// level on that side only.
int32_t TANK::howBuried (int32_t* left, int32_t* right)
{
	int32_t result = 0;
	int32_t old_x  = 0;
	int32_t old_y  = 0;
	int32_t cur_x  = 0;
	int32_t cur_y  = 0;
	double  angles_seen = 0.;

	// Angles are from right (90) to left (270) counter clockwise
	for (int32_t ta = 90; ta < 270; ++ta) {
		cur_x = x + (env.slope[ta][0] * turr_off_x);
		cur_y = y + (env.slope[ta][1] * turr_off_y);

		if ( (cur_x != old_x) || (cur_y != old_y) ) {
			if (PINK != getpixel(global.terrain, cur_x, cur_y))
				++result;
			old_x = cur_x;
			old_y = cur_y;
			angles_seen += 1.;
		}

		if (180 == ta) {
			if (right) *right =  result;
			if (left)  *left  = -result;
		}
	}

	// Add full result to right to negate left half and count only right half
	if (left) {
		*left += result;
		if (*left < 0)
			*left = 0;
	}

	// The result must be adapted according how many *real* angles,
	// meaning "missile starting points" are there.
	// If boxed mode is on, the level is doubled, as the ceiling makes it
	// very difficult to aim otherwise.
	double angle_mod = 180. / angles_seen;

	if (left)  *left  = ROUNDu(*left  * angle_mod / 2.);
	if (right) *right = ROUNDu(*right * angle_mod / 2.);

	result *= angle_mod * (env.isBoxed ? 1.25 : 1.);

	return ROUNDu(result);
}


/// @return true if the tank is moving up or downwards (rocket / fall / glide)
bool TANK::isFlying()
{
	return (yv < 0.) || (yv > 0.);
}


/// @return true if the tank is within the box defined by the given coordinates.
bool TANK::isInBox(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
	double gun_x, gun_y;
	getGuntop(a, gun_x, gun_y);
	return ( (std::min(x1, x2) < std::max(x + tank_off_x, gun_x) )
		  && (std::max(x1, x2) > std::min(x - tank_off_x, gun_x) )
		  && (std::min(y1, y2) < (y + tank_off_y) )
		  && (std::max(y1, y2) > std::min(y, gun_y) ) );
}


/** @return true if the tank is within the given ellipse.
  * @param[in] ex Explosion x coordinate
  * @param[in] ey Explosion y coordinate
  * @param[in] rx Explosion x radius
  * @param[in] ry Explosion y radius
  * @param[out] in_rate_x The rate [0.;1.] of the tank x axis being in the ellipse.
  * @param[out] in_rate_y The rate [0.;1.] of the tank y axis being in the ellipse.
**/
bool TANK::isInEllipse(double ex, double ey, double rx, double ry,
					double &in_rate_x, double &in_rate_y)
{
	in_rate_x = 0.;
	in_rate_y = 0.;

	// The real gun tip height:
	double gun_y_off = env.slope[a][1] * turr_off_y;

	// But not below the tank:
	if (gun_y_off > 0.)
		gun_y_off = 0.;

	// For the tank the real centre position and the radii must be known.
	double ox       = static_cast<double>(tank_off_x);
	double oy       = static_cast<double>(tank_off_y - gun_y_off) / 2.;
	double tx       = x; // For consistencies sake...
	double ty       = y + (static_cast<double>(tank_off_y + gun_y_off) / 2.);

	double simpDist = FABSDISTANCE2(ex, ey, tx, ty);

	// First handle a special case: The explosion takes place within the
	// tank defined ellipse. This is a full direct hit.
    if (simpDist <= (tank_dia / 2.)) {
		in_rate_x = 1.;
		in_rate_y = 1.;
		DEBUG_LOG_PHY("Full Direct Hit", "=== T %d/%d (%dx%d) versus E %d/%d (%dx%d) ===",
					  ROUND(tx), ROUND(ty), ROUND(ox), ROUND(oy),
					  ROUND(ex), ROUND(ey), ROUND(rx), ROUND(ry))
		DEBUG_LOG_PHY("Full Direct Hit", "Distance %5.2lf <= Dia half %5.2lf",
		              simpDist, tank_dia / 2.)
		return true;
    }


	/* The ideal solution would be to calculate the intersection of the two
	 * ellipses which would enable us to tell the absolute correct value of how
	 * much of the tank surface is covered by the explosion.
	 *
	 * Unfortunately this involves a 4th order equation to allow a numerical
	 * solution. (I have found a very nice example written in JavaScript. It
	 * uses several functions and has ~600 Lines. A bit much for a game, right?)
	 *
	 * The second best solution would be to determine the position on the rim of
	 * each ellipse that is on a line between the two centres and then use their
	 * distance. That is, if you blame this file and look up the commits, what I
	 * tried first.
	 * Unfortunately the exact location on the rim is almost as complex to
	 * determine as the intersection. I did not see this in the beginning,
	 * because I simply reused an algorithm of mine to check spheres in a 3D
	 * space for collisions. But the hard truth is, that 3D spheres and 2D
	 * ellipses are very different.
	 *
	 * So here is solution three. Still good enough for a game though. Just
	 * check how much the x and y dimensions overlap and do a rough intersection
	 * in two dimensions by reverse calculating the other axis. (This allows a
	 * finer control of the damage for special shots like shaped charges,
	 * though)
	 */


	// If shields are on, they must be taken into account
	if (sh > 0.) {
		ox = shld_rad_x;
		oy = shld_rad_y;
	}

	// Determine start and end positions of explosion/tank intersections.
	// The check is needed so hit/end are in the correct direction.
	double hit_x = ex > tx ? std::min(ex + rx, tx + ox) : std::max(ex - rx, tx - ox);
	double end_x = ex > tx ? std::max(ex - rx, tx - ox) : std::min(ex + rx, tx + ox);
	double hit_y = ey > ty ? std::min(ey + ry, ty + oy) : std::max(ey - ry, ty - oy);
	double end_y = ey > ty ? std::max(ey - ry, ty - oy) : std::min(ey + ry, ty + oy);
	/* Check:
	 *   ex = 100, tx = 150, rx = 40, ox = 20
	 * hit_x = 100 > 150 ? ... : max(100 - 40, 150 - 20) = max(60, 130)  = 130
	 * end_x = 100 > 150 ? ... : min(100 + 40, 150 + 20) = min(140, 170) = 140
	 *   ex/tx swapped:
	 * hit_x = 150 > 100 ? min(150 + 40, 100 + 20) = min(190, 120) = 120 : ...
	 * end_x = 150 > 100 ? max(150 - 40, 100 - 20) = max(110, 80)  = 110 : ...
	 *
	 * This means start to end is always from explosion to tank.
	 */


	// Check whether the tank is in the explosion if both were boxes.
	if ( (SIGN(ex - tx) != SIGN(hit_x - end_x))
	  || (SIGN(ey - ty) != SIGN(hit_y - end_y)) )
		return false;


    // Handle another special case: The tank is fully within the explosion:
    if ( (std::min(hit_x, end_x) <= (tx - ox))
	  && (std::min(hit_y, end_y) <= (ty - oy))
	  && (std::max(hit_x, end_x) >= (tx + ox))
	  && (std::max(hit_y, end_y) >= (ty + oy)) ) {
		in_rate_x = ((rx - std::abs(tx - ex) ) / rx) + 0.5;
		in_rate_y = ((ry - std::abs(ty - ey) ) / ry) + 0.5;
		if (in_rate_x > 1.0)
			in_rate_x = 1.0;
		if (in_rate_y > 1.0)
			in_rate_y = 1.0;

		// The full in_rate in "full washing over" must not be lower than 1/3
		if (in_rate_x < 0.578)
			in_rate_x = 0.578;
		if (in_rate_y < 0.578)
			in_rate_y = 0.578;

		// Note: This value is taken, because 0.578 * 0.578 = 0.3341 (~1/3)

		DEBUG_LOG_PHY("Full Indirect Hit", "=== T %d/%d (%dx%d) versus E %d/%d (%dx%d) ===",
					  ROUND(tx), ROUND(ty), ROUND(ox), ROUND(oy),
					  ROUND(ex), ROUND(ey), ROUND(rx), ROUND(ry))
		DEBUG_LOG_PHY("Full Indirect Hit", "Left %d <= %d",
		              ROUNDu(std::min(hit_x, end_x)), ROUNDu(tx - ox))
		DEBUG_LOG_PHY("Full Indirect Hit", "Top %d <= %d",
		              ROUNDu(std::min(hit_y, end_y)), ROUNDu(ty - oy))
		DEBUG_LOG_PHY("Full Indirect Hit", "Right %d >= %d",
		              ROUNDu(std::max(hit_x, end_x)), ROUNDu(tx + ox))
		DEBUG_LOG_PHY("Full Indirect Hit", "Bottom %d >= %d",
		              ROUNDu(std::max(hit_y, end_y)), ROUNDu(ty + oy))
		DEBUG_LOG_PHY("Full Indirect Hit", "in_rate_x : %5.2lf", in_rate_x)
		DEBUG_LOG_PHY("Full Indirect Hit", "in_rate_y : %5.2lf", in_rate_y)
		DEBUG_LOG_PHY("Full Indirect Hit", "Distance %5.2lf > Dia half %5.2lf",
		              simpDist, tank_dia / 2.)

		return true;
	}

	// Get the middle of the x range and the explosions and tanks y rim there
	double mid_x   = (hit_x + end_x) / 2.;
	double e_off_y = std::abs(ry * std::sin(std::acos((ex - mid_x) / rx)));
	double t_off_y = std::abs(oy * std::sin(std::acos((tx - mid_x) / ox)));
	double rng_y   = std::min(ey + e_off_y, ty + t_off_y)
	               - std::max(ey - e_off_y, ty - t_off_y);
	/* Note:
	 * If the explosion is within the tank range:
	 *  ey+off < ty+off, ey-off > ty-off => rng = (ey+off) - (ey+off) => rng > 0
	 * If the explosion is above the tank but reaches in:
	 *  ey+off < ty+off, ey-off < ty-off => rng = (ey+off) - (ty-off) => rng > 0
	 * If the explosion is below the tank but reaches is:
	 *  ey+off > ty+off, ey-off > ty-off => rng = (ty+off) - (ey-off) => rng > 0
	 * If the tank is fully within the explosion:
	 *  ey+off > ty+off, ey-off < ty-off => rng = (ty+off) - (ty-off) => rng > 0
	 * If the explosion is fully above the tank:
	 *  ey+off < ty+off, ey-off < ty-off => rng = (ey+off) - (ty+off) => rng < 0
	 * If the explosion is fully below the tank:
	 *  ey+off > ty+off, ey-off > ty-off => rng = (ty+off) - (ey-off) => rng < 0
	 */


	// Opt out if the explosions rim y does not fit the tanks rim y
	if ( rng_y < .01 ) {
		DEBUG_LOG_PHY("Opt Out Y", "=== T %d/%d (%dx%d) versus E %d/%d (%dx%d) ===",
					  ROUND(tx), ROUND(ty), ROUND(ox), ROUND(oy),
					  ROUND(ex), ROUND(ey), ROUND(rx), ROUND(ry))
		DEBUG_LOG_PHY("Opt Out Y", "ey    - ty    (%d - %d = %d / %5.3lf)",
						ROUND(ey), ROUND(ty), ROUND(ey - ty), rng_y)
		DEBUG_LOG_PHY("Opt Out Y", "Exp_y (%d to %d = %d)",
						ROUND(ey - e_off_y), ROUND(ey + e_off_y), ROUND(2 * e_off_y))
		DEBUG_LOG_PHY("Opt Out Y", "Tnk_y (%d to %d = %d)",
						ROUND(ty - t_off_y), ROUND(ty + t_off_y), ROUND(2 * t_off_y))
		return false;
	}

	// Get the middle of the y range and the explosions x rim there
	double mid_y = (hit_y + end_y) / 2.;
	double e_off_x = std::abs(rx * std::cos(std::asin((ey - mid_y) / ry)));
	double t_off_x = std::abs(ox * std::cos(std::asin((ty - mid_y) / oy)));
	double rng_x   = std::min(ex + e_off_x, tx + t_off_x)
	               - std::max(ex - e_off_x, tx - t_off_x);

	// Opt out if the explosions rim x does not fit the tanks rim x
	if ( rng_x < .01 ) {
		DEBUG_LOG_PHY("Opt Out X", "=== T %d/%d (%dx%d) versus E %d/%d (%dx%d) ===",
					  ROUND(tx), ROUND(ty), ROUND(ox), ROUND(oy),
					  ROUND(ex), ROUND(ey), ROUND(rx), ROUND(ry))
		DEBUG_LOG_PHY("Opt Out X", "ex    - tx    (%d - %d = %d / %5.3lf)",
						ROUND(ex), ROUND(tx), ROUND(ex - tx), rng_x)
		DEBUG_LOG_PHY("Opt Out X", "Exp_x (%d to %d = %d)",
						ROUND(ex - e_off_x), ROUND(ex + e_off_x), ROUND(2 * e_off_x))
		DEBUG_LOG_PHY("Opt Out X", "Tnk_x (%d to %d = %d)",
						ROUND(tx - t_off_x), ROUND(tx + t_off_x), ROUND(2 * t_off_x))
		return false;
	}


	// Being here means that both ranges lie within the tanks ellipse.
	in_rate_x = rng_x / ( ox * 2. );
	in_rate_y = rng_y / ( oy * 2. );

	assert ( (in_rate_x > 0.) && (in_rate_y > 0.) && "RANGE ERROR");

	DEBUG_LOG_PHY("Overlapping", "=== T %d/%d (%dx%d) versus E %d/%d (%dx%d) ===",
				  ROUND(tx), ROUND(ty), ROUND(ox), ROUND(oy),
				  ROUND(ex), ROUND(ey), ROUND(rx), ROUND(ry))
	DEBUG_LOG_PHY("Overlapping", "Hit: %4d/%4d ; End: %4d/%4d",
					ROUNDu(std::min(hit_x, end_x)), ROUNDu(std::min(hit_y, end_y)),
					ROUNDu(std::max(hit_x, end_x)), ROUNDu(std::max(hit_y, end_y)) )
	DEBUG_LOG_PHY("Overlapping", "Exp: %4d/%4d ; Tnk %4d/%4d ; Rng %4d/%4d",
					ROUNDu(e_off_x), ROUNDu(e_off_y),
					ROUNDu(t_off_y), ROUNDu(t_off_y),
					ROUNDu(rng_x), ROUNDu(rng_y) )
	DEBUG_LOG_PHY("Overlapping", "in_rate_x  : %5.2lf", in_rate_x)
	DEBUG_LOG_PHY("Overlapping", "in_rate_y  : %5.2lf", in_rate_y)

	// distances over 50% radius are taken to reduce the rates further
	double dist_x_mod = ((rx - std::abs(tx - ex) ) / rx) + 0.5;
	double dist_y_mod = ((ry - std::abs(ty - ey) ) / ry) + 0.5;

	// Limit to 0.578 to 1.0 (See full indirect hit why 0.578 is used)
	if (dist_x_mod > 1.0)
		dist_x_mod = 1.0;
	if (dist_x_mod < 0.578)
		dist_x_mod = 0.578;
	if (dist_y_mod > 1.0)
		dist_y_mod = 1.0;
	if (dist_y_mod < 0.578)
		dist_y_mod = 0.578;

	DEBUG_LOG_PHY("Overlapping", "dist_x_mod : %5.2lf", dist_x_mod)
	DEBUG_LOG_PHY("Overlapping", "dist_y_mod : %5.2lf", dist_y_mod)

	in_rate_x *= dist_x_mod;
	in_rate_y *= dist_y_mod;

	// Trim both rates to 1.0
	if (in_rate_x > 1.0)
		in_rate_x = 1.0;
	if (in_rate_y > 1.0)
		in_rate_y = 1.0;

	DEBUG_LOG_PHY("Overlapping", "Final x    : %5.2lf", in_rate_x)
	DEBUG_LOG_PHY("Overlapping", "Final y    : %5.2lf", in_rate_y)

	return true;
}


/** @brief move the tank one unit
  * This function tries to move the tank either left or right one
  * unit.
  *
  * @param[in] direction The direction to move, DIR_RIGHT or DIR_LEFT.
  *
  * @return true if the tank was moved, false otherwise
**/
bool TANK::moveTank(int32_t direction)
{
	// return now if the tank is flying/falling or has no fuel
	if ( (player->ni[ITEM_FUEL] < 1 ) // No fuel ?
	  || (yv < 0.) || (yv > 0.) )     // flying / falling ?
		return false;

	// Safety: assert DIR_LEFT/RIGHT
	assert ( ((DIR_LEFT == direction) || (DIR_RIGHT == direction))
			&& "ERROR: Call moveTank with either DIR_LEFT or DIR_RIGHT!");
	if ( (DIR_LEFT != direction) && (DIR_RIGHT != direction))
		return false;

	// Check whether the target pixel is beyond the border or occupied
	int32_t nextX = ROUND(x + direction);
	if ( (nextX < 1)
	  || (nextX >= env.screenWidth)
	  || (env.landType == LAND_NONE) )
		return false;

	// select the next pixel on the left/right that is not terrain
	float   nextY  = global.surface[nextX].load(ATOMIC_READ) - 1;

	// If there is more terrain to climb, the pixel after the next must
	// be taken into account, too
	int32_t afterX = nextX + direction;
	float   afterY = nextY;
	if ( (afterX > 0) && (afterX < env.screenWidth) )
		afterY  = global.surface[afterX].load(ATOMIC_READ) - 1;

	// If the tank is not climbing too much, let it move:
	if ( (nextY > (y - tank_sag)) && (afterY > (y - tank_off_y + tank_sag)) ) {
		// move tank
		x = nextX;
		player->ni[ITEM_FUEL]--;

		// climb and correct for the tank extension
		y = nextY - tank_off_y + tank_sag;

		// But secure y
		if (y > (env.screenHeight - tank_off_y))
			y = env.screenHeight - tank_off_y;
		return true;
	}

	// No move:
	return false;
}


void TANK::newRound (int32_t pos_x, int32_t pos_y)
{
	// A new round without set player is futile.
	assert(player && "ERROR: TANK::newRound called with nullptr player");
	if (nullptr == player)
		return;

	static char buf[10] = { 0x0 };

	// Reclaim shield if there is one left from end of last round
	player->reclaimShield();

	// Reset all values
	cw          = 0;
	damage      = 0.;
	para        = 0;
	creditTo    = nullptr;
	p           = MAX_POWER / 2;
	a           = (rand () % 150) + 105;
	sh          = 0;
	sht         = ITEM_NO_SHIELD;
	repulsion   = 0;
	delay_fall  = env.landSlideDelay * 100;

	// Re-calculate max life
	double tmpL = (player->ni[ITEM_ARMOUR]   * item[ITEM_ARMOUR].vals[0])
	            + (player->ni[ITEM_PLASTEEL] * item[ITEM_PLASTEEL].vals[0]);
	maxLife     = 100 + (tmpL > 0. ? static_cast<int32_t>(std::pow(tmpL, .6)) : 0);
	l           = maxLife;

	// (re)-init health text
	snprintf (buf, 9, "%d", l);
	healthText.set_text (buf);
	healthText.set_color(player->color);

	// Re-calculate repair rate
	int32_t num_kits        = player->ni[ITEM_REPAIRKIT];
	int32_t increase_amount = 5;
	repair_rate             = 0;
	while (num_kits-- > 0) {
		repair_rate += increase_amount;
		if (increase_amount > 1)
			--increase_amount;
	}

	// (re-)init name text
	if (env.nameAboveTank) {
		nameText.set_text ( player->getName() );
		nameText.set_color( player->color     );
	}

	fire_another_shot = 0;

	// Set used bitmaps, determine offsets and place tank
	x = pos_x;
	y = pos_y;
	use_tankbitmap   = -1;
	use_turretbitmap = -1;
	setBitmap();
}


void TANK::reactivate_shield ()
{
	// if no shield remains, try to reload
	if (sh > 0)
		return;

	static char buf[5] = { 0x0 };

	sht = ITEM_NO_SHIELD;

	// Try to set the most heavy shield there is available:
	if (player->ni[ITEM_HVY_REPULSOR_SHIELD])
		sht = ITEM_HVY_REPULSOR_SHIELD;
	else if (player->ni[ITEM_HVY_SHIELD])
		sht = ITEM_HVY_SHIELD;
	else if (player->ni[ITEM_MED_REPULSOR_SHIELD])
		sht = ITEM_MED_REPULSOR_SHIELD;
	else if (player->ni[ITEM_MED_SHIELD])
		sht = ITEM_MED_SHIELD;
	else if (player->ni[ITEM_LGT_REPULSOR_SHIELD])
		sht = ITEM_LGT_REPULSOR_SHIELD;
	else if (player->ni[ITEM_LGT_SHIELD])
		sht = ITEM_LGT_SHIELD;

	if (ITEM_NO_SHIELD != sht) {
		player->ni[sht]--;
		sh              = item[sht].vals[SHIELD_ENERGY];
		repulsion       = item[sht].vals[SHIELD_REPULSION];
		shld_col_outer  = makecol (item[sht].vals[SHIELD_RED],
		                           item[sht].vals[SHIELD_GREEN],
		                           item[sht].vals[SHIELD_BLUE]);
		shld_thickness = item[sht].vals[SHIELD_THICKNESS];

		player->last_shield_used = sht;
		shld_phase    = 0.; // Start neutral.
		snprintf (buf, 4, "%d", sh);
		shieldText.set_text (buf);
		setTextPositions(true);
	}
}


/// @brief do tank repairs
void TANK::repair()
{
	if ( (repair_rate > 0) && (l < maxLife) ) {
		int32_t old_life = l;

		// Apply repair
		l += repair_rate;
		if (l > maxLife)
			l = maxLife;

		// update text
		snprintf(buf, 9, "%d", l);
		healthText.set_text(buf);

		// add float text
		if (!global.skippingComputerPlay) {
			try {
				snprintf(buf, 9, "+%d", l - old_life);
				new FLOATTEXT(buf, x, y - 30, .0, -.8, GREEN, CENTRE,
				              TS_NO_SWAY, 120, false);
			} catch (...) {
				perror("tank.cpp: Failed to allocate memory for healing"
					   " text in repair().");
			}
		}
	}
}


bool TANK::repulse (double xpos, double ypos, double* xa, double* ya,
                    ePhysType phys_type)
{
	// If there is no repulsion or the physics type is
	// not sensitive to repulsion, return at once.
	if ( !repulsion
	  || (PT_FUNKY_FLOAT == phys_type)
	  || (PT_NONE        == phys_type)
	  || (PT_ROLLING     == phys_type) )
		return false;

	double xdist = xpos - x;
	double ydist = ypos - (y - turr_off_y + shld_rad_y);

	// Apply a minimum distance so no extreme catapult shots happen
	if (std::abs(xdist) <  0.25) xdist =  0.25 * SIGNd(xdist);
	if (std::abs(ydist) <  0.25) ydist =  0.25 * SIGNd(ydist);

	// Unless this is a burying type that currently comes from below,
	// repulsion is done upwards, assuming the projectile comes from above.
	if ( ( (PT_DIGGING == phys_type) && (ydist < 0.) )
	  || ( (PT_DIGGING != phys_type) && (ydist > 0.) ) )
		ydist *= -1.0; // Missiles normally come from above, diggers from below.

	double distance2 = (xdist * xdist) + (ydist * ydist);
	double distance  = sqrt (distance2);

	if (distance < (5. * std::sqrt(static_cast<double>(repulsion))) ) {
		double rep_mod = PT_DIGGING    == phys_type ? 0.15
		               : PT_DIRTBOUNCE == phys_type ? 0.66
		               : PT_SMOKE      == phys_type ? 0.75
		               : 1.;
		*xa = (repulsion * (xdist / distance) / distance2) * rep_mod * 0.75;
		*ya = (repulsion * (ydist / distance) / distance2) * rep_mod * 1.50;
		return true;
	}

	return false;
}



/// @brief Resets flash_damage and applies damage if flash_damage
/// is greater than half the FPS (meaning ~0.5 seconds) or the tank is dead.
void TANK::resetFlashDamage()
{
	if ( (flashdamage > (env.frames_per_second / 2)) || destroy ) {
		flashdamage = 0;
		if (ROUND(damage) > 0)
			applyDamage();
		requireUpdate();
	}
}


void TANK::setBitmap()
{
	if (!player)
		return;

	bool had_offsets = ((use_tankbitmap > -1) && (use_turretbitmap > -1));

	if (TT_NORMAL == player->tankbitmap) {
		use_tankbitmap   = 0;
		use_turretbitmap = 0;
	} else {
		use_tankbitmap   = player->tankbitmap + TO_TANK;
		use_turretbitmap = player->tankbitmap + TO_TURRET;
	}

	// Set needed offsets
	tank_off_x = ROUNDu( env.tank[use_tankbitmap]->w / 2);
	tank_off_y =         env.tank[use_tankbitmap]->h;
	tank_sag   = ROUNDu(static_cast<double>(tank_off_y) / 2.66);
	turr_off_x = ROUNDu( env.tankgun[use_turretbitmap]->w / 2);
	turr_off_y = ROUNDu(env.tankgun[use_turretbitmap]->h / 2) - 2;
	shld_rad_x = tank_off_x + (turr_off_x / 2) + 1;
	shld_rad_y = ((tank_off_y + turr_off_y) / 2) + 1;

	tank_dia   = FABSDISTANCE2(2. * std::max(tank_off_x, turr_off_x),
	                          tank_off_y + (std::min(turr_off_x, turr_off_y) / 2.),
	                          0., 0.);

	// If these are new offsets, the position of the tank is too low and must be fixed:
	if (!had_offsets)
		y -= (tank_off_y - tank_sag);

	// Be sure the placement is sane:
    assert( ((x - tank_off_x) > 2)                     && "Placement too far left");
    assert( ((x + tank_off_x) < (env.screenWidth - 3)) && "Placement too far right");

    // Without debug mode, this must be fixed:
    if ( (x - tank_off_x) < 3)
		x = tank_off_x + 3;
	if ( (x + tank_off_x) > (env.screenWidth - 4) )
		x = env.screenWidth - 4 - tank_off_x;
}


void TANK::setTextPositions(bool renew_colour)
{
	int32_t textpos = -12 - turr_off_x;

	if (sh > 0) {
		shieldText.set_pos (x, y + textpos);
		textpos -= 14;
		if (renew_colour)
			shieldText.set_color(TURQUOISE);
	} else
		shieldText.set_pos(-1, -1);

	healthText.set_pos (x, y + textpos);
	textpos -= 14;
	if (renew_colour)
		healthText.set_color(player ? player->color : WHITE);

	if (env.nameAboveTank) {
		nameText.set_pos (x, y + textpos);
		if (renew_colour)
			shieldText.set_color(player ? player->color : WHITE);
	}
}


bool TANK::shootClearance (int32_t targetAngle, int32_t minimumClearance,
                           bool &crashed)
{
	int32_t clearance = 2;
	double xmov = env.slope[targetAngle][0];
	double ymov = env.slope[targetAngle][1];
	double xpos = x + (xmov * (turr_off_x + clearance));
	double ypos = y + (ymov * (turr_off_x + clearance));
	bool   done = false;
	crashed     = false;

	while (!done) {
		xpos += xmov;
		ypos += ymov;

		if ( (ypos <= MENUHEIGHT)
		  || (xpos < 2) || (xpos > (env.screenWidth - 2) ) ) {
			clearance = minimumClearance; // done it! There can't be dirt any more!
			done      = true;
		} else {
			if (++clearance >= minimumClearance)
				done = true;
			else {
				if (PINK != getpixel(global.terrain, xpos, ypos))
					done = true;
			} // End of having to check a pixel
		} // End of being within screen bounds
	} // End of checking the path

	// If a minimum clearance lower than the screen width is sought,
	// check whether this results in a wall/ceiling hit.
	if ( (minimumClearance < env.screenWidth)
	  && ( ( env.isBoxed && (ypos <= MENUHEIGHT)
		  && ( (WALL_STEEL == env.current_wallType)
			|| (WALL_WRAP  == env.current_wallType) ) )
		|| ( (  WALL_STEEL == env.current_wallType)
		  && ( (xpos < 2 ) || (xpos > (env.screenWidth - 3)) ) ) ) ) {
		clearance = -1;
		crashed   = true;
	}

	return (clearance >= minimumClearance);
}


/// @brief this is used whenever a weapon really is triggered.
/// In simultaneous play this does not actually mean it is fired.
/// To fire another shot without the trigger action, call
/// activateCurrentSelection().
void TANK::simActivateCurrentSelection ()
{
	static char buf[6] = { 0x0 };

	if (env.turntype != TURN_SIMUL) {
		activateCurrentSelection();

		if (fire_another_shot)
			fire_another_shot--;
	}

	// allow naturals to happen again
	global.naturals_activated = 0;

	snprintf (buf, 5, "%d", l);
	healthText.set_text(buf);

	// avoid having key presses read in next turn
	clear_keybuf();
}


/** @brief return true if this tank landed on another tank
  *
  * This function checks to see if there is a tank directly below this
  * one. This is to determine if we landed on someone.
  *
  * @return true if the tank landed on another one, false otherwise
**/
bool TANK::tank_on_tank()
{
	TANK* lt         = nullptr;
	bool  found_tank = false;

	global.getHeadOfClass(CLASS_TANK, &lt);
	while (lt && !found_tank) {
		if ( (lt != this)
		  && (std::abs(lt->x - x) < tank_off_x)
		  && (lt->y > y)
		  && ((lt->y - y) < tank_off_y) )
			found_tank = true;
		else
			lt->getNext(&lt);
	}

	return found_tank;
}
