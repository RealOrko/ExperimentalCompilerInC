#include "arena.h"
#include "string.h"

char *unescape_string(Arena *arena, const char *input, int length) {
    DEBUG_VERBOSE("Unescaping string of length %d", length);
    char *result = arena_alloc(arena, length + 1);  // Max possible size (no shrinkage needed with arena).
    if (result == NULL) {
        DEBUG_ERROR("Failed to allocate memory for unescaped string");
        return NULL;
    }

    int j = 0;
    for (int i = 0; i < length; ++i) {
        if (input[i] == '\\') {
            ++i;
            if (i >= length) {
                // Truncated escape: treat as literal '\'.
                result[j++] = '\\';
                break;
            }
            switch (input[i]) {
                case 'n': result[j++] = '\n'; break;
                case 't': result[j++] = '\t'; break;
                case 'r': result[j++] = '\r'; break;
                case 'b': result[j++] = '\b'; break;
                case 'f': result[j++] = '\f'; break;
                case '\\': result[j++] = '\\'; break;
                case '"': result[j++] = '"'; break;
                case '\'': result[j++] = '\''; break;
                default:
                    // Unknown escape: append as-is (e.g., \x -> \x).
                    result[j++] = '\\';
                    result[j++] = input[i];
                    break;
            }
        } else {
            result[j++] = input[i];
        }
    }
    result[j] = '\0';
    DEBUG_VERBOSE("Unescaped string to: %s", result);
    return result;
}
