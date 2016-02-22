#include "floattext.h"

FLOATTEXT::FLOATTEXT (const char* text_, int32_t xpos, int32_t ypos,
                      double xv_, double yv_, int32_t color_,
                      alignType alignment, eTextSway sway_type,
                      int32_t max_age) :
	VIRTUAL_OBJECT(),
	color(color_),
	pos_x(xpos),
	pos_y(ypos)
{
	int32_t sky_col = TURQUOISE;
	if ( (pos_x > -1)              && (pos_y > MENUHEIGHT)
	  && (pos_x < env.screenWidth) && (pos_y < env.screenWidth) ) {
		sky_col = getpixel(env.sky, pos_x, pos_y - MENUHEIGHT);
	}

	halfColor = GetShadeColor(color, true, sky_col);
	align     = alignment;
	maxAge    = max_age;

	if (text_)
		set_text(text_);

	// The font and thus its height is fixed:
	dim_cur.h = env.fontHeight + (env.fontHeight % 2);

	x         = xpos;
	y         = ypos;
	dim_cur.x = ROUND(pos_x);
	dim_cur.y = ROUND(pos_y);
	set_sway_type(sway_type);
	set_speed(xv_, yv_);

	// Add to the chain:
	global.addObject(this);
}


FLOATTEXT::~FLOATTEXT()
{
	requireUpdate ();
	this->update();

	// Only do the final update if the dimensions have been set
	if ( dim_cur.w > 0 ) {
		// Update current position
		int32_t left = LEFT  == align ? dim_cur.x
					 : RIGHT == align ? dim_cur.x -  dim_cur.w
					 :                  dim_cur.x - (dim_cur.w / 2);
		int32_t top  = LEFT  == align ? dim_cur.y
					 : RIGHT == align ? dim_cur.y -  dim_cur.h
					 :                  dim_cur.y - (dim_cur.h / 2);
		int32_t right  = std::min(env.screenWidth, left + dim_cur.w + 1);
		int32_t bottom = std::min(env.screenHeight, top + dim_cur.h + 1);

		global.make_bgupdate(left, top, right - left, bottom - top);

		// Update previous position
		left = LEFT  == align ? dim_old.x
			 : RIGHT == align ? dim_old.x -  dim_old.w
			 :                  dim_old.x - (dim_old.w / 2);
		top  = LEFT  == align ? dim_old.y
			 : RIGHT == align ? dim_old.y -  dim_old.h
			 :                  dim_old.y - (dim_old.h / 2);
		right  = std::min(env.screenWidth, left + dim_old.w + 1);
		bottom = std::min(env.screenHeight, top + dim_old.h + 1);

		global.make_bgupdate(left, top, right - left, bottom - top);
	}

	// Free allocated text
	if (text) {
		free(text);
		text = nullptr;
	}

	// Take out of the chain:
	global.removeObject(this);
}


void FLOATTEXT::applyPhysics()
{
	// Opt out early if there is no text to be drawn.
	if ( (nullptr == text) || (dim_cur.w < 1) )
		return;

	if (TS_HORIZONTAL == sway) {
		double x_dist = pos_x - x;
		double rel_xv = static_cast<double>(sway - std::abs(x_dist))
		              / static_cast<double>(sway) * SIGNd(x_dist); // [-1:+1]
		if (std::abs(rel_xv) < 0.15)
			// Only reverse to not stop it.
			xv *= -1.;
		else if (SIGN(xv) == SIGN(rel_xv))
			xv = rel_xv;
		else
			xv = -1. * rel_xv;
	} else if (TS_VERTICAL == sway) {
		double y_dist = pos_y - y;
		double rel_yv = static_cast<double>(sway - std::abs(y_dist))
		              / static_cast<double>(sway) * SIGNd(y_dist); // [-1:+1]
		if (std::abs(rel_yv) < 0.33)
			// Only reverse to not stop it.
			yv *= -1.;
		else if (SIGN(yv) == SIGN(rel_yv))
			yv = rel_yv;
		else
			yv = -1. * rel_yv;
	}
	pos_x += xv;
	pos_y += yv;

	dim_cur.x = ROUND(pos_x);
	dim_cur.y = ROUND(pos_y);

	requireUpdate();

	if ( (maxAge != -1) && (++age > maxAge) )
		destroy = true;
}


/// Little Helper, move it somewhere else if it makes sense to be used elsewhere
#define SAFE_MAKECOL(r_, g_, b_) makecol( \
	r_ < 0 ? 0 : r_ > 255 ? 255 : r_, \
	g_ < 0 ? 0 : g_ > 255 ? 255 : g_, \
	b_ < 0 ? 0 : b_ > 255 ? 255 : b_)


void FLOATTEXT::draw()
{
	// Opt out early if there is no text to be drawn.
	if ( (nullptr == text) || !dim_cur.w)
		return;

	// If the width is not known, yet, determine it
	if (1 > dim_cur.w) {
		dim_cur.w = text_length(font, text) + 3;
		dim_cur.w += dim_cur.w % 2;
	}

	// Current position according to alignment:
	int32_t left   = LEFT  == align ? dim_cur.x
	               : RIGHT == align ? dim_cur.x -  dim_cur.w
	               :                  dim_cur.x - (dim_cur.w / 2);
	int32_t top    = LEFT  == align ? dim_cur.y
	               : RIGHT == align ? dim_cur.y -  dim_cur.h
	               :                  dim_cur.y - (dim_cur.h / 2);

	double  shadeFade = 0.75;
	int32_t frontCol  = color;
	int32_t shadeCol  = halfColor;
	int32_t backCol   = color;

	// If either shadowed or fading text is enabled, a background
	// average colour is needed.
	if ( (env.shadowedText || env.fadingText)
	  && !global.skippingComputerPlay) {
		backCol = global.get_avg_bgcolor(left, top, left + dim_cur.w,
		                                 top + dim_cur.h, xv, yv);

		// If fading text is activated, the front colour must be calculated as well
		if ( env.fadingText
		  && (maxAge > 0)
		  && (age >= (maxAge / 2)) ) {
			double calcMax   = maxAge / 2;
			double calcAge   = age - calcMax;
			double frontFade = 1.0 - (calcAge / calcMax);

			shadeFade -= 0.75 * (calcAge / calcMax);

			if (frontFade < 0.) frontFade = 0.;
			if (shadeFade < 0.) shadeFade = 0.;

			double backFade  = 1.0 - frontFade;
			if (backFade < 0.) backFade = 0.;

			int32_t r = ROUND( (getr(frontCol) * frontFade) + (getr(backCol) * backFade) );
			int32_t g = ROUND( (getg(frontCol) * frontFade) + (getg(backCol) * backFade) );
			int32_t b = ROUND( (getb(frontCol) * frontFade) + (getb(backCol) * backFade) );
			frontCol = SAFE_MAKECOL(r, g, b);
		} // end of calculating fading values

		// The now current values must be applied to the shadow colour if needed
		if (env.shadowedText) {
			double backFade = 1.0 - shadeFade;

			if (backFade < 0.) backFade = 0.;

			int32_t r = ROUND( (getr(shadeCol) * shadeFade) + (getr(backCol) * backFade) );
			int32_t g = ROUND( (getg(shadeCol) * shadeFade) + (getg(backCol) * backFade) );
			int32_t b = ROUND( (getb(shadeCol) * shadeFade) + (getb(backCol) * backFade) );

			shadeCol = SAFE_MAKECOL(r, g, b);
		} // End of calculating shadow values
	} // End of fading / shadow preparations

	// Eventually print out the text:
	if (env.shadowedText && !global.skippingComputerPlay)
		textout_ex (global.canvas, font, text, left + 1, top + 1, shadeCol, -1);
	textout_ex (global.canvas, font, text, left, top, frontCol, -1);
}


void FLOATTEXT::newRound()
{
	if (maxAge > 0)
		age = maxAge + 1;
}


// Reset movement to begin neutrally if the text is swaying
void FLOATTEXT::reset_sway()
{
	xv    = 0.;
	yv    = 0.;
	pos_x = x;
	pos_y = y;
	if (TS_HORIZONTAL == sway) {
		pos_x = x + ((1 + (rand() % (sway / 2))) * (rand() % 2 ? -1 : 1));
		xv    = SIGNd(pos_x - x);
	} else if (TS_VERTICAL == sway) {
		pos_y = y;
		yv    = -1.;
	}
	dim_cur.x = pos_x;
	dim_cur.y = pos_y;
}


void FLOATTEXT::set_color(int32_t color_)
{
	if (color != color_)
		color     = color_;

	int32_t left   = LEFT  == align ? dim_cur.x + (dim_cur.w / 2)
	               :                  dim_cur.x - (dim_cur.w / 2);
	int32_t top    = LEFT  == align ? dim_cur.y + (dim_cur.h / 2)
	               :                  dim_cur.y - (dim_cur.h / 2);

	int32_t sky_col = TURQUOISE;
	if ( (left > -1)              && (top > MENUHEIGHT)
	  && (left < env.screenWidth) && (top < env.screenWidth) ) {
		sky_col = getpixel(env.sky, left, top - MENUHEIGHT);
	}

	halfColor = GetShadeColor(color, true, sky_col);
}


void FLOATTEXT::set_pos(int32_t xpos, int32_t ypos)
{
	if ( (xpos != x) || (ypos !=y) ) {
		x     = xpos;
		y     = ypos;
		reset_sway();
	}
}


void FLOATTEXT::set_speed(double xv_, double yv_)
{
	reset_sway();

	if (TS_HORIZONTAL != sway)
		xv = xv_;

	if (TS_VERTICAL != sway) {
		if (yv_ < 0.) {
			// avoid over-lapping text
			double mix_it_up = ((rand() % 6) - 3.) / 10.; // [-.3;+.2]

			yv = yv_ + mix_it_up;

			// avoid text that does not move up
			if (yv > -0.1)
				yv = -0.1;
		} else
			yv = yv_;
	}
}


void FLOATTEXT::set_sway_type(eTextSway sway_type)
{
	if (sway_type != sway) {
		sway = sway_type;
		reset_sway();
	}
}


void FLOATTEXT::set_text(const char* text_)
{
	if (text && text_ && !strcmp(text, text_))
		return;

	size_t new_len = text_ ? strlen(text_)  : 0;
	size_t old_len = text  ? strlen(text)   : 0;

	// clean up old text
	if (text && old_len)
		memset(text, 0, old_len * sizeof(char));

	// reallocate new text
	if (!text || (new_len > old_len)) {
		char* new_text = (char*)realloc(text, (new_len + 1) * sizeof(char));
		if (new_text) {
			text = new_text;
			memset(text, 0, (new_len + 1) * sizeof(char));
		} else {
			cerr << "Unable to allocate " << ( (new_len + 1) * sizeof(char));
			cerr << " bytes for new text" << endl;
			return;
		}
	}

	// Copy new text if any. If not, _text is { 0x0 } already.
	if (new_len) {
		strncpy(text, text_, new_len);
		dim_cur.w = -1; // draw() must determine width
	} else
		// Width must be reset
		dim_cur.w = 0;
}


/// @param[in] do_lighten If true, the colour is made brighter if the shade colour
///            would be too dark to make a difference.
/// @param[in] bg_colour If not PINK, the background colour is taken into account
///            and the result darkened or lightened more according to @a do_lighten
int32_t GetShadeColor(int32_t colour, bool do_lighten, int32_t bg_colour)
{
	int32_t r = getr(colour), g = getg(colour), b = getb(colour);
	float   h, s, v;

	if (do_lighten) {
		// Be sure something can be done with near black colours
		if ((r < 0x20) && (g < 0x20) && (b < 0x20)) {
			r = 0x28;
			g = 0x28;
			b = 0x28;
		}
	}

	rgb_to_hsv(r, g, b, &h, &s, &v);

	if (do_lighten) {
		if (s < 0.10) s = 0.10;
		if (v < 0.25) v = 0.25;
	}

	int32_t rn, gn, bn;
	double s_mod = s > 0.5 ? (s > 0.9 ? 0.33 : 0.5) : (do_lighten ? 1.75 : .75);
	double v_mod = v > 0.5 ? (v > 0.9 ? 0.33 : 0.5) : (do_lighten ? 1.75 : .75);
	hsv_to_rgb(h, s * s_mod * (v > 0.75 ? 0.5 : 1.),
				        v * v_mod * (s > 0.75 ? 0.5 : 1.),
				        &rn, &gn, &bn);

	// check to see if this all fits with a possible background
	if (bg_colour != PINK) {
		int32_t rb = getr(bg_colour), gb = getg(bg_colour), bb = getb(bg_colour);
		int32_t rm = (rn + r) / 2,    gm = (gn + g) / 2,    bm = (bn + b) / 2;

		if (ABSDISTANCE3(rb, gb, bb, rm, gm, bm) < 0x20) {

			// The middle between the colour and its shade is too near
			// to the background. Here s_mod/v_mod can be reused:
			rn = std::round(  static_cast<double>(rn)
			                * (rn > r ? std::max(s_mod, v_mod)
			                          : std::min(s_mod, v_mod)));
			gn = std::round(  static_cast<double>(gn)
			                * (gn > g ? std::max(s_mod, v_mod)
			                          : std::min(s_mod, v_mod)));
			bn = std::round(  static_cast<double>(bn)
			                * (bn > b ? std::max(s_mod, v_mod)
			                          : std::min(s_mod, v_mod)));
			// Do not overflow:
			if (rn > 255) rn = 255;
			if (gn > 255) gn = 255;
			if (bn > 255) bn = 255;
		}
	}

	return makecol(rn, gn, bn);
}

