#include "optionitemcolour.h"


/** @brief Default constructor.
  *
  * The target is the color instance to handle.
  *
  * @param[in,out] player_ Pointer to the PLAYER instance to handle.
  * @param[in,out] action_ Pointer to the action function handling the button click.
  * @param[in] title_ The title of the option to display.
  * @param[in] titleIdx_ Index value of the submitted title. -1 means @a title_ is fixed.
  * @param[in] top_ Top position of the display area.
  * @param[in] left_ Left position of the display area.
  * @param[in] width_ Width of the display area.
  * @param[in] height_ Height of the display area.
  * @param[in] padding_ Padding of the title and buttons to the display area.
**/
OptionItemColour::OptionItemColour(
            int32_t*      color_,
            const char*   title_,
            int32_t       titleIdx_,
            int32_t top_, int32_t left_, int32_t width_, int32_t height_,
            int32_t padding_, int32_t show_size_) :
	OptionItemBase(ET_COLOR,
	               title_,
	               titleIdx_,
	               nullptr,
	               color_ ? *color_: WHITE, // WHITE to indicate an error.
	               TC_NONE, nullptr,
	               top_, left_, width_, height_, padding_, show_size_)
{
	// For ET_COLOR, only the color_ is needed
	assert (color_ && "ERROR: color_ must be set");
	tgt_color = color_;

	// Create the rainbow box:
	tgt_bitmap = create_bitmap(width, height);

	assert((width  <= tgt_bitmap->w)  && "Width too small");
	assert((height <= tgt_bitmap->h) && "Height too small");

	solid_mode();
	clear_to_color (tgt_bitmap, BLACK);
	int32_t r, g, b;

	for (int32_t x = 0.; x <= width; ++x) {
		for (int32_t y = 0.; y <= height; ++y) {
			double h = static_cast<double>(x) / static_cast<double>(width) * 360.;
			double p = static_cast<double>(y) / static_cast<double>(height);
			double v = 1.;
			double s = p * 2.;

			if (p > 0.5) {
				v = 1. - ((p - 0.5) * 2.);
				s = 1.;
			}

			assert ( (h >= 0.) && (h <= 360.) && "h out of range");
			assert ( (s >= 0.) && (s <=   1.) && "s out of range");
			assert ( (v >= 0.) && (v <=   1.) && "v out of range");

			hsv_to_rgb(h, s, v, &r, &g, &b);

			putpixel(tgt_bitmap, x + 1, y + 1, makecol(r, g, b));
		}
	}
}


/// @brief default dtor only setting nullptr values. No further action needed.
OptionItemColour::~OptionItemColour()
{
	destroy_bitmap(tgt_bitmap);
	tgt_bitmap = nullptr;
	tgt_color  = nullptr;

}


/* ----------------------
 * --- Public methods ---
 * ----------------------
 */

/** @brief pick a new color from @a x / @a y and store it in @a target
  *
  * @param[in] x The x coordinate of the pixel to pick the colour from.
  * @param[in] y The y coordinate of the pixel to pick the colour from.
  */
int32_t OptionItemColour::activate(int32_t, int32_t x, int32_t y, int32_t)
{
	int32_t pick_x = x - left;
	int32_t pick_y = y - top;
	if ( (pick_x >= 1)
	  && (pick_y >= 1)
	  && (pick_x <= (width  - 1))
	  && (pick_y <= (height - 1)) ) {

		// Note down where to draw the cross
		act_x = pick_x;
		act_y = pick_y;

		// To show both, do a redraw
		clear_display(false);
		display(false);

		// Get fresh pixel
		*tgt_color = getpixel(tgt_bitmap, pick_x, pick_y);

		// Draw helper cross:
		displayCross();

		// Redraw decoration:
		this->displayDeco(*tgt_color);
	} else {
		// No cross
		act_x = 0;
		act_y = 0;
	}

	return 0;
}


/// @brief returns always true
bool OptionItemColour::canGoDown()
{
	return true;
}


/// @brief returns always true
bool OptionItemColour::canGoUp()
{
	return true;
}


/** @brief Display a color chooser for @a target and the display box.
  *
  * The display area is where to draw the "rainbow box". The box in which
  * to draw the currently selected color is a "deco", but not drawn by
  * display_deco(), because it would not have access to the target.
  *
  * @param[in] show_full decorations are drawn if this is set to true
  */
void OptionItemColour::display(bool show_full)
{
	if (!drawn) {

		// Here is the chooser, the display box is "deco"
		blit(tgt_bitmap, global.canvas, 0, 0, left, top, width, height);
		rect (global.canvas, left, top, left + width, top + height, BLACK);

		// Draw helper cross
		displayCross();

		drawn = true;
	}

	if (show_full)
		this->displayDeco(*tgt_color);
}


/// @brief Draw the selector cross
void OptionItemColour::displayCross()
{
	if ( (act_x > 0) && (act_y > 0) ) {
		// To make the color picking easier, draw a cross
		// to show where the user clicked in
		int32_t right  = left + width;
		int32_t bottom = top  + height;
		int32_t x      = left + act_x;
		int32_t y      = top  + act_y;
		int32_t xl     = x - 5;
		int32_t xr     = x + 5;
		int32_t yt     = y - 5;
		int32_t yb     = y + 5;

		// Do not overdraw:
		if (xl <= left  ) xl = x > left   ? left   + 1 : 0;
		if (yt <= top   ) yt = y > top    ? top    + 1 : 0;
		if (xr >= right ) xr = x < right  ? right  - 1 : 0;
		if (yb >= bottom) yb = y < bottom ? bottom - 1 : 0;

		// If a coordinate is zero, the line is not drawn.
		int32_t cross_col = makecol(255 - getr(*tgt_color),
		                            255 - getg(*tgt_color),
		                            255 - getb(*tgt_color) );
		if (xl) hline (global.canvas, xl,    y,     x - 1, cross_col);
		if (xr) hline (global.canvas, x + 1, y,     xr,    cross_col);
		if (yt) vline (global.canvas, x,     yt,    y - 1, cross_col);
		if (yb) vline (global.canvas, x,     y + 1, yb,    cross_col);
	}
}


/// @brief return true, the action function must be able to return an exit code.
bool OptionItemColour::isExitButton()
{
	return true;
}
