#ifndef BEAM_DEFINE
#define BEAM_DEFINE

/*
atanks - obliterate each other with oversize weapons
Copyright (C) 2003  Thomas Hudson

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "main.h"
#include "physobj.h"


/** @enum eBeamType
  * @brief Determines what kind of beam is generated
**/
enum eBeamType
{
	BT_WEAPON = 0, //!< Normal weapon, nothing special
	BT_SDI,        //!< Not a weapon but an SDI laser
	BT_NATURAL,    //!< Fired by natural disaster, like lightning.
	BT_MIND_SHOT   //!< AI thinking.
};


class BEAM: public PHYSICAL_OBJECT
{
public:

	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	explicit BEAM ( PLAYER* player_, double x_, double y_,
					int32_t fireAngle, int32_t weaponType,
					eBeamType beam_type);
	BEAM          ( PLAYER* player_, double x_, double y_,
					double tx, double ty, int32_t weaponType,
					bool is_burnt_out);
	~BEAM ();


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	void	 applyPhysics();
    void	 draw        ();
    void     getEndPoint (int32_t &x, int32_t &y); // For mind shots to fetch
	void     moveStart   (double x_, double y_);   // For the satellite

	eClasses getClass() { return CLASS_BEAM; }


private:

	/* -----------------------
	 * --- Private methods ---
	 * -----------------------
	 */

	void createBeamPath();
	void makeLightningPath();
	void traceBeamPath ();


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	eBeamType beamType  = BT_WEAPON;
	int32_t   color     = WHITE;
	double    damage    = 0.;
	int32_t   numPoints = 0;
	POINT_t*  points    = nullptr;
	int32_t   radius    = 0;
	int32_t   seed      = 0;
	int32_t   tgtLeftX  = 0;
	int32_t   tgtRightX = 0;
	WEAPON*   weap      = nullptr;
};

#endif

