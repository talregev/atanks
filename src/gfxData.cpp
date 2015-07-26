#include "main.h"
#include "gfxData.h"

/** @brief explicit constructor, because Visual C++ needs one.
**/
sGfxData::sGfxData()
{
	memset(sky_gradient_strips,  0, sizeof(BITMAP*) * ALL_SKIES);
	memset(land_gradient_strips, 0, sizeof(BITMAP*) * ALL_LANDS);
	memset(stuff_bar,            0, sizeof(BITMAP*) * 2);
	memset(explosions,           0, sizeof(BITMAP*) * EXPLOSIONFRAMES);
	memset(flameFront,           0, sizeof(BITMAP*) * EXPLOSIONFRAMES);
}


/** @brief sGfxData destructor - clean everything up
  */
sGfxData::~sGfxData()
{
	this->destroy();
}


/// @brief should be called from ENVIRONMENT::destroy();
void sGfxData::destroy()
{
	if (initDone) {
		if (topbar)
			destroy_bitmap(topbar);
		if (topbar_gradient_strip)
			destroy_bitmap(topbar_gradient_strip);
		if (stuff_bar[0])
			destroy_bitmap(stuff_bar[0]);
		if (stuff_bar[1])
			destroy_bitmap(stuff_bar[1]);
		if (stuff_icon_base)
			destroy_bitmap(stuff_icon_base);
		if (stuff_bar_gradient_strip)
			destroy_bitmap(stuff_bar_gradient_strip);
		if (explosion_gradient_strip)
			destroy_bitmap(explosion_gradient_strip);

		topbar = nullptr;
		topbar_gradient_strip = nullptr;
		stuff_bar[0] = nullptr;
		stuff_bar[1] = nullptr;
		stuff_icon_base = nullptr;
		stuff_bar_gradient_strip = nullptr;
		explosion_gradient_strip = nullptr;

		if (sky_gradient_strips) {
			for (int32_t i = 0; i < ALL_SKIES; ++i) {
				if ( sky_gradient_strips[i] ) {
					destroy_bitmap(sky_gradient_strips[i]);
					sky_gradient_strips[i] = nullptr;
				}
			}
		}

		if (land_gradient_strips) {
			for (int32_t i = 0; i < ALL_LANDS; ++i) {
				if ( land_gradient_strips[i] ) {
					destroy_bitmap(land_gradient_strips[i]);
					land_gradient_strips[i] = nullptr;
				}
			}
		}

		if (explosions) {
			for (int32_t i = 0; i < EXPLOSIONFRAMES; ++i) {
				if ( explosions[i] ) {
					destroy_bitmap(explosions[i]);
					explosions[i] = nullptr;
				}
			}
		}

		if (flameFront) {
			for (int32_t i = 0; i < EXPLOSIONFRAMES; ++i) {
				if ( flameFront[i] ) {
					destroy_bitmap(flameFront[i]);
					flameFront[i] = nullptr;
				}
			}
		}

		initDone = false;
	}
}


/// @brief create the gfx data to hold
void sGfxData::first_init()
{
	// Note: This method is mostly uncommented, because the original
	// function that did this was uncommented.

	if (initDone)
		return;

	int32_t colour_theme = static_cast<int32_t>(env.colourTheme);
	explosion_gradient_strip = create_gradient_strip(explosion_gradients[colour_theme], 200);
	double expSize     = 25.;
	double flmSize     = 10.;
	double expDisperse = 0.;
	double flmDisperse = 0.;

	for (int32_t i = 0; i < EXPLOSIONFRAMES; ++i) {
		explosions[i] = create_bitmap (214, 214);
		flameFront[i] = create_bitmap (600, 30);
		if (i < EXPLODEFRAMES - 4) {
			expSize += (107. - expSize) / 3.;
			flmSize += (300. - flmSize) / 3.;
		} else if (i < EXPLODEFRAMES) {
			expSize -= 1.;
			flmSize -= 1.;
		} else if (i == EXPLODEFRAMES) {
			expDisperse = 25.;
			flmDisperse = 10.;
		} else {
			expDisperse += (107. - expDisperse) / 2.;
			flmDisperse += (300. - flmDisperse) / 2.;
		}

		clear_to_color(explosions[i], PINK);
		clear_to_color(flameFront[i], PINK);

		for (int32_t y = std::floor(expSize); y > expDisperse; --y) {
			double value = pow (static_cast<double>(y) / expSize, i / 4 + 1);
			int32_t exp_col = getpixel(explosion_gradient_strip, 0,
									static_cast<int32_t>(value * 200.));
			circlefill (explosions[i], 107, 107, y, exp_col);
		}
		if (ROUND(expDisperse) > 0) {
			circlefill (explosions[i], 107, 107, ROUND(expDisperse), PINK);
		}

		for (int32_t y = std::floor(flmSize); y > flmDisperse; --y) {
			double value = pow (static_cast<double>(y) / flmSize, i / 4 + 1);
			int32_t flame_col = getpixel (explosion_gradient_strip, 0,
									static_cast<int32_t>(value * 200.));
			ellipsefill (flameFront[i], 300, 15, y, y / 20, flame_col);
		}
		if (ROUND(flmDisperse) > 0) {
			ellipsefill (flameFront[i], 300, 15, ROUND(flmDisperse),
							ROUND(flmSize / 16.), PINK);
		}
	}

	topbar                = create_bitmap (env.screenWidth, MENUHEIGHT);
	topbar_gradient_strip = create_gradient_strip (topbar_gradient, 100);

	if (!env.ditherGradients) {
		for (int32_t i = 0; i < MENUHEIGHT; ++i) {
			float adjCount = (100. / MENUHEIGHT) * i;
			int32_t col = getpixel(topbar_gradient_strip, 0, adjCount);
			line (topbar, 0, i, env.screenWidth - 1, i, col);
        }
	} else {
		for (int32_t x = 0; x < env.screenWidth; ++x) {
			for (int32_t y = 0; y < MENUHEIGHT; ++y) {
				float adjY = (100.0 / MENUHEIGHT) * y;
				int offset = 0;

				if ((adjY > 1) && (adjY < 99))
					offset = rand () % 4 - 2;

				int32_t col = getpixel(topbar_gradient_strip, 0, adjY + offset);

				putpixel (topbar, x, y, col);
			}
		}
	}

	stuff_bar[0]    = create_bitmap (STUFF_BAR_WIDTH, STUFF_BAR_HEIGHT);
	stuff_bar[1]    = create_bitmap (STUFF_BAR_WIDTH, STUFF_BAR_HEIGHT);
	stuff_icon_base = create_bitmap (STUFF_BAR_WIDTH/10, STUFF_BAR_HEIGHT);

	clear_to_color (stuff_bar[0], PINK);
	clear_to_color (stuff_bar[1], PINK);
	clear_to_color (stuff_icon_base, PINK);

	stuff_bar_gradient_strip = create_gradient_strip (stuff_bar_gradient, STUFF_BAR_WIDTH);

	double halfStuffBarHeight = (STUFF_BAR_HEIGHT / 2) - 2;

	for (double x = 0; x < STUFF_BAR_WIDTH; x += 1.) {
		for (double y = 0; y < STUFF_BAR_HEIGHT; y += 1.) {
			double sides_dist  = 0.1;
			double circle_dist = FABSDISTANCE2(x, y, STUFF_BAR_WIDTH - 75, halfStuffBarHeight);

			if (circle_dist < 75.)
				circle_dist = 1. - (circle_dist / 75.0);
			else
				circle_dist = 0.;

			if (x < (STUFF_BAR_HEIGHT/2 - 2))
				sides_dist -= 0.1 - (x / 150.);
			else if (x > STUFF_BAR_WIDTH - (STUFF_BAR_HEIGHT/2 - 2))
				sides_dist -= (x - (STUFF_BAR_WIDTH - halfStuffBarHeight)) / 150.;

			if (y < STUFF_BAR_HEIGHT/2 - 2)
				sides_dist -= 0.1 - (y / 150.);
			else
				sides_dist -= (y - halfStuffBarHeight) / 150.;

			sides_dist -= circle_dist * circle_dist;

			if (sides_dist > (x / 1000.0))
				sides_dist = x / 1000.0;
			if (sides_dist < 0)
				sides_dist = 0;
			if (circle_dist > 1)
				circle_dist = 1;

			int32_t offset = (sides_dist + circle_dist) * (STUFF_BAR_WIDTH - 1);
			if (offset >= STUFF_BAR_WIDTH)
				offset  = STUFF_BAR_WIDTH - 1;

			int32_t col_a = getpixel(stuff_bar_gradient_strip,
			                                   0, offset);

			offset = (sides_dist + circle_dist + 0.06) * (STUFF_BAR_WIDTH - 1);
			if (offset >= STUFF_BAR_WIDTH)
				offset  = STUFF_BAR_WIDTH - 1;

			int32_t col_b = getpixel(stuff_bar_gradient_strip,
			                                   0, offset);

			if (x < (STUFF_BAR_WIDTH / 10)) {
				putpixel (stuff_icon_base, x, y, col_a);
			}

			if (y < STUFF_BAR_HEIGHT - 5) {
				putpixel (stuff_bar[0], x, y, col_a);
				putpixel (stuff_bar[1], x, y, col_b);
			}
		}
	}

	initDone = true;
}


// === Helper Functions ===
// ========================
BITMAP *create_gradient_strip (const gradient *grad, int32_t len)
{
	BITMAP *strip = create_bitmap (1, len);
	if (! strip)
		return nullptr;

	clear_to_color (strip, BLACK);

	for (int32_t currLine = 0; currLine < len; ++currLine) {
		int32_t color = gradientColorPoint(grad, len, currLine);
		putpixel (strip, 0, currLine, color);
	}

	return strip;
}

int32_t gradientColorPoint(const gradient* grad, double len, double line)
{
	int32_t       pointCount = 0;
	double        point      = line / len;
	int32_t color      = BLACK;

	for ( ;
		(point >= grad[pointCount].point) && (grad[pointCount].point != -1);
		++pointCount) ;
	pointCount--;

	if (pointCount == -1)
		color = makecol (grad[0].color.r, grad[0].color.g, grad[0].color.b);
	else if (grad[pointCount + 1].point == -1)
		color = makecol(grad[pointCount].color.r,
						grad[pointCount].color.g,
						grad[pointCount].color.b);
	else {
		double i = (point - grad[pointCount].point)
		         / (grad[pointCount + 1].point - grad[pointCount].point);
		int32_t r = ROUND(interpolate (grad[pointCount].color.r,
									   grad[pointCount + 1].color.r, i));
		int32_t g = ROUND(interpolate (grad[pointCount].color.g,
									   grad[pointCount + 1].color.g, i));
		int32_t b = ROUND(interpolate (grad[pointCount].color.b,
									   grad[pointCount + 1].color.b, i));

		color = makecol (r, g, b);
	}

	return color;
}

