#pragma once
#ifndef ATANKS_SRC_CLOCK_H_INCLUDED
#define ATANKS_SRC_CLOCK_H_INCLUDED

#include "debug.h"
#include <cstdint>

int32_t game_us_get();
void    game_us_reset();
int32_t menu_ms_get();
void    menu_ms_reset();

/// REMOVE_VS12_WORKAROUND
#if defined(ATANKS_IS_MSVC) && !defined(ATANKS_IS_AT_LEAST_MSVC13)
void win_clock_deinit();
void win_clock_init();
#define WIN_CLOCK_INIT   win_clock_init();
#define WIN_CLOCK_REMOVE win_clock_deinit();
#else
#define WIN_CLOCK_INIT {}
#define WIN_CLOCK_REMOVE {}
#endif

#endif // ATANKS_SRC_CLOCK_H_INCLUDED

