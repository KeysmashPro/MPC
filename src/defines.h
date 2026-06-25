// MACROS DEFINES AND TYPEDEFS

#ifndef DEFINES_U_H
#define DEFINES_U_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <execinfo.h>

#define iterate(i, n) for (size_t (i) = 0; (i) < (size_t)(n); ++(i))

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t  i8;

typedef long double f80;
typedef double      f64;
typedef float       f32;


// ANSI COLOR CODES
#define CLR           "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_PURPLE  "\033[35m"
#define COLOR_BOLD    "\033[1m"


// ERROR HANDLING 
#define error(c, ...) do { \
    fprintf(stderr, COLOR_YELLOW "[ERROR] code %d: " CLR, c); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n"); \
    return (c); \
} while(0)

#define fail(...) do { \
    fprintf(stderr, COLOR_RED COLOR_BOLD "[FAIL] " CLR COLOR_RED); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, CLR "\n"); \
    exit(EXIT_FAILURE); \
} while(0)

#define info(...) do { \
  fprintf(stderr, COLOR_PURPLE "[INFO] " CLR); \
  fprintf(stderr, __VA_ARGS__); \
  fprintf(stderr, "\n"); \
} while(0)

#ifdef DEBUG
    #define debug(...) do { \
        fprintf(stderr, COLOR_BLUE "[DEBUG] " CLR); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } while(0)
#else
    #define debug(...) ((void)0)
#endif

#endif
