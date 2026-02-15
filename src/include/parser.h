#pragma once

#include <stdbool.h>
#include "registry.h"
#include "lexer.h"

void initParser();
void freeParser();
Function* parse(Token* t, int startIdx, size_t len);
void freeLeftOutStrings(bool isCalledDuringEval);
