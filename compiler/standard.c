#include <stdlib.h>
#include <string.h>

static char *my_strndup(const char *s, size_t n)
{
    if (s == NULL)
        return NULL;
    size_t len = strlen(s);
    if (len < n)
        n = len;
    char *new_str = malloc(n + 1);
    if (new_str == NULL)
        return NULL;
    memcpy(new_str, s, n);
    new_str[n] = '\0';
    return new_str;
}

// Custom strdup implementation since it's not part of C99 standard
static char *my_strdup(const char *s)
{
    if (s == NULL)
        return NULL;
    size_t len = strlen(s) + 1;
    char *new_str = malloc(len);
    if (new_str == NULL)
        return NULL;
    return memcpy(new_str, s, len);
}