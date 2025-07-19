/**
 * runtime.c
 * Runtime helpers for the compiler, including string operations.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h> // For potential error handling
#include <math.h>  // For isnan and isinf in rt_print_double
#include <limits.h>

char *rt_str_concat(const char *left, const char *right)
{
    if (left == NULL)
    {
        fprintf(stderr, "rt_str_concat: left argument is NULL\n");
        return NULL;
    }
    if (right == NULL)
    {
        fprintf(stderr, "rt_str_concat: right argument is NULL\n");
        return NULL;
    }

    size_t left_len = strlen(left);
    size_t right_len = strlen(right);
    size_t new_len = left_len + right_len;

    if (new_len > (size_t)-1 - 1)
    { // Check for potential overflow
        fprintf(stderr, "rt_str_concat: concatenated string length overflow\n");
        exit(1);
    }

    char *new_str = malloc(new_len + 1); // +1 for null terminator
    if (new_str == NULL)
    {
        fprintf(stderr, "Memory allocation failed for string concatenation (length: %zu)\n", new_len);
        exit(1); // Or handle gracefully in production
    }

    memcpy(new_str, left, left_len);
    memcpy(new_str + left_len, right, right_len + 1); // Includes null terminator

    return new_str;
}

void rt_print_long(long val)
{
    // No specific checks needed for long, as printf handles all values
    printf("%ld", val);
}

void rt_print_double(double val)
{
    if (isnan(val))
    {
        printf("NaN");
    }
    else if (isinf(val))
    {
        if (val > 0)
        {
            printf("Inf");
        }
        else
        {
            printf("-Inf");
        }
    }
    else
    {
        printf("%.5f", val);
    }
}

void rt_print_char(long c)
{
    if (c < 0 || c > 255)
    {
        fprintf(stderr, "rt_print_char: invalid char value %ld (must be 0-255)\n", c);
        printf("?");
    }
    else
    {
        printf("%c", (int)c);
    }
}

void rt_print_string(const char *s)
{
    if (s == NULL)
    {
        fprintf(stderr, "rt_print_string: NULL string\n");
        printf("(null)");
    }
    else
    {
        printf("%s", s);
    }
}

void rt_print_bool(long b)
{
    // Treat 0 as false, anything else as true; no check needed
    printf("%s", b ? "true" : "false");
}

long rt_add_long(long a, long b)
{
    // Check for potential overflow (though signed overflow is undefined behavior in C;
    // this is a basic check assuming two's complement and no wraparound desired)
    if ((b > 0 && a > LONG_MAX - b) || (b < 0 && a < LONG_MIN - b))
    {
        fprintf(stderr, "rt_add_long: overflow detected\n");
        exit(1);
    }
    return a + b;
}

long rt_sub_long(long a, long b)
{
    // Similar overflow check for subtraction
    if ((b > 0 && a < LONG_MIN + b) || (b < 0 && a > LONG_MAX + b))
    {
        fprintf(stderr, "rt_sub_long: overflow detected\n");
        exit(1);
    }
    return a - b;
}

long rt_mul_long(long a, long b)
{
    // Basic overflow check (not foolproof due to undefined behavior, but approximate)
    if (a != 0 && b != 0 && ((a > LONG_MAX / b) || (a < LONG_MIN / b)))
    {
        fprintf(stderr, "rt_mul_long: overflow detected\n");
        exit(1);
    }
    return a * b;
}

long rt_div_long(long a, long b)
{
    if (b == 0)
    {
        fprintf(stderr, "Division by zero\n");
        exit(1);
    }
    // Check for overflow in division (e.g., LONG_MIN / -1)
    if (a == LONG_MIN && b == -1)
    {
        fprintf(stderr, "rt_div_long: overflow detected (LONG_MIN / -1)\n");
        exit(1);
    }
    return a / b;
}

long rt_mod_long(long a, long b)
{
    if (b == 0)
    {
        fprintf(stderr, "Modulo by zero\n");
        exit(1);
    }
    // Similar check for modulo overflow (LONG_MIN % -1 is undefined)
    if (a == LONG_MIN && b == -1)
    {
        fprintf(stderr, "rt_mod_long: overflow detected (LONG_MIN %% -1)\n");
        exit(1);
    }
    return a % b;
}

int rt_eq_long(long a, long b) { return a == b; }
int rt_ne_long(long a, long b) { return a != b; }
int rt_lt_long(long a, long b) { return a < b; }
int rt_le_long(long a, long b) { return a <= b; }
int rt_gt_long(long a, long b) { return a > b; }
int rt_ge_long(long a, long b) { return a >= b; }

double rt_add_double(double a, double b)
{
    double result = a + b;
    if (isinf(result) && !isinf(a) && !isinf(b))
    {
        fprintf(stderr, "rt_add_double: overflow to infinity\n");
        exit(1);
    }
    return result;
}

double rt_sub_double(double a, double b)
{
    double result = a - b;
    if (isinf(result) && !isinf(a) && !isinf(b))
    {
        fprintf(stderr, "rt_sub_double: overflow to infinity\n");
        exit(1);
    }
    return result;
}

double rt_mul_double(double a, double b)
{
    double result = a * b;
    if (isinf(result) && !isinf(a) && !isinf(b))
    {
        fprintf(stderr, "rt_mul_double: overflow to infinity\n");
        exit(1);
    }
    return result;
}

double rt_div_double(double a, double b)
{
    if (b == 0.0)
    {
        fprintf(stderr, "Division by zero\n");
        exit(1);
    }
    double result = a / b;
    if (isinf(result) && !isinf(a) && b != 0.0)
    {
        fprintf(stderr, "rt_div_double: overflow to infinity\n");
        exit(1);
    }
    return result;
}

int rt_eq_double(double a, double b) { return a == b; }
int rt_ne_double(double a, double b) { return a != b; }
int rt_lt_double(double a, double b) { return a < b; }
int rt_le_double(double a, double b) { return a <= b; }
int rt_gt_double(double a, double b) { return a > b; }
int rt_ge_double(double a, double b) { return a >= b; }

long rt_neg_long(long a)
{
    // Check for overflow (negating LONG_MIN is undefined)
    if (a == LONG_MIN)
    {
        fprintf(stderr, "rt_neg_long: overflow detected (-LONG_MIN)\n");
        exit(1);
    }
    return -a;
}

double rt_neg_double(double a) { return -a; }

int rt_not_bool(int a)
{
    // No additional checks; !a is always valid for int
    return !a;
}

long rt_post_inc_long(long *p)
{
    if (p == NULL)
    {
        fprintf(stderr, "rt_post_inc_long: NULL pointer\n");
        exit(1);
    }
    // Check for overflow on increment
    if (*p == LONG_MAX)
    {
        fprintf(stderr, "rt_post_inc_long: overflow detected\n");
        exit(1);
    }
    return (*p)++;
}

long rt_post_dec_long(long *p)
{
    if (p == NULL)
    {
        fprintf(stderr, "rt_post_dec_long: NULL pointer\n");
        exit(1);
    }
    // Check for overflow on decrement
    if (*p == LONG_MIN)
    {
        fprintf(stderr, "rt_post_dec_long: overflow detected\n");
        exit(1);
    }
    return (*p)--;
}