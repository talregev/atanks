#pragma once
#ifndef ATANKS_SRC_WINCLOCK_H_INCLUDED
#define ATANKS_SRC_WINCLOCK_H_INCLUDED

/* Workaround for the buggy <chrono> implementation of VS12.
 *
 * I do not know whether this is more embarrassing for me or Microsoft.
 * For me, because I didn't realize this before it was too late, I guess.
 * The steady_clock is not steady, and the high_resolution_clock is not
 * high resolution.
 * This will be fixed in VS13, but while writing this only a community
 * preview has been published. This preview is noted to be not production
 * ready and should not be installed on a system that can't be formatted.
 *
 * So until I have a full production ready VS13 (Visual Studio 2015), this
 * header works around the buggy <chrono> implementation.
 *
 * For more information see:
 * https://connect.microsoft.com/VisualStudio/feedback/details/858357/
 * and
 * https://connect.microsoft.com/VisualStudio/feedback/details/753115/
 */

#if defined(ATANKS_IS_MSVC)
#include "main.h"

#ifdef USE_MUTEX_INSTEAD_OF_SPINLOCK
#  include <mutex>
#  define CSpinLock std::mutex
#endif // USE_MUTEX_INSTEAD_OF_SPINLOCK

// timer variable and locker
bool       has_win_clock  = false;
CSpinLock  win_clock_lock;
volatile
int32_t    win_clock      = 0;

// additional functions:
void win_clock_add()
{
	++win_clock;
}
END_OF_FUNCTION(win_clock_add)

void win_clock_deinit()
{
	if ( has_win_clock ) {
		remove_int(win_clock_add);
		has_win_clock = false;
	}
	win_clock = 0;
}

inline int32_t win_clock_get()
{
	win_clock_lock.lock();
	int32_t result = win_clock;
	win_clock = 0;
	win_clock_lock.unlock();
	return result;
}

void win_clock_init()
{
	win_clock = 0;
	if ( !has_win_clock ) {
		LOCK_VARIABLE(win_clock)
		LOCK_FUNCTION(win_clock_add)
		install_int_ex(win_clock_add, MSEC_TO_TIMER(1));
		has_win_clock = true;
	}
}

// Re-implement the millisecond sensitive functions:
int32_t game_us_get()
{
	return win_clock_get() * 1000;
}


void game_us_reset()
{
	win_clock_lock.lock();
	win_clock = 0;
	win_clock_lock.unlock();
}


int32_t menu_ms_get()
{
	return win_clock_get();
}


void menu_ms_reset()
{
	win_clock_lock.lock();
	win_clock = 0;
	win_clock_lock.unlock();
}

#endif // defined(ATANKS_IS_MSVC)

#endif // ATANKS_SRC_WINCLOCK_H_INCLUDED

