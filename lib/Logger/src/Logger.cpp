#include <Logger.h>
#include <cstdio>
#include <stdarg.h>

/**
 * Logs a message including a new line
 * 
 * @param logLevel Log level
 * @param functionName Function name
 * @param fmt Format string
 * @param ... Arguments
 * @return Length of the logged message
 */
int logger_printf(int logLevel, const char *functionName, const char *fmt, ...) {
  int length = 0;
  if (CORE_DEBUG_LEVEL >= logLevel) {
    length += printf("%s(): ", functionName);
    va_list args;
    va_start (args, fmt);
    length = vprintf(fmt, args);
    va_end (args);
    length += printf("\n");
  }
  return length;
}
