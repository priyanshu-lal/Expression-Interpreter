#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* ==================== Arena Allocator ==================== */

#define ARENA_DEFAULT_BLOCK_SIZE (64 * 1024)  /* 64KB default */
#define ARENA_MIN_BLOCK_SIZE 1024
#define ARENA_ALIGNMENT 8

typedef struct ArenaBlock {
	struct ArenaBlock* next;
	size_t size;
	size_t used;
	uint8_t data[];
} ArenaBlock;

typedef struct {
	ArenaBlock* head;
	ArenaBlock* current;
	size_t default_block_size;
	size_t total_allocated;
	size_t block_count;
} Arena;

extern Arena* g_globArena; // defined in: allocator.c

bool arena_init(Arena* arena, size_t initial_size);
void* arena_alloc_aligned(Arena* arena, size_t size, size_t alignment);
void* arena_alloc(Arena* arena, size_t size);
void* arena_calloc(Arena* arena, size_t count, size_t size);
void arena_reset(Arena* arena);
void arena_destroy(Arena* arena);
void arena_get_stats(const Arena* arena, size_t* total_allocated, size_t* total_used, size_t* block_count);
// ArenaBlock* arena_create_checkpoint(const Arena*);
// void arena_restore_checkpoint(Arena*, ArenaBlock* block);
//------------------------------------------------------------------------

/* ==================== Memory Pool ==================== */

typedef struct PoolBlock {
	struct PoolBlock* next;
} PoolBlock;

typedef struct PoolChunk {
	void* memory;
	size_t capacity;
	struct PoolChunk* next;
} PoolChunk;

/* Main Pool Allocator Structure */
typedef struct {
	size_t block_size;
	size_t initial_capacity;
	size_t growth_factor;
	size_t total_blocks;
	size_t used_blocks;
	PoolBlock* free_list;
	PoolChunk* chunks;
} PoolAllocator;

extern PoolAllocator* g_s8Pool;  // defined in: allocator.c
extern PoolAllocator* g_s16Pool; // defined in: allocator.c

PoolAllocator* pool_create(size_t block_size, size_t initial_capacity);
void* pool_alloc(PoolAllocator* pool);
void pool_free(PoolAllocator* pool, void* ptr);
void pool_stats(PoolAllocator* pool);
void pool_destroy(PoolAllocator* pool);

//------------------------------------------------------------------------
void initAllocators();
void freeAllocators();

char* str_from_pool(const char*);
void free_str_from_pool(char* str);
//------------------------------------------------------------------------
