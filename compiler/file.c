#include "file.h"
#include "debug.h"
#include <stdio.h>

char *file_read(Arena *arena, const char *path)
{
    if (!path || !arena) {
        DEBUG_ERROR("Invalid arguments: path=%p, arena=%p", (void*)path, (void*)arena);
        return NULL;
    }

    FILE *file = fopen(path, "rb");
    if (!file) {
        DEBUG_ERROR("Failed to open file: %s", path);
        return NULL;
    }

    // Get file size
    if (fseek(file, 0, SEEK_END) != 0) {
        DEBUG_ERROR("Failed to seek to end of file: %s", path);
        fclose(file);
        return NULL;
    }

    long size = ftell(file);
    if (size < 0) {
        DEBUG_ERROR("Failed to get file size: %s", path);
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        DEBUG_ERROR("Failed to seek to start of file: %s", path);
        fclose(file);
        return NULL;
    }

    // Allocate buffer with arena
    char *buffer = arena_alloc(arena, (size_t)size + 1);
    if (!buffer) {
        DEBUG_ERROR("Failed to allocate memory for file contents: %s", path);
        fclose(file);
        return NULL;
    }

    // Read file contents
    size_t bytes_read = fread(buffer, 1, (size_t)size, file);
    if (bytes_read < (size_t)size) {
        DEBUG_ERROR("Failed to read file contents: %s", path);
        fclose(file);
        return NULL;
    }

    buffer[size] = '\0';
    fclose(file);
    return buffer;
}