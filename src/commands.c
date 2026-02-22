#include "commands.h"
#include "evaluator.h"
#include "parser.h"
#include "lexer.h"
#include "logger.h"
#include "utils.h"
#include "allocator.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct {
	const char* key;
	Command cmd;
} CommandEntry;

static hashmap* s_commandTable;
static bool s_answerInFraction = false;
static bool s_showTimestamp = false;
static bool s_isRunning = true;
static EvalMode s_evalMode = DIRECT;

void loadCommands() {
	s_commandTable = hashmap_new(sizeof(CommandEntry), 32, 0, 0,
		stringKeyHash, stringKeyCompare, NULL, NULL);
	hashmap_set(s_commandTable, &(CommandEntry){"angle", COMMAND_ANGLE});
	hashmap_set(s_commandTable, &(CommandEntry){"clear", COMMAND_CLEAR});
	hashmap_set(s_commandTable, &(CommandEntry){"cls", COMMAND_CLEAR});
	hashmap_set(s_commandTable, &(CommandEntry){"display", COMMAND_DISPLAY});
	hashmap_set(s_commandTable, &(CommandEntry){"eval_mode", COMMAND_EVAL_MODE});
	hashmap_set(s_commandTable, &(CommandEntry){"exit", COMMAND_EXIT});
	hashmap_set(s_commandTable, &(CommandEntry){"help", COMMAND_HELP});
	hashmap_set(s_commandTable, &(CommandEntry){"quit", COMMAND_EXIT});
	hashmap_set(s_commandTable, &(CommandEntry){"list", COMMAND_LIST});
	hashmap_set(s_commandTable, &(CommandEntry){"remove", COMMAND_REMOVE});
	hashmap_set(s_commandTable, &(CommandEntry){"query", COMMAND_QUERY});
	hashmap_set(s_commandTable, &(CommandEntry){"precedence", COMMAND_PRECEDENCE});
	hashmap_set(s_commandTable, &(CommandEntry){"timestamp", COMMAND_TIMESTAMP});
}

void unloadCommands() {
	hashmap_free(s_commandTable);
}

bool isRunning() { return s_isRunning; }
bool isShowTimeEnabled() { return s_showTimestamp; }
bool isAnswerInFrcation() { return s_answerInFraction; }
EvalMode getEvalMode() { return s_evalMode; }

void displayVariables() {
	if (hashmap_count(g_variables) == 0) {
		printStyledText(" <y>:: Variable list is empty\n");
		return;
	}
	size_t i = 0;
	const Variable* vPtr;
	printStyledText("\n<y>Variable List:\n");
	while (hashmap_iter(g_variables, &i, (void**)&vPtr)) {
		changeTextColor(COLOR_CYAN);
		printf("  %s ", vPtr->key);
		resetTextAttribute();
		printf(": %g\n", vPtr->value);
	}
	putchar('\n');
	displayHint("You can use <b>@remove</> command to remove variables"
		"\n      <~u>Syntax</>: <b>@remove</> <c>varName1</>, <c>varName2</>, <c>...");
}

void displayBuiltInFunctions() {
	size_t i = 0;
	const BuiltinFunction** fnPtr;
	printStyledText("\n<y>Function List:\n");

	while (hashmap_iter(g_functions, &i, (void**)&fnPtr)) {
		changeTextColor(COLOR_BLUE);
		printf("  %s", (*fnPtr)->key);
		resetTextAttribute();

		if ((*fnPtr)->isVariadic) {
			printStyledText("(<c>...</>)\n");
		}
		else if ((*fnPtr)->argsCount == 1) {
			printStyledText("(<c>n</>)\n");
		}
		else {
			char c = 'a';
			putchar('(');
			for (int i = 1; i < (*fnPtr)->argsCount; i++) {
				printStyledText(fstring("<c>%c</>, ", c++));
			}
			printStyledText(fstring("<c>%c</>)\n", c));
		}
	}
	resetTextAttribute();
}

void displayUserFunctions() {
	if (hashmap_count(g_userFunctions) == 0) {
		changeTextColor(COLOR_YELLOW);
		puts(" :: No user defined function exists");
		resetTextAttribute();
		return;
	}
	size_t i = 0, k = 1;
	const Function** uFn;
	printStyledText("\n<y>User Defined Functions:\n");

	while (hashmap_iter(g_userFunctions, &i, (void**)&uFn)) {
		printStyledText(fstring("  %zu) <b>%s</>(", k++, (*uFn)->key));
		for (unsigned i = 0; i < (*uFn)->argsCount - 1; i++) {
			printStyledText(fstring("<c>%s</>, ", (*uFn)->argsName[i]));
		}
		// printf("%s) - \n", (*uFn)->argsName[(*uFn)->argsCount - 1]);
		printStyledText(fstring("<c>%s</>) - <b>%u</> instructions\n",
			(*uFn)->argsName[(*uFn)->argsCount - 1], (*uFn)->insCount));
	}
	
}

static bool removeCommand(Token* tokens, size_t len) {
	if (len <= 3) {
		displayError(tokens[1], "Expected a list of variable names after 'remove' command");
		return false;
	}
	PtrVec freeList = newPtrVec(8);
	char* varName;
	bool expectComma = false, isInvalid = false;
	Token prevTk = {0};
	Token currentTk;
	size_t idx = 2;
	
	for (; (currentTk = tokens[idx++]).type != TK_EOL; prevTk = currentTk) {
		if (expectComma) {
			if (currentTk.type == TK_COMMA) {
				expectComma = false;
				continue;
			}
			else {
				isInvalid = true;
				displayError(prevTk, "Expected ',' after variable name");
				break;
			}
		}
		
		if (currentTk.type != TK_IDENTIFIER) {
			isInvalid = true;
			displayError(currentTk, "Expected a list of variable after 'remove' keyword");
			break;
		}

		varName = getIdentifier(currentTk);
		const SymbolType* keyType = (const SymbolType*)hashmap_get(g_symbolTable, &varName);

		if (keyType == NULL) {
			displayError(currentTk, fstring("No variable named '%s' exists", varName));
			isInvalid = true;
		}
		else if (keyType->type == VARIABLE) {
			const VarDependecies* vd = hashmap_get(g_refEntries, &varName);
			if (vd) {
				displayError(currentTk, "Can't remove this variable as it's been referenced in the following function(s):");
				struct FuncList* fList = vd->head;
				int i = 1;
				while (fList) {
					printStyledText(fstring("          %d) <c>%s\n", i++, fList->fnName));
					fList = fList->next;
				}
			}
			else {
				PtrVecPush(&freeList, keyType->symbol);
			}
		}
		else {
			isInvalid = true;
			const char* typeStr = (keyType->type == FUNCTION || keyType->type == BUILTIN_FUNCTION)
				? "function" : "keyword";
			displayError(currentTk, fstring("Expected variable name, found %s '%s'",
				typeStr, varName));
		}
		expectComma = true;
	}

	if (isInvalid || freeList.len == 0) {
		PtrVecFree(&freeList);
		return false;
	}
	
	for (size_t i = 0; i < freeList.len; i++) {
		varName = (char*)PtrVecAt(&freeList, i);
		hashmap_delete(g_symbolTable, &varName);
		hashmap_delete(g_variables, &varName);
		printStyledTextInBox(fstring("variable <c>%s</> deleted", varName));
		free_str_from_pool(varName);
	}
	PtrVecFree(&freeList);
	return true;
}

static bool changeDisplayMode(Token tk) {
	if (tk.type == TK_EOL) {
		displayErrorMsg("expected display mode option after '#y#display#r#' command");
		return false;
	}
	const char* command = tkToString(&tk);
	if (strcmp("fraction", command) == 0) {
		if (s_answerInFraction) {
			printStyledTextInBox("Display mode is already in <c>fraction</>");
		}
		else {
			printStyledTextInBox("Display mode is changed: <c>decimal</> <y>-></> <g>fraction</>");
			s_answerInFraction = true;
		}
	}
	else if (strcmp("decimal", command) == 0) {
		if (s_answerInFraction) {
			printStyledTextInBox("Display mode is changed: <c>fraction</> <y>-></> <g>decimal</>");
			s_answerInFraction = false;
		}
		else {
			printStyledTextInBox("Display mode is already in <c>decimal</>");
		}
	}
	else {
		displayError(tk, "Invalid option after 'display' command\n"
			"       Valid options: <c>decimal</> or <c>fraction</>");
		return false;
	}
	return true;
}

static bool configTimestamp(Token tk) {
	if (tk.type == TK_EOL) {
		displayErrorMsg("expected <y>on</> or <y>off</> option after <c>timestamp</> command");
		return false;
	}

	const char* command = tkToString(&tk);
	if (strcmp(command, "on") == 0) {
		s_showTimestamp = true;
	}
	else if (strcmp(command, "off") == 0) {
		s_showTimestamp = false;
	}
	else {
		displayError(tk, "Invalid option after 'timestamp' command\n"
			"       Valid options are: <c>on</> or <c>off</>");
		return false;
	}
	return true;
}

static bool resolveQuery(Token tk) {
	if (tk.type == TK_EOL) {
		displayErrorMsg("expected status option after <c>query</> command");
		return false;
	}
	
	const char* command = tkToString(&tk);
	if (strcmp(command, "display") == 0) {
		printStyledTextInBox(fstring("Display mode is currently set to <y>%s</>",
			s_answerInFraction ? "fraction" : "decimal"));
	}
	else if (strcmp(command, "angle") == 0) {
		printStyledTextInBox(fstring("Angle unit is currently set to <y>%s</>",
			getAngleUnit() == DEGREE ? "degree" : "radian"));
	}
	else if (strcmp(command, "vars") == 0) {
		size_t i = 0, bytes = 0;
		const Variable* vPtr;
		while (hashmap_iter(g_variables, &i, (void**)&vPtr)) {
			bytes += strlen(vPtr->key) < 8 ? 8 : 16;
			bytes += sizeof(Variable);
		}
		printStyledTextInBox(fstring("<y>%zu</> variables, taking <y>%g <c>KBs</> of memory</>",
			hashmap_count(g_variables), (double)(bytes) / 1000.0));
	}
	else {
		displayError(tk, "Invalid option after 'query' command\n"
			"       Valid options are: <c>display</> or <c>angle</>");
		return false;
	}
	return true;
}

static bool changeAngleUnit(Token tk) {
	if (tk.type == TK_EOL) {
		displayErrorMsg("Missing option argument after list command\n"
			"       Valid options: 'degree', 'radian'");
		return false;
	}

	const char* command = tkToString(&tk);
	if (strcmp(command, "degree") == 0) {
		if (getAngleUnit() == DEGREE) {
			printStyledTextInBox("Angle unit is already set to <g>degree</>");
		} else {
			printStyledTextInBox("Angle unit changed: <c>radian</> <y>-></> <g>degree</>");
			setAngleUnit(DEGREE);
		}
	}
	else if (strcmp(command, "radian") == 0) {
		if (getAngleUnit() == RADIAN) {
			printStyledTextInBox("Angle unit is already set to <g>radian</>");
		} else {
			printStyledTextInBox("Angle unit changed: <c>degree</> <y>-></> <g>radian</>");
			setAngleUnit(RADIAN);
		}
	}
	else {
		displayError(tk, "Invalid token after 'angle' command\n"
			"       Valid options: <g>degree</>, <g>radian</>");
		return false;
	}
	return true;
}

static bool changeEvalMode(Token tk) {
	if (tk.type == TK_EOL) {
		displayErrorMsg("Missing option argument after eval_mode command\n"
			"       Valid options: <g>detailed</>, <g>direct</>");
		return false;
	}

	const char* command = tkToString(&tk);
	if (strcmp("detailed", command) == 0) {
		if (s_evalMode == DETAILED) {
			printStyledTextInBox("Evaluation mode is already set to <g>detailed</>");
		}
		else {
			s_evalMode = DETAILED;
			printStyledTextInBox("Evaluation mode changed: <c>direct</> <y>-></> <g>detailed</>");
		}
	}
	else if (strcmp("direct", command) == 0) {
		if (s_evalMode == DETAILED) {
			s_evalMode = DIRECT;
			printStyledTextInBox("Evaluation mode changed: <c>detailed</> <y>-></> <g>direct</>");
		}
		else {
			printStyledTextInBox("Evaluation mode is already set to <g>direct</>");
		}
	}
	else {
		displayError(tk, "Invalid token after 'eval_mode' command\n"
			"       Valid options: <g>detailed</>, <g>direct</>");
		return false;
	}
	return true;
}

void displayPrecedenceTable() {
	printStyledText(
		"\n<c>╭─────────────────────────────────────────────────────────────────────────────────────╮"
		"\n│ <~u>Level</> <c>│</>       <c><~u>Operator</>       <c>│</>                     <c><~u>Description</>                      <c>│"          
		"\n│───────│──────────────────────│──────────────────────────────────────────────────────│"
		"\n│   1   │ <y>f(x)</>, <y>()</>, <y>|expr|</>     <c>│</> Function Call, Parentheses and Absolute expression   <c>│"
		"\n│   2   │ <y>!</>                    <c>│</> Factorial operator                                   <c>│"
		"\n│   3   │ <y>**</>, <y>^</>                <c>│</> Exponentiation                                       <c>│"
		"\n│   4   │ <y>-x</>, <y>+x</>               <c>│</> Unary plus and Unary minus                           <c>│"
		"\n│   5   │ <y>*</>, <y>/</>, <y>//</>, <y>%</>          <c>│</> Multiplication, Division, Floor division and Modulus <c>│"
		"\n│   6   │ <y>+</>, <y>-</>                 <c>│</> Addition and Subtraction                             <c>│"
		"\n│   7   │ <y>==</>, <y>!=</>, <y>></>, <y>>=</>, <y><</>, <y><=</> <c>│</> Comparision operators                                <c>│"
		"\n│   8   │ <y>not</>                  <c>│</> Logical NOT                                          <c>│"
		"\n│   9   │ <y>and</>                  <c>│</> Logical AND                                          <c>│"
		"\n│   10  │ <y>or</>                   <c>│</> Logical OR                                           <c>│"
		"\n│   11  │ <y>=</>, <y>is</>                <c>│</> Assignment operator                                  <c>│"
		"\n╰─────────────────────────────────────────────────────────────────────────────────────╯\n"
	);
}

static bool listCommand(Token tk) {
	if (tk.type == TK_EOL) {
		displayErrorMsg("Missing option argument after list command\n"
			"       Valid options: <c>vars</>, <c>bultins</>, <c>functions</>");
		return false;
	}
	if (strcmp(tkToString(&tk), "vars") == 0) {
		displayVariables();
	}
	else if (strcmp(tkToString(&tk), "builtins") == 0) {
		displayBuiltInFunctions();
	}
	else if (strcmp(tkToString(&tk), "functions") == 0) {
		displayUserFunctions();
	}
	else {
		displayError(tk, "Invalid token after 'list' command\n"
			"       Valid options: <c>vars</>, <c>bultins</>, <c>functions</>");
		return false;
	}
	return true;
}

static void clearScreen() {
	printf("\033[2J\033[H");
	fflush(stdout);
}

static bool resolveCommand(Token* tokens, size_t len, Command* outCmdType) {
	size_t idx = 1;
    char* command = tkToString(&tokens[idx++]);
    const CommandEntry* ce = hashmap_get(s_commandTable, &command);
	if (!ce) {
		displayError(tokens[1], fstring("<y>%s</> is not a valid command.\n", command));
		return false;
	}
	if (outCmdType) *outCmdType = ce->cmd;

	switch (ce->cmd) {
		case COMMAND_ANGLE:
			return changeAngleUnit(tokens[idx]);

		case COMMAND_CLEAR:
			clearScreen();
			printStartingMsg();
			break;

		case COMMAND_DISPLAY:
			return changeDisplayMode(tokens[idx]);

		case COMMAND_EVAL_MODE:
			return changeEvalMode(tokens[idx]);

		case COMMAND_EXIT:
			s_isRunning = false;
			break;

		case COMMAND_HELP:
			clearScreen();
			displayHelpAndUsageGuide();
			break;

		case COMMAND_LIST:
			return listCommand(tokens[idx]);
			
		case COMMAND_REMOVE:
			return removeCommand(tokens, len);

		case COMMAND_QUERY:
			return resolveQuery(tokens[idx]);

		case COMMAND_PRECEDENCE:
			displayPrecedenceTable();
			break;

		case COMMAND_TIMESTAMP:
			return configTimestamp(tokens[idx]);
	}
	return true;
}
