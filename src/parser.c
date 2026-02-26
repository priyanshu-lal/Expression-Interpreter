#include "allocator.h"
#include "container.h"
#include "logger.h"
#include "evaluator.h"
#include "hashmap.h"
#include "parser.h"

#include <stdlib.h>
#include <math.h>

#if defined(__GNUC__) || defined(__clang__)
	#define FORCE_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
	#define FORCE_INLINE __forceinline
#else
	#define FORCE_INLINE inline
#endif

#define AS_FN_CALL_PTR(PTR) ((FnCallEntry*)(PTR))
#define GET_FN_NAME(FN_PTR) (*(const char**)(FN_PTR))
#define AS_TK_TYPE(PTR)     (*(TokenKind*)(PTR))
#define AS_OP_CODE(PTR)     (*(OpCode*)(PTR))

extern double g_answer;  // defined in evaluator.c

// ======================= Static (local) Globals =======================

// containers:
hashmap* g_newVarDeclMap;       // element type: VarDeclEntry
hashmap* g_refEntries;          // element type: VarDependecies
static hashmap* s_tmpVarRecord; // element type: char* (no ownership)
static PtrVec* s_fnList;        // element type: BuiltinFunction* or Function* (no ownership) depending on context
static PtrVec* s_varList;       // element type: char* (no ownership)
static Vector* s_opCode;        // element type: OP_CODE
static Vector* s_indices;       // element type: unsigned int
static hashmap* s_fnArgsEntry;  // element type: ArgEntry
static PtrVec* s_argNamePtr;    // element type: char* (no ownership)
static NumVec* s_constants;
//------------------------

// stacks:
static Vector* s_operatorStack;    // element type: TokenKind
static Vector* s_fnStack;          // element type: FnCallEntry
static Vector* s_singleArgFnStack; // element type: FnCallEntry
static PtrVec* s_varDeclStack;     // element type: char* (no ownership)
//------------------------

static Function* s_intermediateFn;
static Token* tokens;  // contains the list of tokens
static Token currentTk, nextTk, prevTk;
static char** identifier;
static char* s_strPtr;
static char* s_currentFnName;

static short s_absExprParaCount[128];  // parentheses matching inside absolute expression of depth 'i'
static bool s_expectExpr, s_insideFnBody;
static int s_pipeDepth, s_bracketCount, s_exprCount;
static int LEN, idx;

// ======================= Types =======================
typedef struct {
	const Token tk;
	int argsRequired;
	int argsFound;
	int innerParenCount;
	bool isBuiltin;
	bool isVariadic;
	void* fnPtr;  // can point to BuiltinFunction or Function
} FnCallEntry;

typedef struct {
	char* name;
	bool isInitialized;
} VarDeclEntry;

typedef struct {
	const char* argName;
	unsigned pos;
} ArgEntry;

typedef enum {
	OK,
	ERROR,
	FATAL_ERROR,
} ParseResult;

//------------------------------------------------------------------------------------------
// ======================= Functions =======================

void initParser() {
	s_fnList = arena_alloc(g_globArena, sizeof(PtrVec));
	s_varList = arena_alloc(g_globArena, sizeof(PtrVec));
	s_opCode = arena_alloc(g_globArena, sizeof(Vector));
	s_indices = arena_alloc(g_globArena, sizeof(Vector));
	s_constants = arena_alloc(g_globArena, sizeof(NumVec));
	s_argNamePtr = arena_alloc(g_globArena, sizeof(PtrVec));
	
	*s_fnList = newPtrVec(64);
	*s_varList = newPtrVec(64);
	*s_opCode = newVector(64, sizeof(OpCode));
	*s_indices = newVector(16, sizeof(unsigned));
	*s_constants = newNumVec(128);
	*s_argNamePtr = newPtrVec(4);
	// -----
	
	s_varDeclStack = arena_alloc(g_globArena, sizeof(PtrVec));
	s_fnStack = arena_alloc(g_globArena, sizeof(Vector));
	s_operatorStack = arena_alloc(g_globArena, sizeof(Vector));
	s_singleArgFnStack = arena_alloc(g_globArena, sizeof(Vector));

	*s_varDeclStack = newPtrVec(8);
	*s_fnStack = newVector(8, sizeof(FnCallEntry));
	*s_operatorStack = newVector(64, sizeof(TokenKind));
	*s_singleArgFnStack = newVector(8, sizeof(FnCallEntry));
	// -----

	s_intermediateFn = arena_alloc(g_globArena, sizeof(Function));
	identifier = arena_alloc(g_globArena, sizeof(char*));
	// -----

	s_fnArgsEntry = hashmap_new(sizeof(ArgEntry), 32, 0, 0, 
	stringKeyHash, stringKeyCompare, NULL, NULL);
	g_newVarDeclMap = hashmap_new(sizeof(VarDeclEntry), 32, 0, 0,
		stringKeyHash, stringKeyCompare, NULL, NULL);
	s_tmpVarRecord = hashmap_new(sizeof(char*), 32, 0, 0,
		stringKeyHash, stringKeyCompare, NULL, NULL);
	g_refEntries = hashmap_new(sizeof(VarDependecies), 32, 0, 0,
		stringKeyHash, stringKeyCompare, NULL, NULL);
}

void freeParser() {
	PtrVecFree(s_fnList);
	PtrVecFree(s_varList);
	VecFree(s_opCode);
	VecFree(s_indices);
	NumVecFree(s_constants);
	PtrVecFree(s_argNamePtr);

	PtrVecFree(s_varDeclStack);
	VecFree(s_fnStack);
	VecFree(s_operatorStack);
	VecFree(s_singleArgFnStack);

	hashmap_free(s_fnArgsEntry);
	hashmap_free(g_newVarDeclMap);
	hashmap_free(s_tmpVarRecord);
	hashmap_free(g_refEntries);
}

static FORCE_INLINE void addInstruction(OpCode op) { VecPushByte(s_opCode, op); }
static FORCE_INLINE void addOperator(TokenKind opr) { VecPush(s_operatorStack, &opr); }
static FORCE_INLINE TokenKind peekOperator() { return AS_TK_TYPE(VecTop(s_operatorStack)); }
static FORCE_INLINE TokenKind popOperator() { return AS_TK_TYPE(VecPopBack(s_operatorStack)); }
static FORCE_INLINE void addIndex(unsigned int i) { VecPush(s_indices, &i); }

static struct FuncList* newNode(char* fnName) {
	struct FuncList* ls = arena_alloc(g_globArena, sizeof(struct FuncList));
	ls->fnName = fnName;
	ls->next = NULL;
	return ls;
}

static bool parseInternal();

static inline bool advance() {
	if (idx + 1 < LEN - 1) {
		prevTk = currentTk;
		currentTk = tokens[++idx];
		nextTk = tokens[idx + 1];
		return true;
	}
	return false;
}

static OpCode tokenToInstruction(TokenKind tk, bool* out_hasError) {
	switch (tk) {
	case TK_EXCLAIM: return OP_FACTORIAL;
	case TK_CARET: return OP_POWER;
	case TK_DIV: return OP_DIV;
	case TK_INT_DIV: return OP_INT_DIV;
	case TK_MUL: return OP_MUL;
	case TK_MOD: return OP_MOD;
	case TK_ADD: return OP_ADD;
	case TK_SUB: return OP_SUB;
	case TK_EQUAL: return OP_SET_VAR;
	case TK_EQUALITY: return OP_ISEQUAL;
	case TK_NOT_EQUAL: return OP_NOTEQUAL;
	case TK_KW_AND: return OP_LOGICAL_AND;
	case TK_KW_NOT: return OP_LOGICAL_NOT;
	case TK_KW_OR: return OP_LOGICAL_OR;
	case TK_GT: return OP_GT;
	case TK_SM: return OP_SM;
	case TK_GT_EQ: return OP_GT_OR_EQ;
	case TK_SM_EQ: return OP_SM_OR_EQ;
	// case TK_KW_ELSE: return OP_TERNARY;

	// bad terminology, but due to lack of matching TokenKind and the need for different stacks,
	// the parser uses TK_IDENTIFIER to mark call to single argument builtin functions,
	// TK_COMMA to mark call to multi argument or variadic functions
	// TK_DEF to mark call to single argument user defined functions
	// and TK_ARROW to mark call to user defined functions 😅
	case TK_IDENTIFIER: case TK_COMMA: return OP_CALL_BUILTIN;
	case TK_ARROW: case TK_KW_DEF: return OP_CALL_DEFINED;

	default:
		if (out_hasError != NULL) *out_hasError = true;
		// displayErrorMsg(fstring("wrong TokenKind supplied to tokenToInstruction(TokenKind): %d", tk));
		return OP_LINE_DONE;
	}
}

static FORCE_INLINE void performOperatorSpecificAction(TokenKind sign) {
	if (sign == TK_IDENTIFIER || sign == TK_KW_DEF) {
		PtrVecPush(s_fnList, AS_FN_CALL_PTR(VecPopBack(s_singleArgFnStack))->fnPtr);
	}
	else if (sign == TK_COMMA || sign == TK_ARROW) {
		PtrVecPush(s_fnList, AS_FN_CALL_PTR(VecPopBack(s_fnStack))->fnPtr);
	}
	else if (sign == TK_EQUAL) {
		s_strPtr = PtrVecPopBack(s_varDeclStack);
		VarDeclEntry* vEntry = (VarDeclEntry*)hashmap_get(g_newVarDeclMap, &s_strPtr);
		if (vEntry) vEntry->isInitialized = true;
		PtrVecPush(s_varList, s_strPtr);
	}
}

static void appendOperatorUntil(TokenKind delimiter) {
	TokenKind sign;
	while (s_operatorStack->len != 0 && (sign = popOperator()) != delimiter) {
		bool hasError = false;
		addInstruction(tokenToInstruction(sign, &hasError));
		if (hasError) return;
		performOperatorSpecificAction(sign);	
	}
}

static ParseResult resolveBuiltinFunctionCall() {
	BuiltinFunction* fnPtr = *(BuiltinFunction**)hashmap_get(g_functions, &identifier);

	FnCallEntry callEntry = {
		.tk = currentTk,
		.argsRequired = fnPtr->argsCount,
		.fnPtr = fnPtr,
		.isVariadic = fnPtr->isVariadic,
		.isBuiltin = true
	};

	if (nextTk.type == TK_KW_OF) advance();
	
	if (fnPtr->argsCount == 1 && nextTk.type != TK_OPEN_PAREN) {
		VecPush(s_singleArgFnStack, &callEntry);
		addOperator(TK_IDENTIFIER);
	}
	else if (nextTk.type == TK_OPEN_PAREN) {
		advance();
		VecPush(s_fnStack, &callEntry);
		addOperator(TK_COMMA);  // for marking multi argument function call
		addOperator(TK_OPEN_PAREN);
		addOperator(TK_OPEN_PAREN); // for marking the first inner expression
		if (s_pipeDepth != 0) ++s_absExprParaCount[s_pipeDepth];
		++s_bracketCount;
	}
	else {
		displayError(callEntry.tk, fstring("Expected '(' after function name, as '%s' "
			"excepts more than input", fnPtr->key)
		);
		return FATAL_ERROR;
	}

	if (!s_expectExpr) {
		displayError(currentTk, fstring("Expected a binary operator before function: '%s'", *identifier));
		s_expectExpr = true;
		return ERROR;
	}
	return OK;
}

static ParseResult resolveFunctionCall() {
	Function* fnPtr = *(Function**)hashmap_get(g_userFunctions, &identifier);

	FnCallEntry callEntry = {
		.tk = currentTk,
		.argsRequired = fnPtr->argsCount,
		.fnPtr = fnPtr,
		.isVariadic = false,
		.isBuiltin = false
	};

	if (nextTk.type == TK_KW_OF) advance();

	if (fnPtr->argsCount == 1 && nextTk.type != TK_OPEN_PAREN) {
		VecPush(s_singleArgFnStack, &callEntry);
		addOperator(TK_KW_DEF);
	}
	else if (nextTk.type == TK_OPEN_PAREN) {
		advance();
		VecPush(s_fnStack, &callEntry);
		addOperator(TK_ARROW);  // for marking multi argument function call
		addOperator(TK_OPEN_PAREN);
		addOperator(TK_OPEN_PAREN); // for marking the first inner expression
		if (s_pipeDepth != 0) ++s_absExprParaCount[s_pipeDepth];
		++s_bracketCount;
	}
	else {
		displayError(callEntry.tk, fstring("Expected '(' after function name, as '%s' "
			"excepts more than input", fnPtr->key)
		);
		return FATAL_ERROR;
	}

	if (!s_expectExpr) {
		displayError(currentTk, fstring("Expected a binary operator before function: '%s'", *identifier));
		s_expectExpr = true;
		return ERROR;
	}
	return OK;
}

static ParseResult resolveOfKeyword() {if (nextTk.type == TK_KW_OF) advance();
	if (!s_expectExpr) {
		if (s_operatorStack->len != 0 && peekOperator() == TK_EXCLAIM) {
			addInstruction(tokenToInstruction(popOperator(), NULL));
		}
		addOperator(TK_MUL);
		s_expectExpr = true;
	}
	else if (prevTk.type == TK_MOD) {
		VecPopBack(s_operatorStack);
		NumVecPush(s_constants, 100.0);
		addInstruction(OP_PUSH_NUM);
		addInstruction(OP_DIV);
		addOperator(TK_MUL);
		s_expectExpr = true;
	}
	else {
		displayError(currentTk, "Expected an expression or function before `of` keyword");
		return ERROR;
	}
	return OK;
}

static ParseResult resolveAnsKeyword() {
	if (isnan(g_answer) && s_exprCount == 0) {
		displayError(currentTk, "Invalid reference, 'ans' is not yet initialized");
		return ERROR;
	}
	addInstruction(OP_PUSH_PREV_ANS);
	if (!s_expectExpr) {
		addInstruction(OP_MUL);
	}
	s_expectExpr = false;
	return OK;
}

static ParseResult resolveVariable(char* varNamePtr) {
	if (nextTk.type == TK_EQUAL) {
		if (idx + 2 >= LEN - 1) {
			advance();
			displayError(currentTk,
				fstring("Variable `%s` is left uninitialized", *identifier));
			return ERROR;
		}
		advance();
		PtrVecPush(s_varDeclStack, varNamePtr);
		addOperator(TK_EQUAL);
		if (!s_expectExpr) {
			displayError(currentTk, fstring("Expected a binary operator before variable `%s`", *identifier));
			s_expectExpr = true;
			return ERROR;
		}
		if (s_insideFnBody && !hashmap_get(s_tmpVarRecord, &varNamePtr)) {
			hashmap_set(s_tmpVarRecord, &varNamePtr);
		}
	}
	else {
		if (s_insideFnBody && !hashmap_get(s_tmpVarRecord, &varNamePtr)) {
			hashmap_set(s_tmpVarRecord, &varNamePtr);
		}
		PtrVecPush(s_varList, varNamePtr);
		addInstruction(OP_PUSH_VAR);
		if (!s_expectExpr)
			addInstruction(OP_MUL);
		s_expectExpr = false;
	}
	return OK;
}

static ParseResult resolveNewVariableDecl() {
	if (idx + 1 >= LEN - 1) {
		advance();
		displayError(currentTk, fstring("Variable '%s' is left uninitialized"));
		return ERROR;
	}
	advance();
	s_strPtr = str_from_pool(*identifier);
	PtrVecPush(s_varDeclStack, s_strPtr);
	hashmap_set(g_newVarDeclMap, &(VarDeclEntry){s_strPtr, false});
	addOperator(TK_EQUAL);
	if (!s_expectExpr) {
		displayError(currentTk, fstring("Expected a binary operator before variable '%s'", *identifier));
		s_expectExpr = true;
		return ERROR;
	}
	return OK;
}

static ParseResult registerFunctionHeaderData() {
	if (currentTk.type == TK_KW_OF) advance();
	
	bool expectsComma = false, isInvalid = false;
	int argIdx = 0;
	TokenKind endToken = TK_CLOSE_PAREN;

	if (currentTk.type == TK_IDENTIFIER) {
		endToken = TK_ARROW;
	}
	else if (currentTk.type == TK_OPEN_PAREN) {
		advance();
	}
	else {
		displayError(nextTk, "Expected '(' after function name");
		return FATAL_ERROR;
	}

	while (currentTk.type != endToken && currentTk.type != TK_EOL) {
		if (expectsComma) {
			if (currentTk.type == TK_COMMA || currentTk.type == TK_KW_AND) {
				expectsComma = false;
				advance();
				continue;
			}
			else {
				displayError(currentTk, "Expected ',' after identifier name");
				return FATAL_ERROR;
			}
		}

		if (currentTk.type != TK_IDENTIFIER) {
			displayError(currentTk, fstring("expected identifier found: <r>%s</>",
				tkToString(&currentTk)));
			return FATAL_ERROR;
		}

		*identifier = getIdentifier(currentTk);
		const SymbolType* sType = hashmap_get(g_symbolTable, identifier);

		if (sType && sType->type != VARIABLE) {
			isInvalid = true;
			if (sType->type == BUILTIN_FUNCTION) {
				displayError(currentTk, "Can't use builtin function as argument name");
			}
			else if (sType->type == FUNCTION) {
				displayError(currentTk, fstring("A function named %s already exists, overwriting an existing function is not allowed"));
			}
			else {
				displayError(currentTk, "Cannot use keywords as argument name");
			}
		}
		else if (hashmap_get(s_fnArgsEntry, identifier)) {
			isInvalid = true;
			displayError(currentTk, "Cannot use the same name more than once");
		}
		else {
			char* newArgName = str_from_pool(*identifier);
			PtrVecPush(s_argNamePtr, newArgName);
			hashmap_set(s_fnArgsEntry, &(ArgEntry) {
				.argName = newArgName,
				.pos = argIdx++
			});
		}
		expectsComma = true;
		advance();
	}
	if (currentTk.type == TK_EOL) {
		displayError(currentTk, "Unexpected end of decleration");
		return FATAL_ERROR;
	}

	if (endToken == TK_CLOSE_PAREN) {
		advance();
		if (currentTk.type != TK_ARROW) {
			displayError(currentTk, "Expected <y>=></> after function header");
			return FATAL_ERROR;
		}
	}

	return isInvalid ? FATAL_ERROR : OK;
}

static void freeFunctionMetadata() {
	free_str_from_pool(s_currentFnName);
	while (s_argNamePtr->len != 0) {
		free_str_from_pool(PtrVecPopBack(s_argNamePtr));
	}
}

static void registerUsedGlobalVars() {
	size_t i = 0;
	char** varName;
	while (hashmap_iter(s_tmpVarRecord, &i, (void**)&varName)) {
		VarDependecies* vd = (VarDependecies*)hashmap_get(g_refEntries, varName);
		if (vd) {
			vd->tail->next = newNode(s_currentFnName);
			vd->tail = vd->tail->next;
		} else {
			VarDependecies newVd;
			newVd.name = *varName;
			newVd.head = newNode(s_currentFnName);
			newVd.tail = newVd.head;
			hashmap_set(g_refEntries, &newVd);
		}
	}
}

static ParseResult declareNewFunction() {
	if (!(idx == 0 || prevTk.type == TK_SEMICOLON || s_insideFnBody)) {
		displayError(currentTk, "Invalid use of <c>def</> keyword, can't declare a function here");
		return FATAL_ERROR;
	}
	advance();
	if (currentTk.type != TK_IDENTIFIER) {
		displayError(currentTk, "Expected function name after <c>def</> keyword");
		return FATAL_ERROR;
	}

	*identifier = getIdentifier(currentTk);
	const SymbolType* sm = hashmap_get(g_symbolTable, identifier);

	if (sm) {
		if (sm->type == BUILTIN_FUNCTION || sm->type == FUNCTION) {
			displayError(currentTk, fstring("Cannot use '%s' as the function name,"
				" this name is already in use", *identifier));
		}
		else {
			displayError(currentTk, "A variable with this name is already defined");
		}
		return FATAL_ERROR;
	}
	else if (hashmap_get(g_newVarDeclMap, identifier)) {
		displayError(currentTk, "A variable with this name is already defined");
		return FATAL_ERROR;
	}
	// -----------------------------------------
	s_currentFnName = str_from_pool(*identifier);
	hashmap_clear(s_fnArgsEntry, true);
	advance();
	if (registerFunctionHeaderData() == FATAL_ERROR) {
		freeFunctionMetadata();
		return FATAL_ERROR;
	}

	if (nextTk.type == TK_EOL || nextTk.type == TK_SEMICOLON) {
		freeFunctionMetadata();
		displayError(currentTk, "Unexpected end of function decleration, body is missing");
		return FATAL_ERROR;
	}
	
	VecClear(s_indices);
	const size_t offset_opCode = s_opCode->len;
	const size_t offset_constants = s_constants->len;
	const size_t offset_varList = s_varList->len;
	const size_t offset_fnList = s_fnList->len;

	s_insideFnBody = true;
	hashmap_clear(s_tmpVarRecord, true);
	if (!parseInternal()) {
		freeFunctionMetadata();
		s_insideFnBody = false;
		return FATAL_ERROR;
	}
	s_insideFnBody = false;
	registerUsedGlobalVars();

	// -----------------------------------------
	Function* fn = arena_alloc(g_globArena, sizeof(Function));
	fn->key = s_currentFnName;
	fn->argsCount = hashmap_count(s_fnArgsEntry);
	fn->argsName = arena_alloc(g_globArena, sizeof(char*) * fn->argsCount);
	fn->insCount = s_opCode->len - offset_opCode;
	fn->returnTypeIsNum = !resultsInBool(AS_OP_CODE(VecTop(s_opCode)));

	fn->instructions = arena_alloc(g_globArena, fn->insCount);
	fn->indices = arena_alloc(g_globArena, sizeof(unsigned int) * s_indices->len);
	fn->inputValues = arena_alloc(g_globArena, sizeof(double) * fn->argsCount);

	for (size_t i = 0; i < s_argNamePtr->len; i++) {
		fn->argsName[i] = PtrVecAt(s_argNamePtr, i);
	}
	PtrVecClear(s_argNamePtr);

	const size_t constantLen = s_constants->len - offset_constants;
	const size_t varListLen = s_varList->len - offset_varList;
	const size_t fnListLen = s_fnList->len - offset_fnList;

	fn->constants = arena_alloc(g_globArena, sizeof(double) * constantLen);
	fn->varList = arena_alloc(g_globArena, sizeof(char*) * varListLen);
	fn->fnList = arena_alloc(g_globArena, sizeof(void*) * fnListLen);

	memcpy(fn->instructions, s_opCode->data + offset_opCode, fn->insCount);
	memcpy(fn->indices, s_indices->data, s_indices->len * sizeof(unsigned int));
	memcpy(fn->constants, s_constants->data + offset_constants, sizeof(double) * constantLen);
	memcpy(fn->varList, s_varList->data + offset_varList, sizeof(char*) * varListLen);
	memcpy(fn->fnList, s_fnList->data + offset_fnList, sizeof(void*) * fnListLen);

	s_opCode->len = offset_opCode;
	s_constants->len = offset_constants;
	s_varList->len = offset_varList;
	s_fnList->len = offset_fnList;

	hashmap_set(g_symbolTable, &(SymbolType){s_currentFnName, FUNCTION});
	hashmap_set(g_userFunctions, &fn);
	return OK;
}

static void resolveLetKeyword() {
	if (!s_expectExpr) {
		displayError(currentTk, "Expected a binary operator before 'let' keyword");
	}
	if (nextTk.type == TK_IDENTIFIER && idx + 2 < LEN && tokens[idx + 2].type == TK_EQUAL) {
		advance();
		*identifier = getIdentifier(currentTk);
		const SymbolType* sm;
		if ((sm = hashmap_get(g_symbolTable, identifier))) {
			if (sm->type == VARIABLE) {
				displayError(currentTk, fstring("%s is already defined, "
					"'let' can only be used to declare new variables", tkToString(&currentTk)));
			}
			else {
				displayError(currentTk, fstring("Cannot use %s as variable name", tkToString(&currentTk)));
			}
			advance();
		}
		else if (hashmap_get(g_newVarDeclMap, identifier)) {
			displayError(currentTk, fstring("Cannot re-declare same variable twice "
				"within the same expression", tkToString(&currentTk)));
			advance();
		}
		else {
			resolveNewVariableDecl();
		}
	}
	else {
		displayError(currentTk, "Invalid use of 'let' keyword, not a decleration");
	}
}

static ParseResult resolveIdentifier() {
	*identifier = getIdentifier(currentTk);

	if (s_insideFnBody) {
		const ArgEntry* en = (const ArgEntry*)hashmap_get(s_fnArgsEntry, identifier);
		if (en) {
			addIndex(en->pos);
			addInstruction(OP_PUSH_ARG);
			if (!s_expectExpr)
				addInstruction(OP_MUL);
			s_expectExpr = false;
			return OK;
		}
	}

	const SymbolType* symbolEntry = (const SymbolType*)hashmap_get(g_symbolTable, identifier);
	if (symbolEntry) {
		switch (symbolEntry->type) {
			case VARIABLE:         return resolveVariable(symbolEntry->symbol);
			case BUILTIN_FUNCTION: return resolveBuiltinFunctionCall();
			case FUNCTION:         return resolveFunctionCall();
		}
	}
	
	VarDeclEntry* vEntry = (VarDeclEntry*)hashmap_get(g_newVarDeclMap, identifier);
	if (vEntry) {
		if (vEntry->isInitialized)
			return resolveVariable(vEntry->name);
		else {
			if (nextTk.type == TK_EQUAL) {
				resolveVariable(vEntry->name);
			}
			else {
				displayError(currentTk, fstring(
					"'%s' is used before it could be initialized", tkToString(&currentTk)));
				s_expectExpr = false;
			}
			return ERROR;
		}
	}

	if (nextTk.type == TK_EQUAL) {
		if (s_insideFnBody) {
			displayError(currentTk, "Declaring a new global variable inside function is not allowed");
			advance();
			return ERROR;
		}
		return resolveNewVariableDecl();
	}
	else {
		s_expectExpr = false;
		displayError(currentTk, fstring("'%s' is not a valid identifier or keyword", tkToString(&currentTk)));
		return ERROR;
	}
	return OK;
}

static int precedence(TokenKind tk) {
	switch (tk) {
	case TK_IDENTIFIER:
	case TK_KW_DEF:
		return 11;

	case TK_EXCLAIM:
		return 10;

	case TK_CARET:
		return 9;

	case TK_DIV:
	case TK_INT_DIV:
	case TK_MUL:
	case TK_MOD:
		return 8;

	case TK_ADD:
	case TK_SUB:
		return 7;

	case TK_EQUALITY:
	case TK_NOT_EQUAL:
	case TK_GT:
	case TK_GT_EQ:
	case TK_SM:
	case TK_SM_EQ:
		return 6;

	case TK_KW_NOT:
		return 5;

	case TK_KW_AND:
		return 4;

	case TK_KW_OR:
		return 3;

	case TK_KW_ELSE:
		return 2;

	case TK_EQUAL:
		return 1;

	default: return 0;
	}
}

static void insertInverseFunction() {
	FnCallEntry* f = VecTop(s_singleArgFnStack);
	idx += 2;

	if (strcmp(GET_FN_NAME(f->fnPtr), "sin") == 0) {
		*identifier = "asin";
	}
	else if (strcmp(GET_FN_NAME(f->fnPtr), "cos") == 0) {
		*identifier = "acos";
	}
	else if (strcmp(GET_FN_NAME(f->fnPtr), "tan") == 0) {
		*identifier = "atan";
	}
	else {
		VecPopBack(s_singleArgFnStack);
		popOperator();
		// 	f->tk.start, f->tk.len + tokens[idx].len);
		displayError(f->tk, fstring("No inverse function exists for `%s`", tkToString(&f->tk)));
		return;
	}
	f->fnPtr = *(BuiltinFunction**)hashmap_get(g_functions, &identifier);
}

static inline bool isRightAssociative(TokenKind tk) {
	return tk == TK_CARET || tk == TK_EQUAL || tk == TK_KW_ELSE;
}

static void insertOperator(Token tk) {
	TokenKind sign;
	OpCode oc;

	if (tk.type == TK_CARET && s_singleArgFnStack->len != 0 && idx + 2 < LEN
			&& nextTk.type == TK_SUB && strcmp(tkToString(&tokens[idx + 2]), "1") == 0) {
		insertInverseFunction();
		return;
	}

	if (!s_expectExpr) {
		while (s_operatorStack->len != 0 && (sign = peekOperator()) != TK_OPEN_PAREN
			&& (precedence(sign) > precedence(tk.type)
			|| (precedence(sign) == precedence(tk.type) && !isRightAssociative(tk.type))))
		{
			performOperatorSpecificAction(sign);
			bool hasError = false;
			oc = tokenToInstruction(sign, &hasError);
			VecPopBack(s_operatorStack);
			if (hasError) return;
			addInstruction(oc);
		}

		if (tk.type != TK_EXCLAIM) s_expectExpr = true;
		addOperator(tk.type);
	}

	else if (tk.type == TK_SUB) {
		NumVecPush(s_constants, -1.0);
		addInstruction(OP_PUSH_NUM);
		addOperator(TK_MUL);
		s_expectExpr = true;
	}

	else if (tk.type == TK_KW_NOT) {
		addOperator(TK_KW_NOT);
		s_expectExpr = true;
	}

	else if (tk.type != TK_ADD) {
		displayError(tk, fstring("Expected an expression before `%s` operator", tkToString(&tk)));
		addOperator(tk.type);
	}
}

static void emitMultiArgFunctionCall(FnCallEntry* fnE) {
	appendOperatorUntil(TK_OPEN_PAREN);
	appendOperatorUntil(TK_OPEN_PAREN);
	++fnE->argsFound;
	
	if (fnE->isVariadic) {
		NumVecPush(s_constants, (double)fnE->argsFound);
		addInstruction(OP_PUSH_NUM);
	}
	else if (fnE->argsFound > fnE->argsRequired) {
		displayError(currentTk,
			fstring("Function '%s' only expects %d inputs but %d inputs were provided",
			*(char**)fnE->fnPtr, fnE->argsRequired, fnE->argsFound));
		displayNote(fnE->tk, fstring("'%s' was called here", *(char**)fnE->fnPtr));
	}
	else if (fnE->argsFound < fnE->argsRequired) {
		displayError(currentTk,
			fstring("Function '%s' expects %d inputs but only %d input(s) were provided",
			*(char**)fnE->fnPtr, fnE->argsRequired, fnE->argsFound));
		displayNote(fnE->tk, fstring("'%s' was called here", *(char**)fnE->fnPtr));
	}

	bool hasError = false;
	addInstruction(tokenToInstruction(popOperator(), &hasError));
	if (hasError) return;
	PtrVecPush(s_fnList, AS_FN_CALL_PTR(VecPopBack(s_fnStack))->fnPtr);
}

static bool checkExprEnding() {
	if (s_expectExpr)
		displayError(currentTk, fstring("Expected an expression after '%s'", tkToString(&currentTk)));

	if (s_pipeDepth > 0)
		displayErrorMsg(fstring("%d pipe operator(s) missing or miss matched", s_pipeDepth));

	if (s_bracketCount > 0) {
		displayWarningMsg(fstring("Missing %d closing parentheses", s_bracketCount));
		displayNoteMsg(fstring("Placing %d closing parentheses at the end in an attempt to fix this error\n",
			s_bracketCount));

		while (s_bracketCount-- != 0) {
			if (s_fnStack->len != 0) {
				FnCallEntry* fnE = AS_FN_CALL_PTR(VecTop(s_fnStack));
				if (fnE->innerParenCount == 0) {
					emitMultiArgFunctionCall(fnE);
				} else {
					--fnE->innerParenCount;
					appendOperatorUntil(TK_OPEN_PAREN);
				}
			}
			else {
				appendOperatorUntil(TK_OPEN_PAREN);
			}
		}
	}

	if (g_errorCount != 0)
		return false;

	while (s_operatorStack->len != 0) {
		TokenKind sign = popOperator();
		bool hasError = false;
		addInstruction(tokenToInstruction(sign, &hasError));
		if (hasError) return false;
		performOperatorSpecificAction(sign);
	}
	return true;
}

static void handleCloseParentheses();

static void handleAbsoluteExpression() {
	if (s_pipeDepth > 0) {
		if (s_expectExpr) {
			++s_pipeDepth;
			addOperator(TK_PIPE);
		}
		else {
			if (s_absExprParaCount[s_pipeDepth] > 0) {
				displayWarning(currentTk, fstring(
					"%d unmatched opening parentheses inside this absolute expression",
					s_absExprParaCount[s_pipeDepth]
				));
				displayNoteMsg(fstring("Placing %d closing parentheses in an attempt to fix this error\n",
					s_absExprParaCount[s_pipeDepth]));
				while (s_absExprParaCount[s_pipeDepth] != 0) {
					handleCloseParentheses();
				}
				// s_bracketCount -= s_absExprParaCount[s_pipeDepth];
				// s_absExprParaCount[s_pipeDepth] = 0;
			}
			--s_pipeDepth;
			if (g_errorCount != 0) return;
			appendOperatorUntil(TK_PIPE);
			addInstruction(OP_ABS);
		}
	}
	else {
		if (!s_expectExpr) {
			displayError(currentTk, "Expected an operator before starting this absolute expression");
		}
		if (s_pipeDepth != 0) {
			displayError(currentTk, "Inappropriate use of `|`. Unable to match");
		}
		++s_pipeDepth;
		s_expectExpr = true;
		addOperator(TK_PIPE);
	}
}

static ParseResult handleCommaSeperator() {
	if (s_fnStack->len == 0) {
		displayError(currentTk, "Invalid use of comma seperator, not inside function");
		s_expectExpr = true;
		return ERROR;
	}

	appendOperatorUntil(TK_OPEN_PAREN);
	FnCallEntry* fnE = AS_FN_CALL_PTR(VecTop(s_fnStack));
	if (fnE->innerParenCount > 0) {
		displayWarning(currentTk,
			fstring("Missing %d closing parentheses inside '%s' at input number %d",
			fnE->innerParenCount, *(char**)fnE->fnPtr, fnE->argsFound + 1));

		displayNoteMsg(fstring("Placing %d closing parentheses in an attempt to fix this error\n",
			fnE->innerParenCount));

		while (fnE->innerParenCount != 0) {
			handleCloseParentheses();
		}
		// s_bracketCount -= fnE->innerParenCount;
		// TODO: instead of completely stoping when this error occurs, find a way to continue parsing to catch more errors
		// need to rewrite some parentheses handling parts specially inside abs expr inside function
		// return FATAL_ERROR;
	}

	++fnE->argsFound;
	addOperator(TK_OPEN_PAREN);
	s_expectExpr = true;
	return OK;
}

static bool handleDollarOperator() {
	if (!s_insideFnBody) {
		displayError(currentTk, "'$' can only be used inside function body to refer to global variables");
		return false;
	}

	if (!advance() || currentTk.type != TK_IDENTIFIER) {
		displayError(currentTk, "Missing global variable name after '$'");
		return false;
	}

	*identifier = getIdentifier(currentTk);
	const SymbolType* symbolEntry = (const SymbolType*)hashmap_get(g_symbolTable, identifier);

	if (symbolEntry && symbolEntry->type == VARIABLE) {
		resolveVariable(symbolEntry->symbol);
	}
	else {
		displayError(currentTk, fstring("No global variable named '%s'", *identifier));
		s_expectExpr = false;
	}
	return true;
}

static void handleCloseParentheses() {
	if (s_expectExpr) {
		displayError(currentTk, "Expected an expression, instead found `)`");
	}
	
	if (s_pipeDepth != 0 && --s_absExprParaCount[s_pipeDepth] < 0) {
		s_absExprParaCount[s_pipeDepth] = 0;
		displayError(currentTk, "Unmatched closing parentheses inside an absolute expression");
	}
	else if (--s_bracketCount < 0) {
		displayError(currentTk, "Unmatched closing parentheses");
		s_bracketCount++;
	}

	if (s_fnStack->len != 0) {
		FnCallEntry* fnE = AS_FN_CALL_PTR(VecTop(s_fnStack));
		if (fnE->innerParenCount == 0) {
			emitMultiArgFunctionCall(fnE);
		}
		else {
			--fnE->innerParenCount;
			appendOperatorUntil(TK_OPEN_PAREN);
		}
	}
	else if (g_errorCount == 0) {
		appendOperatorUntil(TK_OPEN_PAREN);
	}
}

static bool parseInternal() {
	double n;

	while (advance()) {
		switch (currentTk.type)
		{
		case TK_NUMBER:
			if (!tkToNumber(&currentTk, &n)) {
				return false;
			}
			NumVecPush(s_constants, n);
			addInstruction(OP_PUSH_NUM);

			if (!s_expectExpr) {
				if (prevTk.type == TK_CLOSE_PAREN)
					addInstruction(OP_MUL);
				else
					displayError(currentTk,
						fstring("Expected a binary operator before `%s`", tkToString(&currentTk)));
			}
			s_expectExpr = false;
			break;

		case TK_OPEN_PAREN:
			if (!s_expectExpr) {
				addOperator(TK_MUL);
				s_expectExpr = true;
			}
			if (s_fnStack->len != 0) {
				AS_FN_CALL_PTR(VecTop(s_fnStack))->innerParenCount++;
			}
			if (s_pipeDepth != 0) ++s_absExprParaCount[s_pipeDepth];
			addOperator(TK_OPEN_PAREN);
			++s_bracketCount;
			break;

		case TK_CLOSE_PAREN:
			handleCloseParentheses();
			break;

		case TK_IDENTIFIER:
			if (resolveIdentifier() == FATAL_ERROR) return false;
			break;

		case TK_KW_DEF:
			if (declareNewFunction() == FATAL_ERROR) return false;
			break;

		case TK_AT_THE_RATE:
			displayError(currentTk, "Unexpected token");
			return false;

		case TK_KW_TRUE:
			NumVecPush(s_constants, 1.0);
			addInstruction(OP_PUSH_NUM);
			s_expectExpr = false;
			break;

		case TK_KW_FALSE:
			NumVecPush(s_constants, 0.0);
			addInstruction(OP_PUSH_NUM);
			s_expectExpr = false;
			break;

		case TK_UNDERSCORE:
			if (!s_expectExpr) {
				displayError(currentTk, "Expected a binary operator before `_`");
			}
			else if (isnan(g_answer) && s_exprCount == 0) {
				displayError(currentTk, "Invalid reference, `_` is not yet initalized");
				s_expectExpr = false;
			}
			else {
				s_expectExpr = false;
				addInstruction(OP_PUSH_PREV_ANS);
			}
			break;

		case TK_KW_OF:
			resolveOfKeyword();
			break;

		case TK_KW_ANS:
			resolveAnsKeyword();
			break;

		case TK_SEMICOLON:
			if (!checkExprEnding()) {
				return false;
			}
			++s_exprCount;
			s_bracketCount = 0;
			if (nextTk.type != TK_EOL) s_expectExpr = true;
			while (s_pipeDepth > 0) s_absExprParaCount[s_pipeDepth--] = 0;
			prevTk = (Token) {TK_EOL, idx, 0};
			if (!s_insideFnBody) addInstruction(OP_LINE_DONE);
			if (s_insideFnBody) return g_errorCount == 0;
			break;

		case TK_KW_LET:
			resolveLetKeyword();
			break;

		case TK_PIPE:
			handleAbsoluteExpression();
			break;

		case TK_EQUAL:
			if (prevTk.type == TK_IDENTIFIER) {
				displayError(prevTk, fstring("Cannot use '%s' as variable name", tkToString(&prevTk)));
				s_expectExpr = true;
			}
			else {
				displayError(currentTk, "Invalid use of `=` operator");
			}
			break;

		case TK_ARROW:
			displayError(currentTk,"Invalid use of '->', not a function decleration");
			break;

		case TK_DOLLAR:
			handleDollarOperator();
			break;

		case TK_COMMA:
			if (handleCommaSeperator() == FATAL_ERROR) {
				displayErrorMsg("parsing stopped here due to fatal error");
				return false;
			}
			break;

		case TK_KW_IF:
		case TK_KW_ELSE:
			displayError(currentTk, "Not implemented");
			return false;
		
		case TK_EOL: break;

		default:
			insertOperator(currentTk);
			break;
		}
    }
	if (checkExprEnding() && !s_insideFnBody) {
		if (s_opCode->len != 0 && AS_OP_CODE(VecTop(s_opCode)) != OP_LINE_DONE) {
			addInstruction(OP_LINE_DONE);
		}
	}
	return g_errorCount == 0;
}

void freeLeftOutStrings(bool isCalledDuringEval) {
	if (hashmap_count(g_newVarDeclMap) == 0) return;
	size_t i = 0;
	VarDeclEntry* varEntry;

	if (isCalledDuringEval) {
		printStyledText("\n<b>info:</> Due to runtime error,\n");
	}

	while (hashmap_iter(g_newVarDeclMap, &i, (void**)&varEntry)) {
		if (isCalledDuringEval) printStyledText(
			fstring("      <r>-> <y>%s</> was not initialized, hence its reference is removed\n", varEntry->name));

		free_str_from_pool(varEntry->name);
	}
	hashmap_clear(g_newVarDeclMap, true);
}

static Function* packIntoFunction() {
	s_intermediateFn->key = NULL;
	s_intermediateFn->argsName = NULL;
	s_intermediateFn->instructions = s_opCode->data;
	s_intermediateFn->insCount = s_opCode->len;
	s_intermediateFn->argsCount = 0;
	s_intermediateFn->constants = s_constants->data;
	s_intermediateFn->varList = (char**)s_varList->data;
	s_intermediateFn->fnList = s_fnList->data;
	if (s_opCode->len > 1) {
		s_intermediateFn->returnTypeIsNum = resultsInBool(AS_OP_CODE(VecAt(s_opCode, s_opCode->len - 2)));
	}
	return s_intermediateFn;
}

Function* parse(Token* t, int startIdx, size_t len) {
	tokens = t;
	LEN = (int)len;
	idx = startIdx - 1;
	s_bracketCount = 0;
	s_expectExpr = true;
	s_pipeDepth = 0;
	s_exprCount = 0;
	s_insideFnBody = false;

	VecClear(s_operatorStack);
	VecClear(s_fnStack);
	VecClear(s_singleArgFnStack);
	PtrVecClear(s_varDeclStack);

	PtrVecClear(s_fnList);
	PtrVecClear(s_varList);
	VecClear(s_opCode);
	NumVecClear(s_constants);

	currentTk = (Token) {TK_EOL, 0, 0};
	while (s_pipeDepth > 0) s_absExprParaCount[s_pipeDepth--] = 0;

	if (!parseInternal()) {
		freeLeftOutStrings(false);
		return NULL;
	}

	return s_opCode->len == 0 ? NULL : packIntoFunction();
}
