#pragma once
#ifndef ATANKS_SRC_DEBRIS_POOL_H_INCLUDED
#define ATANKS_SRC_DEBRIS_POOL_H_INCLUDED

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
 *
 */

#include "main.h"

/** @struct sDebrisItem
  * @brief represent one entry in the debris pool
**/
struct sDebrisItem
{
    BITMAP*      bmp      = nullptr;
	int32_t      idx      = 0;       //!< calculated index from diameter (aka radius/2-1)
    bool         is_free  = true;
    sDebrisItem* next     = nullptr;
    sDebrisItem* prev     = nullptr;

	explicit sDebrisItem(int32_t diameter_, sDebrisItem* next_);
	~sDebrisItem();
};


/** @struct sDebrisPool
  * @brief A pool of bitmaps used to throw around dirt.
  *
  * Note: Currently the pool is limited to radius [1;5] the
  *       debris can have. That is five series of bitmaps.
**/
struct sDebrisPool
{
	typedef sDebrisItem item_t;

	explicit sDebrisPool(int32_t limit_);
	~sDebrisPool();

	void    free_item(item_t* item);
	item_t* get_item (int32_t radius);

private:

	item_t* create_item(int32_t radius);

	int32_t avail[5];      //!< How many items are available for which radius.
	int32_t count_all = 0; //!< Sum of all created items.
	int32_t counts[5];     //!< How many items are used for which radius.
	int32_t limit     = 0; //!< The limit of the pool, set on pool creation.
	item_t* heads[5];
	item_t* tails[5];
};


#endif // ATANKS_SRC_DEBRIS_POOL_H_INCLUDED

