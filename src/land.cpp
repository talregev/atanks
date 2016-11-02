#include "land.h"
#include "files.h"
#include "externs.h"
#include "gameloop.h"


/*****************************************************************************
Static temp land bitmap for faster land creation
*****************************************************************************/
static BITMAP* temp_land = nullptr;



// Define how the land will look.
void generate_land (LevelCreator* lcr, int32_t xoffset, int32_t heightx)
{
	double* depthStrip[2] = { nullptr, nullptr };
	double  smoothness    = 100.;
	int32_t octaves       = 8;
	double  lambda        = .25;
	int32_t curland       = global.curland;
	int32_t land_type     = LAND_RANDOM == env.landType
	                      ? (rand () % LAND_PLAIN) + 1
	                      : env.landType;
	int32_t land_height   = heightx / 6 * 5;

	depthStrip[0] = (double *)calloc(env.screenHeight + 1, sizeof(double));
	depthStrip[1] = (double *)calloc(env.screenHeight + 1, sizeof(double));

	if (!depthStrip[0] || !depthStrip[1]) {
		cerr << "ERROR: Unable to allocate ";
		cerr << ( (env.screenHeight + 1) * 2 * sizeof(double));
		cerr << " bytes in LandGenerator() !" << endl;
		return;
	}

	switch (land_type) {
		case LAND_MOUNTAINS:
			smoothness = 200;
			octaves = 8;
			lambda = 0.65;
			break;
		case LAND_CANYONS:
			smoothness = 50;
			octaves = 8;
			lambda = 0.25;
			break;
		case LAND_VALLEYS:
			smoothness = 200;
			octaves = 8;
			lambda = 0.25;
			break;
		case LAND_HILLS:
			smoothness = 600;
			octaves = 6;
			lambda = 0.40;
			break;
		case LAND_FOOTHILLS:
			smoothness = 1200;
			octaves = 3;
			lambda = 0.25;
			break;
		case LAND_PLAIN:
			smoothness = 4000;
			octaves = 2;
			lambda = 0.2;
			break;
		case LAND_NONE:
		default:
		break;
	}

	temp_land = create_bitmap(env.screenWidth, env.screenHeight);
	clear_to_color(temp_land, PINK);
	clear_to_color(global.terrain, PINK);

	for (int32_t x = 0; lcr->can_work() && (x < env.screenWidth); ++x) {
		int32_t surface = 1;

		// surface[x] will end up being the y coordinate of the
		// top land pixel. It is not the real height (distance from bottom to
		// even level) but this coordinate.
		// This is why the array is called surface (again) and not (no longer)
		// height.

		if (land_type != LAND_NONE)
			surface = (1. + perlin2DPoint(1.0, smoothness, xoffset + x, 0,
			                              lambda, octaves) )
			        / 2. * land_height;
		global.surface[x].store(surface);
	} // End of generating surface array

	// If this is a wrapped landcape, smooth out both sides towards their
	// opposite counterparts
	if (WALL_WRAP == env.current_wallType) {
        int32_t length = env.screenWidth / 20; // 5% left + right = 10% overall

        for (int32_t x = 0; lcr->can_work() && (x < length) ; ++x) {
			// The idea is to compare the strips from left to right with the
			// points being taken by a ratio greater the nearer to its wall.

			int32_t left  = x;
			int32_t right = env.screenWidth - (x + 1);

			double  ratio_n = ( static_cast<double>(x)
			                  / static_cast<double>(length)
			                  / 2.) + .5;   // [n]ear: 50% at the wall, 100% at length.
			double  ratio_f = 1. - ratio_n; // [f]ar : 50% at the wall,   0% at length.

			// Get the heights currently set
			double  old_left_y  = global.surface[left ].load(ATOMIC_READ);
			double  old_right_y = global.surface[right].load(ATOMIC_READ);

			// Assemble new heights
			double  new_left_y  = (old_left_y  * ratio_n) + (old_right_y * ratio_f);
			double  new_right_y = (old_right_y * ratio_n) + (old_left_y  * ratio_f);

			// Now the new surface values can be stored:
			global.surface[left ].store(ROUND(new_left_y));
			global.surface[right].store(ROUND(new_right_y));
        }
	}

	// Generate detailed depths
	for (int32_t x = 0; lcr->can_work() && (x < env.screenWidth); ++x) {
		int32_t depth   = 0;
		int32_t surface = global.surface[x].load();
		if (env.detailedLandscape && (LAND_NONE != land_type)) {
			memcpy (depthStrip[0], depthStrip[1], env.screenHeight * sizeof(double));
			for (depth = 1; depth < env.screenHeight; depth++) {
				depthStrip[1][depth] = (1. + perlin2DPoint (1.0, smoothness,
				                                            xoffset + x,
				                                            depth,
				                                            lambda, octaves) )
				                     / 2. * land_height
				                     - (env.screenHeight - depth);
				if (depthStrip[1][depth] > surface )
					depthStrip[1][depth] = surface;
			}
			depthStrip[1][0] = 0;
			depth = 1;
		}

		// Now generate the height colourization
		for (int32_t y = 1; lcr->can_work() && (y <= surface); ++y) {

			lcr->yield();

			double        offset = 0;
			int32_t color  = BLACK;
			double        shade  = 0;

			if (env.detailedLandscape) {
				while ( (depth < env.screenHeight)
				     && (depthStrip[1][depth] <= y) )
					++depth;

				double bot    = (depthStrip[0][depth - 1]
				              +  depthStrip[1][depth - 1]) / 2;
				double minBot = std::min(depthStrip[0][depth - 1],
				                         depthStrip[1][depth - 1]);
				double maxTop = std::max(depthStrip[0][depth],
				                         depthStrip[1][depth]);
				double btdiff = maxTop - minBot;
				double i      = (y - bot) / btdiff;
				double a1     = RAD2DEG(atan2(depthStrip[0][depth - 1]
				                            - depthStrip[1][depth - 1], 1.0))
				              + 180.;
				double a2     = RAD2DEG(atan2(depthStrip[0][depth]
				                            - depthStrip[1][depth], 1.0))
				              + 180.;
				double angle  = interpolate (a1, a2, i);
				shade = env.slope[(int)angle][0];
			}

			if (env.ditherGradients)
				offset += rand () % 10 - 5;

			if (env.detailedLandscape)
				offset += (env.screenHeight - depth) * 0.5;

			while ( (y + offset) < 0)
				offset /= 2;
			while ( (y + offset) > env.screenHeight)
				offset /= 2;

			color = gradientColorPoint (land_gradients[curland], surface, y + offset);

			if (env.detailedLandscape) {
				float h, s, v;
				int32_t r = getr (color);
				int32_t g = getg (color);
				int32_t b = getb (color);
				rgb_to_hsv(r, g, b, &h, &s, &v);

				shade += (double)(rand () % 1000 - 500) * (1.0/10000);

				if (shade < 0)
					v += v * shade * 0.5;
				else
					v += (1 - v) * shade * 0.5;

				hsv_to_rgb (h, s, v, &r, &g, &b);
				color = makecol (r, g, b);
			}

			if (lcr->can_work()) {
				global.lockLand();
				solid_mode();
				putpixel (temp_land, x, env.screenHeight - y, color);
				drawing_mode(global.current_drawing_mode, NULL, 0, 0);
				global.unlockLand();
			}
		} // End of looping y coordinate
	} // end of looping x coordinate

	// Put temp land onto the real bitmap:
	global.lockLand();
	solid_mode();
	blit(temp_land, global.terrain, 0, 0, 0, 0, env.screenWidth, env.screenHeight);
	global.unlockLand();

	// clean up
	if (temp_land)
		destroy_bitmap(temp_land);
	temp_land = nullptr;

	if (depthStrip[0])
		free(depthStrip[0]);

	if (depthStrip[1])
		free(depthStrip[1]);
}

