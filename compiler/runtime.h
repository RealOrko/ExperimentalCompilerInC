#ifndef RUNTIME_H
#define RUNTIME_H

// String operations
extern char* rt_str_concat(const char* left, const char* right);

// Print functions
extern void rt_print_long(long val);
extern void rt_print_double(double val);
extern void rt_print_char(long c);
extern void rt_print_string(const char* s);
extern void rt_print_bool(long b);

// Long (integer) operations
extern long rt_add_long(long a, long b);
extern long rt_sub_long(long a, long b);
extern long rt_mul_long(long a, long b);
extern long rt_div_long(long a, long b);
extern long rt_mod_long(long a, long b);

extern int rt_eq_long(long a, long b);
extern int rt_ne_long(long a, long b);
extern int rt_lt_long(long a, long b);
extern int rt_le_long(long a, long b);
extern int rt_gt_long(long a, long b);
extern int rt_ge_long(long a, long b);

// Double (floating point) operations
extern double rt_add_double(double a, double b);
extern double rt_sub_double(double a, double b);
extern double rt_mul_double(double a, double b);
extern double rt_div_double(double a, double b);

extern int rt_eq_double(double a, double b);
extern int rt_ne_double(double a, double b);
extern int rt_lt_double(double a, double b);
extern int rt_le_double(double a, double b);
extern int rt_gt_double(double a, double b);
extern int rt_ge_double(double a, double b);

// Unary operations
extern long rt_neg_long(long a);
extern double rt_neg_double(double a);
extern int rt_not_bool(int a);

// Post-increment/decrement
extern long rt_post_inc_long(long *p);
extern long rt_post_dec_long(long *p);

// Pre-increment/decrement
extern long rt_pre_inc_long(long *p);
extern long rt_pre_dec_long(long *p);

#endif
