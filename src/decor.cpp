#include "decor.h"
#include "sound.h"
#include "tank.h"

#include <cassert>

/// @brief Default constructor
DECOR::DECOR(double x_, double y_, double xv_, double yv_,
			int32_t maxRadius, int32_t type_, int32_t delay_) :
	PHYSICAL_OBJECT(false),
	curWind(global.wind),
	delay(delay_),
	maxGravAccel(-4. * env.gravity * env.FPS_mod),
	maxWind(env.windstrength),
	maxWindAccel(global.wind * env.FPS_mod),
	radius(maxRadius),
	type(type_)
{
	x = x_;
	y = y_;
	xv = xv_;
	yv = yv_;

	if (DECOR_DIRT == type) {
		// The core data is taken from the meteors.
		weapType = SML_METEOR
		         + (maxRadius / 2); // results in (0, 1, 1, 2, 2) for radius [1;5]
		mass = naturals[weapType - WEAPONS].mass;
		drag = naturals[weapType - WEAPONS].drag / 5.;

		// Special physics for dirt debris:
        physType = PT_DIRTBOUNCE;

        // Only keep dirt alive while it is really moving,
        // if it becomes too slow, only keep it for 2 seconds
        maxAge = 2 * env.frames_per_second;

        // The diameter is just used so it does not have to
        // be calculated each time updateDirt() is called.
        diameter = radius * 2;

        // Calculate how many pixels are needed per call to updateDirt()
        grabPerCall = ((diameter + 1) * (diameter + 1)) / (delay > 1 ? delay : 1);
	} else if (DECOR_SMOKE == type) {
		int32_t tempCol = 128 + (rand () % 64);

		if (maxRadius <= 3)
			radius = 3;
		else
			radius = 3 + (rand () % (maxRadius - 2));

		color  = makecol (tempCol, tempCol, tempCol);
		mass   = 1.0 + (static_cast<double>(rand () %  5) /  10.);
		drag   = 0.9 + (static_cast<double>(rand () % 90) / 100.);

		// maximum age depends on the maximum radius and the real radius,
		// plus 0 to 2 extra seconds.
		maxAge  = ( (maxRadius - (maxRadius - radius)) / 3) + (rand() % 3);
		maxAge *= env.frames_per_second;

		// Special physics for smoke, only for repulsion
		physType = PT_SMOKE;

		// Smoke does not need the dirt grabber
		ready = true;
	} else
		destroy = true;

	maxVel = env.maxVelocity * (1.20 + (mass / (.01 * MAX_POWER)));

	// Add to the chain:
	global.addObject(this);
}


/// @brief Constructor with bitmap
DECOR::DECOR(double x_, double y_, double xv_, double yv_,
			int32_t maxRadius, int32_t type_, int32_t delay_,
			sDebrisItem* deb_item, sDebrisItem* met_item) :
	DECOR(x_, y_, xv_, yv_, maxRadius, type_, delay_)
{
	// Everything done in delegated ctor, only img to set
	dirt   = deb_item;
	setBitmap(dirt ? dirt->bmp : nullptr);
	// Note: It is safe to distribute dirt->bmp, because bitmap normally holds
	// global graphics and must not be destroyed.
	meteor = met_item;

	if ( (nullptr == dirt) || !hasBitmap() )
		// Can't work without...
		destroy = true;
}


/// @brief default destructor
DECOR::~DECOR()
{
	if (DECOR_DIRT == type) {
		// Draw dirt on terrain and add land slide
		rotate_sprite (global.terrain, dirt->bmp, x - radius, y - radius, itofix (angle));
		global.addLandSlide(x - radius - 1, x + radius + 1, false);
	}

	if (dirt) {
		global.free_debris_item(dirt);
		dirt = nullptr;
	}

	if (meteor) {
		global.free_debris_item(meteor);
		meteor = nullptr;
	}

	// Update the last drawing area
	int32_t calcRadius = radius;

	if (DECOR_DIRT == type)
			++calcRadius;
	else if (DECOR_SMOKE == type)
		// The older, the larger...
		calcRadius = static_cast<int32_t>(radius * (4.0 * age / maxAge));

	setUpdateArea ( x - calcRadius - 1, y - calcRadius - 1,
					(calcRadius * 2) + 2, (calcRadius * 2) + 2);
	requireUpdate ();
	this->update();

	// Take out of the chain:
	global.removeObject(this);
}


/// @brief let smoke drift and disperse with the wind
void DECOR::applyPhysics ()
{
	if (destroy)
		return;

	if (delay > 0) {
		--delay;
		return;
	}

	// for detecting bounces
	double old_yv = yv;

	if (DECOR_DIRT == type) {

		// Check whether movement ended
		double movement = FABSDISTANCE2(xv, yv, 0, 0);
		bool   on_floor = isOnFloor(); // Needed again below.

		if ( on_floor
		  && ( (hitSomething && (movement < 0.8))
		    || (movement < 0.2) ) )  {

			// It ended!

			// fix y:
			int32_t dirt_bottom = y + dirt->bmp->h;
			if ( ( (y - radius) > MENUHEIGHT)
			  && ( dirt_bottom < env.screenHeight)
			  && (PINK != getpixel(global.terrain, x, dirt_bottom)) )
				--y;

			xv = 0.;
			yv = 0.;
			destroy = true;
			requireUpdate();

		} else {
			hitSomething = false; // Enable checking.

			// Now apply physics
			repulseDecor();
			PHYSICAL_OBJECT::applyPhysics();

			// Be sure x/y values are sane (Can drift into walls
			// on rare wind conditions.)
			if (x < 2) x = 2;
			if (x > (env.screenWidth  - 2)) x = env.screenWidth  - 2;
			if (y > (env.screenHeight - 2)) y = env.screenHeight - 2;

			// Maybe play a sound on bounce
			if ( !global.skippingComputerPlay
			  && (old_yv > .5)
			  && (yv < -0.1) )
				play_natural_sound(DIRT_FRAGMENT, x, radius * 32,
						1200 - (radius * 50));
		}

		// raise age if movement is below 0.5
		if ( ( on_floor || (FABSDISTANCE2(xv, yv, 0, 0) < .5) )
		  && (++age > maxAge) )
			destroy = true;

	} else if (DECOR_SMOKE == type) {
		// Apply wind first
		int32_t ageMod  = ROUND(std::abs(curWind / (maxWind / 2.0))) + 1;

		/* This produces: (with max wind = 8)
		 * wind = 0 : round(0 / (8 / 2)) + 1 = round(0 / 4) + 1 = 0 + 1 = 1 <-- normal aging
		 * wind = 1 : round(1 / (8 / 2)) + 1 = round(1 / 4) + 1 = 0 + 1 = 1 <-- normal aging
		 * wind = 4 : round(4 / (8 / 2)) + 1 = round(4 / 4) + 1 = 1 + 1 = 2 <-- raised aging
		 * wind = 6 : round(6 / (8 / 2)) + 1 = round(6 / 4) + 1 = 2 + 1 = 3 <-- fast aging
		 * wind = 8 : round(8 / (8 / 2)) + 1 = round(8 / 4) + 1 = 2 + 1 = 3 <-- fast aging
		*/
		age += ageMod;

		// Set further values
		// Try to reach half distance to the maximum values per second
		double xaccel = ((xv + maxWindAccel) / 2)
		              / static_cast<double>(env.frames_per_second);
		double yaccel = ((yv + maxGravAccel) / 2)
		              / static_cast<double>(env.frames_per_second / 10.);

		// Apply current acceleration
		xv += xaccel;
		yv += yaccel;

		// Add repulsion:
		repulseDecor();

		// Be sure that neither xv outruns wind nor yv is
		// higher than reverse gravity
		if (std::abs(xv) > std::abs(curWind))
			xv = curWind;
		if (yv < maxGravAccel)
			yv = maxGravAccel;

		// Don't push through the floor
		if ( (y + yv) >= env.screenHeight) {
			yv *= -0.5;
			xv *=  0.95;
		}

		// The faster the smoke is blown by the wind, the less it rises:
		if ( (yv < -1.) && (std::abs(xv) > 1.) )
			yv = (yv + (yv / std::abs(xv))) / 2.;
		// and if the smoke is going down, halve yv
		else if (yv > 0.)
			yv /= 2.;

		// Now the velocity can be applied.
		x += xv;
		y += yv;

		// Destroy the smoke if it goes off-screen or is diffused
		int32_t calcRadius = static_cast<int32_t>(radius * (4.0 * age / maxAge));

		if ( (x <  (1 - calcRadius))
		  || (x >= (env.screenWidth + calcRadius))
		  || (y <  (MENUHEIGHT - calcRadius))
		  || (age > maxAge) )
			destroy = true;
	}
}


/// @brief draw decor according to current settings and type.
void DECOR::draw()
{
	if (!ready && !destroy) {
		updateDirt();
		if (ready) {
			// finished! See whether there are enough pixels
			if (gotPixels <= radius)
				// nope.
				destroy = true;
		}
	}

	if (destroy || (delay > 0))
		return;

	int32_t calcRadius = radius;

	if (DECOR_DIRT == type) {
		// Rotate according to xv and yv
		angle += yv + ((SIGNd(xv) * 5.) - xv);

		// Be sure the angle is in order:
		if (angle <   0) angle += 360;
		if (angle > 360) angle -= 360;

		// And draw it:
		if (y > MENUHEIGHT) {
			VIRTUAL_OBJECT::draw();
			++calcRadius;
		}
	} else if (DECOR_SMOKE == type) {
		// The older, the larger...
		calcRadius = static_cast<int32_t>(radius * (4.0 * age / maxAge));

		drawing_mode (DRAW_MODE_TRANS, NULL, 0, 0);
		set_trans_blender (0, 0, 0, 255 - (255 * age / maxAge));
		circlefill (global.canvas, x, y, calcRadius, color);
	}

	drawing_mode (global.current_drawing_mode, NULL, 0, 0);

	setUpdateArea ( x - calcRadius - 1, y - calcRadius - 1,
					(calcRadius * 2) + 2, (calcRadius * 2) + 2);
	requireUpdate ();
}


/// In case of too much decor for the machine, allow forced ageing
void DECOR::force_aging(int32_t frames)
{
	age += frames;
	if (age > maxAge)
		destroy = true;
}


/// return true if a dirt debris item "lies" on the floor, or is squeezed in a
/// dirt slide.
bool DECOR::isOnFloor()
{
	int32_t scr_r  = env.screenWidth - 2;  // shortcut;
	int32_t scr_b  = env.screenHeight - 2; // ditto;

	// If the debris is above the screen or directly on the floor,
	// return at once:
	if (y <= MENUHEIGHT)
		return false;
	if (y >= (scr_b - radius))
		return true;

	// Use safe values:
	int32_t round_x = ROUND(x);
	int32_t round_y = ROUND(y);

	// sanitize x value:
	if      (round_x < 1)     round_x = 1;
	else if (round_x > scr_r) round_x = scr_r;

	// rounded boundaries, clipped to the screen:
	int32_t left    = std::max(1,          round_x - radius);
	int32_t top     = std::max(MENUHEIGHT, round_y - radius);
	int32_t right   = std::min(scr_r,      round_x + radius);
	int32_t bottom  = std::min(scr_b,      round_y + radius);

	// Go from left to right and check whether the surface is above the bottom.
	int32_t surf_hits = 0;
	bool    on_floor  = false;
	for (int32_t i = left; !on_floor && (i <= right); ++i) {
		if (global.surface[i].load(ATOMIC_READ) <= bottom) {
			// Actually this could mean that the debris is within
			// a hole in a mountain that hasn't been slid down, yet.
			bool in_dirt = false;

			for (int32_t j = bottom; !in_dirt && (j >= top) ; --j) {
				if (PINK != getpixel(global.terrain, i, j))
					in_dirt = true;
			}

			if (in_dirt && (++surf_hits >= radius) )
				on_floor = true;
		}
	}

	return on_floor;
}


/// DIRT and Smoke (somewhat) can be repulsed, too
void DECOR::repulseDecor()
{
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
}


/// Small scale dirt grabber
void DECOR::updateDirt()
{
	int32_t togo    = grabPerCall + 1;
	double  deb_rad = static_cast<double>(radius);

	while (togo) {
		double deb_dist = FABSDISTANCE2(static_cast<double>(grab_x),
										static_cast<double>(grab_y),
										deb_rad,
										deb_rad);
		if (deb_dist <= deb_rad) {
			int32_t tcol = getpixel(dirt->bmp, grab_x, grab_y);

			// If this is a meteor and the terrain had no pixel
			// there, take one out of the rock instead.
			if ( (PINK == tcol) && meteor)
				tcol = getpixel(meteor->bmp, grab_x, grab_y);

			// If this is valid, scorch the colour and put it back.
			if (PINK != tcol) {
				double deb_mod = deb_dist / deb_rad;
				int32_t new_r = getr(tcol) / (1.25 + deb_mod);
				int32_t new_g = getg(tcol) / (1.66 + deb_mod);
				int32_t new_b = getb(tcol) / (1.33 + deb_mod);
				putpixel(dirt->bmp, grab_x, grab_y, makecol(new_r, new_g, new_b));
				++gotPixels;
			}
		} // End of position in range

		else
			// If the position is not in range, erase the surplus pixel
			putpixel(dirt->bmp, grab_x, grab_y, PINK);

		// This point is done
		--togo;

		// Advance coordinates
		if (++grab_x > diameter) {
			grab_x = 0;
			if (++grab_y > diameter) {
				// end of work
				togo  = 0;
				ready = true;
				if (meteor) {
					global.free_debris_item(meteor);
					meteor = nullptr;
				}
			}
		}
	} // End of having pixels to grab
}
