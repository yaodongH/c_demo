#pragma once
#include "uv.h"
#include "utilityies.h"
#include <stdarg.h>
#include "slice.h"

void __init_middlewares(int argc, ...);

#define init_middleware(...) __init_middlewares(NARG(__VA_ARGS__), __VA_ARGS__)
