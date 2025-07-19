#ifndef RUNTIME_H
#define RUNTIME_H

#include <stddef.h>

// String operations
char *rt_str_concat(const char *left, const char *right);

char *rt_to_string_long(long val);
char *rt_to_string_double(double val);
char *rt_to_string_char(char val);
char *rt_to_string_bool(int val);
char *rt_to_string_string(const char *val);

// Print functions
void rt_print_long(long val);
void rt_print_double(double val);
void rt_print_char(long c);
void rt_print_string(const char *s);
void rt_print_bool(long b);

// Long (integer) operations
long rt_add_long(long a, long b);
long rt_sub_long(long a, long b);
long rt_mul_long(long a, long b);
long rt_div_long(long a, long b);
long rt_mod_long(long a, long b);

int rt_eq_long(long a, long b);
int rt_ne_long(long a, long b);
int rt_lt_long(long a, long b);
int rt_le_long(long a, long b);
int rt_gt_long(long a, long b);
int rt_ge_long(long a, long b);

// Double (floating point) operations
double rt_add_double(double a, double b);
double rt_sub_double(double a, double b);
double rt_mul_double(double a, double b);
double rt_div_double(double a, double b);

int rt_eq_double(double a, double b);
int rt_ne_double(double a, double b);
int rt_lt_double(double a, double b);
int rt_le_double(double a, double b);
int rt_gt_double(double a, double b);
int rt_ge_double(double a, double b);

// Unary operations
long rt_neg_long(long a);
double rt_neg_double(double a);

int rt_not_bool(int a);

// Post-increment/decrement
long rt_post_inc_long(long *p);
long rt_post_dec_long(long *p);

// Pre-increment/decrement
long rt_pre_inc_long(long *p);
long rt_pre_dec_long(long *p);

#endif // RUNTIME_H
