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
#include "physobj.h"
#include "player.h"
#include "decor.h"
#include "tank.h"
#include "beam.h"
#include "explosion.h"
#include "sound.h"

#include <cassert>


// Static helper for the drawing methods
static int32_t beamRadius = 1;
static int32_t beamSeed   = 0;

// Helper methods for the drawing methods
static void lazerPoint    (BITMAP *dest, int32_t x1, int32_t y1, int32_t color);
static void lightningPoint(BITMAP *dest, int32_t x1, int32_t y1, int32_t age);


/// @brief BEAM constructor
BEAM::BEAM (PLAYER* player_, double x_, double y_, int32_t fireAngle,
            int32_t weaponType, eBeamType beam_type) :
	PHYSICAL_OBJECT(BT_WEAPON == beam_type),
	beamType       (beam_type),
	tgtRightX      (env.screenWidth)
{
	this->player   = player_;
	this->weapType = weaponType;

	assert( ( ((weapType >= SML_LIGHTNING) && (weapType <= LRG_LIGHTNING))
		    ||((weapType >= SML_LAZER)     && (weapType <= LRG_LAZER)) )
		 && "ERROR: BEAM ctor called with something else than Lightning or Laser!");

#ifdef NETWORK
	char buffer[256];
	sprintf(buffer, "BEAM %d %d %d %d", (int) x_, (int) y_, fireAngle, weaponType);
	env.sendToClients(buffer);
#endif // NETWORK

	x     = x_;
	y     = y_;
	angle = fireAngle % 360;
	xv    = env.slope[angle][0];
	yv    = env.slope[angle][1];


	if (weapType < WEAPONS) weap = &(weapon[weapType]);
	else                    weap = &(naturals[weapType - WEAPONS]);
	radius = weap->radius;

	/* All beams should have the same age, no matter what the FPS settings
	 * are.
	 * Based on the default of 60 frames per second, the following frame
	 * lengths are wanted:
	 * Small lightning :  5 frames ( 1/12 second)
	 * Medium lightning: 10 frames ( 1/ 6 second)
	 * Large lightning : 15 frames ( 3/12 second)
	 * Lightning increase : 5 frames per size = 1/12 FPS
	 * Small laser     : 10 frames ( 1/6 second)
	 * Medium lightning: 20 frames ( 1/3 second)
	 * Large lightning : 30 frames ( 1/2 second)
	 * Laser increase     : 10 frames per size = 1/6 FPS
	*/
	int32_t age_per_size = env.frames_per_second / 12; // Doubled for laser
	int32_t base_age     = 5; // Doubled for laser
	int32_t weap_size    = 0; // aka "small"

	if ( (weapType >= SML_LIGHTNING) && (weapType <= LRG_LIGHTNING) ) {
		numPoints = 4 + (rand () % 9); // 4 - 12
		weap_size = weapType - SML_LIGHTNING;
	} else if ( (weapType >= SML_LAZER) && (weapType <= LRG_LAZER) ) {
		base_age     *= 2;
		age_per_size *= 2;
		numPoints     = 2;
		weap_size     = weapType - SML_LAZER;
		if (BT_SDI != beamType)
			// The SDI constructor produces its own color
			color = makecol(255 - ((weapType - SML_LAZER) * 64),
			                    128,
			                     64 + ((weapType - SML_LAZER) * 64));
		if ( !global.skippingComputerPlay
		  && ( (BT_WEAPON == beamType) || (BT_SDI == beamType) ) )
			play_fire_sound(weapType, x, 128 + (radius * 10), 1500 - (radius * 50));
	}

	maxAge = base_age + (age_per_size * weap_size);
	damage = static_cast<double>(weap->damage) / static_cast<double>(maxAge);

	// Set an offset seed
	seed = rand() % std::max(env.screenWidth, env.screenHeight);

	createBeamPath();

	// Now that the points are clear, a lightning bolt can emit its thunder:
	if ( !global.skippingComputerPlay
	  && (BT_NATURAL == beamType) )
		play_natural_sound(weapType, (points[0].x + points[numPoints - 1].x) / 2,
		                   175 + (radius * 10), 1000);

	// Add to the chain unless it is a mind shot:
	if (BT_MIND_SHOT != beamType)
		global.addObject(this);
}


/// @brief special constructor for SDI lasers
BEAM::BEAM(PLAYER* player_, double x_, double y_, double tx, double ty,
           int32_t weaponType, bool is_burnt_out) :
	BEAM(player_, x_, y_, GET_ANGLE(std::abs(ty - y_), tx - x_) + 90,
		weaponType, BT_SDI)
{
	if (player)
		++player->sdiShots;

	// SDI lasers are redder than normal, even more if burnt_out
	color = makecol(is_burnt_out ? 255 : 240 - ((weapType - SML_LAZER) * 16),
	                    is_burnt_out ?  32 :  64,
	                    is_burnt_out ? (weapType - SML_LAZER) * 32 : 128);

	// Limit the laser to the missiles coordinates
	points[numPoints - 1].x = tx;
	points[numPoints - 1].y = ty;
}



/// @brief BEAM destructor
BEAM::~BEAM ()
{
	requireUpdate();
	update();
	weap   = nullptr;
	if (points)
		delete [] points;
	points = nullptr;

	if (BT_MIND_SHOT != beamType) {
		global.make_bgupdate (dim_cur.x, dim_cur.y, dim_cur.w, dim_cur.h);
		global.make_bgupdate (dim_old.x, dim_old.y, dim_old.w, dim_old.h);

		// Let the land slide where the beam burned through:
		global.addLandSlide(tgtLeftX, tgtRightX, false);

		// Apply damage to all hit tanks:
		TANK* lt = nullptr;
		global.getHeadOfClass(CLASS_TANK, &lt);
		while (lt) {
			lt->applyDamage();
			lt->getNext(&lt);
		}

		// Take out of the chain:
		global.removeObject(this);

		// The player is allowed to fire one more SDI laser again:
		if ((BT_SDI == beamType) && player)
			--player->sdiShots;
	}
}


void BEAM::applyPhysics ()
{
	if (++age > maxAge)
		destroy = true;

	if (BT_SDI != beamType) {
		createBeamPath();
	}

	if (BT_MIND_SHOT != beamType) {
		if ( !global.skippingComputerPlay
		  && !(rand() % (env.frames_per_second / 5)) ) {
			try {
				new DECOR ( points[numPoints-1].x,
							points[numPoints-1].y,
							(rand () % 7) - 3,
							1 - (rand () % 6), radius, DECOR_SMOKE, 0);
			} catch (std::exception) {
				perror ( "beam.cpp: Failed to allocate memory for decor in applyPhysics");
			}
		}

		try {
			new EXPLOSION(player, points[numPoints-1].x, points[numPoints-1].y,
			              points[numPoints-1].x - points[0].x,
			              points[numPoints-1].y - points[0].y,
			              weapType, damage, isWeaponFire);
		} catch (std::exception) {
			perror ( "beam.cpp: Failed to allocate memory for explosion in applyPhysics");
		}
	}
}


void BEAM::draw()
{
	// never draw mind shots!
	if (BT_MIND_SHOT == beamType)
		return;

	int32_t oldDrawingMode = global.current_drawing_mode;

	drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
	global.current_drawing_mode = DRAW_MODE_TRANS;
	set_trans_blender (0, 0, 0, 50);

	beamRadius = radius;
	beamSeed   = seed;

	for (int32_t i = 1; i < numPoints; ++i) {
		int32_t left   = std::min(points[i - 1].x, points[i].x);
		int32_t top    = std::min(points[i - 1].y, points[i].y);
		int32_t right  = std::max(points[i - 1].x, points[i].x);
		int32_t bottom = std::max(points[i - 1].y, points[i].y);

		if ( (weapType >= SML_LIGHTNING) && (weapType <= LRG_LIGHTNING) )
			do_line (global.canvas,
			         points[i - 1].x, points[i - 1].y,
			         points[i    ].x, points[i    ].y,
			         age, lightningPoint);
		else if ( (weapType >= SML_LAZER) && (weapType <= LRG_LAZER) )
			do_line (global.canvas,
			         points[i - 1].x, points[i - 1].y,
			         points[i    ].x, points[i    ].y,
			         color, lazerPoint);

		addUpdateArea (left - radius, top - radius,
		               right  - left + (2 * radius),
		               bottom - top  + (2 * radius) );
	}

	drawing_mode(oldDrawingMode, NULL, 0, 0);
	global.current_drawing_mode = oldDrawingMode;

	requireUpdate ();
}


// === Private method implementations ===
// ======================================

/// @brief Create the basic points array with path tracing
void BEAM::createBeamPath()
{
	if (nullptr == points) {
		try {
			points = new POINT_t[numPoints];
		} catch (std::exception) {
			perror ( "beam.cpp: Failed to allocate memory for points in BEAM::createBeamPath()");
		}
	}

	// First determine the direct target - where does the beam end?
	double  tx = x, ty = y;
	hitSomething = false;

	// If this is not the first call, use the already known endpoints
	if ( ( points[0].x             || points[0].y
		|| points[numPoints - 1].x || points[numPoints - 1].y)
	  && !global.isDirtInBox( points[0].x, points[0].y,
	                               points[numPoints - 1].x,
	                               points[numPoints - 1].y) ) {
		tx = points[numPoints - 1].x;
		ty = points[numPoints - 1].y;
	} else {
		// The first point is the starting point, the last will become the target
		points[0].x = x;
		points[0].y = y;
	}

	while ( !hitSomething
	     && (tx > -radius)
	     && (tx < (env.screenWidth + radius))
	     && (ty > -radius)
	     && (ty < (env.screenHeight + radius)) ) {

		// Assume PINK for off screen pixels
		int32_t col = PINK;

		if ( (tx > 0)
		  && (tx < (env.screenWidth - 1))
		  && (ty > MENUHEIGHT)
		  && (ty < (env.screenHeight - 1)) )
			col = getpixel (global.terrain, tx, ty);

		if (PINK == col) {
			tx += xv;
			ty += yv;
		} else
			hitSomething = true;
	} // End of tracing pixels

	// tx and ty now result in the first obstacle (or screen border)
	// on a direct path.
	points[numPoints - 1].x = tx;
	points[numPoints - 1].y = ty;

	// If this is a lightning strike, points between the first and last
	// have to be (re-)generated.
	makeLightningPath();

	// Generate new lightning offsets checking for collisions:
	traceBeamPath();

	// If this is a mind_shot, it is immediately destroyed
	if (BT_MIND_SHOT == beamType)
		destroy = true;
}


/// @brief get the end of a mind shot laser
void BEAM::getEndPoint(int32_t& x, int32_t& y)
{
	x = points[numPoints - 1].x;
	y = points[numPoints - 1].y;
}


/// @brief create the lightning steps between the beginning and the end
void BEAM::makeLightningPath()
{
	if ( (numPoints > 2)
	  && (weapType >= SML_LIGHTNING)
	  && (weapType <= LRG_LIGHTNING) ) {
	  	int32_t maxP     = numPoints - 1;
		double  stepping = FABSDISTANCE2(points[0].x,    points[0].y,
		                                 points[maxP].x, points[maxP].y) / maxP;

		for (int32_t i = 1; i < maxP; ++i) {
			points[i].x = x
			            + (xv * (static_cast<double>(i) * stepping))
			            + (perlin2DPoint (1.0, 10. * radius,
			                        points[i].x + seed, points[i].y,
			                        0.3, 6) * radius * 10.);
			points[i].y = y
			            + (yv * (static_cast<double>(i) * stepping))
			            + (perlin2DPoint (1.0, 10. * radius,
			                        points[i].x, points[i].y + seed,
			                        0.3, 6) * radius * 10.);
		}
	} // End of lightning preparation
}


/// @brief this method is used by the satellite to move the beam with itself.
void BEAM::moveStart(double x_, double y_)
{
	x = x_;
	y = y_;
	if (points) {
		points[0].x = x;
		points[0].y = y;
	}
}


/// @brief walk through the beam points and check whether anything is hit
void BEAM::traceBeamPath()
{
	int32_t minRange = radius + 2;
	bool    canHit   = false;

	hitSomething = false;

	for (int32_t i = 1; !hitSomething && (i < numPoints); ++i) {
		double  startX   = points[i-1].x;
		double  startY   = points[i-1].y;
		double  endX     = points[i].x;
		double  endY     = points[i].y;
		bool    chkTanks = (BT_SDI == beamType)
		                 ? false
		                 : global.areTanksInBox(startX, startY, endX, endY);
		bool    chkDirt  = global.isDirtInBox  (startX, startY, endX, endY);

		// Break this if there is nothing possibly in between
		if (!(chkTanks || chkDirt) )
			continue;

		int32_t range    = 0;
		double  distX    = endX - startX;
		double  distY    = endY - startY;
		double  absX     = std::abs(distX);
		double  absY     = std::abs(distY);
		double  moveX    = distX / (absX > absY ? absX : absY);
		double  moveY    = distY / (absY > absX ? absY : absX);
		int32_t toMove   = ROUND(std::max(absX, absY));

		// Now wander along the path:
		while ( !hitSomething
			  && (range < toMove)
		      && (startX > 0)
		      && (startX < (env.screenWidth - 1))
		      && (startY > MENUHEIGHT)
		      && (startY < (env.screenHeight - 1))
			  && ( !chkDirt
				|| (PINK == getpixel (global.terrain, startX, startY))) ) {

			// Only check for tanks if the total range is large enough
			// and if there are tanks in the path
			if ((range >= minRange) && !canHit)
				canHit = true;
			if (canHit && chkTanks) {
				TANK*  lt        = nullptr;
				global.getHeadOfClass(CLASS_TANK, &lt);
				while (lt) {
					// Tank found, is it hit?
					if (!lt->destroy
					  && lt->isInBox(startX - radius, startY - radius,
									 startX + radius, startY + radius) ) {
					  	hitSomething = true;
						lt->requireUpdate();

						// 'Lock' the beam end on the tank:
						if (startY < (lt->y + radius) )
							startY = lt->y + radius;
						if (startY > (lt->y + (2 * radius)) )
							startY = lt->y + (2 * radius);
						if (startX < (lt->x - (radius / 2)))
							startX = lt->x - (radius / 2);
						if (startX > (lt->x + (radius / 2)))
							startX = lt->x + (radius / 2);

						// Get the in_rates
						double in_rate_x, in_rate_y;
						if ( (BT_MIND_SHOT != beamType)
						  && lt->isInEllipse(startX, startY, radius, radius,
						                     in_rate_x, in_rate_y) ) {
							double in_rate = in_rate_x * in_rate_y;
							if (in_rate < 0.9)
								// Beams do not 'splash'.
								in_rate = 0.9;

							lt->addDamage(player, static_cast<double>(damage)
									   * in_rate
									   * (player ? player->damageMultiplier : 1.) );
						}
						// That's it
						lt = nullptr;
						moveX = 0.;
						moveY = 0.;
					} // End of having a tank
					else
						lt->getNext(&lt);
				} // End of looping tanks
			}

			// Advance startX/Y
			startX += moveX;
			startY += moveY;
			++range;
		} // End of regular check

		// If dirt was hit, hitSomething must be adapted
		if ((PINK != getpixel (global.terrain, startX, startY)))
			hitSomething = true;

		if (hitSomething
		  && ( (ROUND(startX) != points[numPoints - 1].x)
		    || (ROUND(startY) != points[numPoints - 1].y) ) ) {
			// Reset the points to the new circumstances:
			points[numPoints - 1].x = ROUND(startX);
			points[numPoints - 1].y = ROUND(startY);
			makeLightningPath();

			// Note down x position for dirt slide on destruction:
			if ((startX - radius - 1) < tgtLeftX)  tgtLeftX  = startX - radius - 1;
			if ((startX + radius + 1) > tgtRightX) tgtRightX = startX + radius + 1;
		} // End of checking pixels
	} // End of looping points
}


// === static helper methods ===
// =============================
static void lazerPoint (BITMAP *dest, int32_t x1, int32_t y1, int32_t color)
{
	circlefill (dest, x1, y1, beamRadius, color);
}

static void lightningPoint (BITMAP *dest, int32_t x1, int32_t y1, int32_t age)
{
	double pRad = (perlin2DPoint (1.0, 2,  x1 + age, y1 + beamSeed, 0.3, 6) + 1)
	            / 2 * beamRadius + 1;
	double offX = (perlin2DPoint (1.0, 10 * pRad, x1 + age + beamSeed, y1 + age, 0.3, 6) + 1)
	            * pRad / 2.;
	double offY = (perlin2DPoint (1.0, 10 * pRad, x1 + age, y1 + age + beamSeed, 0.3, 6) + 1)
	            * pRad / 2.;

	circlefill (dest, x1 + offX, y1 + offY, pRad, WHITE);
}
