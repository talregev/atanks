#ifndef TEXT_HEADER_FILE__
#define TEXT_HEADER_FILE__

/* This file contains functions for reading text from files and
 * storing it in the game. The entire text file will be kept in memory.
 * Each text object will have the ability to return a line,
 * track its position in the text or return a random line.
 * This should allow for better multi-language handling, faster
 * access to tank speech and remove the need to rely on the lineseq
 * class.
 * -- Jesse
*/

#include <cstdint>

#define MAX_LINE_LENGTH 512
#define MAX_LINES_IN_FILE 1024

struct BOX;

/// @brief alignment of texts
enum alignType {
	CENTRE = 0,
	LEFT,
	RIGHT
};


class TEXTBLOCK
{
public:

	/* -----------------------------------
	 * --- Constructors and destructor ---
	 * -----------------------------------
	 */

	TEXTBLOCK ();
	TEXTBLOCK (const char* filename);
	~TEXTBLOCK();


	/* ----------------------
	 * --- Public methods ---
	 * ----------------------
	 */

	int32_t     Display_All     (bool show_line_numbers); // display all text
	const char* Get_Current_Line() const;                 // return current line
	const char* Get_Line        (int32_t index) const;    // return a specific line
	const char* Get_Random_Line () const;                 // give us a random line
	bool        Load_File       (const char *filename);   // load lines from a file
	int32_t     Lines           () const;                 // Return number of total lines
	bool        Next_Line       ();                       // advance the current line
	bool        Previous_Line   ();                       // move the current line back
	void        Render_Lines    (int32_t scrollOffset,
	                             int32_t spacing,
	                             int32_t top,
	                             int32_t bottom);         // Render to global.canvas


private:

	/* -----------------------
	 * --- Private methods ---
	 * -----------------------
	 */

	void destroy();


	/* -----------------------
	 * --- Private members ---
	 * -----------------------
	 */

	int32_t total_lines   = 0;
	int32_t current_line  = 0;
	char**  complete_text = nullptr;
};




// Functions we can use anywhere

// This function returns a string with
// comma characters between thousands positions
// You *MUST* *NOT* free the returned string.
const char* Add_Comma(int32_t number);

void draw_text_in_box(BOX* region, const char* text, bool with_box);

// hack the newline off a string
void Trim_Newline(char *line);

#endif

