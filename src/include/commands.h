#pragma once

#include <stdbool.h>

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

void loadCommands();
void unloadCommands();

void displayVariables();
void displayBuiltInFunctions();
void displayUserFunctions();

bool resolveCommand(Token* tokens, size_t len);
Command getCmdType(const char* name);
bool isRunning();
bool isShowTimeEnabled();
bool isAnswerInFrcation();
