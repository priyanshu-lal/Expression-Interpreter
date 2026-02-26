#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "allocator.h"

Arena* g_globArena;
PoolAllocator* g_s8Pool;
PoolAllocator* g_s16Pool;
PoolAllocator* g_s4Pool;

static inline size_t align_forward(size_t value, size_t alignment) {
	assert((alignment & (alignment - 1)) == 0); /* Power of 2 check */
	return (value + alignment - 1) & ~(alignment - 1);
}

static inline void* align_pointer(void* ptr, size_t alignment) {
	uintptr_t addr = (uintptr_t)ptr;
	uintptr_t aligned = align_forward(addr, alignment);
	return (void*)aligned;
}

static ArenaBlock* arena_alloc_block(size_t min_size, size_t default_size) {
	size_t block_size = (min_size > default_size) ? min_size : default_size;
	block_size = align_forward(block_size, ARENA_ALIGNMENT);
	size_t total_size = sizeof(ArenaBlock) + block_size;
	ArenaBlock* block = (ArenaBlock*)malloc(total_size);
	
	if (!block) return NULL;
	block->next = NULL;
	block->size = block_size;
	block->used = 0;
	return block;
}

bool arena_init(Arena* arena, size_t initial_size) {
	if (!arena)return false;
	
	arena->default_block_size = (initial_size > 0) ? 
		align_forward(initial_size, ARENA_ALIGNMENT) : ARENA_DEFAULT_BLOCK_SIZE;
	
	if (arena->default_block_size < ARENA_MIN_BLOCK_SIZE) {
		arena->default_block_size = ARENA_MIN_BLOCK_SIZE;
	}
	
	arena->head = arena_alloc_block(0, arena->default_block_size);
	if (!arena->head) return false;
	
	arena->current = arena->head;
	arena->total_allocated = arena->head->size + sizeof(ArenaBlock);
	arena->block_count = 1;
	return true;
}

void* arena_alloc_aligned(Arena* arena, size_t size, size_t alignment) {
	if (!arena->current || size == 0) {
		return NULL;
	}
	
	if (alignment < ARENA_ALIGNMENT) {
		alignment = ARENA_ALIGNMENT;
	}
	
	ArenaBlock* block = arena->current;
	
	void* current_ptr = block->data + block->used;
	void* aligned_ptr = align_pointer(current_ptr, alignment);
	size_t padding = (uint8_t*)aligned_ptr - (uint8_t*)current_ptr;
	size_t total_needed = padding + size;
	
	if (block->used + total_needed <= block->size) {
		block->used += total_needed;
		return aligned_ptr;
	}
	
	size_t min_block_size = size + alignment;
	ArenaBlock* new_block = arena_alloc_block(min_block_size, arena->default_block_size);
	if (!new_block) return NULL;

	block->next = new_block;
	arena->current = new_block;
	arena->total_allocated += new_block->size + sizeof(ArenaBlock);
	arena->block_count++;

	current_ptr = new_block->data;
	aligned_ptr = align_pointer(current_ptr, alignment);
	padding = (uint8_t*)aligned_ptr - (uint8_t*)current_ptr;
	new_block->used = padding + size;
	return aligned_ptr;
}

void* arena_alloc(Arena* arena, size_t size) {
	return arena_alloc_aligned(arena, size, ARENA_ALIGNMENT);
}

void* arena_calloc(Arena* arena, size_t count, size_t size) {
	if (count == 0 || size == 0) return NULL;
	if (count > SIZE_MAX / size) return NULL;
	size_t total_size = count * size;
	void* ptr = arena_alloc(arena, total_size);
	if (ptr) memset(ptr, 0, total_size);
	return ptr;
}

void arena_reset(Arena* arena) {
	if (!arena) return;
	ArenaBlock* block = arena->head;
	while (block) {
		block->used = 0;
		block = block->next;
	}
	arena->current = arena->head;
}

void arena_destroy(Arena* arena) {
	if (!arena) return;
	ArenaBlock* block = arena->head;
	while (block) {
		ArenaBlock* next = block->next;
		free(block);
		block = next;
	}
	
	arena->head = NULL;
	arena->current = NULL;
	arena->total_allocated = 0;
	arena->block_count = 0;
}

void arena_get_stats(const Arena* arena, size_t* total_allocated, size_t* total_used, size_t* block_count) {
	if (!arena) {
		if (total_allocated) *total_allocated = 0;
		if (total_used) *total_used = 0;
		if (block_count) *block_count = 0;
		return;
	}
	
	if (total_allocated) {
		*total_allocated = arena->total_allocated;
	}
	
	if (total_used) {
		size_t used = 0;
		ArenaBlock* block = arena->head;
		while (block) {
			used += block->used;
			block = block->next;
		}
		*total_used = used;
	}
	
	if (block_count) {
		*block_count = arena->block_count;
	}
}

//------------------------------------------------------------------------
/* ==================== Memory Pool ==================== */

static bool pool_add_chunk(PoolAllocator* pool, size_t capacity) {
	PoolChunk* chunk = (PoolChunk*)malloc(sizeof(PoolChunk));
	if (!chunk) return false;

	chunk->memory = malloc(pool->block_size * capacity);
	if (!chunk->memory) {
		free(chunk);
		return false;
	}
	
	chunk->capacity = capacity;
	chunk->next = pool->chunks;
	pool->chunks = chunk;

	char* block_ptr = (char*)chunk->memory;
	for (size_t i = 0; i < capacity; i++) {
		PoolBlock* block = (PoolBlock*)block_ptr;
		block->next = pool->free_list;
		pool->free_list = block;
		block_ptr += pool->block_size;
	}
	
	pool->total_blocks += capacity;
	return true;
}

PoolAllocator* pool_create(size_t block_size, size_t initial_capacity) {
	if (block_size < sizeof(PoolBlock)) {
		block_size = sizeof(PoolBlock);
	}
	
	/* Align block size to pointer size for better performance */
	size_t align = sizeof(void*);
	block_size = (block_size + align - 1) & ~(align - 1);
	PoolAllocator* pool = (PoolAllocator*)malloc(sizeof(PoolAllocator));
	if (!pool) return NULL;
	
	pool->block_size = block_size;
	pool->initial_capacity = initial_capacity > 0 ? initial_capacity : 64;
	pool->growth_factor = 2;
	pool->total_blocks = 0;
	pool->used_blocks = 0;
	pool->free_list = NULL;
	pool->chunks = NULL;
	
	if (!pool_add_chunk(pool, pool->initial_capacity)) {
		free(pool);
		return NULL;
	}
	
	return pool;
}

/* Allocate a block from the pool - O(1) operation */
void* pool_alloc(PoolAllocator* pool) {
	if (!pool) return NULL;
	
	if (!pool->free_list) {
		size_t new_capacity = pool->chunks->capacity * pool->growth_factor;
		if (!pool_add_chunk(pool, new_capacity)) {
			return NULL;  /* Allocation failed */
		}
	}
	
	/* Pop from free list - O(1) */
	PoolBlock* block = pool->free_list;
	pool->free_list = block->next;
	pool->used_blocks++;
	
	// Zero out the block for safety
	// memset(block, 0, pool->block_size);
	return (void*)block;
}

/* Free a block back to the pool - O(1) operation */
void pool_free(PoolAllocator* pool, void* ptr) {
	if (!pool || !ptr) return;
	PoolBlock* block = (PoolBlock*)ptr;
	block->next = pool->free_list;
	pool->free_list = block;
	pool->used_blocks--;
}

void pool_stats(PoolAllocator* pool) {
	if (!pool) return;
	size_t num_chunks = 0;
	PoolChunk* chunk = pool->chunks;
	while (chunk) {
		num_chunks++;
		chunk = chunk->next;
	}
	printf("Pool Statistics:\n");
	printf("  Block size: %zu bytes\n", pool->block_size);
	printf("  Total blocks: %zu\n", pool->total_blocks);
	printf("  Used blocks: %zu\n", pool->used_blocks);
	printf("  Free blocks: %zu\n", pool->total_blocks - pool->used_blocks);
	printf("  Number of chunks: %zu\n", num_chunks);
	printf("  Memory usage: %.2f KB\n", 
		(pool->total_blocks * pool->block_size) / 1024.0);
	printf("  Utilization: %.2f%%\n", 
		(pool->used_blocks * 100.0) / pool->total_blocks);
}

void pool_destroy(PoolAllocator* pool) {
	if (!pool) return;

	PoolChunk* chunk = pool->chunks;
	while (chunk) {
		PoolChunk* next = chunk->next;
		free(chunk->memory);
		free(chunk);
		chunk = next;
	}
	free(pool);
}

//------------------------------------------------------------------------

void initAllocators() {
	g_globArena = (Arena*)malloc(sizeof(Arena));
	arena_init(g_globArena, 4096);
	g_s8Pool = pool_create(sizeof(char) * 8, 128);
	g_s4Pool = pool_create(sizeof(char) * 8, 128);
	g_s16Pool = pool_create(sizeof(char) * 16, 128);
}

void freeAllocators() {
	arena_destroy(g_globArena);
	free(g_globArena);
	pool_destroy(g_s8Pool);
	pool_destroy(g_s4Pool);
	pool_destroy(g_s16Pool);
}

char* str_from_pool(const char* str) {
	const size_t len = strlen(str);
	char* mem;
	if (len < 4) {
		mem = pool_alloc(g_s4Pool);
	}
	else if (len < 8) {
		mem = pool_alloc(g_s8Pool);
	}
	else if (len < 16) {
		mem = pool_alloc(g_s16Pool);
	}
	else {
		printf("trying to allocate string \"%s\" larger than 16 characters", str);
		abort();
		return NULL;
	}
	memcpy(mem, str, len);
	mem[len] = '\0';
	return mem;
}

void free_str_from_pool(char* str) {
	size_t len = strlen(str);
	if (len < 4) {
		pool_free(g_s4Pool, str);
	}
	else if (len < 8) {
		pool_free(g_s8Pool, str);
	}
	else if (len < 16) {
		pool_free(g_s16Pool, str);
	}
}