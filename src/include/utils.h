#pragma once

char* StringInput(const char* msg);
const char* fractionalApproximation(double num);
void printStartingMsg();
void displayHelpAndUsageGuide();

void runInlineInputs(int argc, char* argv[]);
bool evaluateInput(const char* input);
void initPlatform();
void clearScreen();
