#include "clock.h"

#include <chrono>

/// === Used clocks and time granularity ===
using atanks_clock_t = std::chrono::steady_clock;

#if defined(ATANKS_IS_MSVC) && !defined(ATANKS_IS_AT_LEAST_MSVC13)
  // Note: this is a bug in vc12, that is fixed in vc13.
  // See: https://connect.microsoft.com/VisualStudio/feedback/details/858357/steady-clock-now-returning-the-wrong-type
# include "winclock.h"
  using time_point_t = std::chrono::time_point<std::chrono::system_clock>;
#else
  using time_point_t = std::chrono::time_point<atanks_clock_t>;
#endif // MSVC++ 2013 bug

using clock_ms_t = std::chrono::milliseconds;
using clock_us_t = std::chrono::microseconds;

/// === Helper macros to not have ridiculously long lines ===
#define CLOCK_NOW  atanks_clock_t::now()
#define MS_CAST(x) static_cast<int32_t>(std::chrono::duration_cast<clock_ms_t>(x).count())
#define US_CAST(x) static_cast<int32_t>(std::chrono::duration_cast<clock_us_t>(x).count())

/// === Internal values only used here ===
static time_point_t game_us_end   = CLOCK_NOW;
static time_point_t game_us_start = CLOCK_NOW;
static time_point_t menu_ms_end   = CLOCK_NOW;
static time_point_t menu_ms_start = CLOCK_NOW;


/// === Function implementations ===

/// REMOVE_VS12_WORKAROUND
#if !defined(ATANKS_IS_MSVC) || defined(ATANKS_IS_AT_LEAST_MSVC13)
int32_t game_us_get()
{
	game_us_end     = CLOCK_NOW;
	int32_t used_us = US_CAST(game_us_end - game_us_start);
	game_us_start   = game_us_end;
	return used_us > 0 ? used_us : 0;
}


void game_us_reset()
{
	game_us_end   = CLOCK_NOW;
	game_us_start = game_us_end;
}


int32_t menu_ms_get()
{
	menu_ms_end     = CLOCK_NOW;
	int32_t used_us = MS_CAST(menu_ms_end - menu_ms_start);
	menu_ms_start   = menu_ms_end;
	return used_us > 0 ? used_us : 0;
}


void menu_ms_reset()
{
	menu_ms_end   = CLOCK_NOW;
	menu_ms_start = menu_ms_end;
}
#endif // !ATANKS_IS_MSVC
