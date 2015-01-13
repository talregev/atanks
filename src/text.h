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

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define MAX_LINE_LENGTH 512
#define MAX_LINES_IN_FILE 1024


class TEXTBLOCK
{
public:
    int total_lines, current_line;
    char **complete_text;


    TEXTBLOCK();
    TEXTBLOCK(char *filename);
    ~TEXTBLOCK();

    void Trim_Newline(char *line);     // hack the newline off a string
    int Load_File(char *filename);    // load lines from a file
    char *Get_Random_Line();   // give us a random line
    char *Get_Current_Line();  // return current line
    int Next_Line();           // advance the current line counter one
    int Previous_Line();       // move the current line counter back one
    int Display_All(int line_numbers); // display all text

};

// Functions we can use anywhere

// This function returns a string with
// comma characters between thousands positions
// There is no need to free the returned string.
char *Add_Comma(int number);

#endif
