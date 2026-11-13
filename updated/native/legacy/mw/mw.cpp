// Updated 2026: replace the recovered no-op error callback with an exception.
// MegaWave2 authors and laboratory notices are retained in neighboring sources.
#include "mw.h"
#include <cstdarg>
#include <cstdio>
#include <stdexcept>
void mwerror(int severity, int, const char* format, ...) {
    char message[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    if (severity == WARNING) { std::fprintf(stderr, "MegaWave2: %s", message); return; }
    throw std::runtime_error(message);
}
