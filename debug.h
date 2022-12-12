#include <stdio.h>

#ifdef DEBUG_FILE
#define DEBUG(...) printf(__VA_ARGS__)
#else
#define DEBUG(...)
#endif // not DEBUG_FILE
