#ifndef GAMELOOP_HEADER_FILE__
#define GAMELOOP_HEADER_FILE__

#include "globaldata.h"
#include "environment.h"

#define MAX_TEXT_BOUNCE 40

// The massive game loop, re-write here for
// all sorts of reasons.

int game(GLOBALDATA *global, ENVIRONMENT *env);

#endif /* GAMELOOP_HEADER_FILE__ */
