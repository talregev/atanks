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


#include "explosion.h"
#include "missile.h"
#include "decor.h"
#include "tank.h"
#include "player.h"
#include "beam.h"
#include "sound.h"
#include "aicore.h"


/* Note: If you wonder why the MISSILE ctor needs the AI_LEVEL, it is used
 *       for two things:
 *       1. Whether repulsion is considered for mind shots depends on the
 *          ai_level of the bot tracking the missile, and
 *       2. the SDI check must make sure to not re-test its own mind shots.
 */
MISSILE::MISSILE (PLAYER* player_, double xpos, double ypos,
                  double xvel, double yvel, int32_t weapon_type,
                  eMissileType missile_type, int32_t ai_level_,
                  int32_t delay_idx_) :
	PHYSICAL_OBJECT(MT_WEAPON == missile_type),
	ai_level(ai_level_),
	missileType(missile_type)
{
	this->player = player_;

#ifdef NETWORK
	char buffer[256];
	sprintf(buffer, "MISSILE %d %d %lf %lf %d",
			ROUND(xpos), ROUND(ypos),
			xvel, yvel, weapon_type);
	env.sendToClients(buffer);
#endif

	// Set position and movement
	x  = xpos;
	y  = ypos;
	xv = xvel;
	yv = yvel;

	// Get and set weapon/item data
	weapType = weapon_type;
	if (weapType < WEAPONS)
		weap = &weapon[weapType];
	else
		weap = &naturals[weapType - WEAPONS];
	setBitmap(env.missile[weap->picpoint]);
	drag       = weap->drag;
	mass       = weap->mass;
	noimpact   = weap->noimpact;

	// The maxVel value results in a small missile being able to be accelerated
	// by 25% over MAX_POWER, while a large Napalm Bomb can go up to 220%.
	maxVel = env.maxVelocity * (1.20 + (mass / (.01 * MAX_POWER)));
	DEBUG_LOG_PHY("PHYSICAL_OBJECT",
	              "env.maxVel: %5.2lf, mass: %5.2lf, obj.maxVel: %5.2lf",
	              env.maxVelocity, mass, maxVel)

	// Meteors and dirt balls are "volatile" and can not be accelerated
	// over MAX_POWER. (Pre-caution against "forever" going naturals)
	if ( ( (SML_METEOR <= weapType) && (LRG_METEOR    >= weapType) )
	  || ( (DIRT_BALL  <= weapType) && (SUP_DIRT_BALL >= weapType) ) )
		maxVel = std::min(maxVel, static_cast<double>(MAX_POWER));

	if ( (SML_METEOR <= weapType) && (LRG_METEOR >= weapType) ) {
		angle  =  rand () % 360;
		spin   = (rand () %  20) - 10;
		maxAge = MAX_METEOR_AGE;
	} else if (weapType == NAPALM_JELLY) {
		// Napalm grows, others do not:
		isGrowing      = true;
		growRadius     = 1;
		maxAge         = MAX_JELLY_AGE;
		allowDirtyWrap = false;
	} else {
		growRadius = weap->radius;
		if (FUNKY_BOMBLET == weapType)
			maxAge = (MAX_MISSILE_AGE / 7) + (rand() % (MAX_MISSILE_AGE / 4));
			// With MMA 15 seconds, this is 2 + [0;3] = [2;5] seconds
		else if (FUNKY_DEATHLET == weapType)
			maxAge = (MAX_MISSILE_AGE / 5) + (rand() % (MAX_MISSILE_AGE / 3));
			// With MMA 15 seconds, this is 3 + [0;4] = [3;7] seconds
		else
			maxAge = MAX_MISSILE_AGE;
	}

	// Finalize maxAge to be for frames, not seconds:
	maxAge *= env.frames_per_second;

	// Set funky colour of the funky bomblets/deathlets and add some maxAge
	// variation so they do not detonate in groups.
	if ( (weapType == FUNKY_BOMBLET) || (weapType == FUNKY_DEATHLET) ) {
		int32_t temp_number = rand() % 5;
		switch (temp_number)
		{
			case 0: funky_colour = makecol(200,   0,   0); break;
			case 1: funky_colour = makecol(  0, 200,   0); break;
			case 2: funky_colour = makecol(  0,   0, 200); break;
			case 3: funky_colour = makecol(200, 200,   0); break;
			case 4: funky_colour = makecol(200,   0, 200); break;
		}

		// Variation +/- 1 Second in frames:
		maxAge += (rand() % (2 * env.frames_per_second)) - env.frames_per_second;
	}

	// Some weapons must not wrap through dirt ceilings if the bottom
	// pixel they would warp into is occupied:
	if ( ( (SML_ROLLER <= weapType) && (SMALL_MIRV  >= weapType) )
	  || ( (CLUSTER    <= weapType) && (SUP_CLUSTER >= weapType) )
	  || ( (SML_NAPALM <= weapType) && (LRG_NAPALM  >= weapType) )
	  || (CLUSTER_MIRV == weapType) )
		allowDirtyWrap = false;

	// If this is a mind shot with a delay characteristic, like
	// the chain missile, set a delay range by its index.
	// This causes PHYSICAL_OBJECT::applyPhysics() to not detonate
	// the missile unless the set distance was travelled through
	// dirt. This simulates the clearing of the path by previous
	// missiles, so the AI can track those weapons better.
	if (MT_MIND_SHOT == missile_type)
		this->mindDelay = weap->radius * delay_idx_;

	// Otherwise add it to the chain:
	// Do not put mind shots in there or the object updaters
	// will not only try to apply physics, but use delete on
	// them when they get destroyed.
	else
		global.addObject(this);

}


MISSILE::~MISSILE ()
{
	// Take out of the chain:
	if (MT_MIND_SHOT != missileType)
		global.removeObject(this);
}


void MISSILE::applyPhysics ()
{
	int32_t above_ground = -1;

	// Increase age and get rid of the missile if it
	// is caught in some endless loop.
	if ( (++age > maxAge)
	  || (y < -65535) ) {
		hitSomething = true;
		if (MT_MIND_SHOT != missileType)
			trigger();
		else
			destroy = true;
		return;
	} else
		hitSomething = false;

	// Napalm grows first:
	if (isGrowing) {
		if (age < maxAge) {
			growRadius = ROUND(static_cast<double>(weap->radius)
								* (static_cast<double>(age)
										/ static_cast<double>(maxAge)));
			if (growRadius < 2)
				growRadius = 2;
		} else
			isGrowing = false; // Finished growing!
	}

	// Riot charges / blasts go off immediately:
	if ( (RIOT_CHARGE <= weapType) && (RIOT_BLAST >= weapType) ) {
		trigger();
		return;
	}


	// Normal and digging physic types need an angle
	// and can be repulsed
	if ( (PT_NORMAL == physType) || (PT_DIGGING == physType)) {

		if ( (SML_METEOR <= weapType) && (LRG_METEOR >= weapType) )
			angle = (angle + spin) % 360;
		else
			angle = ROUND(RAD2DEG(atan(yv / xv)) * 256./360.)
			      - 64 + ( xv < 0 ? 128 : 0);

		if ( (MT_MIND_SHOT != missileType)
		  || (RAND_AI_0P && RAND_AI_0P) )
			Repulse_Missile();
	}

	// === 1) Handle standard physics projectiles ===
	if (PT_NORMAL == physType) {
		// Standard physics can be applied
		PHYSICAL_OBJECT::applyPhysics ();

		// If a mind shot napalm jelly hit something, it is counted as
		// destroyed immediately.
		if (hitSomething
		  && (MT_MIND_SHOT == missileType)
		  && (NAPALM_JELLY == weapType) )
			destroy = true;

		// mirvs trigger above ground
		if ( !hitSomething
		  && ( (weapType == SMALL_MIRV) || (weapType == CLUSTER_MIRV) )
		  && (yv > 0) ) {

			above_ground = Height_Above_Ground();

			if ( (above_ground > 0)
			  && (above_ground < TRIGGER_HEIGHT) )
				hitSomething = true;
		}


		// Missiles that get too slow on a rubber floor, trigger when stopped.
		if ( !hitSomething && (WALL_RUBBER == env.current_wallType)
		  && (ROUND(y) >= (env.screenHeight - 2))
		  && ( (std::abs(xv) + std::abs(yv)) < 0.8) )
			hitSomething = true;


		// Unless something is hit, smoke might be produced:
		if ( !hitSomething && !global.skippingComputerPlay
		  && (MT_MIND_SHOT != missileType)
		  && !(rand() % (env.frames_per_second / 10)) ) {
			try {
				new DECOR (x, y,
							xv / env.frames_per_second,
							xv / env.frames_per_second,
							weap->radius / 20, DECOR_SMOKE, 0);
			} catch (std::exception) {
				perror ( "missile.cpp: Failed allocating memory for decor in applyPhysics");
			}
		}
	} // --- End of normal physic types ---

	// === 2) Handle rolling projectiles ===
	else if (PT_ROLLING == physType) {

		// check whether anything is hit
		int32_t round_x = ROUND(x);
		int32_t round_y = ROUND(y);
		if ( (x < 2)
		  || (x > (env.screenWidth  - 3) )
		  || (y > (env.screenHeight - 3) )
		  || (PINK != getpixel(global.terrain, round_x, round_y)) )
				hitSomething = true;

		else {
			// To honour the size of the rollers, they have the following
			// rules about the path they roll:
			// - The small roller can climb four and fall six pixels.
			// - The large roller can climb six and fall nine pixels.
			// - The death roller can climb nine and fall twelve pixels.
			int32_t maxClimb = SML_ROLLER == weapType ?  4 :
			                   LRG_ROLLER == weapType ?  6 :
			                   DTH_ROLLER == weapType ?  9 : 1;
			int32_t maxFall  = SML_ROLLER == weapType ?  6 :
			                   LRG_ROLLER == weapType ?  9 :
			                   DTH_ROLLER == weapType ? 12 : 1;

			// get next surface pixel
			float surfY = global.surface[ROUND(x+xv)].load(ATOMIC_READ) - 1;

			// Check whether the terrain is going down
			if (surfY > y) {
				// Do not fall more than 'maxFall' pixels if terrain gives way.
				if ( surfY <= (y + maxFall) )
					y  = surfY - 1;
				else
					y += maxFall;
				// Do not fall through the floor:
				if (y > (env.screenHeight - maxClimb))
					y =  env.screenHeight - maxClimb;
			}

			// Check whether the terrain is going up
			else if (surfY < y) {
				// If terrain is going up, it can be climbed,
				// or the rollers path ends here
				if ( surfY >= (y - maxClimb) )
					y = surfY - 1;
				else
					// too steep!
					hitSomething = true;
			}

			// Normal transversal movement:
			x += xv > 0 ? 1 : xv < 0 ? -1 : 0;

			// Adapt angle according to movement
			if (xv > 0.0) angle = (angle +   3) % 256;
			if (xv < 0.0) angle = (angle + 253) % 256;

			// Fix y if the projectile threats to go through the floor
			if (!hitSomething && (y > (env.screenHeight - 5)) )
				y = env.screenHeight - 5;
		} // End of rolling projectile movement
	} // --- End of rolling physic types ---

	// === 3) Handle funky projectiles ===
	else if (PT_FUNKY_FLOAT == physType)  {

		// Funky Floats have a 0.75% chance to randomly change their direction
		if (0 == (rand() % 150)) {

			int32_t floatee_action = rand() % 4;

			// Three possibilities:
			// A) 25% chance to reverse x movement
			// B) 25% chance to reverse y movement
			// C) 50% chance to pick a random target to home into.
			// If A is chosen and there is no x movement, or B is chosen and
			// there is no y movement, option C is pulled.
			if ( (1 == floatee_action) && (std::abs(xv) > 0.5))
				xv *= -1.;
			else if ( (3 == floatee_action) && (std::abs(yv) > 0.5))
				yv *= -1.;
			else {
				TANK* floatee_tgt = global.get_random_tank();
				if (floatee_tgt) {
					WEAPON* launchWeap = FUNKY_BOMBLET == weapType
					                   ? &weapon[FUNKY_BOMB]
					                   : FUNKY_DEATHLET == weapType
					                   ? &weapon[FUNKY_DEATH]
					                   : nullptr;
					double speed = ( launchWeap->launchSpeed
					               + ROUND( (launchWeap ? launchWeap->speedVariation : 0.0)
					                      * (launchWeap ? launchWeap->launchSpeed    : 0.0)
					                      * Noise(rand() % 1000000) ) )
					             * env.FPS_mod;
					double fdiff = ABSDISTANCE2(floatee_tgt->x, floatee_tgt->y, x, y);
					xv = (floatee_tgt->x - x) / fdiff * speed;
					yv = (floatee_tgt->y - y) / fdiff * speed;
				}
			}
		}

		// Funky floats simply bounce on borders.
		if ( ((x + xv) < 1) || ((x + xv) > (env.screenWidth - 1) ) )
			xv = -xv;
		else
			x += xv;

		// The same applies to the screen bottom,
		// but here according to floor type
		if ((y + yv) >= env.screenHeight) {
			if (WALL_RUBBER == env.current_wallType) {
				yv *= -BOUNCE_CHANGE;
				xv *=  0.95;
			} else if (WALL_SPRING == env.current_wallType) {
				yv *= -SPRING_CHANGE;
				xv *= 1.05;
			} else if ( (WALL_WRAP == env.current_wallType)
			         && env.isBoxed && env.do_box_wrap ) {
				y  = MENUHEIGHT + 1;
			} else {
				y  = env.screenHeight;
				yv = 0;
				hitSomething = true;
				age = maxAge;
			}
		}

		// On the screen top the direction is just reversed, even if
		// there is a ceiling in boxed mode. However, if it is a wrap
		// ceiling, and the ceiling wrap is enabled, got to the bottom.
		else if ( (y + yv) <= MENUHEIGHT) {
			if ( (WALL_WRAP == env.current_wallType)
			  && env.isBoxed && env.do_box_wrap ) {
				y  = env.screenHeight - 2;
			} else {
				yv *= -0.95;
				xv *=  0.95;
			}
		}

		// eventually apply yv
		y += yv;
	} // --- End of funky float physic types ---

	// === 4) Handle other projectiles ===
	else {

		// Check X:
		if ( ((x + xv) < 1) || ((x + xv) > (env.screenWidth - 1)) ) {
			if (WALL_RUBBER == env.current_wallType)
				xv *= -0.5;
			else if (WALL_SPRING == env.current_wallType)
				xv *= -SPRING_CHANGE;
			else if (WALL_WRAP == env.current_wallType)
				x  = xv > 0. ? 1 : env.screenWidth - 1;
			else {
				x  = xv < 0. ? 1 : env.screenWidth - 1;
				xv = 0;
				hitSomething = true;
			}
		}

		// Check Y :
		if ( ((y + yv) >= env.screenHeight) || ((y + yv) < MENUHEIGHT) ) {
			yv *= -0.5;
			xv *=  0.95;
		}

		// Apply gravitation, digging types are reversed and scorch the ground:
		if (PT_DIGGING == physType)
			yv -= env.gravity * 0.05 * env.FPS_mod;
		else
			yv += env.gravity * 0.05 * env.FPS_mod;

		// Apply velocity
		y  += yv;
		x  += xv;

	} // End of checking 'others'

	// Final check against the terrain
	if (!hitSomething && (y > MENUHEIGHT)
	  && (y < (env.screenHeight - 1)) ) {
		int32_t round_x = ROUND(x);
		int32_t round_y = ROUND(y);
		int32_t hitpix = getpixel(global.terrain, round_x, round_y);
		if ( ( (PT_DIGGING == physType) && (PINK == hitpix) )
		  || ( (PT_DIGGING != physType) && (PINK != hitpix) ) )
			hitSomething = true;
	}

	// No "ceiling drops" are triggered in boxed mode
	if (!hitSomething && (y <= MENUHEIGHT) && env.isBoxed) {
		yv = 0;
		hitSomething = true;
	}

	// Be sure any trigger/detonation is on screen ...
	if (hitSomething && (y <= MENUHEIGHT))
		y = MENUHEIGHT + 1;

	triggerTest();
}


/// @return The number of bounces done since the missile was fired
int32_t MISSILE::bounced() const
{
	return bounces;
}


/// @return -1 if the missile flies to the left, 1 if it flies to the right
/// or does not have any vertical movement
int32_t MISSILE::direction() const
{
	return SIGN(xv);
}


void MISSILE::draw ()
{
	if (destroy)
		return;

	// Do not draw mind shots
	if (MT_MIND_SHOT == missileType)
		return;

	// draw arrow impostor if it is above the screen
	if (y < MENUHEIGHT) {
		// Back up original values
		BITMAP* bbitmap = getBitmap();
		double  by      = y;
		int32_t bangle  = angle;

		// Set arrow values
		setBitmap(env.misc[3]);
		y      = MENUHEIGHT + (height / 2);
		angle  = 0;
		VIRTUAL_OBJECT::draw();

		// restore original values:
		setBitmap(bbitmap);
		y      = by;
		angle  = bangle;

		return;
	}

	// draw missile on the screen

	// Napalm jellies need a special drawing due to their growing nature.
	if (weapType == NAPALM_JELLY) {
		if (isGrowing)
			draw_Napalm_Blob(this, x, y, growRadius, age / weap->etime);
		else
			draw_Napalm_Blob(this, x, y, weap->radius, age / weap->etime);

	}   // end of napalm

	// try drawing a funky bomblet
	else if ( (FUNKY_BOMBLET  == weapType)
		   || (FUNKY_DEATHLET == weapType) ) {

		circlefill(global.canvas, x, y, 4, funky_colour );
		circle    (global.canvas, x, y, 5, BLACK );
		setUpdateArea( x - 10, y - 10, 20, 20);
		requireUpdate();
	}

	// draw anything else
	else {

		// Digging weapons scorch the earth they travel through
		if (PT_DIGGING == physType) {
			int32_t scorches = 3 + (3 * ABSDISTANCE2(x, y, x + xv, y + yv));

			for (int32_t i = 0; i < scorches; ++i) {
				int32_t sx = x + ((rand() % 5) - 2); // [-2;2]
				int32_t sy = y + ((rand() % 5) - 2); // [-2;2]

				if ( (sx > 1)          && (sx < env.screenWidth)
				  && (sy > MENUHEIGHT) && (sy < env.screenHeight) ) {
					int32_t pc = getpixel(global.terrain, sx, sy);
					if (PINK != pc) {
						putpixel(global.terrain, sx, sy, makecol(
							ROUNDu(static_cast<double>(getr(pc)) * .900),
							ROUNDu(static_cast<double>(getg(pc)) * .825),
							ROUNDu(static_cast<double>(getb(pc)) * .866)));
					}
				} // end of having valid coordinates
			} // end of looping scorches
		} // end of scorching

		VIRTUAL_OBJECT::draw();
	}
}


/// === Private methods ===
///=========================


/// @brief little helper struct to fire SDI lasers in a fairer way
struct sSDI
{
	int32_t am    = 0;
	double  dist  = 0.;   // Distance used for sorting
	double  lvl   = 0.;   // AI level, human players are counted as deadly.
	sSDI*   next  = nullptr;
	double  range = 100.; // The more SDI, the further the shot
	TANK*   tank  = nullptr;
	double  x     = 0.;
	double  y     = 0.;

};


// Check to see if any tanks have SDI defense. If they
// do, then see if this missile should be shot down.
// Returns the shooting tank if a shot is to be fired
// or NULL if no tank will shoot.
/* Update:
 * -------
 * Check_SDI() can do it all on its own and is now called from
 * triggerTest() if (and *only* if) the missile isn't going off
 * anyway.
 * This allows SDI to be spared on missiles that are exploding anyway.
 * Further it makes the game loop much simpler.
 * - Sven
 *
 * Update:
 * -------
 * If an SDI is really considered to fire, a MIND SHOT is now used to
 * determine where the missile will explode, or whether it'll miss.
 * Tests showed, that it will result in a lower average number of
 * iterations than the old per-pixel check. And as a nice side effect,
 * the result is many times more accurate. ;-)
 * - Sven
*/
void MISSILE::Check_SDI()
{
	static sSDI sdi[MAXPLAYERS];

	// The Predictor don't checks itself:
	if (SDI_PREDICTOR == ai_level)
		return;

	// can't shoot jelly, dirt balls, digging projectiles
	// and anything with submunition.
	if ( (PT_DIGGING == physType)
	  || (weapType == NAPALM_JELLY)
	  || (weap->submunition > 0) )
		return;

	// Funky Floats are "invisible" with a chance of 50%
	if ( (PT_FUNKY_FLOAT == physType)
	  && (rand() % 2) )
		return;

	// Reset SDI list:
	for (int32_t i = 0; i < MAXPLAYERS; ++i)
		sdi[i].next = nullptr;

	// Create SDI list
	TANK*   lt       = nullptr;
	bool    shotDown = false;
	sSDI*   pSDI     = nullptr;
	int32_t idx      = 0;

	global.getHeadOfClass(CLASS_TANK, &lt);
	while (lt) {
		/* A tank is not considered for SDI shots if:
		 * 1 The tank is destroyed (obviously)
		 * 2 The tank is this missiles owners tank
		 * 3 The tank is flying
		 * 4 The tank already has an SDI beam firing
		 * 5 The SDI has no shots left for this sequence
		 * 6 The SDI beam would shoot downwards.
		 */
		if ( !lt->destroy                                      // 1
		  && (lt->player != player)                            // 2
		  && !lt->isFlying()                                   // 3
		  && !lt->player->sdi_has_fired.load(ATOMIC_READ)      // 4
		  && (lt->player->ni[ITEM_SDI] > lt->player->sdiShots) // 5
		  && ( (lt->y - 10.) >= y) ) {                         // 6
			double startX = lt->x;
			double startY = lt->y - 10.;
		  	sdi[idx].am    = lt->player->ni[ITEM_SDI] - lt->player->sdiShots;
			sdi[idx].lvl   = static_cast<double>(
			                            ( ( lt->player->type == HUMAN_PLAYER )
			                           || ( lt->player->type  > DEADLY_PLAYER) )
			                            ? DEADLY_PLAYER : lt->player->type);
			sdi[idx].tank  = lt;
			sdi[idx].range = static_cast<double>(SDI_DISTANCE)
			               + ( static_cast<double>(sdi[idx].am - 1)
			                 * 2.5);
			sdi[idx].dist  = FABSDISTANCE2(x, y, startX, startY);
			sdi[idx].x     = startX;
			sdi[idx].y     = startY;

			/* Add the SDI to the list if:
			 * 1: The missile is within maximum range
			 * 2: but further away than the minimum distance and
			 * 3: no dirt is between the gun top and the missile.
			 */
			if ( (sdi[idx].dist <= sdi[idx].range)                // 1
			  && (sdi[idx].dist >  lt->player->ni[ITEM_SDI])      // 2
			  && !checkPixelsBetweenTwoPoints(&startX, &startY,
			                                  x, y, 0.0, nullptr) /* 3 */ ) {
				// This can be added!
				if (pSDI) {
					// Must be sorted in
					sSDI* curr = pSDI;
					while (curr->next && (curr->next->dist < sdi[idx].dist) )
						curr = curr->next;
					// Now curr->next is either nullptr or is farther away.
					sdi[idx].next = curr->next;
					curr->next    = &sdi[idx];
				} else
					// It is the new head
					pSDI = &sdi[idx];
				++idx;
			} // end of in range
		} // End of having SDI
		lt->getNext(&lt);
	} // End of looping tanks

	// Move through the sorted list of SDI stations and see whether anybody
	// can shoot this one down.
	while (!shotDown && pSDI) {
		// 20% base chance with +1% per SDI over one.
		if ( (rand() % 100) < (19 + pSDI->am) ) {
			// Try to predict the coordinates where the missile will go down:
			MISSILE mind_shot(player, x, y, xv, yv, weapType, MT_MIND_SHOT,
			                  SDI_PREDICTOR, 0);

			// Adapt missile drag if the player has dimpled/slick projectiles
			if (player->ni[ITEM_DIMPLEP])
				mind_shot.drag *= item[ITEM_DIMPLEP].vals[0];
			else if (player->ni[ITEM_SLICKP])
				mind_shot.drag *= item[ITEM_SLICKP].vals[0];

			// Keep flying/rolling/digging/whatever until the missile hits something
			// or the tank is out of explosion range
			double   x_dist    = pSDI->x - mind_shot.x;
			double   y_dist    = pSDI->y - mind_shot.y;
			double   x_vel     = 0.;
			double   y_vel     = 0.;
			uint32_t max_range = pSDI->lvl
			                   * std::max(ROUND(pSDI->range), weap->radius);
			mind_shot.getVelocity(x_vel, y_vel);

			// Apply physics until the missile is either destroyed, or
			// it is moving away from the tank and the tank is outside
			// the blast radius.
			while (!mind_shot.destroy
			    && ( (SIGN(x_dist) == SIGN(x_vel))
			      || (SIGN(y_dist) == SIGN(y_vel))
			      || (  ABSDISTANCE2(pSDI->x, pSDI->y, mind_shot.x, mind_shot.y)
			          < max_range) ) ) {
				mind_shot.applyPhysics();
				x_dist = pSDI->x - mind_shot.x;
				y_dist = pSDI->y - mind_shot.y;
				mind_shot.getVelocity(x_vel, y_vel);
			}

			// If the missile is destroyed, check whether the explosion would
			// a) hit this tank,
			// b) be nearer than the missile is now and
			// c) will not kill the tank if shot down.
			// If so, shoot it down!
			bool will_hit = false;
			if (mind_shot.destroy) {
				int32_t x_rad = DRILLER == weapType
				              ? weap->radius / 20
				              : weap->radius;
				int32_t y_rad = ( (SHAPED_CHARGE <= weapType)
				               && (CUTTER        >= weapType) )
				              ? weap->radius / 20
				              : weap->radius;

				if ( (std::abs(x_dist) <= x_rad) // tank in x range
				  && (std::abs(y_dist) <= y_rad) // tank in y range
				  && ( ( std::abs(x - pSDI->x) > x_rad) // misses x radius now
				    || ( std::abs(y - pSDI->y) > y_rad) // misses y radius now
				        // Is now farther away than when it goes off:
				    || (   ABSDISTANCE2(x, y, pSDI->tank->x, pSDI->tank->y)
				        >= ABSDISTANCE2(x, y, mind_shot.x, mind_shot.y) ) ) ) {

					// The point looks promising, but is it worth it?
					double dmg = get_hit_damage(pSDI->tank,
					                            static_cast<weaponType>(weapType),
					                            x, y);
					if (dmg < (pSDI->tank->sh + pSDI->tank->l))
						will_hit = true;
				}
			}

			if (will_hit) {
				shotDown = true;

				// The actual shooting is only done if this is no mind shot
				if (MT_MIND_SHOT != missileType) {
					lt = pSDI->tank;

					// The player has a 1% chance per SDI (with 50% max)
					// that one of the lasers burns out.
					// The chance can become this high to prevent players with
					// few missiles to shoot down to buy hundreds of SDI units.
					int32_t chance = pSDI->am > 50 ? 50 : pSDI->am;
					bool    burnt  = (rand() % 100) < chance ? true : false;

					try {
						new BEAM(lt->player, pSDI->x, pSDI->y, x, y,
									pSDI->am > 50 ? LRG_LAZER :
									pSDI->am > 25 ? MED_LAZER :
										SML_LAZER, burnt);

						trigger();

						if ( burnt )
							pSDI->tank->player->ni[ITEM_SDI]--;

						pSDI->tank->player->sdi_has_fired.store(true, ATOMIC_WRITE);

					} catch (...) {
						// Just not shot down. ;)
						shotDown = false;
					}
				} else
					// But the bot needs to know that its shot ends here
					destroy = true;
			} // end of not missing
		}
		pSDI = pSDI->next;
	} // End of going through SDI list
}


// This function returns the distance above ground of
// the missile.
int32_t MISSILE::Height_Above_Ground()
{
	int32_t rx = ROUND(x);

	if ( (rx < 1) || (rx >= env.screenWidth) )
		return -1;

	double  sx = xv / yv;
	double  px = x + sx;
	double  py = y + 1.;
	int32_t height = 1;

	while ( (py < env.screenHeight)
		 && (px > .9)
		 && (px < (env.screenWidth - .9) )
		 && ( (py < BOXED_TOP )
		   || (PINK == getpixel(global.terrain, px, py) ) ) ) {
		px += sx;
		py += 1.;
		++height;

		// If this is a wrapping wall, px must be wrapped of course
		if (WALL_WRAP == env.current_wallType) {
			if (px < 1.)
				px = env.screenWidth - 1. - (1. - std::abs(px));
			if (px > (env.screenWidth - 1.) )
				px = 1 + (env.screenWidth - 1. - px);
		}
	}

	return height;
}


// Modify xv/yv according to repulse shields in the vicinity of the missile
void MISSILE::Repulse_Missile()
{
	TANK*  lt     = nullptr;
	double xaccel = 0;
	double yaccel = 0;

	global.getHeadOfClass(CLASS_TANK, &lt);

	while (lt) {
		if ( !lt->destroy && (lt->player != player) ) {

			if (lt->repulse (x + xv, y + yv, &xaccel, &yaccel, physType)) {
				xv += xaccel;
				yv += yaccel;
			}
		}
		lt->getNext(&lt);
	}
}

void MISSILE::trigger ()
{
	// Create explosion
	try {
		new EXPLOSION (player, x, y, xv, yv, weapType, isWeaponFire);
	} catch (...) {
		perror ( "missile.cpp: Failed allocating memory for explosion in MISSILE::trigger");
	}

	// If the explosion is near a wrapping wall, a second "fake"
	// explosion must be generated to display the wrapping effect
	if ( (WALL_WRAP == env.current_wallType) && (weapType < WEAPONS) ) {
		int32_t left   = 0;
		int32_t top    = 0;
		int32_t x_rad  = weapon[weapType].radius;
		int32_t y_rad  = weapon[weapType].radius;

		// Driller is smaller
		if (DRILLER == weapType)
			x_rad /= 20;

		// shaped charges are flatter
		if ( (SHAPED_CHARGE <= weapType) && (CUTTER >= weapType) )
			y_rad /= 20;

		// Set wrapped x position
		if (x < x_rad)
			left = env.screenWidth + x;
		else if (x > (env.screenWidth - x_rad))
			left = x - env.screenWidth;

		// (possibly) set wrapped y position
		if (env.isBoxed && env.do_box_wrap) {
			if (y < (y_rad + MENUHEIGHT) )
				top = env.screenHeight + y;
			else if (y > (env.screenHeight - y_rad))
				top = y - env.screenHeight + MENUHEIGHT;
		}

		// If an x/y-position is found, generate second explosion:
		if ( left || top ) {
			int32_t new_x = left ? left : x;
			int32_t new_y = top  ? top  : y;
			try {
				new EXPLOSION (player, new_x, new_y, xv, yv, weapType, isWeaponFire);
			} catch (...) {
				perror ( "missile.cpp: Failed allocating memory for secondary explosion in MISSILE::trigger");
			}
		}
	}

	destroy = true;

	if (weapType < SML_METEOR)
		play_explosion_sound(weapType, x, 255, 1000);
	else
		play_natural_sound(weapType, x, 255, 1000);
}


void MISSILE::triggerTest ()
{
	bool    quell       = noimpact ? true : false;

	// No tests are needed, if a too high velocity has
	// just lacerated the projectile.
	if ( lacerated ) {
		if ( (MT_MIND_SHOT == missileType) || quell)
			// Only end of performance is needed to know.
			destroy = true;
		else
			trigger();
		return;
	}

	TANK*   lt          = nullptr;
	double  old_delta_x = xv;

	// Has it hit a tank?
	global.getHeadOfClass(CLASS_TANK, &lt);
	while (lt) {
		if ( !lt->destroy
		  && lt->isInBox(x, y, x, y) ) {
			hitSomething = true;
			if (MT_MIND_SHOT != missileType)
				lt->requireUpdate ();

			// Be sure the detonation does not take place
			// on the gun top.
			if (y < lt->y)
				y = lt->y; // I think we can live with this 'shift'.
		}
		lt->getNext(&lt);
	}

	// Unless quelled, check what is to be done
	bool do_check = (!quell && (y > MENUHEIGHT));

	// Only really check if something is hit, the y coordinate is through the
	// floor or a non-PINK pixel is in the way:
	if (do_check && !hitSomething && (y < env.screenHeight)) {
		// neither hit nor floor crunch, check the pixel:
		int32_t round_x = ROUND(x);
		int32_t round_y = ROUND(y);
		if ( (round_x >= 0) && (round_x < env.screenWidth) ) {
			do_check = (PINK != getpixel(global.terrain, round_x, round_y));
		}
	} // End of pixel check

	// Only continue if all checks passed:
	if (do_check) {

		// A roller changes from flying to rolling
		if ( (weapType >= SML_ROLLER)
		  && (weapType <= DTH_ROLLER)
		  && (PT_NORMAL == physType) ) {

			if (age > 1) {
				quell    = true; // No detonation, just switch to rolling
				physType = PT_ROLLING;
				age      = 0;

				// Set rolling start position and initial movement
				y -= 5;
				xv = 0;
				yv = 0;

				// Possibly fix x
				if (x <= 1) {
					x  = 1;
					xv = 1;
				} else if (x >= (env.screenWidth - 2)) {
					x  =  env.screenWidth - 2;
					xv = -1;
				}

				// Possibly fix y
				int32_t round_x = ROUND(x);
				int32_t surf_y  = global.surface[round_x].load(ATOMIC_READ);
				if ( (y >=  surf_y)         // y is surface or below
				  && (y <= (surf_y + 2) ) ) // but not buried more than 2 px
					y = surf_y - 1;

				// Set movement if not done already:
				if ( (xv > -0.9) && (xv < 0.9) ) {
					bool can_go_left  = (round_x > 3);
					bool can_go_right = (round_x < (env.screenWidth - 4));

					if (can_go_left || can_go_right) {
						if (can_go_left)
							can_go_left  = (PINK == getpixel(global.terrain,
							                                     round_x - 1, y));
						if (can_go_right)
							can_go_right = (PINK == getpixel(global.terrain,
							                                     round_x + 1, y));
					} // End of checking direction pixels

					if (can_go_left && can_go_right)
						// Prefer old movement direction
						xv = SIGN(old_delta_x);
					else if (can_go_left)
						xv = -1;
					else if (can_go_right)
						xv = 1;
					else
						// nothing worked, both paths are blocked.
						xv = rand() % 2 ? -1 : 1;
				}

				// If the roller is hammered into a wall, detonate it
				if ( ( (WALL_STEEL == env.current_wallType)
				    && ( (x <= 2) || (x >= (env.screenWidth - 3) ) ) )
				  || (env.isBoxed
				    && (y <= MENUHEIGHT)
				    && ( ( (WALL_WRAP  == env.current_wallType)
				        && ( !env.do_box_wrap
				          || (  global.surface[ROUND(x)].load(ATOMIC_READ)
				              < env.screenHeight) ) )
				      || (WALL_STEEL == env.current_wallType) ) ) ) {
					quell        = false;
					hitSomething = true;
				}
			} // End of switching roller physics
		} // End of roller handling

		// A burrower or penetrator changes from flying to digging
		else if ( (weapType == BURROWER) || (weapType == PENETRATOR) ) {
			if (PT_DIGGING == physType)
				// If it hit, it must not be quelled.
				quell = !hitSomething;
			else if (PT_NORMAL == physType) {
				// This is the switch
				physType = PT_DIGGING;
				quell    = true;
				xv      *= 0.1;
				yv      *= 0.1;
				age      = 0;
			}
		} // End of burrower handling

		// If a weapon has submunition, it goes off now
		else if (weap->submunition >= 0) {
			quell = true; // This one is done

			if ( (weap->numSubmunitions > 0) && (MT_MIND_SHOT != missileType) ) {
				WEAPON*   submunition     = &weapon[weap->submunition];
				double    divergenceStep  = static_cast<double>(weap->divergence)
				                          / static_cast<double>(weap->numSubmunitions - 1);
				int32_t   startPoint      = divergenceStep < 0. ? 0 : 180;
				int32_t   randStart       = rand () % 1000000;
				ePhysType submunitionPhys = PT_NORMAL;
				double    inheritedXV     = weap->impartVelocity * xv;
				double    inheritedYV     = weap->impartVelocity * yv;
				int32_t   startY          = y - 20;
				bool      ceiling_crash   = false;

				// See whether the weapon was fired into a ceiling:
				// This applies for both steel ceilings and wrap ceilings,
				// but the latter only if no ceiling wrap is activated or
				// if the next pixel at the bottom is dirt.
				if (env.isBoxed && (startY <= MENUHEIGHT) // Base condition
				  && ( ( (WALL_WRAP  == env.current_wallType)
					  && (!env.do_box_wrap // <- No wrap makes it steel
						// \/ dirt makes the ceiling unwrapable
				        || (  global.surface[ROUND(x)].load(ATOMIC_READ)
				            < env.screenHeight) ) )
				    || (  WALL_STEEL == env.current_wallType) ) ) { // This always blasts
					ceiling_crash = true;
					// If the weapon is fired into a ceiling, adapt starting y
					startY = MENUHEIGHT + 20;
				}

				// if napalm is going off, play its burn out sound
				if ( (weapType >= SML_NAPALM) && (weapType <= LRG_NAPALM))
					play_explosion_sound(weapType, x, 128 + (weap->radius / 2), 1000);

				// Change physics of the submunitions for the funky bombs
				if ( (weapType == FUNKY_BOMB) || (weapType == FUNKY_DEATH) )
					submunitionPhys = PT_FUNKY_FLOAT;

				// If this is a steel wall hit, the start point angle needs
				// to be adapted.
				if ( (WALL_STEEL == env.current_wallType) && !ceiling_crash) {
					if ( (CLUSTER <= weapType) && (SUP_CLUSTER >= weapType) ) {
						if ( x < 2 )
							startPoint -= weap->divergence + 1 + (rand() % 10);
						else if ( x > (env.screenWidth - 3) )
							startPoint += weap->divergence + 1 + (rand() % 10);
					} else if ( (SML_NAPALM <= weapType)
					         && (LRG_NAPALM >= weapType) ) {
						if ( x < 2 )
							startPoint -= 10 + rand() % 21;
						else if ( x > (env.screenWidth - 3) )
							startPoint += 10 + rand() % 21;
					}
				}

				// If this is a ceiling crash, the start point angle needs
				// to be erased.
				if (ceiling_crash) {
					startPoint = 0;
					if ( (SMALL_MIRV == weapType)
					  || (CLUSTER_MIRV == weapType) )
						inheritedYV = std::abs(inheritedYV);
				}

				// The spread can be created!
				for (int32_t sc = 0; sc < weap->numSubmunitions; ++sc) {
					MISSILE *newmis       = nullptr;
					double   launchSpeed  = weap->launchSpeed;
					int32_t  newMissCount = submunition->countdown;
					int32_t  newMissAngle = ROUND(
								  (divergenceStep * static_cast<double>(sc))
								+  static_cast<double>(startPoint)
								- (static_cast<double>(weap->divergence) / 2.) );

					// Manipulate angle if applicable
					if (weap->spreadVariation > 0.)
						newMissAngle += ROUND(
									  static_cast<double>(weap->divergence)
									* weap->spreadVariation
									* Noise(randStart + 1054 + sc) );

					// Be sure the angle is valid
					while (newMissAngle < 0)
						newMissAngle += 360;
					newMissAngle %= 360;

					// Manipulate number of submunition projectiles if applicable
					if (submunition->countVariation > 0) {
						newMissCount += ROUND(
									  static_cast<double>(submunition->countdown)
									* submunition->countVariation
									* Noise(randStart + 78689 + sc) );
						// This might go wrong, so be sure it doesn't
						if (newMissCount <= 0)
							newMissCount = 0;
					}

					// Manipulate launching speed if applicable
					if (weap->speedVariation > 0)
						launchSpeed += ROUND(
									  weap->speedVariation
									* weap->launchSpeed
									* Noise(randStart + 124786 + sc) );

					// Launch new submunition missile
					// Note on funky floats: They do *not* home in on random
					// tanks when started, it is just a possibility in
					// applyPhysics() *only*
					try {
						newmis = new MISSILE (player, x, startY,
									env.slope[newMissAngle][0]
									* launchSpeed
									* env.FPS_mod
									+ inheritedXV,
									env.slope[newMissAngle][1]
									* launchSpeed
									* env.FPS_mod
									+ inheritedYV, weap->submunition,
									missileType, ai_level, 0);
						newmis->physType  = submunitionPhys;
						newmis->countdown = newMissCount;
						newmis->setUpdateArea(newmis->x - 20, newmis->y - 20, 40, 40);
					} catch (...) {
						perror ( "missile.cpp: Failed to allocate memory for"
								 "newmis in MISSILE::triggerTest (CLUSTER)");
					}
				} // End of looping submunitions
			} // End of having submunition count

			destroy = true;
		} // End of having a submunition type defined

	}

	// If a countdown was set and this is old enough, this missile is no
	// longer quelled, regardless of what this means for this weapon type
	if ( (countdown >= 0) && (age >= countdown) )
		quell = false;

	// Riot charges always go of in an instant.
	if ( (weapType >= RIOT_CHARGE) && (weapType <= RIOT_BLAST) ) {
		quell   = false;
		destroy = true;
	}


	// Eventually trigger missiles that hit something or are lacerated
	if (hitSomething && !quell) {
		if (MT_MIND_SHOT == missileType)
			destroy = true;
		else
			trigger();
	} else if ( (yv > 0.)
	         && ( (weapType < RIOT_BOMB)
	           || (weapType > SMALL_DIRT_SPREAD) )
	         && (weapType < BALLISTICS) )
		// Otherwise it is time to check whether any tank SDI shoots it down
		Check_SDI();
}


/// @brief special method to update private members iof sub munition missiles.
/// This method is only interesting for AICore tracing clusters.
void MISSILE::update_submun(ePhysType p_type, int32_t cnt_down)
{
	physType  = p_type;
	countdown = cnt_down;
}

