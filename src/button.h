#ifndef BUTTON_HEADER_
#define BUTTON_HEADER_

#include "main.h"

class BUTTON
{
public:

	/* --------------------
	 * --- constructors ---
	 * --------------------
	 */

	// Minimum ctor without text
	explicit
	BUTTON (int32_t left_, int32_t top_,
	        BITMAP* bmp_, BITMAP* hover_, BITMAP* depressed_);

	// ctor for using a bitmap.
	BUTTON (const char* text_, bool text_only_,
	        int32_t left_, int32_t top_,
	        BITMAP* bmp_, BITMAP* hover_, BITMAP* depressed_);

	// ctor for drawing a manual box.
	BUTTON (const char* text_, bool text_only_,
	        int32_t left_, int32_t top_, int32_t width_, int32_t height_);


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	void draw ();
	void getLocation(int32_t &x,int32_t &y,int32_t &w,int32_t &h);
	bool isMouseOver ();
	bool isPressed ();
	void setText(const char* text_);

private:

	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	BITMAP*     bmp        = nullptr;
	BITMAP*     depressed  = nullptr;
	BITMAP*     hover      = nullptr;
	BOX         location;               //!< is {0, 0, 0, 0} by default
	const char* text       = nullptr;
	bool        text_only  = false;     //!< If set to true, only the title is displayed.
	int32_t     text_width = 0;         //!< must not be recalculated over and over again...
	int32_t     x1, y1, x2, y2, x3, y3; //!< Shortcuts, as those stay fixed.
};

#endif

