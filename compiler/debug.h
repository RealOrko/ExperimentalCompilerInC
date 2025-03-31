/**
 * debug.h
 * Debugging utilities for the compiler
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdlib.h>

// Debug levels
#define DEBUG_LEVEL_NONE 0
#define DEBUG_LEVEL_ERROR 1
#define DEBUG_LEVEL_WARNING 2
#define DEBUG_LEVEL_INFO 3
#define DEBUG_LEVEL_VERBOSE 4

// Current debug level (can be changed at runtime)
extern int debug_level;

// Initialize debugging
void init_debug(int level);

// Debug macros
#define DEBUG_ERROR(fmt, ...)                                                           \
    if (debug_level >= DEBUG_LEVEL_ERROR)                                               \
    {                                                                                   \
        fprintf(stderr, "[ERROR] %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    }

#define DEBUG_WARNING(fmt, ...)                                                           \
    if (debug_level >= DEBUG_LEVEL_WARNING)                                               \
    {                                                                                     \
        fprintf(stderr, "[WARNING] %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    }

#define DEBUG_INFO(fmt, ...)                                                           \
    if (debug_level >= DEBUG_LEVEL_INFO)                                               \
    {                                                                                  \
        fprintf(stderr, "[INFO] %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    }

#define DEBUG_VERBOSE(fmt, ...)                                                           \
    if (debug_level >= DEBUG_LEVEL_VERBOSE)                                               \
    {                                                                                     \
        fprintf(stderr, "[VERBOSE] %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    }

#define DEBUG_VERBOSE_INDENT(level, fmt, ...)                     \
    if (debug_level >= DEBUG_LEVEL_VERBOSE)                       \
    {                                                             \
        fprintf(stderr, "[VERBOSE] %s:%d: ", __FILE__, __LINE__); \
        if (fmt == NULL)                                          \
        {                                                         \
            fprintf(stderr, "Null format\n");                     \
            abort();                                              \
        }                                                         \
        for (int i = 0; i < level; i++)                           \
        {                                                         \
            fprintf(stderr, " ");                                 \
        }                                                         \
        fprintf(stderr, fmt "\n", ##__VA_ARGS__);                 \
    }

// Assert macro
#define ASSERT(condition, fmt, ...)                                                      \
    if (!(condition))                                                                    \
    {                                                                                    \
        fprintf(stderr, "[ASSERT] %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
        abort();                                                                         \
    }

#endif // DEBUG_H