cmake_minimum_required(VERSION 3.20)

project(ProgrammableCalc VERSION 0.8)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

set(SOURCES
	src/allocator.c
	src/container.c
	src/defs.c
	src/evaluator.c
	src/hashmap.c
	src/lexer.c
	src/logger.c
	src/main.c
	src/registry.c
	src/utils.c
)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

message(STATUS "Project Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

add_compile_options(-Wall -Wextra)
#  -pedantic

add_executable(calc ${SOURCES})

target_include_directories(calc
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

target_link_libraries(calc PRIVATE m)
