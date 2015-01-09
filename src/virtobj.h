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

enum    alignType { CENTRE, LEFT, RIGHT };

#include "main.h"

#include "player.h"

class VIRTUAL_OBJECT
  {
  public:
    BOX	_current, _old;
    alignType	_align;
    BITMAP	*_bitmap;
    ENVIRONMENT	*_env;
    GLOBALDATA	*_global;
    int	_requireUpdate;
    int	_index;
    PLAYER *player;
    double	x, y;
    double	xv, yv;
    int	angle;
    int	destroy;
    int	age, maxAge;
    int	physics;	// Special physics processing?

    /* --- constructor --- */
    VIRTUAL_OBJECT();

    /* --- destructor --- */
    virtual	~VIRTUAL_OBJECT ();

    /* --- non-inline methods --- */
    void	update();

    /* --- pure virtual (abstract) methods --- */
    virtual int	  isSubClass (int classNum)_PURE;
    virtual int	  getClass ()_PURE;
    virtual void  setEnvironment(ENVIRONMENT *env)_PURE; // This is virtual, so objects add themselves to _env!

    /* --- inline methods --- */
    void	requireUpdate ()
    {
      _requireUpdate = TRUE;
    }

    virtual void	initialise ();

    virtual int	applyPhysics ();

    inline void	setGlobalData (GLOBALDATA *global)
    {
      if (!_global || (_global != global))
        _global = global;
    }

virtual void	draw (BITMAP *dest);

void	setUpdateArea (int left, int top, int width, int height);

void	addUpdateArea (int left, int top, int width, int height);

    inline int	getIndex ()
    {
      return (_index);
    }
  };

#endif // VIRTOBJ_DEFINE
