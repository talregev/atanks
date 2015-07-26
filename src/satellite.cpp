#include "environment.h"
#include "satellite.h"
#include "beam.h"


SATELLITE::SATELLITE() :
	x(env.screenWidth / 2)
{
	prev_x = x;
}


void SATELLITE::draw()
{
	drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);
	draw_sprite(global.canvas, env.misc[SATELLITE_IMAGE], x, y);
	global.make_update(x - 20, y, 80, 60);
	global.make_update(prev_x, y, 80, 60);
}


void SATELLITE::move()
{
	// Be sure an owned beam is valid
	if (beam && beam->destroy)
		beam = nullptr;

	// reverse movement if the satellite reaches the screen borders
	if (x < -5)
		xv += 1;
	else if (x > (env.screenWidth - 20) )
		xv -= 1;

	prev_x = x;
	x += xv;

	// If the satellite is firing, move the beam
	if (beam)
		beam->moveStart(xv < 0 ? x + 10: x + 40, y + 20);
}


void SATELLITE::shoot()
{
	if ( (SL_NONE != env.satellite)
	  && (global.naturals_activated < 4)
	  && (nullptr == beam)
		// 1% chance to fire
	  && (!(rand() % 100)) ) {
		try {
			beam = new BEAM(nullptr, xv < 0 ? x + 10: x + 40, y + 20,
							rand() % 35 + (xv < 0 ? 320 : 5),
							SML_LAZER + (rand() % env.satellite),
							BT_NATURAL);
			global.naturals_activated++;
		} catch (...) {
			// No problem... fire another time. ;)
		}
	}
}



