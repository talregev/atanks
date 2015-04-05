#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

// basically all UNIX-like OSes should use stat
#ifndef WIN32
#include <sys/stat.h>
#endif

#include "player.h"
#include "files.h"
#include "main.h"
#include "text.h"

/*
This function saves the game in progress.
All data is saved in a text file for flexiblity.
Returns TRUE on success and FALSE on failure.
*/
int Save_Game(GLOBALDATA *global, ENVIRONMENT *env)
{
    FILE *game_file = NULL;
    char *file_path = NULL;
    int player_count = 0;
    int count;

    // figure out file name
    file_path = (char *) calloc(strlen(global->configDir) + strlen(global->game_name) + 24, sizeof(char));
    if (!file_path)
        return FALSE;

    sprintf(file_path, "%s/%s.sav", global->configDir, global->game_name);

    game_file = fopen(file_path, "w");
    free(file_path);
    if (!game_file)
        return FALSE;

    // write global data
    fprintf(game_file, "GLOBAL\n");
    fprintf(game_file, "ROUNDS=%f\n", global->rounds);
    fprintf(game_file, "CURRENTROUND=%d\n", global->currentround);
    fprintf(game_file, "CAMPAIGNMODE=%f\n", global->campaign_mode);
    fprintf(game_file, "***\n");

    // write envrionment data
    fprintf(game_file, "ENVIRONMENT\n");
    fprintf(game_file, "***\n");

    // write player data
    fprintf(game_file, "PLAYERS\n");
    while (player_count < global->numPlayers)
    {
        PLAYER *my_player = global->players[player_count];
        int global_player_number = 0;
        while ( strcmp(global->allPlayers[global_player_number]->getName(), my_player->getName()) )
            global_player_number++;
        fprintf(game_file, "PLAYERNUMBER=%d\n", global_player_number);
        fprintf(game_file, "SCORE=%d\n", my_player->score);
        fprintf(game_file, "MONEY=%d\n", my_player->money);
        fprintf(game_file, "TYPE=%f\n", my_player->type);
        fprintf(game_file, "TYPESAVED=%f\n", my_player->type_saved);
        for (count = 0; count < WEAPONS; count++)
            fprintf(game_file, "WEAPON=%d %d\n", count, my_player->nm[count]);
        for (count = 0; count < ITEMS; count++)
            fprintf(game_file, "ITEM=%d %d\n", count, my_player->ni[count]);
        fprintf(game_file, "***\n");
        player_count++;
    }

    fclose(game_file);

    // save the current config
    file_path = (char *) calloc(strlen(global->configDir) + strlen(global->game_name) + 24, sizeof(char));
    if (!file_path)
        return FALSE;
    sprintf(file_path, "%s/%s.txt", global->configDir, global->game_name);
    Save_Game_Settings_Text(global, env, file_path);
    free(file_path);

    return TRUE;
}

/*
This function attempts to load a saved
game.
The function returns TRUE on success and
FALSE is an error occures.
-- Jesse
*/
int Load_Game(GLOBALDATA *global, ENVIRONMENT * /*env*/)
{
    char line[512];
    char *field, *value;
    int index, amount, player_count = 0;
    FILE *my_file;
    char *file_path;
    int stage = NO_STAGE;
    char *got_line;
    int global_player_number;

    // load config settings
/*
    file_path = (char *) calloc(strlen(homedir) + strlen(global->game_name) + 24, sizeof(char));
    if (!file_path)
        return FALSE;
    sprintf(file_path, "%s/.atanks/%s.txt", homedir, global->game_name);
    my_file = fopen(file_path, "r");
    free(file_path);
    if (!my_file)
        return FALSE;
    global->loadFromFile_Text(my_file);
    env->loadFromFile_Text(my_file);
    fclose(my_file);
*/

    file_path = (char *) calloc(strlen(global->configDir) + strlen(global->game_name) + 24, sizeof(char));
    if (!file_path)
        return FALSE;

    sprintf(file_path, "%s/%s.sav", global->configDir, global->game_name);

    my_file = fopen(file_path, "r");
    free(file_path);
    if (!my_file)
        return FALSE;

    // read a line from the file
    got_line = fgets(line, 512, my_file);
    // keep reading until we hit EOF or error
    while ((got_line) && (player_count < MAXPLAYERS))
    {
        // clear end of line
        if (strchr(line, '\n'))
            strchr(line, '\n')[0] = '\0';
        if (strchr(line, '\r'))
            strchr(line, '\r')[0] = '\0';

        // check to see if we found a new stage
        if (!strcasecmp(line, "global") )
            stage = GLOBAL_STAGE;
        else if (!strcasecmp(line, "environment"))
            stage = ENVIRONMENT_STAGE;
        else if (!strcasecmp(line, "players"))
            stage = PLAYER_STAGE;

        // not a new stage, seperate field and value
        else
        {
            value = strchr(line, '=');
            if (value)        // valid line
            {
                field = line;
                value[0] = '\0';   // seperate field and value;
                value++;           // go to first place after =
                // interpret data
                switch (stage)
                {
                case GLOBAL_STAGE:
                    if (!strcasecmp(field, "rounds"))
                        sscanf(value, "%lf", &global->rounds);
                    else if (!strcasecmp(field, "currentround"))
                        sscanf(value, "%d", &global->currentround);
                    else if (!strcasecmp(field, "campaignmode"))
                        sscanf(value, "%lf", &global->campaign_mode);

                    break;
                case ENVIRONMENT_STAGE:

                    break;
                case PLAYER_STAGE:
                    if (!strcasecmp(field, "playernumber"))
                    {
                        sscanf(value, "%d", &global_player_number);
                        global->addPlayer(global->allPlayers[global_player_number]);
                        /*  the loading of the preferences is (unfortunately) disabled and I do not know why.
                          As long as they *are* disabled, PerPlay-Config bots have to get a new config here,
                          or they use the global config, rendering PerPlay-Config useless. */
                        if	( (global->players[player_count]->preftype == PERPLAY_PREF)
                              && (global->players[player_count]->type != HUMAN_PLAYER) )
                            global->players[player_count]->generatePreferences();
                        global->players[player_count]->initialise();
                    }
                    else if (!strcasecmp(field, "score"))
                        sscanf(value, "%d", &(global->players[player_count]->score) );
                    else if (!strcasecmp(field, "money"))
                        sscanf(value, "%d", &(global->players[player_count]->money) );
                    else if (!strcasecmp(field, "type"))
                        sscanf(value, "%lf", &(global->players[player_count]->type) );
                    else if (!strcasecmp(field, "typesaved"))
                        sscanf(value, "%lf", &(global->players[player_count]->type_saved) );
                    else if (!strcasecmp(field, "weapon"))
                    {
                        sscanf(value, "%d %d", &index, &amount);
                        global->players[player_count]->nm[index] = amount;
                    }
                    else if (!strcasecmp(field, "item"))
                    {
                        sscanf(value, "%d %d", &index, &amount);
                        global->players[player_count]->ni[index] = amount;
                    }
                    break;
                }    // end of stage

            }       //     end of valid line
            else if ( (!strcmp(line, "***")) && (stage == PLAYER_STAGE) )
                player_count++;

        }     // end of field value area

        // read next line of file
        got_line = fgets(line, 512, my_file);

    }    // end of reading file
    fclose(my_file);
    return TRUE;
}

/*
Check to see if a saved game exists with the given name.
*/
int Check_For_Saved_Game(GLOBALDATA *global)
{
    FILE *my_file;
    char *path_name;

    path_name = (char *) calloc(strlen(global->configDir) + strlen(global->game_name) + 24, sizeof(char));
    if (!path_name)
        return FALSE;

    sprintf(path_name, "%s/%s.sav", global->configDir, global->game_name);

    my_file = fopen(path_name, "r");
    free(path_name);
    if (my_file)
    {
        fclose(my_file);
        return TRUE;
    }
    else
        return FALSE;

}

/*
This function copies the atanks config file
from the HOME_DIR folder to HOME_DIR/.atanks
If the .atanks folder does not exist, this
function will create it.
Returns TRUE on success and FALSE on error.
*/
int Copy_Config_File(GLOBALDATA *global)
{
    FILE *source_file, *dest_file;
    char source_path[1024], dest_path[1024];
    char buffer[1024];
    char *my_home_folder;
    int status;

    // figure out where home is
    my_home_folder = getenv(HOME_DIR);
    if (!my_home_folder)
        my_home_folder = ".";

    // check to see if the config file has already been copied
    snprintf(source_path, 1024, "%s/atanks-config.txt", global->configDir);
    source_file = fopen(source_path, "r");
    if (source_file)     // config file is in the right place
    {
        fclose(source_file);
        return TRUE;
    }

/*
    // check to make sure we have a source file
    snprintf(source_path, 1024, "%s/.atanks-config.txt", my_home_folder);
    source_file = fopen(source_path, "r");
    if (!source_file)
        return TRUE;
*/

    // file not copied yet, create the required directory
    snprintf(buffer, 1024, "%s/.atanks", my_home_folder);
#ifdef WIN32
    status = mkdir(buffer);
#else
    status = mkdir(buffer, 0700);
#endif
    if (status == -1)
    {
        printf("Error occured. Unable to create sub directory.\n");
        return FALSE;
    }

    // check to make sure we have a source file
    snprintf(source_path, 1024, "%s/.atanks-config.txt", my_home_folder);
    source_file = fopen(source_path, "r");
    if (!source_file)
        return TRUE;

    // we already have an open source file, create destination file
    snprintf(dest_path, 1024, "%s/atanks-config.txt", global->configDir);
    dest_file = fopen(dest_path, "wb");
    if (!dest_file)
    {
        printf("Unable to create destination file.\n");
        fclose(source_file);
        return FALSE;
    }

    // we have open files, let's copy
    status = fread(buffer, 1, 1024, source_file);
    while (status)
    {
        status = fwrite(buffer, 1, 1024, dest_file);
        status = fread(buffer, 1, 1024, source_file);
    }

    fclose(source_file);
    fclose(dest_file);
    return TRUE;
}

// Make sure we have a music folder
// Returns TRUE on success or FALSE
// if an error occures.
int Create_Music_Folder(GLOBALDATA *global)
{
    DIR *music_folder;
    char *folder_path;

    folder_path = (char *) calloc(strlen(global->configDir) + 32, sizeof(char));
    if (!folder_path)
        return FALSE;

    sprintf(folder_path, "%s/music", global->configDir);
    music_folder = opendir(folder_path);
    if (!music_folder)
    {
#ifdef WIN32
        mkdir(folder_path);
#else
        mkdir(folder_path, 0700);
#endif
    }
    else    // it already exists
        closedir(music_folder);

    free(folder_path);
    return TRUE;
}

/*
Displays lines of text.
*/
void renderTextLines(GLOBALDATA *global, ENVIRONMENT *env, TEXTBLOCK *lines, int scrollOffset,
                     const FONT* fnt, const int spacing)
{
    const int textheight = text_height(fnt);
    const int textheight_s = textheight * spacing;
    int yOffset = scrollOffset;

    for (int count = 0;
         (count < lines->total_lines) && (yOffset < global->screenHeight);
         count++)
    {
        char *text = lines->complete_text[count];

        textout_centre_ex (env->db, fnt, text,
                           global->halfWidth + 2, global->halfHeight + yOffset + 2, BLACK, -1);
        textout_centre_ex (env->db, fnt, text,
                           global->halfWidth, global->halfHeight + yOffset, WHITE, -1);
        yOffset += textheight_s;
    }
}

/*
Scroll text in a box
*/
void scrollTextList (GLOBALDATA *global, ENVIRONMENT *env, TEXTBLOCK *lines)
{
    /* Justin says: this function, along with renderTextLines, aren't
       exactly efficient.  I think they could use rewrite. */
    static const int numItemsSrc[] = { 100, 30 };

    // DATAFILE *dffont ;
    const FONT* fnt = font;
    int spacing = 2;
    int tOffset = rand();
    int itemType = rand() / (RAND_MAX/2 + 1) ;
    int numItems = numItemsSrc[itemType];
    int my_key, done = FALSE;
    int moving = TRUE;

    draw_circlesBG(global, env->db, 0, 0, global->screenWidth, global->screenHeight, false);
    drawMenuBackground(global, env, BACKGROUND_BLANK, abs (tOffset), numItems);
    quickChange(global, env->db);
    //set_clip_rect(env->db, global->halfWidth - 300 + 1, 100 + 1,
    //              global->halfWidth + 300 - 1, global->screenHeight - 100 - 1);
    set_clip_rect(env->db, global->halfWidth - 300 + 1, global->menuBeginY + 1,
                  global->halfWidth + 300 - 1, global->menuEndY - 1);
    int scrollOffset = 0;
    flush_inputs();
    if (! global->os_mouse)
        show_mouse(NULL);

    // lines->Display_All(TRUE);
    do
    {
        drawMenuBackground(global, env, BACKGROUND_BLANK, abs (tOffset), numItems);

        renderTextLines(global, env, lines, scrollOffset, fnt, spacing);
        //blit(env->db, screen, global->halfWidth - 300, 100, global->halfWidth - 300, 100,
        //     601, global->screenHeight - 199);
        blit(env->db, screen, global->halfWidth - 300, global->menuBeginY,
             global->halfWidth - 300, global->menuBeginY, 601,
             global->screenHeight - 2 * global->menuBeginY);
        LINUX_REST;
        if (moving)
        {
            tOffset++;
            scrollOffset--;
        }

        if (scrollOffset < -(global->halfHeight - 100 + lines->total_lines * 30))
            scrollOffset = global->halfHeight - 100;
        if (global->close_button_pressed)
            break;
        if (keypressed())
        {
            my_key = (readkey()) >> 8;
            switch (my_key)
            {
            case KEY_ESC:
                done = TRUE;
                break;
            case KEY_SPACE:
                moving = TRUE;
                break;
            case KEY_UP:
                tOffset--;
                scrollOffset++;
                moving = FALSE;
                break;
            case KEY_DOWN:
                tOffset++;
                scrollOffset--;
                moving = FALSE;
                break;
            }      // end of switch
        }       // end of key pressed
    }
    while ((!done) && (!mouse_b));

    if (!global->os_mouse)
        show_mouse(screen);
    set_clip_rect(env->db, 0, 0, (global->screenWidth-1), (global->screenHeight-1));
    flush_inputs();
}

/* Flush key buffer and waits for button releases */
void flush_inputs()
{
    do { }
    while (mouse_b);
    clear_keybuf();
}

// This file loads weapons, naturals and items
// from a text file
// Returns TRUE on success and FALSE on failure
int Load_Weapons_Text(GLOBALDATA *global)
{
    FILE *wfile;
    char *path_to_file;
    char *status;
    char line[512];
    int file_stage = 0;     // weapons, natruals, items
    int data_stage = 0;     // name, descrption, data
    int item_count = 0, weapon_count = 0, natural_count = 0;

    setlocale(LC_NUMERIC, "C");
    // get path name
    path_to_file = (char *) calloc(strlen(global->dataDir) + 64, sizeof(char));
    if (!path_to_file)
    {
        printf( "Memory error occured while loading weapons.\n");
        return FALSE;
    }

    if (global->language == LANGUAGE_ENGLISH)
        sprintf(path_to_file, "%s/text/weapons.txt", global->dataDir);
    else if (global->language == LANGUAGE_PORTUGUESE)
        sprintf(path_to_file, "%s/text/weapons.pt_BR.txt", global->dataDir);
    else if (global->language == LANGUAGE_FRENCH)
        sprintf(path_to_file, "%s/text/weapons_fr.txt", global->dataDir);
    else if (global->language == LANGUAGE_GERMAN)
        sprintf(path_to_file, "%s/text/weapons_de.txt", global->dataDir);
    else if (global->language == LANGUAGE_SLOVAK)
        sprintf(path_to_file, "%s/text/weapons_sk.txt", global->dataDir);
    else if (global->language == LANGUAGE_RUSSIAN)
        sprintf(path_to_file, "%s/text/weapons_ru.txt", global->dataDir);
    else if (global->language == LANGUAGE_SPANISH)
        sprintf(path_to_file, "%s/text/weapons_ES.txt", global->dataDir);
    else if (global->language == LANGUAGE_ITALIAN)
        sprintf(path_to_file, "%s/text/weapons_it.txt", global->dataDir);

    // open file
    wfile = fopen(path_to_file, "r");
    // free memory
    // free(path_to_file);
    if (!wfile)
    {
        printf("Unable to open weapons file. (%s)\n", path_to_file);
        free(path_to_file);
        return FALSE;
    }
    free(path_to_file);

    Clear_Weapons();            // make sure arrays are cleared before loading data
    // read line
    status = fgets(line, 512, wfile);
    while (status)
    {
        // clear end of line
        if (strchr(line, '\n') )
            strchr(line, '\n')[0] = '\0';
        if (strchr(line, '\r') )
            strchr(line, '\r')[0] = '\0';

        // skip # and empty lines
        if ((!(line[0] == '#')) && (strlen(line) > 2))
        {
            // check for header
            if (!strcasecmp(line, "*WEAPONS*"))
            {
                file_stage = 0;
                data_stage = 0;
            }
            else if (!strcasecmp(line, "*NATURALS*"))
            {
                file_stage = 1;
                data_stage = 0;
            }
            else if (!strcasecmp(line, "*ITEMS*"))
            {
                file_stage = 2;
                data_stage = 0;
            }

            // not a special line, let's read some data
            else
            {
                // weapon
                if ((file_stage == 0) && (weapon_count < WEAPONS))
                {
                    if (data_stage == 0)   // name
                        strcpy(weapon[weapon_count].name, line);
                    else if (data_stage == 1)
                        strcpy(weapon[weapon_count].description, line);
                    else if (data_stage == 2)
                    {
                        sscanf(line, "%d %d %lf %lf %d %d %d %d %d %d %d %d %d %d %d %d %d %lf %d %lf %lf %lf %d %lf",
                               &(weapon[weapon_count].cost),
                               &(weapon[weapon_count].amt),
                               &(weapon[weapon_count].mass),
                               &(weapon[weapon_count].drag),
                               &(weapon[weapon_count].radius),
                               &(weapon[weapon_count].sound),
                               &(weapon[weapon_count].etime),
                               &(weapon[weapon_count].damage),
                               &(weapon[weapon_count].eframes),
                               &(weapon[weapon_count].picpoint),
                               &(weapon[weapon_count].spread),
                               &(weapon[weapon_count].delay),
                               &(weapon[weapon_count].noimpact),
                               &(weapon[weapon_count].techLevel),
                               &(weapon[weapon_count].warhead),
                               &(weapon[weapon_count].numSubmunitions),
                               &(weapon[weapon_count].submunition),
                               &(weapon[weapon_count].impartVelocity),
                               &(weapon[weapon_count].divergence),
                               &(weapon[weapon_count].spreadVariation),
                               &(weapon[weapon_count].launchSpeed),
                               &(weapon[weapon_count].speedVariation),
                               &(weapon[weapon_count].countdown),
                               &(weapon[weapon_count].countVariation));
                    }
                    data_stage++;
                    if (data_stage > 2)
                    {
                        data_stage = 0;
                        weapon_count++;
                    }
                }       // end of weapon section

                // naturals
                else if ((file_stage == 1) && (natural_count < NATURALS))
                {
                    if (data_stage == 0)   // name
                        strcpy(naturals[natural_count].name, line);
                    else if (data_stage == 1)
                        strcpy(naturals[natural_count].description, line);
                    else if (data_stage == 2)
                    {
                        sscanf(line, "%d %d %lf %lf %d %d %d %d %d %d %d %d %d %d %d %d %d %lf %d %lf %lf %lf %d %lf",
                               &(naturals[natural_count].cost),
                               &(naturals[natural_count].amt),
                               &(naturals[natural_count].mass),
                               &(naturals[natural_count].drag),
                               &(naturals[natural_count].radius),
                               &(naturals[natural_count].sound),
                               &(naturals[natural_count].etime),
                               &(naturals[natural_count].damage),
                               &(naturals[natural_count].eframes),
                               &(naturals[natural_count].picpoint),
                               &(naturals[natural_count].spread),
                               &(naturals[natural_count].delay),
                               &(naturals[natural_count].noimpact),
                               &(naturals[natural_count].techLevel),
                               &(naturals[natural_count].warhead),
                               &(naturals[natural_count].numSubmunitions),
                               &(naturals[natural_count].submunition),
                               &(naturals[natural_count].impartVelocity),
                               &(naturals[natural_count].divergence),
                               &(naturals[natural_count].spreadVariation),
                               &(naturals[natural_count].launchSpeed),
                               &(naturals[natural_count].speedVariation),
                               &(naturals[natural_count].countdown),
                               &(naturals[natural_count].countVariation));
                    }

                    data_stage++;
                    if (data_stage > 2)
                    {
                        data_stage = 0;
                        natural_count++;
                    }

                }       // end of naturals

                // item section
                else if ((file_stage == 2) && (item_count < ITEMS))
                {
                    if (data_stage == 0)   // name
                        strcpy(item[item_count].name, line);
                    else if (data_stage == 1)
                        strcpy(item[item_count].description, line);
                    else if (data_stage == 2)
                    {
                        sscanf(line, "%d %d %d %d %d %lf %lf %lf %lf %lf %lf",
                               &(item[item_count].cost),
                               &(item[item_count].amt),
                               &(item[item_count].selectable),
                               &(item[item_count].techLevel),
                               &(item[item_count].sound),
                               &(item[item_count].vals[0]),
                               &(item[item_count].vals[1]),
                               &(item[item_count].vals[2]),
                               &(item[item_count].vals[3]),
                               &(item[item_count].vals[4]),
                               &(item[item_count].vals[5]));
                    }

                    data_stage++;
                    if (data_stage > 2)
                    {
                        data_stage = 0;
                        item_count++;
                    }
                }         // end of items

            }       // end of reading data from a valid line

        }     // end of valid line

        // read in data
        status = fgets(line, 512, wfile);
    }

    // close file
    fclose(wfile);
    // go home
    return TRUE;
}

/*
Filter out files that do not have .sav in the name.
*/
#ifndef MACOSX
int Filter_File(const struct dirent *my_file)
#else
int Filter_File(struct dirent *my_file)
#endif
{
    if (strstr(my_file->d_name, ".sav"))
        return TRUE;
    else
        return FALSE;
}

/*
This function finds a list of saved games on your profile.
On error, NULL is returned. If all goes well, a list of file names
are returned.
After use, the return value should be freed.
*/
#ifndef WIN32
struct dirent ** Find_Saved_Games(GLOBALDATA *global, int *num_files_found)
{
    struct dirent **my_list;
    int status;

    status = scandir(global->configDir, &my_list, Filter_File, alphasort);
    if (status < 0)
    {
        printf("Error trying to find saved games.\n");
        return NULL;
    }

    *num_files_found = status;
    return my_list;
}
#endif

/*
This function hunts for saved games. If games are found, the
function returns an array of filenames. If an error occures
or no files are found, NULL is returned.
*/
#ifdef WIN32
struct dirent ** Find_Saved_Games(GLOBALDATA *global, int *num_files_found)
{
    struct dirent **my_list;
    struct dirent *one_file;
    int file_count = 0;
    DIR *game_dir;

    my_list = (struct dirent **) calloc(256, sizeof(struct dirent *));
    if (!my_list)
        return NULL;

    game_dir = opendir(global->configDir);
    if (!game_dir)
    {
        free(my_list);
        return NULL;
    }

    one_file = readdir(game_dir);
    while ((one_file) && (file_count < 256))
    {
        // check to see if this is a save game file
        if (strstr( one_file->d_name, ".sav"))
        {
            my_list[file_count] = (struct dirent *) calloc(1, sizeof(struct dirent) );
            if (my_list[file_count])
            {
                memcpy(my_list[file_count], one_file, sizeof(struct dirent) );
                file_count++;
            }
        }

        one_file = readdir(game_dir);
    }

    closedir(game_dir);
    *num_files_found = file_count;
    return my_list;
}
#endif

/*
 * This function grabs a random quote from the war_quotes.txt file.
 * This file is held in the data directory (dataDir) and contains
 * one-line quotes.
 * The function returns a pointer to the quote on success and a NULL
 * on failure. The line should be free()ed after use.
 * -- Jesse
 *  */
/*
char *Get_Random_Quote(GLOBALDATA *global)
{
    char *path_to_file = NULL;
    char *line, *result;
    FILE *quote_file = NULL;
    int line_count = 0;
    int index = 0;
    int random_number;

    line = (char *) calloc(1024, sizeof(char));
    if (!line)
        return NULL;
    path_to_file = (char *) calloc(strlen(global->dataDir) + 32, sizeof(char));
    if (!path_to_file)
    {
        free(line);
        return NULL;
    }

    if (global->language == LANGUAGE_RUSSIAN)
        sprintf(path_to_file, "%s/war_quotes_ru.txt", global->dataDir);
    else
        sprintf(path_to_file, "%s/war_quotes.txt", global->dataDir);
    quote_file = fopen(path_to_file, "r");
    free(path_to_file);
    if (! quote_file)
    {
        free(line);
        return NULL;
    }

    // search through file getting the number of lines
    result = fgets(line, 1024, quote_file);
    while (result)
    {
        line_count++;
        result = fgets(line, 1024, quote_file);
    }

    // return to the start of the file
    rewind(quote_file);

    // generate a random number based on the size of the file
    random_number = rand() % line_count;

    // search through and find the line we want
    result = fgets(line, 1024, quote_file);
    while ((result) && (index < random_number))
    {
        index++;
        result = fgets(line, 1024, quote_file);
    }

    fclose(quote_file);
    if (!result)
    {
        free(line);
        return NULL;
    }

    // trim trailing newline character
    if (strchr(line, '\n'))
        strchr(line, '\n')[0] = '\0';
    return line;
}
*/




/*
 * This function searches for bitmap files (.bmp) in the config folder.
 * The function returns an array of bitmap file names. If no files
 * are found, or an error occures, then NULL is returned.
 * */
char ** Find_Bitmaps(GLOBALDATA *global, int *bitmaps_found)
{
    char **my_list;
    struct dirent *one_file;
    int file_count = 0;
    DIR *game_dir;

    my_list = (char **) calloc(256, sizeof(struct dirent *));
    if (!my_list)
        return NULL;

    game_dir = opendir(global->configDir);
    if (!game_dir)
    {
        free(my_list);
        return NULL;
    }

    one_file = readdir(game_dir);
    while ((one_file) && (file_count < 256))
    {
        // check to see if this is a save game file
#ifdef LINUX
        if (strcasestr( one_file->d_name, ".bmp" ))
#else
        if ( (strstr(one_file->d_name, ".bmp")) || (strstr(one_file->d_name, ".BMP")) )
#endif
        {
            my_list[file_count] = (char *) calloc(strlen(global->configDir) + strlen(one_file->d_name) + 16, sizeof(char));
            if (my_list[file_count])
            {
                sprintf(my_list[file_count], "%s/%s", global->configDir, one_file->d_name);
                file_count++;
            }
        }

        one_file = readdir(game_dir);
    }

    closedir(game_dir);
    *bitmaps_found = file_count;
    if (file_count < 1)
    {
        free(my_list);
        my_list = NULL;
    }
    return my_list;
}

BITMAP *create_gradient_strip(const gradient *grad, int length)
{
    BITMAP *strip;
    int currLine;

    strip = create_bitmap(1, length);
    if (!strip)
        return NULL;

    clear_to_color(strip, BLACK);

    for (currLine = 0; currLine < length; currLine++)
    {
        int color = gradientColorPoint(grad, length, currLine);
        putpixel(strip, 0, currLine, color);
    }

    return strip;
}

int gradientColorPoint(const gradient *grad, double length, double line)
{
    int pointCount = 0;
    double point = line / length;
    int color;

    for (pointCount = 0; (point >= grad[pointCount].point) && (grad[pointCount].point != -1); pointCount++) { }
    pointCount--;

    if (pointCount == -1)
        color = makecol(grad[0].color.r, grad[0].color.g, grad[0].color.b);
    else if (grad[pointCount + 1].point == -1)
        color = makecol(grad[pointCount].color.r, grad[pointCount].color.g, grad[pointCount].color.b);
    else
    {
        double i = (point - grad[pointCount].point) / (grad[pointCount + 1].point - grad[pointCount].point);
        int r = (int) (interpolate(grad[pointCount].color.r, grad[pointCount + 1].color.r, i));
        int g = (int) (interpolate(grad[pointCount].color.g, grad[pointCount + 1].color.g, i));
        int b = (int) (interpolate(grad[pointCount].color.b, grad[pointCount + 1].color.b, i));

        color = makecol(r, g, b);
    }

    return color;
}

/*****************************************************************************
 * colorDistance
 *
 * Treat two color values as 3D vectors of the form <r,g,b>.
 * Compute the scalar size of the difference between the two vectors.
 * *****************************************************************************/
double colorDistance(int col1, int col2)
{
    double distance;
    int col1r, col1g, col1b;
    int col2r, col2g, col2b;

    col1r = getr(col1);
    col1g = getg(col1);
    col1b = getb(col1);
    col2r = getr(col2);
    col2g = getg(col2);
    col2b = getb(col2);

    // Treat the colour-cube as a space
    distance = vector_length_f((float)(col1r - col2r), (float)(col1g - col2g), (float)(col1b - col2b));

    return distance;
}

void Clear_Weapons()
{
    memset(weapon, 0, WEAPONS * sizeof(WEAPON));
    memset(naturals, 0, NATURALS * sizeof(WEAPON));
    memset(item, 0, ITEMS * sizeof(ITEM));

}

int Display_Tank_Bitmap(ENVIRONMENT *env, int xpos, int ypos, const void *image_number)
{
    const double *temp_number = (const double *) image_number;
    int my_number = (int) *temp_number;
    int use_tank_bitmap, use_turret_bitmap;
    BITMAP *dest = env->db;
    GLOBALDATA *global = env->_global;

    switch (my_number)
    {
    case CLASSIC_TANK:
        use_tank_bitmap = 8;
        use_turret_bitmap = 1;
        break;
    case BIGGREY_TANK:
        use_tank_bitmap = 9;
        use_turret_bitmap = 2;
        break;
    case T34_TANK:
        use_tank_bitmap = 10;
        use_turret_bitmap = 3;
        break;
    case HEAVY_TANK:
        use_tank_bitmap = 11;
        use_turret_bitmap = 4;
        break;
    case FUTURE_TANK:
        use_tank_bitmap = 12;
        use_turret_bitmap = 5;
        break;
    case UFO_TANK:
        use_tank_bitmap = 13;
        use_turret_bitmap = 6;
        break;
    case SPIDER_TANK:
        use_tank_bitmap = 14;
        use_turret_bitmap = 7;
        break;
    case BIGFOOT_TANK:
        use_tank_bitmap = 15;
        use_turret_bitmap = 8;
        break;
    case MINI_TANK:
        use_tank_bitmap = 16;
        use_turret_bitmap = 8;
        break;
    default:
        use_tank_bitmap = 0;
        use_turret_bitmap = 0;
        break;

    }

    draw_sprite(dest, global->tank[use_tank_bitmap], xpos, ypos);
    draw_sprite(dest, global->tankgun[use_turret_bitmap], xpos, ypos - TANKHEIGHT + 5);
    
    return TRUE;
}
