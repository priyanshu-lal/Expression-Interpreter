#pragma once

#include <stdbool.h>
#include "registry.h"
#include "lexer.h"

struct FuncList {  // this struct is allocated by a fixed 16 bytes pool allocator!
	char* fnName;
	struct FuncList* next;
};

typedef struct {
	const char* name;
	struct FuncList* head;
	struct FuncList* tail;
} VarDependecies;

extern hashmap* g_refEntries;

void initParser();
void freeParser();
Function* parse(Token* t, int startIdx, size_t len);
void freeLeftOutStrings(bool isCalledDuringEval);
