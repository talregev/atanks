#include "environment.h"
#include "satellite.h"
#include "beam.h"


SATELLITE::SATELLITE(GLOBALDATA *global, ENVIRONMENT *env):_global(global),_env(env)
{ }

SATELLITE::~SATELLITE()
{
  _env    = NULL;
  _global = NULL;
}


void SATELLITE::Init()
{
  x = _global->screenWidth / 2;
  previous_x = x;
  y = 35;
  xv = -2;
}


void SATELLITE::Move()
{
  if (x < -5)
    xv += 1;
  else if (x > (_global->screenWidth - 10) )
    xv -= 1;

  previous_x = x;
  x += xv;
}



void SATELLITE::Draw(BITMAP *dest)
{
  drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);
  draw_sprite(dest, (BITMAP *) _global->misc[SATELLITE_IMAGE],
              x, y);

  _env->make_update(x - 20, y, 80, 60);
  _env->make_update(previous_x, y, 80, 60);
}



void SATELLITE::Shoot()
{
  int chance;

  if (_env->satellite == LASER_NONE)
    return;

  if (_env->naturals_since_last_shot >= 3)
    return;

  chance = rand() % 100;
  if (! chance)        // !% chance to fire
    {
      int laser_type;

      if (_env->satellite == LASER_WEAK) laser_type = SML_LAZER;
      else if (_env->satellite == LASER_STRONG) laser_type = MED_LAZER;
      else if (_env->satellite == LASER_SUPER) laser_type = LRG_LAZER;
      else return;

      int angle = rand() % 30;
      BEAM *my_beam = new BEAM(_global, _env, (xv < 0) ? x + 10: x + 40,
                               y + 20, angle, laser_type);
      if (! my_beam)
        return;
      my_beam->player = NULL;
      _env->naturals_since_last_shot++;
    }
}



