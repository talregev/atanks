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
#include "virtobj.h"
#include "physobj.h"

#define LIGHTNING_SOUND 12

enum beamType
{
  LIGHTNING_BEAM,
  LAZER_BEAM
};

class BEAM: public PHYSICAL_OBJECT
  {
  public:
    int	radius;
    int	length;
    double	damage;
    int	clock;
    int	type;
    WEAPON	*weap;
    int	*points;	// Allows jagged lines
    int	numPoints;
    int	color;
    int	targetX;
    int	targetY;

    virtual ~BEAM ();
    //BEAM (GLOBALDATA *global, ENVIRONMENT *env, double tX, double tY, double tXv, double tYv, int weaponType);
    BEAM (GLOBALDATA *global, ENVIRONMENT *env, double xpos, double ypos, int angle, int weaponType);
    void	setLightningPath();
    void	initialise ();
    void	draw (BITMAP *dest);
    //void	update ();
    int	applyPhysics ();
    virtual int	isSubClass (int classNum);
    inline virtual int	getClass ()
    {
      return (BEAM_CLASS);
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

