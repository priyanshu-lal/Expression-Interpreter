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
double g_answer = NAN;

static NumVec* st;
static Variable* varPtr;
static bool firstErrInFn;
static Vector s_accumulator;  // element type: FinalResult

void initEvaluator() {
	s_accumulator = newVector(8, sizeof(FinalResult));
}

void freeEvaluator() {
	VecClear(&s_accumulator);
}

void clearAccumulator() { VecClear(&s_accumulator); }
Vector* getAccumulator() { return &s_accumulator; }

static bool evaluateDirectly(const Function*);
static bool evaluateInDetail(const Function*, int);

const char* toBoolString(double n) {
	return n > 0.0 ? "true" : "false";
}

bool evaluate(const Function* fn, EvalMode mode) {
	VecClear(&s_accumulator);
	NumVecClear(st);
	return mode == DIRECT ? evaluateDirectly(fn) : evaluateInDetail(fn, 1);
}

bool resultsInBool(OpCode op) {
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

static void logDetail(int indentLevel, const char* msg) {
	changeTextColor(COLOR_CYAN);
	if (indentLevel == 1) {
		putchar(' ');
	}
	else {
		for (int i = 1; i <= indentLevel; i++) {
			putchar(' ');
			if (i != indentLevel) 
				printf("│");
		}
	}
	printStyledText(msg);
}

static inline void startStackTrace(const Function* fn) {
	if (!fn->key) return;
	printStyledTextInBox(fstring("<r>::</> An error occured inside function <y>%s</>, <r>aborting execution</>", fn->key));
	// putchar('\n');
	// displayErrorMsg(fstring("An error occured inside function <y>%s\n", fn->key));
	printStyledText(fstring("\n<y>[<g>~<y>] <b>Stack Trace:</>\n    <g>*</>   <y>%s", fn->key));
}

static void logUserFnHeader(const Function* userFn, int indent) {
	logDetail(indent, "<c>│\n");
	logDetail(indent + 1, fstring("<c>╭</> <<y>%s</>(", userFn->key));
	for (unsigned i = 0; i < userFn->argsCount - 1; i++) {
		printStyledText(fstring("<c>%s </>= <g>%g</>, ", userFn->argsName[i], userFn->inputValues[i]));
	}
	printStyledText(fstring("<c>%s </>= <g>%g</>)>\n",
		userFn->argsName[userFn->argsCount - 1], userFn->inputValues[userFn->argsCount - 1]));
	logDetail(indent + 1, "<c>│\n");
}

static bool evaluateInDetail(const Function* eUnit, int indent) {
	size_t numIdx = 0, fnIdx = 0, identIdx = 0, indicesIdx = 0;
	size_t startIdx;
	unsigned iIdx;
	const BuiltinFunction* fn;
	const Function* userFn = NULL;
	firstErrInFn = false;
	enum OP_CODE oc;
	double n1, n2, res = 0.0;
	char* varName;
	
	if (eUnit->key == NULL) {
		// printStyledText("\n<c>[<g>~<c>] <y>Detailed Evaluation Steps:\n");
		printStyledText(" <c>╭─</> <<y>~</>>\n");
	}

	for (size_t i = 0; i < eUnit->insCount; i++) {
		oc = (enum OP_CODE)eUnit->instructions[i];
		switch (oc) {
		case OP_CALL_BUILTIN:
			fn = eUnit->fnList[fnIdx++];

			if (fn->argsCount == 1) {
				logDetail(indent, fstring("<c>│</> Call <b>%s</> (<c>%g</>)", fn->key, NumVecTop(st)));
			}
			else {
				logDetail(indent, fstring("<c>│</> Call <b>%s</> (", fn->key));
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
				startStackTrace(eUnit);
				return false;
			}
			if (isnan(NumVecTop(st))) {
				printStyledTextInBox(fstring(
					"<r>::</> function <y>%s</> returned an unexpected value, <r>aborting execution", fn->key));
				startStackTrace(eUnit);
				return false;
			}
			printStyledText(fstring(" <g>-></> returned value: <g>%g\n", NumVecTop(st)));
			break;

		case OP_CALL_DEFINED:
			userFn = (const Function*)(eUnit->fnList[fnIdx++]);
			startIdx = st->len - userFn->argsCount;
			size_t k = 0;
			for (size_t i = startIdx; i < st->len; i++) {
				userFn->inputValues[k++] = NumVecAt(st, i);
			}
			st->len = startIdx;
			logUserFnHeader(userFn, indent);
			if (!evaluateInDetail(userFn, indent + 1)) {
				if (eUnit->key)
					printStyledText(fstring(" <b><-- <y>%s</>", eUnit->key));
				else
					printStyledText(" <b><-- <g>main\n");
				return false;
			}

			logDetail(indent + 1, "<c>│\n");
			if (userFn->returnTypeIsNum) {
				logDetail(indent + 1, fstring("<c>╰──> <b>returns: <c>%g\n", NumVecTop(st)));
			}
			else {
				logDetail(indent + 1, fstring("<c>╰──> <b>returns: <c>%s\n", toBoolString(NumVecTop(st))));
			}
			logDetail(indent, "<c>│\n");
			break;

        case OP_PUSH_NUM:
			NumVecPush(st, eUnit->constants[numIdx++]);
			break;

        case OP_SET_VAR:
			varName = eUnit->varList[identIdx++];
			varPtr = (Variable*)hashmap_get(g_variables, &varName);

			if (varPtr) {
				varPtr->value = NumVecTop(st);
				logDetail(indent, fstring("<c>│</> Set <b>%s</> = <c>%g</>\n", varPtr->key, NumVecTop(st)));
			}
			else {
				hashmap_delete(g_newVarDeclMap, &varName);
				hashmap_set(g_variables, &(Variable){varName, NumVecTop(st)});
				hashmap_set(g_symbolTable, &(SymbolType){varName, VARIABLE});

				logDetail(indent, fstring("<c>│</> Create new var <b>%s</> = <c>%g</>\n",
					varName, NumVecTop(st)));
			}
			break;

        case OP_PUSH_VAR:
			varName = eUnit->varList[identIdx++];
			varPtr = (Variable*)hashmap_get(g_variables, &varName);

			if (isnan(varPtr->value)) {
				displayErrorMsg(fstring("Variable <b>%s</> is not initialized", varPtr->key));
				startStackTrace(eUnit);
				return false;
			}
			logDetail(indent, fstring("<c>│</> Take variable <b>%s</> (= <c>%g</>)\n",
				varPtr->key, varPtr->value));

			NumVecPush(st, varPtr->value);
			break;

        case OP_ADD:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			logDetail(indent, fstring("<c>│</> Add (<c>%g <b>+ <c>%g <b>= <g>%g</>)\n", n1, n2, n1 + n2));
			NumVecPush(st, n1 + n2);
			break;

        case OP_SUB:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			logDetail(indent, fstring("<c>│</> Subtract (<c>%g <b>- <c>%g <b>= <g>%g</>)\n", n1, n2, n1 - n2));
			NumVecPush(st, n1 - n2);
			break;

        case OP_MUL:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			logDetail(indent, fstring("<c>│</> Multiply (<c>%g <b>* <c>%g <b>= <g>%g</>)\n", n1, n2, n1 * n2));
			NumVecPush(st, n1 * n2);
			break;

        case OP_DIV:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			if (n2 == 0) {
				displayErrorMsg(fstring("Division by zero: (%g / %g)", n1, n2));
				startStackTrace(eUnit);
				return false;
			}
			logDetail(indent, fstring("<c>│</> Divide (<c>%g <b>/ <c>%g <b>= <g>%g</>)\n", n1, n2, n1 / n2));
			NumVecPush(st, n1 / n2);
			break;

		case OP_INT_DIV:
			n2 = floor(NumVecPopBack(st));
			n1 = floor(NumVecPopBack(st));
			if (n2 == 0) {
				displayErrorMsg(fstring("Division by zero: (%g // %g)", n1, n2));
				startStackTrace(eUnit);
				return false;
			}
			NumVecPush(st, floor(n1 / n2));
			logDetail(indent, fstring("<c>│</> Floor divide (<c>%g <b>// <c>%g <b>= <g>%g</>)\n", 
				n1, n2, NumVecTop(st)));
			break;

        case OP_ABS:
			n1 = NumVecPopBack(st);
			NumVecPush(st, doubleAbs(n1));

			logDetail(indent, fstring("<c>│</> Absolute of <c>%g</> (= <g>%g</>)\n",
				n1, NumVecTop(st)));
			break;

        case OP_MOD:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, fmod(n1, n2));
			logDetail(indent, fstring("<c>│</> Mod (<c>%g <b>%% <c>%g <b>= <g>%g</>)\n", n1, n2, NumVecTop(st)));
			break;

        case OP_POWER:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			if (n1 < 0.0 && floor(n2) != n2) {
				displayErrorMsg(fstring("While solving: (<c>%g</> ^ <c>%g</>), expected a positive value", n1, n2));
				startStackTrace(eUnit);
				return false;
			}
			NumVecPush(st, pow(n1, n2));
			logDetail(indent, fstring("<c>│</> Power (<c>%g <b>^ <c>%g <b>= <g>%g</>)\n", n1, n2, NumVecTop(st)));
			break;

        case OP_FACTORIAL:
			n1 = NumVecPopBack(st);
			res = factorial(n1);
			if (isnan(res)) {
				startStackTrace(eUnit);
				return false;
			}
			NumVecPush(st, res);
			logDetail(indent, fstring("<c>│</> Factorial of <c>%g</> (= <g>%g</>)\n", n1, res));
			break;

		case OP_PUSH_ARG:
			iIdx = eUnit->indices[indicesIdx++];
			logDetail(indent, fstring("<c>│</> Taking input <c>%s</> (= <g>%g</>)\n",
				eUnit->argsName[iIdx], eUnit->inputValues[iIdx]));
			NumVecPush(st, eUnit->inputValues[iIdx]);
			break;

        case OP_LINE_DONE:
			if (st->len != 0) {
				g_answer = NumVecTop(st);
				OpCode lastIns = eUnit->instructions[i - 1];
				if (lastIns == OP_SET_VAR) {
					logDetail(indent, "<c>╰─\n");
					if (i + 1 != eUnit->insCount) {
						logDetail(indent, "<c>╭─</> <<y>~</>>\n");
					} else {
						putchar('\n');
					}
					NumVecPopBack(st);
				}
				else if (resultsInBool(lastIns) || (lastIns == OP_CALL_DEFINED && userFn && !userFn->returnTypeIsNum)) {
					VecPush(&s_accumulator, &(FinalResult) {g_answer, true});
					logDetail(indent, fstring("<c>╰──> %s\n", toBoolString(g_answer)));
					if (i + 1 != eUnit->insCount) {
						logDetail(indent, "<c>╭─</> <<y>~</>>\n");
					} else {
						putchar('\n');
					}
				}
				else {
					VecPush(&s_accumulator, &(FinalResult) {g_answer, false});
					logDetail(indent, fstring("<c>╰──> %g\n", g_answer));
					if (i + 1 != eUnit->insCount) {
						logDetail(indent, "<c>╭─</> <<y>~</>>\n");
					} else {
						putchar('\n');
					}
				}
			}
			if (isnan(g_answer)) return false;
			break;

        case OP_PUSH_PREV_ANS:
			NumVecPush(st, g_answer);
			logDetail(indent, fstring("<c>│</> Take previous result (= <g>%g</>)\n", g_answer));
			break;

		case OP_LOGICAL_AND:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 > 0.0 && n2 > 0.0));
			logDetail(indent, fstring("<c>│</> Logical AND (<c>%s <b>&& <c>%s <b>= <g>%s</>)\n",
				toBoolString(n1), toBoolString(n2), toBoolString(NumVecTop(st))));
			break;

		case OP_LOGICAL_OR:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 > 0.0 || n2 > 0.0));
			logDetail(indent, fstring("<c>│</> Logical OR (<c>%s <b>|| <c>%s <b>= <g>%s</>)\n",
				toBoolString(n1), toBoolString(n2), toBoolString(NumVecTop(st))));
			break;

		case OP_LOGICAL_NOT:
			n1 = NumVecPopBack(st);
			NumVecPush(st, n1 > 0.0 ? 0.0 : 1.0);
			logDetail(indent, fstring("<c>│</> Logical NOT (<b>not <c>%s <b>= <g>%s</>)\n",
				toBoolString(n1), toBoolString(NumVecTop(st))));
			break;

		case OP_GT:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 > n2));
			logDetail(indent, fstring("<c>│</> Greater than (<c>%g <b>> <c>%g <b>=> <g>%s</>)\n",
				n1, n2, toBoolString(NumVecTop(st))));
			break;

		case OP_SM:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 < n2));
			logDetail(indent, fstring("<c>│</> Smaller than (<c>%g <b>< <c>%g <b>=> <g>%s</>)\n",
				n1, n2, toBoolString(NumVecTop(st))));
			break;

		case OP_SM_OR_EQ:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 <= n2));
			logDetail(indent, fstring("<c>│</> Smaller or Equal (<c>%g <b><= <c>%g <b>=> <g>%s</>)\n",
				n1, n2, toBoolString(NumVecTop(st))));
			break;
			
		case OP_GT_OR_EQ:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 >= n2));
			logDetail(indent, fstring("<c>│</> Greater or Equal (<c>%g <b>>= <c>%g <b>=> <g>%s</>)\n",
				n1, n2, toBoolString(NumVecTop(st))));
			break;

		case OP_ISEQUAL:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 == n2));
			logDetail(indent, fstring("<c>│</> Is Equal (<c>%g <b>== <c>%g <b>=> <g>%s</>)\n",
				n1, n2, toBoolString(NumVecTop(st))));
			break;

		case OP_NOTEQUAL:
			n2 = NumVecPopBack(st);
			n1 = NumVecPopBack(st);
			NumVecPush(st, (double)(n1 != n2));
			logDetail(indent, fstring("<c>│</> Is not Equal (<c>%g <b>!= <c>%g <b>=> <g>%s</>)\n",
				n1, n2, toBoolString(NumVecTop(st))));
			break;
		}
	}
	// return st->len == 0 ? false : !isnan(NumVecTop(st));
	if (isnan(NumVecTop(st))) {
		startStackTrace(eUnit);
		return false;
	}
	return true;
}

static bool evaluateDirectly(const Function* eUnit) {
	size_t numIdx = 0, fnIdx = 0, identIdx = 0, indicesIdx = 0;
	firstErrInFn = false;
	const BuiltinFunction* fn;
	const Function* userFn = NULL;
	enum OP_CODE oc;
	double n1, n2, res = 0.0;
	char* varName;

	for (size_t i = 0; i < eUnit->insCount; i++) {
		oc = (enum OP_CODE)eUnit->instructions[i];
		switch (oc) {
        case OP_CALL_BUILTIN:
			fn = eUnit->fnList[fnIdx++];
			if (fn->fnPtr() == 0) {
				printStyledTextInBox(fstring("<r>::</> Builtin function '<y>%s</>' threw an error, <r>aborting execution", fn->key));
				startStackTrace(eUnit);
				return false;
			}
			if (isnan(NumVecTop(st))) {
				printStyledTextInBox(fstring(
					"<r>::</> function <y>%s</> returned an unexpected value, <r>aborting execution", fn->key));
				startStackTrace(eUnit);
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
				if (eUnit->key)
					printStyledText(fstring(" <b><-- <y>%s</>", eUnit->key));
				else
					printStyledText(" <b><-- <g>main\n");
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
				startStackTrace(eUnit);
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
				startStackTrace(eUnit);
				return false;
			}
			NumVecPush(st, n1 / n2);
			break;

		case OP_INT_DIV:
			n2 = floor(NumVecPopBack(st));
			n1 = floor(NumVecPopBack(st));
			if (n2 == 0) {
				displayErrorMsg(fstring("Division by zero: (%g // %g)", n1, n2));
				startStackTrace(eUnit);
				return false;
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
			if (n1 < 0.0 && floor(n2) != n2) {
				displayErrorMsg(fstring("While solving: (<c>%g</> ^ <c>%g</>), expected a positive value", n1, n2));
				startStackTrace(eUnit);
				return false;
			}
			NumVecPush(st, pow(n1, n2));
			break;

        case OP_FACTORIAL:
			n1 = NumVecPopBack(st);
			res = factorial(n1);
			if (isnan(res)) {
				startStackTrace(eUnit);
				return false;
			}
			NumVecPush(st, res);
			break;

		case OP_PUSH_ARG:
			NumVecPush(st, eUnit->inputValues[eUnit->indices[indicesIdx++]]);
			break;

        case OP_LINE_DONE:
			if (st->len != 0) {
				g_answer = NumVecTop(st);
				OpCode lastIns = eUnit->instructions[i - 1];
				if (lastIns == OP_SET_VAR) {
					NumVecPopBack(st);
				}
				else if (resultsInBool(lastIns) || (lastIns == OP_CALL_DEFINED && userFn && !userFn->returnTypeIsNum)) {
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
	if (isnan(NumVecTop(st))) {
		startStackTrace(eUnit);
		return false;
	}
	return true;
}
