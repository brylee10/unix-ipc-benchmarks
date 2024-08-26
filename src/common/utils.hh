#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

// Set to 1 to enable runtime asserts
#define COMPILE_ASSERTS 0

#if COMPILE_ASSERTS == 1
#define ASSERT(expr) assert(expr)
#else
#define ASSERT(expr) ((void)0)
#endif

void report_and_exit(const char *msg);
