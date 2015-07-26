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

#include <cassert>
#include "virtobj.h"
#include "environment.h"

VIRTUAL_OBJECT::VIRTUAL_OBJECT() :
	needsUpdate(ATOMIC_VAR_INIT(false))
{ /* nothing to do */ }




VIRTUAL_OBJECT::~VIRTUAL_OBJECT ()
{
	bitmap = nullptr;
}


void VIRTUAL_OBJECT::addUpdateArea (int32_t left, int32_t top,
                                    int32_t width, int32_t height)
{
	if (left < dim_cur.x)
		dim_cur.x = left;
	if (top < dim_cur.y)
		dim_cur.y = top;
	/* This is prone to the following error:
	   If left is greater than dim_cur.x but the width is
	   smaller than dim_cur.w, (left + width) can
	   nevertheless end up right of (dim_cur.x + dim_cur.w).
	   Setting dim_cur.w to 'width' in that case makes
	   the update area smaller not larger.
	   The same applies to the height.
	   - Sven
	if ((left + width) > (dim_cur.x + dim_cur.w))
		dim_cur.w = width;
	if ((top + height) > (dim_cur.y + dim_cur.h))
		dim_cur.h = height;
	*/
	int32_t new_r = left + width;
	int32_t new_b = top + height;
	int32_t old_r = dim_cur.x + dim_cur.w;
	int32_t old_b = dim_cur.y + dim_cur.h;
	if (new_r > old_r)
		dim_cur.w = new_r - dim_cur.x + 1;
	if (new_b > old_b)
		dim_cur.h = new_b - dim_cur.y + 1;
}


void VIRTUAL_OBJECT::applyPhysics ()
{
	x += xv;
	y += yv;
}


void VIRTUAL_OBJECT::draw ()
{
	assert(bitmap && "ERROR: VIRTUAL_OBJECT::draw() called without bitmap!");

	if (!destroy && bitmap) {

		rotate_sprite (global.canvas, bitmap,
		               x - (width / 2),
		               y - (height / 2),
		               itofix (angle));

		// The update area depends on the rotation state (aka angle)
		if (angle) {
			int32_t length =  std::max(width, height)
			               + (std::min(width, height) / 2);
			setUpdateArea(x - (length/2),
			              y - (length/2),
			              length, length);
		} else
			setUpdateArea(x - (width / 2) - 1,
			              y - (height / 2) - 1,
			              width + 2, height + 2);
		requireUpdate ();
	}
}


void VIRTUAL_OBJECT::initialise ()
{
	age    = 0;
	maxAge = -1;
	x      = 0;
	y      = 0;
	xv     = 0;
	yv     = 0;
	destroy = false;
	dim_cur = dim_old = BOX();
}


/// @brief Set a new bitmap and store width and height for easy drawing.
void VIRTUAL_OBJECT::setBitmap(BITMAP* bitmap_)
{
	if (bitmap_ != bitmap) {
		bitmap = bitmap_;

		if (bitmap) {
			height = bitmap->h;
			width  = bitmap->w;
		} else {
			height = 0;
			width  = 0;
		}
	}
}


void VIRTUAL_OBJECT::setUpdateArea (int32_t left, int32_t top,
                                    int32_t width, int32_t height)
{
	dim_cur.x = left;
	dim_cur.y = top;
	dim_cur.w = width;
	dim_cur.h = height;
}


/** @brief update
  *
  * This method triggers an update of the canvas (aka drawing area) with the
  * dimensions and position of this object.
  */
void VIRTUAL_OBJECT::update()
{
	if (!needsUpdate.load(ATOMIC_READ))
		return;

	// Add update area for the current dimension
	if (dim_cur.w > 0) {
		int32_t left   = LEFT  == align ? dim_cur.x
		               : RIGHT == align ? dim_cur.x -  dim_cur.w
		               :                  dim_cur.x - (dim_cur.w / 2);
		int32_t top    = LEFT  == align ? dim_cur.y
		               : RIGHT == align ? dim_cur.y -  dim_cur.h
		               :                  dim_cur.y - (dim_cur.h / 2);
		int32_t right  = std::min(env.screenWidth, left + dim_cur.w + 2);
		int32_t bottom = std::min(env.screenHeight, top + dim_cur.h + 2);

		if ( (right > left) && (bottom > top) )
			global.make_update(left, top, right - left, bottom - top);
	} // End of updating current area

	// If the dimensions changed, the old area needs an update, too
	if ( (dim_old.w > 0) && (dim_old != dim_cur) ) {
		int32_t left   = LEFT  == align ? dim_old.x
		               : RIGHT == align ? dim_old.x -  dim_old.w
		               :                  dim_old.x - (dim_old.w / 2);
		int32_t top    = LEFT  == align ? dim_old.y
		               : RIGHT == align ? dim_old.y -  dim_old.h
		               :                  dim_old.y - (dim_old.h / 2);
		int32_t right  = std::min(env.screenWidth, left + dim_old.w + 2);
		int32_t bottom = std::min(env.screenHeight, top + dim_old.h + 2);

		if ( (right > left) && (bottom > top) )
			global.make_update(left, top, right - left, bottom - top);
	} // End of updating old area

	dim_old = dim_cur;

	needsUpdate.store(false, ATOMIC_WRITE);
}

