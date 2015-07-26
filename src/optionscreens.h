#pragma once
#ifndef ATANKS_SRC_OPTIONSCREENS_H_INCLUDED
#define ATANKS_SRC_OPTIONSCREENS_H_INCLUDED

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

/** @file optionscreens.h
  * @brief Here the functions providing option screens and menus are declared
**/

#include "menu.h"

void    drawMenuBackground (int32_t itemType, int32_t tOffset, int32_t numItems);
void    editPlayers        ();
void    optionsMenu        ();
int32_t selectPlayers      ();


#endif // ATANKS_SRC_OPTIONSCREENS_H_INCLUDED

