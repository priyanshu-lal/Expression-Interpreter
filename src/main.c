#include <stdlib.h>
#include <time.h>

#if defined(__linux__)
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#define STRING_INPUT(MSG) readline(MSG)
#else
#define STRING_INPUT(MSG) StringInput(MSG)
#endif

#include "lexer.h"
#include "logger.h"
#include "registry.h"
#include "utils.h"
#include "allocator.h"
#include "parser.h"

struct timespec start, end;

static void freeResources() {
	unloadCommands();
	freeLexer();
	unloadRegistry();
	freeAllocators();
	freeParser();
	freeEvaluator();
}

int main(int argc, char* argv[]) {
	initLexer();
	initAllocators();
	loadRegistry();
	initParser();
	loadCommands();
	initEvaluator();
	
	if (argc > 1) {
		runInlineInputs(argc, argv);
		freeResources();
		return 0;
	}

	bool isRunning = true;
	size_t tokenLen = 0;
	Token* tokens;
	char* input = NULL;
	displayHelpAndUsageGuide();
	printStyledText("<c>\nEnter expressions and commands here:\n");

	while (isRunning) {
		printStyledText("<y>>>");;
		if (input) free(input);
		input = STRING_INPUT(" ");

		clock_gettime(CLOCK_MONOTONIC, &start);
		if ((tokens = tokenize(input, strlen(input), &tokenLen)) == NULL) {
			continue;
		}
		else if (tokens[0].type == TK_AT_THE_RATE && tokenLen >= 3) {
			resolveCommand(tokens, tokenLen, &isRunning);
		}
		else if (tokenLen > 1) {
			evaluateInput(tokens, 0, tokenLen);
		}
	}
	freeResources();
	return EXIT_SUCCESS;
}
// sqrt (20% of (x = sqrt |-50 + 14|)!)
// mean of (x = sqrt (20% of sqrt |-50 + 14|!), nCr (x, x / 2), 3x)
// num = -.5(144); x = |10 - root(|num| * 2, 2)|
// num = factorial of -x = lcm of (4, 6)

// error: sqrt (12-|45,9|)  // handle pipe inside function
// strange error: |x(sin)|

/*
Debug Console history:
p *(TokenType(*)[2])s_operatorStack->data
p *(double(*)[4])eUnit->constants
p *(enum OP_CODE(*)[4])eUnit->instructions
*/

// clang -O3 -I src/include src/*.c -lm -lreadline -o calc -march=native -flto -fvisibility=hidden
