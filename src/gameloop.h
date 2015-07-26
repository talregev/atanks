#ifndef GAMELOOP_HEADER_FILE__
#define GAMELOOP_HEADER_FILE__

#define MAX_TEXT_BOUNCE 40

#include "player.h"

/// Helper Class to background level creation
class LevelCreator
{
	volatile bool in_progress[4];
	volatile bool i_must_yield   = true;
	volatile bool i_shall_die    = false;
	CSpinLock     fiLock;
	int32_t       fiVal          = 0;
	void add_fi();
public:
	explicit LevelCreator();
	void operator()();
	bool can_work() const;
	void die_now();
	bool has_progress();
	bool is_finished() const;
	void print_state() const;
	void work_alone();
	void working_on(int32_t what);
	void yield();
};


/// @brief small struct to order the score board, used by the end-of-game-sorting, too
struct sScore
{
	int32_t     color  = BLACK;
	int32_t     diff   = 0;
	int32_t     idx    = -1;
	int32_t     killed = 0;
	int32_t     kills  = 0;
	const char* name   = nullptr;
	sScore*     next   = nullptr;
	sScore*     prev   = nullptr;
	int32_t     score  = 0;

	sScore &operator=(PLAYER &rhs)
	{
		color  = rhs.color;
		idx    = rhs.index;
		killed = rhs.killed;
		kills  = rhs.kills;
		name   = rhs.getName();
		score  = rhs.score;
		diff   = kills - killed;
		return *this;
	}
};


// The massive game loop, rewritten here for
// all sorts of reasons.
void game();

// sort players. The returned scores point to an array that must be deleted.
sScore* sort_scores();


#endif

