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
#include "player.h"
#include "tank.h"
#include "menu.h"
#include "files.h"
#include "floattext.h"
#include "network.h"

// When defined draws AI 'planning'
//#define AI_PLANNING_DEBUG 1

PLAYER::PLAYER(GLOBALDATA *global, ENVIRONMENT *env) : _global(global),_env(env),_turnStage(0),_target(NULL),
    _oldTarget(NULL),iTargettingRound(0),revenge(NULL),tank(NULL),pColor(NULL),menuopts(NULL),menudesc(NULL)
{
    int my_colour;

    money = 15000;
    score = 0;
    played = 0;
    won = 0;
    tank = NULL;
    team = TEAM_NEUTRAL;

    iBoostItemsBought = -1;

    selected = FALSE;
    changed_weapon = false;


    tank_bitmap = 0.0;

    nm[0] = 99;
    for (int count = 1; count < WEAPONS; count++)
        nm[count] = 0;

    for (int count = 0; count < ITEMS; count++)
        ni[count] = 0;

    strncpy(_name, "New", NAME_LENGTH);
    type = HUMAN_PLAYER;

    // 25% of time set to perplay weapon preferences
    preftype = (rand() % 4) ? ALWAYS_PREF : PERPLAY_PREF;
    //  generatePreferences (); <-- not here!

    defensive = (double) ((rand() % 10000) - 5000) / 5000.0;
    vengeful = rand() % 100;
    vengeanceThreshold = (double) (rand() % 1000) / 1000.0;
    revenge = NULL;
    /** @TODO: These two values are nowhere used.
    * We should think about a usage, because bot characteristics could become
    * alot more wide spread with their help. **/
    selfPreservation = (double) (rand() % 3000) / 1000;
    painSensitivity = (double) (rand() % 3000) / 1000;

    // color = rand() % WHITE;
    my_colour = rand() % 4;
    switch (my_colour)
    {
    case 0:
        color = makecol(200 + (rand() % 56), rand() % 25, rand() % 25);
        break;
    case 1:
        color = makecol(rand() % 25, 200 + (rand() % 56), rand() % 25);
        break;
    case 2:
        color = makecol(rand() % 25, rand() % 25, 200 + (rand() % 56));
        break;
    case 3:
        color = makecol(200 + (rand() % 56), rand() % 25, 200 + (rand() % 56));
        break;
    }
    typeText[0] = global->ingame->complete_text[54];
    typeText[1] = global->ingame->complete_text[55];
    typeText[2] = global->ingame->complete_text[56];
    typeText[3] = global->ingame->complete_text[57];
    typeText[4] = global->ingame->complete_text[58];
    typeText[5] = global->ingame->complete_text[59];
    preftypeText[0] = global->ingame->complete_text[60];
    preftypeText[1] = global->ingame->complete_text[61];
    tank_type[0] = global->ingame->complete_text[62];
    tank_type[1] = global->ingame->complete_text[63];
    tank_type[2] = global->ingame->complete_text[64];
    tank_type[3] = global->ingame->complete_text[65];
    tank_type[4] = global->ingame->complete_text[73];
    tank_type[5] = global->ingame->complete_text[74];
    tank_type[6] = global->ingame->complete_text[75];
    tank_type[7] = global->ingame->complete_text[76];
    teamText[0] = global->ingame->complete_text[66];
    teamText[1] = global->ingame->complete_text[67];
    teamText[2] = global->ingame->complete_text[68];

    // Weapon Preferences need to be initalized!
    for (int weapCount = 0; weapCount < THINGS; weapCount++)
        _weaponPreference[weapCount] = 0;

    menudesc = NULL;
    menuopts = NULL;
    initMenuDesc();
    skip_me = false;
#ifdef NETWORK
    server_socket = 0;
#endif
}

/*
PLAYER::PLAYER(GLOBALDATA *global, ENVIRONMENT *env, ifstream &ifsFile, bool file) : _global(global),_env(env),
    _target(NULL),revenge(NULL),tank(NULL),pColor(NULL),menuopts(NULL),menudesc(NULL)
{
    money = 15000;
    score = 0;
    _turnStage = 0;
    selected = FALSE;

    tank_bitmap = 0.0;
    team = TEAM_NEUTRAL;

    iBoostItemsBought = -1;

    nm[0] = 99;
    for (int count = 1; count < WEAPONS; count++)
        nm[count] = 0;

    for (int count = 0; count < ITEMS; count++)
        ni[count] = 0;

    if (file)
        loadFromFile(ifsFile);

    type = (int) type;

    typeText[0] = "Human";
    typeText[1] = "Useless";
    typeText[2] = "Guesser";
    typeText[3] = "Ranger";
    typeText[4] = "Targetter";
    typeText[5] = "Deadly";
    preftypeText[0] = "Per Game";
    preftypeText[1] = "Only Once";
    tank_type[0] = "Normal";
    tank_type[1] = "Classic";
    tank_type[2] = "Big Grey";
    tank_type[3] = "T34";
    teamText[0] = "Sith";
    teamText[1] = "Neutral";
    teamText[2] = "Jedi";
    initMenuDesc();
    skip_me = false;
#ifdef NETWORK
    net_command[0] = '\0';
#endif
}
*/

int PLAYER::getBoostValue()
{
    return ((int) (ni[ITEM_ARMOUR] * ((double) item[ITEM_ARMOUR].vals[0] / (double) item[ITEM_PLASTEEL].vals[0])) + ni[ITEM_PLASTEEL] +
            (int) (ni[ITEM_INTENSITY_AMP] * ((double) item[ITEM_INTENSITY_AMP].vals[0] / (double) item[ITEM_VIOLENT_FORCE].vals[0])) +
            ni[ITEM_VIOLENT_FORCE]);
}

void PLAYER::setComputerValues(int aOffset)
{
    int iType = (int) type + aOffset;
    if (iType > (int) DEADLY_PLAYER)
        iType = (int) DEADLY_PLAYER;
    rangeFindAttempts = (int) (pow(iType + 1, 2) + 1);    // Useless: 5, Deadly: 37
    retargetAttempts = (int) (pow(iType, 2) + 1);    // Useless: 2, Deadly: 26
    focusRate = ((double) iType * 2.0) / 10.0;    // Useless: 0.2, Deadly: 1.0
    errorMultiplier = (double) (DEADLY_PLAYER + 1 - iType) / (double) rangeFindAttempts;
    if (errorMultiplier > 2.0)
        errorMultiplier = 2.0;
}

int displayPlayerName(GLOBALDATA *global, ENVIRONMENT *env, int x, int y, void *data);
void PLAYER::initMenuDesc()
{
    GLOBALDATA *global = _global;
    int destroyPlayer (GLOBALDATA *global, ENVIRONMENT *env, void *data);
    int i = 0;
    int height = -68;

    // before we get started, eraw any old version of the menu
    if (menuopts)
        free(menuopts);
    if (menudesc)
        free(menudesc);

    // menudesc = new MENUDESC;
    menudesc = (MENUDESC *) calloc(1, sizeof(MENUDESC));
    if (!menudesc)
    {
        perror("player.cpp: Failed allocating memory for menudesc in PLAYER::initMenuDesc");
        return;
        // exit (1);
    }
    menudesc->title = _name;

    // Name,Color
    menudesc->numEntries = 10;
    menudesc->okayButton = TRUE;
    menudesc->quitButton = FALSE;

    // menuopts = new MENUENTRY[menudesc->numEntries];
    menuopts = (MENUENTRY *) calloc(menudesc->numEntries, sizeof(MENUENTRY));
    if (!menuopts)
    {
        perror("player.cpp: Failed allocating memory for menuopts in PLAYER::initMenuDesc");
        return;
        // exit (1);
    }

    //init memory
    // memset(menuopts, 0, menudesc->numEntries * sizeof(MENUENTRY));

    // Player name
    menuopts[i].name = global->ingame->complete_text[29];
    menuopts[i].displayFunc = NULL;
    menuopts[i].color = color;
    menuopts[i].value = (double*) _name;
    menuopts[i].specialOpts = NULL;
    menuopts[i].type = OPTION_TEXTTYPE;
    menuopts[i].viewonly = FALSE;
    menuopts[i].x = _global->halfWidth - 3;
    menuopts[i].y = _global->halfHeight + height;
    i++;
    height += 20;

    // Player colour
    menuopts[i].name = global->ingame->complete_text[30];
    menuopts[i].displayFunc = NULL;
    menuopts[i].color = WHITE;
    pColor = &color;
    menuopts[i].value = (double*) pColor;
    menuopts[i].specialOpts = NULL;
    menuopts[i].type = OPTION_COLORTYPE;
    menuopts[i].viewonly = FALSE;
    menuopts[i].x = _global->halfWidth - 3;
    menuopts[i].y = _global->halfHeight + height;
    i++;
    height += 20;

    // Player type (human, computer)
    menuopts[i].name = global->ingame->complete_text[31];
    menuopts[i].displayFunc = NULL;
    menuopts[i].color = WHITE;
    menuopts[i].value = (double*) &type;
    menuopts[i].min = 0;
    menuopts[i].max = LAST_PLAYER_TYPE - 1;
    menuopts[i].increment = 1;
    menuopts[i].defaultv = 0;
    menuopts[i].format = "%s";
    menuopts[i].specialOpts = typeText;
    menuopts[i].type = OPTION_SPECIALTYPE;
    menuopts[i].viewonly = FALSE;
    menuopts[i].x = _global->halfWidth - 3;
    menuopts[i].y = _global->halfHeight + height;
    i++;
    height += 20;

    menuopts[i].name = global->ingame->complete_text[20];
    menuopts[i].displayFunc = NULL;
    menuopts[i].color = WHITE;
    menuopts[i].value = (double *) &team;
    menuopts[i].min = 0;
    menuopts[i].max = TEAM_JEDI;
    menuopts[i].increment = 1;
    menuopts[i].defaultv = TEAM_NEUTRAL;
    menuopts[i].format = "%s";
    menuopts[i].specialOpts = teamText;
    menuopts[i].type = OPTION_SPECIALTYPE;
    menuopts[i].viewonly = FALSE;
    menuopts[i].x = _global->halfWidth - 3;
    menuopts[i].y = _global->halfHeight + height;
    i++;
    height += 20;

    // Player preftype (human, computer)
    menuopts[i].name = global->ingame->complete_text[32];
    menuopts[i].displayFunc = NULL;
    menuopts[i].color = WHITE;
    menuopts[i].value = (double*) &preftype;
    menuopts[i].min = 0;
    menuopts[i].max = ALWAYS_PREF;
    menuopts[i].increment = 1;
    menuopts[i].defaultv = 0;
    menuopts[i].format = "%s";
    menuopts[i].specialOpts = preftypeText;
    menuopts[i].type = OPTION_SPECIALTYPE;
    menuopts[i].viewonly = FALSE;
    menuopts[i].x = _global->halfWidth - 3;
    menuopts[i].y = _global->halfHeight + height;
    i++;
    height += 20;

    menuopts[i].name = global->ingame->complete_text[33];
    menuopts[i].displayFunc = NULL;
    menuopts[i].color = WHITE;
    menuopts[i].value = (double*) &played;
    menuopts[i].format = "%.0f";
    menuopts[i].specialOpts = NULL;
    menuopts[i].type = OPTION_DOUBLETYPE;
    menuopts[i].viewonly = TRUE;
    menuopts[i].x = _global->halfWidth - 3;
    menuopts[i].y = _global->halfHeight + height;
    i++;
    height += 20;

    menuopts[i].name = global->ingame->complete_text[34];
    menuopts[i].displayFunc = NULL;
    menuopts[i].color = WHITE;
    menuopts[i].value = (double*) &won;
    menuopts[i].format = "%.0f";
    menuopts[i].specialOpts = NULL;
    menuopts[i].type = OPTION_DOUBLETYPE;
    menuopts[i].viewonly = TRUE;
    menuopts[i].x = _global->halfWidth - 3;
    menuopts[i].y = _global->halfHeight + height;
    i++;
    height += 20;

    menuopts[i].name = global->ingame->complete_text[35];
    menuopts[i].displayFunc = Display_Tank_Bitmap;
    menuopts[i].color = WHITE;
    menuopts[i].value = &tank_bitmap;
    menuopts[i].min = 0;
    menuopts[i].max = TANK_TYPES;
    menuopts[i].increment = 1;
    menuopts[i].defaultv = 0;
    menuopts[i].format = "%1.0f";
    menuopts[i].specialOpts = tank_type;
    menuopts[i].type = OPTION_SPECIALTYPE;
    menuopts[i].viewonly = FALSE;
    menuopts[i].x = _global->halfWidth - 3;
    menuopts[i].y = _global->halfHeight + height;
    i++;
    height += 20;

    menuopts[i].name = global->ingame->complete_text[36];
    menuopts[i].displayFunc = NULL;
    menuopts[i].color = WHITE;
    menuopts[i].value = (double*) destroyPlayer;
    menuopts[i].data = (void*) this;
    menuopts[i].type = OPTION_ACTIONTYPE;
    menuopts[i].viewonly = FALSE;
    menuopts[i].x = _global->halfWidth - 3;
    menuopts[i].y = _global->halfHeight + height;
    i++;
    height += 20;

    menudesc->entries = menuopts;
}

PLAYER::~PLAYER()
{
    if (tank)
        delete(tank);

    if (menuopts)
        free(menuopts);
    // delete(menuopts);
    if (menudesc)
        free(menudesc);
    // delete(menudesc);

    _global = NULL;
    _env    = NULL;
    _target = NULL;
    revenge = NULL;

    pColor = NULL;
}

/*
Save the player settings in text form. Fields are
formateed as
[name]=[value]\n

Function returns true on success and false on failure.
*/
int PLAYER::saveToFile_Text(FILE *file)
{
    int count;

    if (!file)
        return FALSE;
    // start section with (char *)"*PLAYER*"
    fprintf(file, "*PLAYER*\n");
    fprintf(file, "NAME=%s\n", _name);
    fprintf(file, "VENGEFUL=%d\n", vengeful);
    fprintf(file, "VENGEANCETHRESHOLD=%lf\n", vengeanceThreshold);
    fprintf(file, "TYPE=%lf\n", type);
    fprintf(file, "TYPESAVED=%lf\n", type_saved);
    fprintf(file, "COLOR=%d\n", color);
    fprintf(file, "COLOR2=%d\n", color2);
    for (count = 0; count < THINGS; count++)
        fprintf(file, "WEAPONPREFERENCES=%d %d\n", count, _weaponPreference[count]);

    fprintf(file, "PLAYED=%lf\n", played);
    fprintf(file, "WON=%lf\n", won);
    fprintf(file, "PREFTYPE=%lf\n", preftype);
    fprintf(file, "SELFPRESERVATION=%lf\n", selfPreservation);
    fprintf(file, "PAINSENSITIVITY=%lf\n", painSensitivity);
    fprintf(file, "DEFENSIVE=%lf\n", defensive);
    fprintf(file, "TANK_BITMAP=%lf\n", tank_bitmap);
    fprintf(file, "TEAM=%lf\n", team);
    fprintf(file, "***\n");
    return TRUE;
}

/*
This function tries to load player data from a text file.
Each line is parsed for (char *)"field=value", except WEAPONPREFERENCES
which is parsed (char *)"field=index value".
If all goes well TRUE is returned, on error the function returns FALSE.
-- Jesse
*/

int PLAYER::loadFromFile_Text(FILE *file)
{
    char line[MAX_CONFIG_LINE];
    int equal_position, line_length;
    int index, wp_value;
    char field[MAX_CONFIG_LINE], value[MAX_CONFIG_LINE];
    char *result = NULL;
    bool done = false;

    if (!file)
        return FALSE;

    // read until we hit line (char *)"*PLAYER*" or "***" or EOF
    do
    {
        result = fgets(line, MAX_CONFIG_LINE, file);
        if (!result)     // eof
            return FALSE;
        if (!strncmp(line, "***", 3))    // end of record
            return FALSE;
    }
    while (strncmp(line, "*PLAYER*", 8));    // read until we hit new record

    while (result && (!done))
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
            while ((equal_position < line_length) && (line[equal_position] != '='))
                equal_position++;
            // make sure we have valid equal sign
            if (equal_position <= line_length)
            {
                // seperate field from value
                memset(field, '\0', MAX_CONFIG_LINE);
                memset(value, '\0', MAX_CONFIG_LINE);
                strncpy(field, line, equal_position);
                strcpy(value, & (line[equal_position + 1]));
                // check which field we have and process value
                if (!strcasecmp(field, "name"))
                    strncpy(_name, value, NAME_LENGTH);
                else if (!strcasecmp(field, "vengeful"))
                    sscanf(value, "%d", &vengeful);
                else if (!strcasecmp(field, "vengeancethreshold"))
                    sscanf(value, "%lf", &vengeanceThreshold);
                else if (!strcasecmp(field, "type"))
                    sscanf(value, "%lf", &type);
                else if (!strcasecmp(field, "typesaved"))
                {
                    sscanf(value, "%lf", &type_saved);
                    if (type_saved > HUMAN_PLAYER)
                        type = type_saved;
                }
                else if (!strcasecmp(field, "color"))
                    sscanf(value, "%d", &color);
                else if (!strcasecmp(field, "color2"))
                    sscanf(value, "%d", &color2);
                else if (!strcasecmp(field, "played"))
                    sscanf(value, "%lf", &played);
                else if (!strcasecmp(field, "won"))
                    sscanf(value, "%lf", &won);
                else if (!strcasecmp(field, "preftype"))
                    sscanf(value, "%lf", &preftype);
                else if (!strcasecmp(field, "selfpreservation"))
                    sscanf(value, "%lf", &selfPreservation);
                else if (!strcasecmp(field, "painsensititveity"))
                    sscanf(value, "%lf", &painSensitivity);
                else if (!strcasecmp(field, "defensive"))
                    sscanf(value, "%lf", &defensive);
                else if (!strcasecmp(field, "tank_bitmap"))
                    sscanf(value, "%lf", &tank_bitmap);
                else if (!strcasecmp(field, "team"))
                    sscanf(value, "%lf", &team);
                else if (!strcasecmp(field, "weaponpreferences"))
                {
                    sscanf(value, "%d %d", &index, &wp_value);
                    if ((index < THINGS) && (index >= 0))
                        _weaponPreference[index] = wp_value;
                }

            }    // end of valid data line
        }      // end of if we read a line properly
    }   // end of while not done

    // make sure previous human players are restored as humans
    if (type == PART_TIME_BOT)
        type = HUMAN_PLAYER;

    return TRUE;
}

void PLAYER::exitShop()
{
    double tmpDM = 0;

    damageMultiplier = 1.0;
    tmpDM += ni[ITEM_INTENSITY_AMP] * item[ITEM_INTENSITY_AMP].vals[0];
    tmpDM += ni[ITEM_VIOLENT_FORCE] * item[ITEM_VIOLENT_FORCE].vals[0];
    if (tmpDM > 0)
        damageMultiplier += pow (tmpDM, 0.6);
}

// run this at the begining of each turn
void PLAYER::newRound()
{
    // if the player is under computer control, give it back to the player
    if (type == PART_TIME_BOT)
        type = HUMAN_PLAYER;

    setComputerValues();

    if (!tank)
    {
        tank = new TANK(_global, _env);
        if (tank)
        {
            tank->player = this;
            tank->initialise();
        }
        else
            perror("player.cpp: Failed allocating memory for tank in PLAYER::newRound");
    }
    // newRound() doesn't need to be called, because ENVIRONMENT::newRound() has already done that!

    changed_weapon = false;
    // if we are playing in a campaign, raise the AI level
    if (_global->campaign_mode)
    {
        if ((type > HUMAN_PLAYER) && (type < DEADLY_PLAYER))
            type += 1.0;
    }

    // forget revenge under certain circumstances
    if (revenge)
    {
        if ((team != TEAM_NEUTRAL) && (team == revenge->team))
            revenge = NULL; // No more round breaking revenge on team mates!
        else if ( (team == TEAM_NEUTRAL) || ((team != TEAM_NEUTRAL) && (revenge->team == TEAM_NEUTRAL)) )
        {
            // neutral to !neutral and vice versa might forget...
            if ((!(rand() % (int)labs((type + 3) / 2))) || ((rand() % 100) > vengeful))
                revenge = NULL;
            /* This gives:
           * USELESS: (1 + 3) / 2) = 2 => 50%
           * GUESSER: (2 + 3) / 2) = 2 => 50%
           * RANGEFI: (3 + 3) / 2) = 3 => 34%
           * TARGETT: (4 + 3) / 2) = 3 => 34%
           * DEADLY : (5 + 3) / 2) = 4 => 25%
           * chance to "forgive". Should be okay...
           * The check against "vengeful" makes "peacefull" bots to forget more easily
           */
        }
    }

    if (!revenge && (rand() % ((int)type + 1)))
    {
        // If there is no revengee there is a small chance the player will seek the leader!
        int iMaxScore = score;
        int iCurScore = 0;
        for (int i = 0; i < _global->numPlayers; i++)
        {
            if (_global->players[i])
            {
                iCurScore = _global->players[i]->score;
                if ( (iCurScore > iMaxScore) &&((team == TEAM_NEUTRAL) || (team != _global->players[i]->team)) )
                {
                    // Higher score found, record as possible revengee
                    iMaxScore = iCurScore;
                    if (abs(iMaxScore - score) > (int) type)
                        revenge = _global->players[i];
                }
            }
        }
    }

    time_left_to_fire = (int) _global->max_fire_time;
    skip_me = false;
    iTargettingRound = 0;
    _target = NULL;
    _oldTarget = NULL;
    last_shield_used = 0;
}

void PLAYER::initialise()
{
    long int totalPrefs;
    int rouletteCount;

    nm[0] = 99;
    for (int count = 1; count < WEAPONS; count++)
        nm[count] = 0;

    for (int count = 0; count < ITEMS; count++)
        ni[count] = 0;

    totalPrefs = 0;
    for (int weapCount = 0; weapCount < THINGS; weapCount++)
        totalPrefs += _weaponPreference[weapCount];

    rouletteCount = 0;
    for (int weapCount = 0; weapCount < THINGS; weapCount++)
    {
        int weapRSpace = (int) ((double) _weaponPreference[weapCount] / totalPrefs * NUM_ROULETTE_SLOTS);
        int weapRCount = 0;

        if (weapRSpace < 1)
            weapRSpace = 1;
        while (weapRCount < weapRSpace && rouletteCount + weapRCount < NUM_ROULETTE_SLOTS)
        {
            _rouletteWheel[rouletteCount + weapRCount] = weapCount;
            weapRCount++;
        }
        rouletteCount += weapRSpace;
    }
    while (rouletteCount < NUM_ROULETTE_SLOTS)
        _rouletteWheel[rouletteCount++] = rand() % THINGS;

    kills = 0;
    killed = 0;
    tank = NULL;
    _target = NULL;
    _oldTarget = NULL;
}

void PLAYER::generatePreferences()
{
    double dBaseProb = (double) MAX_WEAP_PROBABILITY / 2.0;
    int currItem = 0;
    double dWorth;
    int iMaxWeapPref = 0;
    int iMaxItemPref = 0;

    defensive = (2.0 * ((double) rand() / (double) RAND_MAX)) - 1.0;
    double dDefenseMod = (defensive * -1.0) + 2.0;
    // DefenseMod will be between 1.0 (defensive) and 3.0 (offensive)
    // and is used to modifiy vengeful, vengeanceThreshold, selfPreservation and painSensitivity
    vengeful *= 1.0 + (dDefenseMod / 5.0); // +0% - +60% (defensive - offensive)
    if (vengeful > 100)
        vengeful = 100;
    vengeanceThreshold *= 1.0 - (dDefenseMod / 5.0);    // -0% - -60%
    selfPreservation /= dDefenseMod;
    painSensitivity /= dDefenseMod;

    // Now defensive can be modified by team:
    if (team == TEAM_JEDI)
    {
        defensive += (double) ((double) (rand() % 1000)) / 1000.0;
        if (defensive > 1.25)
            defensive = 1.25; // + 1.25 is SuperDefensive
    }
    if (team == TEAM_SITH)
    {
        defensive -= (double) ((double) (rand() % 1000)) / 1000.0;
        if (defensive < -1.25)
            defensive = -1.25; // - 1.25 is SuperAggressive
    }
#ifdef DEBUG
    cout << "Generating Preferences for \"" << getName() << "\" (" << defensive << ")" << endl;
    cout << "----------------------------------------------------" << endl;
#endif // DEBUG
    _weaponPreference[0] = 0; // small missiles are always zero!
    for (int weapCount = 1; weapCount < THINGS; weapCount++)
    {
        bool bIsWarhead = false;

        dWorth = -1.0 * ((double) MAX_WEAP_PROBABILITY / 4.0);
        bIsWarhead = false;
        if (weapCount < WEAPONS)
        {
            // Talking about weapons
            currItem = weapCount;
            if ( weapon[currItem].warhead || ((currItem >= SML_METEOR) && (currItem <= LRG_LIGHTNING)) )
                bIsWarhead = true;    // Bots don't think about warheads or environment!
            else
            {
                // 1. Damage:
                int iWarheads = weapon[currItem].spread;    // For non-spread this is always 1
                if (weapon[currItem].numSubmunitions > 0)
                {
                    iWarheads = weapon[currItem].numSubmunitions;    // It's a cluster
                    dWorth = weapon[weapon[currItem].submunition].damage * iWarheads;   // total damage for clusters

                    if ( ((currItem >= SML_NAPALM) && (currItem <= LRG_NAPALM)) || ((currItem >= FUNKY_BOMB) && (currItem <= FUNKY_DEATH)) )
                        dWorth /= defensive + 1.0 + ((double) type / 2.0);
                    // These weapons are too unpredictable to be counted full
                    // But a true offensive useless bot devides only by 1.0 (not all all, doesn't mind)
                    // And a true defensive deadly bot devides by 5.0
                    if (dWorth > dBaseProb)
                        dWorth = dBaseProb; // Or Large Napalm will always be everybodys favorite
                }
                else
                    dWorth = weapon[currItem].damage * (iWarheads * 2.0);    // total damage for (non-)spreads
                // Note: iWarheads is counted twice, because otherwise spread weapons get far too low score!
                dWorth = dWorth * (dBaseProb * 0.0005);    // 1 Damage is worth 0.5%o of dBaseProb

                // 2. Defensiveness multiplier
                // As said above, defensive players avoid spread/cluster weapons. Thus they rate non-spreads higher:
                if (iWarheads == 1)
                    dWorth *= (defensive + 1.5) * ((double) type / 2.0);
                else
                    dWorth -= (defensive * ((double) type / 2.0)) * dWorth;

                // 3. Dirtballs do no damage and have to be rated by defensiveness
                if ( (currItem >= DIRT_BALL) && (currItem <= SUP_DIRT_BALL))
                    dWorth = (double) (currItem - (double) DIRT_BALL + 1.0) * 150.0 * ((double) type / 2.0) * (defensive + 2.0);

                // 4.: Shaped charges, wide boys and cutters are deadly but limited:
                if ((currItem >= SHAPED_CHARGE) && (currItem <= CUTTER))
                    dWorth *= 1.0 - (((double) type + (defensive * 5.0)) / 20.0);
                // useless, full offensive: * 1.20
                // deadly, full defensive : * 0.50

                // 5.: rollers and penetrators are modified by type, as they *are* usefull
                if ( ((currItem >= SML_ROLLER) && (currItem <= DTH_ROLLER)) || ((currItem >= BURROWER) && (currItem <= PENETRATOR)) )
                    dWorth *= 1.0 + ((double) type / 10.0) + (defensive  / 2);

                // 6.: Tectonis need to be raised!
                if ((currItem >= TREMOR) && (currItem <= TECTONIC))
                    dWorth *= 2.0 + ((double) type / 10.0) + (defensive  / 2);

                // finally dWorth must not be greater than the 3/4 of MAX_WEAPON_PROBABILITY
                if (dWorth > (MAX_WEAP_PROBABILITY * 0.75))
                    dWorth = MAX_WEAP_PROBABILITY * 0.75;
            }
        }
        else
        {
            // Talking about items
            currItem = weapCount - WEAPONS;
            // unfortunately we can only switch here...
            /* Theory:
                As for armour/amps/shields, offensive bots go for amps and reflector
                shields, defensive bots go for armour and hard shields. */
            switch (currItem)
            {
            case ITEM_TELEPORT:
                dWorth = -1.0 * ((defensive - 1.5) * ((double) MAX_WEAP_PROBABILITY / 10.0));
                break;
            case ITEM_SWAPPER:
                dWorth = -1.0 * ((defensive - 1.5) * ((double) MAX_WEAP_PROBABILITY / 7.5));
                break;
            case ITEM_FAN:
                dWorth = 0.0; // useless things!
                break;
            case ITEM_VENGEANCE:
            case ITEM_DYING_WRATH:
            case ITEM_FATAL_FURY:
                dWorth = (defensive + 1.5) * ((double) weapon[(int) item[currItem].vals[0]].damage * (double) item[currItem].vals[1]);
                break;
            case ITEM_ARMOUR:
            case ITEM_PLASTEEL:
                dWorth = dBaseProb * ((double) item[currItem].vals[0] / (double) item[ITEM_PLASTEEL].vals[0]);
                dWorth *= defensive;
                break;
            case ITEM_LGT_SHIELD:
            case ITEM_MED_SHIELD:
            case ITEM_HVY_SHIELD:
                dWorth = dBaseProb * ((double) item[currItem].vals[0] / (double) item[ITEM_HVY_SHIELD].vals[0]);
                dWorth *= defensive;
                break;
            case ITEM_INTENSITY_AMP:
            case ITEM_VIOLENT_FORCE:
                dWorth = dBaseProb * ((double) item[currItem].vals[0] / (double) item[ITEM_VIOLENT_FORCE].vals[0]);
                dWorth *= (-1.0 * defensive);
                break;
            case ITEM_LGT_REPULSOR_SHIELD:
            case ITEM_MED_REPULSOR_SHIELD:
            case ITEM_HVY_REPULSOR_SHIELD:
                dWorth = dBaseProb * ((double) item[currItem].vals[0] / (double) item[ITEM_HVY_REPULSOR_SHIELD].vals[0]);
                dWorth *= (-1.0 * defensive);
                break;
            case ITEM_REPAIRKIT:
                dWorth = dBaseProb * ((defensive + 1.0) / 2.0);
                break;
            case ITEM_PARACHUTE:
                dWorth = dBaseProb * ((defensive + 1.0) / 1.5);    // Parachutes *are* popular! :)
                break;
            case ITEM_SLICKP:
                dWorth = (int) type * 250;
                break;
            case ITEM_DIMPLEP:
                dWorth = (int) type * 500;
                break;
            case ITEM_FUEL:
                dWorth = -5000;    // Bots don't need  fuel
                bIsWarhead = true;    // Yes, it's a lie. ;-)
            }
            // dWorth must not be greater than the half of MAX_WEAPON_PROBABILITY
            if (dWorth > (MAX_WEAP_PROBABILITY / 2))
                dWorth = MAX_WEAP_PROBABILITY / 2;
        }
        int iValue = fabs(dWorth);
        if (iValue < (MAX_WEAP_PROBABILITY / 10))
            iValue = MAX_WEAP_PROBABILITY / 10;
        dWorth += (double) (rand() % iValue);    // allow to double (more or less)

        if (dWorth > MAX_WEAP_PROBABILITY)
            dWorth = MAX_WEAP_PROBABILITY;
        if (dWorth < (MAX_WEAP_PROBABILITY / 100.0))
            dWorth = MAX_WEAP_PROBABILITY / 100.0;    // Which is very very little...

        if (bIsWarhead)
            _weaponPreference[weapCount] = 0;    // It will not get any slot!
        else
            _weaponPreference[weapCount] = (int) dWorth;

        if ((weapCount  < WEAPONS) && (_weaponPreference[weapCount] > iMaxWeapPref))
            iMaxWeapPref = _weaponPreference[weapCount];
        if ((weapCount >= WEAPONS) && (_weaponPreference[weapCount] > iMaxItemPref))
            iMaxItemPref = _weaponPreference[weapCount];

#ifdef DEBUG
        if (weapCount < WEAPONS)
            printf("%23s (weapon): % 5d", weapon[weapCount].name, _weaponPreference[weapCount]);
        else
            printf("%23s ( item ): % 5d", item[weapCount-WEAPONS].name, _weaponPreference[weapCount]);
        cout << endl;
#endif // DEBUG
    }

    // Before we are finished, we need to amplify the preferences (well, maybe...)
    if (iMaxWeapPref < MAX_WEAP_PROBABILITY)
    {
        // Yes, amplification for the weapons needed!
        dWorth = (double) MAX_WEAP_PROBABILITY / (double) iMaxWeapPref;
        for (int weapCount = 1; weapCount < WEAPONS; weapCount++)
        {
            if (_weaponPreference[weapCount] > (MAX_WEAP_PROBABILITY / 100.0))
            {
                _weaponPreference[weapCount] = (int) ((double) _weaponPreference[weapCount] * dWorth);
#ifdef DEBUG
                printf( "%23s (weapon) amplified to: % 5d", weapon[weapCount].name, _weaponPreference[weapCount]);
                cout << endl;
#endif // DEBUG
            }
        }
    }
    if (iMaxItemPref < MAX_WEAP_PROBABILITY)
    {
        // Yes, amplification for the items needed!
        dWorth = (double) MAX_WEAP_PROBABILITY / (double) iMaxItemPref;
        for (int weapCount = WEAPONS; weapCount < THINGS; weapCount++)
        {
            if (_weaponPreference[weapCount] > (MAX_WEAP_PROBABILITY / 100.0))
            {
                _weaponPreference[weapCount] = (int)((double)_weaponPreference[weapCount] * dWorth);
#ifdef DEBUG
                printf("%23s ( item ) amplified to: % 5d", item[weapCount-WEAPONS].name, _weaponPreference[weapCount]);
                cout << endl;
#endif // DEBUG
            }
        }
    }

#ifdef DEBUG
    cout << "===================================================" << endl << endl;
#endif // DEBUG
}

int PLAYER::selectRandomItem()
{
    // return (_rouletteWheel[rand() % NUM_ROULETTE_SLOTS]);
    return rand() % THINGS;
}

void PLAYER::setName(char *name)
{
    // initalize name
    memset(_name, '\0', NAME_LENGTH);
    strncpy (_name, name, NAME_LENGTH - 1);
}

int PLAYER::controlTank()
{
    if (key[KEY_F1])
        save_bmp("scrnshot.bmp", _env->db, NULL);

    if (key_shifts & KB_CTRL_FLAG && ctrlUsedUp)
    {
        if (key[KEY_LEFT] || key[KEY_RIGHT]
            || key[KEY_UP] || key[KEY_DOWN]
            || key[KEY_PGUP] || key[KEY_PGDN]
            || key[KEY_A] || key[KEY_D]    //additional control
            || key[KEY_W] || key[KEY_S]
            || key[KEY_R] || key[KEY_F])
            ctrlUsedUp = TRUE;
        else
            ctrlUsedUp = FALSE;
    }
    else
        ctrlUsedUp = FALSE;

    if (_global->computerPlayersOnly &&
            ((int)_global->skipComputerPlay >= SKIP_HUMANS_DEAD))
    {
        if (_env->stage == STAGE_ENDGAME)
            return (-1);
    }

    k = 0;
#ifdef NEW_GAMELOOP
    if (keypressed())
#else
    if (keypressed() && !fi)
#endif
    {
        k = readkey();

        if ((_env->stage == STAGE_ENDGAME)
            && (k >> 8 == KEY_ENTER
            || k >> 8 == KEY_ESC
            || k >> 8 == KEY_SPACE))
            return -1;
        if ((k >> 8 == KEY_ESC) || (k >> 8 == KEY_P))
        {
            void clockadd();
            install_int_ex(clockadd, SECS_TO_TIMER(6000));
            int mm = _env->ingamemenu();
            install_int_ex(clockadd, BPS_TO_TIMER(_global->frames_per_second));
            _env->make_update(0, 0, _global->screenWidth, _global->screenHeight);
            _env->make_bgupdate(0, 0, _global->screenWidth, _global->screenHeight);

            //Main Menu
            if (mm == 1)
            {
                _global->wr_lock_command();
                _global->command = GLOBAL_COMMAND_MENU;
                _global->unlock_command();
                return -1;
            }
            else if (mm == 2)    // Quit game
            {
                _global->wr_lock_command();
                _global->command = GLOBAL_COMMAND_QUIT;
                _global->unlock_command();
                return -1;
            }
            else if (mm == 3)    // skip AI
                return -2;
        }
        // check for number key being pressed
        if ( (k >> 8 >= KEY_0) && (k >> 8 <= KEY_9) )
        {
            int value = (k >> 8) - KEY_0;

            // make sure the value is within range
            if (value < _global->numPlayers)
            {
                if ( _global->players[value] )
                {
                    TANK *my_tank = _global->players[value]->tank;
                    if (my_tank)
                    {
                        sprintf(_global->tank_status, "%s: %d + %d -- Team: %s", _global->players[value]->_name, my_tank->l, my_tank->sh, _global->players[value]->Get_Team_Name() );
                        /* We do this in atanks.cpp, to kill this wretched "No Format Error"
                      strcat(_global->tank_status, _global->players[value]->Get_Team_Name()); */
                        _global->tank_status_colour = _global->players[value]->color;
                        _global->updateMenu = 1;
                    }
                    else
                        _global->tank_status[0] = 0;
                }
            }

        }    // end of check status keys
        
        // Check for scorboard toggle key
        if (k >> 8 == KEY_TILDE)
        {
            _global->show_scoreboard = _global->show_scoreboard == TRUE ? FALSE : TRUE;
            _env->make_update (0, 0, _global->screenWidth, _global->screenHeight);
            _env->make_bgupdate (0, 0, _global->screenWidth, _global->screenHeight);
        }

        if ((k & 0xff) == 'v')
            _env->decreaseVolume();
        else if ((k & 0xff) == 'V')
            _env->increaseVolume();
    }

    if ((int) type == HUMAN_PLAYER || !tank)
        return (humanControls ());

#ifdef NETWORK
    else if ((int) type == NETWORK_CLIENT)
        return (Execute_Network_Command(TRUE));
#endif
    else if (_env->stage == STAGE_AIM)
        return (computerControls());

    return 0;
}

int PLAYER::computerSelectPreBuyItem(int aMaxBoostValue)
{
    double dMood = 1.0 + defensive + (double) ((double) (rand() / ((double) RAND_MAX / 2.0)));
    // dMood is 0.0 <= x <= 4.0
    int currItem = 0;
    /* Prior buying anything else, a 5 step system takes place:
       1.: Parachutes (if gravity is on)
       2.: Minimum weapon probability (aka 5 medium and 3 large missiles
       3.: Armor/Amps
       4.: "Tools" to free themselves like Riot Blasts
       5.: Shields, if enough money is there
       6.: if all is set, look for dimpled/slick projectiles! */

    // Step 1:

    if ((type >= RANGEFINDER_PLAYER) && (_env->landSlideType > LANDSLIDE_NONE) && (ni[ITEM_PARACHUTE] < 10))
        currItem = WEAPONS + ITEM_PARACHUTE;

    // Step 3:
    // Even here bots might forget
    if (!currItem && (rand() % ((int) type + 1)))
    {
        int iLimit = aMaxBoostValue - getBoostValue(); // > 0 means: Someone has more than we have!
#ifdef DEBUG
        printf( "%10s: Boost: %4d, Max: %4d, Limit: %4d\n", getName(), getBoostValue(), aMaxBoostValue, iLimit);
#endif // DEBUG
        if ( ((dMood >= 2.75) && (iLimit > 0)) || ((dMood >= 2.0) && (iLimit > getBoostValue())) )
        {
            // The player is in a defensive mood
            // If we have 25% more money than the plasteel cost, buy it, else the armour will do
            if (money >= (item[ITEM_PLASTEEL].cost * 1.25))
                currItem = WEAPONS + ITEM_PLASTEEL;
            else
                if ( (money >= (item[ITEM_ARMOUR].cost * 2.0)) && ((ni[ITEM_ARMOUR] < ni[ITEM_PLASTEEL]) || (dMood >= 3.5)) )
                    currItem = WEAPONS + ITEM_ARMOUR;
        }

        // Now iBoostItemsBought must be checked:
        if (currItem && (iBoostItemsBought >= (int) type))
            currItem = 0;    // Sorry, bought enough this round!
        if (currItem && (iBoostItemsBought < (int) type))
            iBoostItemsBought++;    // Okay, take it!
    }

    // 5.: Shields
    if (!currItem && (rand() % ((int) type + 1)) && ((int) type >= RANGEFINDER_PLAYER))
    {
        if (dMood <= 1.5)
        {
            // offensive type, go through reflectors
            if ( (ni[ITEM_LGT_REPULSOR_SHIELD] <= (item[ITEM_LGT_REPULSOR_SHIELD].amt * (int) type))
                 && (money >= (item[ITEM_LGT_REPULSOR_SHIELD].cost * 2.0)) )
                currItem = WEAPONS + ITEM_LGT_REPULSOR_SHIELD;

            if ( (ni[ITEM_MED_REPULSOR_SHIELD] <= (item[ITEM_MED_REPULSOR_SHIELD].amt * (int) type))
                 && (money >= (item[ITEM_MED_REPULSOR_SHIELD].cost * 1.75)) )
                currItem = WEAPONS + ITEM_MED_REPULSOR_SHIELD;

            if ( (ni[ITEM_HVY_REPULSOR_SHIELD] <= (item[ITEM_HVY_REPULSOR_SHIELD].amt * (int) type))
                 && (money >= (item[ITEM_HVY_REPULSOR_SHIELD].cost * 1.5)) )
                currItem = WEAPONS + ITEM_HVY_REPULSOR_SHIELD;
        }

        if (dMood >= 2.5)
        {
            // defensive type, go through hard shields
            if ( (ni[ITEM_LGT_SHIELD] <= (item[ITEM_LGT_SHIELD].amt * (int) type))
                 && (money >= (item[ITEM_LGT_SHIELD].cost * 2.0)) )
                currItem = WEAPONS + ITEM_LGT_SHIELD;

            if ( (ni[ITEM_MED_SHIELD] <= (item[ITEM_MED_SHIELD].amt * (int) type))
                 && (money >= (item[ITEM_MED_SHIELD].cost * 1.75)) )
                currItem = WEAPONS + ITEM_MED_SHIELD;

            if ( (ni[ITEM_HVY_SHIELD] <= (item[ITEM_HVY_SHIELD].amt * (int) type))
                 && (money >= (item[ITEM_HVY_SHIELD].cost * 1.5)) )
                currItem = WEAPONS + ITEM_HVY_SHIELD;
        }
    }

    return currItem;
}

int PLAYER::chooseItemToBuy(int aMaxBoostValue)
{
    int currItem = computerSelectPreBuyItem(aMaxBoostValue);
    int cumulative;
    int curramt, newamt;
    int iTRIES = THINGS;    // pow((int)type + 1, 2);
    int iDesiredItems[THINGS];    // Deadly bots have large shopping carts. ;)
    bool bIsSorted = false;    // Whether the cart is sorted or not
    bool bIsPreSelected = currItem ? true : false;    // Whether or not the PreBuy steps found something
    int i = 0;

    if (currItem)
    {
        if (Buy_Something(currItem))
            return currItem;
    }

    // init desired items
    for (i = 0; i < iTRIES; i++)
        iDesiredItems[i] = 0;

    // 1.: Fill cart
    i = 0;
    while (i < iTRIES)
    {
        currItem = (int) fabs(selectRandomItem());
        if (currItem >= THINGS)
            currItem %= THINGS;    // Put in range
        // now currItem is 0<= currItem < THINGS
        if ((_env->isItemAvailable(currItem)) && (currItem != 0))
            iDesiredItems[i] = currItem;
        i++;
    }

    // 2.: sort these items by preferences
    while (!bIsSorted)
    {
        if (bIsPreSelected)
            i = 2; // The first item shall not be sorted somewhere else!
        else
            i = 1;
        bIsSorted = true;
        while (i < iTRIES)
        {
            if (_weaponPreference[iDesiredItems[i-1]] < _weaponPreference[iDesiredItems[i]])
            {
                bIsSorted = false;
                currItem = iDesiredItems[i];
                iDesiredItems[i] = iDesiredItems[i-1];
                iDesiredItems[i-1] = currItem;
            }
            i++;
        }
    }

    // 3.: loop through all weapon preferences
    for (int i = 0; i < iTRIES; i++)
    {
        currItem = iDesiredItems[i];
        int itemNum = currItem - WEAPONS;
        int nextMod = 1;

        //determine the likelyhood of purchasing this selection
        //less likely the more of the item is owned
        //if have zero of item, it is a fifty/fifty chance of purchase
        if (currItem < WEAPONS)
        {
            curramt = nm[currItem];
            newamt = weapon[currItem].amt;
            cumulative = FALSE;
        }
        else
        {
            curramt = ni[itemNum];
            newamt = item[itemNum].amt;
            if ((itemNum >= ITEM_INTENSITY_AMP && itemNum <= ITEM_VIOLENT_FORCE)
                || (itemNum == ITEM_REPAIRKIT) || (itemNum >= ITEM_ARMOUR && itemNum <= ITEM_PLASTEEL))
                cumulative = TRUE;
            else
                cumulative = FALSE;
        }

        if (!cumulative)
            nextMod = curramt / newamt;
        if (nextMod <= 0)
            nextMod = 1;
        if (rand() % nextMod)
            continue;

        //weapon
        if (currItem < WEAPONS)
        {
            //don't buy if already maxed out
            if (nm[currItem] >= 99)
                continue;

            //purchase the item
            if (money >= weapon[currItem].cost)
            {
                money -= weapon[currItem].cost;
                nm[currItem] += weapon[currItem].amt;
                //don't allow more than 99
                if (nm[currItem] > 99)
                    nm[currItem] = 99;
                return currItem;
            }
        }
        else   //item
        {
            //don't buy if already maxed out
            if (ni[itemNum] >= 99)
                continue;
            //purchase the item
            if (money >= item[itemNum].cost)
            {
                // Check against iBoostItemsBought
                if ((itemNum >= ITEM_INTENSITY_AMP && itemNum <= ITEM_VIOLENT_FORCE)
                    || (itemNum >= ITEM_ARMOUR && itemNum <= ITEM_PLASTEEL))
                {
                    if ((iBoostItemsBought >= (int) type) ||(getBoostValue() > aMaxBoostValue))
                        continue;    // no chance pal!
                    else
                        iBoostItemsBought++;    // Okay, take it!
                }
                money -= item[itemNum].cost;
                ni[itemNum] += item[itemNum].amt;
                //don't allow more than 999
                if (ni[itemNum] > 99)
                    ni[itemNum] = 99;
                return currItem;
            }
        }
    }
    return -1;
}

// An item has been selected, this function merely buys it. It
// first does checks to make sure the item can be bought.
// The function returns TRUE if we successfuly bought the item or
// FALSE if we could not get it for some reason.
int PLAYER::Buy_Something(int currItem)
{
    int itemNum = currItem - WEAPONS;
    int bought = FALSE;

    if (currItem < WEAPONS)
    {
        if ((money >= weapon[currItem].cost) && (nm[currItem] < 99))
        {
            money -= weapon[currItem].cost;
            nm[currItem] += weapon[currItem].amt;
            //don't allow more than 99
            if (nm[currItem] > 99)
                nm[currItem] = 99;
            bought = TRUE;
        }
        // check tech level
        if (weapon[currItem].techLevel <= _env->weapontechLevel)
            bought = TRUE;
        else
            bought = FALSE;

    }    // end of weapons

    else   // item
    {
        if ((money > item[itemNum].cost) && (ni[itemNum] < 99))
        {
            money -= item[itemNum].cost;
            ni[itemNum] += item[itemNum].amt;
            // don't allow more than 99
            if (ni[itemNum] > 99)
                ni[itemNum] = 99;
            bought = TRUE;
        }
        // check technology level
        if (item[itemNum].techLevel <= _env->itemtechLevel)
            bought = TRUE;
        else
            bought = FALSE;
    }
    return bought;
}

char *PLAYER::selectRevengePhrase()
{
    char *line;

    line = _global->revenge->Get_Random_Line();
    return line;
}

char *PLAYER::selectGloatPhrase()
{
    char *line;
    line = _global->gloat->Get_Random_Line();
    return line;
}

char *PLAYER::selectSuicidePhrase()
{
    char *line;
    line = _global->suicide->Get_Random_Line();
    return line;
}

char *PLAYER::selectKamikazePhrase()
{
    char *line;
    line = _global->kamikaze->Get_Random_Line();
    return line;
}

char *PLAYER::selectRetaliationPhrase()
{
    char *line;
    char *pText;

    if (!revenge)
        return NULL;

    line = _global->retaliation->Get_Random_Line();
    pText = (char *) calloc (strlen (revenge->getName()) + 32 + strlen(line), sizeof (char));
    if (!pText)
        return NULL;

    strcpy(pText, line);
    strcat(pText, revenge->getName());
    strcat(pText, "!!!");

    return pText;
}

int PLAYER::traceShellTrajectory(double aStartX, double aStartY, double aVelocityX, double aVelocityY, double &aReachedX, double &aReachedY)
{
    TANK *tankPool[10];    // For repulsion
    bool bHasHit = false;    // will be set to true if something is hit
    bool bIsWallHit = false;    // For steel walls and wrap floor/ceiling
    bool bIsWrapped = false;    // For special handling in distance calcualtion when a shot goes through the wall
    int iWallBounce = 0;    // For wrap, rubber and spring walls
    int iMaxBounce = pow((int) type, 2);    // Useless can calculate 1, deadly bots 25 bounces
    int iTargetDistance = ABSDISTANCE(_targetX, _targetY, aStartX, aStartY);
    int iDistance = MAX_OVERSHOOT;
    int iDirection = 0;    // records the current direction of the shot
    int iPriStageTicks = 0;    // There is no unlimited tracking!
    int iMaxPriStTicks = MAX_POWER * focusRate * 2.5;
    int iSecStageTicks = 0;    // rollers and burrowers can't be followed forever!
    int iMaxSecStTicks = MAX_POWER * focusRate * 1.5;
    double dDrag = weapon[tank->cw].drag;
    double dMass = weapon[tank->cw].mass;
    double dPosX = aStartX;
    double dPosY = aStartY;
    double dVelX = aVelocityX;
    double dVelY = aVelocityY;
    double dMaxVelocity = 0;
    double dPrevX, dPrevY;    // to check pixels between two points
    bool bIsSecondStage = false;    // Applies to burrowers and rollers

    dMaxVelocity = _global->dMaxVelocity * (1.20 + (dMass / ((double) MAX_POWER / 10.0)));

    // Set drag do the correct value:
    if (ni[ITEM_DIMPLEP])
        dDrag *= item[ITEM_DIMPLEP].vals[0];
    else if (ni[ITEM_SLICKP])
        dDrag *= item[ITEM_SLICKP].vals[0];

    // Fill tankPool
    for (int i = 0; i < _global->numPlayers; i++)
    {
        if ((_global->players[i]) && (_global->players[i]->tank))
            tankPool[i] = _global->players[i]->tank;
        else
            tankPool[i] = NULL;
    }

    // Initial direction:
    if (dVelX > 0.0)
        iDirection = 1;
    if (dVelX < 0.0)
        iDirection = -1;

    // On y va!
    while (!bHasHit && !bIsWallHit && (iWallBounce < iMaxBounce) && (iPriStageTicks < iMaxPriStTicks)
           && (iSecStageTicks < iMaxSecStTicks))
    {
        /* --- First Stage - Applies to all weapons --- */
        if (!bIsSecondStage)
        {
            // Apply Repulsor effects
            for (int i = 0; i < _global->numPlayers; i++)
            {
                if (tankPool[i] && (tankPool[i] != tank))
                {
                    double xAccel = 0.0, yAccel = 0.0;
                    tankPool[i]->repulse (dPosX + dVelX, dPosY + dVelY, &xAccel, &yAccel, tank->cw);
                    if (tankPool[i] == _target)
                    {
                        // Without this, the shield would be nearly useless!
                        xAccel *= focusRate;
                        yAccel *= focusRate;
                        // But the lesser bots wouldn't hit anything anymore if more than _target would be handled like that!
                    }
                    dVelX += xAccel;
                    dVelY += yAccel;
                }
            }

            dPrevX = dPosX;
            dPrevY = dPosY;
            //motion - wind affected
            if ( ((dPosX + dVelX) < 1) || ((dPosX + dVelX) > (_global->screenWidth - 2)) )
            {
                if ( (((dPosX + dVelX) < 1) && checkPixelsBetweenTwoPoints(_global, _env, &dPrevX, &dPrevY, 1, dPosY))
                     || (((dPosX + dVelX) > (_global->screenWidth - 2))
                     && checkPixelsBetweenTwoPoints(_global, _env, &dPrevX, &dPrevY, (_global->screenWidth - 2), dPosY)) )
                {
                    dPosX = dPrevX;
                    dPosY = dPrevY;
                    bHasHit = true;
                }
                else
                {
                    switch (_env->current_wallType)
                    {
                    case WALL_RUBBER:
                        dVelX = -dVelX;    //bounce on the border
                        iWallBounce++;
                        break;
                    case WALL_SPRING:
                        dVelX = -dVelX * SPRING_CHANGE;
                        iWallBounce++;
                        break;
                    case WALL_WRAP:
                        if (dVelX < 0)
                            dPosX = _global->screenWidth - 2;    // -1 is the wall itself
                        else
                            dPosX = 1;
                        iWallBounce++;
                        bIsWrapped = true;
                        break;
                    default:
                        dPosX += dVelX;
                        if (dPosX < 1)
                            dPosX = 1;
                        if (dPosX > (_global->screenWidth - 2))
                            dPosX= _global->screenWidth - 2;
                        dVelX = 0;    // already applied!
                        bIsWallHit = true;
                    }
                }
            }

            // hit floor or boxed top
            if ( ((dPosY + dVelY) >= (_global->screenHeight - 1)) || (((dPosY + dVelY) <= MENUHEIGHT) && _global->bIsBoxed) )
            {
                if ( (_global->bIsBoxed && ((dPosY + dVelY) <= MENUHEIGHT)
                     && checkPixelsBetweenTwoPoints(_global, _env, &dPrevX, &dPrevY, dPosX, MENUHEIGHT + 1))
                     || (((dPosY + dVelY) > (_global->screenHeight - 2))
                     && checkPixelsBetweenTwoPoints(_global, _env, &dPrevX, &dPrevY, dPosX, (_global->screenHeight - 2))) )
                {
                    dPosX = dPrevX;
                    dPosY = dPrevY;
                    bHasHit = true;
                }
                else
                {
                    switch (_env->current_wallType)
                    {
                    case WALL_RUBBER:
                        dVelY = -dVelY * 0.5;
                        dVelX *= 0.95;
                        iWallBounce++;
                        break;
                    case WALL_SPRING:
                        dVelY = -dVelY * SPRING_CHANGE;
                        dVelX *= 1.05;
                        iWallBounce++;
                        break;
                    default:
                        // steel or wrap floor (ceiling)
                        dPosY += dVelY;
                        if (dPosY >= (_global->screenHeight - 1))
                            dPosY = _global->screenHeight - 2;    // -1 would be the floor itself!
                        else
                            dPosY= MENUHEIGHT + 1;    // +1 or it would be the wall itself
                        dVelY = 0; // already applied!
                        bIsWallHit = true;
                    }
                    if (!bIsWallHit && ((fabs(dVelX) + fabs(dVelY)) < 0.8))
                        bHasHit = true;
                }
            }

            // velocity check:
            double dActVelocity = ABSDISTANCE(dVelX, dVelY, 0, 0); // a²+b²=c² ... says Pythagoras :)
            if ((dActVelocity > dMaxVelocity) && !bHasHit && !bIsWallHit)
            {
                // Velocity is applied first and modified by errorMultiplier
                dPosY += dVelY * errorMultiplier;
                dPosX += dVelX * errorMultiplier;
                dVelX = 0.0;
                dVelY = 0.0;
                if ((dPosY <= MENUHEIGHT) && _global->bIsBoxed)
                    dPosY = MENUHEIGHT + 1;
                if (dPosY > (_global->screenHeight - 2))
                    dPosY = _global->screenHeight - 2;
                if (dPosX < 1)
                {
                    if (_env->current_wallType == WALL_WRAP)
                        dPosX += _global->screenWidth - 2;
                    else
                        dPosX = 1;
                }
                if (dPosX > (_global->screenWidth - 2))
                {
                    if (_env->current_wallType == WALL_WRAP)
                        dPosX -= _global->screenWidth - 2;
                    else
                        dPosX = _global->screenWidth - 2;
                }
                bHasHit = true;
            }

            // Now check for hits
            if (!bHasHit && !bIsWallHit)
            {
                // Save Positions:
                dPrevX = dPosX;
                dPrevY = dPosY;
                // Apply movement:
                dPosX += dVelX;
                dPosY += dVelY;
                dVelX += (double) (_env->wind - dVelX) / dMass * dDrag * _env->viscosity;
                dVelY += _env->gravity * (100.0 / _global->frames_per_second);
                // Barrier test:
                if ((dVelY <= -1.0) && (dPosY <= (_global->screenHeight * -25.0)))
                    dVelY *= -1.0;

                // See, if we have hit something
                // --- Environment-Test -- flight?
                if (checkPixelsBetweenTwoPoints(_global, _env, &dPrevX, &dPrevY, dPosX, dPosY))
                {
                    dPosX = dPrevX;
                    dPosY = dPrevY;
                    bHasHit = true;
                }
            }
            // Now that all modifications are applied, record direction:
            if (dVelX > 0.0)
                iDirection = 1;
            if (dVelX < 0.0)
                iDirection = -1;
        }

        /* --- Second Stage - Applies to burrowers and rollers --- */
        if (bIsSecondStage)
        {
            iSecStageTicks++;
            // The weapon is on the ground and rolling or penetrating the ground:
            if ((tank->cw >= SML_ROLLER) && (tank->cw <= DTH_ROLLER))
            {
                // check whether we have hit anything
                if ((dPosX < 1) || (dPosX > (_global->screenWidth - 2)) || (dPosY > (_global->screenHeight - 2))
                    || (getpixel(_env->terrain, (int) dPosX, (int) dPosY) != PINK))
                    bHasHit = true;
                else
                {
                    // roll roller
                    float fSurfY = (float) _env->surface[(int) dPosX] - 1;
                    if ((fSurfY > dPosY) && (dPosY < (_global->screenHeight - 5)))
                    {
                        if (fSurfY < (dPosY + 5))
                            dPosY = fSurfY;
                        else
                            dPosY+=5;
                    }
                    else
                    {
                        if (dVelX > 0.0)
                            dPosX++;
                        else
                            dPosX--;
                        if (dPosY >= MENUHEIGHT)
                        {
                            if (fSurfY > dPosY)
                                dPosY++;
                            else if (fSurfY >= (dPosY - 2))
                                dPosY = fSurfY - 1;
                        }
                    }
                    if (dPosY > (_global->screenHeight - 5) && !bHasHit)
                        dPosY = (_global->screenHeight - 5);
                }
            }

            if ((tank->cw >= BURROWER) && (tank->cw <= PENETRATOR))
            {
                // Apply Repulsor effects, but not fully, as it is a burrower!
                for (int i = 0; i < _global->numPlayers; i++)
                {
                    if (tankPool[i] && (tankPool[i] != tank))
                    {
                        double xAccel = 0.0, yAccel = 0.0;
                        tankPool[i]->repulse(dPosX + dVelX, dPosY + dVelY, &xAccel, &yAccel, tank->cw);
                        if (tankPool[i] == _target)
                        {
                            // Without this, the shield would be nearly useless!
                            xAccel *= focusRate;
                            yAccel *= focusRate;
                            // But the lesser bots wouldn't hit anything anymore if more than _target would be handled like that!
                        }
                        dVelX += xAccel * 0.1;
                        dVelY += yAccel * 0.1;
                    }
                }
                if ( ((dPosX + dVelX) < 1) || ((dPosX + dVelX) > (_global->screenWidth-1)) )
                {
                    // if the wall is rubber, then bouce
                    if (_env->current_wallType == WALL_RUBBER)
                        dVelX = -dVelX;    //bounce on the border
                    // bounce with more force
                    else if (_env->current_wallType == WALL_SPRING)
                        dVelX = -dVelX * SPRING_CHANGE;
                    // wall is steel, detonate
                    else if (_env->current_wallType == WALL_STEEL)
                        bHasHit = true;
                    // wrap around to other side of the screen
                    else if (_env->current_wallType == WALL_WRAP)
                    {
                        if (dVelX < 1)
                            dPosX = _global->screenWidth - 2;
                        else
                            dPosX = 1;
                    }
                }
                if ((dPosY + dVelY) >= (_global->screenHeight - 1))
                {
                    dVelY = -dVelY * 0.5;
                    dVelX *= 0.95;
                }
                else if ((dPosY + dVelY) < MENUHEIGHT)    //hit screen top
                {
                    dVelY = -dVelY *0.5;
                    dVelX *= 0.95;
                }
                dPosY += dVelY;
                dPosX += dVelX;
                dVelY -= _env->gravity * 0.05 * (100.0 / _global->frames_per_second);
                if (getpixel(_env->terrain, (int) dPosX, (int) dPosY) == PINK)
                    bHasHit = true;
            }
        }
        else
            iPriStageTicks++;

#ifdef DEBUG_AIM_SHOW
        if (_global->bASD)
            circlefill(screen, (int) dPosX, (int) dPosY, 2, color);    // Plot trajectories for debugging purposes
#endif

        /* --- Third Stage - Applies to burrowers and rollers --- */
        if ( (!bIsSecondStage && (bHasHit || bIsWallHit)) && (((tank->cw >= SML_ROLLER)
             && (tank->cw <= DTH_ROLLER)) || ((tank->cw >= BURROWER) && (tank->cw <= PENETRATOR))) )
        {
            // Only Rollers and Penetrators can enter the second stage!
            bIsSecondStage = true;
            bHasHit = false;
            bIsWallHit = false;
            if ((tank->cw >= SML_ROLLER) && (tank->cw <= DTH_ROLLER))
            {
                dPosY -= 5;
                if (dPosX < 1)
                    dPosX = 1;
                if (dPosX > (_global->screenWidth - 2))
                    dPosX = (_global->screenWidth - 2);

                if ((dPosY >= _env->surface[(int)dPosX])    // y is surface or below
                    &&(dPosY <= _env->surface[(int)dPosX] + 2))    // but not burried more than 2 px
                    dPosY = _env->surface[(int)dPosX] - 1;

                dVelX = 0.0;
                if (getpixel(_env->terrain, (int)dPosX + 1, (int)dPosY + 1) == PINK)
                    dVelX = 1.0;
                if (getpixel(_env->terrain, (int)dPosX - 1, (int)dPosY + 1) == PINK)
                    dVelX = -1.0;
                if (dVelX == 0.0)
                    // if both sides are free (should be impossible, but might be a 1-pixel-peak) the shooting direction decides
                    dVelX = (double) iDirection;
                dVelY = 0.0;
            }
            if ((tank->cw == BURROWER) || (tank->cw == PENETRATOR))
            {
                dVelX *= 0.1;
                dVelY *= 0.1;
            }
            if ((dPosY <= MENUHEIGHT) && !_global->bIsBoxed)
                dPosY = MENUHEIGHT + 1;
            if ((dPosY <= MENUHEIGHT) && _global->bIsBoxed)
                bIsWallHit = true; // Sorry, no more ceiling-drop!
        }

        /* --- Fourth Stage - Tank hit test --- */
        if (!bIsWallHit)
        {
            for (int i = 0; i < _global->numPlayers; i++)
            {
                if (tankPool[i])
                {
                    if ((dPosX > (tankPool[i]->x - TANKWIDTH)) && (dPosX < (tankPool[i]->x + TANKWIDTH))
                        && (dPosY > (tankPool[i]->y)) && (dPosY < (tankPool[i]->y + TANKHEIGHT)) && (tankPool[i]->l > 0))
                        bHasHit = true;
                }
            }
        }

        // if something is hit, be sure the values are in range and movement stopped!
        if (bHasHit || bIsWallHit)
        {
            dVelX = 0.0;
            dVelY = 0.0;
            if (dPosY <= MENUHEIGHT)
                dPosY = MENUHEIGHT + 1;
            if (dPosY > (_global->screenHeight - 2))
                dPosY = _global->screenHeight - 2;
            if (dPosX < 1)
                dPosX = 1;
            if (dPosX > (_global->screenWidth - 2))
                dPosX = _global->screenWidth - 2;
        }
    }

#ifdef DEBUG_AIM_SHOW
    if (_global->bASD)
    {
        // Targetting circle for debugging purposes
        circlefill(screen, (int) dPosX, (int) dPosY, 10, BLACK);
        circlefill(screen, (int) _targetX, (int) _targetY, 20, color);
        LINUX_REST;
    }
#endif
#ifdef DEBUG_AIM
    if (bIsWallHit)
        cout << "WALL ";
    if (bHasHit)
        cout << "HIT ";
    if (iWallBounce >= iMaxBounce)
        cout << "BOUNCE (" << iMaxBounce << ") ";
    if (iPriStageTicks >= iMaxPriStTicks)
        cout << "TICKS1 (" << iPriStageTicks << ") ";
    if (iSecStageTicks >= iMaxSecStTicks)
        cout << "TICKS2 (" << iSecStageTicks << ") ";
#endif // DEBUG_AIM

    // Now see where we are and calculate the distance difference between (char *)"hit" and "has to be hit"
    if (!bIsWallHit && (iWallBounce < iMaxBounce))
    {
        iDistance = ABSDISTANCE(_targetX, _targetY, dPosX, dPosY);

#ifdef DEBUG_AIM
        cout << "(" << iDistance << " <- " << (int) dPosX << " x " << (int) dPosY << ") ";
#endif // DEBUG_AIM

        // Special handling for wrapped shots:
        if (bIsWrapped)
        {
            int iHalfX = _global->halfWidth;

            // Only shots where dPosX and _targetX are on opposite sides are relevant
            if ( (((int) dPosX < iHalfX) && (_targetX >=iHalfX)) || (((int) dPosX >=iHalfX) && (_targetX < iHalfX)) )
            {
                if (((int) dPosX < iHalfX) && (_targetX >=iHalfX))
                    iDistance = ABSDISTANCE(dPosX + iHalfX, dPosY, _targetX, _targetY);
                else
                    iDistance = ABSDISTANCE(_targetX + iHalfX, _targetY, dPosX, dPosY);
            }
            else
                bIsWrapped = false; // not relevant
        }

        bool bIsWrongDir = false;
        if ( !bIsWrapped && (((dPosX < aStartX) && (_targetX > aStartX)) || ((dPosX > aStartX) && (_targetX < aStartX))) )
            bIsWrongDir = true; // wrong side!

        if (!iDirection)
        {
            // If we have no direction (very unlikely!) we can only guess by comparing x-coordinates:
            if ( (iTargetDistance > ABSDISTANCE(aStartX, aStartY, dPosX, dPosY)) || (bIsWrongDir
                 && (_env->current_wallType != WALL_STEEL)) )
                // Too short or wrong direction, negate iDistance!
                iDistance *= -1;
        }
        else
        {
            // with the help of the direction, we can exactly see, whether the shot is too short or too far
            if ( !bIsWrapped    // This doesn't apply for wrapped shots!
                 && (((iDirection < 0.0)    // shoot to the left...
                 && (dPosX > _targetX))    // ...and the shot hits right of target
                 || ((iDirection > 0.0)    // shoot to the right...
                 && (dPosX < _targetX))) )    // ...and the shot hits left of target
                iDistance *= -1;    // too short!
        }

        if (bIsWrongDir && (_env->current_wallType == WALL_STEEL))
            iDistance = MAX_OVERSHOOT; // wrong side and target unreachable!
    }
    else
        iDistance = MAX_OVERSHOOT;

    // Give the X- and Y-position back:
    aReachedX = dPosX;
    aReachedY = dPosY;

    return iDistance;
}

int PLAYER::rangeFind(int &aAngle, int &aPower)
{
#ifdef DEBUG_AIM
    printf("This is range find function\n");
#endif
    int iActOvershoot = MAX_OVERSHOOT;
    int iBestOvershoot = MAX_OVERSHOOT + 1;    // So there will be at least one recorded!
    int iAngle = aAngle;
    int iAngleMod = 0;
    double dAngleMul = 0.0;
    double dPowerMul = 0.0;
    int iCalcAngle = aAngle;
    int iLastAngle = aAngle;
    int iBestAngle = aAngle;
    int iAngleBarrier = aAngle;    // This will be the flattest angle possible, thus normalized aAngle + savety
    int iIdealAngle = 135;    // 135 translates to 45°, but will be raised if the barrier is higher
    bool bIsRaisingMode = false;
    int iPower = aPower;
    int iPowerMod = 0;
    int iPowerModFixed = 0;
    int iLastPower = aPower;
    int iBestPower = aPower;
    int iAttempts = 0;
    int iSpread = weapon[tank->cw].spread;
    int iSpreadOdd[] = {0,-1 * SPREAD, SPREAD, -2 * SPREAD, 2 * SPREAD};
    int iSpreadEven[] = { int(-0.5 * SPREAD), int(0.5 * SPREAD), int(-1.5 * SPREAD), 1, int(5 * SPREAD)};
    int iMaxSpread = (int) type;    // more intelligent bots can calculate more shots bearing spreads!
    double dStartX, dStartY, dVelocityX, dVelocityY;    // No initialization needed, they are calculated anyway.
    double dHitX, dHitY;

#ifdef DEBUG_AIM
    printf("Starting RangeFind\n");
#endif

    // We need to be sure that iSpread is at least 1:
    if (iSpread < 1)
        iSpread = 1;

    // iMaxSpread needs to be adapted:
    switch ((int) type)
    {
    case USELESS_PLAYER:
    case GUESSER_PLAYER:
        if (iSpread % 2)
            iMaxSpread = 1;
        else
            iMaxSpread = 2;
        break;
    case RANGEFINDER_PLAYER:
        if (iSpread % 2)
            iMaxSpread = 3;
        else
            iMaxSpread = 4;
        break;
    case TARGETTER_PLAYER:
        if (iSpread % 2)
            iMaxSpread = 5;
        else
            iMaxSpread = 4;
        break;
    default:
        if (iSpread % 2)
            iMaxSpread = 9;    // I know there is nothing like that...
        else
            iMaxSpread = 8; // ... but maybe in the future?
        break;
    }
    if (iSpread > iMaxSpread)
        iSpread = iMaxSpread;    // Now iSpread is limited to iMaxSpread...
    // ...adapted to a test like (iSpreadCount < iSpread) but not larger then weapon[x].spread

    /* Outline:
   * RangeFind tries to adapt the given angle and power to minimize overshoot.
   * to do so we have some facts to keep in mind:
   * - ideal angle is 45° in both directions, giving most distance.
   * - This angle translates into 135° for a shoot to the left, and 225° to the right
   * - aAngle is considered to be the flattest shot possible, as it has been manipulated
   *   by calculateAttackValues(), meaning that lowering it will lead to an obstacle
   *   hit.
   * - as a safety measure, when lowering an angle, a savety range of (int)type above
   *   aAngle is applied
   *
   * The calculation is done in four steps:
   * 1.: Calculate the real angle, as it is, at least for spreads, different with every shot shell
   * 2.: Calculate the overshoot for each shot (or the one if no spread is given)
   * 3.: Record angle, power and overshoot if the overshoot is smaller than the best recorderd so far
   * 4.: Alter angle, or power if the angle can't be manipulated anymore, and start over, if the best
   *     overshoot is != 0;
   *
   * This fourth step is divided into the following parts:
   * a) short shots (overshoot < 0) - raise power
   *    i.: angle lowering mode     - adjust the angle towards 45° until it (or aAngle) is reached
   *    ii.:power raising mode      - after the first positive overshoot is reached, the angle won't be changed any more
   * b) long shots (overshoot > 0)  - lower power
   *    i.: angle raising mode      - adjust the angle towards 89° until it is reached
   *    ii.:power lowering mode     - once the angle can't be changed any more, only power is changed
   * angle raising/lowering modes are entered, and not left until iLastAngle == iAngle
   *
   * Note: Before you think all those step 4 calculations are wrong, they aren't, I am using a math trick to reduce
   *       the amount of calculations:
   *       180 + (180 - Angle) flips an angle between left and right. So the angle will be normalized to the right,
   *       (aka 90° - 180°) and all calculations done there. If it was flipped, it will be flipped back for
   *       traceShellTrajectory. This saves alot of (char *)"if(angle > 180)" lines!
   */

    // Preperations:
    if (aAngle > 180)
        iAngleBarrier = 180 + (180 - iAngleBarrier);    // flip

    // if we have some (char *)"space", add a savety distance:
    if (iAngleBarrier < 170)
        iAngleBarrier += (int) type - 1;

    // We need some savety distance for spreads:
    if (weapon[tank->cw].spread > 1)
    {
        // normal spread size:
        iAngleBarrier += SPREAD * ((int) weapon[tank->cw].spread / 2);
        if (!(weapon[tank->cw].spread % 2))
            // Even spreads have half of SPREAD more than neccessary, so adapt:
            iAngleBarrier -= SPREAD / 2;
        // but be sure the barrier isn't too large:
        if (iAngleBarrier > 179)
            iAngleBarrier = 179;
    }

    // Adapt the ideal angle if we are facing an obstacel:
    if (iAngleBarrier > 135)
        iIdealAngle = iAngleBarrier;

#ifdef DEBUG_AIM
    printf("New Target Try using....\n");
    if (tank->cw < WEAPONS)
        printf("%s\n", weapon[tank->cw].name);
    else
        printf("%s\n", item[tank->cw - WEAPONS].name);
#endif // DEBUG_AIM

    // Okay, here we go:
#ifdef DEBUG_AIM
    printf("Best: %d .. Attempts %d  ..  RFattempts %d\n", iBestOvershoot, iAttempts, rangeFindAttempts);
#endif
    while (iBestOvershoot && (iAttempts < rangeFindAttempts))
    {
        iAttempts++;

#ifdef DEBUG_AIM
        printf("--> %d. rangeFind:\n", iAttempts);
        printf(" Angle: %3d Power: %4d\n", (iAngle - 90), iPower);
#endif // DEBUG_AIM

        /* --- Step 1: Calculate angle for spreads: --- */
        int iSpreadCount = 0;
        int iOvershoot = MAX_OVERSHOOT;
        int iSpreadOvershoot = 0;
        int iSelfHitMult = 0;
        int iLastOvershoot = iOvershoot;

        while (iSpreadCount < iSpread)
        {
#ifdef DEBUG_AIM
            printf("Inside the wee while loop.\n");
#endif
            iCalcAngle = iAngle;

            // Two cases: Even and odd spreads.
            if (iSpread > 1)
            {
                // odd spreads:
                if (weapon[tank->cw].spread % 2)
                    iCalcAngle += iSpreadOdd[iSpreadCount];
                // even spreads:
                else
                    iCalcAngle += iSpreadEven[iSpreadCount];
            }
            dVelocityX = _global->slope[iCalcAngle][0] * (double) iPower * (100.0 / _global->frames_per_second) / 100.0;
            dVelocityY = _global->slope[iCalcAngle][1] * (double) iPower * (100.0 / _global->frames_per_second) / 100.0;

            dStartX = tank->x + (_global->slope[iCalcAngle][0] * GUNLENGTH);
            dStartY = tank->y + (_global->slope[iCalcAngle][1] * GUNLENGTH);

            /* --- Step 2: Calculate overshoots: --- */
#ifdef DEBUG_AIM
            printf("Trace the shell\n");
#endif
            iActOvershoot = traceShellTrajectory(dStartX, dStartY, dVelocityX, dVelocityY, dHitX, dHitY);
#ifdef DEBUG_AIM
            printf("Back from shell tracing.\n");
#endif
            if (abs(iActOvershoot) < _global->screenWidth)
                iSelfHitMult += adjustOvershoot(iActOvershoot, dHitX, dHitY);
            // Otherwise it's a wall hit and not relevant!

#ifdef DEBUG_AIM
            printf("Passed hit multi\n");
#endif
            // With this method only the best hit of spreads is counted.
            if (abs(iActOvershoot) < abs(iOvershoot))
                iOvershoot = iActOvershoot;

#ifdef DEBUG_AIM
            printf("Passed overshoot. Spread: %d\n", iSpread);
#endif
            // iSpreadOvershoot calculates the average absolute Overshoot
            if (iActOvershoot)
                iSpreadOvershoot += (int) (fabs((double) iActOvershoot) / (double) iSpread);

            iSpreadCount++;

#ifdef DEBUG_AIM
            if (iSpreadCount > 1)
            {
                if (iSpreadCount == 2)
                    printf(" (%3d ", iCalcAngle - 90);
                else
                    printf(",%3d ", iCalcAngle - 90);
                if (iSpread == iSpreadCount)
                    printf("\b)");
            }
#endif // DEBUG_AIM
        }

#ifdef DEBUG_AIM
        printf("Moving right along\n");
#endif

        if (iOvershoot < 0)
            iSpreadOvershoot *= -1;

        if (iSelfHitMult > 0)
        {
            // We hit ourselves, so the larger Overshoot will be multiplied and taken!
            if (abs(iSpreadOvershoot) > abs(iOvershoot))
                iOvershoot = iSpreadOvershoot;
            if (abs(iOvershoot) < _global->screenWidth)
                iOvershoot = _global->screenWidth;
            if (abs(iOvershoot) < MAX_OVERSHOOT)
                iOvershoot*= iSelfHitMult;
        }
        else
        {
            // everything okay, just take the smaller one!
            if (abs(iSpreadOvershoot) < abs(iOvershoot))
                iOvershoot = (int) (((double) iSpreadOvershoot + ((double) iOvershoot * focusRate)) / 2);
            else
                iOvershoot = (int) (((double) iOvershoot + ((double) iSpreadOvershoot * focusRate)) / 2);
        }

        /* --- Step 3.: Record angle, power and overshoot if the overshoot is smaller than the best recorderd so far: --- */
        if (abs(iOvershoot) < abs(iBestOvershoot))
        {
            iBestOvershoot = iOvershoot;
            iBestAngle = iAngle;
            iBestPower = iPower;
        }

#ifdef DEBUG_AIM
        printf(" Overshoot: %6d (%4d x %4d) SH: %1d\n", iOvershoot, (int)dHitX, (int)dHitY, iSelfHitMult);
#endif // DEBUG_AIM

        /* --- Step 4.: Alter angle, or power if the angle can't be manipulated anymore, and start over: --- */
        if (iOvershoot)
        {
            // Preperation: flip iAngle if neccessary
            if (iAngle > 180)
                iAngle = 180 + (180 - iAngle);    // flip

            // Preperation: Decide over angle and power modification depending on overshoot
            if (abs(iOvershoot) < _global->screenWidth)
            {
                dAngleMul = 1.0 + fabs((double) iOvershoot / (double) _global->screenWidth); // between 1.0 and 2.0
                iAngleMod = (int) (fabs((double) iOvershoot) / 10.0);
                if (iAngleMod > 15)
                    iAngleMod = 15;    // need a barrier here, too

                // Power modification is calculated depending on overshoot
                // To raise or lower by 100 pixels, we need aproximately 100 power (at 45°)
                dPowerMul = pow(1.0 + (fabs((double) iAngle - 135.0) / 50.0), 2.0);    // between 1.0 and 3,61
                iPowerModFixed = 5 + (int) ((double) type * (fabs(iOvershoot) / 10.0));    // useless 10%, deadly 50% fix
                iPowerMod = 5 + (int) ((10.0 - (double) type) * (fabs(iOvershoot) / 10.0));    // useless 90%, deadly 50% variable
                iPowerMod = (rand() % iPowerMod) + iPowerModFixed;
            }
            else
            {
                // As the overshoot is too high, probably a wall hit, Modification is done in a more limited way:
                dAngleMul = 1.0 + ((double) type / 10.0);
                dPowerMul = 1.0 + ((double) type / 10.0);
                iAngleMod = (rand() % 11) + 5;
                iPowerMod = (MAX_POWER / 8) + (rand() % (MAX_POWER / 8));
                if ((iOvershoot < 0) || (iAngle >= 170))
                    bIsRaisingMode = false;    // we need to raise distance urgently, so cancel bIsRaisingMod
                else
                    bIsRaisingMode = true;    // we need to lower distance urgently, so enter bIsRaisingMod
            }

            // before entering the step 4 modification parts, we could try a trick:
            if ((abs(iLastOvershoot) < abs(iOvershoot))
                && (((iLastOvershoot < 0) && (iOvershoot > 0))    // be sure that the overshoots switches
                || ((iLastOvershoot > 0) && (iOvershoot < 0)))    // between signed and unsigned
                && ((abs(iLastAngle - iAngle) >= 2)    // There has to be something to do or
                || (abs(iLastPower - iPower) >=10))    // we might waste all remaining attempts
                && (abs(iLastOvershoot) < _global->screenWidth)    // don't try it on wallhits, selfhits
                && (abs(iOvershoot) < _global->screenWidth))
            {
                // the current modification made the shot worse,
                // but switched between too short and too long,
                // so revert to half the modification:
                iAngleMod = (iLastAngle + iAngle) / 2;    // We use the mod to save declaring
                iPowerMod = (iLastPower + iPower) / 2;    // two new vars!
                iLastAngle = iAngle;
                iLastPower = iPower;
                iAngle = iAngleMod;
                iPower = iPowerMod;
                // Note: This trick won't work when both are too short or too long,
                // because then bots would never get over too high obstacles!
            }
            else
            {
                // No trick needed, we are getting nearer!
                iLastAngle = iAngle;
                iLastPower = iPower;
                iAngleMod = (int) ((double) iAngleMod * focusRate * dAngleMul);
                iPowerMod = (int) ((double) iPowerMod * focusRate * dPowerMul);
                // * a) short shots (overshoot < 0) - raise power
                if (iOvershoot < 0)
                {
                    // If we are too short and have (char *)"overbend" in raising mode, it has to be cancelled!
                    if (bIsRaisingMode && (iAngle > 180))
                        bIsRaisingMode = false;

                    if (!bIsRaisingMode)
                    {
                        // * i.: angle lowering mode - adjust the angle towards 45° until it (or aAngle) is reached
                        if (iAngle > iIdealAngle)
                        {
                            iAngle -= iAngleMod;
                            if (iAngle < iIdealAngle)
                                iAngle = iIdealAngle;
                        }
                        if (iAngle < iIdealAngle)
                        {
                            iAngle += iAngleMod;
                            if (iAngle > iIdealAngle)
                                iAngle = iIdealAngle;
                        }

                        // Apply as much power as is neccessary:
                        iPower += iPowerMod - (abs(iLastAngle - iAngle) * 10);

                        // check to see whether Raising mode should be entered:
                        if ((iAngle == iIdealAngle) && (iAngle == iLastAngle))
                            bIsRaisingMode = true;
                    }
                    else
                        // * ii.:power raising mode - after the first positive overshoot is reached, the angle won't be changed any more
                        iPower += (int) ((double) iPowerMod * dAngleMul);
                }
                // * b) long shots (overshoot > 0) - lower power
                if (iOvershoot > 0)
                {
                    if (bIsRaisingMode)
                    {
                        //   *    i.: angle raising mode - adjust the angle towards 89° until it is reached
                        if (iAngle > 178)
                            iAngleMod = 1;  // for small (char *)"overbends"
                        iAngle += iAngleMod;

                        // Apply as much power as is neccessary:
                        iPower -= iPowerMod - (abs(iLastAngle - iAngle) * 10);
                    }
                    else
                        //   *    ii.:power lowering mode - once the angle can't be changed any more, only power is changed
                        iPower -= (int) ((double) iPowerMod * dAngleMul);
                }
            }

            // last step: check iPower and iAngle:
            while ( (iPower >= MAX_POWER) || (iPower <= (MAX_POWER / 20)) )
                iPower = ((MAX_POWER / 2) + iPower) / 2;
            iPower -= iPower % 5;

            // check the angle, it must not be flatter than iAngleBarrirer!
            if (iAngle < iAngleBarrier)
                iAngle = iAngleBarrier;

            // Now flip the angle back if neccessary:
            if (aAngle > 180)
                iAngle = 180 + (180 - iAngle); // flip back!

#ifdef DEBUG_AIM
            printf(" --> AngleMod: %3d PowerMod: %4d\n", (iAngle - iLastAngle), (iPower - iLastPower));
#endif // DEBUG_AIM
        } // end of if(iOvershoot)
    }

#ifdef DEBUG_AIM
    printf("looks like the end of the while loop in aiming\n");
#endif
    // Record the best found values in tank if possible
    if (abs(iBestOvershoot) < tank->smallestOvershoot)
    {
#ifdef DEBUG_AIM
        printf(" New best Overshoot: %5d\n", iBestOvershoot);
#endif // DEBUG_AIM
        tank->smallestOvershoot = abs(iBestOvershoot);
        tank->bestAngle = iBestAngle;
        tank->bestPower = iBestPower;
        // Give the best ones back if possible
        if (iBestAngle != aAngle)
            aAngle = iBestAngle;
        if (iBestPower != aPower)
            aPower = iBestPower;
    }
#ifdef DEBUG_AIM
    else
        cout << " No new best Overshoot..." << endl;
#endif // DEBUG_AIM
    return iBestOvershoot;
}

int PLAYER::adjustOvershoot(int &aOvershoot, double aReachedX, double aReachedY)
{
    TANK *pTankHit = NULL;    // For hitting quality analysis
    long int iOvershoot = aOvershoot;    // To calculate with locally
    bool bIsDirectHit = true;    // false for shaped charges and napalm is chosen
    bool bIsShaped = false;    // true for shaped charges (special radius calculation needed!)
    int iDamage = weapon[tank->cw].damage;
    int iRadius = weapon[tank->cw].radius;
    int iSelfHits = 0;    // Will be raised for every self- and team-hit and then returned

    if (iRadius < 10)
        iRadius = 10;    // several things wouldn't function otherwise
    if (iDamage < 10)
        iDamage = 10;    // if we didn't set minimum values

    if ((tank->cw >= SHAPED_CHARGE) && (tank->cw <= CUTTER))
        bIsShaped = true;
    if ( bIsShaped || ((tank->cw >= SML_NAPALM) && (tank->cw <= LRG_NAPALM)) )
        bIsDirectHit = false;

    /* --- Step 1: See whether a tank is hit: */
    for (int i = 0; i < 10; i++)
    {
        if (_global->players[i] && _global->players[i]->tank)
        {
            if ((aReachedX > (_global->players[i]->tank->x - TANKWIDTH)) && (aReachedX < (_global->players[i]->tank->x + TANKWIDTH))
                 &&(aReachedY > (_global->players[i]->tank->y)) && (aReachedY < (_global->players[i]->tank->y + TANKHEIGHT))
                 && (_global->players[i]->tank->l > 0))
                pTankHit = _global->players[i]->tank;
        }
    }

    /* --- Step 2: See whether the target tank is hit or in weapon radius: --- */
    // check these values in case of segfault
    if (_target && (pTankHit) && (pTankHit == _target) && bIsDirectHit && (pTankHit->player != this))
        // A Direct hit with a direct weapon is what we want, so give 0 back
    {
        iOvershoot = 0;
#ifdef DEBUG_AIM
        cout << "ON TARGET! ";
#endif // DEBUG_AIM
    }
    else
    {
        if (!iOvershoot)
        {
            if ( (((tank->x < _targetX) && (_targetX > aReachedX)) || ((tank->x > _targetX) && (_targetX < aReachedX)))
                 &&(iOvershoot > 0) && (iOvershoot < MAX_OVERSHOOT) )
                iOvershoot *= -1;    // the shot is too short
            else
                iOvershoot = 1;
        }
        if (pTankHit)
        {
            // We *have* hit a tank, let's see to that it isn't us or a friend:
            if ((pTankHit == tank) || (pTankHit->player == this))
            {
                // Ourselves, not good!
                iOvershoot *= iRadius * iDamage;
                iSelfHits++;
            }
            else if (pTankHit->player->team == team)
            {
                // We hit someone of the same team, but only Jedi and SitH care, of course:
                if (team == TEAM_JEDI)
                    iOvershoot *= iRadius * (defensive + 2) * focusRate;
                if (team == TEAM_SITH)
                    iOvershoot *= iRadius * (-1 * (defensive - 2)) * focusRate;
                if (team != TEAM_NEUTRAL)
                    iSelfHits++;
            }
        }

        /* --- Step 3: See, whether we, or a team member, is in blast radius and manipulate Overshoot accordingly --- */
        for (int i = 0; i < 10; i++)
        {
            if (_global->players[i] && _global->players[i]->tank && (_global->players[i]->tank != _target))
            {
                // _target is skipped, so we don't get wrong values when revenging on a team mate!
                int iX = _global->players[i]->tank->x;
                int iY = _global->players[i]->tank->y;
                int iRadiusDist;
                int iBlastDist = ABSDISTANCE(aReachedX, aReachedY, iX, iY);

                iRadiusDist = iRadius - iBlastDist;
                if (!iRadiusDist)
                    iRadiusDist = 1;

                if (iBlastDist < iRadius)
                {
                    // Is in Blast range. (maybe)
                    if ( !bIsShaped || (bIsShaped && (abs(iY - (int)aReachedY) <= (iRadius / 20))) )
                    {
                        // Either no shaped charge or in radius
                        if (_global->players[i]->tank == tank)
                        {
                            // Ourselves, not good!
                            iOvershoot *= iRadiusDist * iDamage;
                            iSelfHits++;
                        }
                        else if (_global->players[i]->team == team)
                        {
                            // We hit someone of the same team, but only Jedi and SitH care, of course:
                            if (team == TEAM_JEDI)
                                iOvershoot *= iRadiusDist * (defensive + 2) * focusRate;
                            if (team == TEAM_SITH)
                                iOvershoot *= iRadiusDist * (-1 * (defensive - 2)) * focusRate;
                            if (team != TEAM_NEUTRAL)
                                iSelfHits++;
                        }
                    }
                }
            }
        }

        // Be sure to not give a ridiculously high overshoot back:
        while (abs(iOvershoot) >= MAX_OVERSHOOT)
            iOvershoot /= 2;
    }
    aOvershoot = (int) iOvershoot;

    return iSelfHits;
}

// If Napalm or a shaped weapon is chosen, the target has to be modified!
int PLAYER::getAdjustedTargetX(TANK * aTarget)
{
    int iTargetX, iTargetY;
    int iMinOffset = 0;
    int iMaxOffset = 1;

    if (aTarget)
    {
        iTargetX = aTarget->x;
        iTargetY = aTarget->y;
    }
    else if (_target)
    {
        iTargetX = _target->x;
        iTargetY = _target->y;
    }
    else    // avoid segfault
        return (rand() % _global->screenWidth);


    // tank is dead, bail out to avoid segfault
    if (!tank)
        return iTargetX;

    if ((tank->cw >= SHAPED_CHARGE) && (tank->cw <= CUTTER))
    {
        int iBestLeftOffset = 0;
        int iBestRightOffset = 0;
        int iLeftY = 0;
        int iBestLeftY = 0;
        int iRightY = 0;
        int iBestRightY = 0;
        iMinOffset = (int) ((TANKWIDTH / 2) + ((double) TANKWIDTH * focusRate));
        iMaxOffset = iMinOffset + (TANKWIDTH * (((int) type + 1) / 2));

        for (int i = iMinOffset; i < iMaxOffset; i++)
        {
            // Get Y-Data:
            if ((iTargetX - i) > 1)
                iLeftY = _env->surface[iTargetX - i];
            if ((iTargetX + i) < (_global->screenWidth - 1))
                iRightY= _env->surface[iTargetX + i];
            // Check whether new Y-Data is better than what we have:
            if (abs(iLeftY - iTargetY) < abs(iBestLeftY - iTargetY))
            {
                iBestLeftOffset = i;
                iBestLeftY = iLeftY;
            }
            if (abs(iRightY - iTargetY) < abs(iBestRightY - iTargetY))
            {
                iBestRightOffset = i;
                iBestRightY = iRightY;
            }
        }
        // Now see whether we go left or right:
        if (abs(iBestLeftY - iTargetY) < abs(iBestRightY - iTargetY))
            // use left:
            iTargetX -= iBestLeftOffset;
        else
            // use right:
            iTargetX += iBestRightOffset;
    }

    if ((tank->cw >= SML_NAPALM) && (tank->cw <= LRG_NAPALM))
    {
        // here we only check one side, the one to the wind:
        iMaxOffset = abs((int) ((double) iMaxOffset * _env->wind * focusRate));
        iMinOffset = abs((int) (_env->wind * (double) TANKWIDTH * focusRate));
        int iDirection = 0;

        if (_env->wind < 0)
            iDirection = 1;    // for some reason I do not know, wind is
        if (_env->wind > 0)
            iDirection = -1;    // used (char *)"the wrong way". (???)

        // Don't stretch the offset onto ourselves:
        if ( ((tank->x < iTargetX) && ((iTargetX - tank->x - (iDirection * iMaxOffset)) < (TANKWIDTH * 2)))
             ||((tank->x > iTargetX) && ((tank->x - iTargetX - (iDirection * iMaxOffset)) < (TANKWIDTH * 2))) )
            iMaxOffset = abs(iTargetX - (int) tank->x) - (TANKWIDTH * 2);

        // And don't allow a negative offset either (would be useless due to wind!)
        if (iMaxOffset < TANKWIDTH)
            iMaxOffset = TANKWIDTH;

        // But be sure iMinOffset is smaller than iMaxOffset:
        if (iMinOffset >= iMaxOffset)
            iMinOffset = iMaxOffset - (int) type;

        if (iDirection)
        {
            int iBestOffset = 0;

            // Without wind there is nothing to do!
            for (int i = iMinOffset; i < iMaxOffset; i++)
            {
                int iSurfY = 0;
                int iBestY = 0;
                int iOffset = i * iDirection;

                // Get Y-Data:
                if ( ((iTargetX + iOffset) > 1) && (((iTargetX + iOffset) < (_global->screenWidth - 1))) )
                    iSurfY = _env->surface[iTargetX + iOffset];

                // Check whether new Y-Data is better than what we have:
                if ( ((iTargetY - iSurfY) < (iTargetY - iBestY)) || !iBestY )
                {
                    iBestOffset = iOffset;
                    iBestY = iSurfY;
                }
            }
            iTargetX += iBestOffset;
        }
    }

    return iTargetX;
}

int PLAYER::calculateAttack(TANK *aTarget)
{
    /* There are two general possibilities:
   * - aTarget provided:
   *   This function was called from computerSelectTarget() and will only check if it is possible
   *   to reach a target, aka try only once and give the overshoot back.
   * - default (aTarget is automatically NULL)
   *   This function was called from computerControl() and will go forth and try to hit _target
   *
   * Outline:
   * --------
   * There are the following possibilities:
   * a) An Item or a laser is chosen
   *    -> a direct angle will do!
   * b) We are burried
   *    -> fire unburying tool at +/-60° to the middle of the screen
   * c) Kamikaze
   *    -> indicated by targetX/Y being tank.x/y
   *    -> if shaped weapon is chosen, fire 45° and power 100 to the side where y is nearest to tank.y
   *    -> if napalm is chosen, fire 90° and power 0
   *    -> otherwise fire 90° and power 250
   * d) Fire in non-boxed mode
   *    -> normal calculation
   * e) Fire in non-boxed mode
   *    -> extended power-control after normal calculation
   *    -> if the target can't be reached while staying below the ceiling,
   *       check for an obstacle than can be removed and do so if found.
   * --> boxed mode is included in non-boxed mode now. I hope it works as I intent!
   *
   * Update for dynamization: The previous target is recorded, aiming starts at the old values,
   *                          if the target didn't change. */

    int iXdistance, iYdistance;
    if (aTarget)
    {
        iXdistance = aTarget->x - tank->x;
        iYdistance = aTarget->y - tank->y;
    }
    else
    {
        iXdistance = _targetX - tank->x;
        iYdistance = _targetY - tank->y;
    }

#ifdef DEBUG_AIM
    if (!aTarget && !iTargettingRound)
    {
        printf("\n-----------------------------------------------\n");
        if (_target)
            printf("%s is starting to aim at %s\n", getName(), _target->player->getName() );
        else
            printf("%s is aiming without a target!\n", getName());
    }
#endif // DEBUG_AIM

    /* --- case a) An Item is chosen --- */
    if ( !aTarget && ((tank->cw >= WEAPONS) || ((tank->cw >= SML_LAZER) && (tank->cw <= LRG_LAZER))) )
    {
#ifdef DEBUG_AIM
        printf("About to calc direct angle.\n");
#endif
        _targetAngle = calculateDirectAngle(iXdistance, iYdistance);
        _targetPower = MAX_POWER / 2;

#ifdef DEBUG_AIM
        cout << "Direct " << _targetAngle - 90 << " for ";
        printf("Direct %d for %s\n", _targetAngle -90, (tank->cw < WEAPONS) ? weapon[tank->cw].name : item[tank->cw - WEAPONS].name);
#endif // DEBUG_AIM

        iTargettingRound = retargetAttempts; // So it is done!

        return 0;
    }

    /* --- case b) We are burried --- */
    if (!aTarget && (tank->howBuried() > BURIED_LEVEL))
    {
        // Angle is 60° to the middle of the screen:
        int iAngleVariation = rand() % (20 - ((int) type * 3));
        iAngleVariation -= (int) ((double) type / 1.5);
        if (tank->x <= _global->halfWidth)
            _targetAngle = 150 + iAngleVariation;
        else
            _targetAngle = 210 - iAngleVariation;
#ifdef DEBUG_AIM
        printf("Freeing self with Angle %d and Power %d\n", _targetAngle, _targetPower);
#endif // DEBUG_AIM

        iTargettingRound = retargetAttempts;

        return 0;
    }

    /* --- case c) Kamikaze --- */
    if (!aTarget && (_targetX == tank->x) && (_targetY == tank->y))
    {
#ifdef DEBUG_AIM
        cout << "Going bye bye with ";
        if (tank->cw < WEAPONS)
            cout << weapon[tank->cw].name;
        else
            cout << item[tank->cw - WEAPONS].name;
        cout << " !!!" << endl;
        cout << "GERONNNNNIIIIIMOOOOOOOOOO !!!" << endl;
#endif // DEBUG_AIM

        iTargettingRound = retargetAttempts;

        // For a nice bye bye we set angle/power directly
        if ((tank->cw >= SHAPED_CHARGE) && (tank->cw <= CUTTER))
        {
            // shaped weapons are a bit special:
            int iHLeft, iHRight;
            int iCount = 0;
            for (int i = TANKWIDTH; i <= (TANKWIDTH * 2); i++)
            {
                iCount++;
                iHLeft = _env->surface[(int) (tank->x - i)];
                iHRight= _env->surface[(int) (tank->x + i)];
            }
            iHRight /= iCount;
            iHLeft  /= iCount;
            if (fabs(iHRight - tank->y) < fabs(iHLeft - tank->y))
                _targetAngle = 135;
            else
                _targetAngle = 225;
            _targetPower = MAX_POWER / 20;
            return 0;
        }
        // The other possibilities are easier:
        if ((tank->cw >= SML_NAPALM) && (tank->cw <= LRG_NAPALM))
            _targetPower = 0;
        else
            _targetPower = MAX_POWER / 8;
        _targetAngle = 180;

        return 0;
    }

    /* --- case d) Fire in non-boxed mode --- */
    int iRawAngle, iAdjAngle, iPower, iSavetyPower;
    int iOvershoot = MAX_OVERSHOOT;
    int iLastOvershoot = MAX_OVERSHOOT;
    int iBestOvershoot = MAX_OVERSHOOT;

    _targetX = getAdjustedTargetX(aTarget);
    calculateAttackValues(iXdistance, iYdistance, iRawAngle, iAdjAngle, iPower, false);

    if (!iTargettingRound)
    {
        // Initializiation, if this is the first try:

        if (!aTarget && (_oldTarget == _target))
        {
            // Target didn't change, use last rounds values:
            tank->bestAngle = tank->a;
            tank->bestPower = tank->p;
            iRawAngle = tank->a;
            iAdjAngle = tank->a;
            iPower = tank->p;
        }
        else
        {
            // new target, new values:
            tank->bestAngle = 180;
            tank->bestPower = MAX_POWER;
            if (!aTarget)
                _oldTarget = _target;
        }

        tank->smallestOvershoot = MAX_OVERSHOOT + 1; // So there will be at least one recorded
    }
    else
    {
        // This is a follow-up round, get the last values back:
        iAdjAngle = _targetAngle;
        iPower = _targetPower;
        iBestOvershoot = tank->smallestOvershoot;
    }

    if (aTarget)
    {
#ifdef DEBUG_AIM
        printf("Returning a range find.\n");
#endif
        return (rangeFind(iAdjAngle, iPower));
    }

    // Now that we are here, normal handling is needed:
#ifdef DEBUG_AIM
    if (!iTargettingRound)
    {
        printf("Intitial try:\n");
        printf("About to range find.\n");
    }
#endif // DEBUG_AIM

    iOvershoot = rangeFind(iAdjAngle, iPower);
    iLastOvershoot = iBestOvershoot;
    if (abs(iOvershoot) < abs(iBestOvershoot))
        iBestOvershoot = iOvershoot;

#ifdef DEBUG_AIM
    cout << "Overshoot: " << iOvershoot << " (best so far: " << iBestOvershoot << ")" << endl << endl;
    cout << iTargettingRound + 1 << ". re-try:";
#endif // DEBUG_AIM

    // There is still an Overshoot, so see what we can do:
    if (abs(iLastOvershoot - iOvershoot) <= 1)
    {
        // rangeFind can't get a better version, so start completely different!
        int iAngleRange = 180;
        if (_env->current_wallType == WALL_STEEL)
            iAngleRange /= 2;
        if (_targetX > tank->x)
            // shoot basically to the right:
            iRawAngle = 90 + (rand() % iAngleRange);
        else
            // shoot basically to the left:
            iRawAngle = 270 - (rand() % iAngleRange);
        iAdjAngle = (iAdjAngle + iRawAngle) / 2;
        iPower = (iPower + (rand() % (MAX_POWER / 2))) / 2;
#ifdef DEBUG_AIM
        cout << " Starting over! " << endl;
#endif
    }
    else
    {
        // Just calculate anew:
        calculateAttackValues(iXdistance, iYdistance, iRawAngle, iAdjAngle, iPower, true);
#ifdef DEBUG_AIM
        cout << " recalculating! " << endl;
#endif
    }

    /* SavetyPower is as follows:
   * abs(iAdjAngle -180) changes the value to be between:
   * ->  0 : Straight upwards       (originally 180)
   * -> 90 : Straight Left or right (originally 270/90)
   * 91 - abs(...) gives a value between 1 and 91 which is multiplied with the radius */
    iSavetyPower = ((91 - abs(iAdjAngle - 180)) * (weapon[tank->cw].radius * focusRate)) / focusRate;
    if (iSavetyPower > (MAX_POWER / 4))
        iSavetyPower = MAX_POWER / 4;
    if (iPower < iSavetyPower)
        iPower = iSavetyPower;

    iLastOvershoot = iOvershoot;
    iOvershoot = rangeFind(iAdjAngle, iPower);

    if (abs(iOvershoot) < abs(iBestOvershoot))
        iBestOvershoot = iOvershoot;

    iTargettingRound++;

    // Do not count this attempt if the currently best overshoot is too high:
    if ((iBestOvershoot > _global->screenWidth) && (rand() % ((int) type * 2)))
        iTargettingRound--;    // if ibestOvershoot is negative it will alwys be counted!

    if (tank->smallestOvershoot < (weapon[tank->cw].radius / (int) type))
        iTargettingRound = retargetAttempts;

    // Now the best Version has to be tested:
    iOvershoot = tank->smallestOvershoot;    // Always a positive number!
    _targetAngle = tank->bestAngle;    // Record for foolow-up rounds
    _targetPower = tank->bestPower;    // Record for follow-up rounds

    if (iTargettingRound == retargetAttempts)
    {
#ifdef DEBUG_AIM
        cout << "  ---  ---" << endl;
        cout << "Final Decision:" << endl;
        cout << "Angle    : " << _targetAngle - 90 << endl;
        cout << "Power    : " << _targetPower << endl;
        cout << "Overshoot: " << iBestOvershoot << endl;
#endif // DEBUG_AIM

        if (iOvershoot > weapon[tank->cw].radius)
        {
            if (_targetAngle > 180)
                iRawAngle = _targetAngle + 10 + (rand() % ((int) type * 4));
            else
                iRawAngle = _targetAngle - (10 + (rand() % ((int) type * 4)));

            // There are three possibilities:
            if ( (iBestOvershoot < 0) && (abs(_targetAngle - 180) <= (10 + ((iXdistance / _global->screenWidth) * 10)))
                 &&(!tank->shootClearance(iRawAngle, weapon[tank->cw].radius * 1.5)) )
            {
                // a) there is an obstacle in our way!
                tank->cw = getUnburyingTool();
                if (tank->cw < WEAPONS)
                {
                    _targetAngle = iRawAngle;
#ifdef DEBUG_AIM
                    cout << "Revised Angle for unburying: " << _targetAngle - 90 << endl;
#endif // DEBUG_AIM
                }
            }
            else
            {
                // b) the way is clear, so either try to switch weapon or fire away!
                if (iBestOvershoot < 0)
                {
                    // As we are too short, see whether another weapon might be good
                    if (nm[TREMOR])
                        tank->cw = TREMOR;
                    if (nm[SHOCKWAVE])
                        tank->cw = SHOCKWAVE;
                    if ((_targetY < (_global->screenHeight - iOvershoot)) && nm[BURROWER])
                        tank->cw = BURROWER;
                    if (nm[TECTONIC])
                        tank->cw = TECTONIC;
                    if ((_targetY < (_global->screenHeight - iOvershoot)) && nm[PENETRATOR])
                        tank->cw = PENETRATOR;

                    // if it is boxed, it is better to teleport out!
                    if ((_global->bIsBoxed) && (iOvershoot > (weapon[tank->cw].radius * 2)))
                    {
                        // no way, teleport out!
                        if (ni[ITEM_TELEPORT])
                            tank->cw = ITEM_TELEPORT + WEAPONS;
                        if (ni[ITEM_SWAPPER])
                            tank->cw = ITEM_SWAPPER + WEAPONS;
                    }
                }
                else
                {
                    // look for tectonis and their range
                    if (nm[TREMOR] && (weapon[TREMOR].radius >= iOvershoot))
                        tank->cw = TREMOR;
                    if (nm[SHOCKWAVE] && (weapon[SHOCKWAVE].radius >= iOvershoot))
                        tank->cw = SHOCKWAVE;
                    if (nm[TECTONIC] && (weapon[TECTONIC].radius >= iOvershoot))
                        tank->cw = TECTONIC;
                }

                // c) we have no chance to reach anything!
                if (iOvershoot > _global->screenWidth)
                {
                    // no way, teleport out!
                    if (ni[ITEM_TELEPORT])
                        tank->cw = ITEM_TELEPORT + WEAPONS;
                    if (ni[ITEM_SWAPPER])
                        tank->cw = ITEM_SWAPPER + WEAPONS;
                }

            }
        }
        else
        {
            // If we *are* hitting ok, Angle and Power need to be manipulated by errorMultiplier
            int iAngleMod, iPowerMod;
            int iAngleModLimit = (42 - (10 * ((int) type - 1)));    // =  42,  32,  22, 12,  2 for useless -> deadly
            int iPowerModLimit = (210 - (50 * ((int) type - 1)));    // = 210, 160, 110, 60, 10 for useless -> deadly

            iAngleMod = (rand() % 51) * errorMultiplier;    // useless: 0 -  100, deadly: 0 - 2 (with 2 being very unlikely)
            if (iAngleMod > iAngleModLimit)
                iAngleMod = iAngleModLimit;

            iPowerMod = (rand() % 251) * errorMultiplier;    // useless: 0 - 500, deadly: 0 - 10 (with 10 being *very* unlikely)
            if (iPowerMod > iPowerModLimit)
                iPowerMod = iPowerModLimit;

            // In boxed mode, errors have quite more impact and are therefore cut down to be only two thirds
            if (_global->bIsBoxed)
            {
                iAngleMod = (int) ((double) iAngleMod / 3.0 * 2.0);
                iPowerMod = (int) ((double) iPowerMod / 3.0 * 2.0);
            }

            // 25 % to get a flattening anglemod (aka nearing to the ground)
            if ((!(rand() % 4)) && (_targetAngle < 180))
                iAngleMod *= -1;    // right side angle
            if ((rand() % 4) && (_targetAngle > 180))
                iAngleMod *= -1;    // right side angle (other way round, because here a positive mod is flattening!

            // 25 % to get a lessening powermod. (it is more likely to overshoot than be too short!)
            if (!(rand() % 4))    // (and it leads to too many stupid self
                iPowerMod *= -1;    // hits to allow negative PoerMod too often

            // The modification must not lead to a too stupid angle
            while ( iAngleMod
                    && (((_targetAngle < 180) && (((iAngleMod + _targetAngle) < 95) || ((iAngleMod + _targetAngle) > 175)))
                    || ((_targetAngle >=180) && (((iAngleMod + _targetAngle) > 265) || ((iAngleMod + _targetAngle) < 185)))) )
                iAngleMod /= 2;
            _targetAngle += iAngleMod;

            // Same applies for Power:
            while ( iPowerMod && (((iPowerMod + _targetPower) > MAX_POWER) || ((iPowerMod + _targetPower) < (MAX_POWER * 0.1))) )
                iPowerMod /= 2;

            // iPowerMod needs to be a multiplier of 5:
            iPowerMod -= iPowerMod % 5;

            _targetPower += iPowerMod;

#ifdef DEBUG_AIM
            printf( "Error-Adjusting: Angle %3d Power %4d\n", iAngleMod, iPowerMod);
#endif // DEBUG_AIM
        }

#ifdef DEBUG_AIM
        cout << "HITTING " << _target->player->getName() << " ???" << endl << endl;
#endif // DEBUG_AIM

        // Last one: if we are revenging, tell em!
        // Added _target check to avoid segfault
        if  ( (tank->cw < WEAPONS) && (_target)    // Weapon selected?
              && (weapon[tank->cw].damage > 1)    // One that delivers damage?
              && (revenge == _target->player)    // And the target is our revengee?
              && ((rand() % ((int) DEADLY_PLAYER + 2 - (int) type))) )    // And we do really want to taunt?
        {
            FLOATTEXT *DIEText;
            char *my_text;
            my_text = selectRetaliationPhrase();
            DIEText = new FLOATTEXT(_global, _env, my_text, (int) tank->x, (int) tank->y - 30, color, CENTRE);
            if (my_text)
                free(my_text);
            if (DIEText)
            {
                //DIEText->xv = 0;
                //DIEText->yv = -0.4;
                DIEText->set_speed(0.0, -0.4);
                DIEText->maxAge = 150;
            }
            else
                perror("player.cpp: Failed allocating memory for DieText in calculateAttack().");
        }
        // As we might finish here because of a good overshoot, iTargettingRounds need to be maxed!
        iTargettingRound = retargetAttempts;
    }
    return 0;
}

void PLAYER::calculateAttackValues(int &aDistanceX, int aDistanceY, int &aRawAngle, int &aAdjAngle, int &aPower, bool aAllowFlip)
{
    double dAirTime, dxTime;
    double dSlopeX, dSlopeY;
    double dAngleVariation, dPowerVariation;
    bool bIsWrapped = false;    // Special handling for wrapped walls

    aDistanceX = _targetX - tank->x;

    /* --- Step 1: find the raw angle, aka the obstacle-free optimal angle --- */

    aRawAngle = (int) (atan2((double) aDistanceX, (double) (aDistanceY - abs (aDistanceX))) / PI * 180.0);

    // Bring in Range:
    if (aRawAngle < 0)
        aRawAngle = aRawAngle + 360;
    if (aRawAngle < 90)
        aRawAngle = 135;
    if (aRawAngle > 270)
        aRawAngle = 225;

    if ((_env->current_wallType == WALL_WRAP) && ((rand() % ((int) type + 1)) || !aAllowFlip))
    {
        // Note: We always wrap when possible and flipping not allowed, to have the shorter distance for sure!
        int iWrapDistance = 0;

        // if the distance through the wall is shorter, take it!
        if (tank->x < _targetX)
            iWrapDistance = -1 * (tank->x -1 + _global->screenWidth - 2 - _targetX);
        else
            iWrapDistance = tank->x -1 + _global->screenWidth - 2 - _targetX;

        if (abs(iWrapDistance) < abs(aDistanceX))
        {
            bIsWrapped = true;
            aDistanceX = iWrapDistance;
            aRawAngle = 180 + (180 - aRawAngle);    // flip!
        }
    }

    // Angle variation for more flavour:
    dAngleVariation = (rand() % 21) * focusRate; // useless: 0-4, deadly: 0-20
    if (rand() % 2)
        dAngleVariation *= -1.0;
    while ( (dAngleVariation > 0.0) && (((aRawAngle > 180) && (((aRawAngle + dAngleVariation) < 190)
             || ((aRawAngle + dAngleVariation) > 260))) || ((aRawAngle < 180) && (((aRawAngle + dAngleVariation) > 170)
             || ((aRawAngle + dAngleVariation) < 100)))) )
        dAngleVariation /= 2.0;
    aRawAngle += (int) dAngleVariation;

    // Maybe we could switch sides?
    if ((_env->current_wallType != WALL_STEEL) && (rand() % 2) && (rand() % (int)type) && aAllowFlip)
    {
        // Yes, we can! ( (char *)"Change is coming!" ;-) (b.h.o) )
        aRawAngle = 180 + (180 - aRawAngle); // This switches sides, yes. :-D

        if (tank->x < _targetX)
        {
            // We switch angle from right to left: (original distance is positive)
            switch (_env->current_wallType)
            {
            case WALL_RUBBER:
                // Add Distances to the wall to bounce from:
                aDistanceX += 2 * (tank->x - 1);
                break;
            case WALL_SPRING:
                // Spring walls add velocity, so adapt a bit
                aDistanceX += tank->x - 1 + ((tank->x - 1) * 0.75);
                break;
            case WALL_WRAP:
                // Distance is new: only distances from the wall:
                if (bIsWrapped)
                    aDistanceX = tank->x -1 + _global->screenWidth - 2 - _targetX;
                else
                    aDistanceX = -1 * (tank->x -1 + _global->screenWidth - 2 - _targetX);
                break;
            }
        }
        else
        {
            // We switch angle from left to right: (original distance is negative)
            switch (_env->current_wallType)
            {
            case WALL_RUBBER:
                // Add Distances to the wall to bounce from:
                aDistanceX += 2 * (_global->screenWidth - tank->x - 2);
                break;
            case WALL_SPRING:
                // Spring walls add velocity, so adapt a bit
                aDistanceX += _global->screenWidth - tank->x - 2 + ((_global->screenWidth - tank->x - 2) * 0.75);
                break;
            case WALL_WRAP:
                // Distance is new: only distances from the wall:
                if (bIsWrapped)
                    aDistanceX = -1 * (tank->x -1 + _global->screenWidth - 2 - _targetX);
                else
                    aDistanceX = tank->x -1 + _global->screenWidth - 2 - _targetX;
                break;
            }
        }
    }

    /* --- Step 2: Adjust the Angle given clearance --- */

    aAdjAngle = aRawAngle;
    while ((aAdjAngle < 180) && !(tank->shootClearance(aAdjAngle)))
        aAdjAngle++;
    while ((aAdjAngle > 180) && !(tank->shootClearance(aAdjAngle)))
        aAdjAngle--;

    /* --- Step 3: Find neccessary Power --- */
    dSlopeX = _global->slope[aAdjAngle][0];
    dSlopeY = _global->slope[aAdjAngle][1];

    if (dSlopeX != 0.0)
        dxTime = ((double) aDistanceX / fabs(dSlopeX));
    else
        // entirely down to the elements now
        dxTime = ((double) aDistanceX / 0.000001);

    // Low target, less power
    // xdistance proportional to sqrt(dy)
    dAirTime = fabs(dxTime) + (((double) aDistanceY * dSlopeY) * _env->gravity * (100.0 / _global->frames_per_second)) * 2.0;

    // Less airTime doesn't necessarily mean less power
    // Horizontal firing means more power needed even though
    //   airTime is minimised

    aPower = (int) (sqrt(dAirTime * _env->gravity * (100.0 / _global->frames_per_second))) * 100;

    // Power variation for more flavour:
    dPowerVariation = (rand() % 51) * focusRate;    // useless: 0-10, deadly: 0-50
    dPowerVariation -= (int) dPowerVariation % 5;
    if (rand() % 2)
        dPowerVariation *= -1.0;
    aPower += dPowerVariation;

    if (aPower > MAX_POWER)
        aPower = MAX_POWER;
    if (aPower < (MAX_POWER / 20))
        aPower = MAX_POWER / 20;

}

int PLAYER::calculateDirectAngle(int dx, int dy)
{
    double angle;

    angle = atan2((double) dx, (double) dy) / PI * 180;
    angle += (rand() % 40 - 20) * errorMultiplier;

    if (angle < 0)
        angle = angle + 360;
    if (angle < 90)
        angle = 90;
    else if (angle > 270)
        angle = 270;

    return ((int) angle);
}

TANK * PLAYER::computerSelectTarget(int aPreferredWeapon, bool aRotationMode)
{
    int random_target;
    int attempts = 0;
    int max_attempts = (int)type * 3;
    TANK *best_target = NULL;
    int current_score = 0;
    int best_score = -1 * MAX_OVERSHOOT;    // start with a loooooow score, so that even score<0 tanks can become best target
    TANK *current_tank = NULL;
    TANK *tankPool[10];
    int iMoneyNeed = getMoneyToSave() - money;    // Are we in need for money?
    int target_count = 0;

#ifdef DEBUG
    cout << " -> I need " << iMoneyNeed << " Credits *urgently*!" << endl;
    if (aPreferredWeapon < WEAPONS)
        cout << " -  Searching target for " << weapon[aPreferredWeapon].name << endl;
    else
        cout << " -  Searching target for " << item[aPreferredWeapon - WEAPONS].name << endl;
#endif // DEBUG
    // find out how many tries we have to find a good target

    // Fill tankPool
    for (int i = 0; i < _global->numPlayers; i++)
    {
        if ((_global->players[i]) && (_global->players[i]->tank))
        {
            tankPool[i] = _global->players[i]->tank;
            target_count++;
        }
        else
            tankPool[i] = NULL;
    }

    if (target_count < 2)    // just us left or nobody
        return NULL;

    // who do we want to shoot at?
    while (attempts < max_attempts)
    {
        // select random tank for target
        if (_global->numPlayers > 0)
            random_target = rand() % _global->numPlayers;
        else
            random_target = 0;
        current_tank = tankPool[random_target];

        if (current_tank)
        {
            current_score = 0;
            // only consider living tanks that are not us

            if ((current_tank->l > 0) && (current_tank->player != this))
            {
                int iDamage = 0;

                if (weapon[aPreferredWeapon].numSubmunitions > 1)
                    iDamage = (damageMultiplier * weapon[weapon[aPreferredWeapon].submunition].damage
                              * (weapon[aPreferredWeapon].numSubmunitions / 3.0));
                else
                    iDamage = (damageMultiplier * weapon[aPreferredWeapon].damage * weapon[aPreferredWeapon].spread);

                // compare the targets strength to ours
                int iDiffStrength = ((tank->l + tank->sh) - (current_tank->l + current_tank->sh));

                current_score = iDiffStrength;

                if (iDiffStrength < 0)
                {
                    // The target is stronger. Are we impressed?
                    if ( (defensive < 0.0) && ( (rand() % ((int) type + 1))) )
                        // No we aren't, add defensive-modified Strength
                        // (The more offensive, the less impressed we are)
                        current_score += (int) (((defensive - 3.0) / 2.0) * (double) iDiffStrength);
                }
                else
                    // the target is weaker, add points modified by how defensive we are
                    current_score += (int) ((double) iDiffStrength * ((defensive + 3.0) / 2.0));
#ifdef DEBUG
                cout << " (str)" << current_score;
#endif
                // check to see if we are on the same team
                switch ((int) team)
                {
                case TEAM_JEDI:
                    if ((current_tank->player->team == TEAM_JEDI) && !aRotationMode)
                        current_score -= 500 * (int) type;
                    if ((current_tank->player->team == TEAM_JEDI) && aRotationMode)
                        current_score -= MAX_OVERSHOOT; // no team consideration in rotation mode!

                    if (current_tank->player->team == TEAM_SITH)
                        current_score += -200.0 * (defensive - 2.0) * ((double) type / 2.0);
                    break;
                case TEAM_SITH:
                    if ((current_tank->player->team == TEAM_SITH) && !aRotationMode)
                        current_score -= 500 * (int) type;
                    if ((current_tank->player->team == TEAM_SITH) && aRotationMode)
                        current_score -= MAX_OVERSHOOT;    // no team consideration in rotation mode!
                    if (current_tank->player->team == TEAM_JEDI)
                        current_score += -200.0 * (defensive - 2.0) * ((double) type / 2.0);
                    break;
                    /*
                default:
                  // Neutrals go rather for sith than jedi. (but not much)
                  if (current_tank->player->team == TEAM_JEDI)
                    current_score += -25.0 * (defensive - 2.0) * ( (double) type / 2.0);

                  if (current_tank->player->team == TEAM_SITH)
                    current_score += -50.0 * (defensive - 2.0) * ( (double) type / 2.0);
*/
                }
#ifdef DEBUG
                cout << " (team)" << current_score;
#endif // DEBUG
                // do we have a grudge against the target
                if (current_tank->player == revenge)
                {
                    /*
                  switch ( (int) team)
                    {
                    case TEAM_JEDI:
                      // Revenge is a dark force!
                      current_score += (50 * ( (defensive - 1.5) * -1.0));
                      break;
                    case TEAM_SITH:
                      // Revenge means power!
                      current_score += (200 * ( (defensive - 1.5) * -1.0));
                      break;
                    default:
                   */
                    current_score += (100 * ( (defensive - 1.5) * -1.0));
                    //   }
                }
#ifdef DEBUG
                cout << " (rev)" << current_score;
#endif // DEBUG
                // prefer targets further away when violent death is on
                if (_global->violent_death)
                {
                    int distance;
                    distance = (int) fabs(tank->x - current_tank->x);

                    if (distance > _global->halfWidth)
                        current_score += 100.0 * ((defensive + 3.0) / 2.0);
                }
#ifdef DEBUG
                cout << " (dis)" << current_score;
#endif // DEBUG
                // Add some points if the target is more intelligent than we are (get'em DOWN!)
                // or substract if we are the better one. (Deal with the nutter later...)
                if ((current_tank->player->type != HUMAN_PLAYER) && (current_tank->player->type < DEADLY_PLAYER))
                    current_score += 50 * ((int) current_tank->player->type - (int) type);
                else
                    current_score += 50 * ((int) DEADLY_PLAYER - (int) type);
                // Players, last player type and part time bots are counted as deadly bots.
#ifdef DEBUG
                cout << " (typ)" << current_score;
#endif // DEBUG
                // Add points for score difference if they have more than us
                // useless bot: 1 * diff * 60 --> 4 points difference would mean +240 score
                // deadly bot : 3 * diff * 60 --> 4 points difference would mean +720 score
                if (current_tank->player->score > score)
                    current_score += ((int) type + 1) / 2 * (current_tank->player->score - score) * 60;

#ifdef DEBUG
                cout << " (scr)" << current_score;
#endif // DEBUG
                if (aPreferredWeapon < WEAPONS)
                {
                    // As we are wanting to fire a weapon, add points, if the damage is greater than the targets health
                    // (if we are in need for money, but not on the same team)
                    int iDamageDiff = iDamage - (current_tank->l + current_tank->sh);

                    if ( (iDamageDiff > 0) && (iMoneyNeed > 0) && ((team == TEAM_NEUTRAL)
                         || ((team != TEAM_NEUTRAL) && (current_tank->player->team != team))) )
                        current_score += (double) iDamageDiff * (1.0 + ( (defensive + (double) type) / 10.0));
#ifdef DEBUG
                    cout << " (dmg)" << current_score;
#endif // DEBUG
                    // Check whether the target is buried, and substract points for non-burrower/-penetrator
                    int iBurylevel = current_tank->howBuried();

                    if (iBurylevel > BURIED_LEVEL)
                    {
                        // The target is burried!
                        if ( ((aPreferredWeapon < BURROWER) || (aPreferredWeapon > PENETRATOR))
                              && ((aPreferredWeapon < TREMOR) || (aPreferredWeapon > TECTONIC)) )
                        {
                            // Napalm and shaped charges are absolutely useless
                            if ( ((aPreferredWeapon >= SML_NAPALM) && (aPreferredWeapon <= LRG_NAPALM))
                                 || ((aPreferredWeapon >= SHAPED_CHARGE) && (aPreferredWeapon <= CUTTER)) )
                                current_score -= (int) type * 500;    // Even the useless bot isn't *that* stupid
                            else
                            {
                                // For all other weapons we go for the radius of the blast
                                if (iBurylevel < weapon[aPreferredWeapon].radius)
                                    current_score *= 1.0 - (((double) iBurylevel / (double) weapon[aPreferredWeapon].radius) / 2.0);
                                else
                                    current_score -= (double) iDamage * (double) type * ((double) defensive + 3.0);
                            }
                        }
                        else
                            // As we *want* to fire an appropriate weapon, the target looks rather nice to us!
                            current_score += ((double) (iBurylevel - BURIED_LEVEL) / (((double) type + 1.0) / 2.0)) * (double) iDamage;
#ifdef DEBUG
                        cout << " (bur)" << current_score;
#endif // DEBUG
                    }

                    // Finally, for weapons, see, if we can do good blast damage!
                    if (type >= RANGEFINDER_PLAYER)
                    {
                        int iBlastBonus = 0;
                        iBlastBonus = (int) ((1.0 - ((double) type / 10.0))
                                              * getBlastValue(current_tank, weapon[aPreferredWeapon].damage, aPreferredWeapon)
                                              * ((defensive - 3.0) / -2.0));

                        if (iBlastBonus > 0)
                        {
                            current_score += iBlastBonus;
                            // if we need money, blast bonus is valued higher:

                            if (iMoneyNeed > 0)
                                current_score += iBlastBonus * ((double) type / 2.0);
                        }
                    }
                }

                if (aRotationMode && ((team == TEAM_NEUTRAL) || (team != current_tank->player->team)))
                {
                    // In rotationmode we try to actually reach the target with the preferred weapon
                    tank->cw = aPreferredWeapon;
                    _target = current_tank;
                    _targetX = current_tank->x;
                    _targetY = current_tank->y;
                    iTargettingRound = 0;
                    int iOvershoot = abs(calculateAttack(current_tank));
                    if (iOvershoot > _global->screenWidth)
                        current_score = -1 * MAX_OVERSHOOT;    // Wall-Hit! Target is unreachable!
                    else
                        current_score -= iOvershoot;    // substract overshoot!
#ifdef DEBUG
                    cout << " (rot)" << current_score;
#endif // DEBUG
                }

#ifdef DEBUG
                cout << " => " << current_score<< " : " << current_tank->player->getName() << endl;
#endif // DEBUG

                // decide if this target is better than others
                if ((current_score > best_score) || (!aRotationMode && !best_target))
                {
                    best_score = current_score;
                    best_target = current_tank;
                }
                attempts++;
            }
        }     // end of if we have a valid tank
    }

    if (best_target)
    {
        _target = best_target;
        if (_target)
        {
            _targetX= _target->x;
            _targetY= _target->y;
#ifdef DEBUG
            cout << " -> " << best_target->player->getName() << " wins! (";
            cout << best_target->l << " life, " << best_target->sh << " shield)" << endl;
#endif // DEBUG
        }
    }
#ifdef DEBUG
    else
        cout << " -> Unable to find target!!!" << endl;
#endif // DEBUG

    return best_target;
}

int PLAYER::getUnburyingTool()
{
    int iTool = 0;    // small missile, if nothing else fits
    if (nm[LRG_LAZER])
        iTool = LRG_LAZER;
    if (nm[MED_LAZER])
        iTool = MED_LAZER;
    if (nm[SML_LAZER])
        iTool = SML_LAZER;
    if (ni[ITEM_TELEPORT])
        iTool = WEAPONS + ITEM_TELEPORT;
    if (ni[ITEM_SWAPPER])
        iTool = WEAPONS + ITEM_SWAPPER;
    if (nm[HVY_RIOT_BOMB])
        iTool = HVY_RIOT_BOMB;
    if (nm[RIOT_BOMB])
        iTool = RIOT_BOMB;
    if (nm[RIOT_CHARGE])
        iTool = RIOT_CHARGE;
    if (nm[RIOT_BLAST])
        iTool = RIOT_BLAST;
    return iTool;
}

int PLAYER::computerSelectItem()
{
    int current_weapon = 0;    // Current Weapon (defaults to small missile)
    int iWeaponPool[15];
    int iPoolSize = (int) type * 3;

    // Initialize targetting:
    _target = NULL;
    _targetX= 0;
    _targetY= 0;

#ifdef DEBUG
    cout << getName() << " : Starting target and weapon evaluation..." << endl;
    if (defensive < -0.9)
        cout << "(True Offensive)" << endl;
    if ((defensive >=-0.9) && (defensive < -0.75))
        cout << "(Very Offensive)" << endl;
    if ((defensive >=-0.75) && (defensive < -0.25))
        cout << "(Offensive)" << endl;
    if ((defensive >=-0.25) && (defensive < 0.00))
        cout << "(Slightly Offensive)" << endl;
    if (defensive == 0.0)
        cout << "(Neutral)" << endl;
    if ((defensive >0.0) && (defensive <= 0.25))
        cout << "(Slightly Defensive)" << endl;
    if ((defensive >0.25) && (defensive <= 0.75))
        cout << "(Defensive)" << endl;
    if ((defensive >0.75) && (defensive <= 0.9))
        cout << "(Very Defensive)" << endl;
    if (defensive > 0.9)
        cout << "(True Defensive)" << endl;
    cout << "----------------------------------" << endl;
#endif // DEBUG
    // 1.: Preselection if buried

    if (tank->howBuried() > BURIED_LEVEL)
    {
        current_weapon = getUnburyingTool();
#ifdef DEBUGctank
        if (current_weapon < WEAPONS)
            cout << "I have chosen a \"" << weapon[current_weapon].name << "\" to free myself first!" << endl;
        else
            cout << "I have chosen a \"" << item[current_weapon - WEAPONS].name << "\" to free myself first!" << endl;
#endif // DEBUG
    }
    else
    {
        // 2.: Determine iPoolSize
        if (iPoolSize > 15)
            iPoolSize = 15;    // Or part-time-bots would bust array size!

        // 3.: Fill iWeaponPool
        iWeaponPool[0] = 0;    // The Small missile is always there!
        int count = 1;    // ...so start from second slot!

        while (count < iPoolSize)
        {
            int i = 0;
            // bots get a number of tries depending on their intelligence
            current_weapon = 0;
            while (!current_weapon && (i < ((int) type * 2)))
            {
                current_weapon = Select_Random_Weapon();
                if (!current_weapon)
                    current_weapon = Select_Random_Item();
                if ( (current_weapon >= THINGS)    //should never occur, but make it sure!
                     || ((current_weapon < WEAPONS) && (!nm[current_weapon]))
                     || ((current_weapon >= WEAPONS) && (!ni[current_weapon - WEAPONS])) )
                    current_weapon = 0;
                i++;
            }

            if (!current_weapon && (count > 1))
            {
                // Slot 0 is allways the small missile, slot 1 is always the first thing the bot "things" of
                // "Last Resort" switching takes place from slot 2 on
                if (nm[MED_MIS])
                    current_weapon = MED_MIS;

                // Only bots with 4+ slots (rangefinder and up) revert to the large missile
                if ((count > 2) && nm[LRG_MIS])
                    current_weapon = LRG_MIS;
            }
            iWeaponPool[count] = current_weapon;
            count++;
        }

        // 4a.: check if a dirtball is chosen
        if ((iWeaponPool[1] >= DIRT_BALL) && (iWeaponPool[1] <= SUP_DIRT_BALL))
            current_weapon = iWeaponPool[1];
        else
        {
            // 4b.: Sort iWeaponPool, so that the most liked weapon is first
            //  (...if the bot doesn't "forget" to sort ...)
            if (rand() % ((int) type + 1))
            {
                bool bIsSorted = false;
                while (!bIsSorted)
                {
                    bIsSorted = true;
                    // The bot does only sort the first few weapons (type+1)
                    // Stupid: first two, deadly: first 6
                    for (int i = 1; i < ( (int) type + 1); i++)
                    {
                        if (_weaponPreference[iWeaponPool[i-1]] < _weaponPreference[iWeaponPool[i]])
                        {
                            bIsSorted = false;
                            current_weapon = iWeaponPool[i-1];
                            iWeaponPool[i-1] = iWeaponPool[i];
                            iWeaponPool[i] = current_weapon;
                        }
                    }
                }
            }
            current_weapon = iWeaponPool[0]; // Most liked weapon, or, if not sorted, the small missile
            // Having the small missile here means, that the bot is selecting the most easy target.
            // Obviously that means, that the more stupid a bot is, the more often it will go for the easy strike.
        }
#ifdef DEBUG
        for (int i = 0; i < iPoolSize; i++)
        {
            cout << i << ".: ";
            printf( "% 5d Pref - ", _weaponPreference[iWeaponPool[i]]);
            if (iWeaponPool[i] < WEAPONS)
                cout << "\"" << weapon[iWeaponPool[i]].name << "\"" << endl;
            else
                cout << "\"" << item[iWeaponPool[i] - WEAPONS].name << "\"" << endl;
        }
#endif // DEBUG
        // if boxed mode is on, we have to try to find a target in rotation mode first!
        if ( _global->bIsBoxed && (current_weapon < WEAPONS) && ((current_weapon < DIRT_BALL) || (current_weapon > SUP_DIRT_BALL)) )
        {
            int i = 0;
            while ((i < iPoolSize) && !_target)
            {
                _target = computerSelectTarget(iWeaponPool[i], true);
                i++;
            }
            if (_target)
                current_weapon = iWeaponPool[i - 1];
        }
        if (!_target)
            _target = computerSelectTarget(current_weapon);
#ifdef OLD_GAMELOOP
        if (!_target && _oldTarget)
            _target = _oldTarget;
#endif
        if (!_target)
            return 0;    // If there is no target available, we have nothing more to do.
        _targetX = _target->x;
        _targetY = _target->y;

        // 5.: if a weapon is choosen, cycle through the pool
        // to find the best fitting one
        if ( (current_weapon < WEAPONS) && ((current_weapon < DIRT_BALL) || (current_weapon > SUP_DIRT_BALL)) )
        {
            int iBestWeapon = current_weapon;
            int iWeaponScore = 0;    // Score of the currently used weapon
            int iBestWeapScr = -5000;    // Best score calculated so far
            double dDamageMod = 0.0;    // modifier for how close to targets health
            int iWeaponDamage = 0;    // Calculate the (real) weapon damage
            int iTargetLife = _target->l + _target->sh;
            int iBurylevel = _target->howBuried();
            for (int i = 0; i < iPoolSize; i++)
            {
                current_weapon = iWeaponPool[i];

                // 1.: avoid trying to shoot below the tank's level with lasers
                if ( (_targetY >= tank->y) && ((current_weapon >= SML_LAZER) && (current_weapon <= LRG_LAZER)) )
                    iWeaponPool[i] = current_weapon = 0;    // revert to small missile
#ifdef DEBUG
                if (current_weapon < WEAPONS)
                    cout << " -> \"" << weapon[current_weapon].name << "\" : ";
                else
                    cout << " -> (ERROR!) \"" << item[current_weapon - WEAPONS].name << "\" : ";
#endif // DEBUG
                // 2.: The closer the weapon damage is to the target health, the:
                //     - more points added if it kills
                //     - less points substracted if it doesn't kill

                // avoid trying to use weapons we do not have
                if ((current_weapon < 0) || (current_weapon >= WEAPONS))
                    current_weapon = 0;

                if (weapon[current_weapon].numSubmunitions > 1)
                    iWeaponDamage = damageMultiplier * weapon[weapon[current_weapon].submunition].damage
                                    * (weapon[current_weapon].numSubmunitions / (defensive + 3.0));    // Clusters don't hit well (napalm?)
                else
                {
                    if (weapon[current_weapon].spread > 1)
                        iWeaponDamage = damageMultiplier * weapon[current_weapon].damage
                                        * (weapon[current_weapon].spread / (defensive + 2.0));    // Spreads *might* hit well, but do seldom
                    else
                        iWeaponDamage = damageMultiplier * weapon[current_weapon].damage;
                }

                // The more intelligent and defensive a bot is, the more (char *)"savety bonus" is granted:
                iWeaponDamage = (int) ((double) iWeaponDamage / (1.0 + (((double) type * (defensive + 2.0)) / 50.0)));

                // Examples:
                // Full offensive, useless: 1.0 + ((1.0 * 1.0) / 50.0) = 1.02 <== The damage is like 102% of the real damage
                // Full defensive, deadly : 1.0 + ((5.0 * 3.0) / 50.0) = 1.30 <== The damage is like 130% of the real damage
#ifdef DEBUG
                cout << iWeaponDamage << " damage, ";
#endif // DEBUG
                if (iTargetLife > iWeaponDamage)
                {
                    // The weapon is too weak, substract points. (Less if we are offensive)
                    dDamageMod = (double) iWeaponDamage / (double) iTargetLife;
                    if (defensive < 0)
                        iWeaponScore = (10 - (int) type) * (int) ((-10.0 * (1.1 + defensive)) / dDamageMod);
                    else
                        iWeaponScore = (10 - (int) type) * (int) (-10.0 / dDamageMod);
#ifdef DEBUG
                    cout << "weak: ";
#endif // DEBUG
                    /* Example calculations:
                       Bot is deadly: (int)type = 5
                       Weapon does 25% damage of the targets health: dDamegeMod = 0.25
                       iWeaponScore = (10 - 5) * (-10 / 0.25) = 5 * -40   = -200
                       Weapon does 50% damage of the targets health: dDamegeMod = 0.5
                       iWeaponScore = (10 - 5) * (-10 / 0.5)  = 5 * -20   = -100
                       Weapon does 75% damage of the targets health: dDamegeMod = 0.75
                       iWeaponScore = (10 - 5) * (-10 / 0.75) = 5 * -13,3 = -66,5
                       Weapon does 95% damage of the targets health: dDamegeMod = 0.95
                       iWeaponScore = (10 - 5) * (-10 / 0.95) = 5 * -10,5 = -52,5 */
                }
                else
                {
                    // The weapon is strong enough, add points (More, if we are defensive)
                    dDamageMod = (double) iTargetLife / (double) iWeaponDamage;
                    if (defensive > 0)
                        iWeaponScore = (int) type * (int) ((100.0 * (1.0 + defensive)) * dDamageMod);
                    else
                        iWeaponScore = (int) type * (int) (100.0 * dDamageMod);
#ifdef DEBUG
                    cout << "strong: ";
#endif // DEBUG
                    /* Example calculations:
                       Bot is deadly: (int)type = 5
                       Weapon does 105% damage of the targets health: dDamegeMod = 0.95
                       iWeaponScore = 5 * (100 * 0.95) = 5 * 95   = 475
                       Weapon does 125% damage of the targets health: dDamegeMod = 0.8
                       iWeaponScore = 5 * (100 * 0.8)  = 5 * 80   = 400
                       Weapon does 150% damage of the targets health: dDamegeMod = 0.67
                       iWeaponScore = 5 * (100 * 0.67) = 5 * 67   = 335
                       Weapon does 200% damage of the targets health: dDamegeMod = 0.5
                       iWeaponScore = 5 * (100 * 0.5)  = 5 * 50   = 250 */
                }
#ifdef DEBUG
                cout << dDamageMod << " dMod -> ";
#endif // DEBUG
                // 3.: Check if the way for a laser is clear if choosen
                if ((current_weapon >= SML_LAZER) && (current_weapon <= LRG_LAZER))
                {
                    int iXlow, iXhigh, iX;    // temp vars to calculate with
                    int iRockAmount = 0;    // How much mountain there is in between

                    if (tank->x < _targetX)
                    {
                        iXlow = tank->x;
                        iXhigh = _targetX;
                    }
                    else
                    {
                        iXlow = _targetX;
                        iXhigh = tank->x;
                    }

                    for (iX = iXlow; iX < iXhigh; iX++)
                    {
                        int iY = tank->y - ((tank->y - _targetY) / (iXhigh - iX));    // y the laser will be on it's way
                        if (_env->surface[iX] < iY)
                            iRockAmount++;    // Rock in the way!
                    }
                    iWeaponScore -= (int) type * iRockAmount * 10.0;
                }

                // 4.: If the target is burried, add points if we are using an adequate weapon
                if (iBurylevel > BURIED_LEVEL)
                {
                    // The target is burried!
                    if ( ((current_weapon < BURROWER) || (current_weapon > PENETRATOR))
                         && ((current_weapon < TREMOR) || (current_weapon > TECTONIC)) )
                    {
                        // Napalm and shaped charges are absolutely useless
                        if ( ((current_weapon >= SML_NAPALM) && (current_weapon <= LRG_NAPALM))
                             || ((current_weapon >= SHAPED_CHARGE) && (current_weapon <= CUTTER)) )
                            iWeaponScore -= (int) type * 500;    // Even the useless bot isn't *that* stupid
                        else
                        {
                            // For all other weapons we go for the radius of the blast
                            if (iBurylevel < weapon[current_weapon].radius)
                                iWeaponScore *= 1.0 - (((double) iBurylevel / (double) weapon[current_weapon].radius) / 2.0);
                            else
                                iWeaponScore -= (double) iWeaponDamage * (double) type * ((double) defensive + 3.0);
                        }
                    }
                    else
                        // As we *want* to fire an appropriate weapon, the target looks rather nice to us!
                        iWeaponScore += ((double) (iBurylevel - BURIED_LEVEL) / (((double) type + 1.0) / 2.0)) * (double) iWeaponDamage;
                }

                // 5.: Substract points, if we are within the blast radius
                // 6.: Check, whether other tanks are in the blast radius, and add points according
                //     to the additional damage delivered
                // Note: It seems to be paradox, that defensive bots add points for spread/cluster
                //       weapons, but they a) buy onyl few of them and b) add only noticably points
                //       if the collateral damage is enough to kill.
                if (type >= RANGEFINDER_PLAYER)
                    iWeaponScore += getBlastValue(_target, iWeaponDamage, current_weapon, dDamageMod);

                // 7.: Try to hit this target with the weapon, and substract the overshoot
                if ((type >= RANGEFINDER_PLAYER) && (current_weapon < WEAPONS))
                {
                    tank->cw = current_weapon;
                    iTargettingRound = 0;
                    iWeaponScore -= abs(calculateAttack(_target));
                }

                // 8.: Modify the Score by weapon preferences
                if (iWeaponScore > 0)
                    iWeaponScore = (int) ((double) iWeaponScore * (1.0 + ((double) _weaponPreference[current_weapon] / (double) MAX_WEAP_PROBABILITY)) );
                // (it will only have a slight effect unless the prefs are really wide apart and the
                //  original points very close to each other)
#ifdef DEBUG
                cout << iWeaponScore << " points!" << endl;
#endif // DEBUG
                // See if the choice fits better
                if (iWeaponScore > iBestWeapScr)
                {
                    iBestWeapScr = iWeaponScore;
                    iBestWeapon = current_weapon;
                }
            }
            current_weapon = iBestWeapon;
        }
    }

    // 6.: Kamikaze probability:
    // The more stupid and offensive a bot is, the more chance he has
    // to blow himself up when life is falling below a certain limit.
    if ( ((tank->l + tank->sh) < 30) || (((tank->l + tank->sh) < 50) && (tank->howBuried() > BURIED_LEVEL)) )
    {
        // A buried bot needs to free himself first, leaving all others a free shot!
        double dBlowMod = defensive + 2.0;    // gives 1.0 to 3.0
        dBlowMod *= (double) type / 2.0;    // gives 1.0 to 9.0
        dBlowMod += 1.0;    // gives 2.0 to 10.0
        // dBlowMod is now between  2.0 <== useless full offensive bot (50% chance)
        //                     and 10.0 <== deadly full defensive bots (10% chance)

        if (!(rand() % (int) dBlowMod))
        {
            // okay, I'm a goner, so BANZAAAAIIII! (maybe, check for a way first)
            int iWeaponDamage = 0;
            current_weapon = 0;
            if (nm[NUKE])
                current_weapon = NUKE;
            if (nm[DTH_SPREAD])
                current_weapon = DTH_SPREAD;
            if (nm[DTH_HEAD])
                current_weapon = DTH_HEAD;
            if (nm[ARMAGEDDON])
                current_weapon = ARMAGEDDON;
            if (nm[CUTTER])
                current_weapon = CUTTER;
            if (ni[ITEM_VENGEANCE])
                current_weapon = ITEM_VENGEANCE + WEAPONS;
            if (ni[ITEM_DYING_WRATH])
                current_weapon = ITEM_DYING_WRATH + WEAPONS;
            if (ni[ITEM_FATAL_FURY])
                current_weapon = ITEM_FATAL_FURY + WEAPONS;
            if (current_weapon < WEAPONS)
                iWeaponDamage = weapon[current_weapon].damage * damageMultiplier;
            else
                iWeaponDamage = weapon[(int) item[current_weapon - WEAPONS].vals[0]].damage * item[current_weapon - WEAPONS].vals[1] * damageMultiplier;

            // Is something available, and do we take others with us?
            if ((current_weapon > 0) && (getBlastValue (tank, iWeaponDamage, current_weapon) > 0.0))
            {
                // Yieha! Here we go!
#ifdef DEBUG
                printf("I have chosen to go bye bye with a...\n");
                if (current_weapon < WEAPONS)
                    printf("%s\n", weapon[current_weapon].name);
                else
                    printf("%s\n", item[current_weapon - WEAPONS].name);
#endif // DEBUG
                _targetX = tank->x;
                _targetY = tank->y;
                FLOATTEXT *kamikazeText;
                kamikazeText = new FLOATTEXT(_global, _env, selectKamikazePhrase(), (int) tank->x, (int) tank->y - 30, color, CENTRE);
                if (kamikazeText)
                {
                    // kamikazeText->xv = 0;
                    // kamikazeText->yv = -0.4;
                    kamikazeText->set_speed(0.0, -0.4);
                    kamikazeText->maxAge = 300;
                }
                else
                    perror("player.cpp: Failed allocating memory for kamikazeText in computerSelectItem.");
            }
        }
    }

#ifdef DEBUG
    if (_target)
        printf("Finished!\nShooting at %s\n", _target->player->getName());
    else
        printf("Finished!\nI'll free myself.\n");
    if (current_weapon < WEAPONS)
        printf("Using weapon %s\n", weapon[current_weapon].name);
    else
        printf("Using item %s\n", item[current_weapon - WEAPONS].name);
    printf("=============================================\n");
#endif // DEBUG
    tank->cw = current_weapon;
    _global->updateMenu = 1;
    return current_weapon;
}

int PLAYER::computerControls()
{
    int status = 0;
    // At the most basic: select target, select weapon, aim and fire
    tank->requireUpdate();
    if (_turnStage == SELECT_WEAPON)
    {
        computerSelectItem();
        iTargettingRound = 0;
        _turnStage = CALCULATE_ATTACK;
        _global->updateMenu = 1;
    }
    else if (_turnStage == SELECT_TARGET)
    {
        cout << "ERROR: _turnstage became SELECT_TARGET!" << endl;
        // Target is already chosen by computerSelectItem()
        _turnStage = SELECT_WEAPON;
    }
    else if (_turnStage == CALCULATE_ATTACK)
    {

#ifdef DEBUG_AIM_SHOW
        _global->bASD = true;    // Now it is allowed to be true
#endif

        calculateAttack(NULL);
        _turnStage = AIM_WEAPON;    // If the targetting wasn't finished, it will be done later!

#ifdef DEBUG_AIM_SHOW
        _global->bASD = false;    // And now it isn't!
#endif
    }
    else if (_turnStage == AIM_WEAPON)
    {
        // First: Determine whether there are any updates to be done yet:
        bool bDoAngleUpdate = false;
        bool bDoPowerUpdate = false;
        if (iTargettingRound == retargetAttempts)
        {
            // Do both when finished aiming
            bDoAngleUpdate = true;
            bDoPowerUpdate = true;
        }
        else if (iTargettingRound && (abs(_targetAngle - tank->a) <= 90))
            // Only do Angle update if still aiming and difference isn't too high
            bDoAngleUpdate = true;

        if (bDoAngleUpdate && (_targetAngle > tank->a && tank->a < 270))
        {
            // Left (If already aimed)
            tank->a++;
            _global->updateMenu = 1;
        }
        else if (bDoAngleUpdate && (_targetAngle < tank->a && tank->a > 90))
        {
            // Right (if already aimed)
            tank->a--;
            _global->updateMenu = 1;
        }
        else if (bDoPowerUpdate && (_targetPower < (tank->p - 3) && tank->p > 0))
        {
            // Reduce power (if targetting is finished)
            tank->p -= 5;
            _global->updateMenu = 1;
        }
        else if (bDoPowerUpdate && (_targetPower > (tank->p + 3) && tank->p < MAX_POWER))
        {
            // Increase Power (if targetting is finished)
            tank->p += 5;
            _global->updateMenu = 1;
        }
        else
        {
            // Targetting finished?
            if (iTargettingRound == retargetAttempts)
                _turnStage = FIRE_WEAPON; // Finished aiming, go ahead!
            else
                _turnStage = CALCULATE_ATTACK; // Not finished, do some more aiming
        }
    }
#ifdef OLD_GAMELOOP
    else if (fi)
    {
        // if (fi) don't do any of the following
    }
#endif
    else if (_turnStage == FIRE_WEAPON)
    {
        // tank->activateCurrentSelection();
        _global->updateMenu = 1;
#ifdef DEBUG
        printf("About to activate weapon.\n");
#endif
        tank->simActivateCurrentSelection();
        if (type == VERY_PART_TIME_BOT)
            type = NETWORK_CLIENT;
#ifdef DEBUG
        printf("Weapon was activated.\n");
#endif
        gloating = false;
        _turnStage = 0;
        status = CONTROL_FIRE;
    }
    return status;
}

int PLAYER::humanControls()
{
    int status = 0;

    if (tank)
    {
        if (!_env->mouseclock && mouse_b & 1 && mouse_x >= 250 && mouse_x < 378 && mouse_y >= 11 && mouse_y < 19)
        {
            _global->updateMenu = 1;
            if (tank->fs)
                tank->sht++;
            tank->fs = 1;
            if (tank->sht > SHIELDS - 1)
                tank->sht = 0;
        }
        if (!_env->mouseclock && mouse_b & 1 && mouse_x >= 250 && mouse_x < 378 && mouse_y >= 21
            && mouse_y < 29 && tank->player->ni[tank->sht] > 0 && (tank->fs || tank->sh > 0))
        {
            _global->updateMenu = 1;
            tank->ds = tank->sht;
            tank->player->ni[tank->sht]--;
            tank->sh = (int) item[tank->sht].vals[SHIELD_ENERGY];
        }

        tank->requireUpdate();
    }

    //Keyboard control
    if ( _env->stage == STAGE_AIM)
    {
        if (tank)
        {
            if ((key[KEY_LEFT] || key[KEY_A]) && !ctrlUsedUp && tank->a < 270)
            {
                tank->a++;
                _global->updateMenu = 1;
                if (key_shifts & KB_CTRL_FLAG)
                    ctrlUsedUp = TRUE;
            }
            if ((key[KEY_RIGHT] || key[KEY_D]) && !ctrlUsedUp && tank->a > 90)
            {
                tank->a--;
                _global->updateMenu = 1;
                if (key_shifts & KB_CTRL_FLAG)
                    ctrlUsedUp = TRUE;
            }
            if ((key[KEY_DOWN] || key[KEY_S]) && !ctrlUsedUp && tank->p > 0)
            {
                tank->p -= 5;
                _global->updateMenu = 1;
                if (key_shifts & KB_CTRL_FLAG)
                    ctrlUsedUp = TRUE;
            }
            if ((key[KEY_UP] || key[KEY_W]) && !ctrlUsedUp && tank->p < MAX_POWER)
            {
                tank->p += 5;
                _global->updateMenu = 1;
                if (key_shifts & KB_CTRL_FLAG)
                    ctrlUsedUp = TRUE;
            }
            if ((key[KEY_PGUP] || key[KEY_R]) && !ctrlUsedUp && tank->p < MAX_POWER )
            {
                tank->p += 100;
                if (tank->p > MAX_POWER)
                    tank->p = MAX_POWER;
                _global->updateMenu = 1;
                if (key_shifts & KB_CTRL_FLAG)
                    ctrlUsedUp = TRUE;
            }
            if ((key[KEY_PGDN] || key[KEY_F]) && !ctrlUsedUp && tank->p > 0)
            {
                tank->p -= 100;
                if (tank->p < 0)
                    tank->p = 0;
                _global->updateMenu = 1;
                if (key_shifts & KB_CTRL_FLAG)
                    ctrlUsedUp = TRUE;
            }
        }
    }

#ifdef OLD_GAMELOOP
    if (k && !fi)
#endif
#ifdef NEW_GAMELOOP
        // if (keypressed())
        //    k = readkey();
        // else
        //   k = 0;
        if (!k)
        {
            if (keypressed())
                k = readkey();
        }
    if (k)
#endif
    {
        status = CONTROL_PRESSED;
        if (_env->stage == STAGE_AIM)
        {
            if (tank)
            {
                int moved = 0;

                if (k >> 8 == KEY_N)
                {
                    tank->a = 180;
                    _global->updateMenu = 1;
                }
                if ((k >> 8 == KEY_TAB) || (k >> 8 == KEY_C))
                {
                    _global->updateMenu = 1;
                    while (1)
                    {
                        tank->cw++;
                        if (tank->cw >= THINGS)
                            tank->cw = 0;
                        if (tank->cw < WEAPONS)
                        {
                            if (tank->player->nm[tank->cw])
                                break;
                        }
                        else
                        {
                            if (item[tank->cw - WEAPONS].selectable && tank->player->ni[tank->cw - WEAPONS])
                                break;

                        }
                    }
                    //calcDamageMatrix (tank, tank->cw);
                    //selectTarget ();
                    changed_weapon = false;
                }
                if (( k >> 8 == KEY_BACKSPACE) || ( k >> 8 == KEY_Z))
                {
                    _global->updateMenu = 1;
                    while (1)
                    {
                        tank->cw--;
                        if (tank->cw < 0)
                            tank->cw = THINGS - 1;

                        if (tank->cw < WEAPONS)
                        {
                            if (tank->player->nm[tank->cw])
                                break;
                        }
                        else
                        {
                            if (item[tank->cw - WEAPONS].selectable && tank->player->ni[tank->cw - WEAPONS])
                                break;

                        }
                    }
                    changed_weapon = false;
                }

                // put the tank under computer control
                if (k >> 8 == KEY_F10)
                {
                    type = PART_TIME_BOT;
                    setComputerValues();
                    return (computerControls());
                }

                // move the tank
                if (k >> 8 == KEY_COMMA) // || (key >> 8 == KEY_H))
                    moved = tank->Move_Tank(DIR_LEFT);
                else if (k >> 8 == KEY_H)
                    moved = tank->Move_Tank(DIR_LEFT);

                if (k >> 8 == KEY_STOP) // || (key >> 8 == KEY_J))
                    moved = tank->Move_Tank(DIR_RIGHT);
                else if (k >> 8 == KEY_J)
                    moved = tank->Move_Tank(DIR_RIGHT);
                if (moved)
                    _global->updateMenu = 1;

                if ( (k >> 8 == KEY_SPACE) &&
                     (((tank->cw < WEAPONS) && (tank->player->nm[tank->cw] > 0))
                      || ((tank->cw < THINGS) && (tank->player->ni[tank->cw - WEAPONS] > 0))) )
                {
                    // tank->activateCurrentSelection();
                    tank->simActivateCurrentSelection();
                    gloating = false;
                    status = CONTROL_FIRE;
                }
            }
        }
        if ((_env->stage == STAGE_ENDGAME) && (k >> 8 == KEY_ENTER || k >> 8 == KEY_ESC || k >> 8 == KEY_SPACE))
            return -1;
    }
    return status;
}

// returns a static string to the player's team name
char *PLAYER::Get_Team_Name()
{
    char *team_name = "";

    switch ((int) team)
    {
    case TEAM_JEDI:
        team_name = "Jedi";
        break;
    case TEAM_NEUTRAL:
        team_name = "Neutral";
        break;
    case TEAM_SITH:
        team_name = "Sith";
        break;
    }

    return team_name;
}

// Pick a weapon to fire at random.
// The weapon number is returned. If no other
// weapon is in inventory, then 0 - small missile is
// used.
int PLAYER::Select_Random_Weapon()
{
    int index;
    int num_weapons = 0;
    int random_weapon;

    // count number of different weapons we have
    for (index = 1; index < WEAPONS; index++)
    {
        if (nm[index])
            num_weapons++;
    }

    // we have no weapons, use default
    if (num_weapons == 0)
        return 0;

    // pick a random offset from the bottom of the list
    random_weapon = (rand() % num_weapons) + 1;

    index = WEAPONS - 1;
    while (index && random_weapon)
    {
        if (nm[index])
            random_weapon--;
        if (random_weapon)
            index--;
    }

    // If the (char *)"weapon" is a dirtball, skip it 75% of the time:
    if (((index == DIRT_BALL) || (index == LRG_DIRT_BALL)) && (rand() % 4))
        return 0;

    // Skip if it is an unburying tool
    if ((index == RIOT_CHARGE) || (index == RIOT_BOMB) || (index == RIOT_BLAST) || (index == HVY_RIOT_BOMB))
        return 0;
    return index;
}

// This function selects a random item for
// the AI to use.
// This item to use is returned. If no
// item can be found, then the function
// returns 0
int PLAYER::Select_Random_Item()
{
    int index, item_num;
    int num_items = 0;
    int random_item;

    // count the items we have

    for (index = WEAPONS; index < THINGS; index++)
    {
        item_num = index - WEAPONS;

        if ((ni[item_num]) && (item[item_num].selectable))
            num_items++;
    }    // end of counting items

    if (!num_items)
        return 0;

    // if we have an item, there is still a 75% chance
    // that we may not use it
    if (rand() % 4)
        return 0;

    // pick a random offset from the bottom of the list
    random_item = (rand() % num_items) + 1;

    index = THINGS - 1;

    item_num = index - WEAPONS;

    while (item_num && random_item)
    {
        if ((ni[item_num]) && (item[item_num].selectable))
            random_item--;

        if (random_item)
        {
            index--;
            item_num = index - WEAPONS;
        }
    }

    if ((item_num == ITEM_TELEPORT) || (item_num == ITEM_SWAPPER))
        index = 0;

    //  // do not use teleport without a parachute
    //  if ( (item_num == ITEM_TELEPORT) && (! ni[ITEM_PARACHUTE]))
    //    index = 0;

    // do not self destruct
    if ((item_num == ITEM_VENGEANCE) || (item_num == ITEM_DYING_WRATH) || (item_num == ITEM_FATAL_FURY))
        index = 0;

    if (index > 0)
        index = item_num + WEAPONS;
    return index;
}

// This function takes one off the player's time to fire.
// If the player runs out of time, the function returns TRUE.
// If the player has time left, or no time clock is being used,
// then the function returns FALSE.
int PLAYER::Reduce_Time_Clock()
{
    if (!time_left_to_fire)    // not using clock
        return FALSE;

    time_left_to_fire--;
    if (!time_left_to_fire)    // ran out of time
    {
        tank->shots_fired++;    // pretend we fired
        time_left_to_fire = _global->max_fire_time;
        return TRUE;
    }
    return FALSE;
}

// The damage provided is not the weapon damage, but what the bot calculated!
int PLAYER::getBlastValue(TANK * aTarget, int aDamage, int aWeapon, double aDamageMod)
{
    int iHealth = 0;    // Health of the evaluated tank
    int iTeam;    // Team of the evaluated tank
    double dDistance;    // Distance of the target to the evaluated tank
    double dXdist, dYdist;    // needed to calculate dDistance
    int iDmgValue = 0;    // The value of the blast
    int iBlastDamage = 0;    // The damage the weapon is doing right there
    double dTeamMod = 1.0;    // Teams react differently on TAs
    int iWeapon = 0;    // Items (self destruc) count as CUTTERS for their blast radius!
    TANK * cOppTank;

    if (aWeapon < WEAPONS)
        iWeapon = aWeapon;
    else
        iWeapon = CUTTER;

    for (int i = 0; i < _global->numPlayers; i++)
    {
        cOppTank = _global->players[i]->tank;
        if (cOppTank && aTarget)
        {
            if (this == _global->players[i])
                // If we'd hit ourself, life is valued twice.
                iHealth = (cOppTank->l / 2) + cOppTank->sh;
            else
                iHealth = cOppTank->l + cOppTank->sh;
            iTeam = _global->players[i]->team;
            // Only count living targets that are not the official target
            if ((iHealth > 0) && (cOppTank != aTarget))
            {
                dXdist = fabs(aTarget->x - cOppTank->x) - (TANKWIDTH / 2);
                dYdist = fabs(aTarget->y - cOppTank->y) - (TANKHEIGHT / 2);
                if (dXdist < 0)
                    dXdist = 1.0;
                if (dYdist < 0)
                    dYdist = 0.0;
                if ( (aWeapon >= SHAPED_CHARGE) && (aWeapon <= CUTTER)
                      && ((dYdist >= (weapon[iWeapon].radius / 20)) || (dXdist <= (TANKWIDTH / 2))) )
                    dDistance = 2.0 * (double) weapon[iWeapon].radius;    // out of the way!
                else
                    dDistance = ABSDISTANCE(dXdist, dYdist, 0, 0) - ((double) type * (defensive + 2.0));

                // Now see whether the tank is in weapon blast range:
                if (dDistance <= weapon[iWeapon].radius)
                {
                    // Yep, it is!
                    iBlastDamage = (int) ((double) aDamage * (1.0 - ((dDistance / (double) weapon[iWeapon].radius) / 2.0)));
                    if ( ((team != TEAM_NEUTRAL) && (team == iTeam)) || (this == _global->players[i]) )
                    {
                        // it is a fellow team mate, or ourself
                        // teams act differently
                        switch ((int) team)
                        {
                        case TEAM_JEDI:
                            dTeamMod = ((double) type / 2.0) + 1.5;    // 2.0 up to 4.0
                            break;
                        case TEAM_SITH:
                            dTeamMod = ((double) type / 4.0) + 0.75;    // 1.0 up to 2.0
                            break;
                        default:
                            dTeamMod = (double) type;    // neutrals won't like to hit themselves at all!
                        }

                        if (iBlastDamage > iHealth)
                            // We would kill our mate, or cost us too much health!
                            iDmgValue -= (int) (dTeamMod * ((double) aDamage / aDamageMod) * ((defensive + 3.0) / 2.0));
                        else
                            // We don't kill, but count the blast at least
                            iDmgValue -= (int) (dTeamMod * ((double) iBlastDamage / aDamageMod) * ((defensive + 3.0) / 2.0));

                        // Note: The /=aDamageMod results in blast damage counted worse, if the aDamageMod is low
                    }
                    else
                    {
                        // Just Someone... see if we kill them
                        if (iBlastDamage > iHealth)
                            // Woohoo!
                            iDmgValue += (int) (((1.0 + (double) type / 10)) * (double) (iBlastDamage + iHealth) * ((defensive + 3.0) / 2.0));
                        else
                            // Well, at least...
                            iDmgValue += (int) (((1.0 + (double) type / 10)) * aDamageMod * (double) (iBlastDamage) * ((defensive - 3.0) / -2.0));
                    }
                }
            }
        }
    }
#ifdef DEBUG
    if ((iDmgValue != 0) && (aDamageMod != 1.0))
        cout << "(blast: " << iDmgValue << ") ";
#endif
    return iDmgValue;
}

int PLAYER::getMoneyToSave()
{
    double dMoneyToSave = 0.0;
    int iAvgPref = 0;
    int iPrefLimit = 0;
    int iPrefCount = 0;
    int iInvCount = 0;
    int iInvMoney = 0;
    int iMaxCost = 0;

    // if the preferences are exceptionally low, a div by 0 might occur, so it has to be dynamized:
    for (int i = 0; i < WEAPONS; i++)
    {
        if (_weaponPreference[i] > iPrefLimit)
            iPrefLimit = (iPrefLimit + _weaponPreference[i]) / 2;
    }
    // Now it is guaranteed, that iPrefCount and iAvgPref will result in values > 0!
    for (int i = 0; i < WEAPONS; i++)
    {
        if (_weaponPreference[i] > iPrefLimit)
        {
            iPrefCount++;
            iAvgPref += _weaponPreference[i];
        }
    }
    // we are running into divide by zero here.
    if (iPrefCount < 1)
        iPrefCount = 1;
    iAvgPref /= iPrefCount; // Now the average of all relevant weapon preferences

    // Now we search for everything over this average and count costs and inventory value:
    for (int i = 0; i < WEAPONS; i++)
    {
        if (_weaponPreference[i] > iAvgPref)
        {
            // This weapon is (char *)"wanted"
            dMoneyToSave += weapon[i].cost;
            if (weapon[i].cost > iMaxCost)
                iMaxCost = weapon[i].cost;
            if (nm[i])
            {
                // We have (some) if this already!
                iInvCount++;
                iInvMoney += (int) (((double) nm[i] / (double) weapon[i].amt) * (double) weapon[i].cost);
            }
        }
    }

    // The Maximum amount is (type * 2) times the most expensive weapon we like:
    iMaxCost *= (int) type * 2;
    // As of writing this, this means a maximum of:
    // Armageddon: 100,000 credits
    // Deadly Bot: (int)type == 5
    // iMaxCost = 100,000 * 5 * 2 = 1,000,000 credits maximum!

    dMoneyToSave = (double) iMaxCost * 0.25;
#ifdef DEBUG
    printf("% 4d Prefs, worth % 6d\n", iPrefCount, iAvgPref);
    printf("% 4d Items, worth % 6d\n", iInvCount, iInvMoney);
    printf("MoneyToSave:      % 6d\n", (int)dMoneyToSave);
    printf("Money: % 6d -> MaxCost: %6d\n", (int)money, iMaxCost);
#endif // DEBUG
    // Whenever dMoneyToSave is less than the money owned, iBoostItemsBought is resetted
    if (money > (int) dMoneyToSave)
        iBoostItemsBought = 0; // Let's go!
    return ((int) dMoneyToSave);
}

// if we have some shield strength at the end of the round, then
// reclaim this shield back into our inventory
int PLAYER::Reclaim_Shield()
{
    if (!tank)
        return FALSE;
    if (!last_shield_used)
        return FALSE;

    if (tank->sh > 0)
        ni[last_shield_used] += 1;

    last_shield_used = 0;
    return TRUE;
}

#ifdef NETWORK
// Removes the newline character and anything after it
// from a string.
void PLAYER::Trim_Newline(char *line)
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

// This function checks for incoming data from a client.
// If data is coming in, we put the incoming data in the net_command
// variable. If the socket connection is broken, then we will
// close the socket and hand control over to the AI.
int PLAYER::Get_Network_Command()
{
    int status;

    status = Check_For_Incoming_Data(server_socket);
    if (status)
    {
        // we have something coming down the pipe
        memset(net_command, '\0', NET_COMMAND_SIZE);    // clear buffer
        status = read(server_socket, net_command, NET_COMMAND_SIZE);
        if (!status)    // connection is broken
        {
            close(server_socket);
            type = DEADLY_PLAYER;
            printf("%s lost network connection. Returning control to AI.\n", _name);
            return FALSE;
        }
        else   // we got data
        {
            net_command[NET_COMMAND_SIZE - 1] = '\0';
            Trim_Newline(net_command);
        }
    }
    return TRUE;
}

// This function gets called during a round when a networked
// player gets to act. The function checks to see if anything
// is in the net_command variable. If there is, it handles
// the request. If not, the function returns.
//
// We should have some time keeping in here before this goes live
// to avoid hanging the game.
int PLAYER::Execute_Network_Command(int my_turn)
{
    char buffer[NET_COMMAND_SIZE];
    static int player_index = -1;
    static int fire_delay = 0, net_delay = 0;

    if (my_turn)
    {
        fire_delay++;
        // if enough time has passed, we give up and turn control over to the AI
        if (fire_delay >= NET_DELAY)
        {
            setComputerValues();
            type = VERY_PART_TIME_BOT;
            fire_delay = 0;
            return (computerControls());
        }
    }

    if (!net_command[0])
    {
        net_delay++;
        /*
        if (my_delay >= NET_DELAY)
        {
            my_delay = 0;
            setComputerValues();
            type = VERY_PART_TIME_BOT;
            strcpy(buffer, "PING");
            write(server_socket, buffer, strlen(buffer));
            return (computerControls());
        }
        else */
        if (net_delay >= NET_DELAY_SHORT)
        {
            // prompt the client to respond
            strcpy(buffer, "PING");
            write(server_socket, buffer, strlen(buffer));
            return 0;
        }
        return 0;
    }     // we did not get a command to process

    else
        net_delay = 0;    // we got something, so reset timer

    if (!strncmp(net_command, "VERSION", 7))
    {
        sprintf(buffer, "SERVERVERSION %s", VERSION);
        write(server_socket, buffer, strlen(buffer));
    }
    else if (!strncmp(net_command, "CLOSE", 5))
    {
        close(server_socket);
        type = DEADLY_PLAYER;
    }
    else if (!strncmp(net_command, "BOXED", 5))
    {
        char buffer[32];
        if (_global->bIsBoxed)
            strcpy(buffer, "BOXED 1");
        else
            strcpy(buffer, "BOXED 0");
        write(server_socket, buffer, strlen(buffer));
    }
    else if (!strncmp(net_command, "GOSSIP", 6))
    {
        snprintf(_global->tank_status, 128, "%s", & (net_command[7]));
        _global->updateMenu = TRUE;
    }
    else if (!strncmp(net_command, "HEALTH", 6))
    {
        int tank_index;

        sscanf( &(net_command[7]), "%d", &tank_index);
        if ( (tank_index >= 0) && (tank_index < _global->numPlayers))
        {
            if (_global->players[tank_index]->tank)
            {
                char buffer[64];

                snprintf(buffer, 64, "HEALTH %d %d %d %d", tank_index, _global->players[tank_index]->tank->l,
                         _global->players[tank_index]->tank->sh, _global->players[tank_index]->tank->sht);
                write(server_socket, buffer, strlen(buffer));
            }
        }

    }
    else if (!strncmp(net_command, "ITEM", 4))
    {
        int item_index;
        sscanf( &(net_command[5]), "%d", &item_index);
        if ( (item_index >= 0) && (item_index < ITEMS))
        {
            char buffer[32];

            sprintf(buffer, "ITEM %d %d", item_index, ni[item_index]);
            write(server_socket, buffer, strlen(buffer));
        }
    }
    else if (!strncmp(net_command, "MOVE", 4))
    {
        if (!my_turn)
            return 0;
        if (tank)
        {
            if (strstr(net_command, "LEFT"))
                tank->Move_Tank(DIR_LEFT);
            else
                tank->Move_Tank(DIR_RIGHT);
            _global->updateMenu = 1;
        }
    }
    else if (!strncmp(net_command, "FIRE", 4))
    {
        int angle = 180, power = 1000, item = 0;
        if (!my_turn)
            return 0;
        sscanf( & (net_command[5]), "%d %d %d", &item, &angle, &power);
        fire_delay = 0;
        if (tank)
        {
            if (item >= THINGS) item = 0;
            tank->cw = item;
            if (item < WEAPONS)
            {
                if (nm[tank->cw] < 1)
                    tank->cw = 0;
            }
            else    // item
            {
                if (ni[tank->cw - WEAPONS] < 1)
                    tank->cw = 0;
            }
            tank->a = angle;
            if (tank->a > 270)
                tank->a = 270;
            else if (tank->a < 90)
                tank->a = 90;
            tank->p = power;
            if (tank->p > 2000)
                tank->p = 2000;
            else if (tank->p < 0)
                tank->p = 0;
            tank->simActivateCurrentSelection();
            gloating = false;
            net_command[0] = '\0';
            return CONTROL_FIRE;
        }
    }

    // find out which player this is
    else if (!strncmp(net_command, "WHOAMI", 6))
    {
        bool found = false;
        char buffer[128];

        while ((player_index < _global->numPlayers) && !found)
        {
            if (_global->players[player_index] == this)
            {
                found = true;
                sprintf(buffer, "YOUARE %d", player_index);
            }
            else
                player_index++;
        }
        // check to see if something went very wrong
        if (!found)
            strcpy(buffer, "YOUARE -1");
        write(server_socket, buffer, strlen(buffer));
    }
    // return wind speed
    else if (!strncmp(net_command, "WIND", 4))
    {
        char buffer[64];
        sprintf(buffer, "WIND %f", _env->wind);
        write(server_socket, buffer, strlen(buffer));
    }

    // find out how many players we have
    else if (!strncmp(net_command, "NUMPLAYERS", 10))
    {
        char buffer[32];
        sprintf(buffer, "NUMPLAYERS %d", _global->numPlayers);
        write(server_socket, buffer, strlen(buffer));
    }
    else if (!strncmp(net_command, "PLAYERNAME", 10))
    {
        int my_number;
        sscanf(&(net_command[11]), "%d", &my_number);
        if ( (my_number >= 0) && (my_number < _global->numPlayers) )
        {
            char buffer[128];

            sprintf(buffer, "PLAYERNAME %d %s", my_number, _global->players[my_number]->getName() );
            write(server_socket, buffer, strlen(buffer));
        }
    }

    // how many rounds are we playing
    else if (!strncmp(net_command, "ROUNDS", 6))
    {
        char buffer[64];
        sprintf(buffer, "ROUNDS %d %d", (int) _global->rounds, _global->currentround);
        write(server_socket, buffer, strlen(buffer));
    }
    // send back the position of each tank
    else if (!strncmp(net_command, "TANKPOSITION", 12))
    {
        int count;

        sscanf( &(net_command[13]), "%d", &count);
        if ( (count >= 0) && (count < _global->numPlayers))
        {
            if (_global->players[count]->tank)
            {
                char buffer[64];

                sprintf(buffer, "TANKPOSITION %d %d %d", count, (int) _global->players[count]->tank->x,
                        (int) _global->players[count]->tank->y);
                write(server_socket, buffer, strlen(buffer));
            }
        }
    }

    // send back the surface height of the dirt
    else if (!strncmp(net_command, "SURFACE", 7))
    {
        int x;

        sscanf(&(net_command[8]), "%d", &x);
        if ( (x >= 0) && (x < _global->screenWidth))
        {
            char buffer[32];

            sprintf(buffer, "SURFACE %d %d", x, _env->surface[x]);
            write(server_socket, buffer, strlen(buffer));
        }
    }
    else if (!strncmp(net_command, "SCREEN", 6))
    {
        char buffer[64];
        sprintf(buffer, "SCREEN %d %d", _global->screenWidth, _global->screenHeight);
        write(server_socket, buffer, strlen(buffer));
    }
    else if (!strncmp(net_command, "TEAMS", 5))
    {
        int count;

        sscanf( &(net_command[6]), "%d", &count);
        if ((count < _global->numPlayers) && (count >= 0))
        {
            char buffer[32];

            sprintf(buffer, "TEAM %d %d", count, (int) _global->players[count]->team);
            write(server_socket, buffer, strlen(buffer));
        }
    }
    else if (!strncmp(net_command, "WALLTYPE", 8))
    {
        char buffer[32];
        sprintf(buffer, "WALLTYPE %d", _env->current_wallType);
        write(server_socket, buffer, strlen(buffer));
    }
    else if (!strncmp(net_command, "WEAPON", 6))
    {
        int weapon_number;

        sscanf(&(net_command[7]), "%d", &weapon_number);
        if ((weapon_number >= 0) && (weapon_number < WEAPONS))
        {
            char buffer[32];

            sprintf(buffer, "WEAPON %d %d", weapon_number, nm[weapon_number]);
            write(server_socket, buffer, strlen(buffer));
        }
    }

    net_command[0] = '\0';
    return 0;
}

#endif
