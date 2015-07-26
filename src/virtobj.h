#ifndef	VIRTOBJ_DEFINE
#define	VIRTOBJ_DEFINE

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

#ifndef _PURE
#define _PURE =0
#endif // _PURE

#include "main.h"
#include "text.h"


/// @enum ePhysType
/// @brief Determine which kind of physics should be used
enum ePhysType
{
	PT_NORMAL = 0,  //!< No special processing, just a normal curve shot and impact.
	PT_FUNKY_FLOAT, //!< Funky bomb-lets ignore gravitation.
	PT_DIGGING,     //!< Burrowers and the like dig through dirt in a reverse curve.
	PT_ROLLING,     //!< Roll over the surface.
	PT_DIRTBOUNCE,  //!< Dirt debris bounces off of dirt and all walls.
	PT_SMOKE,       //!< Smoke reacts on nothing but repulsor shields.
	PT_NONE,        //!< Special values for age caused detonation triggering.
};


#ifndef HAS_PLAYER
class PLAYER;
#endif // HAS_PLAYER

class VIRTUAL_OBJECT
{
public:

	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */
	explicit VIRTUAL_OBJECT();
	virtual	~VIRTUAL_OBJECT();


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	/* --- non-inline methods --- */
	void         addUpdateArea (int32_t left,  int32_t top,
	                            int32_t width, int32_t height);
	virtual void applyPhysics  ();
	virtual void draw          ();
	virtual void initialise    ();
	void         setUpdateArea (int32_t left, int32_t top,
	                            int32_t width, int32_t height);
	void         update        ();

	/* --- inline methods --- */
	void         requireUpdate () { needsUpdate.store(true, ATOMIC_WRITE); }

	/* --- pure virtual (abstract) methods --- */
	virtual eClasses getClass  ()  _PURE;


	/* ------------------------------
	 * --- templated list getters ---
	 * ------------------------------
	 */

	/// @brief If not nullptr, set @a prev_ to the predecessor of this.
	template<typename obj_T>
	void getPrev(obj_T** prev_)
	{
		obj_T* prev_obj = static_cast<obj_T*>(prev);
		if (prev_)
			*prev_ = prev_obj;
	}

	/// @brief If not nullptr, set @a next_ to the successor of this.
	template<typename obj_T>
	void getNext(obj_T** next_)
	{
		obj_T* next_obj = static_cast<obj_T*>(next);
		if (next_)
			*next_ = next_obj;
	}

	/* ----------------------
	 * --- Public members ---
	 * ----------------------
	 */

	bool            destroy = false;
	VIRTUAL_OBJECT* next    = nullptr;
	PLAYER*         player  = nullptr;
	VIRTUAL_OBJECT* prev    = nullptr;
	double          x       = 0.;
	double          y       = 0.;

protected:


	/* -------------------------
	 * --- Protected methods ---
	 * -------------------------
	 */

	BITMAP* getBitmap() const { return bitmap; }
	bool            hasBitmap() const { return (bitmap != nullptr); }
	void            setBitmap(BITMAP* bitmap_);


	/* -------------------------
	 * --- Protected members ---
	 * -------------------------
	 */

	int32_t   age      = 0;
	alignType align    = LEFT;
	int32_t   angle    = 0;
	BOX       dim_cur;
	BOX       dim_old;
	int32_t   height   = 0;
	int32_t   maxAge   = -1;
	ePhysType physType = PT_NORMAL; // Special physics processing?
	int32_t   width    = 0;
	double    xv       = 0.;
	double    yv       = 0.;

private:

	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	BITMAP* bitmap = nullptr;
	abool_t         needsUpdate;
};

/// === Shorten the usage of virtual objects ===
typedef VIRTUAL_OBJECT vobj_t;

#endif // VIRTOBJ_DEFINE
