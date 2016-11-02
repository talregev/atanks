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


#include "physobj.h"
#include "environment.h"
#include "globaldata.h"


PHYSICAL_OBJECT::PHYSICAL_OBJECT(bool is_weapon) :
	VIRTUAL_OBJECT(),
    isWeaponFire(is_weapon)
{ /* nothing to do here */ }


void PHYSICAL_OBJECT::initialise()
{
	VIRTUAL_OBJECT::initialise();
	hitSomething = false;
}


/// @brief return true if this object was fired from a player weapon
bool PHYSICAL_OBJECT::isWeapon()
{
	return isWeaponFire;
}


/// @brief get the current velocity. Only important for AICore to track clusters.
void PHYSICAL_OBJECT::getVelocity(double &xv_, double &yv_)
{
	xv_ = xv;
	yv_ = yv;
}


/// @brief check whether the object hit something and return true if it has
/// Note: The objects x and y position is updated to the impact coordinates
///       if it hit anything.
bool PHYSICAL_OBJECT::checkPixelsBetweenPrevAndNow()
{
	double startX = x - xv;
	double startY = y - yv;

	if (checkPixelsBetweenTwoPoints(&startX, &startY, x, y, mindDelay, &mindPassed)) {
		x = startX;
		y = startY;
		return true;
	}

	return false;
}



/** @brief applyPhysics
  *
  * Moves the object according to momentum and wind, bounces off of
  * walls/ceiling/floor, and checks whether something is hit.
  *
  * @return true if something was hit, false otherwise.
  */
void PHYSICAL_OBJECT::applyPhysics()
{
	// Apply wind to x-movement
	xv += (global.wind - xv) / mass * drag * env.viscosity;

	// Apply gravity to y movement
	yv += env.gravity * env.FPS_mod;

	// Barrier test:
	if ( (yv <= -1.0) && (y <= (env.screenHeight * -25.0)))
		yv *= -1.0;

	bool isMoving = (std::abs(xv) + std::abs(yv)) < 0.01 ? false : true;

	if (!isMoving)
		return; // early out

	/* There are 6 steps:
	 * 1. Does the object hit a wall?
	 * 2. Does it hit something else before reaching the wall?
	 * 3. If nothing is hit, do the wall handling.
	 * 4. If nothing is hit and if movement is left, determine movement delta
	 *    and continue with 1.
	 * 5. Does the object hit something on its way to its final destination?
	 * 6. If nothing is hit, check the object velocity and detonate if too fast.
	 */

	// Shortcuts:
	bool    hasTop   = env.isBoxed;
	int32_t left     = 1;
	int32_t right    = env.screenWidth - 2;
	int32_t top      = MENUHEIGHT + (hasTop ? 1 : 0);
	int32_t bottom   = env.screenHeight - 2;
	double  xv_cur   = xv;
	double  yv_cur   = yv;

	// Special handling for Napalm Jellies if this is wrap or steel
	// ceiling. They sort of 'glide off' of the ceiling instead of
	// getting glued to it.
	bool jelly = (NAPALM_JELLY == weapType)
	          && ( (WALL_STEEL == env.current_wallType)
	            || ( (WALL_WRAP  == env.current_wallType)
	              && (!env.isBoxed || !env.do_box_wrap) ) )
	           ? true : false;

	// Easiest way is a loop that traces the path step-wise
	while (isMoving && !hitSomething) {
		double currX    = x;
		double currY    = y;
		double nextX    = x + xv_cur;
		double nextY    = y + yv_cur;
		double deltaX   = 0.;
		double deltaY   = 0.;
		double hitX     = 0.; // <0 = left, >0 = right
		double hitY     = 0.; // <0 = top,  >0 = bottom
		bool   hitWall  = false;
		bool   hitFloor = false;
		bool   hitLeft  = false; // Helper to not needing to check deltaX against 0.
		bool   hitTop   = false; // Helper to not needing to check deltaY against 0.

		// === 1.: Does the object hit a wall ? ===
		// ========================================
		if (nextX < left  ) {
			deltaX  = left - x;
			hitX    = deltaX / xv_cur;
			hitWall = true;
			hitLeft = true;
		} else if (nextX > right ) {
			deltaX  = right - x;
			hitX    = deltaX / xv_cur;
			hitWall = true;
		}
		if (nextY > bottom) {
			deltaY   = bottom - y;
			hitY     = deltaY / yv_cur;
			hitFloor = true;
		} else if (hasTop && (nextY < top) ) {
			deltaY   = top - y;
			hitY     = deltaY / yv_cur;
			hitFloor = true;
			hitTop   = true;
		}

		// Note: hit[XY] is now the percentage of the full movement to the hit.

		if (hitWall || hitFloor) {
			// === 2. Does it hit something else before reaching the wall? ===
			// Note: This is just preparation, check 5 can do this once prepared.
			// ===============================================================
			if (!hitFloor || (hitWall && (std::abs(hitX) <= std::abs(hitY))) ) {
				// The X-movement hits a wall earlier or at the same time (corner).
				nextX    = hitLeft ? left : right;
				deltaY   = yv_cur * hitX;
				nextY    = y + deltaY;
				hitFloor = false; // not reached
			} else {
				// The Y-movement hits bottom/top earlier
				nextY   = hitTop ? top : bottom;
				deltaX  = xv_cur * hitY;
				nextX   = x + deltaX;
				hitWall = false; // not reached
				if (jelly && hitTop) {
					nextY += 1.0;
					yv      = static_cast<double>( (rand() % 10) + 1 ) / 25.00; // 0.04 - 0.40
					xv     /= static_cast<double>( (rand() %  4) + 2 ) /  1.66; // 1.20 - 3.01
				}
			}
			xv_cur -= deltaX;
			yv_cur -= deltaY;
		}

		// === 5. Does the object hit something on its way to its final  ===
		// ===    destination?                                           ===
		// Note: This is done before the wall handling (step 3/4) as it also
		//       terminates the movement towards a wall. Only if the path
		//       really is clear wall handling makes sense.
		// =================================================================
		if (checkPixelsBetweenTwoPoints(&currX, &currY, nextX, nextY, mindDelay, &mindPassed)) {
			xv_cur = currX - nextX;
			yv_cur = currY - nextY;
			nextX  = currX;
			nextY  = currY;

			if (PT_DIRTBOUNCE == physType) {
				double rxv, ryv;
				getDirtBounceReact(nextX, nextY, xv, yv, rxv, ryv);

				// Modify rxv/ryv, this is no full bounce:
                if (std::abs(rxv) > std::abs(ryv)) {
					rxv *= 0.66;
					if (ryv < 0.)
						ryv *= 0.5;
                } else {
					rxv *= 0.5;
					if (ryv < 0.)
						ryv *= 0.66;
                }

				// See how much of the current movement is left
				double vel_rest = FABSDISTANCE2(xv_cur, yv_cur, 0., 0.)
				                / FABSDISTANCE2(xv,     yv,     0., 0.);

				// Now apply what is left:
				xv_cur = rxv * vel_rest;
				yv_cur = ryv * vel_rest;
				xv = rxv;
				yv = ryv;
			} else {
				hitSomething = true;
				isMoving = false;
			}

			hitWall  = false;
			hitFloor = false;
		} else if (hitWall || hitFloor) {
			// === 3. If nothing is hit, do the wall handling. ===
			// ===================================================

			// Note: Dirt bounce must be done first, it is handled
			//       differently but for x-wrapping
			if ( (PT_DIRTBOUNCE == physType)
			  && ( (WALL_WRAP != env.current_wallType)
				|| hitFloor) ) {
				if (hitWall) {
					xv_cur *= -0.5;
					xv     *= -0.5;
					if (yv < 0.) {
						yv_cur *= 0.66;
						yv     *= 0.66;
					}
				} else {
					// Yes, dirt does not wrap though ceilings.
					xv_cur *= 0.66;
					xv     *= 0.66;
					yv_cur *= nextY >= bottom ? -0.5 : -1.0;
					yv     *= nextY >= bottom ? -0.5 : -1.0;
				}
			} else {
				// count the bounce:
				++bounces;

				switch(env.current_wallType) {
					case WALL_RUBBER:
						if (hitWall) {
							xv_cur  = -xv_cur * BOUNCE_CHANGE;
							xv      = -xv     * BOUNCE_CHANGE;
							yv_cur *= BOUNCE_CHANGE;
							yv     *= BOUNCE_CHANGE;
						} else {
							yv_cur  = -yv_cur * BOUNCE_CHANGE;
							yv      = -yv     * BOUNCE_CHANGE;
							xv_cur *= BOUNCE_CHANGE;
							xv     *= BOUNCE_CHANGE;
						}
						break;
					case WALL_SPRING:
						if (hitWall) {
							xv_cur = -xv_cur * SPRING_CHANGE;
							xv     = -xv     * SPRING_CHANGE;
						} else {
							yv_cur = -yv_cur * SPRING_CHANGE;
							yv     = -yv     * SPRING_CHANGE;
						}
						break;
					case WALL_WRAP:
						if (hitWall) {
							if (hitLeft) nextX = right;
							else         nextX = left;
						} else if (env.isBoxed && env.do_box_wrap) {
							if (hitTop) {
								// Some weapons do not warp through the
								// ceiling if the bottom pixel is occupied
								// and trigger instead:
								int32_t bX = ROUNDu(nextX);
								bool floor_free = global.surface[bX].load(ATOMIC_READ)
								                >= bottom;
								if (allowDirtyWrap || floor_free)
									nextY = bottom;
								else {
									yv *= -1.;
									hitSomething = true;
								}
							} else
								nextY = top;
						} else
							hitSomething  = true;
						break;
					case WALL_STEEL:
					default:
						hitSomething = true;
						break;
				} // End of wall type switch
			}

		} // End of nothing hit, wall handling needed


		// === 6. If nothing is hit, check the object velocity and ===
		// ===    detonate if too fast. (depending on the mass)    ===
		// ===========================================================
		if (!hitSomething) {
			double actVel = FABSDISTANCE2(xv_cur, yv_cur, 0, 0); // a²+b²=c² ... says Pythagoras :)
			if ( (actVel > maxVel)
			  || std::isinf(xv_cur) || std::isinf(yv_cur)
			  || std::isinf(xv)     || std::isinf(yv)     ) {
				// apply *some* velocity, as the thing is killed on its way
				// (unless the current veocity is infinite of course
				double velMod = 1.0 + ((double)(rand() % 40) / 10.0);
				// This produces something between 1.0 and 5.0
				if (!std::isinf(xv_cur))
					nextX = x + (xv_cur / velMod);
				if (!std::isinf(yv_cur))
					nextY = y + (yv_cur / velMod);
				xv = 0.0;
				yv = 0.0;
				if (nextY < top)
					nextY = top;
				else if (nextY > bottom)
					nextY = bottom;
				if (nextX < left) {
					if (WALL_WRAP == env.current_wallType)
						nextX = right - (static_cast<int32_t>(std::abs(nextX)) % right);
					else
						nextX = left;
				} else if (nextX > right) {
					if (WALL_WRAP == env.current_wallType)
						nextX = static_cast<int32_t>(nextX) % right;
					else
						nextX = right;
				}
				hitSomething = true;
				lacerated    = true; // oh dear...
			}

			// If the velocities were not only partly applied due to
			// some wall/floor hit, all movement has been used up now.
			if (!(hitWall || hitFloor))
				isMoving = false;
		} // End of velocity check

		// === 4. If nothing is hit and if movement is left, check ===
		// ===    remaining movement and prepare for 1. or exit    ===
		// ===========================================================
		if ( !hitSomething && isMoving && (hitWall || hitFloor)
		  && ( (std::abs(xv) + std::abs(yv)) < 0.8) )
			// If the movement has slowed down too much, take it as a hit
			hitSomething = true;
		else if (!hitSomething && isMoving && (hitWall || hitFloor)
			  && ( (std::abs(xv_cur) + std::abs(yv_cur)) < 0.01) )
			// Just stop, wall bouncing/wrapping didn't leave enough rest
			isMoving = false;

		// Finally set x/y
		x = nextX;
		y = nextY;
	} // End of tracing the movement path
}

/* --- global function --- */
bool checkPixelsBetweenTwoPoints(double* startX,    double* startY,
                                 double  endX,      double  endY,
                                 double  can_delay, double* has_delayed)
{
	// return at once if there can't be any dirt in the box.
	if (!global.isDirtInBox(*startX, *startY, endX, endY)) {
		*startX = endX;
		*startY = endY;
		return false;
	}

	bool   result = false;
	double xDist  = endX - *startX;
	double yDist  = endY - *startY;
	double length = FABSDISTANCE2(xDist, yDist, 0, 0);

	// Shortcuts:
	bool   hasDelay = has_delayed && (can_delay > *has_delayed);
	bool   hasTop   = env.isBoxed;
	double left     = 1;
	double right    = env.screenWidth - 2;
	double top      = MENUHEIGHT + (hasTop ? 1 : 0);
	double bottom   = env.screenHeight - 2;


	// Drop out early if a neighbouring pixel is checked and it is a hit
	if (length < 2.) {
		if ( (endX > left)
		  && (endX < right)
		  && (endY > top)
		  && (endY < bottom) ) {

			*startX = endX;
			*startY = endY;

			if (PINK != getpixel(global.terrain, endX, endY) ) {
				result  = true;

				// For mind shot delays the distance is only added
				// to the travelled distance, but the result remains
				// false if the distance is not used up.
				if (hasDelay) {
					*has_delayed += length;
					if (can_delay > *has_delayed)
						result = false;
				}
			}

		} // End of having a valid position

		return result;
	} // End of early drop out

	// Otherwise the path must be checked
	double xInc  = xDist / length;
	double yInc  = yDist / length;
	double iDist = ABSDISTANCE2(0.0, 0.0, xInc, yInc); // [i]ncrease[Dist]ance

	// sanity check
	if (length > (env.screenWidth + env.screenHeight))
		length = env.screenWidth + env.screenHeight;

	// check all pixels along the line for land

	// As xInc/yInc are known now, left, right, top and bottom can
	// be corrected if the line would not leave the screen.
	left   = std::min(std::min(*startX,   left), *startX + (length * xInc));
	top    = std::min(std::min(*startY,    top), *startY + (length * yInc));
	right  = std::max(std::max(*startX,  right), *startX + (length * xInc));
	bottom = std::max(std::max(*startY, bottom), *startY + (length * yInc));

	// Note: Start with 1 and increase startX/Y first, as
	//       the starting pixel can be assumed to be clean.
	for (int32_t pos = 1; !result && (pos < length); ++pos) {
		*startX += xInc;
		*startY += yInc;

		if ( (*startX > left)
		  && (*startX < right)
		  && (*startY > top)
		  && (*startY < bottom) ) {

			if (PINK != getpixel (global.terrain, *startX, *startY) ) {
				result = true;
				// Note: startX/startY now point to the hit pixel

				// For mind shot delays we revert to false as long
				// as the allowed distance through dirt is used up.
				if (hasDelay) {
					*has_delayed += iDist;
					if (can_delay > *has_delayed)
						result = false;
				}

			}

		} // End of having a valid position
	} // End of walking positions

	// If nothing was hit, make sure startX/Y point to endX/Y
	if (!result) {
		*startX = endX;
		*startY = endY;
	}

	return result;
}

/** @brief Return the reaction velocity values of an object that hits dirt
  *
  * The function analyses the pixels around @a x and @a y to find
  * a plane the vector @a xv / @a yv has an angle to and returns
  * appropriate reaction velocity values in @a rxv and @a ryv.
**/
void getDirtBounceReact(int32_t x, int32_t y, double xv, double yv,
						double &rxv, double &ryv)
{
    int32_t from_x    = xv < 0. ? 1 : -1;
    double  vel       = FABSDISTANCE2(xv, yv, 0., 0.);

	// First find the heights around x/y:
    int32_t y_map[5];
    for (int i = 0 ; i < 5 ; ++i) {
		int32_t xpos = x + (i - 2);
		int32_t min_y = y - 2;
		int32_t max_y = y + 2;
		int32_t start_y = std::max(min_y, MENUHEIGHT);
		int32_t stop_y  = std::min(max_y + 1, env.screenHeight);
		y_map[i] = -1;

		if ( (xpos > 0) && (xpos < env.screenWidth)
		  && (min_y < env.screenHeight) && (max_y > MENUHEIGHT)
		  && ( (stop_y - start_y) > 0) ) {
			y_map[i] = 5;

			for (int32_t j = 4; (j >= 0) && (5 == y_map[i]); --j) {
				int32_t ypos = y + (j - 2);

				if ( (ypos <  start_y)
				  || (ypos >= stop_y)
				  || (PINK != getpixel(global.terrain, xpos, ypos)) )
					y_map[i] = j;
			}
		}
    } // End of generating height map.

	/* atan2(x, y) will be used which results in the following angles:
	 * Only right  1/ 0 :   90 (270) (The number in brackets is the normalized
	 * Only left  -1/ 0 : - 89 ( 91)  angle used in atanks)
	 * Only down   0/ 1 :    0 (180)
	 * Only up     0/-1 :  180 (360)
	 * Down right  1/ 1 :   45 (225)
	 * Up   right  1/-1 :  135 (315)
	 * Down left  -1/ 1 : - 44 (136)
	 * Up   left  -1/-1 : -134 ( 46)
	 *
	 * The plane is calculated using the same x direction if present.
	 * If the plane is a vertical wall, only the x-movement is reversed using
	 * a plane angle of 0.
	 *
	 * HA = Hit Angle, PA = Plane Angle, RA = Reaction Angle
	 *
	 * 1: HA = PA - Movement angle
	 * 2: RA = PA + (PA - HA)
	 * 3: Values >  180 gets 360 substracted
	 * 4: Value  < -180 gets 360 added
	 *
	 * Examples:
	 * 1: Shot from left to right and down (down right = 1/1 = 45°)
	 *    Plane is horizontal (only right = 1/0 = 90°)
	 *   HA = 90 - 45        =  45
	 *   RA = 90 + (90 - 45) = 135 (Up right)
	 * 2: Shot is the same, but the plane goes up right (1/-1 = 135°)
	 *   HA = 135 - 45         = 90
	 *   RA = 135 + (135 - 45) = 225 => 225 - 360 = -135 (Up left)
	 * 3: Shot from right to left and down (down left = -1/1 = -45)
	 *    Plane is horizontal (only left = -1/0 = -90°)
	 *   HA = -90 - -45         = -45
	 *   RA = -90 + (-90 - -45) = -135 (Up left)
	 * 4: Shot from the right bottom (up-left (-1/-1) = -135)
	 *    Plane is a vertical wall (only down = 0)
	 *   HA = 0 - -135 = 135 = RA
	 * 5: Shot from right to left down (-1/1) = -40
	 *    Plane up left (-1/-1) = -135
	 *   HA = -135 - -40 = -95
	 *   RA = -135 + (-135 - -95) = -175
	 */

	// Find a plane with angle using the height map
	int32_t MA = GET_ANGLE(xv, yv);
	int32_t PA = 0, HA = 0, RA = 0;

	// Look for the special case of a vertical wall first:
	if ( (y_map[2] < 2) && (y_map[2 + from_x] > 3) )
		if (MA)
			RA = 0 - MA;
		else
			// Just let it drip off
			RA = 5 * (y_map[3] ? 1 : -1);
	else {
		// Here a plane must be determined.
		double x1 = 2., y1 = y_map[2];
		double x2 = 2., y2 = y_map[2];
		double p1 = 1., p2 = 1.;

		// If y2 is 0, no further look is needed into movement direction
		if (y2 > 0.) {
			// Ah, it is.
			int32_t ly1 = y_map[2 + (-1 * from_x)];
			int32_t ly2 = y_map[2 + (-2 * from_x)];

			if (ly1 > -1) {
				x2 += 2. + (-1. * from_x);
				y2 += ly1;
				p2 += 1.;

				// Must ly2 be considered?
				if (ly2 > -1) {
					if ( ( (ly1 <= y_map[2]) && (ly2 <= ly1) )
					  || ( (ly1 >= y_map[2]) && (ly2 >= ly1) ) ) {
						x2 += 2. + (-2. * from_x);
						y2 += ly2;
						p2 += 1.;
					}
				} // End of having a non-wall ly2

				// Set final x2/y2:
				x2 /= p2;
				y2 /= p2;
			} // End of having a non-wall ly1
		} // end of having a non-vertical hit plane.

		// For the 'from'-direction the already set y2 can be used
		int32_t ly1 = y_map[2 +      from_x];
		int32_t ly2 = y_map[2 + (2 * from_x)];

		if ( (ly1 > -1)
		  && ( ( (ly1 >= y_map[2])
			  && (ly1 >= y2) )
			|| ( (ly1 <= y_map[2])
			  && (ly1 <= y2) ) ) ) {
			x1 += 2. + (1. * from_x);
			y1 += ly1;
			p1 += 1.;

			// Only continue if useful.
			if (ly2 > -1) {
				if ( ( (ly1 <= y_map[2]) && (ly2 <= ly1) )
				  || ( (ly1 >= y_map[2]) && (ly2 >= ly1) ) ) {
					x1 += 2. + (2. * from_x);
					y1 += ly2;
					p1 += 1.;
				}
			} // end of having a non-wall ly2

			// Set final x1/y1:
			x1 /= p1;
			y1 /= p1;
		} // End of having a non-wall ly1 without contra-slope.

		// Set resulting angles:
		PA = GET_ANGLE(x2 - x1, y2 - y1);
		HA = PA - MA;
		RA = PA + (PA - HA);

		// Secure against vertical drop traps:
		if (!MA && !PA && !HA)
			// RA is now 0 but must be 180
			RA = 180;

		if (RA >  180) RA -= 360;
		if (RA < -180) RA += 360;
	} // End of plane determination

	// The [R]eaction [A]angle now has to be translated into
	// atanks compatible velocity values:
	if (RA < 0) RA += 360; // atanks range

	rxv = env.slope[RA][0] * vel;
	ryv = env.slope[RA][1] * vel;
}
