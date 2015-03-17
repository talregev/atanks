/*
 * atanks - obliterate each other with oversize weapons
 * Copyright (C) 2002,2003  Thomas Hudson,Juraj Michalek
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * */


#include <dirent.h>

#include "globals.h"
#include "menu.h"
#include "button.h"
#include "team.h"
#include "files.h"
#include "satellite.h"
#include "update.h"
#include "network.h"
#include "land.h"

#include "floattext.h"
#include "explosion.h"
#include "beam.h"
#include "missile.h"
#include "decor.h"
#include "teleport.h"
#include "sky.h"
#include "gameloop.h"

#ifdef THREADS
#include <pthread.h>
#endif

#ifdef NETWORK
#include "client.h"
#endif


enum cmdTokens
{
    ARGV_NOTHING_EXPECTED,
    ARGV_GFX_DEPTH,
    ARGV_SCREEN_WIDTH,
    ARGV_SCREEN_HEIGHT,
    ARGV_DATA_DIR,
    ARGV_CONFIG_DIR
};
#define SWITCH_HELP "-h"
#define SWITCH_FULL_SCREEN "-fs"
#define SWITCH_WINDOWED "--windowed"
#define SWITCH_NOSOUND "--nosound"
#define SWITCH_DATADIR "--datadir"
#define SWITCH_CONFIGDIR "-c"
#define SWITCH_NO_CONFIG "--noconfig"


int screen_mode = GFX_AUTODETECT_WINDOWED;

// BITMAP *create_gradient_strip(const gradient *gradient, int length);
// int draw_circlesBG(GLOBALDATA *global, BITMAP *dest, int x, int y, int width, int height, bool image);

using namespace std;

void fpsadd()
{
    fps = frames;
    frames = 0;
}

#ifdef THREADS
pthread_mutex_t* cclock_lock;
#endif
void destroy_cclock_lock()
{
#ifdef THREADS
    if (cclock_lock)
    {
        int result = pthread_mutex_destroy(cclock_lock);
        switch (result)
        {
        case 0:
            //Successfully destroyed
            break;
        case EBUSY:
            //Some thread forgot to unlock result
            printf("%s:%i: Lock is still held.\n", __FILE__, __LINE__);
            break;
        case EINVAL:
            //Invalid lock
            printf("%s:%i: Lock is invalid.\n", __FILE__, __LINE__);
            break;
        default:
            printf("%s:%i: Unknown error code (%i) returned by pthread_mutex_destroy.\n", __FILE__, __LINE__, result);
            break;
        }
        free(cclock_lock);
    }
#endif
}
void init_cclock_lock()
{
#ifdef THREADS
    cclock_lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (cclock_lock == NULL)
    {
        printf("%s:%i: Could not allocate memory.\n", __FILE__, __LINE__);
    }
    int result = pthread_mutex_init(cclock_lock, NULL);
    switch (result)
    {
    case 0:
        //Succesfully initialized
        break;
    case EAGAIN:
        //Not enough resources
        printf("%s:%i: Not enough resources to create mutex.\n", __FILE__, __LINE__);
        break;
    case ENOMEM:
        printf("%s:%i: Not enough memory to create mutex.\n", __FILE__, __LINE__);
        break;
    case EPERM:
        printf("%s:%i: Not authorized.\n", __FILE__, __LINE__);
        break;
    case EBUSY:
        printf("%s:%i: The mutex is already initialized.\n", __FILE__, __LINE__);
        break;
    case EINVAL:
        printf("%s:%i: Invalid attribute.\n", __FILE__, __LINE__);
        break;
    default:
        printf("%s:%i: Unknown error code (%i) returned by pthread_mutex_init.\n", __FILE__, __LINE__, result);
        break;
    }
    atexit(destroy_cclock_lock);
#endif
}
void lock_cclock()
{
#ifdef THREADS
    int result = pthread_mutex_lock(cclock_lock);
    switch (result)
    {
    case 0:
        //Got the lock.
        break;
    case EINVAL:
        //Priority too high
        printf("%s:%i: Either this thread's priority is higher than the mutex's priority, or the mutex is uninitialized.\n", __FILE__, __LINE__);
        break;
    case EAGAIN:
        printf("%s:%i: Too many locks on the mutex.\n", __FILE__, __LINE__);
        break;
    case EDEADLK:
        //We have the lock already
        printf("%s:%i: Already have lock.\n", __FILE__, __LINE__);
        break;
    default:
        //What error is this?
        printf("%s:%i: Unknown error code (%i) returned by pthread_mutex_lock.\n", __FILE__, __LINE__, result);
        break;
    }
#endif
}
void unlock_cclock()
{
#ifdef THREADS
    int result = pthread_mutex_unlock(cclock_lock);
    switch (result)
    {
    case 0:
        //Released the lock
        break;
    case EPERM:
        //Forgot to get a lock on the mutex
        printf("%s:%i: Mutex isn't locked\n", __FILE__, __LINE__);
        break;
    case EINVAL:
        //Uninitialized mutex
        printf("%s:%i: cclock_lock is uninitialized.\n", __FILE__, __LINE__);
        break;
    default:
        //?
        printf("%s:%i: Unknown error code (%i) returned by pthread_mutex_unlock.\n", __FILE__, __LINE__, result);
        break;
    }
#endif
}
int get_cclock()
{
    lock_cclock();
    int c = cclock;
    unlock_cclock();
    return c;
}
void clockadd()
{
    lock_cclock();
    cclock++;
    unlock_cclock();
}
END_OF_FUNCTION(clockadd)

/*****************************************************************************
drawMenuBackground

Draws a 600x400 centered box, fills it with some random lines or circles.
Someday, we should make this more generic; have it take the box dimensions
as an input parameter.
*****************************************************************************/
void drawMenuBackground(GLOBALDATA *global, ENVIRONMENT *env, int itemType, int tOffset, int numItems)
{
    rectfill(env->db, global->halfWidth - 300, global->menuBeginY, // 100,
             global->halfWidth + 300, global->menuEndY, // global->screenHeight - 100,
             makecol(0,79,0));
    rect(env->db, global->halfWidth - 300, global->menuBeginY, // 100,
         global->halfWidth + 300, global->menuEndY, // global->screenHeight - 100,
         makecol(128,255,128));

    drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
    env->current_drawing_mode = DRAW_MODE_TRANS;
    set_trans_blender(0, 0, 0, 15);
    for (int tCount = 0; tCount < numItems; tCount++)
    {
        int radius, xpos, ypos;
        switch (itemType)
        {
        case BACKGROUND_CIRCLE: // circles
            radius = (int) ((perlin1DPoint (1.0, 5, (tOffset * 0.02) + tCount + 423346, 0.5, 8) + 1) / 2 * 40);
            xpos = global->halfWidth + (int) (perlin1DPoint(1.0, 3, (tOffset * 0.02) + tCount + 232662, 0.3, 6) * 250);
            ypos = global->halfHeight + (int) (perlin1DPoint(1.0, 2, (tOffset * 0.02) + tCount + 42397, 0.3, 6) * (global->halfHeight - 100));
            circlefill(env->db, xpos, ypos, radius, makecol(200,255,200));
            break;
        case BACKGROUND_LINE: // Horz lines
            radius = (int) ((perlin1DPoint(1.0, 5, (tOffset * 0.02) + tCount + 423346, 0.5, 8) + 1) / 2 * 40);
            xpos = global->halfWidth + (int) (perlin1DPoint(1.0, 3, (tOffset * 0.02) + tCount + 232662, 0.3, 6) * 250);
            rectfill(env->db, xpos - radius / 2, 101, xpos + radius / 2, global->screenHeight - 101, makecol(200,255,200));
            break;
        case BACKGROUND_BLANK:
        default:
            break;
        }

    }
    solid_mode();
    env->current_drawing_mode = DRAW_MODE_SOLID;
}

void initialisePlayers(GLOBALDATA *global)
{
    int z;

    for (z = 0; z < global->numPlayers; z++)
    {
        global->players[z]->money = (int) global->startmoney;
        global->players[z]->score = 0;
        // global->players[z]->initialise();
        // global->players[z]->type_saved = global->players[z]->type;
        if (((int) global->players[z]->type != HUMAN_PLAYER) && (global->players[z]->preftype == PERPLAY_PREF))
            global->players[z]->generatePreferences();

        global->players[z]->initialise();
        global->players[z]->type_saved = global->players[z]->type;
    }
}

void wait_for_input()
{
    do
    {
        LINUX_SLEEP;
    }
    while ((!keypressed()) && !mouse_b);

    flush_inputs();
}

int pickColor(int left, int top, int width, int height, int x, int y)
{
    int r, g, b;
    double value, saturation;
    double hue = ((double) (x - left) / width) * 360;

    double hPos = (double) (y - top) / height;
    if (hPos > 0.5)
    {
        value = 1.0 - ((hPos - 0.5) * 2);
        saturation = 1.0;
    }
    else
    {
        value = 1.0;
        saturation = hPos * 2;
    }

    hsv_to_rgb(hue, saturation, value, &r, &g, &b);

    return (makecol(r, g, b));
}

void colorBar(ENVIRONMENT *env, int left, int top, int width, int height)
{
    int right = left + width;
    int bottom = top + height;

    for (int x = left; x < right; x++)
    {
        for (int y = top; y < bottom; y++)
            putpixel(env->db, x, y, pickColor(left, top, width, height, x, y));
    }
}

int textEntryBox(GLOBALDATA *global, ENVIRONMENT *env, int modify, int x, int y, char *text, unsigned int textLength)
{
    int ke = 0;
    int fontWidth = text_length(font, "Z");
    int fontHeight = text_height(font);
    int leftX = x - (fontWidth * textLength / 2);
    int rightX = x + (fontWidth * textLength / 2);
    int boxWidth = fontWidth * textLength;
    //  char tempText[textLength + 1];
    char * tempText;
    int flashCount = 0;
    int lx = mouse_x, ly = mouse_y;

    tempText = (char *) calloc(textLength + 1, sizeof(char));

    if (!tempText)
    {
        // Die hard!
        cerr << "ERROR: Unable to allocate " << (textLength + 1) << " bytes in textEntryBox() !!!" << endl;
        // exit (1);
    }

    if (!text)
        text = "";
    else if ( (text == (void*)0xb) || (text == (void*)0xc) )
        text = "";
    rectfill(env->db, leftX, y - 2, rightX, y + fontHeight + 2, WHITE);
    rect(env->db, leftX, y - 2, rightX, y + fontHeight + 2, BLACK);
    if (!modify)
        textout_centre_ex (env->db, font, text, x, y, BLACK, -1);

    env->make_update(leftX - 2, y - 4, fontWidth * textLength + 4, fontHeight + 6);
    env->do_updates();

    if (!modify)
    {
        free(tempText);
        return boxWidth;
    }
    strncpy(tempText, text, textLength + 1);

    while( ((ke >> 8) != KEY_ENTER && (ke >> 8) != KEY_ESC && (ke >> 8) != KEY_ENTER_PAD) || strlen(tempText) < 1 )
    {
        int tWidth = text_length(font, tempText);

        LINUX_SLEEP;

        rectfill(env->db, leftX, y - 2, rightX, y + fontHeight + 2, WHITE);
        rect(env->db, leftX, y - 2, rightX, y + fontHeight + 2, BLACK);
        //rectfill(screen, x - (tWidth / 2), y, x + (tWidth / 2) + 10, y + text_height (font), (flashCount < 5)?WHITE:BLACK);
        textout_centre_ex(env->db, font, tempText, x, y, BLACK, -1);
        rectfill(env->db, x + (tWidth / 2) + 2, y, x + (tWidth / 2) + 10, y + text_height (font), (flashCount < 25)?WHITE:BLACK);
        env->make_update(leftX - 2, y - 4, fontWidth * textLength + 4, fontHeight + 6);
        env->make_update(mouse_x, mouse_y, ((BITMAP *) (global->misc[0]))->w, ((BITMAP *) (global->misc[0]))->h);
        env->make_update(lx, ly, ((BITMAP *) (global->misc[0]))->w, ((BITMAP *) (global->misc[0]))->h);
        lx = mouse_x;
        ly = mouse_y;
        if (!global->os_mouse)
            show_mouse(NULL);

        if (keypressed())
            ke = readkey();
        else
            ke = 0;

        if ( ((ke >> 8) == KEY_BACKSPACE) && (strlen(tempText) > 0) )
        {
            tempText[strlen(tempText) - 1] = 0;
            rectfill(screen, x - (tWidth / 2), y, x + (tWidth / 2) + 10, y + text_height(font), WHITE);
            env->make_update(x - (tWidth / 2) - 2, y - 2, tWidth + 14, text_height(font) + 4);
        }
        else if ((ke & 0xff) >= 32 && strlen(tempText) < textLength)
        {
            // tempText[strlen (tempText)] = ke & 0xff;
            tempText[strlen(tempText) + 1] = 0;
            tempText[strlen(tempText)] = ke & 0xff;
            //textprintf(screen, font, x + text_length (font, tempText), y, WHITE, "%c", ke & 0xff);
        }
        else
            env->do_updates();
        if (! global->os_mouse)
            show_mouse(screen);
        rest(1);
        flashCount++;
        flashCount = flashCount % 50;
    }
    if ((ke >> 8) != KEY_ESC)
        strncpy(text, tempText, textLength);

    flush_inputs();

    free(tempText);

    return boxWidth;
}

void credits(GLOBALDATA *global, ENVIRONMENT *env)
{
    char dataDir[2048];
    TEXTBLOCK *my_text;

    sprintf(dataDir, "%s/credits.txt", global->dataDir);
    my_text = new TEXTBLOCK(dataDir);
    scrollTextList(global, env, my_text);
    delete my_text;
}

/* Save all players to a text file. */
int savePlayers_Text(GLOBALDATA *global, FILE *my_file)
{
    int count = 0;

    while (count < global->numPermanentPlayers)
    {
        global->allPlayers[count]->saveToFile_Text(my_file);
        count++;
    }
    return TRUE;
}

int loadPlayers_Text(GLOBALDATA *global, ENVIRONMENT *env, FILE *file)
{
    int count, max;
    int status = TRUE;
    PLAYER *new_player;

    max = global->numPermanentPlayers;
    if (global->allPlayers)
        free(global->allPlayers);    // avoid leak
    global->allPlayers = (PLAYER **) malloc(sizeof(PLAYER*) * max);
    if (!global->allPlayers)
    {
        perror("atanks.cpp: Failed to allocate memory for allPlayers in loadPlayers_Text");
        return FALSE;
    }

    count = 0;
    while (status)
    {
        // global->allPlayers[count] = new PLAYER(global, env, file, true);
        new_player = new PLAYER(global, env);
        if (!new_player)
        {
            perror("atanks.cpp: Failed to allocate memory for players in loadPlayers_Text");
            return FALSE;
        }
        status = new_player->loadFromFile_Text(file);
        if (status)
        {
            global->allPlayers[count] = new_player;
            count++;
            if (count == max)
            {
                max += 5;
                global->allPlayers = (PLAYER**) realloc(global->allPlayers, sizeof(PLAYER *) * max);
            }
        }
        else
            // free(new_player);
            delete new_player;
    }     // end of while status

    global->numPermanentPlayers = count;
    return TRUE;
}

void newgame(GLOBALDATA *global, ENVIRONMENT *env)
{
    int objCount;
    TANK *tank;

    env->initialise();
    global->initialise();

    // if a game should be loaded, try it or deny loading of the game
    if ( (global->load_game) && (!Load_Game(global, env)) )
        global->load_game = false;

    // Now check back whether to load a game
    if (!global->load_game)
        initialisePlayers(global);

    // There must not be any tanks!
    for (objCount = 0; (tank = (TANK*) env->getNextOfClass(TANK_CLASS, &objCount)) && tank; objCount++)
    {
        tank->player = NULL;    // To avoid violent death!
        delete(tank);
    }

    // This is always true here, as a newly started game is handled like a loaded one:
    global->bIsGameLoaded = true;
}

// This function draws the background for most screens
int draw_circlesBG(GLOBALDATA *global, BITMAP *dest, int /*x*/, int /*y*/, int /*width*/, int /*height*/, bool image)
{
    // int largestCircle, circleCount;
    // BITMAP *drawTo = dest;

    // try to prevent crashes on 64-bit systems by avoiding this function
    if (!global->draw_background)
    {
        rectfill(dest, 0, 0, global->screenWidth - 1, global->screenHeight - 1, BLACK);
        return 0;
    }

    if (image && (global->misc[17]))
        stretch_blit(global->misc[17], dest, 0, 0, global->misc[17]->w, global->misc[17]->h, 0, 0,
                     global->screenWidth, global->screenHeight);
    else
        rectfill(dest, 0, 0, global->screenWidth - 1, global->screenHeight - 1, DARK_GREEN);

/*
    if (global->cacheCirclesBG)
    {
        if (!global->gfxData.circlesBG)
        {
            global->gfxData.circlesBG = create_bitmap(width, height);
            drawTo = global->gfxData.circlesBG;
        }
        else
        {
            blit(global->gfxData.circlesBG, dest, 0, 0, 0, 0, width, height);
            return 0;
        }
    }
    else
        drawTo = dest;

    largestCircle = (int) (global->halfWidth * (4.0/3.0));
    if (largestCircle > 1000)
        largestCircle = 1000;    // perhaps avoid crash on large screens
    global->gfxData.circle_gradient_strip = create_gradient_strip(circles_gradient, largestCircle);
    for (circleCount = largestCircle; circleCount > 0; circleCount -= 2)
        circlefill(drawTo, width/2, height/2, circleCount, getpixel(global->gfxData.circle_gradient_strip, 0, largestCircle - circleCount));

    if (global->cacheCirclesBG)
        draw_circlesBG(global, dest, x, y, width, height);
*/
    return 0;
}

ENVIRONMENT *init_game_settings(GLOBALDATA *global)
{
    int count, x, y, z;
    ENVIRONMENT *env;
    double expSize, disperseSize;
    // char dataDir[2048];
    int colour_theme = (int) global->colour_theme;
    int status;

#ifdef WIN32
    if (global->full_screen == FULL_SCREEN_TRUE)
        global->os_mouse = FALSE;
#endif

    status = allegro_init();
    if (status)
    {
        printf("Unable to start Allegro.\n");
        exit(1);
    }

    set_window_title("Atomic Tanks");
    // before we get started, make sure if we are using
    // full screen mode to ignore width and height settings
    if (global->full_screen == FULL_SCREEN_TRUE)
    {
        status = get_desktop_resolution(&(global->screenWidth), &(global->screenHeight));
        if (status < 0)
        {
            global->screenWidth = 800;
            global->screenHeight = 600;
        }
        screen_mode = GFX_AUTODETECT_FULLSCREEN;
    }
    // check for X pressed on the window bar
    LOCK_FUNCTION(close_button_handler);
    set_close_button_callback(close_button_handler);

    if (!global->colourDepth)
        global->colourDepth = desktop_color_depth();

    if ((global->colourDepth != 16) && (global->colourDepth != 32))
        global->colourDepth = 16;

    set_color_depth (global->colourDepth);
    if (global->width_override)
    {
        global->screenWidth = global->width_override;
        global->halfWidth = global->screenWidth / 2;
    }
    if (global->height_override)
    {
        global->screenHeight = global->height_override;
        global->halfHeight = global->screenHeight / 2;
    }
    if (set_gfx_mode(screen_mode, global->screenWidth, global->screenHeight, 0, 0) < 0)
    {
        perror("set_gfx_mode");
        // exit (1);
        status = set_gfx_mode(screen_mode, 800, 600, 0, 0);
        if (status < 0)
            exit(1);
        global->screenWidth = 800;
        global->screenHeight = 600;
        global->halfWidth = 400;
        global->halfHeight = 300;
    }

#ifdef WIN32
    if (global->full_screen == FULL_SCREEN_TRUE)
        set_display_switch_mode(SWITCH_BACKAMNESIA);
    else
        set_display_switch_mode(SWITCH_BACKGROUND);
#endif

    if (install_keyboard() < 0)
    {
        perror("install_keyboard failed");
        exit (1);
    }
    if (install_timer() < 0)
    {
        perror("install_timer failed");
        exit (1);
    }
    if (install_mouse() < 0)
    {
        perror("install_mouse failed");
        // exit (1);
    }

    // check to see if we want sound
    if (global->sound > 0.0)
    {
        /*
      // don't stop program if no sound since the game can be played without
      if (install_sound (DIGI_AUTODETECT, MIDI_NONE, NULL) < 0)
        fprintf (stderr, "install_sound: %s", allegro_error);
    }
    */
        int sound_type = DIGI_AUTODETECT;
#ifdef LINUX
        switch ((int) global->sound_driver)
        {
        case SOUND_OSS:
            sound_type = DIGI_OSS;
            break;
        case SOUND_ESD:
            sound_type = DIGI_ESD;
            break;
        case SOUND_ARTS:
            sound_type = DIGI_ARTS;
            break;
        case SOUND_ALSA:
            sound_type = DIGI_ALSA;
            break;
        case SOUND_JACK:
            sound_type = DIGI_JACK;
            break;
        default:
            sound_type = DIGI_AUTODETECT;
            break;
        }
#endif
#ifdef UBUNTU
        if (sound_type == DIGI_AUTODETECT)
            sound_type = DIGI_OSS;
#endif
        if (detect_digi_driver(sound_type))
        {
            if (install_sound(sound_type, MIDI_NONE, NULL) < 0)
            {
                fprintf(stderr, "install_sound: failed initialising sound\n");
                fprintf(stderr, "Please try selecting a different Sound Driver from the Options menu.\n");
            }
        }
        else
            fprintf(stderr, "detect_digi_driver detected no sound device\n");
    }    // end of we want sound

    lock_cclock();
    LOCK_VARIABLE(cclock);
    unlock_cclock();
    LOCK_FUNCTION(clockadd);
    // if (install_int_ex(clockadd, BPS_TO_TIMER (FRAMES_PER_SECOND)) < 0) {
    if (install_int_ex(clockadd, BPS_TO_TIMER(global->frames_per_second)) < 0)
    {
        perror("install_int_ex");
        exit(1);
    }
    if (install_int(fpsadd, 1000) < 0)
    {
        perror("install_int");
        exit(1);
    }

    srand(time(0));
    WHITE = makecol(255, 255, 255);
    BLACK = makecol(0, 0, 0);
    PINK = makecol(255, 0, 255);
    RED = makecol(255, 0, 0);
    GREEN = makecol(0, 255, 0);
    DARK_GREEN = makecol(0, 80, 0);
    BLUE = makecol(0, 0, 255);
    PURPLE = makecol(200, 0, 200);
    YELLOW = makecol(255, 255, 0);

    global->Load_Bitmaps();
    global->Load_Fonts();

    if (!global->os_mouse)
        show_mouse(NULL);
    blit((BITMAP *) global->title[0], screen, 0, 0, global->halfWidth - 320, global->halfHeight - 240, 640, 480);
    if (!global->os_mouse)
        show_mouse(screen);

    global->Load_Sounds();

    for (count = 0; count < ALL_LANDS; count++)
        global->gfxData.land_gradient_strips[count] = NULL;
    for (count = 0; count < ALL_SKIES; count++)
        global->gfxData.sky_gradient_strips[count] = NULL;

    global->gfxData.explosion_gradient_strip = create_gradient_strip(explosion_gradients[colour_theme], 200);
    // for (count = 0; count < 2; count++)
    //  global->gfxData.explosion_gradient_strips[count] = create_gradient_strip(explosion_gradients[count], 200);

    expSize = 0;
    disperseSize = 0;
    for (count = 0; count < EXPLOSIONFRAMES; count++)
    {
        global->gfxData.explosions[count] = create_bitmap(214, 214);
        if (count == 0)
        {
            expSize = 25;
            disperseSize = 0;
        }
        else if (count < EXPLODEFRAMES - 4)
            expSize += (107 - expSize) / 3;
        else if (count < EXPLODEFRAMES)
            expSize--;
        else if (count == EXPLODEFRAMES)
            disperseSize = 25;
        else
            disperseSize += (107 - disperseSize) / 2;

        clear_to_color(global->gfxData.explosions[count], PINK);
        for (y = (int) expSize; y > disperseSize; y--)
        {
            double value;
            value = pow ((double) y / expSize, count / 4 + 1);
            circlefill(global->gfxData.explosions[count], 107, 107, y, getpixel(global->gfxData.explosion_gradient_strip, 0, (int)(value * 200)));
        }
        if (disperseSize)
            circlefill(global->gfxData.explosions[count], 107, 107, (int) disperseSize, PINK);
    }

    expSize = 0;
    disperseSize = 0;
    for (count = 0; count < EXPLOSIONFRAMES; count++)
    {
        global->gfxData.flameFront[count] = create_bitmap(600, 30);
        if (count == 0)
        {
            expSize = 10;
            disperseSize = 0;
        }
        else if (count < EXPLODEFRAMES - 4)
            expSize += (300 - expSize) / 3;
        else if (count < EXPLODEFRAMES)
            expSize--;
        else if (count == EXPLODEFRAMES)
            disperseSize = 10;
        else
            disperseSize += (300 - disperseSize) / 2;

        clear_to_color(global->gfxData.flameFront[count], PINK);
        for (y = (int) expSize; y > disperseSize; y--)
        {
            double value;
            value = pow((double) y / expSize, count / 4 + 1);
            ellipsefill(global->gfxData.flameFront[count], 300, 15, y, y / 20, getpixel(global->gfxData.explosion_gradient_strip, 0, (int)(value * 200)));
        }
        if (disperseSize)
            ellipsefill(global->gfxData.flameFront[count], 300, 15, (int) disperseSize, (int) disperseSize / 16, PINK);
    }

    global->gfxData.topbar = create_bitmap (global->screenWidth, MENUHEIGHT);
    global->gfxData.topbar_gradient_strip = create_gradient_strip (topbar_gradient, 100);
    if (!global->ditherGradients)
    {
        for (count = 0; count < MENUHEIGHT; count++)
        {
            float adjCount = (100.0 / MENUHEIGHT) * count;
            line(global->gfxData.topbar, 0, count, global->screenWidth - 1, count, getpixel(global->gfxData.topbar_gradient_strip, 0, (int)adjCount));
        }
    }
    else
    {
        for (x = 0; x < global->screenWidth; x++)
        {
            for (y = 0; y < MENUHEIGHT; y++)
            {
                float adjY = (100.0 / MENUHEIGHT) * y;
                int offset;
                if ((adjY < 2) || (adjY > 100 - 2))
                    offset = 0;
                else
                    offset = rand() % 4 - 2;
                putpixel(global->gfxData.topbar, x, y, getpixel(global->gfxData.topbar_gradient_strip, 0, (int)adjY + offset));
            }
        }
    }

    global->gfxData.stuff_bar[0] = create_bitmap(STUFF_BAR_WIDTH, STUFF_BAR_HEIGHT);
    global->gfxData.stuff_bar[1] = create_bitmap(STUFF_BAR_WIDTH, STUFF_BAR_HEIGHT);
    global->gfxData.stuff_icon_base = create_bitmap(STUFF_BAR_WIDTH/10, STUFF_BAR_HEIGHT);
    clear_to_color(global->gfxData.stuff_bar[0], PINK);
    clear_to_color(global->gfxData.stuff_bar[1], PINK);
    clear_to_color(global->gfxData.stuff_icon_base, PINK);
    global->gfxData.stuff_bar_gradient_strip = create_gradient_strip(stuff_bar_gradient, STUFF_BAR_WIDTH);
    for (x = 0; x < STUFF_BAR_WIDTH; x++)
    {
        for (y = 0; y < STUFF_BAR_HEIGHT; y++)
        {
            double sides_dist = 0.1, circle_dist;
            circle_dist = vector_length_f((float)x-(STUFF_BAR_WIDTH - 75), (float)y - (STUFF_BAR_HEIGHT/2 - 2), 0);
            if (circle_dist < 75)
                circle_dist = 1 - (circle_dist / 75.0);
            else
                circle_dist = 0;

            if (x < (STUFF_BAR_HEIGHT/2 - 2))
                sides_dist -= 0.1 - ((float) x / 150.0);
            else if (x > STUFF_BAR_WIDTH - (STUFF_BAR_HEIGHT/2 - 2))
                sides_dist -= ((float)(x - (STUFF_BAR_WIDTH - (STUFF_BAR_HEIGHT/2 - 2))) / 150.0);

            if (y < STUFF_BAR_HEIGHT/2 - 2)
                sides_dist -= 0.1 - ((float) y / 150.0);
            else
                sides_dist -= ((float) (y - (STUFF_BAR_HEIGHT/2 - 2)) / 150.0);

            sides_dist -= circle_dist * circle_dist;
            if (sides_dist > ((double) x / 1000.0))
                sides_dist = ((double) x / 1000.0);
            if (sides_dist < 0)
                sides_dist = 0;
            if (circle_dist > 1)
                circle_dist = 1;

            if (x < STUFF_BAR_WIDTH/10)
                putpixel(global->gfxData.stuff_icon_base, x, y, getpixel(global->gfxData.stuff_bar_gradient_strip, 0, (int)((sides_dist + circle_dist) * (STUFF_BAR_WIDTH-1))));

            if (y < STUFF_BAR_HEIGHT - 5)
            {
                putpixel(global->gfxData.stuff_bar[0], x, y, getpixel(global->gfxData.stuff_bar_gradient_strip, 0, (int)((sides_dist + circle_dist) * (STUFF_BAR_WIDTH-1))));
                putpixel(global->gfxData.stuff_bar[1], x, y, getpixel(global->gfxData.stuff_bar_gradient_strip, 0, (int)((sides_dist + circle_dist + 0.05) * (STUFF_BAR_WIDTH-1))));
            }
        }
    }

    if (!global->os_mouse)
    {
        set_mouse_sprite((BITMAP *) global->misc[0]);
        set_mouse_sprite_focus(0, 0);
    }

    global->window.x = 0;
    global->window.y = 0;
    global->window.w = 0;
    global->window.h = 0;
    for (z = 0; z < MAXUPDATES; z++)
    {
        global->updates[z].x = 0;
        global->updates[z].y = 0;
        global->updates[z].w = 0;
        global->updates[z].h = 0;
    }

    env = new ENVIRONMENT(global);
    if (!env)
    {
        perror("atanks.cpp: Allocating env in init_game_settings");
        exit(1);
    }

    clear_to_color(env->db, BLACK);
    global->env = env;

    return env;
}

int options(GLOBALDATA *global, ENVIRONMENT *env, MENUDESC *menu);

int createNewPlayer(GLOBALDATA *global, ENVIRONMENT *env)
{
    PLAYER *newPlayer = global->createNewPlayer(env);
    if (!newPlayer)
    {
        perror("atanks.cpp: Failed allocating memory in createNewPlayer");
        // exit (1);
    }
    options(global, env, (MENUDESC*) newPlayer->menudesc);
    return -1;
}

int destroyPlayer(GLOBALDATA *global, ENVIRONMENT *env, void *data)
{
    int optionsRetVal;
    char sureMessage[200];
    PLAYER *tempPlayer = (PLAYER*) data;
    MENUDESC areYouSureMenu = {"Are You Sure?", 0, NULL, TRUE, TRUE};

    sprintf(sureMessage, "This player (%s) will be permanently deleted", tempPlayer->getName());
    errorMessage = sureMessage;
    errorX = global->halfWidth - text_length(font, errorMessage) / 2;
    errorY = 170;
    optionsRetVal = options(global, env, &areYouSureMenu);
    if (optionsRetVal >> 8 != KEY_ESC)
    {
        global->destroyPlayer(tempPlayer);
        return -2;
    }
    return (KEY_SPACE << 8);
}

int displayPlayerName(ENVIRONMENT *env, int x, int y, void *data)
{
    PLAYER *player = (PLAYER*)data;
    char *name = player->getName ();
    int textHeight = text_height (font);
    int textLength = text_length (font, name);

    if ((int) player->type == HUMAN_PLAYER)
    {
        int radius = 5;
        circlefill(env->db, x - textLength - textHeight / 2 - 2, y + textHeight / 2,
                   radius, makecol (200, 100, 255));
        circle(env->db, x - textLength - textHeight / 2 - 2, y + textHeight / 2, radius, BLACK);
    }
    else
    {
        rectfill(env->db, x - textLength - 2 - ((int) player->type * 3), y + textHeight - 10,
                 x - textLength - 2, y + textHeight - 2, makecol(100, 255, 100));
        rect(env->db, x - textLength - 2 - ((int) player->type * 3), y + textHeight - 10,
             x - textLength - 2, y + textHeight - 2, BLACK);
        for (int lineCount = 1; lineCount < player->type; lineCount++)
            vline(env->db, x - textLength - 2 - (lineCount * 3), y + textHeight - 2, y + textHeight - 10, BLACK);
    }
    textout_ex(env->db, font, name, x - textLength, y, player->color, -1);

    return 0;
}

int options(GLOBALDATA *global, ENVIRONMENT *env, MENUDESC *menu)
{
    MENUENTRY *opts;
    char my_pointer[2];
    BUTTON *reset_button = NULL;
    int selected_index = 0, my_key = 0;
    int numEntries;
    const char *title;
    int ke, z;
    int mouseLeftPressed;
    char *reset_text, *back_text;
#include "menucontent.h"

    if (global->language == LANGUAGE_GERMAN)
    {
        reset_text = "Alles zurucksetzen";
        back_text = "Zuruck";
    }
    else
    {
        reset_text = "Reset All";
        back_text = "Back";
    }

    if (!menu)
    {
        menu = &mainMenu;
        reset_button = new BUTTON(global, env, global->halfWidth - 5 - text_length (font, menu->title), global->menuBeginY + 70, // 170,
                                  reset_text, (BITMAP *) global->misc[7], (BITMAP *) global->misc[7], (BITMAP *) global->misc[8]);
    }

    opts = menu->entries;
    numEntries = menu->numEntries;
    title = menu->title;

    char name_buff[64] = {0x0};
    char format_buff[64] = {0x0};
    int done, lb;
    int stop = 0;

    int *updateoption;

    updateoption = (int *) calloc(numEntries, sizeof(int));
    if (!updateoption)
    {
        // Die hard!
        cerr << "ERROR: Unable to allocate " << numEntries << " bytes in options() !!!" << endl;
        // exit (1);
    }

    BUTTON *but_okay = NULL, *but_quit = NULL;
    if (menu->okayButton)
    {
        int xpos = global->halfWidth - 80;
        if (menu->quitButton)
            xpos -= 80;
        // but_okay = new BUTTON(global, env, xpos, global->halfHeight + 160, "Okay", (BITMAP*)global->misc[7], (BITMAP*)global->misc[7], (BITMAP*)global->misc[8]);
        but_okay = new BUTTON(global, env, xpos, global->menuBeginY + 360 , "Okay", (BITMAP*)global->misc[7], (BITMAP*)global->misc[7], (BITMAP*)global->misc[8]);
        if (!but_okay)
        {
            perror("atanks.cpp: Failed allocating memory for but_okay in options");
            // exit (1);
        }
    }
    if (menu->quitButton)
    {
        int xpos = global->halfWidth - 80;
        if (menu->okayButton)
            xpos += 80;
        but_quit = new BUTTON(global, env, xpos, global->menuBeginY + 360, back_text, (BITMAP*)global->misc[7], (BITMAP*)global->misc[7], (BITMAP*)global->misc[8]);
        // but_quit = new BUTTON(global, env, xpos, global->halfHeight + 160, back_text, (BITMAP*)global->misc[7], (BITMAP*)global->misc[7], (BITMAP*)global->misc[8]);
        if (!but_quit)
        {
            perror("atanks.cpp: Failed allocating memory for but_quit in options");
            // exit (1);
        }
    }

    lock_cclock();
    mouseLeftPressed = done = lb = env->mouseclock = cclock = 0;
    unlock_cclock();
    fi = 1;

    for (z = 0; z < numEntries; z++)
        updateoption[z] = 1;

    flush_inputs();

    do
    {
        LINUX_SLEEP;
        while (get_cclock() > 0)
        {
            lock_cclock();
            cclock--;
            unlock_cclock();
            my_key = 0;
            if (keypressed())
            {
                my_key = readkey();
                my_key = my_key >> 8;
                if (my_key == KEY_DOWN)
                {
                    selected_index++;
                    if (selected_index >= numEntries)
                        selected_index = 0;
                    my_key = 0;
                }
                else if (my_key == KEY_UP)
                {
                    selected_index--;
                    if (selected_index < 0)
                        selected_index = numEntries - 1;
                    my_key = 0;
                }
                else if (my_key == KEY_ENTER_PAD)
                    my_key = KEY_ENTER;

                for (z = 0; z < numEntries; z++)
                    updateoption[z] = TRUE;
                fi = TRUE;

            }    // end of a key was pressed

            if (!lb && mouse_b & 1)
            {
                env->mouseclock = 0;
                mouseLeftPressed = 1;
            }
            else
                mouseLeftPressed = 0;

            lb = (mouse_b & 1) ? 1 : 0;
            if ( ((mouse_b & 1 || mouse_b & 2) && !env->mouseclock) || my_key )
            {
                for (z = 0; z < numEntries; z++)
                {
                    int midX = opts[z].x;
                    int midY = opts[z].y;
                    if (opts[z].type == OPTION_MENUTYPE)
                    {
                        sprintf(name_buff, "-> %s", opts[z].name);
                        if ( ((!opts[z].viewonly) && mouse_x > midX - text_length (font, name_buff) && mouse_x < midX && mouse_y >= midY && mouse_y < midY + 10)
                             || ((my_key == KEY_SPACE) && (selected_index == z)) )
                        {
                            int optsRetVal = options(global, env, (MENUDESC*) opts[z].value);
                            if (optsRetVal < 0)
                                return (optsRetVal + 1);

                            fi = 1;
                            for (z = 0; z < numEntries; z++)
                                updateoption[z] = 1;

                            my_key = selected_index = 0;
                        }
                    }
                    else if (opts[z].type == OPTION_ACTIONTYPE)
                    {
                        sprintf(name_buff, "-> %s", opts[z].name);
                        if ( ((!opts[z].viewonly) && mouse_x > midX - text_length(font, name_buff) && mouse_x < midX && mouse_y >= midY && mouse_y < midY + 10)
                             || ((my_key == KEY_SPACE) && (selected_index == z)) )
                        {
                            int (*action) (GLOBALDATA*, ENVIRONMENT*, void*) = (int (*)(GLOBALDATA*, ENVIRONMENT*, void*))opts[z].value;
                            int actionRetVal = action(global, env, opts[z].data);
                            if (actionRetVal)
                                return (actionRetVal);
                        }
                    }
                    else if (opts[z].type == OPTION_TEXTTYPE)
                    {
                        int my_text_length;
                        strcmp(opts[z].name, "Name") ? my_text_length = 11 : my_text_length = NAME_LENGTH;
                        int boxWidth;
                        if (!strcmp(opts[z].name, "Server address"))
                            my_text_length = ADDRESS_LENGTH;

                        if (my_text_length == NAME_LENGTH)
                            boxWidth = textEntryBox(global, env, FALSE, midX + 100, midY, (char*) opts[z].value, my_text_length);
                        else if (my_text_length == ADDRESS_LENGTH)
                            boxWidth = textEntryBox(global, env, FALSE, midX + 70, midY, (char*) opts[z].value, my_text_length);
                        else
                            boxWidth = textEntryBox (global, env, FALSE, midX + 50, midY, (char*)opts[z].value, my_text_length);
                        if ( ((!opts[z].viewonly) && mouse_x > midX - text_length(font, name_buff) && mouse_x < midX + 50 + boxWidth && mouse_y >= midY && mouse_y < midY + 10)
                             || ((selected_index == z) && (my_key == KEY_SPACE)) )
                        {
                            if (my_text_length == NAME_LENGTH)
                                textEntryBox(global, env, TRUE, midX + 100, midY, (char*) opts[z].value, my_text_length);
                            else if (my_text_length == ADDRESS_LENGTH)
                                textEntryBox(global, env, TRUE, midX + 70, midY, (char *) opts[z].value, my_text_length);
                            else
                                textEntryBox(global, env, TRUE, midX + 50, midY, (char*) opts[z].value, my_text_length);
                            updateoption[z] = 1;
                        }
                    }
                    else if (opts[z].type == OPTION_COLORTYPE)
                    {
                        if ((!opts[z].viewonly) && mouse_x > midX && mouse_x < midX + 100 && mouse_y >= midY && mouse_y < midY + 15)
                        {
                            *(int*)opts[z].value = pickColor(midX, midY, 100, 15, mouse_x, mouse_y);
                            updateoption[z] = 1;
                        }
                        colorBar(env, midX, midY, 100, 15);
                        rectfill(env->db, midX + 110, midY, midX + 130, midY + 10, *(int*)opts[z].value);
                        rect(env->db, midX + 110, midY, midX + 130, midY + 10, BLACK);
                    }
                    else if (opts[z].type == OPTION_TOGGLETYPE)
                    {
                        int tlen = text_length(font, name_buff);
                        int thgt = text_height(font);
                        if ( (mouseLeftPressed && (!opts[z].viewonly) && mouse_x > midX - tlen / 2 && mouse_x < midX + tlen / 2 && mouse_y >= midY && mouse_y < midY + thgt)
                             || ((my_key == KEY_SPACE) && (selected_index == z)) )
                        {
                            if (*opts[z].value == 0)
                                *opts[z].value = 1;
                            else
                                *opts[z].value = 0;
                            mouseLeftPressed = 1;
                            // updateoption[z] = 1;
                            // Crude, but hopefulyl useful
                            for (int temp_counter = 0; temp_counter < numEntries; temp_counter++)
                                updateoption[temp_counter] = 1;
                        }
                    }
                    else
                    {
                        if (!opts[z].viewonly)
                        {
                            if (mouse_x >= midX + 100 && mouse_x < midX + 110 && mouse_y >= midY && mouse_y < midY + 10)
                            {
                                if (mouse_b & 1)
                                    *opts[z].value -= opts[z].increment;
                                else if (mouse_b & 2)
                                    *opts[z].value -= opts[z].increment * 10;
                                updateoption[z] = 1;
                            }
                            if (mouse_x >= midX + 112 && mouse_x < midX + 122 && mouse_y >= midY && mouse_y < midY + 10)
                            {
                                if (mouse_b & 1)
                                    *opts[z].value += opts[z].increment;
                                else if (mouse_b & 2)
                                    *opts[z].value += opts[z].increment * 10;
                                updateoption[z] = 1;
                            }

                            if ((my_key == KEY_RIGHT) && (selected_index == z))
                            {
                                *opts[z].value += opts[z].increment;
                                updateoption[z] = 1;
                            }
                            else if ((my_key == KEY_LEFT) && (selected_index == z))
                            {
                                *opts[z].value -= opts[z].increment;
                                updateoption[z] = 1;
                            }

                            // This if block is a nasty hack to get the tank
                            // styles to redraw on the Players menu.
                            if (updateoption[z])
                            {
                                int my_counter;
                                for (my_counter = 0; my_counter < numEntries; my_counter++)
                                    updateoption[my_counter] = TRUE;
                                fi = TRUE;
                            }
                            /*if (mouse_x >= midX + 134 && mouse_x < midY + 154 && mouse_y >= midY && mouse_y < midY + 10) {
                            *opts[z].value = opts[z].defaultv;
                            updateoption[z] = 1;
                          }*/
                            if (*opts[z].value > opts[z].max)
                                *opts[z].value = opts[z].min;

                            if (*opts[z].value < opts[z].min)
                                *opts[z].value = opts[z].max;
                        }
                    }
                }
            }
            env->mouseclock++;
            if (env->mouseclock > 10)
                env->mouseclock = 0;
        }

        env->make_update(mouse_x, mouse_y, ((BITMAP *) (global->misc[0]))->w, ((BITMAP *) (global->misc[0]))->h);
        env->make_update(lx, ly, ((BITMAP *) (global->misc[0]))->w, ((BITMAP *) (global->misc[0]))->h);
        lx = mouse_x;
        ly = mouse_y;
        if (!global->os_mouse)
            show_mouse(NULL);

        // draw menu stuff
        if (fi)
        {
            drawMenuBackground(global, env, BACKGROUND_BLANK, rand(), 400);
            textout_ex(env->db, font, title, global->halfWidth - 3 - text_length(font, title), global->menuBeginY + 50, BLACK, -1);
            textout_ex(env->db, font, title, global->halfWidth - 5 - text_length(font, title), global->menuBeginY + 50, WHITE, -1);
            for (z = 0; z < numEntries; z++)
            {
                int midX = opts[z].x;
                int midY = opts[z].y;

                if (z == selected_index)
                    strcpy(my_pointer, "*");
                else
                    my_pointer[0] = '\0';

                if (opts[z].type == OPTION_TOGGLETYPE)
                {
                    int color = (*opts[z].value)?WHITE:BLACK;
                    int radius = text_length(font, opts[z].name) / 2;
                    int y_radius = text_height(font);
                    if (y_radius > 8)
                        y_radius = 8;
                    ellipsefill(env->db, midX, midY + text_height(font) / 2, radius, y_radius, color);
                    textout_ex(env->db, font, my_pointer, (midX - radius) - 40 , midY, WHITE, -1);
                }

                if (opts[z].displayFunc)
                {
                    if (opts[z].type == OPTION_TOGGLETYPE)
                        opts[z].displayFunc(env, midX + text_length (font, opts[z].name) / 2, midY, opts[z].data);
                    else if (opts[z].type == OPTION_MENUTYPE)
                    {
                        opts[z].displayFunc(env, midX, midY, opts[z].data);
                        sprintf(name_buff, "%s", my_pointer);
                        textout_ex(env->db, font, name_buff, midX - 125, midY, WHITE, -1);
                    }
                    else if (opts[z].type == OPTION_SPECIALTYPE)
                    {
                        opts[z].displayFunc(env, midX + text_length(font, opts[z].name) / 2, midY, opts[z].value );
                        sprintf(name_buff, "%s %s:", my_pointer, opts[z].name);
                        textout_ex(env->db, font, name_buff, midX - text_length(font, name_buff), midY, opts[z].color, -1);
                        if (!opts[z].viewonly)
                        {
                            draw_sprite_v_flip(env->db, (BITMAP *) global->misc[6], midX + 100, midY);
                            draw_sprite(env->db, (BITMAP *) global->misc[6], midX + 112, midY);
                        }
                    }
                }
                else if (opts[z].type == OPTION_MENUTYPE)
                {
                    sprintf(name_buff, "%s -> %s", my_pointer, opts[z].name);
                    textout_ex(env->db, font, name_buff, midX - text_length(font, name_buff), midY, opts[z].color, -1);
                }
                else if (opts[z].type == OPTION_ACTIONTYPE)
                {
                    sprintf(name_buff, "%s -> %s", my_pointer, opts[z].name);
                    textout_ex(env->db, font, name_buff, midX - text_length(font, name_buff), midY, opts[z].color, -1);
                }
                else if (opts[z].type == OPTION_TEXTTYPE)
                {
                    sprintf(name_buff, "%s %s:", my_pointer, opts[z].name);
                    textout_ex(env->db, font, name_buff, midX - text_length(font, name_buff), midY, opts[z].color, -1);
                }
                else if (opts[z].type == OPTION_COLORTYPE)
                {
                    sprintf(name_buff, "%s %s:", my_pointer, opts[z].name);
                    textout_ex(env->db, font, name_buff, midX - text_length(font, name_buff), midY, opts[z].color, -1);
                }
                else if (opts[z].type == OPTION_TOGGLETYPE)
                {
                    sprintf(name_buff, "%s %s", my_pointer, opts[z].name);
                    textout_centre_ex(env->db, font, name_buff, midX, midY, opts[z].color, -1);
                }
                else
                {
                    sprintf(name_buff, "%s %s:", my_pointer, opts[z].name);
                    textout_ex(env->db, font, name_buff, midX - text_length(font, name_buff), midY, opts[z].color, -1);
                    if (!opts[z].viewonly)
                    {
                        draw_sprite_v_flip(env->db, (BITMAP *) global->misc[6], midX + 100, midY);
                        draw_sprite(env->db, (BITMAP *) global->misc[6], midX + 112, midY);
                    }
                }

                if (my_pointer[0])
                    env->make_update(midX - 200, midY - 20, 400, 40);

            }    // end of for loop
            if (but_okay)
                but_okay->draw(env->db);
            if (but_quit)
                but_quit->draw(env->db);
            if (reset_button)
                reset_button->draw(env->db);
        }      // end of if fi

        for (z = 0; z < numEntries; z++)
        {
            int midX = opts[z].x;
            int midY = opts[z].y;
            if (updateoption[z])
            {
                updateoption[z] = 0;
                if (opts[z].type == OPTION_TOGGLETYPE)
                {
                    int color = (*opts[z].value) ? WHITE : BLACK;
                    int text_tall = text_height(font);
                    if (text_tall > 8)
                        text_tall = 8;
                    ellipsefill(env->db, midX, midY + text_height(font) / 2, text_length(font, opts[z].name) / 2, text_tall, color);
                }
                if (opts[z].displayFunc)
                {
                    if (opts[z].type == OPTION_TOGGLETYPE)
                        opts[z].displayFunc(env, midX + text_length (font, opts[z].name) / 2, midY, opts[z].data);
                    else if (opts[z].type == OPTION_MENUTYPE)
                        opts[z].displayFunc(env, midX, midY, opts[z].data);

                    env->make_update(midX - 200, midY - text_height(font), 450, 50);
                    env->do_updates();
                }
                else if (opts[z].type != OPTION_MENUTYPE && opts[z].type != OPTION_ACTIONTYPE)
                {
                    if (opts[z].type == OPTION_DOUBLETYPE)
                    {
                        sprintf(format_buff, opts[z].format, *opts[z].value);
                        textEntryBox(global, env, FALSE, midX + 50, midY, format_buff, 11);
                    }
                    else if (opts[z].type == OPTION_TEXTTYPE)
                        textEntryBox(global, env, FALSE, midX + 100, midY, (char*)opts[z].value, NAME_LENGTH);
                    else if (opts[z].type == OPTION_COLORTYPE)
                    {
                        colorBar(env, midX, midY, 100, 15);
                        rectfill(env->db, midX + 110, midY, midX + 130, midY + 10, *(int*)opts[z].value);
                        rect(env->db, midX + 110, midY, midX + 130, midY + 10, BLACK);
                    }
                    else if (opts[z].type == OPTION_TOGGLETYPE)
                    {
                        sprintf(format_buff, "%s", opts[z].name);
                        textout_centre_ex(env->db, font, format_buff, midX, midY, opts[z].color, -1);
                    }
                    else if (opts[z].specialOpts)
                        textEntryBox(global, env, FALSE, midX + 50, midY, opts[z].specialOpts[(int) *opts[z].value], 11);

                    env->make_update(midX - 100, midY - 2, 250, 20);
                }
            }
        }

        if (fi)
        {
            fi = 0;
            quickChange(global, env->db);
        }

        if ((but_quit && but_quit->isPressed()) || (my_key == KEY_ESC))
        {
            global->wr_lock_command();
            global->command = GLOBAL_COMMAND_MENU;
            global->unlock_command();
            stop = 1;
        }
        if ((but_okay && but_okay->isPressed()) || (my_key == KEY_ENTER))
            stop = 2;

        if ( reset_button && (reset_button->isPressed()) )
        {
            // RESET all options!
            env->Reset_Options();
            global->Reset_Options();
        }

        if (but_okay)
            but_okay->draw(env->db);
        if (but_quit)
            but_quit->draw(env->db);
        if (reset_button)
            reset_button->draw(env->db);

        if (!global->os_mouse)
            show_mouse(env->db);
        if (global->close_button_pressed)
            stop = 1;
        env->do_updates();
    }
    // while ((!keypressed()) && !stop);
    while (!stop);

    if (!stop)
        ke = readkey();
    else if (stop == 2)
        ke = KEY_ENTER << 8;
    else
        ke = KEY_ESC << 8;

    flush_inputs();

    if (but_quit)
        delete but_quit;
    if (but_okay)
        delete but_okay;

    if (reset_button)
        delete reset_button;

    free(updateoption);

    return ke;
}

int editPlayers(GLOBALDATA *global, ENVIRONMENT *env)
{
    int optionsRetVal;
    // int rows = (global->screenHeight - 400) / 15;
    int rows = (global->screenHeight - 2 * global->menuBeginY - 200) / 15;
    int columns = (global->numPermanentPlayers / rows) + 1;
    rows = (rows / columns) + 1;

    MENUENTRY *playersOpts;
    MENUDESC playersMenu;
    // playersOpts = new MENUENTRY[1 + global->numPermanentPlayers];
    playersOpts = (MENUENTRY *) calloc(global->numPermanentPlayers + 1, sizeof(MENUENTRY));
    if (!playersOpts)
    {
        perror("atanks.cpp: Failed allocating memory for playersOpts in editPlayers");
        return 0;
        // exit (1);
    }
    playersOpts[0].name = "Create New";
    playersOpts[0].displayFunc = NULL;
    playersOpts[0].color = WHITE;
    playersOpts[0].value = (double*) createNewPlayer;
    playersOpts[0].data = NULL;
    playersOpts[0].type = OPTION_ACTIONTYPE;
    playersOpts[0].viewonly = FALSE;
    playersOpts[0].x = global->halfWidth - 3;
    // playersOpts[0].y = global->halfHeight - 68 - 15;
    playersOpts[0].y = global->menuBeginY + 117;

    playersMenu.title = "Players";
    playersMenu.numEntries = 1 + global->numPermanentPlayers;
    playersMenu.entries = playersOpts;
    playersMenu.quitButton = TRUE;
    playersMenu.okayButton = FALSE;

    for (int count = 0; count < global->numPermanentPlayers; count++)
    {
        MENUENTRY *opt = &playersOpts[1 + count];

        opt->name = global->allPlayers[count]->getName();
        opt->displayFunc = displayPlayerName;
        opt->data = global->allPlayers[count];
        opt->color = global->allPlayers[count]->color;
        opt->value = (double*) global->allPlayers[count]->menudesc;
        opt->type = OPTION_MENUTYPE;
        opt->viewonly = FALSE;
        opt->x = global->halfWidth - (((count % columns) - (columns / 2)) * 90) - (((columns + 1) % 2) * 45);
        // opt->y = global->halfHeight - 68 + ((count / columns) * 15);
        opt->y = global->menuBeginY + 132 + ((count / columns) * 15);
    }
    optionsRetVal = options(global, env, &playersMenu);

    // delete playersOpts;
    free(playersOpts);

    return optionsRetVal;
}

int selectPlayers(GLOBALDATA *global, ENVIRONMENT *env)
{
    MENUENTRY roundOpt = { global->ingame->complete_text[1], NULL, WHITE, (double*)&global->rounds, NULL, "%2.0f", 1, MAX_ROUNDS, 1, 5, NULL,
                           OPTION_DOUBLETYPE, FALSE, global->halfWidth - 3, global->menuBeginY + 82};
    MENUENTRY gamename = { global->ingame->complete_text[2], NULL, WHITE, (double *) global->game_name, NULL, "%s", 0, 0, 0, 0, NULL,
                           OPTION_TEXTTYPE, FALSE, global->halfWidth - 3, global->menuBeginY + 100 };
    MENUENTRY loadgame, campaign;
    int save_game_exists;

    int optionsRetVal, z;
    // int rows = (global->screenHeight - 400) / 15;
    int rows = (global->screenHeight - 2 * global->menuBeginY - 200) / 15;
    int columns = (global->numPermanentPlayers / rows) + 1;
    int playerCount = 0;

    int number_saved_games = 0;
    struct dirent **saved_game_names;
    char **game_list = NULL;
    MENUENTRY *playersOpts;
    MENUDESC playersMenu;
    MENUENTRY load_game_entry;

    // find saved games
    saved_game_names = Find_Saved_Games(global, &number_saved_games);
    if (saved_game_names && number_saved_games)
    {
        int count;

        // move the names into a char list
        game_list = (char **) calloc(number_saved_games, sizeof(char *));
        for (count = 0; count < number_saved_games; count++)
        {
            game_list[count] = saved_game_names[count]->d_name;
            // clear trailign extension
            if (strchr(game_list[count], '.'))
                strchr(game_list[count], '.')[0] = '\0';
        }

        global->saved_game_list = game_list;
        // set up menu for selecting saved games
        load_game_entry.name = global->ingame->complete_text[3];
        load_game_entry.displayFunc = NULL;
        load_game_entry.color = WHITE;
        load_game_entry.value = (double *) &global->saved_game_index;
        load_game_entry.data = NULL;
        load_game_entry.format = "%s";
        load_game_entry.min = 0;
        load_game_entry.max = number_saved_games - 1;
        load_game_entry.increment = 1;
        load_game_entry.defaultv = 0;
        load_game_entry.specialOpts = global->saved_game_list;
        load_game_entry.type = OPTION_SPECIALTYPE;
        load_game_entry.viewonly = FALSE;
        load_game_entry.x = global->halfWidth;
        load_game_entry.y = global->menuBeginY + 120;
    }

    rows = (rows / columns) + 1;
    // playersOpts = new MENUENTRY[global->numPermanentPlayers + 4];
    playersOpts = (MENUENTRY *) calloc(global->numPermanentPlayers + 5, sizeof(MENUENTRY));
    if (!playersOpts)
    {
        perror("atanks.cpp: Failed allocating memory for playersOpts in selectPlayers");
        // exit (1);
    }

    loadgame.name = global->ingame->complete_text[4];
    loadgame.displayFunc = NULL;
    loadgame.data = &global->load_game;
    loadgame.color = WHITE;
    loadgame.value = (double *) &global->load_game;
    loadgame.type = OPTION_TOGGLETYPE;
    loadgame.viewonly = FALSE;
    loadgame.x = global->halfWidth - 50;
    loadgame.y = global->menuBeginY + 140;

    campaign.name = global->ingame->complete_text[5];
    campaign.displayFunc = NULL;
    campaign.data = &global->campaign_mode;
    campaign.color = WHITE;
    campaign.value = (double *) &global->campaign_mode;
    campaign.type = OPTION_TOGGLETYPE;
    campaign.viewonly = FALSE;
    campaign.x = global->halfWidth + 50;
    campaign.y = global->menuBeginY + 140;

    playersMenu.title = global->ingame->complete_text[0];
    playersMenu.numEntries = global->numPermanentPlayers + 5;
    playersMenu.entries = playersOpts;
    playersMenu.quitButton = TRUE;
    playersMenu.okayButton = TRUE;

    for (int count = 0; count < global->numPermanentPlayers; count++)
    {
        MENUENTRY *opt = &playersOpts[count];

        opt->name = global->allPlayers[count]->getName();
        opt->displayFunc = displayPlayerName;
        opt->data = global->allPlayers[count];
        opt->color = global->allPlayers[count]->color;
        opt->value = (double*)&global->allPlayers[count]->selected;
        opt->type = OPTION_TOGGLETYPE;
        opt->viewonly = FALSE;
        opt->x = global->halfWidth - (((count % columns) - (columns / 2)) * 90) - (((columns + 1) % 2) * 45);
        // opt->y = 265 + ( (count / columns) * 15 );
        opt->y = global->menuBeginY + 165 + ((count / columns) * 15);
    }
    memcpy(&playersOpts[global->numPermanentPlayers], &roundOpt, sizeof(MENUENTRY));
    memcpy(&playersOpts[global->numPermanentPlayers + 1], &gamename, sizeof(MENUENTRY));
    memcpy(&playersOpts[global->numPermanentPlayers + 2], &loadgame, sizeof(MENUENTRY));
    memcpy(&playersOpts[global->numPermanentPlayers + 3], &campaign, sizeof(MENUENTRY));
    if (number_saved_games && game_list)
        memcpy (&playersOpts[global->numPermanentPlayers + 4], &load_game_entry, sizeof(MENUENTRY));

    do
    {
        optionsRetVal = options(global, env, &playersMenu);
        if (global->load_game)
        {
            if ( (global->saved_game_list) && (global->saved_game_list[(int) global->saved_game_index ][0]) )
            {
                memset(global->game_name, '\0', GAME_NAME_LENGTH);
                strncpy(global->game_name, global->saved_game_list[(int) global->saved_game_index ], 16);
            }
        }

        if (optionsRetVal >> 8 == KEY_ENTER)
        {
            if (!global->load_game )    // trying to play a game
            {
                playerCount = 0;
                global->numPlayers = 0;
                for (z = 0; z < global->numPermanentPlayers; z++)
                {
                    if (global->allPlayers[z]->selected)
                    {
                        global->addPlayer(global->allPlayers[z]);
                        playerCount++;
                    }
                }
                if ((playerCount < 2) || (playerCount > MAXPLAYERS))
                {
                    if (playerCount < 2)
                        errorMessage = global->ingame->complete_text[8];
                    else if (playerCount > MAXPLAYERS)
                        errorMessage = global->ingame->complete_text[9];
                    errorX = global->halfWidth - text_length(font, errorMessage) / 2;
                    errorY = global->menuBeginY + 70;
                    optionsRetVal = 0;
                }
                else
                    optionsRetVal = PLAY_GAME;

            }    // end of trying to start a new game
            else     // start to load an existing game
            {
                save_game_exists = Check_For_Saved_Game(global);
                if (save_game_exists)
                    optionsRetVal = LOAD_GAME;
                else
                {
                    optionsRetVal = 0;
                    errorMessage = global->ingame->complete_text[39];
                    errorX  = global->halfWidth - text_length(font, errorMessage) / 2;
                    errorY = global->menuBeginY + 70;
                }
            }
        }
        // zero means an error occured.
        // keep running the loop until ESC is pressed or a non-zero value appears
    }
    // while (optionsRetVal == 0);
    while ((optionsRetVal >> 8 != KEY_ESC) && (optionsRetVal != PLAY_GAME) && (optionsRetVal != LOAD_GAME));

    // delete playersOpts;
    free(playersOpts);
    if (optionsRetVal >> 8 == KEY_ESC)
        optionsRetVal = ESC_MENU;

    if (saved_game_names)
        free(saved_game_names);

    return optionsRetVal;
}

void title(GLOBALDATA *global)
{
    if (!global->os_mouse)
        show_mouse(NULL);
    blit((BITMAP *) global->title[0], screen, 0, 0, global->halfWidth - 320, global->halfHeight - 240, 640, 480);
    if (!global->os_mouse)
        show_mouse(screen);
    clear_keybuf();

    //wait_for_input();

    if (!global->os_mouse)
        show_mouse(NULL);
}

int menu(GLOBALDATA *global, ENVIRONMENT *env)
{
    int ban, anclock, lb, updateplayers, done, updaterounds, z;
    int move_text;
    int seconds_idle = 0;
    int current_index = 0, max_index = 7;
    int my_key = 0;
    int bn = 0;    // button number
    int shift_menu = global->halfHeight - 240;
    if (shift_menu < 0) shift_menu = -shift_menu;
    else shift_menu = 0;

    move_text = 75;
    if (global->language == LANGUAGE_RUSSIAN)
        bn += MENUBUTTONS * 2;

    BUTTON but_play(global, env, global->halfWidth - move_text, global->halfHeight - 235 + shift_menu, NULL, (BITMAP*)global->button[bn], (BITMAP*)global->button[bn], (BITMAP*)global->button[bn+1]);
    bn += 2;
    BUTTON but_help(global, env, global->halfWidth - move_text, global->halfHeight - 185 + shift_menu, NULL, (BITMAP*)global->button[bn], (BITMAP*)global->button[bn], (BITMAP*)global->button[bn+1]);
    bn += 2;
    BUTTON but_options(global, env, global->halfWidth - move_text, global->halfHeight - 135 + shift_menu, NULL, (BITMAP*)global->button[bn], (BITMAP*)global->button[bn], (BITMAP*)global->button[bn+1]);
    bn += 2;
    BUTTON but_players(global, env, global->halfWidth - move_text, global->halfHeight - 85 + shift_menu, NULL, (BITMAP*)global->button[bn], (BITMAP*)global->button[bn], (BITMAP*)global->button[bn+1]);
    bn += 2;
    BUTTON but_credits(global, env, global->halfWidth - move_text, global->halfHeight - 35 + shift_menu, NULL, (BITMAP*)global->button[bn], (BITMAP*)global->button[bn], (BITMAP*)global->button[bn+1]);
    bn += 2;
    BUTTON but_quit(global, env, global->halfWidth - move_text, global->halfHeight + 65 + shift_menu, NULL, (BITMAP*)global->button[bn], (BITMAP*)global->button[bn], (BITMAP*)global->button[bn+1]);
    bn += 2;
    BUTTON but_network(global, env, global->halfWidth - move_text, global->halfHeight + 15 + shift_menu, NULL, (BITMAP*)global->button[bn], (BITMAP*)global->button[bn], (BITMAP*)global->button[bn+1]);

    BUTTON *button[MENUBUTTONS] = {&but_play, &but_help, &but_options, &but_players, &but_credits, &but_network, &but_quit};

    ban = -1;
    lock_cclock();
    cclock = global->updateCount = lx = ly = anclock = env->mouseclock = 0;
    unlock_cclock();
    lb = updateplayers = done = updaterounds = 0;
    fi = global->stopwindow = 1;
    while (!done)
    {
        if (global->Check_Time_Changed())
        {
            seconds_idle++;
            if (seconds_idle > DEMO_WAIT_TIME)
            {
                done = 1;
                global->wr_lock_command();
                global->command = GLOBAL_COMMAND_DEMO;
                global->unlock_command();
            }
        }

        // LINUX_SLEEP;
        LINUX_REST;
        while (get_cclock() > 0)
        {
            lock_cclock();
            cclock--;
            unlock_cclock();
            for (z = 0; z < MENUBUTTONS; z++)
            {
                if (button[z]->isMouseOver())
                {
                    if (ban > -1 && ban != z)
                    {
                        button[z]->draw(env->db);
                        //env->make_update (button[ban]->location.x, button[ban]->location.y, button[ban]->location.w, button[ban]->location.h);
                    }
                    ban = z;
                    break;
                }
            }
            if (!lb && mouse_b & 1)
                env->mouseclock = 0;
            lb = (mouse_b & 1) ? 1 : 0;
            if (mouse_b & 1)
            {
                global->wr_lock_command();
                for (z = 0; z < MENUBUTTONS; z++)
                {
                    if (button[z]->isPressed())
                    {
                        if (z == 0)
                            global->command = GLOBAL_COMMAND_PLAY;
                        else if (z == 1)
                            global->command = GLOBAL_COMMAND_HELP;
                        else if (z == 2)
                            global->command = GLOBAL_COMMAND_OPTIONS;
                        else if (z == 3)
                            global->command = GLOBAL_COMMAND_PLAYERS;
                        else if (z == 4)
                            global->command = GLOBAL_COMMAND_CREDITS;
                        else if (z == 5)
                            global->command = GLOBAL_COMMAND_NETWORK;
                        else if (z == 6)
                            global->command = GLOBAL_COMMAND_QUIT;
                        done = 1;
                    }
                }
                global->unlock_command();
            }
            env->mouseclock++;
            if (env->mouseclock > 10)
                env->mouseclock = 0;
        }
        if (updaterounds)
        {
            updaterounds = 0;
            env->make_update(global->halfWidth + 27, global->halfHeight + 198, 32, 32);
        }
        if (!global->os_mouse)
            show_mouse(NULL);
        if (fi)
        {
            draw_circlesBG(global, env->db, 0, 0, global->screenWidth, global->screenHeight, true);
            //textout(env->db, font, "Rounds: ", global->halfWidth - 45, global->halfHeight + 200, BLACK);
            //draw_sprite_v_flip (env->db, (BITMAP *) global->gfxData.M[6].dat, global->halfWidth - 60, global->halfHeight + 199);
            //draw_sprite(env->db, (BITMAP *) global->gfxData.M[6].dat, global->halfWidth + 64, global->halfHeight + 199);
            for (z = 0; z < MENUBUTTONS; z++)
            {
                button[z]->draw (env->db);
                if (z == current_index)
                {
                    // int delta_width = (global->language == LANGUAGE_RUSSIAN) ? 20 : 0;
                    // if (global->language == LANGUAGE_GERMAN) delta_width = 20;
                    // draw rectangle around selected option
                    rect(env->db, global->halfWidth - move_text - 8,
                         global->halfHeight - 240 + (50 * current_index) + shift_menu,
                         global->halfWidth + move_text + 8, // - delta_width,
                         global->halfHeight - 190 + (50 * current_index) + shift_menu, YELLOW);
                }
            }
        }
        if (ban > -1)
        {
            button[ban]->draw(env->db);
            //env->make_update(button[ban]->location.x, button[ban]->location.y, button[ban]->location.w, button[ban]->location.h);
        }
        //rectfill(env->db, global->halfWidth + 27, global->halfHeight + 198, global->halfWidth + 59, global->halfHeight + 210, WHITE);
        //rect(env->db, global->halfWidth + 27, global->halfHeight + 198, global->halfWidth + 59, global->halfHeight + 210, BLACK);
        //textprintf_centre(env->db, font, global->halfWidth + 43, global->halfHeight + 200, BLACK, "%d", global->rounds);
        if (!global->os_mouse)
            show_mouse(env->db);
        env->make_update(mouse_x, mouse_y, ((BITMAP *) (global->misc[0]))->w, ((BITMAP *) (global->misc[0]))->h);
        env->make_update(lx, ly, ((BITMAP *) (global->misc[0]))->w, ((BITMAP *) (global->misc[0]))->h);
        lx = mouse_x;
        ly = mouse_y;

        // check for key press
        if (keypressed())
        {
            my_key = readkey();
            my_key = my_key >> 8;
            fi = 2;
        }

        if ((my_key == KEY_DOWN) || (my_key == KEY_S))
        {
            current_index++;
            if (current_index >= max_index)
                current_index = 0;
        }
        else if ((my_key == KEY_UP) || (my_key == KEY_W))
        {
            current_index--;
            if (current_index < 0)
                current_index = max_index - 1;
        }
        else if ((my_key == KEY_ENTER) || (my_key == KEY_ENTER_PAD) || (my_key == KEY_SPACE))
        {
            done = 1;
            global->wr_lock_command();
            if (current_index == 0)
                global->command = GLOBAL_COMMAND_PLAY;
            else if (current_index == 1)
                global->command = GLOBAL_COMMAND_HELP;
            else if (current_index == 2)
                global->command = GLOBAL_COMMAND_OPTIONS;
            else if (current_index == 3)
                global->command = GLOBAL_COMMAND_PLAYERS;
            else if (current_index == 4)
                global->command = GLOBAL_COMMAND_CREDITS;
            else if (current_index == 5)
                global->command = GLOBAL_COMMAND_NETWORK;
            else if (current_index == 6)
                global->command = GLOBAL_COMMAND_QUIT;
            global->unlock_command();

        }
        else if ((my_key == KEY_Q) || (my_key == KEY_ESC))
            return SIG_QUIT_GAME;

        my_key = 0;

        if (fi > 0)
        {
            fi--;
            change(global, env->db);
        }
        if (global->close_button_pressed)
            return SIG_QUIT_GAME;
        if ((global->update_string) && (global->update_string[0]))
        {
            textout_centre_ex(env->db, font, global->update_string, global->halfWidth - 20, 500, WHITE, -1);
            env->make_update(50, 450, 300, 50);
        }
        if (global->client_message)
        {
            textout_centre_ex(env->db, font, global->client_message, global->halfWidth - 20, 520, WHITE, -1);
            env->make_update(50, 450, 300, 100);
        }
        env->do_updates();
    }
    clear_keybuf();
    return SIG_OK;
}

void draw_text_in_box(ENVIRONMENT *env, BOX *region, char *text)
{
    char buffer[1024];
    unsigned int lineBegin;
    int lastSpace = 0;
    int lineCount;

    rectfill(env->db, region->x, region->y, region->w, region->h, makecol(0,0,128));
    rect(env->db, region->x, region->y, region->w, region->h, makecol(128,128,255));

    lineBegin = 0;
    lineCount = 0;
    while (lineBegin < strlen(text))
    {
        int charCount = 0;
        int buffCount = 0;
        do
        {
            buffer[buffCount] = text[lineBegin + charCount];
            buffer[buffCount+1] = 0;
            if (buffer[buffCount] == ' ')
                lastSpace = 0;

            else if (buffer[buffCount] == '\n')
            {
                lineCount++;
                charCount++;
                break;
            }
            lastSpace++;
            buffCount++;
            charCount++;
        }
        while (text[lineBegin + charCount] && (text_length (font, buffer) < region->w - 20));
        if ((lastSpace > 0) && (text_length (font, buffer) >= region->w - 20))
        {
            charCount -= lastSpace - 1;
            buffer[buffCount - lastSpace] = 0;
        }
        else
            buffer[buffCount] = 0;

        textout_ex(env->db, font, buffer, region->x + 5, region->y + (lineCount * text_height (font)) + 5, WHITE, -1);
        lineBegin = lineBegin + charCount;
        charCount = 0;
        lineCount++;
    }
    env->make_update(region->x, region->y, region->w, region->h);
}

void draw_buystuff(GLOBALDATA *global, ENVIRONMENT *env, PLAYER *pl)
{
    int z;
    env->make_update(0, 0, global->screenWidth, global->screenHeight);
    if (!global->os_mouse)
        show_mouse(NULL);

    draw_circlesBG(global, env->db, 0, 0, global->screenWidth, global->screenHeight, false);
    draw_sprite(env->db, (BITMAP *) global->misc[DONE_IMAGE], global->halfWidth - 100, global->screenHeight - 50);
    draw_sprite(env->db, (BITMAP *) global->misc[FAST_UP_ARROW_IMAGE], global->screenWidth - STUFF_BAR_WIDTH - 30, global->halfHeight - 50);
    draw_sprite(env->db, (BITMAP *) global->misc[UP_ARROW_IMAGE], global->screenWidth - STUFF_BAR_WIDTH - 30, global->halfHeight - 25);
    draw_sprite(env->db, (BITMAP *) global->misc[DOWN_ARROW_IMAGE], global->screenWidth - STUFF_BAR_WIDTH - 30, global->halfHeight);
    draw_sprite(env->db, (BITMAP *) global->misc[FAST_DOWN_ARROW_IMAGE], global->screenWidth - STUFF_BAR_WIDTH - 30, global->halfHeight + 25);
    drawing_mode(DRAW_MODE_TRANS, NULL, 0, 0);
    env->current_drawing_mode = DRAW_MODE_TRANS;

    for (z = 0; z < global->halfWidth - 200; z++)
    {
        set_trans_blender(0, 0, 0, (int) ((double) ((double) z / (global->halfWidth - 200)) * 240) + 15);
        vline(env->db, z, 0, 29, pl->color);
    }

    for (z = 0; z < global->halfWidth - 200; z++)
    {
        set_trans_blender(0, 0, 0, (int) ((double) ((double) z / (global->halfWidth - 200)) * 240) + 15);
        vline(env->db, (global->screenWidth-1) - z, 0, 29, pl->color);
    }

    solid_mode();
    env->current_drawing_mode = DRAW_MODE_SOLID;
    textout_ex(env->db, font, global->ingame->complete_text[14], 20, 420, WHITE, -1);
    textout_ex(env->db, font, global->ingame->complete_text[15], 20, 450, WHITE, -1);
    textout_ex(env->db, font, global->ingame->complete_text[16], 20, 465, WHITE, -1);

}

int btps;
void draw_weapon_list(GLOBALDATA *global, ENVIRONMENT *env, PLAYER *pl, int *trolley, int scroll, int pressed)
{
    int slot, zzz;
    double tempbtps = (global->screenHeight - 55) / STUFF_BAR_HEIGHT;
    int font_diff;

    (font == global->unicode) ? font_diff = 8 : font_diff = 0;

    // To be sure it rounds down
    btps = (int) tempbtps;
    if (tempbtps < btps)
        btps--;


    // go through all items and draw them on the screen with
    // the amount of items in the trolly
    for (slot = 1, zzz = scroll; (slot < btps) && (zzz < env->numAvailable); zzz++)
    {
        int itemNum = env->availableItems[zzz];
        draw_sprite(env->db, (BITMAP *)global->gfxData.stuff_bar[(pressed == itemNum)?1:0], global->screenWidth - STUFF_BAR_WIDTH, slot * STUFF_BAR_HEIGHT);

        draw_sprite(env->db, (BITMAP *) global->gfxData.stuff_icon_base, global->screenWidth - STUFF_BAR_WIDTH, (slot * STUFF_BAR_HEIGHT));
        draw_sprite(env->db, (BITMAP *) global->stock[itemNum], global->screenWidth - STUFF_BAR_WIDTH, (slot * STUFF_BAR_HEIGHT) - 5);
        env->make_update(global->screenWidth - STUFF_BAR_WIDTH, slot * STUFF_BAR_HEIGHT, STUFF_BAR_WIDTH, STUFF_BAR_HEIGHT + 5);

        if (itemNum >= WEAPONS)    /* Items part */
        {

            textout_ex(env->db, font, item[itemNum - WEAPONS].name,
                       global->screenWidth - STUFF_BAR_WIDTH + 45, (slot * STUFF_BAR_HEIGHT) + 5 - font_diff, BLACK, -1);
            // Amount in inventory
            textprintf_ex(env->db, font, global->screenWidth - STUFF_BAR_WIDTH + 45, (slot * STUFF_BAR_HEIGHT) + (STUFF_BAR_HEIGHT/2) - font_diff, BLACK, -1, "%s: %d", global->ingame->complete_text[40], pl->ni[itemNum - WEAPONS]);
            // Anything in the trolley
            if (trolley[itemNum] != 0)
            {
                int textCol;
                if (trolley[itemNum] > 0)
                    textCol = makecol(255,255,0);
                else
                    textCol = makecol(176,0,0);
                textprintf_ex(env->db, font, global->screenWidth - STUFF_BAR_WIDTH + 45 + text_length(font, "Qty. in inventory: ddd"), (slot * STUFF_BAR_HEIGHT) + (STUFF_BAR_HEIGHT/2) - font_diff, textCol, -1, "%+d", trolley[itemNum]);
            }
            sprintf(buf, "$%s", Add_Comma(item[itemNum - WEAPONS].cost));
            textout_ex(env->db, font, buf,
                       global->screenWidth - 45 - text_length(font, buf), (slot * STUFF_BAR_HEIGHT) + 5 - font_diff, BLACK, -1);
            sprintf(buf, "for %d", item[itemNum - WEAPONS].amt);
            textout_ex(env->db, font, buf,
                       global->screenWidth - 45 - text_length(font, buf), (slot * STUFF_BAR_HEIGHT) + (STUFF_BAR_HEIGHT/2) - font_diff, BLACK, -1);
        }
        else    /* Weapons part */
        {

            textout_ex(env->db, font, weapon[itemNum].name, global->screenWidth - STUFF_BAR_WIDTH + 45, (slot * STUFF_BAR_HEIGHT) + 5 - font_diff, BLACK, -1);
            textprintf_ex(env->db, font, global->screenWidth - STUFF_BAR_WIDTH + 45, (slot * STUFF_BAR_HEIGHT) + (STUFF_BAR_HEIGHT/2) - font_diff, BLACK, -1, "%s: %d", global->ingame->complete_text[40], pl->nm[itemNum]);
            // Anything in the trolley
            if (trolley[itemNum] != 0)
            {
                int textCol;
                if (trolley[itemNum] > 0)
                    textCol = makecol (255,255,0);
                else
                    textCol = makecol (176,0,0);
                textprintf_ex(env->db, font, global->screenWidth - STUFF_BAR_WIDTH + 45 + text_length (font, "Qty. in inventory: ddd"), (slot * STUFF_BAR_HEIGHT) + (STUFF_BAR_HEIGHT/2) - font_diff, textCol, -1, "%+d", weapon[itemNum].delay ? trolley[itemNum] / weapon[itemNum].delay : trolley[itemNum]);
            }
            sprintf(buf, "$%s", Add_Comma(weapon[itemNum].cost));
            textout_ex(env->db, font, buf,
                       global->screenWidth - 45 - text_length(font, buf), (slot * STUFF_BAR_HEIGHT) + 5 - font_diff, BLACK, -1);
            sprintf (buf, "for %d", weapon[itemNum].amt);
            textout_ex(env->db, font, buf, global->screenWidth - 45 - text_length(font, buf), (slot * STUFF_BAR_HEIGHT) + (STUFF_BAR_HEIGHT/2) - font_diff, BLACK, -1);
        }
        slot++;
    }

}

bool buystuff(GLOBALDATA *global, ENVIRONMENT *env)
{
    int pl, done;
    int updatename, pressed, scroll, lb, lastMouse_b;
    int hoverOver = 0, z, zz, zzz;
    char buf[50];
    int mouse_wheel_previous, mouse_wheel_current;
    int performed_save_game = FALSE;
    char description[1024];
    BOX area = {20, 60, 300, 400};
    int my_cclock;
    int item_index = 1;
    int my_key = 0;
    int cost, amt, inInv;
    int buy_count = 0;

    strcpy(description, " ");
    //avoid compiler warning via initalize
    lastMouse_b = 0;

    // check starting position of the mouse wheel
    mouse_wheel_previous = mouse_z;

    lock_cclock();
    global->updateCount = cclock = lb = env->mouseclock = 0;
    unlock_cclock();
    fi = global->stopwindow = updatename = scroll = 1;

    // before we do anything else, put a cap on money
    for (z = 0; z < global->numPlayers; z++)
    {
        if (global->players[z]->money > 1000000000)
            global->players[z]->money = 1000000000;
        if (global->players[z]->money < 0)
            global->players[z]->money = 0;
    }


    if (global->bIsGameLoaded)
        // after the first shopping loop the game isn't fresh anymore
#ifdef DEBUG
    {
        cout << endl << "===========================================" << endl;
        cout << "==          New or Loaded Game!          ==" << endl;
        cout << "===========================================" << endl << endl;
#endif // DEBUG
        global->bIsGameLoaded = false;
#ifdef DEBUG
    }
#endif // DEBUG
    else if (global->divide_money)
    {
        // This only applies if this is not a fresh/loaded game
        // And if players want to divide the money
        int iJediMoney = 0;
        int iJediCount = 0;
        int iSithMoney = 0;
        int iSithCount = 0;
        int iTeamFee = 0;

        for (z = 0; z < global->numPlayers; z++)
        {
/*
            if (global->players[z]->money > 1000000000)
                global->players[z]->money = 1000000000;
            if (global->players[z]->money < 0)
                global->players[z]->money = 0;
*/
            // Sum up team money:
            if (global->players[z]->team == TEAM_JEDI)
            {
                iTeamFee = (int) ((double) global->players[z]->money * 0.25);
                if (iTeamFee > MAX_TEAM_AMOUNT)
                    iTeamFee = MAX_TEAM_AMOUNT;
                iJediMoney += iTeamFee;
                global->players[z]->money -= iTeamFee;
                iJediCount++;
            }
            if (global->players[z]->team == TEAM_SITH)
            {
                iTeamFee = (int) ((double) global->players[z]->money * 0.25);
                if (iTeamFee > MAX_TEAM_AMOUNT)
                    iTeamFee = MAX_TEAM_AMOUNT;
                iSithMoney += iTeamFee;
                global->players[z]->money -= iTeamFee;
                iSithCount++;
            }
        }
#ifdef DEBUG
        cout << endl << "Jedi Count: " << iJediCount << " - SitH Count: " << iSithCount << endl;
#endif // DEBUG
        // Now apply the team money:
        if (iJediCount || iSithCount)
        {
#ifdef DEBUG
            if (iJediCount)
                printf("The Jedi summed up a pool of %13d bucks!\n", iJediMoney);
            if (iSithCount)
                printf("The Sith summed up a pool of %13d bucks!\n", iSithMoney);
#endif // DEBUG
            if (iJediCount)
                iJediMoney = (int) (((double) iJediMoney * 0.90) / (double) iJediCount);
            if (iSithCount)
                iSithMoney = (int) (((double) iSithMoney * 0.90) / (double) iSithCount);
#ifdef DEBUG
            if (iJediCount)
                printf("Every Jedi will receive %10d credits out of the pool!\n", iJediMoney);
            if (iSithCount)
                printf("Every Sith will receive %10d credits out of the pool!\n", iSithMoney);
#endif // DEBUG
            for (z = 0; z < global->numPlayers; z++)
            {
                if (global->players[z]->team == TEAM_JEDI)
                    global->players[z]->money += iJediMoney;
                if (global->players[z]->team == TEAM_SITH)
                    global->players[z]->money += iSithMoney;
            }
        }
    }

    env->generateAvailableItemsList();
    int iMaxBoost = 0;
    int iMaxScore = 0;
    for (pl = 0; pl < global->numPlayers; pl++)
    {
        int iCurrentBoostLevel = global->players[pl]->getBoostValue();
        if (iCurrentBoostLevel > iMaxBoost)
            iMaxBoost = iCurrentBoostLevel;
        if (global->players[pl]->score > iMaxScore)
            iMaxScore = global->players[pl]->score;
    }

    for (pl = 0; pl < global->numPlayers; pl++)
    {
        int money = global->players[pl]->money;
        int trolley[THINGS];
        memset(trolley, 0, sizeof(int) * THINGS);

        //have computer players buy stuff
        if ((int) global->players[pl]->type != HUMAN_PLAYER)
        {
            int pressed = 0;
            int moneyToSave = 0;    // How much money will the player save?
#ifdef DEBUG
            cout << "(" << global->players[pl]->getName() << ") Starting to buy:" << endl;
            if (global->players[pl]->defensive < -0.9)
                cout << "(True Offensive)" << endl;
            if ((global->players[pl]->defensive >=-0.9) && (global->players[pl]->defensive < -0.75))
                cout << "(Very Offensive)" << endl;
            if ((global->players[pl]->defensive >=-0.75) && (global->players[pl]->defensive < -0.25))
                cout << "(Offensive)" << endl;
            if ((global->players[pl]->defensive >=-0.25) && (global->players[pl]->defensive < 0.00))
                cout << "(Slightly Offensive)" << endl;
            if (global->players[pl]->defensive == 0.0)
                cout << "(Neutral)" << endl;
            if ((global->players[pl]->defensive >0.0) && (global->players[pl]->defensive <= 0.25))
                cout << "(Slightly Defensive)"<< endl;
            if ((global->players[pl]->defensive >0.25) && (global->players[pl]->defensive <= 0.75))
                cout << "(Defensive)" << endl;
            if ((global->players[pl]->defensive >0.75) && (global->players[pl]->defensive <= 0.9))
                cout << "(Very Defensive)" << endl;
            if (global->players[pl]->defensive > 0.9)
                cout << "(True Defensive)" << endl;
            cout << "Inventory:" << endl <<  "----------" << endl;
            for (int i = 1; i < WEAPONS; i++)
            {
                if (global->players[pl]->nm[i])
                    cout << global->players[pl]->nm[i] << " x " << weapon[i].name << endl;
            }
            cout << " - - - - - - -" << endl;
            for (int i = 1; i < ITEMS; i++)
            {
                if (global->players[pl]->ni[i])
                    cout << global->players[pl]->ni[i] << " x " << item[i].name << endl;
            }
            cout << "----------" << endl;
#endif // DEBUG
            // moneysaving will be made possible when:
            // 1. It's not the first three rounds
            // 2. It's not the last 5 rounds
            // and, dynamically:
            // 3. We have at least 10 parachutes  or no gravity
            // 4. We have at least 5 damage dealing (not small missile) weapons
            // 5. We have a sum of 50 slick/dimpled projectiles

            if ((global->currentround > 5) && (((int) global->rounds - global->currentround) > 3))
            {
                moneyToSave = global->players[pl]->getMoneyToSave();
#ifdef DEBUG
                cout << "Maximum Money to save: " << moneyToSave << " (I have: " << global->players[pl]->money << ")" << endl;
#endif //DEBUG
            }
#ifdef DEBUG
            else
                cout << "No money to save this round!!!" << endl;
#endif // DEBUG
            // Check for a minimum of damage dealing weapons and parachutes, then buy until moneyToSave is reached
            buy_count = 0;
            do
            {
                int numPara = global->players[pl]->ni[ITEM_PARACHUTE];    // How many parachutes do we have?
                int numProj = global->players[pl]->ni[ITEM_SLICKP] + global->players[pl]->ni[ITEM_DIMPLEP];    // How many slick/dimpled projectiles do we have?
                int numDmgWeaps = 0;    // How many damage dealing weapons apart from small missiles do they have?

                for (int i = 1; i < WEAPONS; i++)
                {
                    // start from 1, as 0 is the small missile
                    if (weapon[i].damage > 0)
                        numDmgWeaps += global->players[pl]->nm[i];
                }

                if ((global->players[pl]->money > moneyToSave) || ((numPara < 10) && (env->landSlideType > LANDSLIDE_NONE))
                    || (numDmgWeaps < 8) || (numProj < 50))
                    pressed = global->players[pl]->chooseItemToBuy(iMaxBoost);
                else
                    pressed = -1;

#ifdef DEBUG
                if (pressed > -1)
                {
                    cout << "I have bought: ";
                    if (pressed < WEAPONS)
                        cout << weapon[pressed].name;
                    else
                        cout << item[pressed - WEAPONS].name;
                    cout << " (" << global->players[pl]->money << " bucks left)" << endl;
                }
                else
                    cout << "Finished, with " << global->players[pl]->money << " Credits left!" << endl;
#endif // DEBUG
                buy_count++;
            }
            while ((pressed != -1) && (buy_count < 100));

#ifdef DEBUG
            cout << "============================================" << endl << endl;
#endif //DEBUG
            continue;  //go to next player
        }        // end of AI player

        done = 0;
        updatename = scroll = 1;
        pressed = -1;

        draw_buystuff(global, env, global->players[pl]);

        while (!done)
        {

            LINUX_SLEEP;
            my_cclock = CLOCK_MAX;
            while (my_cclock > 0)
            {
                LINUX_SLEEP;
                if (global->close_button_pressed)
                {
                    clear_keybuf();
                    return false;
                }
                my_cclock--;
                if (!lb && mouse_b & 1 && mouse_x >= global->halfWidth - 100 && mouse_x < global->halfWidth + 100 && mouse_y >= global->screenHeight - 50 && mouse_y < global->screenHeight - 25)
                    done = 1;
                if (!lb && mouse_b & 1)
                    env->mouseclock = 0;
                lb = (mouse_b & 1) ? 1 : 0;

                my_key = 0;
                //Keyboard control
                if (keypressed())
                {
                    my_key = readkey();
                    my_key = my_key >> 8;
                }

                if  (my_key == KEY_UP || my_key == KEY_W) // && (scroll > 1)
                    //  && (!env->mouseclock))
                {
                    if (item_index > 1)
                        item_index--;
                    if (item_index < scroll)
                        scroll = item_index;
                }
                if (my_key == KEY_PGUP || my_key == KEY_R) // && (scroll > 1)
                    // && (!env->mouseclock))
                {
                    item_index -= btps - 1;
                    if (item_index < 1)
                        item_index = 1;
                    if (item_index < scroll)
                        scroll = item_index;
                }

                if (my_key == KEY_DOWN || my_key == KEY_S)
                    //    && (scroll <= env->numAvailable - btps)
                    //    && (!env->mouseclock))
                {
                    if (item_index < (env->numAvailable - 1))
                        item_index++;
                    if (item_index - scroll >= (btps - 1))
                        scroll = item_index - (btps - 2);
                }
                if ((my_key == KEY_PGDN || my_key == KEY_F) && (scroll <= env->numAvailable - btps + 1))
                    //  && (!env->mouseclock))
                {
                    item_index += btps - 1;
                    if (item_index > env->numAvailable - 1)
                        item_index = env->numAvailable - 1;
                    if (item_index - scroll >= (btps - 1))
                        scroll = item_index - (btps - 2);
                }

                // make sure the selected item is on the visible screen
                if (item_index < scroll)
                    item_index = scroll;
                else if ( item_index > (scroll + btps + 3) )
                    item_index = scroll + btps + 3;

                // buy or sell an item
                if (my_key == KEY_RIGHT || my_key == KEY_D)
                {
                    pressed = env->availableItems[item_index];
                    if (pressed >= WEAPONS)
                    {
                        cost = item[pressed - WEAPONS].cost;
                        amt = item[pressed - WEAPONS].amt;
                        inInv = global->players[pl]->ni[pressed - WEAPONS];
                    }
                    else
                    {
                        cost = weapon[pressed].cost;
                        amt = weapon[pressed].amt;
                        inInv = global->players[pl]->nm[pressed];
                    }
                    if ( (money >= cost) && ((inInv + trolley[pressed]) < (999 - amt)) )
                    {
                        if (trolley[pressed] <= -amt)
                        {
                            if (global->sellpercent > 0.01)
                            {
                                money -= (int) (cost * global->sellpercent);
                                trolley[pressed] += amt;
                                updatename = 1;
                            }
                        }
                        else
                        {
                            money -= cost;
                            trolley[pressed] += amt;
                            updatename = 1;
                            if (inInv + trolley[pressed] > 999)
                                trolley[pressed] = 999;
                        }
                    }
                    pressed = -1;
                }    // end of buying

                if (my_key == KEY_LEFT || my_key == KEY_A)
                {
                    pressed = env->availableItems[item_index];
                    if (pressed >= WEAPONS)
                    {
                        cost = item[pressed - WEAPONS].cost;
                        amt = item[pressed - WEAPONS].amt;
                        inInv = global->players[pl]->ni[pressed - WEAPONS];
                    }
                    else
                    {
                        cost = weapon[pressed].cost;
                        amt = weapon[pressed].amt;
                        inInv = global->players[pl]->nm[pressed];
                    }
                    if (inInv + trolley[pressed] >= amt)
                    {
                        if (trolley[pressed] >= amt)
                        {
                            money += cost;
                            trolley[pressed] -= amt;
                            updatename = 1;
                        }
                        else
                        {
                            if (global->sellpercent > 0.01)
                            {
                                money += (int) (cost * global->sellpercent);
                                trolley[pressed] -= amt;
                                updatename = 1;
                            }
                        }
                    }
                    pressed = -1;
                }           // end of selling


                // check for adding or removing rounds
                if ((my_key == KEY_PLUS_PAD) || (my_key == KEY_EQUALS))
                {
                    if ((global->rounds < 999) && (!env->mouseclock))
                    {
                        global->rounds++;
                        global->currentround++;
                        updatename = 1;
                    }
                }
                if ((my_key == KEY_MINUS_PAD) || (my_key == KEY_MINUS))
                {
                    if ((global->rounds > 1) && (global->currentround > 1) && (!env->mouseclock))
                    {
                        global->rounds--;
                        global->currentround--;
                        updatename = 1;
                    }
                }

                // check for saving the game
                if (my_key == KEY_F10)
                {
                    if (!performed_save_game)
                    {
                        int status = Save_Game(global, env);
                        performed_save_game = TRUE;
                        if (status)
                            snprintf(description, 64, "%s \"%s\".", global->ingame->complete_text[17], global->game_name);
                        else
                            strcpy(description, global->ingame->complete_text[41]);
                        draw_text_in_box(env, &area, description);
                    }
                }

                if (my_key == KEY_ENTER)
                    done = TRUE;

                // Mouse control

                // check mouse wheel
                mouse_wheel_current = mouse_z;
                if (mouse_wheel_current < mouse_wheel_previous)
                {
                    scroll++;
                    if (scroll > env->numAvailable - btps + 1)
                        scroll = env->numAvailable - btps + 1;
                    if (scroll > item_index)
                        item_index = scroll;
                }
                else if (mouse_wheel_current > mouse_wheel_previous)
                {
                    scroll--;
                    if (scroll < 1)
                        scroll = 1;
                    if (item_index > scroll + btps)
                        item_index = scroll + btps - 3;
                }
                mouse_wheel_previous = mouse_wheel_current;


                if (mouse_x >= global->screenWidth - STUFF_BAR_WIDTH && mouse_x < global->screenWidth)
                {
                    int newlyOver;
                    zz = 0;
                    for (z = 1, zzz = scroll; z < btps; z++, zzz++)
                    {
                        if (mouse_y >= z * STUFF_BAR_HEIGHT && mouse_y < (z * STUFF_BAR_HEIGHT) + 30)
                        {
                            zz = 1;
                            break;
                        }
                    }
                    if (zz)
                        newlyOver = env->availableItems[zzz];
                    else
                        newlyOver = -1;
                    if (hoverOver != newlyOver)
                    {
                        // char description[1024];
                        // BOX area = {20, 60, 300, 400};

                        if (newlyOver > -1)
                        {
                            if (newlyOver < WEAPONS)
                            {
                                WEAPON *weap = &weapon[newlyOver];
                                sprintf(description, "Radius: %d\nYield: %ld\n\n%s",
                                        weap->radius, calcTotalPotentialDamage(newlyOver) * weap->spread, weap->description);
                            }
                            else
                            {
                                int itemNum = newlyOver - WEAPONS;
                                ITEM *it = &item[itemNum];
                                if (itemNum >= ITEM_VENGEANCE && itemNum <= ITEM_FATAL_FURY)
                                    sprintf(description, "Potential Damage: %ld\n\n%s",
                                            calcTotalPotentialDamage ((int)it->vals[0]) * (int)it->vals[1], it->description);
                                else
                                    sprintf (description, "%s", it->description);

                            }
                        }
                        else
                            description[0] = 0;

                        hoverOver = newlyOver;

                        draw_text_in_box(env, &area, description);
                    }
                }
                if (mouse_b & 1 && !env->mouseclock)
                {
                    int scrollArrowPos = global->screenWidth - STUFF_BAR_WIDTH - 30;
                    if (mouse_x >= scrollArrowPos && mouse_x < scrollArrowPos + 24)
                    {
                        if ((mouse_y >= global->halfHeight - 50 && mouse_y < global->halfHeight - 25) && (scroll > 1))
                        {
                            scroll -= btps / 2;
                            if (scroll < 1)
                                scroll = 1;
                        }
                        if ((mouse_y >= global->halfHeight - 24 && mouse_y < global->halfHeight) && (scroll > 1))
                            scroll--;
                        if ((mouse_y >= global->halfHeight + 1 && mouse_y < global->halfHeight + 25) && (scroll <= env->numAvailable - btps))
                            scroll++;
                        if ((mouse_y >= global->halfHeight + 25 && mouse_y < global->halfHeight + 50) && (scroll <= env->numAvailable - btps + 1))
                        {
                            scroll += btps / 2;
                            if (scroll > env->numAvailable - btps + 1)
                                scroll = env->numAvailable - btps + 1;
                        }
                    }
                    if (item_index < scroll)
                        item_index = scroll;
                    else if (item_index > (scroll + btps))
                        item_index = scroll + btps - 3;
                }
                if (mouse_b & 1 || mouse_b & 2)
                {
                    int itemButtonClicked = 0;
                    for (int buttonCount = 1, currItem = scroll; buttonCount < btps; buttonCount++, currItem++)
                    {
                        if (mouse_x >= global->screenWidth - STUFF_BAR_WIDTH && mouse_x < global->screenWidth && mouse_y >= buttonCount * STUFF_BAR_HEIGHT && mouse_y < (buttonCount * STUFF_BAR_HEIGHT) + 30)
                        {
                            itemButtonClicked = 1;
                            // Remember which button was pressed
                            pressed = env->availableItems[currItem];
                        }
                    }
                    if (!itemButtonClicked)
                        pressed = -1;
                }
                if (pressed > -1 && !(mouse_b & 1 || mouse_b & 2))
                {
                    // Cost, amount and in-inventory amount
                    // of pressed item
                    // int cost,amt,inInv;
                    bool control_key = false;

                    if ((key[KEY_LCONTROL]) || (key[KEY_RCONTROL]))
                        control_key = true;

                    if (pressed > WEAPONS - 1)
                    {
                        cost = item[pressed - WEAPONS].cost;
                        amt = item[pressed - WEAPONS].amt;
                        inInv = global->players[pl]->ni[pressed - WEAPONS];
                    }
                    else
                    {
                        cost = weapon[pressed].cost;
                        amt = weapon[pressed].amt;
                        inInv = global->players[pl]->nm[pressed];
                    }

                    if (control_key)
                    {
                        cost *= 10;
                        amt *= 10;
                    }

                    if (lastMouse_b & 2)
                    {
                        if (inInv + trolley[pressed] >= amt)
                        {
                            if (trolley[pressed] >= amt)
                            {
                                money += cost;
                                trolley[pressed] -= amt;
                                updatename = 1;
                            }
                            else
                            {
                                if (global->sellpercent > 0.01)
                                {
                                    money += (int) (cost * global->sellpercent);
                                    trolley[pressed] -= amt;
                                    updatename = 1;
                                }
                            }
                        }
                    }
                    else
                    {
                        if ( (money >= cost) && ((inInv + trolley[pressed]) < (999 - amt)) )
                        {
                            if (trolley[pressed] <= -amt)
                            {
                                if (global->sellpercent > 0.01)
                                {
                                    money -= (int) (cost * global->sellpercent);
                                    trolley[pressed] += amt;
                                    updatename = 1;
                                }
                            }
                            else
                            {
                                money -= cost;
                                trolley[pressed] += amt;
                                updatename = 1;
                                if (inInv + trolley[pressed] > 999)
                                    trolley[pressed] = 999;
                            }
                        }
                    }
                    pressed = -1;
                }
                env->mouseclock++;
                if (env->mouseclock > 5)
                    env->mouseclock = 0;
                lastMouse_b = mouse_b;
            }
            if (!global->os_mouse)
                show_mouse(NULL);
            if (updatename)
            {
                updatename = 0;
                // env->make_update(global->halfWidth - 315, 0, 400, 30);
                env->make_update(global->halfWidth - 315, 0, global->screenWidth - 1, 30);
                draw_sprite(env->db, (BITMAP *) global->gfxData.stuff_bar[0], global->halfWidth - 200, 0);
                textprintf_ex(env->db, font, global->halfWidth - 190, 5, BLACK, -1, "%s %d: %s", global->ingame->complete_text[10], pl + 1, global->players[pl]->getName ());
                // textprintf_ex(env->db, font, global->halfWidth - 190, 17, BLACK, -1, "Money: $%d", money);
                textprintf_ex(env->db, font, global->halfWidth - 190, 17, BLACK, -1, "%s: $%s", global->ingame->complete_text[11], Add_Comma(money));
                sprintf(buf, "%s: %d/%d", global->ingame->complete_text[12], (int) (global->rounds - global->currentround) + 1, (int)global->rounds);
                textout_ex(env->db, font, buf, global->halfWidth + 170 - text_length (font, buf), 5, BLACK, -1);
                sprintf(buf, "%s: %d", global->ingame->complete_text[13], global->players[pl]->score);
                textout_ex(env->db, font, buf, global->halfWidth + 155 - text_length (font, buf), 17, BLACK, -1);
            }

            draw_weapon_list(global, env, global->players[pl], trolley, scroll, (pressed < 0) ? env->availableItems[item_index] : pressed );
            env->make_update(mouse_x, mouse_y, ((BITMAP *) (global->misc[0]))->w, ((BITMAP *) (global->misc[0]))->h);
            env->make_update(lx, ly, ((BITMAP *) (global->misc[0]))->w, ((BITMAP *) (global->misc[0]))->h);
            lx = mouse_x;
            ly = mouse_y;
            if (!global->os_mouse)
                show_mouse(env->db);
            if (fi)
            {
                change(global, env->db);
                fi = 0;
            }
            else
                env->do_updates();
        }
        for (int tItem = 0; tItem < WEAPONS; tItem++)
            global->players[pl]->nm[tItem] += trolley[tItem];
        for (int tItem = WEAPONS; tItem < THINGS; tItem++)
            global->players[pl]->ni[tItem - WEAPONS] += trolley[tItem];
        global->players[pl]->money = money;
    }
    // clear_keybuf ();

    for (z = 0; z < global->numPlayers; z++)
    {
        int iMoney = global->players[z]->money;
        int iLevel = 0;
        int iInterSum = 0;    // The summed up interest
#ifdef DEBUG
        cout << endl << "======================================================" << endl;
        printf("%2d.: %s enters the bank to get interest:\n", (z+1), global->players[z]->getName());
        printf("     Starting Account: %10d\n", global->players[z]->money);
        cout << "------------------------------------------------------" << endl;
#endif // DEBUG
        while (iMoney && (iLevel < 5))
        {
            // Enter next level
            iLevel++;
            float fIntPerc = (global->interest - 1.0) / iLevel;
            int iInterest = (int) ((float) iMoney * fIntPerc);
            // The limit is only applicable on the first four levels, in the fifth level interest is fully applied!
            if ((iInterest > MAX_INTEREST_AMOUNT) && (iLevel < 5))
                iInterest = MAX_INTEREST_AMOUNT;
            // Now sum the interest up and substract the counted money!
            iInterSum += iInterest;
            iMoney -= (int) ((float) iInterest / fIntPerc);
#ifdef DEBUG
            printf("     Level %1d:  %8d credits are rated,\n", iLevel, (int)(iInterest / fIntPerc));
            printf("     Interest: %8d credits. (%5.2f%%)\n", iInterest, (fIntPerc * 100));
#endif // DEBUG
            // To get rid of (possible) rounding errors, add a security check:
            if ((iMoney < (4 * iLevel)) || (iInterest < 1))
                iMoney = 0;    // With less there won't be any more interest anyway!
#ifdef DEBUG
            printf("     Unrated : %8d credits left.\n", iMoney);
#endif // DEBUG
        }
        // Now giv'em their money:
#ifdef DEBUG
        printf("     Sum:      %8d credits.\n", iInterSum);
        cout << "------------------------------------------------------" << endl;
#endif // DEBUG
        global->players[z]->money += iInterSum;
#ifdef DEBUG
        printf("     Final Account   : %10d\n", global->players[z]->money);
        cout << "======================================================" << endl;
#endif // DEBUG
    }
    return true;
}

#ifdef GETV_IS_EVER_USED
double getv(int color)
{
    float h, s, v;
    int r, g, b;

    r = getr(color);
    g = getg(color);
    b = getb(color);
    rgb_to_hsv(r, g, b, &h, &s, &v);

    return v;
}
#endif //GETV_IS_EVER_USED

void set_level_settings(GLOBALDATA *global, ENVIRONMENT *env)
{
    int taken[MAXPLAYERS];
    int chosen = 0, chosenCount = 0, peak_height = 0;
    int z, zz;
    int objCount;
    TANK *ltank;

    // srand(time(NULL));

    if (!global->os_mouse)
        show_mouse(NULL);
    draw_sprite(screen, (BITMAP *) global->misc[1], global->halfWidth - 120, global->halfHeight + 115);
    textout_centre_ex(screen, font, global->ingame->complete_text[42], global->halfWidth, global->halfHeight + 120, WHITE, -1);

    // Choose appropriate gradients for sky and land
    while ((chosenCount < 60) && !chosen)
    {
        BITMAP *sky_gradient_strip, *land_gradient_strip;

        global->lock_curland();
        global->curland = rand() % LANDS;
        global->unlock_curland();
        if (global->colour_theme == COLOUR_THEME_CRISPY)
        {
            global->lock_curland();
            global->curland += LANDS;
            global->unlock_curland();
        }
        global->lock_curland();
        if (!global->gfxData.land_gradient_strips[global->curland])
            global->gfxData.land_gradient_strips[global->curland] = create_gradient_strip(land_gradients[global->curland], (global->screenHeight - MENUHEIGHT));
        land_gradient_strip = global->gfxData.land_gradient_strips[global->curland];
        global->unlock_curland();

        global->cursky = rand() % SKIES;
        if (global->colour_theme == COLOUR_THEME_CRISPY)
            global->cursky += SKIES;
        if (!global->gfxData.sky_gradient_strips[global->cursky])
            global->gfxData.sky_gradient_strips[global->cursky] = create_gradient_strip(sky_gradients[global->cursky], (global->screenHeight - MENUHEIGHT));
        sky_gradient_strip = global->gfxData.sky_gradient_strips[global->cursky];

        chosen = 1;
        for (z = 0; z < global->screenWidth; z++)
            if (peak_height < env->height[z])
                peak_height = (int)env->height[z];
        for (z = 0; z <= peak_height; z++)
        {
            int skyi, landi;
            double distance;
            skyi = getpixel(sky_gradient_strip, 0, (global->screenHeight - MENUHEIGHT - z));
            landi = getpixel(land_gradient_strip, 0, z);

            distance = colorDistance(skyi, landi);
            if (distance < 30)
                chosen = 0;
        }
        chosenCount++;
    }

    if (!global->os_mouse)
        show_mouse(NULL);
    draw_sprite(screen, (BITMAP *) global->misc[1], global->halfWidth - 120, global->halfHeight + 155);
    textout_centre_ex(screen, font, global->ingame->complete_text[43], global->halfWidth, global->halfHeight + 160, WHITE, -1);
    // It looks like we do not use this anymore  xoffset = rand();
    //generate_sky (global, env, xoffset, 0, global->screenWidth, global->screenHeight);

    if (env->sky)
    {
        destroy_bitmap(env->sky);
        env->sky = NULL;
    }
    // see if we want a custom background
    if ((env->custom_background) && (env->bitmap_filenames))
    {
        // if ( env->sky) destroy_bitmap(env->sky);
        env->sky = load_bitmap(env->bitmap_filenames[rand() % env->number_of_bitmaps], NULL);
    }

    // if we do not have a custom background (or do not want one) create a new background
    if ((!env->custom_background) || (!env->sky))
    {
        // if (env->sky) destroy_bitmap(env->sky);
#ifdef THREADS
        // On Linux we will have a thread creating a sky for us in the background
        // to avoid wait times. If there is not a pre-created sky waiting for us
        // then fall back to generating one the regular way.
        if (env->get_waiting_sky())
        {
            env->sky = env->get_waiting_sky();
            env->lock(env->waiting_sky_lock);
            env->waiting_sky = NULL;
            env->unlock(env->waiting_sky_lock);
        }
        else
        {
#endif
            env->sky = create_bitmap(global->screenWidth, global->screenHeight - MENUHEIGHT);
            generate_sky(global, env->sky, sky_gradients[global->cursky],
                         (global->ditherGradients ? GENSKY_DITHERGRAD : 0 ) | (global->detailedSky ? GENSKY_DETAILED : 0 ));
#ifdef THREADS
        }
#endif
    }

    if (!global->os_mouse)
        show_mouse(NULL);
    draw_sprite(screen, (BITMAP *) global->misc[1], global->halfWidth - 120, global->halfHeight + 195);
    textout_centre_ex(screen, font, global->ingame->complete_text[44], global->halfWidth, global->halfHeight + 200, WHITE, -1);

#ifdef THREADS
    // we have threads, so check for pre-main terrain
    if (env->get_waiting_terrain())
    {
        // we have one waiting, so destroy the current one
        if (env->terrain)
            destroy_bitmap(env->terrain);
        env->terrain = env->get_waiting_terrain();
        env->lock(env->waiting_terrain_lock);
        env->waiting_terrain = NULL;    // set this so a new one will be made
        env->unlock(env->waiting_terrain_lock);
    }
    else    // one is not waiting, so we need to create one now
    {
#endif

        clear_to_color(env->terrain, PINK);

        int xoffset = rand();
        generate_land(global, env, env->terrain, xoffset, 0, global->screenHeight);
#ifdef THREADS
    }    // end of else
#endif

    for (z = 0; z < global->numTanks; z++)
        taken[z] = 0;

    for (objCount = 0; (ltank = (TANK*)env->getNextOfClass (TANK_CLASS, &objCount)) && ltank; objCount++)
    {
        for (zz = rand() % global->numTanks; taken[zz]; zz = rand() % global->numTanks) { }
        taken[zz] = objCount + 1;
        ltank->x = (zz + 1) * (global->screenWidth / (global->numTanks + 1));
        ltank->y = (global->screenHeight - (int) env->height[(int) ltank->x]) - (TANKHEIGHT - TANKSAG);
        ltank->newRound ();
    }
    for (z = 0; z < MAXPLAYERS; z++)
    {
        env->order[z] = NULL;
        env->playerOrder[z] = NULL;
    }

    global->maxNumTanks = global->numTanks;
    for (objCount = 0; (ltank = (TANK*)env->getNextOfClass (TANK_CLASS, &objCount)) && ltank; objCount++)
    {
        for (z = rand() % global->numTanks; env->order[z]; z = rand() % global->numTanks) { }
        env->order[z] = ltank;
    }
    // if (global->turntype != TURN_RANDOM) {
    if ((global->turntype != TURN_RANDOM) && (global->turntype != TURN_SIMUL))
    {
        for (int index = 0; index < global->maxNumTanks - 1; index++)
        {
            int swap = FALSE;
            if (global->turntype == TURN_HIGH)
            {
                if (env->order[index]->player->score < env->order[index + 1]->player->score)
                    swap = TRUE;
            }
            else if (global->turntype == TURN_LOW)
            {
                if (env->order[index]->player->score > env->order[index + 1]->player->score)
                    swap = TRUE;
            }
            if (swap)
            {
                TANK *tempTank = env->order[index];
                env->order[index] = env->order[index + 1];
                env->order[index + 1] = tempTank;
                index = -1;
            }
        }
    }
    
    // Create the ordered list of players
    // Since tanks get deleted as they are destroyed, we loose the information
    // about their order.  The player order list will be complete through the
    // entire round.
    for (int i = 0; i < global->maxNumTanks; i++)
        env->playerOrder[i] = env->order[i]->player;
}

char *do_winner(GLOBALDATA *global, ENVIRONMENT *env)
{
    int maxscore = -1;
    int winindex = -1;
    int i, index, *order;
    bool multiwinner = false;
    int fonthgt = text_height(font) + 10;
    int jedi_index = -1, sith_index = -1, neutral_index = -1;
    // char *my_quote;
    char *return_string = NULL;

    return_string = (char *) calloc(256, sizeof(char));
    if (!return_string)
        return NULL;

    //find the maxscore and print out winner
    for (i=0;i<global->numPlayers;i++)
    {
        if (global->players[i]->score == maxscore)
        {
            multiwinner=true;
            if (global->players[i]->team == TEAM_NEUTRAL)
                neutral_index = i;
        }

        else if (global->players[i]->score > maxscore)
        {
            maxscore = global->players[i]->score;
            winindex=i;
            multiwinner=false;
            if (global->players[i]->team == TEAM_NEUTRAL)
                neutral_index = i;
        }
        if (global->players[i]->team == TEAM_JEDI)
            jedi_index = i;
        else if (global->players[i]->team == TEAM_SITH)
            sith_index = i;
    }

    //stop mouse during drawing
    if (!global->os_mouse)
        show_mouse(NULL);

    //draw background and winner bitmap
    draw_circlesBG(global, env->db, 0, 0, global->screenWidth, global->screenHeight, false);
    draw_sprite(env->db, (BITMAP *) global->misc[9], global->halfWidth - 150, global->halfHeight - 150);

    //draw winner names and info about all players
    int boxtop = global->halfHeight-40;
    int boxleft = global->halfWidth-200;
    int boxright = global->halfWidth+280;
    int boxbottom = boxtop +4+(fonthgt*2)+(fonthgt*global->numPlayers);

    rectfill(env->db, boxleft, boxtop, boxright, boxbottom, BLACK);
    rect(env->db, boxleft, boxtop, boxright, boxbottom, WHITE);
    if (multiwinner)
    {
        // check for team win
        if (global->players[winindex]->team == TEAM_JEDI)
        {
            if ( (sith_index >= 0) && ((global->players[sith_index]->score == global->players[winindex]->score)) )
                snprintf(return_string, 256, "%s", global->ingame->complete_text[48]);
            else if ( (neutral_index >= 0) && ( (global->players[neutral_index]->score == global->players[winindex]->score)) )
                snprintf(return_string, 256, "%s", global->ingame->complete_text[48]);
            else
                snprintf(return_string, 256, "%s", global->ingame->complete_text[45]);
        }
        else if (global->players[winindex]->team == TEAM_SITH)
        {
            if ( (jedi_index >= 0) && ((global->players[jedi_index]->score == global->players[winindex]->score)) )
                snprintf(return_string, 256, "%s", global->ingame->complete_text[48]);
            else if ( (neutral_index >= 0) && ( (global->players[neutral_index]->score == global->players[winindex]->score)) )
                snprintf(return_string, 256, "%s", global->ingame->complete_text[48]);
            else
                snprintf(return_string, 256, "%s", global->ingame->complete_text[46]);
        }
        else
            snprintf(return_string, 256, "%s", global->ingame->complete_text[48]);
    }
    else
        snprintf(return_string, 256, "%s: %s", global->ingame->complete_text[47], global->players[winindex]->getName());

    textprintf_centre_ex(env->db, font, global->halfWidth, boxtop + 4, global->players[winindex]->color, -1, "%s", return_string);

    textout_centre_ex (env->db, font, global->ingame->complete_text[49], global->halfWidth, boxtop+4+fonthgt, WHITE, -1);
    order = Sort_Scores(global);
    for (index = 0; index < global->numPlayers; index++)
    {
        int i = order[index];
        int textypos = (index * 10) + boxtop+4+(fonthgt*2);
        int money;

        textprintf_ex(env->db, font, boxleft+10, textypos , global->players[i]->color, -1, "%s:", global->players[i]->getName());

        money = 0;
        for (int weapNum = 0; weapNum < WEAPONS; weapNum++)
        {
            int individValue;
            if (weapon[weapNum].amt)
                individValue = weapon[weapNum].cost / weapon[weapNum].amt;
            else
                individValue = 0;
            money += (int) (individValue * global->players[i]->nm[weapNum]);
        }
        for (int itemNum = 0; itemNum < ITEMS; itemNum++)
        {
            int individValue;
            if (item[itemNum].amt)
                individValue = item[itemNum].cost / item[itemNum].amt;
            else
                individValue = 0;
            money += (int) (individValue * global->players[i]->ni[itemNum]);
        }
        textprintf_ex(env->db, font, boxleft+190, textypos, WHITE, -1, "%3d  $%s   %10d :%2d", global->players[i]->score, Add_Comma(money), global->players[i]->kills, global->players[i]->killed);
    }

/*
    my_quote = global->war_quotes->Get_Random_Line();
    if (my_quote)
    {
        char *little_string;
        int start_index = 0, to_index = 0;
        int total_length = strlen(my_quote);
        int quote_count = 1;

        little_string = (char *) calloc(total_length + 1, sizeof(char));
        if (little_string)
        {
            do
            {
                memset(little_string, '\0', total_length + 1);
                while ( ((my_quote[start_index] != ' ') || (to_index < 50)) && (start_index < total_length) )
                {
                    little_string[to_index] = my_quote[start_index];
                    to_index++;
                    start_index++;
                }

                textprintf_ex(env->db,font,boxleft, boxbottom + (10 * quote_count), WHITE, -1, "%s", little_string);
                to_index = 0;
                quote_count++;
                while ((start_index < total_length) && (my_quote[start_index] == ' '))
                    start_index++;
            } while (start_index < total_length);
            free(little_string);
        }
    }
    //do fade and wait for user keypress
    change(global, env->db);
    readkey();
    // for (i = 0; i < global->numPlayers; i++)
    //  global->players[i]->type = global->players[i]->type_saved;
*/
    return return_string;
}

void do_quote(GLOBALDATA *global, ENVIRONMENT *env)
{
    char *my_quote;
    int fonthgt = text_height(font)+10;
    int boxleft = global->halfWidth-200;
    int boxtop = global->halfHeight-40;
    int boxbottom = boxtop + 4 + (fonthgt * 2) + (fonthgt*global->numPlayers);

    my_quote = global->war_quotes->Get_Random_Line();
    if (my_quote)
    {
        char *little_string;
        int total_length = strlen(my_quote);
        int quote_count = 1;

        little_string = (char *) calloc(total_length + 1, sizeof(char));
        if (little_string)
        {
            int start_index = 0;

            do
            {
                int to_index = 0;

                memset(little_string, '\0', total_length + 1);
                while ( ((my_quote[start_index] != ' ') || (to_index < 50)) && (start_index < total_length) )
                {
                    little_string[to_index] = my_quote[start_index];
                    to_index++;
                    start_index++;
                }

                textprintf_ex(env->db,font,boxleft, boxbottom + (10 * quote_count), WHITE, -1, "%s", little_string);
                to_index = 0;
                quote_count++;
                while ((start_index < total_length) && (my_quote[start_index] == ' '))
                    start_index++;
            } while (start_index < total_length);
            free(little_string);
        }
    }

    //do fade and wait for user keypress
    change(global, env->db);
    readkey();
    for (int i = 0; i < global->numPlayers; i++)
        global->players[i]->type = global->players[i]->type_saved;
}

//draws indication bar
void graph_bar(ENVIRONMENT *env, int x, int y, long int col, int actual, int max)
{
    rect(env->db, x, y, x + max + 2, y + 8, BLACK);
    rectfill(env->db, x + 1, y + 1, x + 1 + actual, y + 7, col);
}

//draws indication bar - centred
void graph_bar_center(ENVIRONMENT *env, int x, int y, long int col, int actual, int max)
{
    rect(env->db, x, y, x + max + 2, y + 8, BLACK);
    rectfill(env->db, x + 1 + max / 2, y + 1, x + 1 + actual + max / 2, y + 7, col);
}

//Some global parameters
int ord;
void loadshields(ENVIRONMENT *env)
{
    TANK *tank;
    int objCount;

    for (objCount = 0; (tank = (TANK*) env->getNextOfClass(TANK_CLASS, &objCount)) && tank; objCount++)
        tank->reactivate_shield();
}

void change_wind_strength(ENVIRONMENT *env)
{
    if (env->windvariation == 0.0 || (int) env->windstrength == 0)
        return;
    else
    {
        env->wind = env->lastwind + (double) (rand() % (int) (env->windvariation * 100)) / 100 - (env->windvariation / 2);
        if (env->wind > (env->windstrength / 2))
            env->wind = env->windstrength / 2;
        else if (env->wind < (-env->windstrength / 2))
            env->wind = -env->windstrength / 2;

        env->lastwind = env->wind;
    }

    // make sure game clients have up to date wind data
#ifdef NETWORK
    char buffer[64];
    sprintf(buffer, "WIND %f", env->wind);
    env->_global->Send_To_Clients(buffer);
#endif
}

TANK *nextturn(GLOBALDATA *global, ENVIRONMENT *env, bool skippingComputerPlay)
{
    TANK *tank = NULL;
    int ordCurrently = ord;
    int lowest_shots = INT_MAX;

    // check whether there currently *are* active tanks first
    if (global->numTanks)
    {
        static int do_wind = 0;
        static int next_wind = 0;
        int index = 0, lowest_index = 0;

        // find first tank with lowest number of shots fired
        while (index < global->maxNumTanks)
        {
            if (env->order[index])    // make sure tank exists
            {
                if (env->order[index]->shots_fired < lowest_shots)
                {
                    lowest_shots = env->order[index]->shots_fired;
                    lowest_index = index;
                }
            }
            index++;
        }    // end of looking for low index

        do
        {
            ord++;
            // coming around to the next turn
            if (ord >= global->maxNumTanks)
            {
                ord = 0;
                doLaunch(global, env);
                // launch before we change the wind
                next_wind = 1;
            }

        } while ((!env->order[ord]) && (ord != ordCurrently));
        tank = env->order[ord];
        global->currTank = tank;

        if (tank->shots_fired > lowest_shots)
        {
            tank = env->order[lowest_index];
            global->currTank = tank;
        }

        // Wind is blowing :-)
        // change_wind_strength (env);
        if ((global->turntype != TURN_SIMUL) || do_wind)
        {
            change_wind_strength(env);
            do_wind = next_wind = 0;
        }
        else
        {
            do_wind = next_wind;
            next_wind = 0;
        }
    }

    if (tank)
    {
        if (!skippingComputerPlay)
        {
            env->make_fullUpdate();
            env->do_updates();
        }
        tank->reactivate_shield();
        clear_keybuf();
        if (global->max_fire_time)
        {
            tank->player->time_left_to_fire = global->max_fire_time;
            tank->player->skip_me = false;
        }
    }

    return tank;
}

void showRoundEndScoresAt(GLOBALDATA *global, ENVIRONMENT *env, BITMAP *bitmap, int x, int y, int winner)
{
    int z;

    env->make_update(x - 150, y - 100, 301, 301);
    rectfill(bitmap, x - 150, y - 100, x + 100, y + 100, BLACK);
    rect(bitmap, x - 150, y - 100, x + 100, y + 100, WHITE);
    if (winner == JEDI_WIN)
        textout_centre_ex(bitmap, font, "Jedi", x - 20, y - 90, WHITE, -1);
    else if (winner == SITH_WIN)
        textout_centre_ex(bitmap, font, "Sith", x - 20, y - 90, WHITE, -1);
    else if (winner == -2)
        textout_centre_ex(bitmap, font, "Draw", x - 20, y - 90, WHITE, -1);
    else
        textprintf_centre_ex(bitmap, font, x - 30, y - 90, global->players[winner]->color, -1, "%s: %s", global->ingame->complete_text[47], global->players[winner]->getName());

    textout_centre_ex(bitmap, font, global->ingame->complete_text[50], x - 30, y - 70, WHITE, -1);
    for (z = 0; z < global->numPlayers; z++)
    {
        textprintf_ex(bitmap, font, x - 140, (z * 10) + y - 50, global->players[z]->color, -1, "%s:", global->players[z]->getName());
        textprintf_ex(bitmap, font, x + 60, (z * 10) + y - 50, WHITE, -1, "%d", global->players[z]->score);
    }
}

int setSlideColumnDimensions(GLOBALDATA *global, ENVIRONMENT *env, int x, bool reset)
{
    int pixelHeight;
    char *done = env->done;
    int *dropTo = env->dropTo;
    int *h = env->h;
    int *fp = env->fp;
    double *velocity = env->velocity;
    double *dropIncr = env->dropIncr;

    if (x < 0 || x > (global->screenWidth-1))
        return 0;

    if (reset)
    {
        h[x] = 0;
        fp[x] = 0;
        dropTo[x] = global->screenHeight - 1;
    }
    done[x] = 0;

    // Calc the top and bottom of the column to slide

    // Find top-most non-PINK pixel
    for (pixelHeight = h[x]; pixelHeight < dropTo[x]; pixelHeight++)
        if (getpixel(env->terrain, x, pixelHeight) != PINK)
            break;
    h[x] = pixelHeight;
    env->surface[x] = pixelHeight;

    // Find bottom-most PINK pixel
    for (pixelHeight = dropTo[x]; pixelHeight > h[x]; pixelHeight--)
        if (getpixel(env->terrain, x, pixelHeight) == PINK)
            break;
    dropTo[x] = pixelHeight;

    // Find bottom-most unsupported pixel
    for (; pixelHeight >= h[x]; pixelHeight--)
        if (getpixel(env->terrain, x, pixelHeight) != PINK)
            break;

    // If there's some processing to do
    if ((pixelHeight >= h[x]) && (h[x] < dropTo[x]))
    {
        fp[x] = pixelHeight - (int) h[x] + 1;
        return 0;
    }
    else
    {
        if (velocity[x])
            play_sample((SAMPLE *) global->sounds[10], env->scaleVolume((velocity[x] / 10) * 255), (int) ((double) (x - global->halfWidth) / global->halfWidth * 128 + 128), 1000 - (int)((double)fp[x] / global->screenHeight) * 1000, 0);
        h[x] = 0;
        fp[x] = 0;
        done[x] = 1;
        velocity[x] = 0;
        dropIncr[x] = 0;
        dropTo[x] = global->screenHeight - 1;
        return 1;
    }
    return 0;
}

int drawFracture(GLOBALDATA *global, ENVIRONMENT *env, BITMAP *dest, BOX *updateArea, int x, int y, int angle, int width, int segmentLength, int maxRecurse, int recurseDepth)
{
    int x1, x2, x3;
    int y1, y2, y3;

    x1 = (int) (x + global->slope[angle][0] * width);
    y1 = (int) (y + global->slope[angle][1] * width);
    x2 = (int) (x - global->slope[angle][0] * width);
    y2 = (int) (y - global->slope[angle][1] * width);
    x3 = (int) (x + global->slope[angle][1] * segmentLength);
    y3 = (int) (y + global->slope[angle][0] * segmentLength);
    triangle(dest, x1, y1, x2, y2, x3, y3, PINK);

    if (recurseDepth == 0)
    {
        updateArea->x = x1;
        updateArea->y = y1;
        updateArea->w = x1;
        updateArea->h = y1;
    }
    updateArea->x = MIN(MIN(MIN(x1, x2), x3), updateArea->x);
    updateArea->y = MIN(MIN(MIN(y1, y2), y3), updateArea->y);
    updateArea->w = MAX(MAX(MAX(x1, x2), x3), updateArea->w);
    updateArea->h = MAX(MAX(MAX(y1, y2), y3), updateArea->h);

    if (recurseDepth < maxRecurse)
    {
        for (int branchCount = 0; branchCount < 3; branchCount++)
        {
            if ((branchCount == 0) || (Noise(x + y + branchCount) < 0))
            {
                int newAngle, reduction;
                newAngle = (angle + (int) (Noise(x + y + 4) * 30));
                while (newAngle < 0)
                    newAngle += 360;
                newAngle %= 360;

                reduction = 2;
                if (branchCount == 1)
                {
                    newAngle = (int) (angle + 90 + (Noise(x + y + 25 + branchCount) * 22.5)) % 360;
                    reduction = abs((int) Noise(x + y + 1 + branchCount) * 4 + 3);
                }
                else if (branchCount == 2)
                {
                    newAngle = (int)(angle + 270 + (Noise(x + y + 32 + branchCount) * 22.5)) % 360;
                    reduction = abs((int) Noise (x + y + 2 + branchCount) * 4 + 3);
                }
                drawFracture(global, env, dest, updateArea, x3, y3, newAngle, width / reduction, segmentLength / reduction, maxRecurse, recurseDepth + 1);
            }
        }
    }

    // Calculate width and height, previously right and bottom
    if (recurseDepth == 0)
    {
        updateArea->w -= updateArea->x;
        updateArea->h -= updateArea->y;
    }

    return 0;
}

/*
void initSurface(GLOBALDATA *global, ENVIRONMENT *env)
{
    int pixelHeight;
    for (int x = 0; x < global->screenWidth; x++)
    {
        for (pixelHeight = 0; pixelHeight < global->screenHeight; pixelHeight++)
            if (getpixel(env->terrain, x, pixelHeight) != PINK)
                break;
        env->surface[x] = pixelHeight;
    }
}
*/


int slideLand(GLOBALDATA *global, ENVIRONMENT *env)
{
    char *done = env->done;
    int	*dropTo = env->dropTo;
    int	*h = env->h;
    int	*fp = env->fp;
    double *velocity = env->velocity;
    double *dropIncr = env->dropIncr;
    int zz;
    double land_slide_type = LANDSLIDE_INSTANT;

    // land-slide, make it fall etc.
    int allDone = 1;
    if ((env->landSlideType == LANDSLIDE_NONE) || (env->landSlideType == LANDSLIDE_TANK_ONLY))
        return allDone;

    else if (env->landSlideType == LANDSLIDE_CARTOON)
    {
        if (env->time_to_fall > 0)
            land_slide_type = LANDSLIDE_CARTOON;
        else
            land_slide_type = LANDSLIDE_GRAVITY;
    }

    else if (env->landSlideType == LANDSLIDE_GRAVITY)
        land_slide_type = LANDSLIDE_GRAVITY;

    if (land_slide_type == LANDSLIDE_CARTOON)
        return allDone;

    for (zz = 0; zz < global->screenWidth; zz++)
    {
        if (!done[zz])
        {
            allDone = 0;
            if (land_slide_type == LANDSLIDE_GRAVITY)
            {
                if (fp[zz] > 0)
                {
                    velocity[zz] += env->gravity;
                    dropIncr[zz] += velocity[zz];
                    if (dropIncr[zz] >= 1)
                    {
                        if (dropIncr[zz] > dropTo[zz] - (h[zz] + fp[zz]))
                            dropIncr[zz] = dropTo[zz] - (h[zz] + fp[zz]) + 1;
                        blit(env->terrain, env->terrain, zz, h[zz] - (int) dropIncr[zz], zz, h[zz], 1, fp[zz] + (int) dropIncr[zz]);
                        env->make_bgupdate(zz, h[zz] - (int) dropIncr[zz], 1, fp[zz] + ((int) dropIncr[zz] * 2) + 1);
                        env->make_update(zz, h[zz] - (int) dropIncr[zz], 1, fp[zz] + ((int) dropIncr[zz] * 2) + 1);
                        h[zz] += (int) dropIncr[zz];
                        dropIncr[zz] -= (int) dropIncr[zz];
                    }
                    setSlideColumnDimensions(global, env, zz, FALSE);
                }
                else
                    setSlideColumnDimensions(global, env, zz, FALSE);
            }
            else if (land_slide_type == LANDSLIDE_INSTANT)
            {
                if (fp[zz] > 0)
                {
                    env->make_bgupdate(zz, h[zz], 1, dropTo[zz] - h[zz] + 1);
                    env->make_update(zz, h[zz], 1, dropTo[zz] - h[zz] + 1);
                    done[zz] = 1;
                    blit(env->terrain, env->terrain, zz, h[zz], zz, dropTo[zz] - fp[zz] + 1, 1, fp[zz]);
                    vline(env->terrain, zz, h[zz], dropTo[zz] - fp[zz], PINK);
                }
                setSlideColumnDimensions(global, env, zz, FALSE);
            }
        }
    }

    return allDone;
}

void drawTopBar(GLOBALDATA *global, ENVIRONMENT *env, BITMAP *dest)
{
    TANK *tank = global->currTank;
    char *name = "";
    int color = 0;
    char *team_name = "";
    int time_to_fire = 0;
    int minus_font = (font == global->unicode) ? 9 : 0;

    if (tank)
    {
        name = global->currTank->player->getName();
        color = global->currTank->player->color;
        team_name = global->currTank->player->Get_Team_Name();
        time_to_fire = tank->player->time_left_to_fire;
    }

    global->updateMenu = 0;
    blit(global->gfxData.topbar, dest, 0, 0, 0, 0, global->screenWidth, MENUHEIGHT);

    if (tank)
    {
        textout_ex(dest, font, name, 2, 2 - minus_font, BLACK, -1);
        textout_ex(dest, font, name, 1, 1 - minus_font, color, -1);
        textprintf_ex(dest, font, 1, 11 - minus_font, BLACK, -1, "%s", global->ingame->complete_text[18]);
        graph_bar_center(env, 50, 11, color, -(tank->a - 180) / 2, 180 / 2);
        // 0 is directly left, 180 points directly right
        textprintf_ex(dest, font, 150, 11 - minus_font, BLACK, -1, "%d", 180 - (tank->a - 90));

        textprintf_ex(dest, font, 1, 21 - minus_font, BLACK, -1, "%s", global->ingame->complete_text[19]);
        graph_bar(env, 50, 20, color, (tank->p) / (MAX_POWER/90), 90);
        textprintf_ex(dest, font, 150, 21 - minus_font, BLACK, -1, "%d", tank->p);
        textprintf_ex(dest, font, 200, 21 - minus_font, BLACK, -1, "%s: %s", global->ingame->complete_text[20], team_name);
        if (tank->cw < WEAPONS)
        {
            int weapon_amount;

            if (!weapon[tank->cw].delay)
                weapon_amount = tank->player->nm[tank->cw];
            else
                weapon_amount = tank->player->nm[tank->cw] / weapon[tank->cw].delay;

            if (tank->player->changed_weapon)
            {
                static int change_weapon_colour = RED;

                textprintf_ex(dest, font, 180, 1, change_weapon_colour, -1, "%s: %d", weapon[tank->cw].name, weapon_amount);
                // tank->player->nm[tank->cw]);
                if (change_weapon_colour == RED)
                    change_weapon_colour = WHITE;
                else
                    change_weapon_colour = RED;
            }
            else
                textprintf_ex(dest, font, 180, 1, BLACK, -1,"%s: %d", weapon[tank->cw].name, weapon_amount);

        }    // end of less than WEAPONS
        else
            textprintf_ex(dest, font, 180, 1, BLACK, -1, "%s: %d",
                          item[tank->cw - WEAPONS].name, tank->player->ni[tank->cw - WEAPONS]);

        draw_sprite(env->db, (BITMAP *) global->stock[(tank->cw) ? tank->cw : 1], 700, 1);
        textprintf_ex(dest, font, 350, 1, BLACK, -1, "$%s", Add_Comma(tank->player->money));
        textprintf_ex(dest, font, 350, 12, BLACK, -1, "%s: %d", global->ingame->complete_text[21], tank->player->ni[ITEM_FUEL]);
    }

    textprintf_ex(dest, font, 500,  1 - minus_font, BLACK, -1, "%s %d/%d",
                  global->ingame->complete_text[12], (int) (global->rounds - global->currentround) + 1, (int) global->rounds);

    if (global->tank_status[0])
        textprintf_ex(dest, font, 350, 21, global->tank_status_colour, -1, "%s", global->tank_status);

    if (env->windstrength > 0)
    {
        int wind_col1 = 0, wind_col2 = 0;

        textprintf_ex(dest, font, 500, 11 - minus_font, BLACK, -1, "%s", global->ingame->complete_text[22]);
        if (env->wind > 0)
        {
            wind_col1 = 1;
            wind_col2 = 0;
        }
        if (env->wind < 0)
        {
            wind_col1 = 0;
            wind_col2 = 1;
        }
        rect(dest, 540, 12, (int) (540 + env->windstrength * 4 + 2), 18, BLACK);
        rectfill(dest, (int) (541 + env->windstrength * 2), 13,
                 (int) (541 + env->wind * 4 + env->windstrength * 2), 17, makecol(200 * wind_col1, 200 * wind_col2, 0));
    }

    if (global->max_fire_time)
        textprintf_ex(dest, font, 500, 20, BLACK, -1, "Time: %d", time_to_fire);

    global->stopwindow = 1;
    env->make_update(0, 0, global->screenWidth, MENUHEIGHT);
    global->stopwindow = 0;
}

///
/// display the scoreboard in the corner of the game screen
///
void drawScoreboard(GLOBALDATA *global, ENVIRONMENT *env, BITMAP *dest, TANK *curTank)
{
    int minus_font = (font == global->unicode) ? 9 : 0;
    int liney = MENUHEIGHT + 2;
    
    for (int i = 0; i < global->numPlayers; i++)
    {
        PLAYER *player = env->playerOrder[i];
        
        if (player != NULL)
        {
            // Draw the tank score
            char *name = player->getName();
            int color = player->color;
            // char *team_name = player->Get_Team_Name();
            char *money = Add_Comma(player->money);
            
            // TODO: if team_name != "", add (team name) after player name

            textout_ex(dest, font, name, 11, liney + 1 - minus_font, BLACK, -1);
            textout_ex(dest, font, name, 10, liney - minus_font, color, -1);
            textprintf_ex(dest, font, 181, liney + 1 - minus_font, BLACK, -1, "$%s", money);
            textprintf_ex(dest, font, 180, liney - minus_font, color, -1, "$%s", money);

            if (curTank == player->tank)
            {
                // Draw an arrow indicating the current player
                hline(dest, 4, liney + 4, 8, color);
                hline(dest, 5, liney + 5, 9, BLACK);
            }
            
            if (!player->tank)
            {
                // Strikethrough on dead players
                //hline(dest, 4, liney + 4, 275, makecol(192, 192, 192));
                hline(dest, 4, liney + 5, 275, BLACK);
            }

            liney += 11;
        }
    }

    env->make_update(0, MENUHEIGHT, 300, liney);
}

/*
 *  Calculate the effective damage for a given weapon.
 *  Recursively add the damage of sub-munitions, factor in chaos
 *    and munition density.
 */
long int calcTotalEffectiveDamage(int weapNum)
{
    WEAPON *weap = &weapon[weapNum];
    long int total = 0;

    if (weap->submunition >= 0)
    {
        WEAPON *subm = &weapon[weap->submunition];

        // How chaotic is this weapon?
        double chaosVal = (weap->spreadVariation + weap->speedVariation + subm->countVariation) / 3;
        double coverage = (weap->numSubmunitions * subm->radius) / (double)weap->radius;

        total += calcTotalEffectiveDamage (weap->submunition) * weap->numSubmunitions;
        total = (long int) (total * coverage / weap->numSubmunitions);
        total -= (long int) ((total / 2) * (1.0 - chaosVal));
    }
    else
        total += weap->damage;

    return total;
}

/*
 *  Calculate the potential damage for a given weapon.
 *  Recursively add the damage of sub-munitions.
 */
long int calcTotalPotentialDamage(int weapNum)
{
    WEAPON *weap = &weapon[weapNum];
    long int total = 0;

    if ((weap->submunition >= 0) && (weap->numSubmunitions > 0))
        total += calcTotalPotentialDamage(weap->submunition) * weap->numSubmunitions;
    else
        total += weap->damage;

    return total;
}

void doNaturals(GLOBALDATA *global, ENVIRONMENT *env)
{
    int chance;

    if (env->naturals_since_last_shot >= 5)
        return;

    if (env->lightning)
    {
        chance = (int) (600 / env->lightning) + 100;
        if (!(rand() % chance))
        {
            BEAM *newbeam;
            int ca = ((rand() % 160) + (360 - 80)) % 360;

            newbeam = new BEAM(global, env, rand() % global->screenWidth, 0,
                               ca, SML_LIGHTNING + (rand() % (int) env->lightning));
            if (newbeam)
            {
                newbeam->player = NULL;
                env->naturals_since_last_shot++;
            }
            else
                perror("atanks.cpp: Failed allocating memory for newbeam in doNaturals");
        }
    }      // end of lightning

    // only create meteors  if we are not in aim mode on simul turn type
    if ((global->turntype == TURN_SIMUL) && (env->stage == STAGE_AIM))
        return;

    if (env->meteors)
    {
        chance = (int) (600 / env->meteors) + 100;
        if (!(rand() % chance))
        {
            MISSILE *newmis;
            int ca = ((rand() % 160) + (360 - 80)) % 360;
            double mxv = global->slope[ca][0] * 5;
            double myv = global->slope[ca][1] * 5;

            newmis = new MISSILE(global, env, rand() % global->screenWidth, 0,
                                 mxv, myv, SML_METEOR + (rand() % (int) env->meteors));
            if (newmis)
            {
                newmis->player = NULL;
                env->naturals_since_last_shot++;
            }
            else
                perror("atanks.cpp: Failed allocating memory for newmis in doNaturals");
        }
    }

    if (env->falling_dirt_balls)
    {
        chance = (int) (600 / env->falling_dirt_balls) + 100;
        if (!(rand() % chance))
        {
            MISSILE *newmis;
            int ca = ((rand() % 100) + (360 - 80) ) % 360;
            double mxv = global->slope[ca][0] * 5;
            double myv = global->slope[ca][1] * 5;

            newmis = new MISSILE(global, env, rand() % global->screenWidth, 0,
                                 mxv, myv, DIRT_BALL + (rand() % (int) env->falling_dirt_balls));
            if (newmis)
            {
                newmis->player = NULL;
                env->naturals_since_last_shot++;
            }
            else
                perror("atanks.cpp: Failed to allocate memory for falling dirt ball in doNaturals");
        }
    }
}

void print_text_help()
{
    cout << "-h\tThis screen\n"
         << "-fs\tFull screen\n"
         << "--windowed\tRun in a window\n"
         << "-w <width> or --width <width>\tSpecify the screen width in pixels\n"
         << "-t <height> --tall <height>\tSpecify the screen height in pixels\n"
         << "\tAdjust the screen size at your own risk (default is 800x600)\n"
         << "-d <depth> or --depth <depth>\tCurrently either 16 or 32\n"
         << "--datadir <data directory>\t Path to the data directory\n"
         << "-c <config directory>\t Path to config and saved game directory\n"
         << "--noconfig\t Do not load game settings from the config file.\n"
         << "--nosound\t Disable sound\n"
         << "--noname\t Do not show player name above tank\n"
         << "--nonetwork\t Do not allow the game to accept network connection.\n"
         << "--nobackground\t Do not display the green menu background.\n"
         << "--nothread\t Do not use threads to perform background tasks.\n"
         << "--thread\t Do use threads to perform background tasks.\n";
}

void print_text_initmsg()
{
    printf("Atomic Tanks Version %s (-h for help)\n", VERSION);
    printf("Authors: \tTom Hudson (rewrite, additions, improvements)\n");
    printf("\t\tStevante Software (original design)\n");
    printf("\t\tKota543 Software (fixes and updates)\n");
    printf("\t\tJesse Smith (additions, fixes and updates)\n");
    printf("\t\tSven Eden (ai rewrite, additions, fixes and updates)\n");
    printf("\t\tBruno Victal (code clean-up)\n\n");

    // putchar ('\n');
}

void endgame_cleanup(GLOBALDATA *global, ENVIRONMENT *env)
{
    while (global->numPlayers > 0)
    {
        if (global->players[0]->tank)
            delete(global->players[0]->tank);
        // make sure networked clients say good-bye and return to old AI level
        if (global->players[0]->type >= NETWORK_CLIENT)
            global->players[0]->type = global->players[0]->previous_type;

        global->players[0]->tank = NULL;
        global->removePlayer(global->players[0]);
    }
    for (int objCount = 0; objCount < MAX_OBJECTS; objCount++)
    {
        if (env->objects[objCount])
        {
            delete (env->objects[objCount]);
            env->objects[objCount] = NULL;
        }
    }
}

/*
This function calls the functions which save data to a text file.
The function requires the global data, environment and the path to
the config file name.
The function returns TRUE on success and FALSE on failure.
-- Jesse
*/
int Save_Game_Settings_Text(GLOBALDATA *global, ENVIRONMENT *env, char *text_file)
{
    FILE *my_file;

    my_file = fopen(text_file, "w");
    if (!my_file)
    {
        perror("Error trying to open text file for writing.\n");
        return FALSE;
    }

    global->saveToFile_Text(my_file);
    env->saveToFile_Text(my_file);
    savePlayers_Text(global, my_file);
    fclose(my_file);
    return TRUE;
}

/*
This function detects changes to the global settings (mouse and sound)
and, if a change has happened, makes the required changes to the
game environment.
The function returns TRUE.
-- Jesse
*/
int Change_Settings(double old_mouse, double old_sound, double new_mouse, double new_sound, void *mouse_image)
{
    BITMAP *my_mouse_image = (BITMAP *) mouse_image;

    // first, check for a change in the sound settings
    if (old_sound != new_sound)
    {
        if (new_sound > 0.0)    // we turned ON sound
        {
            if (detect_digi_driver(DIGI_AUTODETECT))
            {
                if (install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL) < 0)
                    fprintf(stderr, "install_sound: failed turning on sound\n");
            }
            else
                fprintf(stderr, "detect_digi_driver found no sound device\n");

            //if (install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL) < 0)
            //{
            //    fprintf(stderr, "install_sound: %s", allegro_error);
            //}
        }
        else if (new_sound == 0.0)  // we turned OFF sound
            remove_sound();
    }

    // check for a change in mouse settings
    if (old_mouse != new_mouse)
    {
        if (new_mouse > 0.0)   // use OS cursor
        {
            set_mouse_sprite(NULL);
            show_os_cursor(MOUSE_CURSOR_ARROW);
        }
        else if (new_mouse == 0.0)     // use Allgero cursor
        {
            set_mouse_sprite(my_mouse_image);
            set_mouse_sprite_focus (0, 0);
        }
    }
    return TRUE;
}

/*
This function catches the close command, usually given by
the user pressing the close window button. We'll
try to clean-up.

Note: This function causes the app to hang in Windows.
Make this compile on non-Windows systems only.
*/
void close_button_handler(void)
{
    // allegro_exit();
    // exit(0);
    my_global->close_button_pressed = true;
}

int main(int argc, char **argv)
{
    int status;
    string tmp;
    ENVIRONMENT *env = NULL;
    GLOBALDATA *global = NULL;
    // ifstream configFile;
    char fullPath[2048];
    bool load_config_file = true;
    bool bLoadingSuccess = false;
    double temp_mouse, temp_sound;    // I wish I was not using these
    int menu_action;
    int playerCount, player_index;
#ifdef NETWORK
#ifdef THREADS
    SEND_RECEIVE_TYPE *send_receive = NULL;
#endif
    int client_socket = -1;
#endif
    bool allow_network = true, allow_thread = false;
    double music_place_holder;
    double full_screen = FULL_SCREEN_EITHER;
    init_cclock_lock();

    quit_right_now = false;
    global = new GLOBALDATA();
    if (!global)
    {
        perror("Allocating global");
        exit(1);
    }
    my_global = global;
    print_text_initmsg();

    // try to find data dir
    if (!global->Find_Data_Dir())
        printf("Could not find data dir.\n");

    if (argc >= 2)    /* Parse command-line switches */
    {
        cmdTokens nextToken = ARGV_NOTHING_EXPECTED;

        for (int argument = 1; argument < argc; argument++)
        {
            tmp = argv[argument];

            if (nextToken == ARGV_GFX_DEPTH)
            {
                global->colourDepth = strtol(tmp.c_str(), NULL, 10);
                if (global->colourDepth != 16 && global->colourDepth != 32)
                {
                    cout << "Invalid graphics depth, only 16 or 32 are valid\n";
                    print_text_help();
                    return 0;
                }
                nextToken = ARGV_NOTHING_EXPECTED;
            }
            else if (nextToken == ARGV_SCREEN_WIDTH)
            {
                global->screenWidth = strtol(tmp.c_str(), NULL, 10);
                if (global->screenWidth < 512)
                {
                    cout << "Width too small (minimum 512)\n";
                    return 0;
                }
                global->width_override = global->screenWidth;
                global->halfWidth = global->screenWidth / 2;
                nextToken = ARGV_NOTHING_EXPECTED;
            }
            else if (nextToken == ARGV_SCREEN_HEIGHT)
            {
                global->screenHeight = strtol(tmp.c_str(), NULL, 10);
                if (global->screenHeight < 320)
                {
                    cout << "Height too small (minimum 320)\n";
                    return 0;
                }
                global->height_override = global->screenHeight;
                global->halfHeight = global->screenHeight / 2;
                nextToken = ARGV_NOTHING_EXPECTED;
            }
            else if (nextToken == ARGV_DATA_DIR)
            {
                // Would use strndup, but the win compiler
                //   doesn't know of it.
                if (strlen(tmp.c_str()) > 2048)
                {
                    cout << "Datadir path too long:\n"
                         << "\"" << tmp
                         << "\"\n\n"
                         << "Maximum length 2048 characters\n";
                    return 0;
                }
                global->dataDir = strdup(tmp.c_str());
                nextToken = ARGV_NOTHING_EXPECTED;
            }
            else if (nextToken == ARGV_CONFIG_DIR)
            {
                if (strlen (tmp.c_str()) > 2048)
                {
                    cout << "Configdir path too long:\n"
                         << "\"" << tmp
                         << "\"\n\n"
                         << "Maximum length 2048 characters\n";
                    return 0;
                }
                global->configDir = strdup(tmp.c_str());
                nextToken = ARGV_NOTHING_EXPECTED;
            }
            if ((tmp == SWITCH_HELP) || (tmp == "--help"))
            {
                print_text_help();
                return 0;
            }
            else if (tmp == SWITCH_FULL_SCREEN)
            {
                screen_mode = GFX_AUTODETECT_FULLSCREEN;
                full_screen = FULL_SCREEN_TRUE;
            }
            else if (tmp == SWITCH_WINDOWED)
            {
                screen_mode = GFX_AUTODETECT_WINDOWED;
                full_screen = FULL_SCREEN_FALSE;
            }
            else if (tmp == "-d" || tmp == "--depth")
                nextToken = ARGV_GFX_DEPTH;
            else if (tmp == "-w" || tmp == "--width")
                nextToken = ARGV_SCREEN_WIDTH;
            else if (tmp == "-t" || tmp == "--tall")
                nextToken = ARGV_SCREEN_HEIGHT;
            else if (tmp == "--datadir")
                nextToken = ARGV_DATA_DIR;
            else if (tmp == "-c")
                nextToken = ARGV_CONFIG_DIR;
            else if (tmp == "--noconfig")
            {
                nextToken = ARGV_NOTHING_EXPECTED;
                load_config_file = false;
            }
            else if (tmp == "--nosound")
            {
                nextToken = ARGV_NOTHING_EXPECTED;
                global->sound = 0.0;
            }
            else if (tmp == "--noname")
            {
                nextToken = ARGV_NOTHING_EXPECTED;
                global->name_above_tank = FALSE;
            }
            else if (tmp == "--nonetwork")
            {
                allow_network = false;
                nextToken = ARGV_NOTHING_EXPECTED;
            }
            else if (tmp == "--nobackground")
            {
                global->draw_background = FALSE;
                nextToken = ARGV_NOTHING_EXPECTED;
            }
            else if (tmp == "--nothread")
            {
                allow_thread = false;
                nextToken = ARGV_NOTHING_EXPECTED;
            }
            else if (tmp == "--thread")
            {
                allow_thread = true;
                nextToken = ARGV_NOTHING_EXPECTED;
            }

        }
        if (nextToken != ARGV_NOTHING_EXPECTED)
        {
            cout << "Expecting an argument to follow " << tmp << endl;
            return 0;
        }
    }

    if (!global->configDir)
    {
        global->configDir = global->Get_Config_Path();

        // copy the file over, if we did not yet
        if (!Copy_Config_File(global))
        {
            // If it did not work, look whether the directory already exists:
            DIR * pDestDir;
            pDestDir = opendir(global->configDir);
            if (!pDestDir)
                printf("An error has occured trying to set up Atomic Tank folders.\n");
            else
            {
                closedir(pDestDir);
                pDestDir = NULL;
            }
        }
    }       // end of no config file on the command line



    memset(fullPath, '\0', sizeof(fullPath));
    FILE *old_config_file = NULL;
    snprintf(fullPath, sizeof(fullPath) - 1, "%s/atanks-config.txt", global->configDir);
    if (load_config_file)
        old_config_file = fopen(fullPath, "r");
    else
        old_config_file = NULL;
    if (old_config_file)
    {
        global->loadFromFile_Text(old_config_file);
        // over-ride full screen setting with command line
        if ((full_screen == FULL_SCREEN_TRUE) || (full_screen == FULL_SCREEN_FALSE))
            global->full_screen = full_screen;
        env = init_game_settings(global);
        if (global->os_mouse)
            show_os_cursor(MOUSE_CURSOR_ARROW);
        global->Load_Text_Files();
        env->loadFromFile_Text(old_config_file);
        loadPlayers_Text(global, env, old_config_file);
        fclose(old_config_file);
        bLoadingSuccess = true;
    }

    if (!bLoadingSuccess)         // no config file found or failed to load
    {
        global->numPermanentPlayers = 0;
        if ((full_screen == FULL_SCREEN_TRUE) || (full_screen == FULL_SCREEN_FALSE))
            global->full_screen = full_screen;
        env = init_game_settings (global);
        global->Load_Text_Files();
        if (global->os_mouse) show_os_cursor(MOUSE_CURSOR_ARROW);
        char *defaultNames[] =
        {
            "Caesar",
            "Alex",
            "Hatshepsut",
            "Patton",
            "Napoleon",
            "Attila",
            "Catherine",
            "Hannibal",
            "Stalin",
            "Mao"
        };
        PLAYER *tempPlayer;
        tempPlayer = global->createNewPlayer(env);
        tempPlayer->setName(global->ingame->complete_text[52] );
        options(global, env, (MENUDESC*)tempPlayer->menudesc);
        for (int count = 0; count < 10; count++)
        {
            tempPlayer = global->createNewPlayer(env);
            tempPlayer->type = rand() % (LAST_PLAYER_TYPE - 1) + 1;
            tempPlayer->setName(defaultNames[count]);
            tempPlayer->generatePreferences();
        }
    }

    status = Load_Weapons_Text(global);
    if (!status)
    {
        printf( "An error occured trying to read weapons file.\n");
        exit(1);
    }

    snprintf(fullPath, sizeof(fullPath) - 1, "%s/atanks-config.txt", global->configDir);
    global->temp_screenWidth = global->screenWidth;
    global->temp_screenHeight = global->screenHeight;
    global->halfWidth = global->screenWidth / 2;
    global->halfHeight = global->screenHeight / 2;
    global->menuBeginY = (global->screenHeight - 400) / 2;
    if (global->menuBeginY < 0) global->menuBeginY = 0;
    global->menuEndY = global->screenHeight - global->menuBeginY;

    title(global);
    env->bitmap_filenames = Find_Bitmaps(global, &(env->number_of_bitmaps));
    env->my_sky_gradients = (const gradient **) sky_gradients;
    env->my_land_gradients = (const gradient **) land_gradients;
    Create_Music_Folder(global);
#ifdef THREADS
    if (allow_thread)
    {
        pthread_t sky_thread, terrain_thread;
        pthread_create(&sky_thread, NULL, Generate_Sky_In_Background, (void *) env);
        pthread_create(&terrain_thread, NULL, Generate_Land_In_Background, (void *) env);
    }    // end of allowing threads
#endif

    // new networking area
#ifdef THREADS
#ifdef NETWORK
    pthread_t network_thread;
    if (global->check_for_updates)
        global->update_string = Check_For_Update("projects.sourceforge.net", "version.txt", "atanks.sourceforge.net", VERSION);

    if ((global->enable_network) && allow_network)
    {
        send_receive = (SEND_RECEIVE_TYPE *) calloc(1, sizeof(SEND_RECEIVE_TYPE));
        if (!send_receive)
            printf("Could not create networking data.\n");
    }
    else
        send_receive = NULL;
    if (send_receive)
    {
        send_receive->listening_port = (int) global->listen_port;
        send_receive->global = global;
        // quit option already cleared by calloc call
        pthread_create(&network_thread, NULL, Send_And_Receive, (void *) send_receive);
    }
#endif
#endif

    do
    {
        //show the main menu
        global->wr_lock_command();
        global->command = GLOBAL_COMMAND_MENU;
        global->unlock_command();
        int signal = menu (global, env);
        if (global->client_message)
        {
            free(global->client_message);
            global->client_message = NULL;
        }

        // did the user signal to quit the game
        global->wr_lock_command();
        if (signal == SIG_QUIT_GAME)
            global->command = GLOBAL_COMMAND_QUIT;
        global->unlock_command();

        //determine which menu item is selected
        switch (global->get_command())
        {
        case GLOBAL_COMMAND_HELP:
            scrollTextList(global, env, global->instructions);
            break;
        case GLOBAL_COMMAND_OPTIONS:
            // save old settings
            temp_mouse = global->os_mouse;
            temp_sound = global->sound;

            options(global, env, NULL);
            if (!Save_Game_Settings_Text(global, env, fullPath))
            {
                perror("atanks.cpp: Failed to save game settings from atanks::main()!");
            }

            // check for changes to settings
            Change_Settings(temp_mouse, temp_sound, global->os_mouse, global->sound, global->misc[0]);
            global->Change_Font();
            global->Update_Player_Menu();
            break;
        case GLOBAL_COMMAND_PLAYERS:
            //loop until done editing players where return value is not ESC
            do
            {
                status = editPlayers(global, env);
            } while ((status != KEY_ESC << 8) && (status != KEY_ENTER << 8));
            break;
        case GLOBAL_COMMAND_CREDITS:
            credits(global, env);
            break;
        case GLOBAL_COMMAND_QUIT:
            break;
        case GLOBAL_COMMAND_NETWORK:
#ifdef NETWORK
            client_socket = Setup_Client_Socket(global->server_name, global->server_port);
            if (client_socket >= 0)
            {
                int keep_playing = TRUE;

                printf("Ready to play networked\n");
                while (keep_playing)
                    keep_playing = Game_Client(global, env, client_socket);

                Clean_Up_Client_Socket(client_socket);
            }
            else
                printf("Unable to connect to server %s, port %s.\n", global->server_name, global->server_port);
#else
            printf("This version of Atanks is not compiled to handle network games.\n");
#endif
            break;
        case GLOBAL_COMMAND_DEMO:
            global->demo_mode = true;
            global->load_game = false;
            music_place_holder = global->play_music;
            global->play_music = 0.0;

            // set up a bunch of players (non-human, less than 10)
            playerCount = 0;
            global->numPlayers = 0;
            for (player_index = 0; player_index < global->numPermanentPlayers; player_index++)
            {
                if ((global->allPlayers[player_index]->type > HUMAN_PLAYER) && (playerCount < MAXPLAYERS))
                {
                    global->addPlayer(global->allPlayers[player_index]);
                    playerCount++;
                }
            }

            player_index = (int) global->skipComputerPlay;
            global->skipComputerPlay = SKIP_NONE;
            global->currentround = 1;    // might add more later
            while ((global->currentround > 0) && (!global->close_button_pressed))
            {
                game(global, env);
                if ((global->get_command() == GLOBAL_COMMAND_QUIT) || (global->get_command() == GLOBAL_COMMAND_MENU))
                    break;
                global->currentround--;
                // skipping winner screen for now
            }
            endgame_cleanup(global, env);
            global->demo_mode = false;
            global->skipComputerPlay = player_index;
            global->play_music = music_place_holder;
            break;

        default: //must have commanded to play game
            menu_action = selectPlayers(global, env);
            if (menu_action == ESC_MENU)
                break;

            //selected players, start new game
            else
            {
                // make sure the game has a name
                if (!global->game_name[0])
                    strcpy(global->game_name,global->ingame->complete_text[53] );

                newgame(global, env);

                // play the game for the selected number of rounds
                // for (global->currentround = (int)global->rounds; global->currentround > 0; global->currentround--)
                if (!global->load_game)
                    global->currentround = (int) global->rounds;
                while((global->currentround > 0) && (!global->close_button_pressed))
                {
                    game(global, env);    // play a round
                    if (global->background_music)
                    {
                        stop_sample(global->background_music);
                        destroy_sample(global->background_music);
                        global->background_music = NULL;
                    }

                    // if user selected to quit or return to main menu during game play
                    if ((global->get_command() == GLOBAL_COMMAND_QUIT) || (global->get_command() == GLOBAL_COMMAND_MENU))
                    {
#ifdef NETWORK
                        global->Send_To_Clients("CLOSE");
#endif
                        break;
                    }

                    global->currentround--;
#ifdef NETWORK
                    if (global->currentround != 0)    // end of the round
                        global->Send_To_Clients("ROUNDEND");
#endif
                }

                // only show winner if finished all rounds
                if ((global->currentround == 0) && (!global->close_button_pressed))
                {
                    char buffer[256], *my_player;

                    // strcpy(buffer, "GAMEEND");
                    // global->Send_To_Clients(buffer);
                    my_player = do_winner(global, env);
                    if (my_player)
                    {
                        snprintf(buffer, 256, "GAMEEND The game went to %s.", my_player);
                        free(my_player);
                    }
                    else
                        strcpy(buffer, "GAMEEND");
#ifdef NETWORK
                    global->Send_To_Clients(buffer);
#endif
                    do_quote(global, env);
                }

                endgame_cleanup(global, env);
            }    // end of start new game

            break;

        }      // end of menu switch

    }
    while ((global->get_command() != GLOBAL_COMMAND_QUIT));

    // print out if there is an update
    if ((global->update_string) && (global->update_string[0]))
    {
        cout << global->update_string << endl;
        free(global->update_string);
        global->update_string = NULL;
    }

#ifdef THREADS
#ifdef NETWORK
    if (send_receive)
    {
        send_receive->shut_down = TRUE;
        // sleep(1);
        LINUX_REST;
        // we should probably wait and do a join here, but if the network thread does not
        // finish after a full second, then something has gone wrong anyway and we should move on....
        free(send_receive);
    }
#endif
#endif

    if (!Save_Game_Settings_Text(global, env, fullPath))
    {
        // This is a very critical issue, but as we are ending here, we just report it
        perror("atanks.cpp: Failed to save game settings from atanks::main()!");
        allegro_exit();
        cout << "See http://atanks.sourceforge.net for the latest news and downloads." << endl;

        return 1;
    }
    else
    {
        Save_Game_Settings_Text(global, env, fullPath);
        allegro_exit();
        cout << "See http://atanks.sourceforge.net for the latest news and downloads." << endl;

        return 0 ;
    }
}


END_OF_MAIN ()

/*
Adding function here to avoid patch incompatibilties.
This function should launch all items from all of
the living tanks.
*/
void doLaunch(GLOBALDATA *global, ENVIRONMENT *env)
{
    // If we're in simultaneous mode, launch all selections
    int i;
    int savestage = env->stage;

    if (global->turntype != TURN_SIMUL)
        return;

    for (i = 0; i < global->maxNumTanks; i++)
    {
        TANK *tank = env->order[i];
        if (tank)
        {
            if (!tank->player->skip_me)
                tank->activateCurrentSelection();
            else
                tank->player->skip_me = false;
            tank->player->time_left_to_fire = global->max_fire_time;
        }
    }

    env->stage = savestage;
}

// load a colour from a file
bool popColo(int &aData, ifstream &ifsFile)
{
    bool bResult = false;
    if (ifsFile.is_open())
    {
        int iColor = 0;
        ifsFile >> iColor; // reads the next found number
        if (ifsFile.good())
        {
            // Now transform to color:
            aData = makecol((iColor & 0x00ff0000) >> 16, (iColor & 0x0000ff00) >>  8, iColor & 0x000000ff);
            bResult = true;
        }
    }
    return bResult;
}

int *Sort_Scores(GLOBALDATA *global)
{
    static int order[MAXPLAYERS];
    int counter;
    bool made_change = true;
    int temp;

    for (counter = 0; counter < global->numPlayers; counter++)
        order[counter] = counter;

    // bubble sort
    while (made_change)
    {
        made_change = false;
        counter = 0;
        // check for swap
        while (counter < (global->numPlayers - 1))
        {
            if (global->players[order[counter]]->score < global->players[order[counter + 1]]->score)
            {
                temp = order[counter];
                order[counter] = order[counter + 1];
                order[counter + 1] = temp;
                made_change = true;
            }

            counter++;
        }    // end of check for swaps
    }         // end of bubble sort

    return order;
}

// Client version of the game
// Really, this loop should do some basic things.
// 1. Find out what the landscape should look like from the server.
// 2. Place tanks on the battle field
// 3. Create missiles, beam weapons and such when the server asks us to
// 4. Get input from the player and forward it to the server.
// 5. Clean up at the end of the round.
//
// Function return TRUE if everything went well or FALSE
// if an error occured.
#ifdef NETWORK

int Game_Client(GLOBALDATA *global, ENVIRONMENT *env, int socket_number)
{
    int surface_x = 1, tank_position = 1, team_number = 1, name_number = 1;
    int weapon_number = 1, item_number = 1, tank_health = 1;
    int end_of_round = FALSE, keep_playing = FALSE;
    int game_stage = CLIENT_VERSION;
    char buffer[BUFFER_SIZE];
    int my_key;
    int time_clock = 0;
    int screen_update = FALSE;
    int count;    // generic counter
    int stuff_going_down = FALSE;    // explosions, missiles etc on the screen
    VIRTUAL_OBJECT *my_object;
    BEAM *beam;
    EXPLOSION *explosion;
    MISSILE *missile;
    TELEPORT *teleport;
    FLOATTEXT *floattext;
    int my_class;
    bool fired = false;
    

    global->dMaxVelocity = (double) MAX_POWER * (100.0 / (double) global->frames_per_second) / 100.0;
    clear_to_color(env->terrain, PINK);    // get terrain ready
    clear_to_color(env->db, BLACK);

    // clean up old text
    for (count = 0; (floattext = (FLOATTEXT*) env->getNextOfClass(FLOATTEXT_CLASS, &count)) && floattext; count++)
    {
        floattext->newRound();
        delete floattext;
    }

    Create_Sky(env, global);    // so we have a background
    strcpy(buffer, "VERSION");
    write(socket_number, buffer, strlen(buffer));

    while (!end_of_round)
    {
        // check for waiting input from the server
        int incoming = Check_For_Incoming_Data(socket_number);
        int object_count;

        if (incoming)
        {
            int bytes_read;

            memset(buffer, '\0', BUFFER_SIZE);
            bytes_read = read(socket_number, buffer, BUFFER_SIZE);
            if (bytes_read > 0)
            {
                // do something with this input
                if (!strncmp(buffer, "CLOSE", 5))
                {
                    end_of_round = TRUE;
                    keep_playing = FALSE;
                    printf("Got close message.\n");
                    global->client_message = global->ingame->complete_text[81];
                }
                else if (!strncmp(buffer, "NOROOM", 6))
                {
                    end_of_round = TRUE;
                    keep_playing = FALSE;
                    printf("The server is full or the game has not started. Please try again later.\n");
                    global->client_message = global->ingame->complete_text[80];
                }
                else if (!strncmp(buffer, "GAMEEND", 7))
                {
                    end_of_round = TRUE;
                    keep_playing = FALSE;
                    printf("The game is over.\n");
                    if ( strlen(buffer) > 7)
                        global->client_message = strdup(& (buffer[8])) ;
                    else
                        global->client_message = strdup(global->ingame->complete_text[82]);
                }
                else if (!strncmp(buffer, "ROUNDEND", 8))
                {
                    end_of_round = TRUE;
                    keep_playing = TRUE;
                    printf("Round is over.\n");
                }

                else    // not a special command, parse it
                {
                    if (Parse_Client_Data(global, env, buffer))
                    {
                        if (game_stage < CLIENT_PLAYING)
                            game_stage++;

                        // Request more information
                        if (game_stage < CLIENT_PLAYING)
                        {
                            switch (game_stage)
                            {
                            case CLIENT_SCREEN:
                                strcpy(buffer, "SCREEN");
                                break;
                            case CLIENT_WIND:
                                strcpy(buffer, "WIND");
                                break;
                            case CLIENT_NUMPLAYERS:
                                strcpy(buffer, "NUMPLAYERS");
                                break;
                            case CLIENT_TANK_POSITION:
                                strcpy(buffer, "TANKPOSITION 0");
                                break;
                            case CLIENT_SURFACE:
                                strcpy(buffer, "SURFACE 0");
                                break;
                            case CLIENT_WHOAMI:
                                strcpy(buffer, "WHOAMI");
                                screen_update = TRUE;
                                break;
                            case CLIENT_WEAPONS:
                                strcpy(buffer, "WEAPON 0");
                                break;
                            case CLIENT_ITEMS:
                                strcpy(buffer, "ITEM 0");
                                break;
                            case CLIENT_ROUNDS:
                                strcpy(buffer, "ROUNDS");
                                break;
                            case CLIENT_TEAMS:
                                strcpy(buffer, "TEAMS 0");
                                global->updateMenu = TRUE;
                                break;
                            case CLIENT_WALL_TYPE:
                                strcpy(buffer, "WALLTYPE");
                                break;
                            case CLIENT_BOXED:
                                strcpy(buffer, "BOXED");
                                break;
                            case CLIENT_NAME:
                                strcpy(buffer, "PLAYERNAME 0");
                                break;
                            case CLIENT_TANK_HEALTH:
                                strcpy(buffer, "HEALTH 0");
                                break;
                            default:
                                buffer[0] = '\0';
                            }
                            write(socket_number, buffer, strlen(buffer));
                        }   // end of getting more info
                    }    // our game stage went up
                    else  // we got data, but our game stage did not go up
                    {
                        if (fired)
                        {
                            if ((global->client_player) && (global->client_player->tank))
                            {
                                fired = false;
                                if (global->client_player->tank->cw < WEAPONS)
                                    sprintf(buffer, "WEAPON %d", global->client_player->tank->cw);
                                else
                                    sprintf(buffer, "ITEM %d", global->client_player->tank->cw - WEAPONS);
                                write(socket_number, buffer, strlen(buffer));
                            }
                        }
                        else if (game_stage == CLIENT_SURFACE)
                        {
                            sprintf(buffer, "SURFACE %d", surface_x);
                            write(socket_number, buffer, strlen(buffer));
                            surface_x++;
                        }
                        else if (game_stage == CLIENT_ITEMS)
                        {
                            sprintf(buffer, "ITEM %d", item_number);
                            write(socket_number, buffer, strlen(buffer));
                            item_number++;
                        }
                        else if (game_stage == CLIENT_TANK_POSITION)
                        {
                            sprintf(buffer, "TANKPOSITION %d", tank_position);
                            write(socket_number, buffer, strlen(buffer));
                            tank_position++;
                            if (tank_position >= global->numPlayers)
                                tank_position = 0;
                        }
                        else if (game_stage == CLIENT_TANK_HEALTH)
                        {
                            sprintf(buffer, "HEALTH %d", tank_health);
                            write(socket_number, buffer, strlen(buffer));
                            tank_health++;
                            if (tank_health >= global->numPlayers)
                                tank_health = 0;
                        }
                        else if (game_stage == CLIENT_TEAMS)
                        {
                            sprintf(buffer, "TEAMS %d", team_number);
                            write(socket_number, buffer, strlen(buffer));
                            team_number++;
                        }
                        else if (game_stage == CLIENT_NAME)
                        {
                            sprintf(buffer, "PLAYERNAME %d", name_number);
                            write(socket_number, buffer, strlen(buffer));
                            name_number++;
                        }
                        else if (game_stage == CLIENT_WEAPONS)
                        {
                            sprintf(buffer, "WEAPON %d", weapon_number);
                            write(socket_number, buffer, strlen(buffer));
                            weapon_number++;
                        }
                        else if (game_stage == CLIENT_PLAYING)
                        {
                            time_clock++;
                            if (time_clock > 1)    // check positions every few inputs
                            {
                                time_clock = 0;
                                if (surface_x < global->screenWidth)
                                {
                                    game_stage = CLIENT_SURFACE;
                                    sprintf(buffer, "SURFACE %d", surface_x);
                                    write(socket_number, buffer, strlen(buffer));
                                    surface_x++;
                                }
                                else
                                {
                                    game_stage = CLIENT_TANK_POSITION;
                                    tank_position = 1;
                                    strcpy(buffer, "TANKPOSITION 0");
                                    write(socket_number, buffer, strlen(buffer));
                                }    // game stage stuff
                            }
                        }    // end of playing commands
                    }

                }    // end of we got something besides the close command

            }
            else    // connection was broken
            {
                close(socket_number);
                printf("Server closed connection.\n");
                end_of_round = TRUE;
            }
        }

        object_count = 0;
        while (object_count < MAX_OBJECTS)
        {
            my_object = env->objects[object_count];
            if (my_object)
            {
                my_class = my_object->getClass();
                if (my_class == BEAM_CLASS)
                {
                    beam = (BEAM *) my_object;
                    beam->applyPhysics();
                    if (beam->destroy)
                    {
                        beam->requireUpdate();
                        beam->update();
                        delete beam;
                    }
                    stuff_going_down = TRUE;
                }
                else if (my_class == MISSILE_CLASS)
                {
                    missile = (MISSILE *) my_object;
                    missile->hitSomething = 0;
                    missile->applyPhysics();
                    missile->triggerTest();
                    if (missile->destroy)
                    {
                        missile->requireUpdate();
                        missile->update();
                        delete missile;
                    }
                    stuff_going_down = TRUE;
                }
                else if (my_class == EXPLOSION_CLASS)
                {
                    explosion = (EXPLOSION *) my_object;
                    explosion->explode();
                    explosion->applyPhysics();
                    if (explosion->destroy)
                    {
                        explosion->requireUpdate();
                        explosion->update();
                        delete explosion;
                    }
                    stuff_going_down = TRUE;
                }
                else if (my_class == TELEPORT_CLASS)
                {
                    teleport = (TELEPORT *) my_object;
                    teleport->applyPhysics();
                    if (teleport->destroy)
                    {
                        teleport->requireUpdate();
                        teleport->update();
                        delete teleport;
                        time_clock = 2;
                    }
                    stuff_going_down = TRUE;
                }
                else if (my_class == FLOATTEXT_CLASS)
                {
                    floattext = (FLOATTEXT *) my_object;
                    floattext->applyPhysics();
                    if (floattext->destroy)
                    {
                        floattext->requireUpdate();
                        floattext->update();
                        delete floattext;
                    }
                }
                else if (my_class == DECOR_CLASS)
                {
                    DECOR *decor = (DECOR *) my_object;
                    decor->applyPhysics();
                    if (decor->destroy)
                    {
                        decor->requireUpdate();
                        decor->update();
                        delete decor;
                    }
                }
            }      // end of got valid object
            object_count++;
        }

        slideLand(global, env);

        // update everything on the screen
        if (global->updateMenu)
        {
            set_clip_rect(env->db, 0, 0, (global->screenWidth-1), MENUHEIGHT - 1);
            drawTopBar(global, env, env->db);
        }

        set_clip_rect(env->db, 0, MENUHEIGHT, (global->screenWidth-1), (global->screenHeight-1));
        if (screen_update)
        {
            blit(env->sky, env->db, global->window.x, global->window.y - MENUHEIGHT, global->window.x, global->window.y, (global->window.w - global->window.x) + 1, (global->window.h - global->window.y) + 1);
            masked_blit(env->terrain, env->db, global->window.x, global->window.y, global->window.x, global->window.y, (global->window.w - global->window.x) + 1, (global->window.h - global->window.y) + 2);
            drawTopBar(global, env, env->db);
            int iLeft = 0;
            int iRight = global->screenWidth - 1;
            int iTop = MENUHEIGHT;
            int iBottom = global->screenHeight - 1;
            set_clip_rect(env->db, 0, 0, iRight, iBottom);
            vline(env->db, iLeft, iTop, iBottom, env->wallColour);    // Left edge
            vline(env->db, iRight, iTop, iBottom, env->wallColour);    // right edge
            hline(env->db, iLeft, iBottom, iRight, env->wallColour);    // bottom edge
            if (global->bIsBoxed)
                hline(env->db, iLeft, iTop, iRight, env->wallColour);    // top edge

            env->make_update(0, 0, global->screenWidth, global->screenHeight);
        }
        else
            env->replaceCanvas();

        for (count = 0; count < global->numPlayers; count++)
        {
            if ((global->players[count]) && (global->players[count]->tank))
            {
                global->players[count]->tank->drawTank(env->db, 0);
                global->players[count]->tank->update();
            }
        }
        // env->do_updates();
        screen_update = TRUE;

        object_count = 0;
        while (object_count < MAX_OBJECTS)
        {
            my_object = env->objects[object_count];
            if (my_object)
            {
                my_class = my_object->getClass();

                if (my_class == BEAM_CLASS)
                {
                    beam = (BEAM *) my_object;
                    beam->draw(env->db);
                    beam->update();
                }
                else if (my_class == MISSILE_CLASS)
                {
                    missile = (MISSILE *) my_object;
                    missile->draw(env->db);
                    missile->update();
                }
                else if (my_class == EXPLOSION_CLASS)
                {
                    explosion = (EXPLOSION *) my_object;
                    explosion->draw(env->db);
                    explosion->update();
                }
                else if (my_class == TELEPORT_CLASS)
                {
                    teleport = (TELEPORT *) my_object;
                    if (teleport->object)
                        teleport->draw(env->db);
                    teleport->update();
                }
                else if (my_class == DECOR_CLASS)
                {
                    DECOR *decor = (DECOR *) my_object;
                    decor->draw(env->db);
                    decor->update();
                }
                else if (my_class == FLOATTEXT_CLASS)
                {
                    floattext = (FLOATTEXT *) my_object;
                    floattext->draw (env->db);
                    floattext->requireUpdate();
                    floattext->update();
                }
            }    // end of valid object
            object_count++;
        }    // end of while updating objects
        env->do_updates();

        
        // check for input from the user
        if (keypressed())
        {
            my_key = readkey();
            my_key = my_key >> 8;
            if (my_key == KEY_SPACE)
            {
                Client_Fire(global->client_player, socket_number);
                fired = true;
            }
            else if (my_key == KEY_ESC)
            {
                end_of_round = TRUE;
                close(socket_number);
            }
            else if (my_key == KEY_UP)
                Client_Power(global->client_player, CLIENT_UP);
            else if (my_key == KEY_DOWN)
                Client_Power(global->client_player, CLIENT_DOWN);
            else if (my_key == KEY_LEFT)
                Client_Angle(global->client_player, CLIENT_LEFT);
            else if (my_key == KEY_RIGHT)
                Client_Angle(global->client_player, CLIENT_RIGHT);
            else if ((my_key == KEY_Z) || (my_key == KEY_BACKSPACE))
                Client_Cycle_Weapon(global->client_player, CYCLE_BACK);
            else if ((my_key == KEY_C) || (my_key == KEY_TAB))
            {
                Client_Cycle_Weapon(global->client_player, CYCLE_FORWARD);
                global->updateMenu = TRUE;
            }

            screen_update = FALSE;
            global->updateMenu = TRUE;
        }

        // pause for a moment
        // if (game_stage < CLIENT_PLAYING)
        if (stuff_going_down)
        {
            LINUX_SLEEP;
            stuff_going_down = FALSE;
        }
    }

    // we should clean up here
    for (count = 0; count < global->numPlayers; count++)
    {
        if (global->players[count]->tank)
        {
            delete global->players[count]->tank;
            global->players[count]->tank = NULL;
        }
    }

    return keep_playing;
}

#endif
