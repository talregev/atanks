/*
This file contains funcions for the BUTTON class. These
are being moved out of the atanks.cc file.
-- Jesse

Updated to be more variable and self managing
-- Sven
*/

#include "button.h"
#include "sound.h"


/** @brief Create a graphical button with no optional title and default sound
  *
  * Note: if @a click_ is nullptr, global_->sounds[8] is used.
  */
BUTTON::BUTTON(int32_t left_, int32_t top_,
               BITMAP* bmp_, BITMAP* hover_, BITMAP* depressed_) :
	bmp      (bmp_       ? bmp_       : hover_ ? hover_ : depressed_ ? depressed_ : nullptr),
	depressed(depressed_ ? depressed_ : hover_ ? hover_ : bmp_       ? bmp_       : nullptr),
	hover    (hover_     ? hover_     : bmp_   ? bmp_   : depressed_ ? depressed_ : nullptr)
{
	location.x = left_;
	location.y = top_;
	location.w = bmp ? bmp->w  : 0;
	location.h = bmp ? bmp->h : 0;
	x1 = location.x;
	y1 = location.y;
	x2 = location.w + x1;
	y2 = location.h + y1;
	x3 = x1 + (location.w / 2);
	y3 = y1 + (location.h / 2);
}


/** @brief Create a graphical button
  *
  * Note: if @a click_ is nullptr, global_->sounds[8] is used.
  */
BUTTON::BUTTON(const char* text_, bool text_only_,
               int32_t left_, int32_t top_,
               BITMAP* bmp_, BITMAP* hover_, BITMAP* depressed_) :
	BUTTON(left_, top_, bmp_, hover_, depressed_)
{
	text_only  = text_only_;
	text       = text_;
	text_width = text ? text_length(font, text) : 0;
}


/** @brief Create a button with manual drawing
  */
BUTTON::BUTTON(const char* text_, bool text_only_,
               int32_t left_, int32_t top_, int32_t width_, int32_t height_) :
	BUTTON(text_, text_only_, left_, top_,
			nullptr, nullptr, nullptr)
{
	location.w = width_;
	location.h = height_;
	x2 = location.w + x1;
	y2 = location.h + y1;
	x3 = x1 + (location.w / 2);
	y3 = y1 + (location.h / 2);
}




void BUTTON::draw()
{
	bool mouse_over = isMouseOver();
	bool pressed    = isPressed();

	if (!text_only) {
		if (bmp)
			draw_sprite (global.canvas,
				mouse_over ? (pressed ? depressed : hover) : bmp,
				x1, y1);
		else {
			int32_t fg = mouse_over ? (pressed ? SILVER : GREY)   : WHITE;
			int32_t lc = mouse_over ? (pressed ? GREY   : WHITE)  : SILVER;
			int32_t dc = mouse_over ? (pressed ? WHITE  : SILVER) : GREY;

			rect     (global.canvas, x1    , y1    , x2    , y2    , BLACK);
			rectfill (global.canvas, x1 + 1, y1 + 1, x2 - 1, y2 - 1, fg);
			line     (global.canvas, x2 - 2, y2 - 2, x1 + 2, y2 - 2, dc);
			line     (global.canvas, x2 - 3, y2 - 3, x1 + 3, y2 - 3, dc);
			line     (global.canvas, x2 - 2, y2 - 2, x2 - 2, y1 + 2, dc);
			line     (global.canvas, x2 - 3, y2 - 3, x2 - 3, y1 + 3, dc);
			line     (global.canvas, x1 + 2, y1 + 2, x2 - 2, y1 + 2, lc);
			line     (global.canvas, x1 + 3, y1 + 3, x2 - 3, y1 + 3, lc);
			line     (global.canvas, x1 + 2, y1 + 2, x1 + 2, y2 - 2, lc);
			line     (global.canvas, x1 + 3, y1 + 3, x1 + 3, y2 - 3, lc);
			rect     (global.canvas, x1 + 4, y1 + 4, x2 - 4, y2 - 4, BLACK);
		}
	}

	if (text)
		textout_centre_ex (global.canvas, font, text, x3, y3 - (text_height(font) / 2),
							text_only ? BLACK : WHITE, -1);

	global.make_update (x1 - 5, y1 - 5, x2 + 5, y2 + 5);
}


void BUTTON::getLocation(int32_t &x,int32_t &y,int32_t &w,int32_t &h)
{
	x = location.x;
	y = location.y;
	w = location.w;
	h = location.h;
}


bool BUTTON::isMouseOver ()
{
	if ( (mouse_x >= x1)
	  && (mouse_y >= y1)
	  && (mouse_x <= x2)
	  && (mouse_y <= y2) )
		return true;
	return false;
}


bool BUTTON::isPressed()
{
	if ((mouse_b & 3) && isMouseOver ()) {
		play_interface_sound(SND_INTE_BUTTON_CLICK);
		return true;
	}
	return false;
}

void BUTTON::setText(const char* text_)
{
	text = text_;
}
