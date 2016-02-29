#ifndef	TANK_DEFINE
#define	TANK_DEFINE

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
#include "floattext.h"

#define DIR_RIGHT 1
#define DIR_LEFT -1

#define VIOLENT_CHANCE 6


class PLAYER;
class EXPLOSION;
class TANK: public PHYSICAL_OBJECT
{

public:

	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	explicit TANK ();
	~TANK ();


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	void    activate();
	void    activateCurrentSelection ();
	void    addDamage(PLAYER* damageFrom, double damage_);
	void    applyDamage();
	void    applyPhysics();
	void    check_weapon();
	void    deactivate();
	void    draw ();
	void    explode(bool allow_vengeance);
	int32_t getBottom();
	double  getDiameter();
	void    getGuntop(int32_t angle_, double &top_x, double &top_y);
	int32_t getMaxLife();
	bool    hasRepulsorActivated();
	int32_t howBuried(int32_t* left, int32_t* right);
	bool    isFlying();
	bool    isInBox(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
	bool    isInEllipse(double ex, double ey, double rx, double ry,
	                    double &in_rate_x, double &in_rate_y);
	bool    moveTank(int32_t direction);
	void    newRound (int32_t pos_x, int32_t pos_y);
	void    reactivate_shield ();
	void    repair();
	bool    repulse (double xpos, double ypos, double* xa, double* ya,
	                 ePhysType phys_type);
	void    resetFlashDamage();
	bool    shootClearance (int32_t targetAngle, int32_t minimumClearance,
	                        bool &crashed);
	void    simActivateCurrentSelection();

	eClasses getClass() { return CLASS_TANK; }


	/* ----------------------
	 * --- Public members ---
	 * ----------------------
	 */

	int32_t   a                 = 90;      // [a]ngle
	int32_t   cw                = SML_MIS; // [c]urrent [w]eapon
	int32_t   fire_another_shot = 0;
	FLOATTEXT healthText;
	int32_t   l                 = 100;     // [l]ive
	FLOATTEXT nameText;
	int32_t   p                 = MAX_POWER / 2; // [p]ower
	int32_t   sh                = 0;       // [sh]ield
	FLOATTEXT shieldText;
	int32_t   sht               = 0;       // [sh]ield [t]ype

private:

	/* -----------------------
	 * --- Private methods ---
	 * -----------------------
	 */

	void    setBitmap();
	void    setTextPositions(bool renew_colour);
	bool    tank_on_tank(); // is this tank on top of another?


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	PLAYER*       creditTo         = nullptr;
	double        damage           = 0.;
	CSpinLock     damage_lock;
	int32_t       delay_fall       = env.landSlideDelay * 100; // time the tank will hover
	int32_t       flashdamage      = 0;
	bool          isTeleported     = false; // Set to true if a teleport occurs to award falling damage.
	int32_t       maxLife          = 100; // amount awarded at beginning of round
	bool          newDamager       = false;
	int32_t       para             = 0;
	int32_t       repair_rate      = 0;
	int32_t       repulsion        = 0;
	int32_t       shld_col_inner   = BLACK;
	int32_t       shld_col_outer   = BLACK;
	double        shld_delta       = 360.; // divided by FPS in ctor
	double        shld_phase       = 0.; // Neutral
	int32_t       shld_rad_x       = 0; // Determined by the used bitmap
	int32_t       shld_rad_y       = 0; // Determined by the used bitmap
	int32_t       shld_thickness   = 0;
	double        tank_dia         = 1.; // Tank diameter, determined by the used bitmap
	int32_t       tank_off_x       = 0; // Determined by the used bitmap
	int32_t       tank_off_y       = 0; // Determined by the used bitmap
	int32_t       tank_sag         = 0; // Determined by the used bitmap
	int32_t       turr_off_x       = 0; // Determined by the used bitmap
	int32_t       turr_off_y       = 0; // Determined by the used bitmap
	int32_t       use_tankbitmap   = -1;
	int32_t       use_turretbitmap = -1;
};

#define HAS_TANK 1

#endif
