#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "utils.h"
#include "logger.h"
#include "parser.h"
#include "container.h"
#include "evaluator.h"
#include "registry.h"
#include "allocator.h"

typedef enum {
	COMMAND_ANGLE,
	COMMAND_CLEAR,
	COMMAND_DISPLAY,
	COMMAND_EVAL_MODE,
	COMMAND_EXIT,
	COMMAND_HELP,
	COMMAND_LIST,
	COMMAND_REMOVE,
	COMMAND_QUERY,
	COMMAND_PRECEDENCE,
	COMMAND_TIMESTAMP
} Command;

typedef struct {
	const char* key;
	Command cmd;
} CommandEntry;

static hashmap* s_commandTable;

char* StringInput(const char* msg) {
	changeTextColor(COLOR_YELLOW);
	if (msg) printf("%s", msg);
	resetTextAttribute();
	size_t len = 0, capacity = 64;
	char* str = (char*) malloc(sizeof(char) * capacity);
	char ch;
	while ((ch = getchar()) != '\n') {
		if (len == capacity) {
			capacity *= 2;
			char* p = realloc(str, sizeof(char) * capacity);
			if (!p) {
				printf("Failed to reallocate more memory\n");
				abort();
			}
			str = p;
		}
		str[len++] = ch;
	}
	str[len] = '\0';
	return str;
}

const char* fractionalApproximation(double num) {
	static const double TOLERANCE = 1.0e-6;
	if (doubleAbs(num - floor(num)) < TOLERANCE) {
		return fstring("%lf", num);
	}
	double fraction = 1.0;
	double numerator = 1.0, denominator = 1.0;

	while (doubleAbs(fraction - num) > TOLERANCE) {
		if (fraction < num) {
			numerator++;
		} else {
			denominator++;
			numerator = round(num * denominator);
		}
		fraction = numerator / denominator;
	}
	
	long a = (long) numerator, b = (long) denominator;
	while (b != 0) {
		long tmp = b;
		b = a % b;
		a = tmp;
	}
	return fstring("%ld / %ld", (long)(numerator / a), (long)(denominator / a));
}

void printStartingMsg() {
	changeTextColor(COLOR_BLUE);
	printf("╭════════════════════════════════════════════════════════════╮\n│");
	printStyledText(" <c>Expression Interpreter (<b>REPL<c>)<y> (enter \"<g><~u>@help</><y>\" to view help) ");
	changeTextColor(COLOR_BLUE);
	puts("│\n╰════════════════════════════════════════════════════════════╯");
	
	resetTextAttribute();
}

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
		printf("  %s", (*fnPtr)->key);

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
	size_t i = 0;
	const Function* uFn;
	printStyledText("\n<y>User Defined Functions:");
	changeTextColor(COLOR_CYAN);

	while (hashmap_iter(g_userFunctions, &i, (void**)&uFn)) {
		printf("  %s(", uFn->key);
		for (unsigned i = 0; i < uFn->argsCount - 1; i++) {
			printf("%s, ", uFn->argsName[i]);
		}
		printf("%s)\n", uFn->argsName[uFn->argsCount - 1]);
	}
}

void displayHelpAndUsageGuide() {
	changeTextColor(COLOR_YELLOW);
	printf(
		"\n  █████╗   █████╗  ██╗      ██████╗"
		"\n██╔════╝  ██╔══██╗ ██║     ██╔════╝"
		"\n██║       ███████║ ██║     ██║     "
		"\n██║       ██╔══██║ ██║     ██║     "
		"\n╚██████╗  ██║  ██║ ███████╗╚██████╗"
		"\n ╚═════╝  ╚═╝  ╚═╝ ╚══════╝ ╚═════╝\n");

	printStyledText("\n<~u>List of Commands:\n");
	changeTextColor(COLOR_CYAN);
	puts("╭══════════════════════════════════════════════════════════════════════════════╮");
	resetTextAttribute();
	printStyledText(
		"<c>│ <~u>help</>                 --  to view help section (<g>this screen</>)                  <c>│"
		"\n│ <~u>precedence</>           --  to view operator precedence table                   <c>│"
		"\n│ <~u>quit</>, <c><~u>exit</>           --  exit out of the application                         <c>│"
		"\n│ <~u>clear</>, <c><~u>cls</>           --  clear the screen                                    <c>│"
		"\n│ <~u>list</> <b>vars</>            --  display all the variables with their name and value <c>│"
		"\n│ <~u>list</> <b>builtins</>        --  display prototype of all the predefined functions   <c>│"
		"\n│ <~u>list</> <b>functions</>       --  display prototype of all the scripted functions     <c>│"
		"\n│ <~u>display</> <b>fraction</>     --  display answer in fractional form                   <c>│"
		"\n│ <~u>display</> <b>decimal</>      --  display answer in decimal form (<g>Default</>)            <c>│"
		"\n│ <~u>angle</> <b>degree</>         --  set angle unit as degree (<g>Default</>)                  <c>│"
		"\n│ <~u>angle</> <b>radian</>         --  set angle unit as radian                            <c>│"
		"\n│ <~u>query</> <b>display</>        --  prints current display mode                         <c>│"
		"\n│ <~u>query</> <b>angle</>          --  prints current angle unit                           <c>│"
		"\n│ <~u>eval_mode</> <b>detailed</>   --  prints each steps taken by the evaluation engine    <c>│"
		"\n│ <~u>eval_mode</> <b>direct</>     --  prints the result directly (<g>Default</>)                <c>│"
		"\n│ <~u>timestamp</> <b>on</> / <b>off</>   --  toggle timestamp display option                     <c>│"
		"\n│ <~u>remove</> <b>varNames...</>   --  remove variables                                    <c>│\n"
	);
	changeTextColor(COLOR_CYAN);
	puts("╰══════════════════════════════════════════════════════════════════════════════╯");
	resetTextAttribute();
	printStyledText("<r>NOTE:</> All the above listed commands start with '<y>@</>' symbol. Ex.: <y>@clear\n");
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
		"\n│   11  │ <y>=</>                    <c>│</> Assignment operator                                  <c>│"
		"\n╰─────────────────────────────────────────────────────────────────────────────────────╯\n"
	);
}

static bool s_answerInFraction = false;

void evaluateInput(Token* tokens, size_t startIdx, size_t len) {
	NumVecClear(st);
	Function* ufn = parse(tokens, startIdx, len);

	if (ufn) {
		// if (evaluate(ufn)) {
		// 	for (size_t i = 0; i < st->len; i++) {
		// 		changeTextColor(COLOR_BLUE);
		// 		printf("==> ");
		// 		changeTextColor(COLOR_GREEN);
		// 		if (ufn->insCount >= 2 && resultsInBool(ufn->instructions[ufn->insCount - 2])) {
		// 			printf("%s\n", toBoolString(NumVecAt(st, i)));
		// 		}
		// 		else if (s_answerInFraction) {
		// 			printf("%s\n", fractionalApproximation(NumVecAt(st, i)));
		// 		}
		// 		else {
		// 			printf("%g\n", NumVecAt(st, i));
		// 		}
		// 		resetTextAttribute();
		// 	}
		// }
		// else {
		// 	freeLeftOutStrings(true);
		// 	putchar('\n');
		// }
		if (!evaluate(ufn)) {
			freeLeftOutStrings(true);
			putchar('\n');
		}
	}
	else if (g_errorCount > 1) {
		changeTextColor(COLOR_RED);
		printf("\n  :: Found %d errors in total\n\n", g_errorCount);
		resetTextAttribute();
	}
	else {
		printf("\n");
	}
}

void listCommand(Token tk) {
	if (tk.type == TK_EOL) {
		displayErrorMsg("Missing option argument after list command\n"
			"       Valid options: <c>vars</>, <c>bultins</>, <c>functions</>");
		return;
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
	}
}

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

void removeCommand(Token* tokens, size_t len) {
	if (len <= 3) {
		displayError(tokens[1], "Expected a list of variable names after 'remove' command");
		return;
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
			PtrVecPush(&freeList, keyType->symbol);
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
	if (!isInvalid && freeList.len != 0) {
		for (size_t i = 0; i < freeList.len; i++) {
			varName = (char*)PtrVecAt(&freeList, i);
			hashmap_delete(g_symbolTable, &varName);
			hashmap_delete(g_variables, &varName);
			printStyledTextInBox(fstring("variable <c>%s</> deleted", varName));
			free_str_from_pool(varName);
		}
	}
	PtrVecFree(&freeList);
}

static void changeDisplayMode(Token tk) {
	if (tk.type == TK_EOL) {
		displayErrorMsg("expected display mode option after '#y#display#r#' command");
		return;
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
	}
}

static void configTimestamp(Token tk) {
	if (tk.type == TK_EOL) {
		displayErrorMsg("expected <y>on</> or <y>off</> option after <c>timestamp</> command");
		return;
	}
	const char* command = tkToString(&tk);
	if (strcmp(command, "on") == 0) {
		g_showTimestamp = true;
	}
	else if (strcmp(command, "off") == 0) {
		g_showTimestamp = false;
	}
	else {
		displayError(tk, "Invalid option after 'timestamp' command\n"
			"       Valid options are: <c>on</> or <c>off</>");
	}
}

static void resolveQuery(Token tk) {
	if (tk.type == TK_EOL) {
		displayErrorMsg("expected status option after <c>query</> command");
		return;
	}
	
	const char* command = tkToString(&tk);
	if (strcmp(command, "display") == 0) {
		printStyledTextInBox(fstring("Display mode is currently set to <y>%s</>", s_answerInFraction ? "fraction" : "decimal"));
	}
	else if (strcmp(command, "angle") == 0) {
		printStyledTextInBox(fstring("Angle unit is currently set to <y>%s</>", getAngleUnit() == DEGREE ? "degree" : "radian"));
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
	}
}

static void changeAngleUnit(Token tk) {
	if (tk.type == TK_EOL) {
		displayErrorMsg("Missing option argument after list command\n"
			"       Valid options: 'degree', 'radian'");
		return;
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
	}
}

static void changeEvalMode(Token tk) {
	if (tk.type == TK_EOL) {
		displayErrorMsg("Missing option argument after eval_mode command\n"
			"       Valid options: <g>detailed</>, <g>direct</>");
		return;
	}

	const char* command = tkToString(&tk);
	if (strcmp("detailed", command) == 0) {
		if (getEvaluationMode() == DETAILED) {
			printStyledTextInBox("Evaluation mode is already set to <g>detailed</>");
		}
		else {
			setEvaluationMode(DETAILED);
			printStyledTextInBox("Evaluation mode changed: <c>direct</> <y>-></> <g>detailed</>");
		}
	}
	else if (strcmp("direct", command) == 0) {
		if (getEvaluationMode() == DETAILED) {
			setEvaluationMode(DIRECT);
			printStyledTextInBox("Evaluation mode changed: <c>detailed</> <y>-></> <g>direct</>");
		}
		else {
			printStyledTextInBox("Evaluation mode is already set to <g>direct</>");
		}
	}
	else {
		displayError(tk, "Invalid token after 'eval_mode' command\n"
			"       Valid options: <g>detailed</>, <g>direct</>");
	}
}

void resolveCommand(Token* tokens, size_t len, bool* isRunning) {
	size_t idx = 1;
	char* command = tkToString(&tokens[idx++]);

	const CommandEntry* ce = hashmap_get(s_commandTable, &command);
	if (!ce) {
		displayError(tokens[1], fstring("<y>%s</> is not a valid command.\n", command));
		return;
	}
	switch (ce->cmd) {
		case COMMAND_ANGLE:
			changeAngleUnit(tokens[idx]);
			break;

		case COMMAND_CLEAR:
			system("clear");
			printStartingMsg();
			return;

		case COMMAND_DISPLAY:
			changeDisplayMode(tokens[idx]);
			break;

		case COMMAND_EVAL_MODE:
			changeEvalMode(tokens[idx]);
			break;

		case COMMAND_EXIT:
			*isRunning = false;
			return;

		case COMMAND_HELP:
			system("clear");
			displayHelpAndUsageGuide();
			break;

		case COMMAND_LIST:
			listCommand(tokens[idx]);
			break;
			
		case COMMAND_REMOVE:
			removeCommand(tokens, len);
			break;

		case COMMAND_QUERY:
			resolveQuery(tokens[idx]);
			break;

		case COMMAND_PRECEDENCE:
			displayPrecedenceTable();
			break;

		case COMMAND_TIMESTAMP:
			configTimestamp(tokens[idx]);
			break;
	}
	putchar('\n');
}

void runInlineInputs(int argc, char* argv[]) {
	Token* tokens;
	size_t tkLen;
	for (int i = 1; i < argc; i++) {
		char* expr = argv[i];
		if (expr[0] == '-') {
			if (strcmp("--angle:degree", expr) == 0) {
				setAngleUnit(DEGREE);
				continue;
			}
			else if (strcmp("--angle:radian", expr) == 0) {
				setAngleUnit(RADIAN);
				continue;
			}
		}
		if ((tokens = tokenize(expr, strlen(expr), &tkLen)) != NULL) {
			NumVecClear(st);
			Function* ufn = parse(tokens, 0, tkLen);
			if (ufn) {
				if (!evaluate(ufn)) {
					freeLeftOutStrings(true);
					putchar('\n');
				}
			}
		}
	}
}
