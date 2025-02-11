#include <cstdarg>
#ifndef LOGGER_H
#define LOGGER_H

#ifndef CORE_DEBUG_LEVEL
  #define CORE_DEBUG_LEVEL 4
#endif

//#define logTest(format,__VA_ARGS__...) log_printf(ARDUHAL_LOG_FORMAT(E, format), ## __VA_ARGS__)

/**
 * Logger class for logging debug messages
 * Logging is based on CORE_DEBUG_LEVEL values
 * 0 - no logging
 * 1 - errors only
 * 2 - errors and warnings
 * 3 - errors, warnings and info
 * 4 - errors, warnings, info and debug
 */
class Logger {
 public:
  static void logDebug(const char *format, ...);
};

#endif