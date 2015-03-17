/*
 * atanks - obliterate each other with oversize weapons
 * Copyright (C) 2003  Thomas Hudson
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


#include "environment.h"
#include "globaldata.h"
#include "virtobj.h"
#include "missile.h"
#include "tank.h"
#include "files.h"

ENVIRONMENT::ENVIRONMENT(GLOBALDATA *global) : _global(global),done(NULL),fp(NULL),h(NULL),surface(NULL),dropTo(NULL),
                         velocity(NULL),dropIncr(NULL),height(NULL),db(NULL),terrain(NULL),sky(NULL)
{
    gravity = 0.150;
    viscosity = 0.5;
    landSlideType = LANDSLIDE_GRAVITY;
    landSlideDelay = MAX_GRAVITY_DELAY;
    windstrength = 8;
    windvariation = 1;
    weapontechLevel = itemtechLevel = 5;
    landType = LANDTYPE_RANDOM;
    meteors = 0;
    lightning = 0;
    satellite = 0.0;
    falling_dirt_balls = 0.0;
    fog = 0;
    wallType = 0.0;
    dBoxedMode = 0.0; // default is 0 = off
    dFadingText = 0.0; // defaults to 0 for backwards compatibilities sake
    dShadowedText = 0.0; // defaults to 0 for backwards compatibilities sake
    current_wallType = 0;
    custom_background = 0.0;
    bitmap_filenames = NULL;
    number_of_bitmaps = 0;
    current_drawing_mode = DRAW_MODE_SOLID;

    done = new char[global->screenWidth];
    fp = new int[global->screenWidth];
    h = new int[global->screenWidth];
    surface = new int[global->screenWidth];
    dropTo = new int[global->screenWidth];
    velocity = new double[global->screenWidth];
    dropIncr = new double[global->screenWidth];
    height = new double[global->screenWidth];

    db = create_bitmap (global->screenWidth, global->screenHeight);
    if (!db)
        cout << "Failed to create db bitmap: " << allegro_error;
    terrain = create_bitmap (global->screenWidth, global->screenHeight);
    if (!terrain)
        cout << "Failed to create terrain bitmap: " << allegro_error;
    sky = create_bitmap (global->screenWidth, global->screenHeight - MENUHEIGHT);
    if (!sky)
        cout << "Failed to create sky bitmap: " << allegro_error;
    waiting_sky = NULL;
    waiting_terrain = NULL;
    _global = global;
    global_tank_index = 0;
    volume_factor = MAX_VOLUME_FACTOR;

    initialise();
#ifdef THREADS
    waiting_sky_lock = init_lock(waiting_sky_lock);
    waiting_terrain_lock = init_lock(waiting_terrain_lock);
#endif
}

/** @brief ~ENVIRONMENT default dtor
 *  *   *
 *   *     * Cleanly remove created objects
 *    *       */
ENVIRONMENT::~ENVIRONMENT()
{
#ifdef THREADS
    destroy_lock(waiting_sky_lock);
    destroy_lock(waiting_terrain_lock);
#endif

    if (db)      destroy_bitmap(db);      db      = NULL;
    if (sky)     destroy_bitmap(sky);     sky     = NULL;
    if (waiting_sky) destroy_bitmap(sky); waiting_sky = NULL;
    if (terrain) destroy_bitmap(terrain); terrain = NULL;

    _global = NULL;

    if (done)     delete [] (done);     done = NULL;
    if (fp)       delete [] (fp);       fp = NULL;
    if (h)        delete [] (h);        h = NULL;
    if (surface)  delete [] (surface);  surface = NULL;
    if (dropTo)   delete [] (dropTo);   dropTo = NULL;
    if (velocity) delete [] (velocity); velocity = NULL;
    if (dropIncr) delete [] (dropIncr); dropIncr = NULL;
    if (height)   delete [] (height);   height = NULL;
    if (bitmap_filenames)
    {
        for (int count = 0; count < number_of_bitmaps; count++)
        {
            if (bitmap_filenames[count])
                free(bitmap_filenames[count]);
        }
        free(bitmap_filenames);
    }
}
#ifdef THREADS
void ENVIRONMENT::destroy_lock(pthread_mutex_t* lock)
{
    if (lock)
    {
        int result = pthread_mutex_destroy(lock);
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
        free(lock);
    }
}
#endif
#ifdef THREADS
pthread_mutex_t* ENVIRONMENT::init_lock(pthread_mutex_t* lock)
{
    lock = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
    if (!lock)
    {
        printf("%s:%i: Could not allocate memory.\n", __FILE__, __LINE__);
    }
    int result = pthread_mutex_init(lock, NULL);
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
    return lock;
}
#endif
#ifdef THREADS
void ENVIRONMENT::lock(pthread_mutex_t* lock)
{
    int result = pthread_mutex_lock(lock);
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
}
#endif
#ifdef THREADS
void ENVIRONMENT::unlock(pthread_mutex_t* lock)
{
    int result = pthread_mutex_unlock(lock);
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
        printf("%s:%i: waiting_sky_lock is uninitialized.\n", __FILE__, __LINE__);
        break;
    default:
        //?
        printf("%s:%i: Unknown error code (%i) returned by pthread_mutex_unlock.\n", __FILE__, __LINE__, result);
        break;
    }
}
#endif
BITMAP* ENVIRONMENT::get_waiting_sky()
{
#ifdef THREADS
    lock(waiting_sky_lock);
#endif
    BITMAP* w = waiting_sky;
#ifdef THREADS
    unlock(waiting_sky_lock);
#endif
    return w;
}

BITMAP* ENVIRONMENT::get_waiting_terrain()
{
#ifdef THREADS
    lock(waiting_terrain_lock);
#endif
    BITMAP* w = waiting_terrain;
#ifdef THREADS
    unlock(waiting_terrain_lock);
#endif
    return w;
}

/*
This function saves the environment settings to a text
file. Each line has the format
name=value\n

The function returns TRUE on success and FALSE on failure.
-- Jesse
*/
int ENVIRONMENT::saveToFile_Text(FILE *file)
{
    if (!file)
        return FALSE;

    fprintf(file, "*ENV*\n");

    fprintf(file, "WINDSTRENGTH=%f\n", windstrength);
    fprintf(file, "WINDVARIATION=%f\n", windvariation);
    fprintf(file, "VISCOSITY=%f\n", viscosity);
    fprintf(file, "GRAVITY=%f\n", gravity);
    fprintf(file, "WEAPONTECHLEVEL=%f\n", weapontechLevel);
    fprintf(file, "ITEMTECHLEVEL=%f\n", itemtechLevel);
    fprintf(file, "METEORS=%f\n", meteors);
    fprintf(file, "LIGHTNING=%f\n", lightning);
    fprintf(file, "SATELLITE=%f\n", satellite);
    fprintf(file, "FOG=%f\n", fog);
    fprintf(file, "LANDTYPE=%f\n", landType);
    fprintf(file, "LANDSLIDETYPE=%f\n", landSlideType);
    fprintf(file, "WALLTYPE=%f\n", wallType);
    fprintf(file, "BOXMODE=%f\n", dBoxedMode);
    fprintf(file, "TEXTFADE=%f\n", dFadingText);
    fprintf(file, "TEXTSHADOW=%f\n", dShadowedText);
    fprintf(file, "LANDSLIDEDELAY=%f\n", landSlideDelay);
    fprintf(file, "FALLINGDIRTBALLS=%f\n", falling_dirt_balls);
    fprintf(file, "CUSTOMBACKGROUND=%f\n", custom_background);
    fprintf(file, "***\n");
    return TRUE;
}

/*
This function loads environment settings from a text
file. The function returns TRUE on success and FALSE if
any erors are encountered.
-- Jesse
*/
int ENVIRONMENT::loadFromFile_Text(FILE *file)
{
    char line[MAX_CONFIG_LINE];
    int equal_position, line_length;
    char field[MAX_CONFIG_LINE], value[MAX_CONFIG_LINE];
    char *result = NULL;
    bool done = false;

    // read until we hit line (char *)"*ENV*" or "***" or EOF
    do
    {
        result = fgets(line, MAX_CONFIG_LINE, file);
        if (!result)     // eof
            return FALSE;
        if (!strncmp(line, "***", 3))     // end of record
            return FALSE;
    }
    while (strncmp(line, "*ENV*", 5));     // read until we hit new record

    while ((result) && (!done))
    {
        // read a line
        memset(line, '\0', MAX_CONFIG_LINE);
        result = fgets(line, MAX_CONFIG_LINE, file);
        if (result)
        {
            // if we hit end of the record, stop
            if (!strncmp(line, "***", 3))
                return TRUE;
            // find equal sign
            line_length = strlen(line);
            // strip newline character
            if (line[line_length - 1] == '\n')
            {
                line[line_length - 1] = '\0';
                line_length--;
            }
            equal_position = 1;
            while (( equal_position < line_length) && (line[equal_position] != '='))
                equal_position++;
            // make sure we have valid equal sign

            if (equal_position <= line_length)
            {
                // seperate field from value
                memset(field, '\0', MAX_CONFIG_LINE);
                memset(value, '\0', MAX_CONFIG_LINE);
                strncpy(field, line, equal_position);
                strcpy(value, & (line[equal_position + 1]));
                // check for fields and values
                if (!strcasecmp(field, "windstrength"))
                    sscanf(value, "%lf", &windstrength);
                else if (!strcasecmp(field, "windvariation"))
                    sscanf(value, "%lf", &windvariation);
                else if (!strcasecmp(field, "viscosity"))
                    sscanf(value, "%lf", &viscosity);
                else if (!strcasecmp(field, "gravity"))
                    sscanf(value, "%lf", &gravity);
                else if (!strcasecmp(field, "techlevel"))
                {
                    sscanf(value, "%lf", &weapontechLevel);
                    itemtechLevel = weapontechLevel;    // for backward compatibility
                }
                else if (!strcasecmp(field, "weapontechlevel"))
                    sscanf(value, "%lf", &weapontechLevel);
                else if (!strcasecmp(field, "itemtechlevel"))
                    sscanf(value, "%lf", &itemtechLevel);
                else if (!strcasecmp(field, "meteors"))
                    sscanf(value, "%lf", &meteors);
                else if (!strcasecmp(field, "lightning"))
                    sscanf(value, "%lf", &lightning);
                else if (!strcasecmp(field, "satellite"))
                    sscanf(value, "%lf", &satellite);
                else if (!strcasecmp(field, "fog"))
                    sscanf(value, "%lf", &fog);
                else if (!strcasecmp(field, "landtype"))
                    sscanf(value, "%lf", &landType);
                else if (!strcasecmp(field, "landslidetype"))
                    sscanf(value, "%lf", &landSlideType);
                else if (!strcasecmp(field, "walltype"))
                    sscanf(value, "%lf", &wallType);
                else if (!strcasecmp(field, "boxmode"))
                    sscanf(value, "%lf", &dBoxedMode);
                else if (!strcasecmp(field, "textfade"))
                    sscanf(value, "%lf", &dFadingText);
                else if (!strcasecmp(field, "textshadow"))
                    sscanf(value, "%lf", &dShadowedText);
                else if (!strcasecmp(field, "landslidedelay"))
                    sscanf(value, "%lf", &landSlideDelay);
                else if (!strcasecmp(field, "fallingdirtballs"))
                    sscanf(value, "%lf", &falling_dirt_balls);
                else if (!strcasecmp(field, "custombackground"))
                    sscanf(value, "%lf", &custom_background);

            }    // end of found field=value line

        }     // end of read a line properly
    }     // end of while not done

    return TRUE;
}

void ENVIRONMENT::initialise()
{
    for (int count = 0; count < _global->screenWidth; count++)
    {
        h[count] = 0;
        fp[count] = 0;
        done[count] = 1;
        dropTo[count] = _global->screenHeight - 1;
        surface[count] = 0;
    }
    for (int count = 0; count < MAX_OBJECTS; count++)
        objects[count] = NULL;
    for (int count = 0; count < MAXPLAYERS; count++)
    {
        order[count] = NULL;
        playerOrder[count] = NULL;
    }

    clear_to_color(sky, PINK);
    clear_to_color(db, WHITE);
    clear_to_color(terrain, PINK);

    // oldFogX = 0;
    // oldFogY = 0;

    combineUpdates = TRUE;
}

int ENVIRONMENT::isItemAvailable(int itemNum)
{
    if (itemNum < WEAPONS)
    {
        if ((weapon[itemNum].warhead) || (weapon[itemNum].techLevel > weapontechLevel))
            return (FALSE);
    }
    else if (item[itemNum - WEAPONS].techLevel > itemtechLevel)
        return FALSE;

    return TRUE;
}

void ENVIRONMENT::generateAvailableItemsList()
{
    int slot = 0;
    for (int itemNum = 0; itemNum < THINGS; itemNum++)
    {
        if (!isItemAvailable (itemNum))
            continue;
        availableItems[slot] = itemNum;
        slot++;
    }
    numAvailable = slot;
}

int ENVIRONMENT::addObject(VIRTUAL_OBJECT *object)
{
    int objCount = 0;

    while ((objects[objCount] != NULL) && (objCount < MAX_OBJECTS))
        objCount++;
    if (objCount < MAX_OBJECTS)
        objects[objCount] = object;

    return objCount;
}

int ENVIRONMENT::removeObject(VIRTUAL_OBJECT *object)
{
    int objCount = object->getIndex();
    int objClass = object->getClass();

    object->requireUpdate();
    object->update();

    if (objCount < MAX_OBJECTS)
    {
        objects[objCount] = NULL;
        if (objClass == TANK_CLASS)
            for (int ordCount = 0; ordCount < MAXPLAYERS; ordCount++)
                if (order[ordCount] == object)
                    order[ordCount] = NULL;
    }
    else
        return (objCount);

    return 0;
}

VIRTUAL_OBJECT *ENVIRONMENT::getNextOfClass(int classNum, int *objCount)
{
    for (;*objCount < MAX_OBJECTS; (*objCount)++)
    {
        if ( (objects[*objCount] != NULL) && ((classNum == ANY_CLASS)
             || (classNum == objects[*objCount]->getClass())) )
            break;
    }

    if (*objCount < MAX_OBJECTS)
        return objects[*objCount];
    else
        return NULL;
}

void ENVIRONMENT::newRound()
{
    for (int objCount = 0; objCount < MAX_OBJECTS; objCount++)
    {
        if (objects[objCount])
        {
            if ( (!objects[objCount]->isSubClass(TANK_CLASS))
                 && (!objects[objCount]->isSubClass(FLOATTEXT_CLASS)) )
            {
                delete objects[objCount];
                objects[objCount] = NULL;
            }
            else if (objects[objCount]->isSubClass(TANK_CLASS))
                ((TANK *) objects[objCount])->newRound();
        }
    }

    naturals_since_last_shot = 0;
    // set wall type
    if (wallType == WALL_RANDOM)
        current_wallType = rand() % 4;
    else
        current_wallType = (int) wallType;

    // time_to_fall = (rand() % MAX_GRAVITY_DELAY) + 1;
    time_to_fall = (rand() & (int) landSlideDelay) + 1;
    global_tank_index = 0;
}

int ENVIRONMENT::ingamemenu()
{
    int button[INGAMEBUTTONS];
    int pressed = -1;
    int updatew[INGAMEBUTTONS];
    char *buttext[INGAMEBUTTONS];
    buttext[0] = _global->ingame->complete_text[69];
    buttext[1] =  _global->ingame->complete_text[70];
    buttext[2] = _global->ingame->complete_text[71];
    buttext[3] =  _global->ingame->complete_text[72];
    // char buttext_de[INGAMEBUTTONS][32] = { "Zuruck", "Hauptmenu", "Verlassen", "Skip AI" };
    int z, zz;

    // Set/calcualte button size and positions
    int button_width = 150;
    int button_height = 20;
    int button_space = 5;
    int button_halfwidth = button_width / 2;
    // int button_halfheight = button_height / 2;
    int dialog_width = 200;
    int dialog_height = ((INGAMEBUTTONS + 2) * button_height) + ((INGAMEBUTTONS + 1) * button_space);
    int dialog_halfwidth = dialog_width / 2;
    int dialog_halfheight = dialog_height / 2;

    // Calculate button y values and set all button status to 0
    int y = -dialog_halfheight + button_height + button_space;

    for (z = 0; z < INGAMEBUTTONS; z++)
    {
        updatew[z] = 0;
        button[z] = y;
        y += button_height + button_space;
    }

    if (!_global->os_mouse)
        show_mouse(NULL);
    make_update(_global->halfWidth - dialog_halfwidth, _global->halfHeight - dialog_halfheight, dialog_width, dialog_height);
    rectfill(db, _global->halfWidth - dialog_halfwidth, _global->halfHeight - dialog_halfheight, _global->halfWidth + dialog_halfwidth - 1, _global->halfHeight + dialog_halfheight - 1, makecol (128, 128, 128));
    rect(db, _global->halfWidth - dialog_halfwidth, _global->halfHeight - dialog_halfheight, _global->halfWidth + dialog_halfwidth - 1, _global->halfHeight + dialog_halfheight - 1, BLACK);

    while (1)
    {
        LINUX_SLEEP;
        if (keypressed())
        {
            k = readkey();
            if ((k >> 8 == KEY_ESC) || (k >> 8 == KEY_P))
                return 0;
        }

        if (mouse_b & 1)
        {
            zz = 0;
            for (z = 0; z < INGAMEBUTTONS; z++)
            {
                if (mouse_x >= _global->halfWidth - button_halfwidth && mouse_x < _global->halfWidth + button_halfwidth
                    && mouse_y >= button[z] + _global->halfHeight && mouse_y < button[z] + button_height + _global->halfHeight)
                {
                    zz = 1;
                    if (pressed > -1)
                        updatew[pressed] = 1;
                    pressed = z;
                    updatew[z] = 1;
                }
            }
            if (!zz)
            {
                if (pressed > -1)
                    updatew[pressed] = 1;
                pressed = -1;
            }
        }
        if (pressed > -1 && !mouse_b & 1)
            return pressed;
        for (z = 0; z < INGAMEBUTTONS; z++)
        {
            if (updatew[z])
            {
                updatew[z] = 0;
                make_update(_global->halfWidth - button_halfwidth, _global->halfHeight + button[z], button_width, button_height);
            }
        }
        make_update(mouse_x, mouse_y, ((BITMAP *) (_global->misc[0]))->w, ((BITMAP *) (_global->misc[0]))->h);
        make_update(lx, ly, ((BITMAP *) (_global->misc[0]))->w, ((BITMAP *) (_global->misc[0]))->h);
        lx = mouse_x;
        ly = mouse_y;
        if (! _global->os_mouse)
            show_mouse(NULL);
        for (z = 0; z < INGAMEBUTTONS; z++)
        {
            draw_sprite(db, (BITMAP *) _global->misc[(pressed == z) ? 8 : 7], _global->halfWidth - button_halfwidth, _global->halfHeight + button[z]);
            textout_centre_ex(db, font, buttext[z], _global->halfWidth, _global->halfHeight + button[z] + 6, WHITE, -1);
        }
        if (!_global->os_mouse)
            show_mouse(db);
        do_updates();
    }
}

void ENVIRONMENT::do_updates()
{
    bool bIsBgUpdNeeded = false;
    if (_global->lastUpdatesCount)
        bIsBgUpdNeeded = true;

    if (_global->updateCount >= MAXUPDATES)
        make_fullUpdate();
    else
    {
        for (int count = 0; count < _global->updateCount && count < MAXUPDATES; count++)
        {
            blit(db, screen, _global->updates[count].x, _global->updates[count].y, _global->updates[count].x, _global->updates[count].y, _global->updates[count].w, _global->updates[count].h);
            // Debug rectangle
            //rect (screen, _global->updates[count].x, _global->updates[count].y, _global->updates[count].x + _global->updates[count].w, _global->updates[count].y + _global->updates[count].h, WHITE);
            if (bIsBgUpdNeeded)
                make_bgupdate(_global->updates[count].x, _global->updates[count].y, _global->updates[count].w, _global->updates[count].h);
        }

        if (!bIsBgUpdNeeded)
        {
            _global->lastUpdatesCount = _global->updateCount;
            memcpy(_global->lastUpdates, _global->updates, sizeof (BOX) * _global->updateCount);
        }
        _global->updateCount = 0;
    }
}

void ENVIRONMENT::replaceCanvas()
{
    if (_global->lastUpdatesCount >= MAXUPDATES)
        make_fullUpdate();
    else
    {
        for (int count = 0; count < _global->lastUpdatesCount && count < MAXUPDATES; count++)
        {
            blit(sky, db, _global->lastUpdates[count].x, _global->lastUpdates[count].y - MENUHEIGHT, _global->lastUpdates[count].x, _global->lastUpdates[count].y, _global->lastUpdates[count].w, _global->lastUpdates[count].h);
            masked_blit(terrain, db, _global->lastUpdates[count].x, _global->lastUpdates[count].y, _global->lastUpdates[count].x, _global->lastUpdates[count].y, _global->lastUpdates[count].w, _global->lastUpdates[count].h);
        }

        int iLeft = 0;
        int iRight = _global->screenWidth - 1;
        int iTop = MENUHEIGHT;
        int iBottom = _global->screenHeight - 1;

        vline(db, iLeft, iTop, iBottom, wallColour);    // Left edge
        vline(db, iLeft + 1, iTop, iBottom, wallColour);    // Left edge
        vline(db, iRight, iTop, iBottom, wallColour);   // right edge
        vline(db, iRight - 1, iTop, iBottom, wallColour);   // right edge
        hline(db, iLeft, iBottom, iRight, wallColour);// bottom edge
        if (_global->bIsBoxed)
            hline(db, iLeft, iTop, iRight, wallColour);// top edge

        _global->lastUpdatesCount = 0;
        // The menu needs to be redrawn:
        make_update(0, 0, _global->screenWidth - 1, MENUHEIGHT);
    }
}

void ENVIRONMENT::make_update(int x, int y, int w, int h)
{
    int combined = 0;
    if (combineUpdates && _global->updateCount && _global->updateCount < MAXUPDATES)
    {
        BOX prev, next;
        prev.x = _global->updates[_global->updateCount - 1].x;
        prev.y = _global->updates[_global->updateCount - 1].y;
        // .w = .x + .w = x2 (aka renamed to be the right x-position)
        prev.w = _global->updates[_global->updateCount - 1].x + _global->updates[_global->updateCount - 1].w;
        // .h = .y + .h = y2 (aka renamed to be the bottom y-position)
        prev.h = _global->updates[_global->updateCount - 1].y + _global->updates[_global->updateCount - 1].h;

        next.x = x;
        next.y = y;
        next.w = x + w; // same as above, becoming x2 and not the width!
        next.h = y + h; // same as above, becoming y2 and not the height!

        if ( ((next.w > prev.x - 3) && (prev.w > next.x - 3))
             && ((next.h > prev.y - 3) && (prev.h > next.y - 3)) )
        {
            next.x = (next.x < prev.x)?next.x:prev.x;
            next.y = (next.y < prev.y)?next.y:prev.y;
            next.w = (next.w > prev.w)?next.w:prev.w;
            next.h = (next.h > prev.h)?next.h:prev.h;
            _global->updates[_global->updateCount - 1].x = next.x;
            _global->updates[_global->updateCount - 1].y = next.y;
            _global->updates[_global->updateCount - 1].w = next.w - next.x;
            _global->updates[_global->updateCount - 1].h = next.h - next.y;
            combined = 1;
        }
    }
    if (!combined)
    {
        _global->updates[_global->updateCount].x = x;
        _global->updates[_global->updateCount].y = y;
        _global->updates[_global->updateCount].w = w;
        _global->updates[_global->updateCount].h = h;
        if (_global->updateCount < MAXUPDATES)
            _global->updateCount++;
    }

    if (_global->updateCount >= MAXUPDATES)
        make_fullUpdate();
    else if (!_global->stopwindow)
    {
        if (x < _global->window.x)
            _global->window.x = x;
        if (y < _global->window.y)
            _global->window.y = y;
        if (x + w > _global->window.w)
            _global->window.w = (x + w) - 1;
        if (y + h > _global->window.h)
            _global->window.h = (y + h) - 1;
        if (_global->window.x < 0)
            _global->window.x = 0;
        if (_global->window.y < MENUHEIGHT)
            _global->window.y = MENUHEIGHT;
        if (_global->window.w > (_global->screenWidth-1))
            _global->window.w = (_global->screenWidth-1);
        if (_global->window.h > (_global->screenHeight-1))
            _global->window.h = (_global->screenHeight-1);
    }
}

void ENVIRONMENT::make_bgupdate(int x, int y, int w, int h)
{
    int combined = 0;
    if (_global && combineUpdates && _global->lastUpdatesCount && (_global->lastUpdatesCount < MAXUPDATES))
    {
        BOX prev, next;
        prev.x = _global->lastUpdates[_global->lastUpdatesCount - 1].x;
        prev.y = _global->lastUpdates[_global->lastUpdatesCount - 1].y;
        prev.w = _global->lastUpdates[_global->lastUpdatesCount - 1].x + _global->lastUpdates[_global->lastUpdatesCount - 1].w;
        prev.h = _global->lastUpdates[_global->lastUpdatesCount - 1].y + _global->lastUpdates[_global->lastUpdatesCount - 1].h;

        next.x = x;
        next.y = y;
        next.w = x + w;
        next.h = y + h;

        if ( ((next.w > prev.x - 3) && (prev.w > next.x - 3))
             && ((next.h > prev.y - 3) && (prev.h > next.y - 3)) )
        {
            next.x = (next.x < prev.x)?next.x:prev.x;
            next.y = (next.y < prev.y)?next.y:prev.y;
            next.w = (next.w > prev.w)?next.w:prev.w;
            next.h = (next.h > prev.h)?next.h:prev.h;
            _global->lastUpdates[_global->lastUpdatesCount - 1].x = next.x;
            _global->lastUpdates[_global->lastUpdatesCount - 1].y = next.y;
            _global->lastUpdates[_global->lastUpdatesCount - 1].w = next.w - next.x;
            _global->lastUpdates[_global->lastUpdatesCount - 1].h = next.h - next.y;
            combined = 1;
        }
    }
    if (_global && !combined)
    {
        _global->lastUpdates[_global->lastUpdatesCount].x = x;
        _global->lastUpdates[_global->lastUpdatesCount].y = y;
        _global->lastUpdates[_global->lastUpdatesCount].w = w;
        _global->lastUpdates[_global->lastUpdatesCount].h = h;
        if (_global->lastUpdatesCount < MAXUPDATES)
            _global->lastUpdatesCount++;
    }

    if (_global && _global->lastUpdatesCount >= MAXUPDATES)
        make_fullUpdate();
}

void ENVIRONMENT::make_fullUpdate()
{
    // Replace Updates with a full screen update:
    int iOldCombUpd = combineUpdates;
    combineUpdates = 0;

    // They are splitted into 4 x 2 updates:
    int iQuartWidth = _global->halfWidth / 2;
    int iHalfHeight = _global->halfHeight;

    _global->updateCount = 0;
    for (int x = 0; x < 4; x++)
        for (int y = 0; y < 2; y++)
            make_update(iQuartWidth * x, iHalfHeight * y, iQuartWidth * (x+1), iHalfHeight * (y+1));

    _global->lastUpdatesCount = 0;
    for (int x = 0; x < 4; x++)
        for (int y = 0; y < 2; y++)
            make_bgupdate(iQuartWidth * x, iHalfHeight * y, iQuartWidth * (x+1), iHalfHeight * (y+1));

    combineUpdates = iOldCombUpd;
}

int ENVIRONMENT::getAvgBgColor(int aTopLeftX, int aTopLeftY, int aBotRightX, int aBotRightY, double aVelX, double aVelY)
{
    // Coordinate sets:
    int iLeftX = aTopLeftX;
    int iRightX = aBotRightX;
    int iCentX; // Will be calculated later
    int iTopY = aTopLeftY;
    int iBottomY = aBotRightY;
    int iCentY; // Will be calculated later!

    // Colors:
    int iColorToLe, iColorToCe, iColorToRi; // top row
    int iColorCeLe, iColorCeCe, iColorCeRi; // center row
    int iColorBoLe, iColorBoCe, iColorBoRi; // bottom row
    int iRed = 0;
    int iGreen = 0;
    int iBlue = 0;

    // Get the coordinates into range:
    if (aTopLeftX < 1)
        iLeftX = 1;
    if (aTopLeftX > (_global->screenWidth - 2))
        iLeftX = _global->screenWidth - 2;
    if (aBotRightX < 1)
        iRightX = 1;
    if (aBotRightX > (_global->screenWidth - 2))
        iRightX = _global->screenWidth - 2;
    iCentX = (iLeftX + iRightX) / 2;

    if (aTopLeftY < (MENUHEIGHT + 1))
        iTopY = MENUHEIGHT + 1;
    if (aTopLeftY > (_global->screenHeight - 2))
        iTopY = _global->screenHeight - 2;
    if (aBotRightY < (MENUHEIGHT + 1))
        iBottomY = MENUHEIGHT + 1;
    if (aBotRightY > (_global->screenHeight - 2))
        iBottomY = _global->screenHeight - 2;
    iCentY = (iTopY + iBottomY) / 2;

    // Get Sky color or bg color, whatever fits:
    /*---------------------
         --- Left side ---
      ---------------------*/
    if (surface[iLeftX] <= iTopY)
    {
        // All infront of the surface:
        iColorBoLe = getpixel(terrain, iLeftX, iBottomY);
        iColorCeLe = getpixel(terrain, iLeftX, iCentY);
        iColorToLe = getpixel(terrain, iLeftX, iTopY);
    }
    else
    {
        // look where we are going...
        if (surface[iLeftX] > iBottomY)
        {
            // Left part is guaranteed to be in front of the sky, get all three colors at once:
            iColorBoLe = getpixel(sky, iLeftX, iBottomY - MENUHEIGHT);
            iColorCeLe = getpixel(sky, iLeftX, iCentY - MENUHEIGHT);
            iColorToLe = getpixel(sky, iLeftX, iTopY - MENUHEIGHT);
        }
        else
        {
            // At least the bottom color is infront of the terrain
            iColorBoLe = getpixel(terrain, iLeftX, iBottomY);
            if (surface[iLeftX] > iCentY)
            {
                // ...but the other two are in front of the sky:
                iColorCeLe = getpixel(sky, iLeftX, iCentY - MENUHEIGHT);
                iColorToLe = getpixel(sky, iLeftX, iTopY - MENUHEIGHT);
            }
            else
            {
                // Nope, they are in front of the terrain
                iColorCeLe = getpixel(terrain, iLeftX, iCentY);
                iColorToLe = getpixel(terrain, iLeftX, iTopY);
            }
        }
    }
    /*---------------------
         --- The Center ---
      ---------------------*/
    if (surface[iCentX] <= iTopY)
    {
        // All infront of the surface:
        iColorBoCe = getpixel(terrain, iCentX, iBottomY);
        iColorCeCe = getpixel(terrain, iCentX, iCentY);
        iColorToCe = getpixel(terrain, iCentX, iTopY);
    }
    else
    {
        // look where we are going...
        if (surface[iCentX] > iBottomY)
        {
            // Left part is guaranteed to be in front of the sky, get all three colors at once:
            iColorBoCe = getpixel(sky, iCentX, iBottomY - MENUHEIGHT);
            iColorCeCe = getpixel(sky, iCentX, iCentY - MENUHEIGHT);
            iColorToCe = getpixel(sky, iCentX, iTopY - MENUHEIGHT);
        }
        else
        {
            // At least the bottom color is infront of the terrain
            iColorBoCe = getpixel(terrain, iCentX, iBottomY);
            if (surface[iCentX] > iCentY)
            {
                // ...but the other two are in front of the sky:
                iColorCeCe = getpixel(sky, iCentX, iCentY - MENUHEIGHT);
                iColorToCe = getpixel(sky, iCentX, iTopY - MENUHEIGHT);
            }
            else
            {
                // Nope, they are in front of the terrain
                iColorCeCe = getpixel(terrain, iCentX, iCentY);
                iColorToCe = getpixel(terrain, iCentX, iTopY);
            }
        }
    }
    /*----------------------
         --- Right side ---
      ----------------------*/
    if (surface[iRightX] <= iTopY)
    {
        // All infront of the surface:
        iColorBoRi = getpixel(terrain, iRightX, iBottomY);
        iColorCeRi = getpixel(terrain, iRightX, iCentY);
        iColorToRi = getpixel(terrain, iRightX, iTopY);
    }
    else
    {
        // look where we are going...
        if (surface[iRightX] > iBottomY)
        {
            // Left part is guaranteed to be in front of the sky, get all three colors at once:
            iColorBoRi = getpixel(sky, iRightX, iBottomY - MENUHEIGHT);
            iColorCeRi = getpixel(sky, iRightX, iCentY - MENUHEIGHT);
            iColorToRi = getpixel(sky, iRightX, iTopY - MENUHEIGHT);
        }
        else
        {
            // At least the bottom color is infront of the terrain
            iColorBoRi = getpixel(terrain, iRightX, iBottomY);
            if (surface[iRightX] > iCentY)
            {
                // ...but the other two are in front of the sky:
                iColorCeRi = getpixel(sky, iRightX, iCentY - MENUHEIGHT);
                iColorToRi = getpixel(sky, iRightX, iTopY - MENUHEIGHT);
            }
            else
            {
                // Nope, they are in front of the terrain
                iColorCeRi = getpixel(terrain, iRightX, iCentY);
                iColorToRi = getpixel(terrain, iRightX, iTopY);
            }
        }
    }

    // Fetch the rgb parts, according to movement:
    /* --- X-Movement --- */
    if (aVelX < 0.0)
    {
        // Movement to the left, weight left side color twice
        iRed += (_GET_R(iColorBoLe) * 2) + (_GET_R(iColorCeLe) * 2) + (_GET_R(iColorToLe) * 2);
        iGreen += (_GET_G(iColorBoLe) * 2) + (_GET_G(iColorCeLe) * 2) + (_GET_G(iColorToLe) * 2);
        iBlue += (_GET_B(iColorBoLe) * 2) + (_GET_B(iColorCeLe) * 2) + (_GET_B(iColorToLe) * 2);
        // The others are counted once
        iRed += _GET_R(iColorBoCe) + _GET_R(iColorCeCe) + _GET_R(iColorToCe) + _GET_R(iColorBoRi) + _GET_R(iColorCeRi) + _GET_R(iColorToRi);
        iGreen += _GET_G(iColorBoCe) + _GET_G(iColorCeCe) + _GET_G(iColorToCe) + _GET_G(iColorBoRi) + _GET_G(iColorCeRi) + _GET_G(iColorToRi);
        iBlue += _GET_B(iColorBoCe) + _GET_B(iColorCeCe) + _GET_B(iColorToCe) + _GET_B(iColorBoRi) + _GET_B(iColorCeRi) + _GET_B(iColorToRi);
    }
    if (aVelX == 0.0)
    {
        // No X-Movement, weight center twice
        iRed += (_GET_R(iColorBoCe) * 2) + (_GET_R(iColorCeCe) * 2) + (_GET_R(iColorToCe) * 2);
        iGreen += (_GET_G(iColorBoCe) * 2) + (_GET_G(iColorCeCe) * 2) + (_GET_G(iColorToCe) * 2);
        iBlue += (_GET_B(iColorBoCe) * 2) + (_GET_B(iColorCeCe) * 2) + (_GET_B(iColorToCe) * 2);
        // The others are counted once
        iRed += _GET_R(iColorBoLe) + _GET_R(iColorCeLe) + _GET_R(iColorToLe) + _GET_R(iColorBoRi) + _GET_R(iColorCeRi) + _GET_R(iColorToRi);
        iGreen += _GET_G(iColorBoLe) + _GET_G(iColorCeLe) + _GET_G(iColorToLe) + _GET_G(iColorBoRi) + _GET_G(iColorCeRi) + _GET_G(iColorToRi);
        iBlue += _GET_B(iColorBoLe) + _GET_B(iColorCeLe) + _GET_B(iColorToLe) + _GET_B(iColorBoRi) + _GET_B(iColorCeRi) + _GET_B(iColorToRi);
    }
    if (aVelX > 0.0)
    {
        // Movement to the right, weight right side color twice
        iRed += (_GET_R(iColorBoRi) * 2) + (_GET_R(iColorCeRi) * 2) + (_GET_R(iColorToRi) * 2);
        iGreen += (_GET_G(iColorBoRi) * 2) + (_GET_G(iColorCeRi) * 2) + (_GET_G(iColorToRi) * 2);
        iBlue += (_GET_B(iColorBoRi) * 2) + (_GET_B(iColorCeRi) * 2) + (_GET_B(iColorToRi) * 2);
        // The others are counted once
        iRed += _GET_R(iColorBoCe) + _GET_R(iColorCeCe) + _GET_R(iColorToCe) + _GET_R(iColorBoLe) + _GET_R(iColorCeLe) + _GET_R(iColorToLe);
        iGreen += _GET_G(iColorBoCe) + _GET_G(iColorCeCe) + _GET_G(iColorToCe) + _GET_G(iColorBoLe) + _GET_G(iColorCeLe) + _GET_G(iColorToLe);
        iBlue += _GET_B(iColorBoCe) + _GET_B(iColorCeCe) + _GET_B(iColorToCe) + _GET_B(iColorBoLe) + _GET_B(iColorCeLe) + _GET_B(iColorToLe);
    }
    /* --- Y-Movement --- */
    if (aVelY < 0.0)
    {
        // Movement upwards, weight upper side color twice
        iRed += (_GET_R(iColorToLe) * 2) + (_GET_R(iColorToCe) * 2) + (_GET_R(iColorToRi) * 2);
        iGreen += (_GET_G(iColorToLe) * 2) + (_GET_G(iColorToCe) * 2) + (_GET_G(iColorToRi) * 2);
        iBlue += (_GET_B(iColorToLe) * 2) + (_GET_B(iColorToCe) * 2) + (_GET_B(iColorToRi) * 2);
        // The others are counted once
        iRed += _GET_R(iColorBoLe) + _GET_R(iColorCeLe) + _GET_R(iColorBoCe) + _GET_R(iColorCeCe) + _GET_R(iColorBoRi) + _GET_R(iColorCeRi);
        iGreen += _GET_G(iColorBoLe) + _GET_G(iColorCeLe) + _GET_G(iColorBoCe) + _GET_G(iColorCeCe) + _GET_G(iColorBoRi) + _GET_G(iColorCeRi);
        iBlue += _GET_B(iColorBoLe) + _GET_B(iColorCeLe) + _GET_B(iColorBoCe) + _GET_B(iColorCeCe) + _GET_B(iColorBoRi) + _GET_B(iColorCeRi);
    }
    if (aVelY == 0.0)
    {
        // No Y-Movement, weight center twice
        iRed += (_GET_R(iColorCeLe) * 2) + (_GET_R(iColorCeCe) * 2) + (_GET_R(iColorCeRi) * 2);
        iGreen += (_GET_G(iColorCeLe) * 2) + (_GET_G(iColorCeCe) * 2) + (_GET_G(iColorCeRi) * 2);
        iBlue += (_GET_B(iColorCeLe) * 2) + (_GET_B(iColorCeCe) * 2) + (_GET_B(iColorCeRi) * 2);
        // The others are counted once
        iRed += _GET_R(iColorBoLe) + _GET_R(iColorToLe) + _GET_R(iColorBoCe) + _GET_R(iColorToCe) + _GET_R(iColorBoRi) + _GET_R(iColorToRi);
        iGreen += _GET_G(iColorBoLe) + _GET_G(iColorToLe) + _GET_G(iColorBoCe) + _GET_G(iColorToCe) + _GET_G(iColorBoRi) + _GET_G(iColorToRi);
        iBlue += _GET_B(iColorBoLe) + _GET_B(iColorToLe) + _GET_B(iColorBoCe) + _GET_B(iColorToCe) + _GET_B(iColorBoRi) + _GET_B(iColorToRi);
    }
    if (aVelY > 0.0)
    {
        // Movement downwards, weight lower side color twice
        iRed += (_GET_R(iColorBoRi) * 2) + (_GET_R(iColorBoCe) * 2) + (_GET_R(iColorBoLe) * 2);
        iGreen += (_GET_G(iColorBoRi) * 2) + (_GET_G(iColorBoCe) * 2) + (_GET_G(iColorBoLe) * 2);
        iBlue += (_GET_B(iColorBoRi) * 2) + (_GET_B(iColorBoCe) * 2) + (_GET_B(iColorBoLe) * 2);
        // The others are counted once
        iRed += _GET_R(iColorToLe) + _GET_R(iColorCeLe) + _GET_R(iColorToCe) + _GET_R(iColorCeCe) + _GET_R(iColorToRi) + _GET_R(iColorCeRi);
        iGreen += _GET_G(iColorToLe) + _GET_G(iColorCeLe) + _GET_G(iColorToCe) + _GET_G(iColorCeCe) + _GET_G(iColorToRi) + _GET_G(iColorCeRi);
        iBlue += _GET_B(iColorToLe) + _GET_B(iColorCeLe) + _GET_B(iColorToCe) + _GET_B(iColorCeCe) + _GET_B(iColorToRi) + _GET_B(iColorCeRi);
    }
    /* I know this looks weird, but what we now have is some kind of summed matrix, which is always the same:
       * Let's assume that dVelX and dVelY are both 0.0, so no movement is happening. The result is: (In counted times)
       * 2|3|2  ( =  7)
       * -+-+-
       * 3|4|3  ( = 10)
       * -+-+-
       * 2|3|2  ( =  7)
       *          = 24
       * And it is always 24, no matter which movement combination you try. */
    iRed /= 24;
    if (iRed > 0xff)
        iRed = 0xff;
    iGreen/= 24;
    if (iGreen> 0xff)
        iGreen = 0xff;
    iBlue /= 24;
    if (iBlue > 0xff)
        iBlue = 0xff;

    return (makecol(iRed, iGreen, iBlue));
}

/*
 * This function puts all the of the environment settings back
 * to their default values. These are settings which get written
 * to the config file.
 * -- Jesse
 *  */
void ENVIRONMENT::Reset_Options()
{
    windstrength = 8;
    windvariation = 1;
    viscosity = 0.5;
    gravity = 0.150;
    weapontechLevel = 5;
    itemtechLevel = 5;
    meteors = 0;
    lightning = 0;
    satellite = 0.0;
    fog = 0;
    landType = LANDTYPE_RANDOM;
    landSlideType = LANDSLIDE_GRAVITY;
    landSlideDelay = MAX_GRAVITY_DELAY;
    falling_dirt_balls = 0.0;
    wallType = 0.0;
    dBoxedMode = 0.0;
    dFadingText = 0.0;
    dShadowedText = 0.0;
    custom_background = 0.0;
}

/*
This function checks to see which wall type we are using
and sets the appropriate colour.
*/
void ENVIRONMENT::Set_Wall_Colour()
{
    switch (current_wallType)
    {
    case WALL_RUBBER:
        wallColour = makecol(0, 255, 0);
        break;
    case WALL_STEEL:
        wallColour = makecol(255, 0, 0);
        break;
    case WALL_SPRING:
        wallColour = makecol(0, 0, 255);
        break;
    case WALL_WRAP:
        wallColour = makecol(255, 255, 0);
        break;
    }
}

bool ENVIRONMENT::Get_Boxed_Mode(GLOBALDATA *global)
{
    if (dBoxedMode == 2.0)
    {
        if (rand() % 2)
            global->bIsBoxed = true;
        else
            global->bIsBoxed = false;
    }
    else if (dBoxedMode == 1.0)
        global->bIsBoxed = true;
    else if (dBoxedMode == 0.0)
        global->bIsBoxed = false;
    return global->bIsBoxed;
} 

TANK *ENVIRONMENT::Get_Next_Tank(int *wrapped_around)
{
    int index;
    int found = FALSE;
    int old_index;
    int wrapped = 0;

    index = global_tank_index;
    old_index = index;
    index++;
    while (!found)
    {
        if (index >= MAXPLAYERS)
        {
            index = 0;
            *wrapped_around = TRUE;
            wrapped++;
        }
        if ((order[index]) && (index != old_index))
            found = TRUE;
        else
            index++;
        if (wrapped > 1)
            break;
    }

    /*
   if (! found)
      *wrapped_around = TRUE;
   else
      *wrapped_around = FALSE;
   */
    global_tank_index = index;
    return order[index];
}

void ENVIRONMENT::increaseVolume()
{
    if (volume_factor < MAX_VOLUME_FACTOR)
        ++volume_factor;
}

void ENVIRONMENT::decreaseVolume()
{
    if (volume_factor > 0)
        --volume_factor;
}

int ENVIRONMENT::scaleVolume(int vol) const
{
    return (vol * volume_factor) / MAX_VOLUME_FACTOR;
}
