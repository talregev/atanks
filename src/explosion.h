#ifndef EXPLOSION_DEFINE
#define EXPLOSION_DEFINE

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

#define DIRT_CEILING 50


class EXPLOSION: public PHYSICAL_OBJECT
  {
  public:
    int	etime;
    int	damage;
    int	eframes;
    int	radius;
    int	sound;
    int	exclock;
    int	type, a;
    int	peaked;
    int	dispersing;
    WEAPON	*weap;
		/* to allow synchronous tank explosions, explosions need to know what they are */
		bool bIsWeaponExplosion;

    virtual ~EXPLOSION ();
    EXPLOSION (GLOBALDATA *global, ENVIRONMENT *env, double xpos, double ypos, int weaponType);
    EXPLOSION (GLOBALDATA *global, ENVIRONMENT *env, double xpos, double ypos, double xvel, double yvel, int weaponType);
    void	draw (BITMAP *dest);
    void	initialise ();
    int	applyPhysics ();
    void	explode ();
    int	isSubClass (int classNum);
    inline virtual int	getClass ()
    {
      return (EXPLOSION_CLASS);
    }
    inline virtual void setEnvironment(ENVIRONMENT *env)
    {
      if (!_env || (_env != env))
        {
          _env = env;
          _index = _env->addObject (this);
        }
    }
  };

#endif
