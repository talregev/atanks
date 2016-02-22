#ifndef MISSILE_DEFINE
#define MISSILE_DEFINE

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


#include "main.h"
#include "physobj.h"

// The ages are *seconds* and transformed to frames in the ctor.
#define MAX_JELLY_AGE    1
#define MAX_MISSILE_AGE 15
#define MAX_METEOR_AGE   5

#define SDI_DISTANCE   100
#define TRIGGER_HEIGHT 300


/** @enum eMissileType
  * @brief Determines what kind of weapon is shot
**/
enum eMissileType
{
	MT_WEAPON = 0, //!< Normal weapon, nothing special
	MT_ITEM,       //!< Not a weapon but an item
	MT_NATURAL,    //!< Fired by natural disaster, like meteors and dirt balls.
	MT_MIND_SHOT   //!< AI thinking.
};


class MISSILE: public PHYSICAL_OBJECT
{
public:

	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	explicit MISSILE (PLAYER* player_, double xpos, double ypos,
	                  double xvel, double yvel,
	                  int32_t weapon_type, eMissileType missile_type,
	                  int32_t ai_level_, int32_t delay_idx_);
	~MISSILE ();


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	void    applyPhysics();
	int32_t bounced     () const;
	int32_t direction   () const;
	void    draw        ();

	eClasses getClass() { return CLASS_MISSILE; }

	void    update_submun(ePhysType p_type, int32_t cnt_down);


private:

	/* -----------------------
	 * --- Private methods ---
	 * -----------------------
	 */

	void    Check_SDI(); // see if missile should be shot down
	int32_t Height_Above_Ground();
	void    Repulse_Missile();
	void    trigger     ();
	void    triggerTest ();


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	int32_t       ai_level     =  0; // Level of the AI shooting a mind shot
	int32_t       countdown    = -1;
	int32_t       funky_colour = BLACK;
	int32_t       growRadius   =  0;
	bool          isGrowing    = false;
	eMissileType  missileType  = MT_WEAPON;
	WEAPON*       weap         = nullptr;
};

#endif

