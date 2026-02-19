#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "container.h"

struct Function;

typedef uint8_t OpCode;

enum {
	OP_CALL_BUILTIN,
	OP_CALL_DEFINED,
	OP_PUSH_NUM,
	OP_SET_VAR,
	OP_PUSH_VAR,
	OP_ADD,
	OP_LOGICAL_AND,
	OP_LOGICAL_OR,
	OP_LOGICAL_NOT,
	OP_PUSH_ARG,
	OP_GT,
	OP_SM,
	OP_SM_OR_EQ,
	OP_GT_OR_EQ,
	OP_ISEQUAL,
	OP_NOTEQUAL,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_INT_DIV,
	OP_ABS,
	OP_MOD,
	OP_POWER,
	OP_FACTORIAL,
	OP_LINE_DONE,
	OP_PUSH_PREV_ANS,
};

enum EvalMode {
	DIRECT,
	DETAILED
};

extern NumVec* st;  // defined in evaluator.c

void initEvaluator();
void freeEvaluator();

const char* toBoolString(double n);
bool resultsInBool(OpCode op);

void setEvaluationMode(enum EvalMode);
enum EvalMode getEvaluationMode(void);

bool evaluate(const struct Function*);
struct NumVec* getExecStack();
