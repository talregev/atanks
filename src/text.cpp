#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <cassert>
#include "text.h"
#include "main.h"


// Basic constructor to kick things off
TEXTBLOCK::TEXTBLOCK()
{ /* nothing to do here */ }


// A constructor that also loads text from a file
TEXTBLOCK::TEXTBLOCK(const char* filename)
{
	if (filename && !Load_File(filename)) {
		cerr << "Something went wrong loading text from file:";
		cerr << filename << " !" << endl;
	}
}


// clean everything
TEXTBLOCK::~TEXTBLOCK()
{
	destroy();
}


/// @brief unified method to delete the present text
void TEXTBLOCK::destroy()
{
	if (complete_text) {
		for (int32_t i = 0; i < total_lines; ++i) {
			if ( complete_text[i] )
				free(complete_text[i]);
		}
		free(complete_text);
		complete_text = nullptr;
	}
}


// This function display all lines of text. Optionally, it
// will also print line numbers before each line.
// The number of lines printed is returned.
int32_t TEXTBLOCK::Display_All(bool show_line_numbers)
{
	int32_t i = 0;
	for ( ; i < total_lines; ++i) {
		if (show_line_numbers)
			printf("%d. ", i);
		printf("%s\n", complete_text[i]);
    }
	return i;
}


/// @brief Draw @a text in the box @a region with border and blue background
/// if @a with_box is true.
/// This method releases the display and can therefore be used in parallel.
void draw_text_in_box (BOX* region, const char* text, bool with_box)
{
	if (with_box) {
		global.lockLand();
		rectfill (global.canvas, region->x, region->y, region->w, region->h,
				  makecol (0,0,128));
		rect     (global.canvas, region->x, region->y, region->w, region->h,
				  makecol (128,128,255));
		global.unlockLand();
	}

	char     buffer[1024] = { 0 };
	uint32_t lastSpace    = 0;
	uint32_t lineBegin    = 0;
	uint32_t lineCount    = 0;
	int32_t  lineWidth    = region->w - 27;
	uint32_t textLength   = static_cast<uint32_t>(strlen(text));

	while (lineBegin < textLength) {
		uint32_t charCount = 0;
		uint32_t buffCount = 0;
		int32_t  buffWidth = 0;
		memset(buffer, 0, sizeof(char) * 1024);

		// Fill buffer until a line break is found or the maximum width
		// is exceeded. For the latter case remember the last space found.
		do {
			buffer[buffCount] = text[lineBegin + charCount];

			if (buffer[buffCount] == ' ')
				lastSpace = 0;
			else
				++lastSpace;

			if (buffer[buffCount] != '\n')
				buffCount++;

			charCount++;

			buffWidth = text_length(font, buffer);
		} while ( text[lineBegin + charCount]
		       && (buffWidth < lineWidth)
		       && (buffer[buffCount] != '\n') );

		// If the line width got exceeded, revert to last space seen
		if (lastSpace && (buffWidth >= lineWidth) ) {
			charCount -= lastSpace;
			buffer[buffCount - lastSpace - 1] = 0;
		} else
			buffer[buffCount] = 0;

		// Print out the result:
		if (buffer[0]) {
			textout_ex (global.canvas, font, buffer, region->x + 5,
						region->y + (lineCount * env.fontHeight) + 5, WHITE, -1);
		}
		lineBegin += charCount;
		lineCount++;
	}

	global.make_update (region->x, region->y, region->w, region->h);
	fi = 1;
}


// Returns the current line
const char* TEXTBLOCK::Get_Current_Line() const
{
	return complete_text[current_line];
}


/// @brief Return a specific line or nullptr if @a index is out of bounds
const char* TEXTBLOCK::Get_Line(int32_t index) const
{
	if ( (index > 0) && (index < total_lines))
		return complete_text[index];
	return nullptr;
}


// Find a random line and return it
const char* TEXTBLOCK::Get_Random_Line() const
{
	return complete_text[rand() % total_lines];
}


// This function does most of the work. It loads an entire text
// file into memory. Returns true on success or false if
// something goes wrong.
bool TEXTBLOCK::Load_File(const char* filename)
{
	char    line[MAX_LINE_LENGTH] = { 0 };
	int32_t lines_loaded  = 0;
	int32_t we_have_space = 10;

	// open the file
	FILE* fIn = fopen(filename, "r");
	if (!fIn)
		return false;

	// give us some space
	destroy();
	complete_text = (char**)calloc(we_have_space, sizeof(char*));
	if (!complete_text) {
		fclose(fIn);
		return false;
	}

    // time to load some text!
	while ( fgets(line, MAX_LINE_LENGTH, fIn)
	    && (lines_loaded < MAX_LINES_IN_FILE)
	    && complete_text) {

		// Reserve more space if full
		if (lines_loaded == we_have_space) {
			we_have_space += 10;

			char** new_text = (char**)realloc(complete_text, we_have_space * sizeof(char*));

			if (new_text)
				complete_text = new_text;
			else {
				destroy();
				fclose(fIn);
				return false;
			}
		}

		// Store the line:
		Trim_Newline(line);
		complete_text[lines_loaded] = (char*)calloc(strlen(line) + 1, sizeof(char));

		if (complete_text[lines_loaded])
			strncpy(complete_text[lines_loaded++], line, strlen(line));
	} // End of loading text from a file

	fclose(fIn);
	total_lines = lines_loaded;
	current_line = 0;

	return true;
}


int32_t TEXTBLOCK::Lines() const
{
	return total_lines;
}


// Move the line counter ahead, if possible
// Returns false if we hit the end of lines, or
// true if everything is OK
bool TEXTBLOCK::Next_Line()
{
	if (++current_line >= total_lines) {
		current_line = total_lines - 1;
		return false;
	}
	return true;
}


// Go to the previous line. The function returns
// true is everything is OK. If we were already
// at the beginning, then false is returned.
bool TEXTBLOCK::Previous_Line()
{
    if (--current_line < 0) {
		current_line = 0;
		return false;
    }
    return true;
}


// This method renders a part of the text to global.canvas
void TEXTBLOCK::Render_Lines(int32_t scrollOffset, int32_t spacing,
                             int32_t top, int32_t bottom)
{
	int32_t txtheight = env.fontHeight * spacing ;
	int32_t xPos      = env.halfWidth;
	int32_t yPos      = env.halfHeight + scrollOffset;
	for ( int32_t i = 0
	    ; (i < total_lines) && (yPos < env.screenHeight)
	    ; ++i) {

	    if ( (yPos > (top - txtheight)) && (yPos < (bottom + txtheight)) ) {
			textout_centre_ex (global.canvas, font, complete_text[i],
			                   xPos + 2, yPos + 2, BLACK, -1);
			textout_centre_ex (global.canvas, font, complete_text[i],
			                   xPos,     yPos,     WHITE, -1);
	    }
		yPos += txtheight;
	}
}


// This is a free floating function
const char* Add_Comma(int32_t number)
{
	static char return_value[128] = { 0 };
	memset(return_value, 0, 128);

	// MAX_MONEY_IN_WALLET is: 1,000,000,000 = 13 characters
	char buffer[15] = { 0 };
	snprintf(buffer, 14, "%d", number);

	int32_t index       = static_cast<int32_t>(strlen(buffer)); // start from the end
	int32_t returnindex = index + (index / 3) - 1;
	int32_t th_count    = 0;

	if (0 == (index % 3) )
		returnindex--;

	while ( (index-- > 0) && (returnindex >= 0) ) {
		return_value[returnindex--] = buffer[index];
		th_count++;

		if ( (th_count == 3) && (returnindex > 0) ) {
			th_count = 0;
			return_value[returnindex--] = ',';
		}
	}

	assert((index < 1) && "ERROR: index is not smaller than 1!");
	assert((returnindex < 1) && "ERROR: returnindex is not smaller than 1!");

	return return_value;
}


void Trim_Newline(char *line)
{
	int32_t index = 0;

	while ( line[index] ) {
		if ( (line[index] == '\n') || (line[index] == '\r') )
			line[index] = '\0';
		else
			++index;
	}
}

