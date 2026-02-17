#include "container.h"
#include "registry.h"
#include "evaluator.h"
#include "logger.h"

#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <stdio.h>

extern hashmap* g_newVarDeclMap;  // defined in parser.c
extern struct timespec start, end;  // defined in main.c

NumVec* st;
bool g_showTimestamp = false;
double g_answer = NAN;

typedef struct {
	double value;
	bool isBool;
} FinalResult;

static Variable* varPtr;
static bool firstErrInFn;
static enum EvalMode s_evalMode = DIRECT;
static Vector s_accumulator;  // element type: FinalResult

void initEvaluator() {
	s_accumulator = newVector(8, sizeof(FinalResult));
}

void freeEvaluator() {
	VecClear(&s_accumulator);
}

void setEvaluationMode(enum EvalMode em) { s_evalMode = em; }
enum EvalMode getEvaluationMode(void) { return s_evalMode; }

static int evaluateDirectly(const Function*);
static int evaluateInDetail(const Function*);

const char* toBoolString(double n) {
	return n > 0.0 ? "true" : "false";
}

bool evaluate(const Function* fn) {
	VecClear(&s_accumulator);
	if (s_evalMode == DIRECT ? evaluateDirectly(fn) : evaluateInDetail(fn)) {

		clock_gettime(CLOCK_MONOTONIC, &end);
		double timeTaken = ((end.tv_sec - start.tv_sec) * 1000.0) + ((end.tv_nsec - start.tv_nsec) / 1e+6);

		for (size_t i = 0; i < s_accumulator.len; i++) {
			FinalResult res = *(FinalResult*)VecAt(&s_accumulator, i);
			if (res.isBool) printStyledText(fstring("<b>==> <g>%s\n", toBoolString(res.value)));
			else  printStyledText(fstring("<b>==> <g>%g\n", res.value));
		}
		if (s_evalMode == DIRECT && g_showTimestamp)
			printStyledText(fstring(" <c>(in <b>%g<c> ms)\n", timeTaken));
		return 1;
	}
	return 0;
}

bool resultsInBool(enum OP_CODE op) {
	return op == OP_LOGICAL_AND || op == OP_LOGICAL_NOT || op == OP_LOGICAL_OR
		|| op == OP_GT || op == OP_SM
		|| op == OP_GT_OR_EQ || op == OP_SM_OR_EQ
		|| op == OP_ISEQUAL || op == OP_NOTEQUAL;
}

static bool isEqual(double a, double b) {
	double diff = fabs(a - b);
	double abs_tolerance = 1e-12, rel_tolerance = 1e-9;
	if (diff <= abs_tolerance) return true;
	return diff <= fmax(fabs(a), fabs(b)) * rel_tolerance;
}

static int evaluateInDetail(const Function* eUnit) {
	size_t numIdx = 0, fnIdx = 0, identIdx = 0, indicesIdx = 0;
	unsigned iIdx;
	firstErrInFn = false;
	const BuiltinFunction* fn;
	double n1, n2, res = 0.0;
	char* varName;
	
	printStyledText("\n<c>[<g>~<c>] <y>Detailed Evaluation Steps:\n");
	printStyledText("  <y>╔\n");

	for (size_t i = 0; i < eUnit->insCount; i++) {
		switch (eUnit->instructions[i]) {
		case OP_CALL_BUILTIN:
			fn = eUnit->fnList[fnIdx++];

			if (fn->argsCount == 1) {
				printStyledText(fstring("  <y>║</> Call <b>%s</> (<c>%g</>)", fn->key, NumVecTop(st)));
			}
			else {
				printStyledText(fstring("  <y>║</> Call <b>%s</> (", fn->key));
				size_t startIdx;
				if (fn->isVariadic) {
					startIdx = st->len - (size_t)NumVecTop(st) - 1;
					for (size_t i = startIdx; i < st->len - 2; i++) {
						printStyledText(fstring("<c>%g</>, ", st->data[i]));
					}
					printStyledText(fstring("<c>%g</>)", st->data[st->len - 2]));
				}
				else {
					startIdx = st->len - fn->argsCount;
					for (size_t i = startIdx; i < st->len - 1; i++) {
						printStyledText(fstring("<c>%g</>, ", st->data[i]));
					}
					printStyledText(fstring("<c>%g</>)", st->data[st->len - 1]));
				}
			}

			if (fn->fnPtr() == 0) {
				printStyledTextInBox(fstring("<r>::</> function '<y>%s</>' threw an error, <r>aborting execution", fn->key));
				return false;
			}
			if (isnan(NumVecTop(st))) {
				printStyledTextInBox(fstring(
					"<r>::</> function <y>%s</> returned an unexpected value, <r>aborting execution", fn->key));
				return false;
			}
			printStyledText(fstring(" <g>-></> returned value: <g>%g\n", NumVecTop(st)));
			break;

		case OP_CALL_DEFINED:
			if (!evaluate((const Function*)(eUnit->fnList[fnIdx++]))) {
				return false;
			}
			break;

        case OP_PUSH_NUM:
			NumVecPush(st, eUnit->constants[numIdx++]);
			break;

        case OP_SET_VAR:
			varName = eUnit->varList[identIdx++];
			varPtr = (Variable*)hashmap_get(g_variables, &varName);

			if (varPtr) {
				varPtr->value = NumVecTop(st);
				printStyledText(fstring("  <y>║</> Set <b>%s</> = <c>%g</>\n", varPtr->key, NumVecTop(st)));
			}
			else {
				hashmap_delete(g_newVarDeclMap, &varName);
				hashmap_set(g_variables, &(Variable){varName, NumVecTop(st)});
				hashmap_set(g_symbolTable, &(SymbolType){varName, VARIABLE});

				printStyledText(fstring("  <y>║</> Create new var <b>%s</> = <c>%g</>\n",
					varName, NumVecTop(st)));
			}
			break;

        case OP_PUSH_VAR:
			varName = eUnit->varList[identIdx++];
			varPtr = (Variable*)hashmap_get(g_variables, &varName);

			if (isnan(varPtr->value)) {
				displayErrorMsg(fstring("Variable <b>%s</> is not initialized", varPtr->key));
				return false;
			}
			printStyledText(fstring("  <y>║</> Take variable <b>%s</> (= <c>%g</>)\n",
				varPtr->key, varPtr->value));

			NumVecPush(st, varPtr->value);
			break;

        case OP_ADD:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			printStyledText(fstring("  <y>║</> Add (<c>%g <b>+ <c>%g <b>= <g>%g</>)\n", n1, n2, n1 + n2));
			NumVecPush(st, n1 + n2);
			break;

        case OP_SUB:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			printStyledText(fstring("  <y>║</> Subtract (<c>%g <b>- <c>%g <b>= <g>%g</>)\n", n1, n2, n1 - n2));
			NumVecPush(st, n1 - n2);
			break;

        case OP_MUL:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			printStyledText(fstring("  <y>║</> Multiply (<c>%g <b>* <c>%g <b>= <g>%g</>)\n", n1, n2, n1 * n2));
			NumVecPush(st, n1 * n2);
			break;

        case OP_DIV:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			if (n2 == 0) {
				displayErrorMsg(fstring("Division by zero: (%g / %g)", n1, n2));
				return false;
			}
			printStyledText(fstring("  <y>║</> Divide (<c>%g <b>/ <c>%g <b>= <g>%g</>)\n", n1, n2, n1 / n2));
			NumVecPush(st, n1 / n2);
			break;

		case OP_INT_DIV:
			n2 = floor(NumVecPopBack(st));
			n1 = floor(NumVecPopBack(st));
			if (n2 == 0) {
				displayErrorMsg(fstring("Division by zero: (%g / %g)", n1, n2));
			}
			NumVecPush(st, floor(n1 / n2));
			printStyledText(fstring("  <y>║</> Floor divide (<c>%g <b>// <c>%g <b>= <g>%g</>)\n", 
				n1, n2, NumVecTop(st)));
			break;

        case OP_ABS:
			n1 = NumVecPopBack(st);
			NumVecPush(st, doubleAbs(n1));

			printStyledText(fstring("  <y>║</> Absolute of <c>%g</> (= <g>%g</>)\n",
				n1, NumVecTop(st)));
			break;

        case OP_MOD:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, fmod(n1, n2));
			printStyledText(fstring("  <y>║</> Mod (<c>%g <b>%% <c>%g <b>= <g>%g</>)\n", n1, n2, NumVecTop(st)));
			break;

        case OP_POWER:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, pow(n1, n2));
			printStyledText(fstring("  <y>║</> Power (<c>%g <b>^ <c>%g <b>= <g>%g</>)\n", n1, n2, NumVecTop(st)));
			break;

        case OP_FACTORIAL:
			n1 = NumVecPopBack(st);
			res = factorial(n1);
			if (isnan(res)) return false;
			NumVecPush(st, res);
			printStyledText(fstring("  <y>║</> Factorial of <c>%g</> (= <g>%g</>)\n", n1, res));
			break;

		case OP_PUSH_ARG:
			iIdx = eUnit->indices[indicesIdx++];
			printStyledText(fstring("  <y>║</> Taking input <c>%s</> (= <g>%g</>)\n",
				eUnit->argsName[iIdx], eUnit->inputValues[iIdx]));
			NumVecPush(st, eUnit->inputValues[iIdx]);
			break;

        case OP_LINE_DONE:
			if (st->len != 0) {
				g_answer = NumVecTop(st);
				enum OP_CODE lastIns = eUnit->instructions[i - 1];
				if (lastIns == OP_SET_VAR) {
					if (i + 1 == eUnit->insCount) {
						printStyledText("  <y>╚\n");
					} else {
						printStyledText("  <y>╚\n  ╔\n");
					}
					NumVecPopBack(st);
				}
				else if (resultsInBool(lastIns)) {
						VecPush(&s_accumulator, &(FinalResult) {g_answer, true});
						if (i + 1 == eUnit->insCount) {
							printStyledText(fstring("  <y>║</> Final Answer: <g><~u>%s</>\n  <y>╚\n",
								toBoolString(g_answer)));
						}
						else {
							printStyledText(fstring("  <y>║</> Final Answer: <g><~u>%s</>\n  <y>╚\n  ╔\n",
								toBoolString(g_answer)));
						}
					}
				else {
					VecPush(&s_accumulator, &(FinalResult) {g_answer, false});
					if (i + 1 == eUnit->insCount) {
						printStyledText(fstring("  <y>║</> Final Answer: <g><~u>%g</>\n  <y>╚\n",
							g_answer));
					}
					else {
						printStyledText(fstring("  <y>║</> Final Answer: <g><~u>%g</>\n  <y>╚\n  ╔\n",
							g_answer));
					}
				}
			}
			if (isnan(g_answer)) return false;
			break;

        case OP_PUSH_PREV_ANS:
			NumVecPush(st, g_answer);
			printStyledText(fstring("  <y>║</> Take previous result (= <g>%g</>)\n", g_answer));
			break;

		case OP_LOGICAL_AND:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 > 0.0 && n2 > 0.0));
			printStyledText(fstring("  <y>║</> Logical AND (<c>%s <b>&& <c>%s <b>= <g>%s</>)\n",
				toBoolString(n1), toBoolString(n2), toBoolString(NumVecTop(st))));
			break;

		case OP_LOGICAL_OR:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 > 0.0 || n2 > 0.0));
			printStyledText(fstring("  <y>║</> Logical OR (<c>%s <b>|| <c>%s <b>= <g>%s</>)\n",
				toBoolString(n1), toBoolString(n2), toBoolString(NumVecTop(st))));
			break;

		case OP_LOGICAL_NOT:
			n1 = NumVecPopBack(st);
			NumVecPush(st, n1 > 0.0 ? 0.0 : 1.0);
			printStyledText(fstring("  <y>║</> Logical NOT (<b>not <c>%s <b>= <g>%s</>)\n",
				toBoolString(n1), toBoolString(NumVecTop(st))));
			break;

		case OP_GT:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 > n2));
			printStyledText(fstring("  <y>║</> Greater than (<c>%g <b>> <c>%g <b>=> <g>%s</>)\n",
				n1, n2, toBoolString(NumVecTop(st))));
			break;

		case OP_SM:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 < n2));
			printStyledText(fstring("  <y>║</> Smaller than (<c>%g <b>< <c>%g <b>=> <g>%s</>)\n",
				n1, n2, toBoolString(NumVecTop(st))));
			break;

		case OP_SM_OR_EQ:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 <= n2));
			printStyledText(fstring("  <y>║</> Smaller or Equal (<c>%g <b><= <c>%g <b>=> <g>%s</>)\n",
				n1, n2, toBoolString(NumVecTop(st))));
			break;
			
		case OP_GT_OR_EQ:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 >= n2));
			printStyledText(fstring("  <y>║</> Greater or Equal (<c>%g <b>>= <c>%g <b>=> <g>%s</>)\n",
				n1, n2, toBoolString(NumVecTop(st))));
			break;

		case OP_ISEQUAL:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 == n2));
			printStyledText(fstring("  <y>║</> Is Equal (<c>%g <b>== <c>%g <b>=> <g>%s</>)\n",
				n1, n2, toBoolString(NumVecTop(st))));
			break;

		case OP_NOTEQUAL:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 != n2));
			printStyledText(fstring("  <y>║</> Is not Equal (<c>%g <b>!= <c>%g <b>=> <g>%s</>)\n",
				n1, n2, toBoolString(NumVecTop(st))));
			break;
		}
	}
	// return st->len == 0 ? false : !isnan(NumVecTop(st));
	return !isnan(g_answer);
}

static int evaluateDirectly(const Function* eUnit) {
	size_t numIdx = 0, fnIdx = 0, identIdx = 0, indicesIdx = 0;
	firstErrInFn = false;
	const BuiltinFunction* fn;
	const Function* userFn;
	double n1, n2, res = 0.0;
	char* varName;

	for (size_t i = 0; i < eUnit->insCount; i++) {
		switch (eUnit->instructions[i]) {
        case OP_CALL_BUILTIN:
			fn = eUnit->fnList[fnIdx++];
			if (fn->fnPtr() == 0) {
				printStyledTextInBox(fstring("<r>::</> function '<y>%s</>' threw an error, <r>aborting execution", fn->key));
				return false;
			}
			if (isnan(NumVecTop(st))) {
				printStyledTextInBox(fstring(
					"<r>::</> function <y>%s</> returned an unexpected value, <r>aborting execution", fn->key));
				return false;
			}
			break;

		case OP_CALL_DEFINED:
			userFn = (const Function*)(eUnit->fnList[fnIdx++]);
			size_t startIdx = st->len - userFn->argsCount, k = 0;
			for (size_t i = startIdx; i < st->len; i++) {
				userFn->inputValues[k++] = NumVecAt(st, i);
			}
			st->len = startIdx;
			if (!evaluateDirectly(userFn)) {
				return false;
			}
			break;

        case OP_PUSH_NUM:
			NumVecPush(st, eUnit->constants[numIdx++]);
			break;

        case OP_SET_VAR:
			varName = eUnit->varList[identIdx++];
			varPtr = (Variable*)hashmap_get(g_variables, &varName);

			if (varPtr) {
				varPtr->value = NumVecTop(st);
			}
			else {
				hashmap_delete(g_newVarDeclMap, &varName);
				hashmap_set(g_variables, &(Variable){varName, NumVecTop(st)});
				hashmap_set(g_symbolTable, &(SymbolType){varName, VARIABLE});
			}
			break;

        case OP_PUSH_VAR:
			varName = eUnit->varList[identIdx++];
			varPtr = (Variable*)hashmap_get(g_variables, &varName);

			if (isnan(varPtr->value)) {
				displayErrorMsg(fstring("Variable <b>%s</> is not initialized", varPtr->key));
				return false;
			}
			NumVecPush(st, varPtr->value);
			break;

        case OP_ADD:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, n1 + n2);
			break;

        case OP_SUB:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, n1 - n2);
			break;

        case OP_MUL:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, n1 * n2);
			break;

        case OP_DIV:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			if (n2 == 0) {
				displayErrorMsg(fstring("Division by zero: (%g / %g)", n1, n2));
				return false;
			}
			NumVecPush(st, n1 / n2);
			break;

		case OP_INT_DIV:
			n2 = floor(NumVecPopBack(st));
			n1 = floor(NumVecPopBack(st));
			if (n2 == 0) {
				displayErrorMsg(fstring("Division by zero: (%g / %g)", n1, n2));
			}
			NumVecPush(st, floor(n1 / n2));
			break;

        case OP_ABS:
			n1 = NumVecPopBack(st);
			NumVecPush(st, doubleAbs(n1));
			break;

        case OP_MOD:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, fmod(n1, n2));
			break;

        case OP_POWER:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, pow(n1, n2));
			break;

        case OP_FACTORIAL:
			n1 = NumVecPopBack(st);
			res = factorial(n1);
			if (isnan(res)) return false;
			NumVecPush(st, res);
			break;

		case OP_PUSH_ARG:
			NumVecPush(st, eUnit->inputValues[eUnit->indices[indicesIdx++]]);
			break;

        case OP_LINE_DONE:
			if (st->len != 0) {
				g_answer = NumVecTop(st);
				enum OP_CODE lastIns = eUnit->instructions[i - 1];
				if (lastIns == OP_SET_VAR) {
					NumVecPopBack(st);
				}
				else if (resultsInBool(lastIns) || (lastIns == OP_CALL_DEFINED && !userFn->returnTypeIsNum)) {
					VecPush(&s_accumulator, &(FinalResult) {g_answer, true});
				}
				else {
					VecPush(&s_accumulator, &(FinalResult) {g_answer, false});
				}
			}
			if (isnan(g_answer)) return false;
			break;

        case OP_PUSH_PREV_ANS:
			NumVecPush(st, g_answer);
			break;

		case OP_LOGICAL_AND:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 > 0.0 && n2 > 0.0));
			break;

		case OP_LOGICAL_OR:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 > 0.0 || n2 > 0.0));
			break;

		case OP_LOGICAL_NOT:
			n1 = NumVecPopBack(st);
			NumVecPush(st, n1 > 0.0 ? 0.0 : 1.0);
			break;

		case OP_GT:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 > n2));
			break;

		case OP_SM:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 < n2));
			break;

		case OP_SM_OR_EQ:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 <= n2));
			break;
			
		case OP_GT_OR_EQ:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 >= n2));
			break;

		case OP_ISEQUAL:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)isEqual(n1, n2));
			break;

		case OP_NOTEQUAL:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(!isEqual(n1, n2)));
			break;
        }
    }
	return eUnit->key ? true : !isnan(g_answer);
}
