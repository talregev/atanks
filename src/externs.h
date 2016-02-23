#pragma once
#ifndef ATANKS_SRC_EXTERNS_H_INCLUDED
#define ATANKS_SRC_EXTERNS_H_INCLUDED

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

#include "globaldata.h"
#ifndef HAS_GLOBALDATA
class GLOBALDATA;
#endif // HAS_GLOBALDATA

#include "environment.h"
#ifndef HAS_ENVIRONMENT
class ENVIRONMENT;
#endif // HAS_ENVIRONMENT

#define CLOCK_MAX 10

#ifndef ATANKS_SRC_ATANKS_CPP

// === The two most important things in the game: ;) ===
extern GLOBALDATA  global;
extern ENVIRONMENT env;


// === Defined colours used everywhere ===
extern int32_t BLACK, BLUE, DARK_GREEN, DARK_GREY, DARK_RED, GOLD, GREY,
               GREEN, LIGHT_GREEN, LIME_GREEN, ORANGE, PINK, PURPLE, RED,
               SILVER, TURQUOISE, WHITE, YELLOW;


// === General values that are globally used ===
extern char        buf[100];
extern const char* errorMessage;
extern int32_t     errorX, errorY;
extern int32_t     k, K;
extern int32_t     fi, lx, ly;
extern WEAPON      weapon[WEAPONS];    // from files.cpp
extern WEAPON      naturals[NATURALS]; // from files.cpp
extern ITEM        item[ITEMS];        // from files.cpp


// === Gradients ===
extern gradient  topbar_gradient[4];
extern gradient  stuff_bar_gradient[11];
extern gradient  circles_gradient[4];
extern gradient  explosion_gradient1[3];
extern gradient  explosion_gradient2[3];
extern gradient* explosion_gradients[2];

#endif // ATANKS_SRC_ATANKS_CPP
#endif // ATANKS_SRC_EXTERNS_H_INCLUDED

