// Updated 2026 portable logging adapter. Algorithm code retains original credits.
#include "PlatformDependent.h"
#include <iostream>
void showString(std::stringstream& message) {
#ifdef CUTOUT_VERBOSE_LOGGING
    std::clog << message.str() << '\n';
#else
    (void)message;
#endif
}
