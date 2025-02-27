/**
 * debug.c
 * Implementation of debugging utilities
 */

 #include "debug.h"

 // Default debug level
 int debug_level = DEBUG_LEVEL_ERROR;
 
 void init_debug(int level) {
     debug_level = level;
 }