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

#include <time.h>
#include <cassert>
#include "player.h"
#include "globaldata.h"
#include "files.h"
#include "tank.h"
#include "sound.h"
#include "debris_pool.h"


GLOBALDATA::GLOBALDATA()
{
	// memset initialization, because Visual C++ 2013 can't do lists, yet.
	memset(order,       0, sizeof(TANK*)   * MAXPLAYERS);
	memset(tank_status, 0, sizeof(char)    * 128);
	memset(heads,       0, sizeof(vobj_t*) * CLASS_COUNT);
	memset(tails,       0, sizeof(vobj_t*) * CLASS_COUNT);
}


GLOBALDATA::~GLOBALDATA()
{
	this->destroy();
}

/// @brief goes through the columns from @a left to @a right and sets slide type according to @a do_lock
void GLOBALDATA::addLandSlide(int32_t left, int32_t right, bool do_lock)
{
	// Opt out soon if no landslide is to be done
	if ( (SLIDE_NONE      == env.landSlideType)
	  || (SLIDE_TANK_ONLY == env.landSlideType) )
		return;

	int32_t minX = std::min(left, right);
	int32_t maxX = std::max(left, right);

	if (minX < 1)
		minX = 1;
	if (minX > (env.screenWidth - 1) )
		minX =  env.screenWidth - 1;
	if (maxX < 1)
		maxX = 1;
	if (maxX > (env.screenWidth - 1) )
		maxX =  env.screenWidth - 1;

	if (do_lock)
		memset(&done[minX], 3, sizeof(char) * (maxX - minX + 1) );
	else
		memset(&done[minX], 2, sizeof(char) * (maxX - minX + 1) );
}


void GLOBALDATA::addObject (vobj_t *object)
{
	if (nullptr == object)
		return;

	eClasses class_ = object->getClass();

	objLocks[class_].lock();

	/// --- case 1: first of its kind ---
	if (nullptr == tails[class_]) {
		heads[class_] = object;
		tails[class_] = object;
	}

	/// --- case 2: normal addition ---
	else {
		tails[class_]->next = object;
		object->prev = tails[class_];
		tails[class_] = object;
	}

	objLocks[class_].unlock();
}


// Combine both make_update and make_bgupdate with safety checks for
// the dimensions. This reduces code duplication.
void GLOBALDATA::addUpdate(int32_t x, int32_t y, int32_t w, int32_t h,
                           BOX* target, int32_t &target_count)
{
	assert (target && "ERROR: addUpdate called with nullptr target!");

	bool combined = false;

	assert ( (w > 0) && (h > 0) ); // No zero/negative updates, please!

	int32_t left   = std::max(x - 1, 0);
	int32_t top    = std::max(y - 1, 0);
	int32_t right  = std::min(x + w + 1, env.screenWidth);
	int32_t bottom = std::min(y + h + 1, env.screenHeight);

	// If the update is outside the screen, it is not needed:
	if ( (bottom <= 0) /* most common case */
	  || (left   >= env.screenWidth)
	  || (right  <= 0)
	  || (top    >= env.screenHeight) )
		return;

	assert( (left < right ) );
	assert( (top  < bottom) );

	if ( combineUpdates && target_count
	  && (target_count < env.max_screen_updates)) {
		// Re-purpose BOX::w as x2 and BOX::h as y2:
		BOX prev(target[target_count - 1].x,
		         target[target_count - 1].y,
		         target[target_count - 1].x + target[target_count - 1].w,
		         target[target_count - 1].y + target[target_count - 1].h);
		BOX next(left, top, right, bottom);

		if ( (next.w > (prev.x - 3))
		  && (prev.w > (next.x - 3))
		  && (next.h > (prev.y - 3))
		  && (prev.h > (next.y - 3)) ) {
			next.set(next.x < prev.x ? next.x : prev.x,
			         next.y < prev.y ? next.y : prev.y,
			         next.w > prev.w ? next.w : prev.w,
			         next.h > prev.h ? next.h : prev.h);
			// recalculate x2/y2 back into w/h
			target[target_count - 1].set(next.x, next.y,
			                             next.w - next.x,
			                             next.h - next.y);

			// Make sure the target update is sane:
			assert( (target[target_count - 1].w > 0)
			     && (target[target_count - 1].h > 0) );

			combined = true;
		}
	}

	if (!combined)
		target[target_count++].set(left, top, right - left, bottom - top);

	if (!stopwindow && (target_count <= env.max_screen_updates))
		env.window_update(left, top, right - left, bottom - top);
}


// return true if any living tank is in the given box.
// left/right and top/bottom are determined automatically.
bool GLOBALDATA::areTanksInBox(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
	TANK* lt = static_cast<TANK*>(heads[CLASS_TANK]);

	while (lt) {
		// Tank found, is it in the box?
		if ( (!lt->destroy) && lt->isInBox(x1, y1, x2, y2))
			return true;
		lt->getNext(&lt);
	}

	return false;
}



// This function checks to see if one full second has passed since the
// last time the function was called.
// The function returns true if time has passed. The function
// returns false if time hasn't passed or it was unable to tell
// how much time has passed.
bool GLOBALDATA::check_time_changed()
{
	volatile
	static time_t last_second    = 0;
	static time_t current_second = 0;

	time(&current_second);

	if ( current_second == last_second )
		return false;

	// time has changed
	last_second = current_second;

	return true;
}


/// @brief remove and delete *all* objects stored.
void GLOBALDATA::clear_objects()
{
	int32_t class_ = 0;

	while (class_ < CLASS_COUNT) {
		while (tails[class_])
			delete tails[class_];
		++class_;
	}
}


// Call before calling allegro_exit()!
void GLOBALDATA::destroy()
{
	clear_objects();

	if (debris_pool) {
		delete debris_pool;
		debris_pool = nullptr;
	}

	if (canvas)  destroy_bitmap(canvas);     canvas       = nullptr;
	if (terrain) destroy_bitmap(terrain);    terrain      = nullptr;
	if (done)         delete [] done;        done         = nullptr;
	if (fp)           delete [] fp;          fp           = nullptr;
	if (surface)      delete [] surface;     surface      = nullptr;
	if (dropTo)       delete [] dropTo;      dropTo       = nullptr;
	if (velocity)     delete [] velocity;    velocity     = nullptr;
	if (dropIncr)     delete [] dropIncr;    dropIncr     = nullptr;
	if (updates)      delete [] updates;     updates      = nullptr;
	if (lastUpdates)  delete [] lastUpdates; lastUpdates  = nullptr;
}


void GLOBALDATA::do_updates ()
{
	bool isBgUpdNeeded = lastUpdatesCount > 0;

	acquire_bitmap(screen);
	for (int32_t i = 0; i < updateCount; ++i) {
		blit( canvas, screen,
				updates[i].x, updates[i].y, updates[i].x, updates[i].y,
				updates[i].w, updates[i].h);

		if (isBgUpdNeeded)
			make_bgupdate( updates[i].x, updates[i].y,
							updates[i].w, updates[i].h);
	}
	release_bitmap(screen);
	if (!isBgUpdNeeded) {
		lastUpdatesCount = updateCount;
		memcpy (lastUpdates, updates, sizeof (BOX) * updateCount);
	}
	updateCount = 0;
}


// Do what has to be done after the game starts
void GLOBALDATA::first_init()
{
	// get memory for updates
	try {
		updates = new BOX[env.max_screen_updates];
	} catch (std::bad_alloc &e) {
		cerr << "globaldata.cpp:" << __LINE__ << ":first_init() : "
		     << "Failed to allocate memory for updates ["
		     << e.what() << "]" << endl;
		exit(1);
	}

	// get memory for lastUpdates
	try {
		lastUpdates = new BOX[env.max_screen_updates];
	} catch (std::bad_alloc &e) {
		cerr << "globaldata.cpp:" << __LINE__ << ":first_init() : "
		     << "Failed to allocate memory for lastUpdates ["
		     << e.what() << "]" << endl;
		exit(1);
	}

	canvas = create_bitmap (env.screenWidth, env.screenHeight);
	if (!canvas) {
		cout << "Failed to create canvas bitmap: " << allegro_error << endl;
		exit(1);
	}

	terrain = create_bitmap (env.screenWidth, env.screenHeight);
	if (!terrain) {
		cout << "Failed to create terrain bitmap: " << allegro_error << endl;
		exit(1);
	}


	// get memory for the debris pool
	try {
		debris_pool = new sDebrisPool(env.max_screen_updates);
	} catch (std::bad_alloc &e) {
		cerr << "globaldata.cpp:" << __LINE__ << ":first_init() : "
		     << "Failed to allocate memory for debris_pool ["
		     << e.what() << "]" << endl;
		exit(1);
	}


	try {
		done     = new int8_t[env.screenWidth]{0};
		fp       = new int32_t[env.screenWidth]{0};
		surface  = new ai32_t[env.screenWidth]{ { 0 } };
		dropTo   = new int32_t[env.screenWidth]{0};
		velocity = new double[env.screenWidth]{0};
		dropIncr = new double[env.screenWidth]{0};
	} catch (std::bad_alloc &e) {
		cerr << "globaldata.cpp:" << __LINE__ << ":first_init() : "
		     << "Failed to allocate memory for base data arrays ["
		     << e.what() << "]" << endl;
		exit(1);
	}

	initialise ();
}


/** @brief delegate freeing of a debris item to the debris pool.
  *
  * This delegating function, instead of making the debris pool public,
  * exists as a point where locking, if it becomes necessary, can be
  * added without having to rewrite a lot of code.
**/
void GLOBALDATA::free_debris_item(item_t* item)
{
	debris_pool->free_item(item);
}



int32_t GLOBALDATA::get_avg_bgcolor(int32_t x1, int32_t y1,
                                    int32_t x2, int32_t y2,
                                    double xv, double yv)
{
	// Movement
	int32_t mvx      = ROUND(10. * xv); // eliminate slow movement
	int32_t mvy      = ROUND(10. * yv); // eliminate slow movement
	bool    mv_left  = mvx < 0;
	bool    mv_right = mvx > 0;
	bool    mv_up    = mvy < 0;
	bool    mv_down  = mvy > 0;

	// Boundaries
	int32_t min_x = 1;
	int32_t max_x = env.screenWidth - 2;
	int32_t min_y = env.isBoxed ? MENUHEIGHT + 1 : MENUHEIGHT;
	int32_t max_y = env.screenHeight - 2;

	// Coordinates
	int32_t left   = std::max(std::min(x1, x2), min_x);
	int32_t right  = std::min(std::max(x1, x2), max_x);
	int32_t centre = (x1 + x2) / 2;
	int32_t top    = std::max(std::min(y1, y2), min_y);
	int32_t bottom = std::min(std::max(y1, y2), max_y);
	int32_t middle = (y1 + y2) / 2;


	// Colors:
	int32_t col_tl, col_tc, col_tr; // top row
	int32_t col_ml, col_mc, col_mr; // middle row
	int32_t col_bl, col_bc, col_br; // bottom row
	int32_t r = 0, g = 0, b = 0;


	// Get Sky or Terrain colour, whatever fits:
	/*---------------------
	  --- Left side ---
	  ---------------------*/
	if ( PINK == (col_tl = getpixel(terrain, left, top)) )
		col_tl = getpixel(env.sky, left, top);
	if ( PINK == (col_ml = getpixel(terrain, left, middle)) )
		col_ml = getpixel(env.sky, left, middle);
	if ( PINK == (col_bl = getpixel(terrain, left, bottom)) )
		col_bl = getpixel(env.sky, left, bottom);

	/*---------------------
	  --- The Center ---
	---------------------*/
	if ( PINK == (col_tc = getpixel(terrain, centre, top)) )
		col_tc = getpixel(env.sky, centre, top);
	if ( PINK == (col_mc = getpixel(terrain, centre, middle)) )
		col_mc = getpixel(env.sky, centre, middle);
	if ( PINK == (col_bc = getpixel(terrain, centre, bottom)) )
		col_bc = getpixel(env.sky, centre, bottom);

	/*----------------------
	  --- Right side ---
	----------------------*/
	if ( PINK == (col_tr = getpixel(terrain, right, top)) )
		col_tr = getpixel(env.sky, right, top);
	if ( PINK == (col_mr = getpixel(terrain, right, middle)) )
		col_mr = getpixel(env.sky, right, middle);
	if ( PINK == (col_br = getpixel(terrain, right, bottom)) )
		col_br = getpixel(env.sky, right, bottom);


	// Fetch the rgb parts, according to movement:

	/* --- X-Movement --- */
	if (mv_left) {
		// Movement to the left, weight left side colour twice
		r += (GET_R(col_tl) + GET_R(col_ml) + GET_R(col_bl)) * 2;
		g += (GET_G(col_tl) + GET_G(col_ml) + GET_G(col_bl)) * 2;
		b += (GET_B(col_tl) + GET_B(col_ml) + GET_B(col_bl)) * 2;
		// The others are counted once
		r += GET_R(col_tc) + GET_R(col_mc) + GET_R(col_bc)
		   + GET_R(col_tr) + GET_R(col_mr) + GET_R(col_br);
		g += GET_G(col_tc) + GET_G(col_mc) + GET_G(col_bc)
		   + GET_G(col_tr) + GET_G(col_mr) + GET_G(col_br);
		b += GET_B(col_tc) + GET_B(col_mc) + GET_B(col_bc)
		   + GET_B(col_tr) + GET_B(col_mr) + GET_B(col_br);
	} else if (mv_right) {
		// Movement to the right, weight right side colour twice
		r += (GET_R(col_tr) + GET_R(col_mr) + GET_R(col_br)) * 2;
		g += (GET_G(col_tr) + GET_G(col_mr) + GET_G(col_br)) * 2;
		b += (GET_B(col_tr) + GET_B(col_mr) + GET_B(col_br)) * 2;
		// The others are counted once
		r += GET_R(col_tc) + GET_R(col_mc) + GET_R(col_bc)
		   + GET_R(col_tl) + GET_R(col_ml) + GET_R(col_bl);
		g += GET_G(col_tc) + GET_G(col_mc) + GET_G(col_bc)
		   + GET_G(col_tl) + GET_G(col_ml) + GET_G(col_bl);
		b += GET_B(col_tc) + GET_B(col_mc) + GET_B(col_bc)
		   + GET_B(col_tl) + GET_B(col_ml) + GET_B(col_bl);
	} else {
		// No x-movement, weight centre colour twice
		r += (GET_R(col_tc) + GET_R(col_mc) + GET_R(col_bc)) * 2;
		g += (GET_G(col_tc) + GET_G(col_mc) + GET_G(col_bc)) * 2;
		b += (GET_B(col_tc) + GET_B(col_mc) + GET_B(col_bc)) * 2;
		// The others are counted once
		r += GET_R(col_tl) + GET_R(col_ml) + GET_R(col_bl)
		   + GET_R(col_tr) + GET_R(col_mr) + GET_R(col_br);
		g += GET_G(col_tl) + GET_G(col_ml) + GET_G(col_bl)
		   + GET_G(col_tr) + GET_G(col_mr) + GET_G(col_br);
		b += GET_B(col_tl) + GET_B(col_ml) + GET_B(col_bl)
		   + GET_B(col_tr) + GET_B(col_mr) + GET_B(col_br);
	}

	/* --- Y-Movement --- */
	if (mv_up) {
		// Movement upwards, weight top side colour twice
		r += (GET_R(col_tl) + GET_R(col_tc) + GET_R(col_tr)) * 2;
		g += (GET_G(col_tl) + GET_G(col_tc) + GET_G(col_tr)) * 2;
		b += (GET_B(col_tl) + GET_B(col_tc) + GET_B(col_tr)) * 2;
		// The others are counted once
		r += GET_R(col_ml) + GET_R(col_mc) + GET_R(col_mr)
		   + GET_R(col_bl) + GET_R(col_bc) + GET_R(col_br);
		g += GET_G(col_ml) + GET_G(col_mc) + GET_G(col_mr)
		   + GET_G(col_bl) + GET_G(col_bc) + GET_G(col_br);
		b += GET_B(col_ml) + GET_B(col_mc) + GET_B(col_mr)
		   + GET_B(col_bl) + GET_B(col_bc) + GET_B(col_br);
	} else if (mv_down) {
		// Movement downwards, weight bottom side colour twice
		r += (GET_R(col_bl) + GET_R(col_bc) + GET_R(col_br)) * 2;
		g += (GET_G(col_bl) + GET_G(col_bc) + GET_G(col_br)) * 2;
		b += (GET_B(col_bl) + GET_B(col_bc) + GET_B(col_br)) * 2;
		// The others are counted once
		r += GET_R(col_ml) + GET_R(col_mc) + GET_R(col_mr)
		   + GET_R(col_tl) + GET_R(col_tc) + GET_R(col_tr);
		g += GET_G(col_ml) + GET_G(col_mc) + GET_G(col_mr)
		   + GET_G(col_tl) + GET_G(col_tc) + GET_G(col_tr);
		b += GET_B(col_ml) + GET_B(col_mc) + GET_B(col_mr)
		   + GET_B(col_tl) + GET_B(col_tc) + GET_B(col_tr);
	} else {
		// No y-movement, weight middle colour twice
		r += (GET_R(col_ml) + GET_R(col_mc) + GET_R(col_mr)) * 2;
		g += (GET_G(col_ml) + GET_G(col_mc) + GET_G(col_mr)) * 2;
		b += (GET_B(col_ml) + GET_B(col_mc) + GET_B(col_mr)) * 2;
		// The others are counted once
		r += GET_R(col_tl) + GET_R(col_tc) + GET_R(col_tr)
		   + GET_R(col_bl) + GET_R(col_bc) + GET_R(col_br);
		g += GET_G(col_tl) + GET_G(col_tc) + GET_G(col_tr)
		   + GET_G(col_bl) + GET_G(col_bc) + GET_G(col_br);
		b += GET_B(col_tl) + GET_B(col_tc) + GET_B(col_tr)
		   + GET_B(col_bl) + GET_B(col_bc) + GET_B(col_br);
	}


	/* I know this looks weird, but what we now have is some kind of summed
	 * matrix, which is always the same:
	 * Let's assume that xv and yv are both 0.0, so no movement is happening.
	 * The result is: (In counted times)
	 * 2|3|2  ( =  7)
	 * -+-+-
	 * 3|4|3  ( = 10)
	 * -+-+-
	 * 2|3|2  ( =  7)
	 *          = 24
	 * And it is always 24, no matter which movement combination you try
	 */

	r /= 24;
	g /= 24;
	b /= 24;

	return makecol(r > 0xff ? 0xff : r,
	               g > 0xff ? 0xff : g,
	               b > 0xff ? 0xff : b);
}


// Locks global->command for reading, reads value, then unlocks the variable
// and returns the value.
int32_t GLOBALDATA::get_command()
{
	cmdLock.lock();
	int32_t c = command;
 	cmdLock.unlock();
	return c;
}


TANK* GLOBALDATA::get_curr_tank()
{
	return currTank;
}


/** @brief delegate getting a debris item to the debris pool.
  *
  * This delegating function, instead of making the debris pool public,
  * exists as a point where locking, if it becomes necessary, can be
  * added without having to rewrite a lot of code.
**/
sDebrisItem* GLOBALDATA::get_debris_item(int32_t radius)
{
	return debris_pool->get_item(radius);
}


TANK* GLOBALDATA::get_next_tank(bool *wrapped_around)
{
	bool    found    = false;
	int32_t index    = tankindex + 1;
	int32_t oldindex = tankindex;
	int32_t wrapped  = 0;

	while (!found && (wrapped < 2)) {
		if (index >= MAXPLAYERS) {
			index = 0;
			if (wrapped_around)
				*wrapped_around = true;
			wrapped++;
		}

		if ( order[index]
		  && (index != oldindex)
		  && !order[index]->destroy)
			found = true;
		else
			++index;
	}

	tankindex = index;

	// If this tank is valid, the currently selected weapon must be checked
	// first and changed if depleted
	TANK* next_tank = order[index];
	if (next_tank && next_tank->player)
		next_tank->check_weapon();

	// Whatever happened, the status bar needs an update:
	if (oldindex != index)
		updateMenu = true;

	return next_tank;
}


/// @brief randomly return one active tank
TANK* GLOBALDATA::get_random_tank()
{
	int32_t idx      = rand() % MAXPLAYERS;
	int32_t attempts = 2;
	while ( (!order[idx] || order[idx]->destroy)
	     && (idx < MAXPLAYERS) && attempts ) {
		if (++idx >= MAXPLAYERS) {
			idx = 0;
			--attempts;
		}
	}

	return order[idx];
}


void GLOBALDATA::initialise ()
{
	clear_objects();
	numTanks = 0;
	clear_to_color (canvas, WHITE);
	clear_to_color (terrain, PINK);

	for (int32_t i = 0; i < env.screenWidth; ++i) {
		done[i]    = 0;
		dropTo[i]  = env.screenHeight - 1;
		fp[i]      = 0;
	}
}


// return true if the dirt reaches into the given box.
// left/right and top/bottom are determined automatically.
bool GLOBALDATA::isDirtInBox(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
	int32_t top = std::max(std::min(y1, y2),
	                       env.isBoxed ? MENUHEIGHT + 1 : MENUHEIGHT);
	// Exit early if the box is below the playing area
	if (top >= env.screenHeight)
		return false;

	int32_t bottom = std::min(std::max(y1, y2), env.screenHeight - 2);
	// Exit early if the box is over the playing area
	if (bottom <= MENUHEIGHT)
		return false;

	int32_t left   = std::max(std::min(x1, x2), 1);
	int32_t right  = std::min(std::max(x1, x2), env.screenWidth - 2);

	// If the box is outside the playing area, this loop won't do anything
	for (int32_t x = left; x <= right; ++x) {
		if (surface[x].load(ATOMIC_READ) <= bottom)
			return true;
	}

	return false;
}


/// @return true if the close button was pressed
bool GLOBALDATA::isCloseBtnPressed()
{
	cbpLock.lock();
	bool result = close_button_pressed;
	cbpLock.unlock();

	return result;
}


/** @brief load global data from a file
  * This method is still present to provide backwards
  * compatibility with configurations that were saved
  * before the values were moved to ENVIRONMENT
**/
void GLOBALDATA::load_from_file (FILE* file)
{
	char  line[ MAX_CONFIG_LINE + 1] = { 0 };
	char  field[MAX_CONFIG_LINE + 1] = { 0 };
	char  value[MAX_CONFIG_LINE + 1] = { 0 };
	char* result                     = nullptr;

	setlocale(LC_NUMERIC, "C");

	// read until we hit line "*GLOBAL*" or "***" or EOF
	do {
		result = fgets(line, MAX_CONFIG_LINE, file);
		if ( !result
		  || !strncmp(line, "***", 3) )
			// eof OR end of record
			return;
	} while ( strncmp(line, "*GLOBAL*", 8) );

	bool  done = false;

	while (result && !done) {
		// read a line
		memset(line, '\0', MAX_CONFIG_LINE);
		if ( ( result = fgets(line, MAX_CONFIG_LINE, file) ) ) {

			// if we hit end of the record, stop
			if (! strncmp(line, "***", 3) )
				return;

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

			// seperate field from value
			memset(field, '\0', MAX_CONFIG_LINE);
			memset(value, '\0', MAX_CONFIG_LINE);
			strncpy(field, line, equal_position);
			strncpy(value, &( line[equal_position + 1] ), MAX_CONFIG_LINE);


			// Values that were moved to ENVIRONMENT:
			// They are loaded, for compatibility, but the next
			// save will put them into the correct section anyway.
			// So these can eventually be removed.
			if (!strcasecmp(field, "acceleratedai")) {
				sscanf(value, "%d", &env.skipComputerPlay);
				if (env.skipComputerPlay > SKIP_HUMANS_DEAD)
					env.skipComputerPlay = SKIP_HUMANS_DEAD;
			} else if (!strcasecmp(field, "checkupdates")) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				env.check_for_updates = val > 0 ? true : false;
			} else if (!strcasecmp(field, "colourtheme") ) {
				sscanf(value, "%d", &env.colourTheme);
				if (env.colourTheme < CT_REGULAR) env.colourTheme = CT_REGULAR;
				if (env.colourTheme > CT_CRISPY)  env.colourTheme = CT_CRISPY;
			} else if (!strcasecmp(field, "debrislevel") )
				sscanf(value, "%d", &env.debris_level);
			else if (!strcasecmp(field, "detailedland")) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				env.detailedLandscape = val > 0 ? true : false;
			} else if (!strcasecmp(field, "detailedsky")) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				env.detailedSky = val > 0 ? true : false;
			} else if (!strcasecmp(field, "dither")) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				env.ditherGradients = val > 0 ? true : false;
			} else if (!strcasecmp(field, "dividemoney") ) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				env.divide_money = val > 0 ? true : false;
			} else if (!strcasecmp(field, "enablesound")) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				env.sound_enabled = val > 0 ? true : false;
			} else if (!strcasecmp(field, "frames") ) {
				int32_t new_fps = 0;
				sscanf(value, "%d", &new_fps);
				env.set_fps(new_fps);
			} else if (!strcasecmp(field, "fullscreen"))
				sscanf(value, "%d", &env.full_screen);
			else if (!strcasecmp(field, "interest"))
				sscanf(value, "%lf", &env.interest);
			else if (!strcasecmp(field, "language") ) {
				uint32_t stored_lang = 0;
				sscanf(value, "%u", &stored_lang);
				env.language = static_cast<eLanguages>(stored_lang);
			} else if (!strcasecmp(field, "listenport"))
				sscanf(value, "%d", &env.network_port);
			else if (!strcasecmp(field, "maxfiretime") )
				sscanf(value, "%d", &env.maxFireTime);
			else if (!strcasecmp(field, "networking")) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				env.network_enabled = val > 0 ? true : false;
			} else if (!strcasecmp(field, "numpermanentplayers"))
				sscanf(value, "%d", &env.numPermanentPlayers);
			else if (!strcasecmp(field, "OSMOUSE")) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				env.osMouse = val > 0 ? true : false;
			} else if (!strcasecmp(field, "playmusic")) {
				int32_t val = 0;
				sscanf(value, "%d", &val);
				env.play_music = val > 0 ? true : false;
			} else if (!strcasecmp(field, "rounds") )
				sscanf(value, "%u", &env.rounds);
			else if (!strcasecmp(field, "screenwidth")
				  && !env.temp_screenWidth) {
				sscanf(value, "%d", &env.screenWidth);
				env.halfWidth = env.screenWidth / 2;
				env.temp_screenWidth = env.screenWidth;
			} else if (!strcasecmp(field, "screenheight")
				  && !env.temp_screenHeight) {
				sscanf(value, "%d", &env.screenHeight);
				env.halfHeight = env.screenHeight / 2;
				env.temp_screenHeight = env.screenHeight;
			}
			else if (!strcasecmp(field, "scorehitunit"))
				sscanf(value, "%d", &env.scoreHitUnit);
			else if (!strcasecmp(field, "scoreselfhit"))
				sscanf(value, "%d", &env.scoreSelfHit);
			else if (!strcasecmp(field, "scoreroundwinbonus"))
				sscanf(value, "%d", &env.scoreRoundWinBonus);
			else if (!strcasecmp(field, "scoreteamhit"))
				sscanf(value, "%d", &env.scoreTeamHit);
			else if (!strcasecmp(field, "scoreunitdestroybonus"))
				sscanf(value, "%d", &env.scoreUnitDestroyBonus);
			else if (!strcasecmp(field, "scoreunitselfdestroy"))
				sscanf(value, "%d", &env.scoreUnitSelfDestroy);
			else if (!strcasecmp(field, "sellpercent"))
				sscanf(value, "%lf", &env.sellpercent);
			else if (!strcasecmp(field, "sounddriver"))
				sscanf(value, "%d", &env.sound_driver);
			else if (!strcasecmp(field, "startmoney"))
				sscanf(value, "%d", &env.startmoney);
			else if (!strcasecmp(field, "turntype"))
				sscanf(value, "%d", &env.turntype);
			else if (!strcasecmp(field, "violentdeath") )
				sscanf(value, "%d", &env.violent_death);
		}     // end of read a line properly
	}     // end of while not done
}


void GLOBALDATA::lockClass(eClasses class_)
{
	objLocks[class_].lock();
}


void GLOBALDATA::lockLand()
{
	landLock.lock();
}


void GLOBALDATA::make_bgupdate (int32_t x, int32_t y, int32_t w, int32_t h)
{
	if (lastUpdatesCount >= env.max_screen_updates) {
		make_fullUpdate();
		return;
	}

	assert( (w > 0) && (h > 0) );

	if ( (w > 0) && (h > 0) )
		addUpdate(x, y, w, h, lastUpdates, lastUpdatesCount);
}


void GLOBALDATA::make_fullUpdate()
{
	// Replace Updates with a full screen update:
	combineUpdates   = false;
	updateCount      = 0;
	lastUpdatesCount = 0;

	// They are split into 2 x 2 updates:
	for (int32_t x = 0; x < 2; ++x) {
		make_update(  env.halfWidth * x, 0,
		              env.halfWidth,     env.halfHeight);
		make_bgupdate(env.halfWidth * x, 0,
		              env.halfWidth,     env.halfHeight);
		make_update(  env.halfWidth * x, env.halfHeight,
		              env.halfWidth,     env.halfHeight);
		make_bgupdate(env.halfWidth * x, env.halfHeight,
		              env.halfWidth,     env.halfHeight);
	}

	combineUpdates   = true;
}


void GLOBALDATA::make_update (int32_t x, int32_t y, int32_t w, int32_t h)
{
	if (updateCount >= env.max_screen_updates) {
		make_fullUpdate();
		return;
	}

	// These asserts should catch screwed updates that make no sense
	assert( (h <= env.screenHeight) && (w <= env.screenWidth) );
	assert( (w > 0) && (h > 0) );

	if ( (h > 0) && (w > 0) )
		addUpdate(x, y, w, h, updates, updateCount);
}


void GLOBALDATA::newRound()
{
	if ( (currentround > 0) && (currentround-- < env.nextCampaignRound) )
		env.nextCampaignRound -= env.campaign_rounds;

	tankindex          = 0;
	naturals_activated = 0;
	combineUpdates     = true;

	// clean all but texts and tanks
	int32_t class_ = 0;
	while (class_ < CLASS_COUNT) {
		if ( (CLASS_FLOATTEXT != class_) && (CLASS_TANK != class_) ) {
			while (tails[class_])
				delete tails[class_];
		}
		++class_;
	}


	// Re-init land slide
	for (int32_t i = 0; i < env.screenWidth; ++i) {
		done[i]    = 2; // Check at once
		dropTo[i]  = env.screenHeight - 1;
		fp[i]      = 0;
	}

	// Init order array
	for (int32_t i = 0; i < MAXPLAYERS; ++i)
		order[i] = nullptr;
}


/// @brief Tell global that the close button was pressed
void GLOBALDATA::pressCloseButton()
{
	cbpLock.lock();
	close_button_pressed = true;
	cbpLock.unlock();
	set_command(GLOBAL_COMMAND_QUIT);
}


void GLOBALDATA::removeObject (vobj_t *object)
{
	if (nullptr == object)
		return;

	eClasses class_ = object->getClass();

	/// --- 1: Is the list empty? ---
	if (nullptr == heads[class_])
		return;

	objLocks[class_].lock();

	/// --- 2: If the object is head, set it anew:
	if (object == heads[class_])
		heads[class_] = object->next;

	/// --- 4: If the object is tail, set it anew:
	if (object == tails[class_])
		tails[class_] = object->prev;

	/// --- 5: Take it out of the list:
	if (object->prev)
		object->prev->next = object->next;
	if (object->next)
		object->next->prev = object->prev;
	object->prev = nullptr;
	object->next = nullptr;

	objLocks[class_].unlock();
}


void GLOBALDATA::removeTank(TANK* tank)
{
	if (nullptr == tank)
		return;

	for (int32_t i = 0 ; i < MAXPLAYERS ; ++i) {
		if (tank == order[i])
			order[i] = nullptr;
	}
}


void GLOBALDATA::replace_canvas ()
{

	for (int32_t i = 0; i < lastUpdatesCount; ++i) {
		if ((lastUpdates[i].y + lastUpdates[i].h) > MENUHEIGHT) {
			blit (env.sky, canvas, lastUpdates[i].x, lastUpdates[i].y - MENUHEIGHT,
			                       lastUpdates[i].x, lastUpdates[i].y,
			                       lastUpdates[i].w, lastUpdates[i].h);
			masked_blit (terrain, canvas, lastUpdates[i].x, lastUpdates[i].y,
			                              lastUpdates[i].x, lastUpdates[i].y,
			                              lastUpdates[i].w, lastUpdates[i].h);
		} // End of having an update below the top bar
	}

	int32_t l = 0;
	int32_t r = env.screenWidth - 1;
	int32_t t = MENUHEIGHT;
	int32_t b = env.screenHeight - 1;

	vline(canvas, l,     t, b, env.wallColour); // Left edge
	vline(canvas, l + 1, t, b, env.wallColour); // Left edge
	vline(canvas, r,     t, b, env.wallColour); // right edge
	vline(canvas, r - 1, t, b, env.wallColour); // right edge
	hline(canvas, l,     b, r, env.wallColour); // bottom edge
	if (env.isBoxed)
		hline(canvas, l, t, r, env.wallColour); // top edge

	lastUpdatesCount = 0;
}


// Set a new command, lock guarded
void GLOBALDATA::set_command(int32_t cmd)
{
	cmdLock.lock();
	command = cmd;
	cmdLock.unlock();
}


void GLOBALDATA::set_curr_tank(TANK* tank_)
{
	if (tank_ != currTank) {
		if (currTank)
			currTank->deactivate();
		currTank = tank_;
		if (currTank)
			currTank->activate();
	}
}


/** @brief go through the land and slide what is to be slid and is not locked
  * Slide land basic control is done using the 'done[]' array.
  * done[x] == 0 : Nothing to do. All values assumed to be correct.
  * done[x] == 1 : This column is currently in sliding.
  * done[x] == 2 : This column is about to be slid, but the base values aren't set.
  * done[x] == 3 : This column is about to be slid but locked. (Explosion not done)
**/
void GLOBALDATA::slideLand()
{
	// Opt out soon if no landslide is to be done
	if ( (SLIDE_NONE      == env.landSlideType)
	  || (SLIDE_TANK_ONLY == env.landSlideType)
	  || ( (SLIDE_CARTOON == env.landSlideType)
		&& (env.time_to_fall > 0) ) )
		return;

	for (int32_t col = 1; col < (env.screenWidth - 1); ++col) {

		// Skip this column if it is done or locked
		if (!done[col] || (3 == done[col]))
			continue;

		// Set base settings if this hasn't happen, yet
		if (2 == done[col]) {
			surface[col].store(0, ATOMIC_WRITE);
			dropTo[col] = env.screenHeight - 1;
			done[col]   = 1;

			// Calc the top and bottom of the column to slide

			// Find top-most non-PINK pixel
			int32_t row = MENUHEIGHT + (env.isBoxed ? 1 : 0);

			for ( ;(row < dropTo[col])
				&& (PINK == getpixel(terrain, col, row));
				++row) ;
			surface[col].store(row, ATOMIC_WRITE); // This is the top pixel with all gaps

			// Find bottom-most PINK pixel
			int32_t top_row = row;
			for (row = dropTo[col];
				   (row > top_row)
				&& (PINK != getpixel(terrain, col, row));
				--row) ;
			dropTo[col] = row;

			// Find bottom-most unsupported pixel
			for ( ;(row >= top_row)
				&& (PINK == getpixel(terrain, col, row));
				--row) ;

			// Check whether there is anything to do or not
			if ((row >= top_row) && (top_row < dropTo[col])) {
				fp[col]       = row - top_row + 1;
				velocity[col] = 0; // Not yet
				done[col]     = 1; // Can be processed
			}

			// Otherwise this column is done
			else {
				if ( !skippingComputerPlay
				  && (velocity[col] > .5)
				  && (fp[col] > 1) )
					play_natural_sound(DIRT_FRAGMENT, col, 64,
							1000 - (fp[col] * 800 / env.screenHeight));
				done[col] = 0; // Nothing to do
				fp[col]   = 0;
			}
		} // End of preparations

		// Do the slide if possible
		if (1 == done[col]) {

			// Only slide if no neighbours are locked
			bool can_slide = true;
			for (int32_t j = col - 1; can_slide && (j > 0) ; --j) {
				if (3 == done[j])
					can_slide = false;
				else if (!done[j])
					j = 0; // no further look needed.
			}
			for (int32_t j = col + 1; can_slide && (j < (env.screenWidth - 1)) ; ++j) {
				if (3 == done[j])
					can_slide = false;
				else if (!done[j])
					j = env.screenWidth; // no further look needed.
			}

			if (can_slide) {
				// Do instant first, because only GRAVITY remains
				// which is the case if cartoon wait time is over.
				if ( (SLIDE_INSTANT == env.landSlideType) || skippingComputerPlay) {
					int32_t surf = surface[col].load(ATOMIC_READ);
					make_bgupdate (col, surf, 1, dropTo[col] - surf + 1);
					make_update   (col, surf, 1, dropTo[col] - surf + 1);
					blit (terrain, terrain, col, surf, col,
					      dropTo[col] - fp[col] + 1, 1, fp[col]);
					vline(terrain, col, surf, dropTo[col] - fp[col], PINK);
					velocity[col] = fp[col]; // Or no sound would be played if done
					done[col]     = 2; // Recheck
				} else {
					velocity[col] += env.gravity;
					dropIncr[col] += velocity[col];

					int32_t dropAdd = ROUND(dropIncr[col]);
					int32_t max_top = MENUHEIGHT + (env.isBoxed ? 1 : 0);

					if (dropAdd > 0) {

						int32_t top_row = surface[col].load(ATOMIC_READ);

						assert( (top_row >= 0)
						     && (top_row < terrain->h)
						     && "ERROR: top_row out of range!");

						// If the top pixel is not PINK, and the source is not
						// too high, increase dropAdd:
						int32_t over_top = top_row - dropAdd;
						while ( ( over_top <= max_top)
						     && ( over_top >  0)
						     && ( PINK != getpixel(terrain, col, over_top) ) ) {
								++dropAdd;
								--over_top;
						}

						if (dropAdd > (dropTo[col] - (top_row + fp[col])) ) {
							dropAdd       = static_cast<int32_t>(dropTo[col]
							                               - (top_row + fp[col])
							                               + 1);
							dropIncr[col] = dropAdd;
							done[col]     = 2; // Recheck
							over_top      = top_row - dropAdd;
						}

						int32_t slide_height = fp[col] + dropAdd;

						assert( (over_top >= 0)
						     && (over_top < terrain->h)
						     && "ERROR: top_row - dropAdd out of range!");
						assert( (slide_height > 0)
						     && (slide_height <= terrain->h)
						     && "ERROR: slide_height out of range!");
						assert( (  (over_top + slide_height) <= terrain->h)
						     && "ERROR: over_top + slide_height is out of range!");
						assert( (  (top_row + slide_height) <= terrain->h)
						     && "ERROR: over_top + slide_height is out of range!");

						blit (terrain, terrain,
								col, over_top,
								col, top_row, 1,
								slide_height);
						make_bgupdate(col, over_top, 1,
						              slide_height + dropAdd + 1);
						make_update  (col, over_top, 1,
						              slide_height + dropAdd + 1);
						// If the top row reaches to the ceiling, there might
						// not be a PINK pixel to blit. In that case, one has
						// to be painted "by hand", or the slide will produce
						// nice long columns. (Happens with dirt balls when
						// "fixed" under the menubar.
						if (over_top <= max_top) {
							putpixel(terrain, col, max_top,     PINK);
							putpixel(terrain, col, max_top + 1, PINK);
						}

						surface[col].fetch_add(dropAdd);
						dropIncr[col] -= dropAdd;
					}
				}
			}
		} // End of actual slide
	} // End of looping columns
}


void GLOBALDATA::unlockClass(eClasses class_)
{
	objLocks[class_].unlock();
}


void GLOBALDATA::unlockLand()
{
	landLock.unlock();
}


/// @brief goes through the columns from @a left to @a right and unlocks what is locked.
void GLOBALDATA::unlockLandSlide(int32_t left, int32_t right)
{
	// Opt out soon if no landslide is to be done
	if ( (SLIDE_NONE      == env.landSlideType)
	  || (SLIDE_TANK_ONLY == env.landSlideType) )
		return;

	int32_t minX = std::min(left, right);
	int32_t maxX = std::max(left, right);

	if (minX < 1)
		minX = 1;
	if (maxX > (env.screenWidth - 1) )
		maxX =  env.screenWidth - 1;

	for (int32_t col = minX; col <= maxX; ++col) {
		if ((done[col] > 2) || !done[col])
			done[col] = 2;
	}
}

#ifndef USE_MUTEX_INSTEAD_OF_SPINLOCK

/// === Spin Lock Implementations ===

/// @brief Default ctor
CSpinLock::CSpinLock() :
	is_destroyed(ATOMIC_VAR_INIT(false))
{
	lock_flag.clear(); // Done this way, because VC++ can't do it normally.
	owner_id = std::thread::id();
}


/// @brief destructor - mark as destroyed, lock and go
CSpinLock::~CSpinLock()
{
	std::thread::id this_id = std::this_thread::get_id();
	bool need_lock = (owner_id != this_id);

	if (need_lock)
		lock();
	is_destroyed.store(true);
	if (need_lock)
		unlock();
}


/// @brief return true if this thread has an active lock
bool CSpinLock::hasLock()
{
	// This works, because unlock() sets the owner_id to -1.
	return (std::this_thread::get_id() == owner_id);
}


/** @brief Get a lock
  * Warning: No recursive locking possible! Only lock once!
**/
void CSpinLock::lock()
{
	std::thread::id this_id = std::this_thread::get_id();
	assert( (owner_id != this_id) && "ERROR: Lock already owned!");

	if (false == is_destroyed.load(ATOMIC_READ)) {
		while (lock_flag.test_and_set()) {
			std::this_thread::yield();
		}
		owner_id = this_id;
	}
}

/// @brief unlock if this thread owns the lock. Otherwise do nothing.
void CSpinLock::unlock()
{
	std::thread::id this_id = std::this_thread::get_id();
	assert( (owner_id == this_id) && "ERROR: Lock *NOT* owned!");

	if (owner_id == this_id) {
		owner_id = std::thread::id();
		lock_flag.clear(std::memory_order_release);
	}
}

#endif // USE_MUTEX_INSTEAD_OF_SPINLOCK

