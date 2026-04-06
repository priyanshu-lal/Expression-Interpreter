#pragma once

#include "hashmap.h"
#include "evaluator.h"

#include <stdbool.h>

#define MAX_VAR_LEN 16

typedef int(*CalcFn)(void);
typedef double(*ConversionFn)(double);

enum VarType {
	ALIAS,
	BUILTIN_FUNCTION,
	FUNCTION,
	VARIABLE,
};

typedef enum {
	UNIT_LENGTH,
	UNIT_VOLUME,
	UNIT_TEMP,
	UNIT_MASS,
	UNIT_ENERGY,
	UNIT_AREA,
	UNIT_SPEED,
	UNIT_TIME,
	UNIT_POWER,
	UNIT_PRESSURE,
	UNIT_ANGLE
} UnitType;

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

typedef struct {
	const char* key;
	const char* fullName;
	UnitType type;
	bool isPrimary;
	ConversionFn toPrimary;
	ConversionFn fromPrimary;
} Unit;

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
	Unit** convList;
} Function;

typedef struct {
	char* name;
	bool isBuiltin;
	union {
		BuiltinFunction* bFn;
		Function* fn;	
	};
} Alias;

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
extern hashmap* g_unitsTable;     // entry type: Unit

enum AngleUnit {
	DEGREE, RADIAN
};

void loadRegistry();
void unloadRegistry();

void displayFunctionProto(const Function* fn);
const char* unitAsString(UnitType);

int stringKeyCompare(const void* a, const void* b, void* udata);
uint64_t stringKeyHash(const void* item, uint64_t seed0, uint64_t seed1);

enum AngleUnit getAngleUnit();  // implemented inside defs.c
void setAngleUnit(enum AngleUnit unit);  // implemented inside defs.c
