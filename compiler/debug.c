#include "debug.h"

int debug_level = DEBUG_LEVEL_ERROR;

void init_debug(int level)
{
    debug_level = level;
}