#pragma once

#include "lexer.h"

char* StringInput(const char* msg);
const char* fractionalApproximation(double num);
void printStartingMsg();
void displayVariables();
void displayBuiltInFunctions();
void displayUserFunctions();
void displayHelpAndUsageGuide();

void runInlineInputs(int argc, char* argv[]);
void resolveCommand(Token* tokens, size_t len, bool* isRunning);
void evaluateInput(Token* tokens, size_t startIdx, size_t len);
void loadCommands();
void unloadCommands();
void displayHelpAndUsageGuide();
