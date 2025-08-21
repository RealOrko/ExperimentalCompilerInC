#ifndef STRING_H
#define STRING_H

#include "arena.h"
#include "debug.h"

char *unescape_string(Arena *arena, const char *input, int length);

#endif