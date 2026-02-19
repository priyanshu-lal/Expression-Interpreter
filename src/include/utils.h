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
bool evaluateInput(const char* input);
bool isRunning(void);
void loadCommands();
void unloadCommands();
void displayHelpAndUsageGuide();
