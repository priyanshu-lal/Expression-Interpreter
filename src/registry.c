#include "registry.h"
#include "hashmap.h"
#include "container.h"
#include "allocator.h"
#include "logger.h"

#include <stdlib.h>
#include <stdio.h>

hashmap* g_symbolTable;
hashmap* g_variables;
hashmap* g_functions;
hashmap* g_userFunctions;

extern void loadSymbols();
static int ptrStringKeyCompare(const void* a, const void* b, void* udata);
static uint64_t ptrStringKeyHash(const void* item, uint64_t seed0, uint64_t seed1);

void loadRegistry() {
	st = arena_alloc(g_globArena, sizeof(NumVec));
	*st = newNumVec(256);
	g_variables = hashmap_new(sizeof(Variable), 128, 0xABC123456789ULL, 0xDeadFacadeULL,
		stringKeyHash, stringKeyCompare, NULL, NULL);

	g_userFunctions = hashmap_new(sizeof(Function*), 64, 0, 0,
		ptrStringKeyHash, ptrStringKeyCompare, NULL, NULL);

	g_functions = hashmap_new(sizeof(BuiltinFunction*), 256, 0xBadBeeFadedDeadULL, 0xBadBeeDeadBee,
		ptrStringKeyHash, ptrStringKeyCompare, NULL, NULL);

	g_symbolTable = hashmap_new(sizeof(SymbolType), 512, 0xDead3FacadeULL, 0xAceCafeeeULL,
		stringKeyHash, stringKeyCompare, NULL, NULL);

	loadSymbols();
}

void unloadRegistry() {
	hashmap_free(g_variables);
	hashmap_free(g_functions);
	hashmap_free(g_userFunctions);
	hashmap_free(g_symbolTable);
}

// ---------- Hashmap callback functions ----------
static int ptrStringKeyCompare(const void* a, const void* b, void* udata) {
	return strcmp(**(const char***)a, **(const char***)b);
}

static uint64_t ptrStringKeyHash(const void* item, uint64_t seed0, uint64_t seed1) {
	const char* key = **(const char***)item;
	return hashmap_xxhash3(key, strlen(key), seed0, seed1);
}

int stringKeyCompare(const void* a, const void* b, void* udata) {
	return strcmp(*(const char**)a, *(const char**)b);
}

uint64_t stringKeyHash(const void* item, uint64_t seed0, uint64_t seed1) {
	const char* key = *(const char**)item;
	return hashmap_xxhash3(key, strlen(key), seed0, seed1);
}

void displayFunctionProto(const Function* fn) {
	changeTextColor(COLOR_BLUE);
	printf("%s", fn->key);
	resetTextAttribute();
	putchar('(');
	if (fn->argsCount == 0) {
		putchar(')');
		return;
	}
	for (unsigned i = 0; i < fn->argsCount - 1; i++) {
		printStyledText(fstring("<c>%s</>, ", fn->argsName[i]));
	}
	printStyledText(fstring("<c>%s</>)", fn->argsName[fn->argsCount - 1]));
}

// static void freeFunctionCallback(void* ptr) {
// 	free_str_from_pool(**(char***)ptr);
// }

// int stringArKeyCompare(const void* a, const void* b, void* udata) {
// 	return strcmp((char*)a, *(const char**)b);
// }

// uint64_t stringArKeyHash(const void* item, uint64_t seed0, uint64_t seed1) {
//     char* key = (char*)item;
//     return hashmap_sip(key, strlen(key), seed0, seed1);
// }
