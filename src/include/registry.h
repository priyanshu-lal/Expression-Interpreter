#pragma once

#include "hashmap.h"
#include "evaluator.h"

#include <stdbool.h>

#define MAX_VAR_LEN 16

typedef int(*CalcFn)(void);

enum VarType {
	BUILTIN_FUNCTION,
	FUNCTION,
	VARIABLE,
};

typedef struct {
	char* key;
	double value;
} Variable;

typedef struct {
	char* key;
	const char** argNames;
	CalcFn fnPtr;
	unsigned argsCount;
	bool isVariadic;
} BuiltinFunction;

typedef struct Function {
	char* key;
	const char** argsName;
	OpCode* instructions;
	bool returnTypeIsNum;
	unsigned insCount;
	unsigned argsCount;
	unsigned* indices;
	double* inputValues; // container for input values, the evaluator fills this before it calls this function
	double* constants;   // list of constant numbers (literals)
	char** varList;      // list of Variable name (char*)
	void** fnList;       // list of pointers to: Function* or BuiltinFunction* depending on instruction
} Function;

typedef struct {
	char* symbol;
	enum VarType type;
} SymbolType;

/* ----------------------------------------------------------------------------------
NOTE:
- this hashmap implementation stores a shallow copy of entries,
  it uses memcpy to copy data on its bucket
  	- memcpy copies N raw bytes to 'dest' starting from the given address 'src'
  the implementation accepts data as: entry_type* and copies sizeof(entry_type)

- g_functions and g_userFunctions stores the address of their corrosponding value type
  which points to actual data allocated using arena_allocator
  the hashmap_get on these two just returns pointer to data (which in this case is already a pointer)
    i.e return type: (entry_type**)
*/

extern hashmap* g_variables;      // entry type: Variable
extern hashmap* g_functions;      // entry type: Function*
extern hashmap* g_userFunctions;  // entry type: UserFunc*
//----------------------------------------------------------------------------------

extern hashmap* g_symbolTable;    // entry type: SymbolType

enum AngleUnit {
	DEGREE, RADIAN
};

void loadRegistry();
void unloadRegistry();

void displayFunctionProto(const Function* fn);

int stringKeyCompare(const void* a, const void* b, void* udata);
uint64_t stringKeyHash(const void* item, uint64_t seed0, uint64_t seed1);

enum AngleUnit getAngleUnit();  // implemented inside defs.c
void setAngleUnit(enum AngleUnit unit);  // implemented inside defs.c
