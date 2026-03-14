#include <stdlib.h>

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
#include "commands.h"
#include "allocator.h"
#include "parser.h"

static void freeResources() {
	unloadCommands();
	freeLexer();
	unloadRegistry();
	freeParser();
	freeEvaluator();
	freeAllocators();
}

int main(int argc, char* argv[]) {
	initPlatform();
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

	displayHelpAndUsageGuide();

	while (isRunning()) {
		printStyledText("<y>>>");
		char* input = STRING_INPUT(" ");
		evaluateInput(input);
		if (input) free(input);
	}
	freeResources();
	return EXIT_SUCCESS;
}
