cmake_minimum_required(VERSION 3.16)

project(ExpressionInterpreter
    VERSION 1.0.0
    LANGUAGES C
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Platform guard ────────────────────────────────────────────────────────────
# if(WIN32)
#     message(FATAL_ERROR
#         "Windows is not yet supported. "
#         "Build on a POSIX system (Linux, macOS, FreeBSD)."
#     )
# endif()

# ── C standard ────────────────────────────────────────────────────────────────
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
# set(CMAKE_C_EXTENSIONS OFF)

# ── Build type default ────────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type specified – defaulting to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose: Debug or Release" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release)
endif()

# ── Sources ───────────────────────────────────────────────────────────────────
set(JADE_SOURCES
    src/allocator.c
    src/container.c
    src/defs.c
    src/evaluator.c
    src/hashmap.c
    src/lexer.c
    src/logger.c
    src/main.c
    src/parser.c
    src/registry.c
    src/utils.c
)

# ── Executable ────────────────────────────────────────────────────────────────
add_executable(jade ${JADE_SOURCES})

# ── Include directories ───────────────────────────────────────────────────────
target_include_directories(jade PRIVATE src/include)

# ── Per-configuration compile options ────────────────────────────────────────
target_compile_options(jade PRIVATE
    $<$<CONFIG:Debug>:
        -Wall
        -Wextra
        -g
    >
    $<$<CONFIG:Release>:
        -Wall
        -O3
        -march=native
        -fvisibility=hidden
        -DNDEBUG
    >
)

# ── Math & POSIX real-time library ────────────────────────────────────────────
# clock_gettime() lives in librt on some older POSIX systems (not needed on
# macOS / FreeBSD where it is part of libc).
target_link_libraries(jade PRIVATE m)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # librt provides clock_gettime on older glibc (< 2.17).
    find_library(RT_LIBRARY rt)
    if(RT_LIBRARY)
        target_link_libraries(jade PRIVATE ${RT_LIBRARY})
    endif()
endif()

# ── readline (Linux only) ─────────────────────────────────────────────────────
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_library(READLINE_LIBRARY readline REQUIRED)
    find_path(READLINE_INCLUDE_DIR readline/readline.h)

    target_link_libraries(jade PRIVATE ${READLINE_LIBRARY})

    if(READLINE_INCLUDE_DIR)
        target_include_directories(jade PRIVATE ${READLINE_INCLUDE_DIR})
    endif()
endif()

# ── Pre-compiled header ───────────────────────────────────────────────────────
target_precompile_headers(jade PRIVATE
    src/include/allocator.h
    src/include/container.h
    src/include/evaluator.h
    src/include/hashmap.h
    src/include/lexer.h
    src/include/logger.h
    src/include/parser.h
    src/include/registry.h
    src/include/utils.h
)

# ── Summary ───────────────────────────────────────────────────────────────────
message(STATUS "---------------------------------------")
message(STATUS "  Project : ${PROJECT_NAME}")
message(STATUS "  Target  : jade")
message(STATUS "  Build   : ${CMAKE_BUILD_TYPE}")
message(STATUS "  System  : ${CMAKE_SYSTEM_NAME}")
message(STATUS "  C std   : C${CMAKE_C_STANDARD}")
message(STATUS "---------------------------------------")