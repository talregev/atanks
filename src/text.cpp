#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "text.h"


// Basic constructor to kick things off
TEXTBLOCK::TEXTBLOCK()
{
    complete_text = NULL;
    total_lines = current_line = 0;
}

// A constructor that also loads text from a file
TEXTBLOCK::TEXTBLOCK(char *filename)
{
    complete_text = NULL;
    total_lines = current_line = 0;
    if (filename)
    {
        int status;
        status = Load_File(filename);
        if (!status)
            printf("Something went wrong loading text from file: %s\n", filename);
    }
}

// clean everything
TEXTBLOCK::~TEXTBLOCK()
{
    if (complete_text)
    {
        for (int count = 0; count < total_lines; count++)
        {
            if (complete_text[count])
                free(complete_text[count]);
        }
        free(complete_text);
    }
}

void TEXTBLOCK::Trim_Newline(char *line)
{
    int index = 0;

    while (line[index])
    {
        if ((line[index] == '\n') || (line[index] == '\r'))
            line[index] = '\0';
        else
            index++;
    }
}

// This function does most of the work. It loads an entire text
// file into memory. Returns TRUE on success or FALSE if
// somethign goes wrong.
int TEXTBLOCK::Load_File(char *filename)
{
    FILE *phile;
    int lines_loaded = 0;
    int we_have_space = 10;
    char line[MAX_LINE_LENGTH], *status;

    // open the file
    phile = fopen(filename, "r");
    if (!phile)
        return FALSE;

    // give us some space
    complete_text = (char **) calloc(we_have_space, sizeof(char *));
    if (!complete_text)
    {
        fclose(phile);
        return FALSE;
    }

    // time to load some text!
    status = fgets(line, MAX_LINE_LENGTH, phile);
    while (status && (lines_loaded < MAX_LINES_IN_FILE) && complete_text)
    {
        Trim_Newline(line);
        complete_text[lines_loaded] = (char *) calloc(strlen(line) + 1, sizeof(char));
        if (complete_text[lines_loaded])
        {
            strcpy(complete_text[lines_loaded], line);
            lines_loaded++;
        }
        if (lines_loaded >= we_have_space)
        {
            we_have_space += 10;
            complete_text = (char **) realloc(complete_text, we_have_space * sizeof(char *));
        }
        status = fgets(line, MAX_LINE_LENGTH, phile);
    }       // all done loading

    if (complete_text)
        complete_text = (char **) realloc(complete_text, lines_loaded * sizeof(char *));
    fclose(phile);
    total_lines = lines_loaded;
    current_line = 0;
    return TRUE;
}

// Find a random line and return it
char *TEXTBLOCK::Get_Random_Line()
{
    int my_line;
    char *my_text;

    my_line = rand() % total_lines;
    my_text = complete_text[my_line];
    return my_text;
}

// Returns the current line
char *TEXTBLOCK::Get_Current_Line()
{
    char *my_text;

    my_text = complete_text[current_line];
    return my_text;
}

// Move the line counter ahead, if possible
// Returns FALSE if we hit the end of lines, or
// true if everything is OK
int TEXTBLOCK::Next_Line()
{
    current_line++;
    if (current_line >= total_lines)
    {
        current_line = total_lines - 1;
        return FALSE;
    }
    return TRUE;
}

// Go to the previous line. The function returns
// TRUE is everything is OK. If we were already
// at the beginning, then FALSE is returned.
int TEXTBLOCK::Previous_Line()
{
    if (current_line > 0)
    {
        current_line--;
        return TRUE;
    }
    return FALSE;
}

// This function display all lines of text. Optionally, it
// will also print line numbers before each line.
// The number of lines printed is returned.
int TEXTBLOCK::Display_All(int line_numbers)
{
    int counter = 0;

    while (counter < total_lines)
    {
        if (line_numbers)
            printf("%d. ", counter);
        printf("%s\n", complete_text[counter]);
        counter++;
    }
    return counter;
}

// This is a free floating function
char *Add_Comma(int number)
{
    char buffer[64];
    static char return_value[128];
    int index, return_index;
    int place_counter = 0;

    // we won't worry about over-flow with such a big buffer
    sprintf(buffer, "%d", number);
    memset(return_value, '\0', 128);
    index = strlen(buffer);    // start from the end
    return_index = index + (index / 3);
    return_index -= 1;
    if (!(index % 3))
        return_index--;
    index -= 1;
    while (index >= 0)
    {
        return_value[return_index] = buffer[index];
        return_index--;
        index--;
        place_counter++;
        if ((place_counter == 3) && (return_index > 0))
        {
            place_counter = 0;
            return_value[return_index] = ',';
            return_index--;
        }
    }
    return return_value;
}
