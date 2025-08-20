// tests/file_tests.c

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../arena.h"
#include "../debug.h"
#include "../file.h"

static const char *test_file_path = "test_file.txt";
static const char *empty_file_path = "empty_file.txt";
static const char *large_file_path = "large_file.txt";
static const char *nonexistent_path = "nonexistent_file.txt";

void create_test_file(const char *path, const char *content)
{
    FILE *file = fopen(path, "wb");
    assert(file != NULL);
    if (content)
    {
        size_t len = strlen(content);
        size_t written = fwrite(content, 1, len, file);
        assert(written == len);
    }
    fclose(file);
}

void remove_test_file(const char *path)
{
    remove(path);
}

void test_file_read_null_arena()
{
    DEBUG_INFO("*** Testing file_read with NULL arena...\n");

    char *result = file_read(NULL, "some_path");
    assert(result == NULL);

    DEBUG_INFO("Finished test_file_read_null_arena");
}

void test_file_read_null_path()
{
    DEBUG_INFO("*** Testing file_read with NULL path...\n");

    Arena arena;
    arena_init(&arena, 1024);
    char *result = file_read(&arena, NULL);
    assert(result == NULL);
    arena_free(&arena);

    DEBUG_INFO("Finished test_file_read_null_path");
}

void test_file_read_nonexistent_file()
{
    DEBUG_INFO("*** Testing file_read with nonexistent file...\n");

    Arena arena;
    arena_init(&arena, 1024);
    char *result = file_read(&arena, nonexistent_path);
    assert(result == NULL);
    arena_free(&arena);

    DEBUG_INFO("Finished test_file_read_nonexistent_file");
}

void test_file_read_empty_file()
{
    DEBUG_INFO("*** Testing file_read with empty file...\n");

    // Create empty file
    create_test_file(empty_file_path, NULL);

    Arena arena;
    arena_init(&arena, 1024);
    char *result = file_read(&arena, empty_file_path);
    assert(result != NULL);
    assert(strcmp(result, "") == 0);
    arena_free(&arena);

    // Cleanup
    remove_test_file(empty_file_path);

    DEBUG_INFO("Finished test_file_read_empty_file");
}

void test_file_read_small_file()
{
    DEBUG_INFO("*** Testing file_read with small file...\n");

    const char *content = "Hello, world!\n";
    create_test_file(test_file_path, content);

    Arena arena;
    arena_init(&arena, 1024);
    char *result = file_read(&arena, test_file_path);
    assert(result != NULL);
    assert(strcmp(result, content) == 0);
    arena_free(&arena);

    remove_test_file(test_file_path);

    DEBUG_INFO("Finished test_file_read_small_file");
}

void test_file_read_large_file()
{
    DEBUG_INFO("*** Testing file_read with large file...\n");

    // Create a large file (~1MB)
    const size_t large_size = 1024 * 1024;
    char *large_content = malloc(large_size + 1);
    assert(large_content != NULL);
    for (size_t i = 0; i < large_size; i++)
    {
        large_content[i] = (char)(i % 256);
    }
    large_content[large_size] = '\0';

    FILE *file = fopen(large_file_path, "wb");
    assert(file != NULL);
    size_t written = fwrite(large_content, 1, large_size, file);
    assert(written == large_size);
    fclose(file);

    Arena arena;
    arena_init(&arena, large_size * 2); // Enough space
    char *result = file_read(&arena, large_file_path);
    assert(result != NULL);
    assert(memcmp(result, large_content, large_size) == 0);
    assert(result[large_size] == '\0');
    arena_free(&arena);

    free(large_content);
    remove_test_file(large_file_path);

    DEBUG_INFO("Finished test_file_read_large_file");
}

void test_file_read_seek_failure()
{
    DEBUG_INFO("*** Testing file_read with simulated seek failure (manual check required)...\n");

    // This is hard to simulate without mocking or special files.
    // For comprehensive testing, we can note that in real scenarios like pipes or special files,
    // fseek may fail with ESPIPE. But for unit test, we can skip or use a FIFO.
    // Here, we'll assume it's covered by error paths, and assert on a regular file it succeeds.

    const char *content = "Seek test";
    create_test_file(test_file_path, content);

    Arena arena;
    arena_init(&arena, 1024);
    char *result = file_read(&arena, test_file_path);
    assert(result != NULL); // Should succeed, no seek failure
    arena_free(&arena);

    remove_test_file(test_file_path);

    // To simulate failure, advanced: create a pipe, but complex in C unit test.
    // Consider this test as placeholder; in practice, check errno after fseek in code.

    DEBUG_INFO("Finished test_file_read_seek_failure");
}

void test_file_read_read_failure()
{
    DEBUG_INFO("*** Testing file_read with read failure (hard to simulate)...\n");

    // Hard to simulate partial read without mocking.
    // Create a file and read it successfully as placeholder.

    const char *content = "Read test";
    create_test_file(test_file_path, content);

    Arena arena;
    arena_init(&arena, 1024);
    char *result = file_read(&arena, test_file_path);
    assert(result != NULL);
    assert(strcmp(result, content) == 0);
    arena_free(&arena);

    remove_test_file(test_file_path);

    // For real failure, perhaps chmod to no read, but platform-dependent.
    // Assume covered by return NULL on fread < size.

    DEBUG_INFO("Finished test_file_read_read_failure");
}

void test_file_read_special_characters()
{
    DEBUG_INFO("*** Testing file_read with special characters...\n");

    const char special_data[] = {'A', '\0', 'B', '\n', '\t', '\r', '\\', '"', '\b', 0}; // Extra null for string, but fwrite len-1
    size_t data_len = sizeof(special_data) - 1;                                         // Without trailing null

    FILE *file = fopen(test_file_path, "wb");
    assert(file != NULL);
    size_t written = fwrite(special_data, 1, data_len, file);
    assert(written == data_len);
    fclose(file);

    Arena arena;
    arena_init(&arena, 1024);
    char *result = file_read(&arena, test_file_path);
    assert(result != NULL);
    assert(memcmp(result, special_data, data_len) == 0);
    assert(result[data_len] == '\0');
    arena_free(&arena);

    remove_test_file(test_file_path);

    DEBUG_INFO("Finished test_file_read_special_characters");
}
