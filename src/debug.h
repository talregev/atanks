#pragma once
#ifndef ATANKS_SRC_DEBUG_H_INCLUDED
#define ATANKS_SRC_DEBUG_H_INCLUDED

#include <cstdio>

/********************************************************
 * Determine whether we build for BSD, Linux or Windows *
 *******************************************************/
#if defined(_WIN32) || defined(__WIN32__)
# define ATANKS_IS_WINDOWS
# if defined(_MSC_VER)
#   define ATANKS_IS_MSVC
    // See whether the chrono bug is fixed:
#   if (_MSC_VER >= 1900)
#     define ATANKS_IS_AT_LEAST_MSVC13
#   endif // VS 2015
# endif // _MSVC_VER
#endif // Win 32

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__) || defined(__DragonFly__)
# define ATANKS_IS_BSD
#endif // BSD

#if defined(__linux__) || defined(__linux) || defined(linux) || defined(ATANKS_IS_BSD)
# define ATANKS_IS_LINUX
#endif // Linux

#if !defined(ATANKS_IS_WINDOWS) && !defined(ATANKS_IS_LINUX)
# error "Only Windows, Linux and BSD are supported at the moment"
#endif // Windows versus Linux


/**********************************************************
 * Define some macros to work around compiler differences *
 *********************************************************/
#if defined(ATANKS_IS_MSVC)
# define atanks_snprintf(target, char_count, fmt, ...) \
    _snprintf_s(target, char_count + 1, char_count, fmt, __VA_ARGS__)
# define atanks_tzset() _tzset()
# define atanks_localtime(tm_p, time_p) localtime_s(tm_p, time_p)
#else
# define atanks_snprintf(target, char_count, fmt, ...) \
    snprintf(target, char_count, fmt, __VA_ARGS__)
# define atanks_tzset() tzset()
# define atanks_localtime(tm_p, time_p) localtime_r(time_p, tm_p)
# define vsprintf_s(tgt, size, fmt, listvar) vsprintf(tgt, fmt, listvar)
#endif // MSVC++ versus GCC/Clang


// Windows special definitions
#if defined(ATANKS_IS_WINDOWS)
# ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
# endif

# ifndef WIN32_EXTRA_LEAN
#   define WIN32_EXTRA_LEAN
# endif

# ifndef VC_EXTRALEAN
#   define VC_EXTRALEAN
# endif

# define NOMINMAX

#endif // IS_WINDOWS


/// ==============================================================
/// === The following is only used if ATANKS_DEBUG is defined. ===
/// === Otherwise the macro DEBUG_LOG() does nothing.          ===
/// ==============================================================


#if defined(ATANKS_DEBUG)

// ATANKS_GET_FILE - This macro simply sets target to the current filename without a path
#ifdef ATANKS_IS_MSVC
#  define ATANKS_GET_FILE(target) { \
     char _atanks_fname_info[64]; \
     char _atanks_extension[8]; \
     _splitpath_s(__FILE__, NULL, 0, NULL, 0, _atanks_fname_info, 63, _atanks_extension, 7); \
     atanks_snprintf(target, 255, "%s%s", _atanks_fname_info, \
               _atanks_extension); \
   }
#else
#  define ATANKS_GET_FILE(target) { \
     atanks_snprintf(target, 255, "%s", basename(__FILE__)); \
   }
#endif


// ATANKS_GET_POS - This macro gathers positional information
#ifdef ATANKS_IS_MSVC
#  define ATANKS_GET_POS(target) { \
     ATANKS_GET_FILE(target) \
     atanks_snprintf(target, 255, "%-10s:%4d %-24s", target, __LINE__, __FUNCTION__); \
   }
#else
#  define ATANKS_GET_POS(target) { \
     atanks_snprintf(target, 255, "%-10s:%4d %-24s", basename(__FILE__), __LINE__, __FUNCTION__); \
   }
#endif


// DEBUG_LOG - This macro is a wrapper for debug_log
#define DEBUG_LOG(Title, Msg, ...) { \
   char _atanks_trace_info[256]; \
   ATANKS_GET_POS(_atanks_trace_info) \
   debug_log(_atanks_trace_info, Title, Msg, __VA_ARGS__); \
 }

// declaration if debug_log
void debug_log(const char* moduleName, const char* title, const char* message, ...);


#else

#define DEBUG_LOG(...) {}

#endif // defined(ATANKS_DEBUG)

// Enable / Disable specific debug message flavours
// Those do nothing, even if enabled, if ATANKS_DEBUG is not defined

#ifdef ATANKS_DEBUG_AIMING
#  define DEBUG_LOG_AIM(Title, Msg, ...) DEBUG_LOG(Title, Msg, __VA_ARGS__)
#else
#  define DEBUG_LOG_AIM(...) {}
#endif // ATANKS_DEBUG_AIMING

#ifdef ATANKS_DEBUG_EMOTIONS
#  define DEBUG_LOG_EMO(Title, Msg, ...) DEBUG_LOG(Title, Msg, __VA_ARGS__)
#else
#  define DEBUG_LOG_EMO(...) {}
#endif // ATANKS_DEBUG_EMOTIONS

// The next is a helper so those messages can show up whenever
// either aiming or emotions shall be logged.
#if defined(ATANKS_DEBUG_AIMING) || defined(ATANKS_DEBUG_EMOTIONS)
#  define DEBUG_LOG_AI(Title, Msg, ...) DEBUG_LOG(Title, Msg, __VA_ARGS__)
#else
#  define DEBUG_LOG_AI(...) {}
#endif // ATANKS_DEBUG_AIMING || ATANKS_DEBUG_EMOTIONS


#ifdef ATANKS_DEBUG_FINANCE
#  define DEBUG_LOG_FIN(Title, Msg, ...) DEBUG_LOG(Title, Msg, __VA_ARGS__)
#else
#  define DEBUG_LOG_FIN(...) {}
#endif // ATANKS_DEBUG_FINANCE

#ifdef ATANKS_DEBUG_OBJECTS
#  define DEBUG_LOG_OBJ(Title, Msg, ...) DEBUG_LOG(Title, Msg, __VA_ARGS__)
#else
#  define DEBUG_LOG_OBJ(...) {}
#endif // ATANKS_DEBUG_OBJECTS

#ifdef ATANKS_DEBUG_PHYSICS
#  define DEBUG_LOG_PHY(Title, Msg, ...) DEBUG_LOG(Title, Msg, __VA_ARGS__)
#else
#  define DEBUG_LOG_PHY(...) {}
#endif // ATANKS_DEBUG_OBJECTS


#endif // ATANKS_SRC_DEBUG_H_INCLUDED

