/*
This file contains team related functions that
do not fit anywhere else.
*/

#ifndef TEAM_HEADER_FILE__
#define TEAM_HEADER_FILE__

#include "globaldata.h"

#define NO_WIN 0
#define JEDI_WIN 100
#define SITH_WIN 200

// This function checks to see if one
// team (Jedi or Sith) has won. Neutral
// is not considered a team.
// The function return the following:
// 0 - no team won
// JEDI_WON - Jedi team won
// SITH_WON - Sith team won

int Team_Won(GLOBALDATA *global);

#endif

