#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "logger.h"
#include "lexer.h"

extern const char* g_source;  // defined in lexer.c
int g_errorCount;

static int getStyledTextLen(const char* str);

void printStyledText(const char* str) {
	for (size_t i = 0; str[i] != '\0'; ++i) {
		if (str[i] == '<') {
			if (str[i + 1] == '~' && str[i + 2] != '\0' && str[i + 3] == '>') {
				switch (str[i + 2]) {
				case 'b':
					changeTextStyle(STYLE_BOLD);
					i += 3; break;

				case 'u':
					changeTextStyle(STYLE_UNDERLINE);
					i += 3; break;

				case 'd':
					changeTextStyle(STYLE_DIM);
					i += 3; break;

				case 'r':
					changeTextStyle(STYLE_REVERSE);
					i += 3; break;

				case 'l':
					changeTextStyle(STYLE_BLINK);
					i += 3; break;
					
				default:
					putchar(str[i]);
					break;
				}
			}
			else if (str[i + 1] != '\0' && str[i + 2] == '>') {
				switch (str[i + 1]) {
				case 'r':
					changeTextColor(COLOR_RED);
					i += 2; break;

				case 'b':
					changeTextColor(COLOR_BLUE);
					i += 2; break;

				case 'g':
					changeTextColor(COLOR_GREEN);
					i += 2; break;

				case 'y':
					changeTextColor(COLOR_YELLOW);
					i += 2; break;

				case 'c':
					changeTextColor(COLOR_CYAN);
					i += 2; break;

				case 'm':
					changeTextColor(COLOR_MAGENTA);
					i += 2; break;

				case '/':
					resetTextAttribute();
					i += 2; break;

				default:
					putchar(str[i]);
					break;
				}
			}
			else {
				putchar(str[i]);
			}
		}
		else {
			putchar(str[i]);
		}
	}
	resetTextAttribute();
}

const char* fstring(const char* fmt, ...) {
    static char buffer[1024];  
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    return buffer;
}

static void markError(int idx, int area, TextColor lineCol) {
    if (idx >= (int)strlen(g_source)) return;
    int j;
	changeTextColor(lineCol);
    printf("\n | ");

	resetTextAttribute();
    for (j = 0; j < idx; j++) {
        putchar(g_source[j]);
    }
	changeTextColor(COLOR_YELLOW);
    for (j = idx; j < area + idx; j++) {
        putchar(g_source[j]);
    }
	resetTextAttribute();
    printf("%s\n", g_source + idx + area);
	changeTextColor(lineCol);
	printf(" | ");
	resetTextAttribute();

    for (j = 0; j < idx; j++) {
        putchar(' ');
    }
	changeTextColor(COLOR_GREEN);
	if (area == 1) {
		putchar('^');
	}
	else {
    	for (j = 0; j < area; j++) {
    	    putchar('~');
    	}
	}
	resetTextAttribute();
    putchar('\n');
}

void displayWarningMsg(const char *msg) {
	changeTextColor(COLOR_YELLOW);
	printf("warning: ");
	resetTextAttribute();
	printStyledText(msg);
	putchar('\n');
}

void displayWarning(struct Token tk, const char* msg) {
	markError(tk.start, tk.len, COLOR_YELLOW);
	displayWarningMsg(msg);
}

void printStyledTextInBox(const char* msg) {
	const int LEN = getStyledTextLen(msg) + 2;
	changeTextColor(COLOR_CYAN);
	printf("╭");
	for (int i = 0; i < LEN; ++i) {
		printf("─");
	}
	printf("╮\n│ ");
	resetTextAttribute();
	printStyledText(msg);
	changeTextColor(COLOR_CYAN);
	printf(" │\n╰");
	for (int i = 0; i < LEN; ++i) {
		printf("─");
	}
	printf("╯\n");
	resetTextAttribute();
}

void displayHint(const char *msg) {
	changeTextColor(COLOR_CYAN);
	printf("hint: ");
	resetTextAttribute();
	printStyledText(msg);
	putchar('\n');
}

void displayError(Token tk, const char *msg) {
    markError(tk.start, tk.len, COLOR_RED);
	changeTextColor(COLOR_RED);
	printf("error: ");
	resetTextAttribute();
    printStyledText(msg);
	putchar('\n');
    g_errorCount++;
}

void displayNote(struct Token tk, const char* msg) {
	markError(tk.start, tk.len, COLOR_CYAN);
	changeTextColor(COLOR_CYAN);
	printf("note: ");
	resetTextAttribute();
	printStyledText(msg);
	putchar('\n');
}

void displayNoteMsg(const char* msg) {
	changeTextColor(COLOR_CYAN);
	printf("note: ");
	resetTextAttribute();
	printStyledText(msg);
	putchar('\n');
}

void displayErrorAt(const char *msg, int start, int area) {
    if (start != -1) markError(start, area, COLOR_RED);
    changeTextColor(COLOR_RED);
	printf("error: ");
	resetTextAttribute();
    printStyledText(msg);
	putchar('\n');
    g_errorCount++;
}

void displayErrorMsg(const char *msg) {
	changeTextColor(COLOR_RED);
	printf("error: ");
	resetTextAttribute();
	printStyledText(msg);
	putchar('\n');
	g_errorCount++;
}

#if defined(__linux__) || (defined(__APPLE__) && defined(__MACH__)) || defined(__unix__)

#include <unistd.h>

#define C_BLACK   "\033[0;30m"
#define C_WHITE   "\033[0;37m"

void changeTextColor(TextColor col) {
	if (!isatty(STDOUT_FILENO)) return;

	switch (col) {
		case COLOR_RED:
			printf("\033[0;31m");
			break;

		case COLOR_GREEN:
			printf("\033[0;32m");
			break;

		case COLOR_BLUE:
			printf("\033[0;34m");
			break;

		case COLOR_CYAN:
			printf("\033[0;36m");
			break;

		case COLOR_YELLOW:
			printf("\033[0;33m");
			break;

		case COLOR_MAGENTA:
			printf("\033[0;35m");
			break;
	}
}

void resetTextAttribute() {
	if (isatty(STDOUT_FILENO))
		printf("\033[0m");
}

void changeTextStyle(TextStyle style) {
	if (!isatty(STDOUT_FILENO)) return;
	
	switch (style) {
		case STYLE_BOLD:
			printf("\033[1m");
			break;

		case STYLE_DIM:
			printf("\033[2m");
			break;

		case STYLE_UNDERLINE:
			printf("\033[4m");
			break;

		case STYLE_BLINK:
			printf("\033[5m");
			break;

		case STYLE_REVERSE:
			printf("\033[7m");
			break;

		default: break;
	}
}

#elif defined(_WIN32) || defined(_WIN64)
// Windows types
typedef void* HANDLE;
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef int BOOL;

// Standard handles
#define STD_OUTPUT_HANDLE ((DWORD)-11)

// Console attributes
#define FOREGROUND_BLUE      0x0001
#define FOREGROUND_GREEN     0x0002
#define FOREGROUND_RED       0x0004
#define FOREGROUND_INTENSITY 0x0008

#define BACKGROUND_BLUE      0x0010
#define BACKGROUND_GREEN     0x0020
#define BACKGROUND_RED       0x0040
#define BACKGROUND_INTENSITY 0x0080

__declspec(dllimport) HANDLE __stdcall GetStdHandle(DWORD nStdHandle);
__declspec(dllimport) BOOL __stdcall SetConsoleTextAttribute(HANDLE hConsoleOutput, WORD wAttributes);

static HANDLE hConsole;
static bool handleInitialized = false;

#warning "Only reverse text styling is supported on WIN32 platform"

void changeTextStyle(TextStyle style) {
	if (!handleInitialized) {
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		handleInitialized = true;
	}

	if (style == STYLE_REVERSE) {
		SetConsoleTextAttribute(hConsole, BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_GREEN | FOREGROUND_INTENSITY);
	}
}

void changeTextColor(TextColor col) {
	if (!handleInitialized) {
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		handleInitialized = true;
	}
	switch (col) {
		case COLOR_RED:
			SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
			break;

		case COLOR_GREEN:
			SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			break;

		case COLOR_BLUE:
			SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_BLUE);
			break;

		case COLOR_CYAN:
			SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
			break;

		case COLOR_YELLOW:
			SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			break;

		case COLOR_MAGENTA:
			SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
			break;
	}
}

void resetTextAttribute() {
	SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

#else

#error "Unsupported platform, supported platforms include: Linux, MacOS, Windows and BSD"
#endif

static int getStyledTextLen(const char* str) {
	int len = 0;
	for (size_t i = 0; str[i] != '\0'; ++i) {
		if (str[i] == '<') {
			if (str[i + 1] == '~' && str[i + 2] != '\0' && str[i + 3] == '>') {
				switch (str[i + 2]) {
				case 'b':
				case 'u':
				case 'd':
				case 'r':
				case 'l':
					i += 3;
					break;
					
				default:
					++len;
					break;
				}
			}
			else if (str[i + 1] != '\0' && str[i + 2] == '>') {
				switch (str[i + 1]) {
				case 'r':
				case 'b':
				case 'g':
				case 'y':
				case 'c':
				case 'm':
				case '/':
					i += 2;
					break;

				default:
					++len;
					break;
				}
			}
			else {
				++len;
			}
		}
		else {
			++len;
		}
	}
	return len;
}
