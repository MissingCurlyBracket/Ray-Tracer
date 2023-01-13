#pragma once

#if defined(__clang__)
#define CLANG 1
#elif defined(__GNUC__)
#define GCC 1
#elif defined(_MSC_VER)
#define MSVC 1
#endif

#if defined(CLANG)
#define DISABLE_WARNINGS_PUSH() _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wunused-function\"") \
    _Pragma("clang diagnostic ignored \"-Wpragma-pack\"") \
    _Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"")
#define DISABLE_WARNINGS_POP() _Pragma("clang diagnostic pop")
#elif defined(GCC)
#define DISABLE_WARNINGS_PUSH() \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wsign-compare\"") \
    _Pragma("GCC diagnostic ignored \"-Wsign-conversion\"") \
    _Pragma("GCC diagnostic ignored \"-Wdouble-promotion\"") \
    _Pragma("GCC diagnostic ignored \"-Wold-style-cast\"") \
    _Pragma("GCC diagnostic ignored \"-Wduplicated-branches\"") \
    _Pragma("GCC diagnostic ignored \"-Wuseless-cast\"") \
    _Pragma("GCC diagnostic ignored \"-Wconversion\"") \
    _Pragma("GCC diagnostic ignored \"-Wunused-but-set-variable\"")
#define DISABLE_WARNINGS_POP() _Pragma("GCC diagnostic pop")
#elif defined(MSVC)
#define DISABLE_WARNINGS_PUSH() __pragma(warning(push, 0)) __pragma(warning())
#define DISABLE_WARNINGS_POP() __pragma(warning(pop))
#else
#define DISABLE_WARNINGS_PUSH()
#define DISABLE_WARNINGS_POP()
#endif
