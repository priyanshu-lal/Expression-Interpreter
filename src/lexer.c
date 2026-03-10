#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "lexer.h"
#include "logger.h"
#include "hashmap.h"

const char* g_source;

static Token* s_tokens;
static size_t s_start, s_current;
static size_t s_tkLen, s_tkCapacity;
static size_t s_idx;
static hashmap* s_keywordMap;

typedef struct {
	const char* keyword;
	unsigned int len;
	TokenKind keyType;
} KeywordEntry;

uint64_t keywordHashCallback(const void *item, uint64_t seed0, uint64_t seed1) {
	const KeywordEntry* entry = item;
	return hashmap_xxhash3(entry->keyword, entry->len, seed0, seed1);
}

int keywordCompareCallback(const void* a, const void* b, void* udata) {
	const KeywordEntry* e1 = a;
	const KeywordEntry* e2 = b;
	if (e1->len != e2->len) return 1;
	return memcmp(e1->keyword, e2->keyword, e1->len);
}

void initLexer() {
	s_tkCapacity = 2 << 8;
	s_tokens = (Token*) malloc(sizeof(Token) * s_tkCapacity);

	s_keywordMap = hashmap_new(sizeof(KeywordEntry), 32, 0xABC123456789ULL, 0xDeadFacadeULL, 
		keywordHashCallback, keywordCompareCallback, NULL, NULL);

	hashmap_set(s_keywordMap, &(KeywordEntry) {"and", 3, TK_KW_AND});
	hashmap_set(s_keywordMap, &(KeywordEntry) {"or", 2, TK_KW_OR});
	hashmap_set(s_keywordMap, &(KeywordEntry) {"not", 3, TK_KW_NOT});
	hashmap_set(s_keywordMap, &(KeywordEntry) {"if", 2, TK_KW_IF});
	hashmap_set(s_keywordMap, &(KeywordEntry) {"else", 4, TK_KW_ELSE});
	hashmap_set(s_keywordMap, &(KeywordEntry) {"def", 3, TK_KW_DEF});
	hashmap_set(s_keywordMap, &(KeywordEntry) {"ans", 3, TK_KW_ANS});
	hashmap_set(s_keywordMap, &(KeywordEntry) {"of", 2, TK_KW_OF});
	hashmap_set(s_keywordMap, &(KeywordEntry) {"true", 4, TK_KW_TRUE});
	hashmap_set(s_keywordMap, &(KeywordEntry) {"false", 5, TK_KW_FALSE});
	hashmap_set(s_keywordMap, &(KeywordEntry) {"is", 2, TK_EQUALITY});
	hashmap_set(s_keywordMap, &(KeywordEntry) {"be", 2, TK_EQUAL});
	hashmap_set(s_keywordMap, &(KeywordEntry) {"let", 3, TK_KW_LET});
	hashmap_set(s_keywordMap, &(KeywordEntry) {"as", 2, TK_ARROW});
	hashmap_set(s_keywordMap, &(KeywordEntry) {"alias", 5, TK_KW_ALIAS});
}

void freeLexer() {
	free(s_tokens);
	hashmap_free(s_keywordMap);
}

void resetLexer() {
	s_start = 0;
	s_current = 0;
	s_idx = 0;
	s_tkLen = 0;
	g_errorCount = 0;
}

char* tkToString(const Token* tk) {
	static char tkString[256];
	if (tk->len > 254) {
		displayError(*tk, "size of a single token cannot exceed 254 characters");
		return NULL;
	}
	if (tk->len < 12) {
		for (size_t i = 0; i < tk->len; ++i) {
			tkString[i] = g_source[tk->start + i];
		}
	}
	else {
		memcpy(tkString, g_source + tk->start, tk->len);
	}
	tkString[tk->len] = '\0';
	return tkString;
}

char* getIdentifier(const Token tk) {
	if (tk.type != TK_IDENTIFIER) {
		displayErrorMsg("getIdentifier(token) was called with wrong token type");
		abort();
	}
	static char identStr[16];
	for (size_t i = 0; i < tk.len; ++i) {
		identStr[i] = g_source[tk.start + i];
	}
	identStr[tk.len] = '\0';
	return identStr;
}

bool tkToNumber(const Token* tk, double* outNum) {
	char* str = tkToString(tk);
	char* endPtr;
	double value = strtod(str, &endPtr);

	if (errno == ERANGE) {
		if (value == HUGE_VAL || value == -HUGE_VAL) {
            displayError(*tk, "Overflow occurred.\n");
        } else {
            displayError(*tk, "Underflow occurred.\n");
        }
		return false;
	}
	else if (str == endPtr) {
		displayError(*tk, fstring("No numeric digits found in input: '%s'\n", str));
		return false;
	}
	else if (*endPtr != '\0') {
		displayError(*tk, fstring("Partial conversion: %lf (Trailing junk: '%s')\n", value, endPtr));
	}
	*outNum = value;
	return true;
}

static inline bool isDigit(char ch) {
	return ch >= '0' && ch <= '9';
}

static inline bool isAlpha(char ch) {
	return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_';
}

static bool match(char expected) {
	if (g_source[s_current] == '\0') return false;
	if (g_source[s_current] != expected) return false;
	s_current++;
	return true;
}

static void addToken(TokenKind type) {
	if (s_idx > s_tkCapacity) {
		s_tkCapacity *= 2;
		void* ptr = realloc(s_tokens, sizeof(Token) * s_tkCapacity);
		if (!ptr) {
			fprintf(stdin, "fatal error: failed to allocate more memory\n");
			abort();
		}
		s_tokens = (Token*) ptr;
	}
	s_tokens[s_idx++] = (Token) {type, s_start, s_current - s_start};
}

static bool tokenizeString() {
	char ch = g_source[s_current];
	while (isAlpha(ch) || isDigit(ch) || ch == '_') {
		ch = g_source[++s_current];
	}
	size_t strLen = s_current - s_start;
	if (strLen > 15) {
		displayErrorAt("length of identifiers cannot exceed 15 characters", s_start, strLen);
		return false;
	}
	if (strLen > 1 && strLen < 6) {
		const KeywordEntry* kEntry = hashmap_get(s_keywordMap,
			&(KeywordEntry){g_source + s_start, strLen, TK_IDENTIFIER});
		if (kEntry) {
			addToken(kEntry->keyType);
			return true;
		}
	}
	addToken(TK_IDENTIFIER);
	return true;
}

static bool tokenizeNumber() {
	bool pointFound = false, eFound = false;
	for (; g_source[s_current] != '\0'; s_current++) {
		const char ch = g_source[s_current];
		if (isDigit(ch)) {
			continue;
		}
		else if (ch == '.') {
			if (pointFound) {
				displayErrorAt("Inappropriate use of floating-point", s_current, 1);
				return false;
			}
			if (eFound) {
				displayErrorAt("Floating point is not allowed after 'e'", s_current, 1);
				return false;
			}
			pointFound = true;
		}
		else if (ch == 'e' || ch == 'E') {
			if (eFound) {
				displayErrorAt(fstring("`%c` is not expected here, removing it can fix this error", ch),
					s_current, 1);
				return false;
			}
			eFound = true;
		}
		else {
			break;
		}
	}
	if (g_source[s_current - 1] == '.') {
		displayErrorAt("Trailing floating-point is not allowed", s_current - 1, 1);
		return false;
	}
	if (g_source[s_current - 1] == 'e' || g_source[s_current - 1] == 'E') {
		if ((g_source[s_current] == '+' || g_source[s_current] == '-') && isDigit(g_source[s_current + 1])) {
			for (s_current += 1; g_source[s_current] != '\0' && isDigit(g_source[s_current]); s_current++);
		}
		else if (g_source[s_current] == '\0' || !isDigit(g_source[s_current])) {
			displayErrorAt(fstring("Mantissa part is missing, remove `%c` to fix this error", g_source[s_current - 1]), s_current - 1, 1);
			return false;
		}
	}
	addToken(TK_NUMBER);
	return true;
}

Token* tokenize(const char* str, size_t* outLen) {
	if (!str) return NULL;
	resetLexer();
	g_source = str;

	while (g_source[s_current] != '\0') {
		s_start = s_current++;
		const char ch = g_source[s_start];
		switch (ch) {
		case '(': addToken(TK_OPEN_PAREN); break;
		case ')': addToken(TK_CLOSE_PAREN); break;
		case '|': addToken(TK_PIPE); break;
		case '%': addToken(TK_MOD); break;
		
		case '*':
			if (match('*')) addToken(TK_CARET);
			else addToken(TK_MUL);
			break;

		case '^': addToken(TK_CARET); break;
		case '+': addToken(TK_ADD); break;
		
		case '!':
			if (match('=')) addToken(TK_NOT_EQUAL);
			else addToken(TK_EXCLAIM);
			break;

		case ',': addToken(TK_COMMA); break;
		case ';': addToken(TK_SEMICOLON); break;
		case '_': addToken(TK_UNDERSCORE); break;
		
		case '-':
			if (match('>')) addToken(TK_ARROW);
			else addToken(TK_SUB);
			break;

		case '/':
			if (match('/')) addToken(TK_INT_DIV);
			else addToken(TK_DIV);
			break;

		case '=':
			if (match('=')) addToken(TK_EQUALITY);
			else addToken(TK_EQUAL);
			break;

		case '>':
			if (match('=')) addToken(TK_GT_EQ);
			else addToken(TK_GT);
			break;

		case '<':
			if (match('=')) addToken(TK_SM_EQ);
			else addToken(TK_SM);
			break;

		case '@': addToken(TK_AT_THE_RATE); break;
		case '$': addToken(TK_DOLLAR); break;

		case ' ': break;

		case '\0':
			s_tkLen = s_idx + 1;
			addToken(TK_EOL);
			*outLen = s_tkLen;
			return s_tokens;

		case '\t':
			displayErrorAt("Tab character is not allowed, use spaces", s_start, 1);
			return NULL;

		default:
			if (isAlpha(ch)) {
				if (!tokenizeString()) return NULL;
			}
			else if (isDigit(ch) || (ch == '.' && isDigit(g_source[s_current]))) {
				if (!tokenizeNumber()) return NULL;
			}
			else {
				displayErrorAt(fstring("Unexpected character: '%c'\n", ch), s_start, 1);
				return NULL;
			}
		}
	}
	s_tkLen = s_idx + 1;
	s_start = s_current;
	addToken(TK_EOL);
	*outLen = s_tkLen;
	return s_tokens;
}
