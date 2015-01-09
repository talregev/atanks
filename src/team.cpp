#include "tank.h"
#include "team.h"
#include "player.h"

// check to see if a team has won.
int Team_Won(GLOBALDATA *global)
{
  bool all_jedi = true;
  bool all_sith = true;
  int current_player = 0;
  int my_team = 1;
  int player_count = 0;

  while ((all_jedi || all_sith) && ( current_player < global->numPlayers) )
    {
      if ( (global->players[current_player]->tank) &&
           (global->players[current_player]->tank->l) )
        {
          my_team = (int)global->players[current_player]->team;
          if ( (my_team == TEAM_JEDI) || (my_team == TEAM_NEUTRAL) )
            all_sith = false;

          if ( (my_team == TEAM_SITH) || (my_team == TEAM_NEUTRAL) )
            all_jedi = false;
          player_count++;
        }
      current_player++;
    }

  if (! player_count) return NO_WIN;
  if (all_jedi) return JEDI_WIN;
  else if (all_sith) return SITH_WIN;
  else return NO_WIN;
}

