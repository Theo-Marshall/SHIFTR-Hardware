#include <Logger.h>
#include <cstdio>

/**
 * Logs a debug message including a new line
 * CORE_DEBUG_LEVEL must be set to at least 4
 * 
 * @param format Format string
 * @param args Arguments
 */
void Logger::logDebug(const char *format, ...) {
  if (CORE_DEBUG_LEVEL >= 4) {
    va_list args;
    va_start (args, format);
    vprintf (format, args);
    va_end (args);
    printf("\n");
  }
}
