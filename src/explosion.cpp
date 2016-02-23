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
 */

#include "main.h"

#include <cassert>

#include "environment.h"
#include "globaldata.h"
#include "explosion.h"
#include "missile.h"
#include "decor.h"
#include "tank.h"
#include "player.h"


/// @brief constructor for all detonations that are not caused by BEAMs
EXPLOSION::EXPLOSION (PLAYER* player_, double x_, double y_,
                      double xv_, double yv_, int32_t type, bool is_weapon) :
	PHYSICAL_OBJECT(is_weapon),
	impact_xv(xv_),
	impact_yv(yv_)
{
	xv = xv_;
	yv = yv_;

	if ( (TREMOR <= type) && (TECTONIC >= type) )
		angle = GET_ANGLE(xv, yv);
	else
		angle = GET_SAFE_ANGLE(xv, yv, 0);

	player = player_;
	drag = 0.95;
	mass = 3;
	x    = x_;
	y    = y_;
	maxVel = env.maxVelocity * (1.20 + (mass / (.01 * MAX_POWER)));

	WEAPON* weap = nullptr;
	weapType = type;
	if (weapType < WEAPONS)
		weap = &weapon[weapType];
	else
		weap = &naturals[weapType - WEAPONS];

	radius  = weap->radius;
	etime   = weap->etime;
	damage  = weap->damage;

	if ( ( (SHAPED_CHARGE <= weapType) && (CUTTER >= weapType) )
	  || ( DRILLER == weapType) ) {
	  	// Uses FlameFront
		explo_w  = FLAME_W;
		explo_h  = FLAME_H;
		centre_x = FLAME_CX;
		centre_y = FLAME_CY;
		flame_w  = static_cast<float>(radius) *  2.f;
		flame_h  = static_cast<float>(radius) / 10.f;
	} else {
		// explo_* and centre_* are already set
		flame_w  = static_cast<float>(radius) * 2.f;
		flame_h  = static_cast<float>(radius) * 2.f;
	}
	scale    = static_cast<float>(radius) / centre_x;

	// make sure dirt appears on the screen, not above the playing area,
	// and all other explosions at least reach into the area:
	int32_t minHeightMiss = MENUHEIGHT + (env.isBoxed ? 1 : 0);
	int32_t minHeightDirt = minHeightMiss + radius;
	if ( (weapType >= DIRT_BALL)
	  && (weapType <= SMALL_DIRT_SPREAD) ) {
		if (y < minHeightDirt)
			y = minHeightDirt;
	} else if ( (y < minHeightMiss)
			&& ( !env.isBoxed
			  || !env.do_box_wrap
			  || (WALL_WRAP != env.current_wallType) ) )
		y = minHeightMiss;

	// For all others
	// Some weapons have no damage to apply:
	// Note: Napalm Jellies deal damage over time and not at once.
	if ( (NAPALM_JELLY == weapType)
	  || ( (weapType >= RIOT_BOMB) && (weapType <= CLUSTER_MIRV) ) ) {
		apply_damage = false;
		// And those wouldn't throw debris or clear terrain either
		hasThrown  = true;
		hasCleared = true;
	} else if (env.debris_level > 0) {
		maxDebris = (radius * 2) / (8 - (2 * env.debris_level));
		maxFrame  = 2 + (2 * env.debris_level);

		// Tremor, Shockwave and Tectonic Shift must not throw debris around
		if ( (TREMOR <= weapType) && (TECTONIC >= weapType))
			maxDebris = 0;
		else if (maxDebris < 5)
			maxDebris = 5;
	}

	// Lasers and beams do only issue a tiny explosion to add the
	// dirt throw effect and some boom bang.
    if ( ( (weapType >= SML_LAZER)     && (weapType <= LRG_LAZER) )
	  || ( (weapType >= SML_LIGHTNING) && (weapType <= LRG_LIGHTNING) ) ) {
		apply_damage = false;
		etime        = 0;

		if (radius < 2)
			radius = 2;

		maxDebris   /= radius;

		if (maxDebris < 3)
			maxDebris = 3;
		else if (maxDebris > 6)
			maxDebris = 6;
	}

	// Napalm Jellies need a bit more variation:
	if (NAPALM_JELLY == weapType)
		curFrame = ( (rand() % (2 * env.frames_per_second))
		         - env.frames_per_second) / etime;

	// Unless this is a napalm jelly, that does not clear away any dirt,
	// lock our field of devastation so no sliding into the explosion occurs
	else
		global.addLandSlide(x - radius - 1, x + radius + 1, true);

	// Add to the chain:
	global.addObject(this);
}


/// This one is just for the beam and lightning dirt mills.
EXPLOSION::EXPLOSION (PLAYER* player_, double x_, double y_,
                      double xv_, double yv_, int32_t type, double damage_,
                      bool is_weapon) :
	// delegate base settings
	EXPLOSION(player_, x_, y_, xv_, yv_, type, is_weapon)
{
	damage = damage_;
}


/// @brief default dtor
EXPLOSION::~EXPLOSION ()
{
	// If this is a tremor, the land slide has to be released
	if ( (TREMOR <= weapType) && (TECTONIC >= weapType))
		global.unlockLandSlide(dim_cur.x, dim_cur.x + dim_cur.w);

	// Take out of the chain:
	global.removeObject(this);
}


/// @brief Physics for the Napalm Jelly, the only explosion that can 'move'
void EXPLOSION::applyPhysics ()
{
	if (NAPALM_JELLY == weapType) {
		if ( !global.skippingComputerPlay
		  && !(rand() % (env.frames_per_second / 2)) ) {
			try {
				new DECOR (x, y, 0, -2. * env.gravity * env.FPS_mod,
							radius / 2, DECOR_SMOKE, 0);
			} catch (std::exception) { /* No reason to fuss, its just smoke. */ }
		}

		// Instead of the radius, the current blob size is used
		// for hit and damage calculation. Thus a tank that is only
		// grazed by the full blob receives a lot less damage.
		double blobSize = radius - (
							  static_cast<double>(curFrame)
							/ static_cast<double>(EXPLOSIONFRAMES)
							* radius) + 1.;
		if ( blobSize < 1.)
			blobSize = 1.;

		// Stop all movement if dirt is hit:
		bool can_move = (y < env.screenHeight);

		if (can_move && (y < (env.screenHeight - 1))) {
			if (PINK != getpixel (global.terrain, x, y + 1))
				can_move = false;
		}

		if (can_move) {
			// If the dirt below is falling away, napalm can fall, too:
			PHYSICAL_OBJECT::applyPhysics();

			// And falling napalm can be repulsed
			TANK*  lt     = nullptr;
			double xaccel = 0;
			double yaccel = 0;

			global.getHeadOfClass(CLASS_TANK, &lt);

			while (lt) {
				if (!lt->destroy) {

					if (lt->repulse (x + xv, y + yv, &xaccel, &yaccel, physType)) {
						xv += xaccel;
						yv += yaccel;
					}
				}
				lt->getNext(&lt);
			}
		} else {
			xv = 0.;
			yv = 0.;
		} // End of normal physics

		// Enable next round checking, because dirt can fall
		// away so the jelly might follow.
		hitSomething = false;

		// Napalm keeps burning, check all tanks
		double in_rate_x  = 0.;
		double in_rate_y  = 0.;
		TANK*  lt         = nullptr;
		double damage_mod = blobSize / static_cast<double>(radius);

		global.getHeadOfClass(CLASS_TANK, &lt);

		while (lt) {

			if ( !lt->destroy
			  && lt->isInEllipse(x, y, blobSize, blobSize, in_rate_x, in_rate_y) ) {
				double full_rate = in_rate_x * in_rate_y;
				// If the tank is hit enough for the Napalm to "attach",
				// stop all movement:
				if ( (full_rate > 0.9) && (y >= lt->y) ) {
					xv = 0.;
					yv = 0.;
					hitSomething = true;
					// Note: When the blob shrinks, it can "fall off".
				}

				// Napalm is *HOT*. Never do less than 50% damage
				if (full_rate < 0.5)
					full_rate = 0.5;
				// Apply damage, but do it per frame
				lt->addDamage(player,
							  (  static_cast<double>(damage)
								* damage_mod * full_rate
								* (player ? player->damageMultiplier : 1.) )
						/ static_cast<double>(env.frames_per_second) );
			}

			lt->getNext(&lt);
		} // End of looping tanks
	} // End of NAPALM_JELLY special physics
}


/// @brief draw the explosions according to weapon type and shape
void EXPLOSION::draw()
{
	if ( (curFrame > 1) && (curFrame <= (EXPLOSIONFRAMES + 1))
	  && (NAPALM_JELLY != weapType) ) {
		/* This group includes:
		 * - all regular explosives,
		 * - all items and naturals,
		 * - tremor, shockwave and tectonic shift.
		 */
		int32_t flameIdx = curFrame - 2;
		int32_t rad      = (radius * curFrame) / EXPLODEFRAMES;

		switch (weapType) {
			case SHAPED_CHARGE:
			case WIDE_BOY:
			case CUTTER:
				rotate_scaled_sprite (global.canvas, env.gfxData.flameFront[flameIdx],
									x - radius,
									y - (radius / 20),
									itofix (0),
									ftofix (static_cast<double>(radius) / 300.) );

				setUpdateArea (x - radius - 1, y - (radius / 20) - 1,
							(radius + 1) * 2, ((radius / 20) + 1) * 2);
				break;
			case DRILLER:
				rotate_scaled_sprite(global.canvas, env.gfxData.flameFront[flameIdx],
									x - radius,
									y - (radius / 20),
									itofix(192),
									ftofix(static_cast<double>(radius) / 300.) );
				setUpdateArea(x - (radius / 20) - 1, y - radius - 1,
							((radius / 20) + 1) * 2, (radius + 1) * 2);
				break;
			case TREMOR:
			case SHOCKWAVE:
			case TECTONIC:
				if (curFrame <= (EXPLODEFRAMES + 1) ) {
					double tst_width = static_cast<double>(curFrame)
					                 / static_cast<double>(EXPLODEFRAMES)
					                 * static_cast<double>(radius) / 3.;
					drawFracture (x, y, angle,
					              static_cast<int32_t>(.75 + tst_width),
					              radius * 1.75,
					              (weapType - TREMOR + 1) * 3, 0);
					global.addLandSlide(dim_cur.x, dim_cur.x + dim_cur.w, true);
				}
				break;
			case RIOT_BOMB:
			case HVY_RIOT_BOMB:
				if (curFrame <= EXPLODEFRAMES) {
					int32_t colour = player ? player->color : BLUE;

					circlefill (global.terrain, x, y, rad, PINK);
					circle (global.canvas, x, y, rad, colour);

					setUpdateArea(x - radius - 1, y - radius - 1,
								(radius + 1) * 2, (radius + 1) * 2);
				} else if (!peaked) {
					// Do it here or the slide has an ugly delay.
					peaked  = true;
					do_clear();
				}
				break;
			case RIOT_CHARGE:
			case RIOT_BLAST:
				if (curFrame <= EXPLODEFRAMES) {
					double  sx  = x  - env.slope[angle][0] * 15;
					double  sy  = y  - env.slope[angle][1] * 15;
					int32_t x1  = sx + env.slope[(angle +  45) % 360][0] * rad;
					int32_t y1  = sy + env.slope[(angle +  45) % 360][1] * rad;
					int32_t x2  = sx + env.slope[(angle + 315) % 360][0] * rad;
					int32_t y2  = sy + env.slope[(angle + 315) % 360][1] * rad;

					triangle (global.canvas, sx, sy, x1, y1, x2, y2, player ? player->color : BLUE);
					triangle (global.terrain, sx, sy, x1, y1, x2, y2, PINK);

					setUpdateArea (sx - rad - 1, sy - rad - 1,
								(rad + 1) * 2, (rad + 1) * 2);
					global.addLandSlide(sx - rad - 1, sx + rad + 1, true);
				} else if (!peaked) {
					// Do it here or the slide has an ugly delay.
					peaked  = true;
					do_clear();
				}
				break;
			case SML_LAZER:
			case MED_LAZER:
			case LRG_LAZER:
			case SML_LIGHTNING:
			case MED_LIGHTNING:
			case LRG_LIGHTNING:
				setUpdateArea(x - radius - 1, y - radius - 1,
							(radius + 1) * 2, (radius + 1) * 2);
				break;
			case PERCENT_BOMB:
			default:
				if ( (weapType <= LAST_EXPLOSIVE)
				  || (weapType >= WEAPONS)
				  || (weapType == PERCENT_BOMB) ) {
					rotate_scaled_sprite(global.canvas, env.gfxData.explosions[flameIdx],
									x - radius, y - radius,
									itofix (0),
									ftofix (static_cast<double>(radius) / 107.) );
					setUpdateArea(x - radius - 1, y - radius - 1,
								(radius + 1) * 2, (radius + 1) * 2);
				} else if (curFrame <= EXPLODEFRAMES) {
					if ( !peaked
					  && (weapType >= DIRT_BALL)
					  && (weapType <= SMALL_DIRT_SPREAD) ) {
						BITMAP* tmp    = create_bitmap(rad * 2, rad * 2); // for mixing
						int32_t   colour = player ? player->color : GREEN;

						clear_to_color(tmp, PINK);

						circlefill(tmp, rad, rad, rad - 1, colour);

						// copy terrain over explosion
						masked_blit(global.terrain, tmp,
									x - rad, y - rad, 0, 0,
									rad * 2, rad * 2);

						// blit back exploded terrain
						masked_blit(tmp, global.terrain,
									0, 0, x - rad, y - rad,
									rad * 2, rad * 2);
						destroy_bitmap(tmp);
						setUpdateArea (x - rad - 1, y - rad - 1,
									(rad + 1) * 2, (rad + 1) * 2);
					} else if (REDUCER == weapType) {
						int32_t col_front = player ? player->color : SILVER;
						int32_t col_back  = GetShadeColor(PURPLE, false, col_front);

						int32_t col_mid   = makecol ( (getr(col_front) + getr(col_back)) / 2,
						                              (getg(col_front) + getg(col_back)) / 2,
						                              (getb(col_front) + getb(col_back)) / 2);
						circlefill (global.canvas, x, y, rad, col_front);

						for (int32_t i = 1 + (curFrame % 2); i < rad; i += 2)
							circle(global.canvas, x, y, i, i < (rad / 2) ? col_mid : col_back);
						setUpdateArea(x - rad - 1, y - rad - 1,
						              (rad + 1) * 2, (rad + 1) * 2);
					} else if (THEFT_BOMB == weapType) {
						int32_t col_front = GOLD;
						int32_t col_back  = GetShadeColor(BLACK, false, col_front);

						circlefill(global.canvas, x, y, rad,     col_front);
						circle    (global.canvas, x, y, rad / 2, col_back);

						setUpdateArea(x - rad - 1, y - rad - 1,
						              (rad + 1) * 2, (rad + 1) * 2);
					} else {
						// This is something else. But what?
						fprintf(stderr, "EXPLOSION::draw() Unknown weapon type %d\n",
									weapType);
						circlefill (global.canvas, x, y, rad, RED);
						setUpdateArea (x - rad - 1, y - rad - 1,
									(rad + 1) * 2, (rad + 1) * 2);
					}
				} else if (!peaked && (curFrame > EXPLODEFRAMES)
						&& (DIRT_BALL <= weapType)
						&& (SUP_DIRT_BALL >= weapType) ) {
					// Do it here or the slide has an ugly delay.
					peaked  = true;
					do_clear();
				}
				break;
		}
	} else if (NAPALM_JELLY == weapType) {
		// The Jelly does not use flameFront and is drawn individually.
		int32_t blobSize = curFrame > 0
		                 ? static_cast<int32_t>(radius
								- (   static_cast<double>(curFrame)
									/ static_cast<double>(EXPLOSIONFRAMES)
									* radius)) + 1
		                 : radius;
		if ( (blobSize > 0) && (curFrame <= (EXPLOSIONFRAMES + 1)) ) {
			if (blobSize < 2)
				// avoid circle size crash
				blobSize = 2;
			draw_Napalm_Blob(this, x, y, blobSize, curFrame);
		}
	}

	/// === Allow clearing once peaked on high enough frame ===
	///---------------------------------------------------------
	if (!peaked && (curFrame > EXPLODEFRAMES) && (NAPALM_JELLY != weapType)) {
		peaked  = true;
		do_clear();
	}

	/// === Throw if the frame is in range and it has not thrown, yet ===
	///-------------------------------------------------------------------
	if ( !hasThrown
	  && (curFrame > 1)
	  && (curFrame <= EXPLOSIONFRAMES)
	  && ( (weapType <= TECTONIC) || (weapType > REDUCER) )
	  && (NAPALM_JELLY != weapType) ) {

		do_throw();

		// The tremor types do not need clearing!
		if ( (TREMOR > weapType) || (TECTONIC < weapType) )
			do_clear();
	}
}


/// @brief Draw recursive fractures
void EXPLOSION::drawFracture(int32_t x, int32_t y, int32_t frac_angle,
                             int32_t width, int32_t segmentLength,
                             int32_t maxRecurse, int32_t recurseDepth)
{
	double xLen = env.slope[frac_angle][1] * width;
	double yLen = env.slope[frac_angle][0] * width;
	int32_t x1 = x + xLen;
	int32_t y1 = y + yLen;
	int32_t x2 = x - xLen;
	int32_t y2 = y - yLen;
	int32_t x3 = x + (env.slope[frac_angle][0] * segmentLength);
	int32_t y3 = y + (env.slope[frac_angle][1] * segmentLength);

	triangle (global.terrain, x1, y1, x2, y2, x3, y3, PINK);

	if (!recurseDepth) {
		dim_cur.x = x1;
		dim_cur.y = y1;
		dim_cur.w = x1;
		dim_cur.h = y1;
	} else {
		dim_cur.x = std::min(std::min(std::min(x1, x2), x3), dim_cur.x);
		dim_cur.y = std::min(std::min(std::min(y1, y2), y3), dim_cur.y);
		dim_cur.w = std::max(std::max(std::max(x1, x2), x3), dim_cur.w);
		dim_cur.h = std::max(std::max(std::max(y1, y2), y3), dim_cur.h);
	}

	if (recurseDepth < maxRecurse) {
		for (int32_t branchCount = 0; branchCount < 3; ++branchCount) {
			if ( branchCount || (Noise(x + y + branchCount) < 0)) {
				int32_t reduction = 2;
				int32_t newAngle  = frac_angle;

				switch(branchCount) {
				case 1:
					newAngle += 90 + (Noise (x + y + 25 + branchCount) * 22.5);
					reduction = ROUNDu(Noise(x + y + 1 + branchCount) * 4) + 3;
					break;
				case 2:
					newAngle += 270 + (Noise (x + y + 32 + branchCount) * 22.5);
					reduction = ROUNDu(Noise (x + y + 2 + branchCount) * 4) + 3;
					break;
				case 0:
				default:
					newAngle += Noise(x + y + 4) * 30;
					break;
				}

				while (newAngle < 0)
					newAngle += 360;
				newAngle %= 360;

				if (reduction < 2)
					reduction = 2;

				drawFracture (x3, y3, newAngle, width / reduction,
				              segmentLength / reduction, maxRecurse,
				              recurseDepth + 1);
			}
		}
	}

	// Calculate width and height, previously right and bottom
	if (!recurseDepth) {
		dim_cur.w -= dim_cur.x;
		dim_cur.h -= dim_cur.y;
	}
}


void EXPLOSION::explode ()
{
	/// === Time and frame advancement ===
	///------------------------------------
	if (curFrame <= (EXPLOSIONFRAMES + 1) ) {
		if (++exclock > etime) {
			exclock = 0;
			++curFrame;
			requireUpdate();

		}
	} // End of time and frame advancement

	// Check whether the explosion ends:
	if (curFrame > (EXPLOSIONFRAMES + 1))
		destroy = true;

	/// === Apply Damage if not done, yet ===
	///---------------------------------------
	if (apply_damage) {
		// In this case the affected tanks must be checked first
		TANK*      lt    = nullptr;
		weaponType wType = static_cast<weaponType>(weapType);

		// But do not check dirt balls, they deal no damage
		if ( (DIRT_BALL     > weapType)
		  || (SUP_DIRT_BALL < weapType) ) {

			global.getHeadOfClass(CLASS_TANK, &lt);

			while (lt) {
				double dmg = get_hit_damage(lt, wType, x, y);

				if (dmg > 0.) {
					if (PERCENT_BOMB == weapType)
						lt->addDamage(player, dmg); // already set, no multiplier
					else if (REDUCER == weapType) {
						// Note: dmg was set to a fake damage of 1.0
						lt->player->damageMultiplier *= 0.667; // already checked
					} else if ( (THEFT_BOMB == weapType)
					         && (lt->player != player) ) {
						// Note: dmg was set to a fake damage of 1.0
						int32_t max_amount = ROUND(player->damageMultiplier * 5000);
						int32_t amount     = lt->player->money <= max_amount
						                   ? lt->player->money
						                   : max_amount; // you have?

						// We indicate the theft by a red string on top of the tank
						static char the_money[17] = { 0x0 };
						snprintf(the_money, 16, "-$%s", Add_Comma(amount) );

						if (!global.skippingComputerPlay) {
							// show how much the shooter gets
							try {
								new FLOATTEXT(the_money,
								              lt->x, lt->y - 30,
								              .0, -.5, RED,
								              CENTRE,
								              env.swayingText ? TS_HORIZONTAL : TS_NO_SWAY,
								              200, false);
								if (global.stage < STAGE_SCOREBOARD)
									global.updateMenu = true;
							} catch (...) {
								perror("tank.cpp: Failed allocating memory for"
								       "money text in explode().");
							}
						}

						lt->player->money -= amount; // the actual theft.
						player->money     += amount; // money goes to the shooter.
					} else if (THEFT_BOMB != weapType)
						lt->addDamage(player, dmg
						            * (player ? player->damageMultiplier : 1.) );
				} // End of having damage to deal

				lt->getNext(&lt);
			} // End of looping tanks
		} // end of having no dirt ball
		apply_damage = false;
	} // End of handling damage
}


// ======================================
// === Private method implementations ===
// ======================================

/// @brief Clear the background (Display must be locked!)
void EXPLOSION::do_clear()
{
	if (hasCleared && hasSlid)
		return;

	// Use a calculated radius for pre-mature clearing
	int32_t rad = (radius * curFrame) / EXPLODEFRAMES;

	// If the radius is below 3, the early clearing would jeopardize
	// debris throwing, so opt out if the radius is much larger
	if ( (rad < 3) && (radius > 5) )
		return;

	// Do not clear/slide more than the real radius
	if (rad > radius)
		rad = radius;

	// Raise rad so no flood of "(rad + 1)" calculations is needed.
	int32_t area_rad = rad + 1;

	if (!hasCleared) {

		// Now clear according to weapon type
		if ( (weapType >= SHAPED_CHARGE) && (weapType <= CUTTER) ) {
			int32_t yrad = (rad - 1) / 20;
			ellipsefill (global.terrain, x, y, rad, yrad, PINK);
			global.addLandSlide(x - area_rad, x + area_rad, true);
			addUpdateArea (x - area_rad, y - (yrad + 1),
						area_rad * 2, (yrad + 1) * 2);
		} else if (weapType == DRILLER) {
			int32_t xrad = rad / 20;
			ellipsefill (global.terrain, x, y, xrad, rad, PINK);
			global.addLandSlide(x - xrad - 1, x + xrad + 1, true);
			addUpdateArea (x - (xrad + 1), y - area_rad,
						(xrad + 1) * 2, area_rad * 2);
		} else if (( (weapType <= LAST_EXPLOSIVE)
				  || (weapType >= WEAPONS)
				  || (weapType == PERCENT_BOMB)
				  || ( (weapType >= SML_LAZER)     && (weapType <= LRG_LAZER) )
				  || ( (weapType >= SML_LIGHTNING) && (weapType <= LRG_LIGHTNING) ) )
				&& (NAPALM_JELLY != weapType) ) {
			circlefill (global.terrain, x, y, rad, PINK);
			global.addLandSlide(x - area_rad, x + area_rad, true);
			addUpdateArea (x - area_rad, y - area_rad,
						area_rad * 2, area_rad * 2);
		}

	} // End of clearing

	// If rad did not reach radius (yet), this isn't done
	if ( (rad >= radius) || peaked) {

		if (!hasSlid) {
			// Allow the land slide to happen:
			int32_t area_rad = 1 + ((weapType == DRILLER) ? rad / 20 : rad);
			global.unlockLandSlide(x - area_rad, x + area_rad);
			hasSlid = true;
		}

		if (!hasCleared)
			hasCleared = true;
	}
}


/// @brief Throw some debris
void EXPLOSION::do_throw()
{
	// Do never throw when skipping AI play, and
	// opt out if debris generation is forbidden
	if (!hasThrown
	  && (  global.skippingComputerPlay
		|| (hasDebris >= maxDebris)
		|| (curFrame  >  maxFrame ) ) )
		hasThrown = true;

	// Early out if this is already done or no further deco is allowed
	if (hasThrown || global.hasTooMuchDeco)
		return;

	// The delay used for smoke and debris.
	int32_t delay_dirt  = (etime * (maxFrame - curFrame)) - exclock;
	int32_t delay_smoke = (etime * (EXPLODEFRAMES - curFrame)) - exclock;
	// Note: DECOR checks against delay>0, thus a negative delay
	//       is no problem here.

	// The radius is not always the radius, so use shortcuts:
	int32_t rad  = (radius * maxFrame) / EXPLODEFRAMES;
	int32_t xrad =    DRILLER       == weapType    ? rad / 20 : rad;
	int32_t yrad = ( (SHAPED_CHARGE <= weapType)
				  && (CUTTER        >= weapType) ) ? rad / 20 : rad;

	// A minimum radius of 1 is needed for the debris seek to make sense:
	if (rad < 1)
		return;

	// Use limited rounds for debris creation to ensure no
	// endless loops are created if there is no terrain to throw.
	int32_t deb_round  = 0;
	int32_t max_rounds = 3 * env.debris_level;

	// Initial velocity is modified by damage:
	double damage_mod = 1. + (static_cast<double>(damage)
							/ static_cast<double>(weapon[DTH_HEAD].damage)
							/ 2. );

	// If this is a meteor, the damage mod is tweaked or the debris more
	// looks like a collapsing rock than anything thrown.
	bool    isMeteor = ((SML_METEOR <= weapType) && (LRG_METEOR >= weapType));
	BITMAP* meteor   = nullptr;
	if (isMeteor) {
		meteor      = env.missile[naturals[weapType - WEAPONS].picpoint];
		damage_mod *= 2. * static_cast<double>(weapType - SML_METEOR + 1);
	}

	// Now move through the x-axis and create debris and smoke.
	double  xpos   = 0;
	double  ypos   = 0;
	double  bottom = env.screenHeight;
	double  alpha  = 0.;
	double  minX   = x - xrad;
	double  maxY   = 0.;
	int32_t round  = 1;
	int32_t deb_rad, diameter, seek_area = hasDebris % 3;
	double  max_x_vel, max_y_vel, mod_x_vel, mod_y_vel;

	do {
		deb_rad   = 1 + (rad > 1 ? (rand() % std::min(5, rad)) : 0);
		diameter  = 2 * deb_rad;
		max_x_vel =  16. - (static_cast<double>(deb_rad) * 2.0);
		max_y_vel = -17. + (static_cast<double>(deb_rad) * 1.5);

		// The mods are lower than the max, as the velocity values
		// can be modified up by damage and position.
		mod_x_vel = max_x_vel *  0.66;
		mod_y_vel = max_y_vel * -0.50; // Make positive

		// If this isn't weapon fire, reduce the maximum velocities
		if (!isWeaponFire) {
			max_x_vel *= 0.75;
			max_y_vel *= 0.66;
		}


		// Were the routine looks for dirt is determined by the
		// current amount of debris found:
		xpos = ( seek_area
				 ? 1 == seek_area
				   ? minX         // left
				   : x            // right
				 : x - (xrad / 2) // centre
			   ) + (rand() % xrad);

		/* Circle Coordinates:
		 * X = cos(alpha) * radius (xrad)
		 * Y = sin(alpha) * radius (yrad)
		 * alpha is the angle between x and y.
		 * cos(alpha) is x / xrad
		 * sin(alpha) is y / yrad
		 * So if we have alpha, we can produce Y.
		 */
		alpha = std::acos((xpos - x) / xrad);
		// Note: This results in radians, but that is okay,
		//       as sin() needs radians anyway.
		ypos  = y - ROUND(std::sin(alpha) * yrad);
		maxY  = std::min(y + (y - ypos), bottom - deb_rad);

        // find first earth pixel:
        if ( (ypos < y) && (maxY > ypos)
		  && checkPixelsBetweenTwoPoints(&xpos, &ypos, xpos, maxY, 0.0, nullptr)) {

			// Try to get a free debris pool item
			sDebrisItem* deb_item = global.get_debris_item(deb_rad);

			if (nullptr == deb_item) {
				// pool is full and at its limit.
				hasDebris = maxDebris;
				break;
			}

			// Try to get another free item if this is a meteor
			sDebrisItem* met_item = isMeteor
			                      ? global.get_debris_item(deb_rad)
			                      : nullptr;
			// Note: It is not a problem if a meteor got no met_item.

			// Move down a bit...
			ypos += 1 + (rand() % deb_rad);
			// ... but do not end up below maxY
			if (ypos > maxY)
				ypos = maxY;


			// Extract the pixels around xpos/ypos
			int32_t left = xpos - deb_rad;
			int32_t top  = ypos - deb_rad;

			// Blit in terrain
			blit(global.terrain, deb_item->bmp, left, top,
			     0, 0, diameter + 1, diameter + 1);

			// Blit in meteor if needed
			if (isMeteor && meteor && met_item)
				blit(meteor, met_item->bmp,
						left % ( meteor->w - diameter),
						top  % (meteor->h - diameter),
						0, 0, diameter + 1, diameter + 1);

			// Now the distance from the lowest centre point can be used
			// to determine the initial velocity of the debris.
			double dyp = y + radius + diameter;
			double dxv = ((xpos - x  ) / xrad  ) * mod_x_vel * damage_mod;
			double dyv = ((ypos - dyp) / radius) * mod_y_vel * damage_mod;

			assert((dyv < 0.) && "Check it: Dirt shall be thrown down?");

			// Modify x and y velocity a bit randomly for more variance
			dxv *= 1. + (static_cast<double>((rand() % 9) - 4) / 10.);
			dyv *= 1. + (static_cast<double>((rand() % 7) - 3) / 10.);

			// Apply impact velocity
			dxv -= impact_xv / ( (std::abs(dxv) * .75) + 1.5);
			dyv -= impact_yv / ( (std::abs(dyv) * .75) + 1.5);

			// Maximum x and y velocity depends on the radius of the debris:
			if (std::abs(dxv) > max_x_vel)
				dxv = SIGNd(dxv) * max_x_vel;
			if (dyv < max_y_vel)
				dyv = max_y_vel;

			// Move the decoration out to the rim of the final explosion:
			double rimx = x + ROUND(std::cos(alpha) * xrad);
			double rimy = y - ROUND(std::sin(alpha) * yrad);

			try {
				new DECOR(rimx, rimy, dxv, dyv,
						  deb_rad, DECOR_DIRT, delay_dirt, deb_item, met_item);
				// Clear the source to not take these pixels again.
				circlefill (global.terrain, xpos, ypos, deb_rad, PINK);
				addUpdateArea(xpos - deb_rad, ypos - deb_rad, diameter, diameter);
				addUpdateArea(rimx - deb_rad, rimy - deb_rad, diameter, diameter);
			} catch (...) {
				// As the decor was not created, the item can be released again
				global.free_debris_item(deb_item);
				if (met_item)
					global.free_debris_item(met_item);
			}

			// Every throw needs a smoke... ;)
			try {
				new DECOR(xpos, ypos, dxv, dyv,
						  deb_rad * 3, DECOR_SMOKE, delay_smoke);
			} catch (...) { /* nothing.. really... it doesn't matter */ }

			// Advance hasDebris and get new seeking area
			seek_area = ++hasDebris % 3;
        } // End of having hit a pixel

        // Count tries to advance rounds (if needed)
		if (++deb_round >= maxDebris) {
			deb_round = 0;
			++round;
		}
	} while ( (round < max_rounds) && (hasDebris < maxDebris) );

	// If the calculated radius did not reach radius (yet), and hasDebris
	// has not reached maxDebris, yet, the throwing is not finished, yet.
	if ( (rad >= radius) || (hasDebris >= maxDebris))
		hasThrown = true;

}


// =======================
// === Global helpers: ===
// =======================


/// @brief draw one blob of Napalm (display be locked!)
void draw_Napalm_Blob(VIRTUAL_OBJECT* blob, int32_t x, int32_t y,
					int32_t radius, int32_t frame)
{
	if (nullptr == blob)
		return;

	int32_t phase = std::abs(frame) % 4;
	int32_t lo_mod = phase % 2;
	int32_t hi_mod = (phase + 1) % 2;

	circlefill (global.canvas, x, y, radius, RED);

	for (int32_t i = 1; i < radius; i += 3) {
		int32_t r = 255 - (20 * phase);
		int32_t g =        25 * phase;
		int32_t b =        10 * phase;
		circle(global.canvas, x, y, i + lo_mod, makecol(r,      g, 10 + b));
		circle(global.canvas, x, y, i + hi_mod, makecol(r, 25 + g,      b));
	}

	blob->setUpdateArea (x - radius - 1, y - radius - 1, (radius + 1) * 2, (radius + 1) * 2);
	blob->requireUpdate ();
}


/** @brief return the damage of a weapon against a tank
  * @param[in] tank Pointer to the tank to check
  * @param[in] type The weapon used
  * @param[in] hit_x X coordinate of the impact
  * @param[in] hit_y Y coordinate of the impact
  * @return The part damage of the weapon without player modification
**/
double get_hit_damage(TANK* tank, weaponType type, int32_t hit_x, int32_t hit_y)
{
	if ( (nullptr == tank) || (tank->destroy) )
		return 0.;

	double weap_rad  = weapon[type].radius;
	double xrad      =    DRILLER       == type    ? weap_rad / 20. : weap_rad;
	double yrad      = ( (SHAPED_CHARGE <= type)
	                  && (CUTTER        >= type) ) ? weap_rad / 20. : weap_rad;
	double in_rate_x = 0.;
	double in_rate_y = 0.;
	double dmg       = 0.;

	if (tank->isInEllipse(hit_x, hit_y, xrad, yrad, in_rate_x, in_rate_y) ) {
		if (PERCENT_BOMB == type)
			dmg = ((tank->l + tank->sh) / 2) + 1;
		else if ( (REDUCER == type)
		       && (tank->player->damageMultiplier > 0.1) )
			dmg = 1.; // So result > 0 can be checked
		else if (THEFT_BOMB == type)
			dmg = 1.; // So result > 0 can be checked

		// Shaped charges and drillers have a minimum distance under which they
		// deal no damage:
		else if ( ( ( (SHAPED_CHARGE > type) || (CUTTER < type) ) //     ( Not shaped
		         || (std::abs(tank->x - hit_x) > yrad) )          //       or x distance okay )
		       && ( (DRILLER != type)                             // and (not driller
		         || (std::abs(tank->y - hit_y) > xrad) ) ) {      //       or y distance okay )
			/* Note: The radii are reversed as the opposite radius is the
			 *       minimum distance needed for the main blast radius.
			 * Note: The above is built from the following formula:
			 * Be a = Weapon is a shaped charge, wide boy or cutter,
			 *    b = x distance is greater than the weapons x range minimum,
			 *    c = Weapon is the driller,
			 *    d = y distance is greater than the weapons y range minimum.
			 * Then the following term must be true to be allowed to enter this block:
			 * (~a v b) ^ (~c v d) which reads like:
			 *       (Not a shaped weapon OR the x distance is in order)
			 *   AND (Not the driller     OR the y distance is in order)
			 * The above if block actually is exactly that from bottom to top.
			 */

			dmg = weapon[type].damage;

			// Some weapons have minimum rates on axis ratings
			if (DRILLER == type) {
				// The driller has its force focused vertically:
				if (in_rate_y < 0.95)
					in_rate_y = 0.95;
			} else if ( (SHAPED_CHARGE <= type)
			         && (CUTTER        >= type) ) {
				// The shaped ones have their force on the horizontal axis
				if (in_rate_x <  0.95)
					in_rate_x = 0.95;
			} else if ( (TREMOR        <= type)
			         && (TECTONIC      >= type) ) {
				if (in_rate_x < 0.25)
					in_rate_x = 0.25;
				if (in_rate_y < 0.25)
					in_rate_y = 0.25;
			}

			// The full in_rate must not be lower than 10% on any weapon.
			double in_rate = in_rate_x * in_rate_y;
			if (in_rate < 0.1)
				in_rate = 0.1;
			dmg *= in_rate;
		} // End of having damage to deal
	} // End of tank in ellipse

	return dmg;
}
