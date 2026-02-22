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
#include "commands.h"

static void startTimer();
static double stopAndGetTimeInMs();

#if defined(_WIN32) || defined(_WIN64)

// Maximum trimming extra parts
#define WIN32_LEAN_AND_MEAN  // Excludes many APIs
#define NOMINMAX             // Prevents min/max macros conflicting with C++ std
#define NOKERNEL             // Excludes kernel APIs
#define NOUSER               // Excludes user APIs
#define NOSERVICE            // Excludes service APIs
#define NOSOUND              // Excludes sound APIs
#define NOMCX                // Excludes Media Control Extensions
#define NOGDI                // Excludes Graphics Device Interface
#include <windows.h>

static LARGE_INTEGER i_start, i_end, i_freq;

void initPlatform() {
	if (!SetConsoleOutputCP(CP_UTF8)) {
		printf("[error]: Console does not support UTF-8\n");
		exit(1);
	}

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE) return;
	
	DWORD dwMode = 0;
	if (GetConsoleMode(hOut, &dwMode)) {
		dwMode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
		SetConsoleMode(hOut, dwMode);
	}
}

static void startTimer() {
	if (!QueryPerformanceFrequency(&i_freq)) {
		printf("High-resolution performance counter not supported.\n");
		abort();
		return;
	}
	QueryPerformanceCounter(&i_start);
}

static double stopAndGetTimeInMs() {
	QueryPerformanceCounter(&i_end);
	double elapsedSeconds = (double)(i_end.QuadPart - i_start.QuadPart) / i_freq.QuadPart;
	return elapsedSeconds * 1e+3;
}

#elif defined(__linux__) || (defined(__APPLE__) && defined(__MACH__)) || defined(__unix__)

#include <time.h>
static struct timespec i_start, i_end;

void initPlatform() { return; }

static void startTimer() {
	clock_gettime(CLOCK_MONOTONIC, &i_start);
}

static double stopAndGetTimeInMs() {
	clock_gettime(CLOCK_MONOTONIC, &i_end);
	return ((i_end.tv_sec - i_start.tv_sec) * 1000.0) + ((i_end.tv_nsec - i_start.tv_nsec) / 1e+6);
}

#else
#error "Unsupported Platform"
#endif

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

void displayHelpAndUsageGuide() {
	changeTextColor(COLOR_YELLOW);
	printf(
		"\n     ██╗  █████╗  ██████╗ ███████╗"
		"\n     ██║ ██╔══██╗ ██╔══██╗██╔════╝"
		"\n     ██║ ███████║ ██║  ██║█████╗  "
		"\n██   ██║ ██╔══██║ ██║  ██║██╔══╝  "
		"\n╚█████╔╝ ██║  ██║ ██████╔╝███████╗"
		"\n ╚════╝  ╚═╝  ╚═╝ ╚═════╝ ╚══════╝\n");

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

bool evaluateInput(const char* input) {
	startTimer();
	size_t tokenLen = 0;
	Token* tokens = tokenize(input, strlen(input), &tokenLen);

	if (!tokens) return false;
	if (tokenLen == 1) return true;

	if (tokens[0].type == TK_AT_THE_RATE && tokenLen >= 3) {
		Command cmd;
		bool res = resolveCommand(tokens, tokenLen, &cmd);
		if (cmd != COMMAND_CLEAR && cmd != COMMAND_EXIT) putchar('\n');
		return res;
	}

	double timeInMs = 0.0;
	Function* ufn = parse(tokens, 0, tokenLen);

	if (ufn) {
		if (evaluate(ufn, getEvalMode())) {
			if (getEvalMode() == DIRECT && isShowTimeEnabled()) {
				timeInMs = stopAndGetTimeInMs();
			}

			Vector* accumulator = getAccumulator();
			for (size_t i = 0; i < accumulator->len; i++) {
				FinalResult* res = (FinalResult*)VecAt(accumulator, i);
				if (res->isBool) {
					printStyledText(fstring("<b>==> <g>%s\n", toBoolString(res->value)));
				}
				else if (isAnswerInFrcation()) {
					printStyledText(fstring("<b>==> <g>%s\n", fractionalApproximation(res->value)));
				}
				else {
					printStyledText(fstring("<b>==> <g>%g\n", res->value));
				}
			}

			if (getEvalMode() == DIRECT && isShowTimeEnabled()) {
				printStyledText(fstring(" <c>(in <b>%g<c> ms)\n", timeInMs));
			}
			return true;
		}
		else {
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
	return false;
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
		if ((tokens = tokenize(expr, strlen(expr), &tkLen)) == NULL) {
			return;
		}

		Function* ufn = parse(tokens, 0, tkLen);
		if (ufn && evaluate(ufn, getEvalMode())) {
			Vector* accumulator = getAccumulator();
			for (size_t i = 0; i < accumulator->len; i++) {
				FinalResult* res = (FinalResult*)VecAt(accumulator, i);
				if (res->isBool) {
					printStyledText(fstring("<b>==> <g>%s\n", toBoolString(res->value)));
				}
				else if (isAnswerInFrcation()) {
					printStyledText(fstring("<b>==> <g>%s\n", fractionalApproximation(res->value)));
				}
				else {
					printStyledText(fstring("<b>==> <g>%g\n", res->value));
				}
			}
		}
	}
}
