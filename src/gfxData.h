#pragma once
#ifndef ATANKS_SRC_GFXDATA_H_INCLUDED
#define ATANKS_SRC_GFXDATA_H_INCLUDED


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

#define ALL_SKIES 16
#define ALL_LANDS 16

#define	STUFF_BAR_WIDTH	 400
#define	STUFF_BAR_HEIGHT 35

#define EXPLODEFRAMES  18
#define	DISPERSEFRAMES 10
#define	EXPLOSIONFRAMES	(EXPLODEFRAMES + DISPERSEFRAMES)

#define EXPLO_CX 107.f
#define EXPLO_CY 107.f
#define EXPLO_H  214.f
#define EXPLO_W  214.f

#define FLAME_CX 300.f
#define FLAME_CY  15.f
#define FLAME_H   30.f
#define FLAME_W  600.f

/// @brief Consolidate global gfx data in a struct to have RAII in effect.
struct sGfxData
{
	explicit sGfxData();
	~sGfxData();

	void destroy();
	void first_init();

	BITMAP* sky_gradient_strips[ALL_SKIES];
	BITMAP* land_gradient_strips[ALL_LANDS];
	BITMAP* stuff_bar_gradient_strip = nullptr;
	BITMAP* topbar_gradient_strip    = nullptr;
	BITMAP* explosion_gradient_strip = nullptr;
	BITMAP* stuff_bar[2];
	BITMAP* stuff_icon_base          = nullptr;
	BITMAP* topbar                   = nullptr;
	BITMAP* explosions[EXPLOSIONFRAMES];
	BITMAP* flameFront[EXPLOSIONFRAMES];

private:
	bool initDone = false;
};


// === Helper Functions ===
// ========================
BITMAP* create_gradient_strip (const gradient* grad, int32_t len);
int32_t gradientColorPoint    (const gradient* grad, double len, double line);



#endif // ATANKS_SRC_GFXDATA_H_INCLUDED
