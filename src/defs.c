#include "registry.h"
#include "logger.h"
#include "container.h"
#include "allocator.h"

#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>

static int __toRadian();
static int __toDegree();
static int fib();
static int _sqrt();
static int _cbrt();
static int factorialFn();
static int sinFn();
static int cosFn();
static int tanFn();
static int floorFn();
static int ceilFn();
static int roundFn();
static int expFn();
static int logFn();
static int log2Fn();
static int lnFn();
static int asinFn();
static int acosFn();
static int atanFn();
static int permutation();
static int combination();
static int mean();
static int lcm();
static int rootFn();
static int distanceFn();
static int slopeFn();
static int logBase();
static int maxFn();
static int minFn();
static int sumFn();
static int truncateFn();
static int sinhFn();
static int coshFn();
static int tanhFn();
static int clampFn();
static int copySignFn();
static int asinhFn();
static int acoshFn();
static int atanhFn();
static int randomFn();
static int atan2Fn();
static int productFn();

#define ARG_LEN 16
const char* ARGNAMES[ARG_LEN] = {
	"a", "b", "c", "d", "e", "f",
	"g", "h", "j", "k", "l", "m",
	"n", "o", "p", "q"
};

static void newBuiltinFunction(char* key, CalcFn ptr, bool argNameProvided, unsigned argsCount, ...) {
	BuiltinFunction* fn = arena_alloc(g_globArena, sizeof(BuiltinFunction));
	*fn = (BuiltinFunction) {key, NULL, ptr, argsCount, false};
	
	if (argsCount == UINT_MAX) {
		fn->isVariadic = true;
	}
	else if (argNameProvided) {
		va_list args;
		va_start(args, argsCount);
		fn->argNames = (const char**)arena_alloc(g_globArena, sizeof(char*) * argsCount);
		for (unsigned i = 0; i < argsCount; ++i) {
			fn->argNames[i] = va_arg(args, char*);
		}
		va_end(args);
	}
	else if (argsCount == 1) {
		fn->argNames = &ARGNAMES[12];
	}
	else if (argsCount <= ARG_LEN) {
		fn->argNames = ARGNAMES;
	}
	hashmap_set(g_functions, &fn);
}

static void setVariables() {
	hashmap_set(g_variables, &(Variable){str_from_pool("pi"), 3.141592653589793});
	hashmap_set(g_variables, &(Variable){str_from_pool("phi"), 1.618033988749895});
	hashmap_set(g_variables, &(Variable){str_from_pool("R"), 8.3144621});
	hashmap_set(g_variables, &(Variable){str_from_pool("e"), 2.7182818284590452354});
	hashmap_set(g_variables, &(Variable){str_from_pool("inf"), INFINITY});
}

static void setFunctions() {
	newBuiltinFunction("sqrt", _sqrt, false, 1u);
	newBuiltinFunction("fib", fib, true, 1u, "range");
	newBuiltinFunction("factorial", factorialFn, false, 1u);
	newBuiltinFunction("cbrt", _cbrt, false, 1);
	newBuiltinFunction("sin", sinFn, false, 1);
	newBuiltinFunction("cos", cosFn, false, 1);
	newBuiltinFunction("tan", tanFn, false, 1);
	newBuiltinFunction("log10", logFn, false, 1);
	newBuiltinFunction("log", logBase, true, 2, "base", "num");
	newBuiltinFunction("log2", log2Fn, false, 1);
	newBuiltinFunction("asin", asinFn, false, 1);
	newBuiltinFunction("acos", acosFn, false, 1);
	newBuiltinFunction("atan", atanFn, false, 1);
	newBuiltinFunction("round", roundFn, false, 1);
	newBuiltinFunction("floor", floorFn, false, 1);
	newBuiltinFunction("ceil", ceilFn, false, 1);
	newBuiltinFunction("nPr", permutation, true, 2, "n", "r");
	newBuiltinFunction("nCr", combination, true, 2, "n", "r");
	newBuiltinFunction("rad", __toRadian, true, 1, "angle");
	newBuiltinFunction("deg", __toDegree, true, 1, "angle");
	newBuiltinFunction("exp", expFn, false, 1);
	newBuiltinFunction("ln", lnFn, false, 1);
	newBuiltinFunction("mean", mean, false, UINT_MAX);
	newBuiltinFunction("lcm", lcm, false, UINT_MAX);
	newBuiltinFunction("max", maxFn, false, UINT_MAX);
	newBuiltinFunction("min", minFn, false, UINT_MAX);
	newBuiltinFunction("root", rootFn, true, 2, "n", "base");
	newBuiltinFunction("distance", distanceFn, true, 4
		, "x1", "y1", "x2", "y2");
	newBuiltinFunction("slope", slopeFn, false, 4);
	newBuiltinFunction("sum", sumFn, false, UINT_MAX);
	newBuiltinFunction("truncate", truncateFn, false, 1);
	newBuiltinFunction("sinh", sinhFn, false, 1);
	newBuiltinFunction("cosh", coshFn, false, 1);
	newBuiltinFunction("tanh", tanhFn, false, 1);
	newBuiltinFunction("clamp", clampFn, true, 3, "value", "min", "max");
	newBuiltinFunction("copy_sign", copySignFn, true, 2, "dest", "src");
	newBuiltinFunction("asinh", asinhFn, false, 1);
	newBuiltinFunction("acosh", acoshFn, false, 1);
	newBuiltinFunction("atanh", atanhFn, false, 1);
	newBuiltinFunction("random", randomFn, true, 2, "start", "end");
	newBuiltinFunction("atan2", atan2Fn, true, 2, "x", "y");
	newBuiltinFunction("product", productFn, false, UINT_MAX);
}

static void populateSymbolTable() {
	size_t i = 0;
	const BuiltinFunction** fnPtr;
	while (hashmap_iter(g_functions, &i, (void**)&fnPtr)) {
		hashmap_set(g_symbolTable, &(SymbolType){(*fnPtr)->key, BUILTIN_FUNCTION});
	}

	i = 0;
	Variable* v;
	while (hashmap_iter(g_variables, &i, (void**)&v)) {
		hashmap_set(g_symbolTable, &(SymbolType){v->key, VARIABLE});
	}
}

void loadSymbols() {
	srand(time(0));
	setVariables();
	setFunctions();
	populateSymbolTable();
}

//--------------------------------------
static enum AngleUnit s_angleUnit;

enum AngleUnit getAngleUnit() { return s_angleUnit; }
void setAngleUnit(enum AngleUnit unit) { s_angleUnit = unit; }

// ************ Definitions ************
static inline double toRadian(double n) { return n / 180.0 * 3.14159265358979323846; }
static inline double toDegree(double n) { return n * 180.0 / 3.14159265358979323846; }

static double gcd(double a, double b) {
	double tmp;
	while (b != 0) {
		tmp = b;
		b = fmod(a, b);
		a = tmp;
	}
	return a;
}

extern int printf(const char*, ...);

static inline bool verifyArgCount(const char* fnName) {
	if (NumVecTop(st) < (double)INT_MAX) {
		return true;
	}
	putchar('\n');
	displayErrorMsg(fstring("(variadic function: %s) -> invalid argument count", fnName));
	return false;
}

static int __toRadian() {
	NumVecPush(st, toRadian(NumVecPopBack(st)));
	return 1;
}

static int __toDegree() {
	NumVecPush(st, toDegree(NumVecPopBack(st)));
	return 1;
}

static int lcm() {
	if (!verifyArgCount("lcm")) return 0;
	int args = (int)NumVecPopBack(st);
	double a = NumVecPopBack(st), b;
	for (int i = 1; i < args; i++) {
		b = NumVecPopBack(st);
		a = a / gcd(a, b) * b;;
	}
	NumVecPush(st, a);
	return 1;
}

// static int clampFn() {
// 	double n1, n2, n3;
// 	n3 = NumVecPopBack(st);
// 	n2 = NumVecPopBack(st);
// 	n1 = NumVecPopBack(st);
	
// }

static int rootFn() {
	double p = NumVecPopBack(st), n = NumVecPopBack(st);
	if (n < 0.0) {
		displayErrorMsg(fstring("root (%g, %g) => can't find root of negative number", n, p));
		return 0;
	}
	NumVecPush(st, pow(n, 1.0 / p));
	return 1;
}

static int maxFn() {
	if (!verifyArgCount("max")) return 0;
	int args = (int)NumVecPopBack(st);
	double m = NumVecPopBack(st), n;
	for (int i = 1; i < args; i++) {
		n = NumVecPopBack(st);
		m = m > n ? m : n;
	}
	NumVecPush(st, m);
	return 1;
}

static int minFn() {
	if (!verifyArgCount("min")) return 0;
	int args = (int)NumVecPopBack(st);
	double m = NumVecPopBack(st), n;
	for (int i = 1; i < args; i++) {
		n = NumVecPopBack(st);
		m = m < n ? m : n;
	}
	NumVecPush(st, m);
	return 1;
}

static int sumFn() {
	if (!verifyArgCount("sum")) return 0;
	int args = (int)NumVecPopBack(st);
	double s = 0.0;
	for (int i = 0; i < args; i++) {
		s += NumVecPopBack(st);
	}
	NumVecPush(st, s);
	return 1;
}

static int distanceFn() {
	double x1, y1, x2, y2;
	y2 = NumVecPopBack(st);
	x2 = NumVecPopBack(st);
	y1 = NumVecPopBack(st);
	x1 = NumVecPopBack(st);
	NumVecPush(st, sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2)));
	return 1;
}

static int slopeFn() {
	double x1, y1, x2, y2;
	y2 = NumVecPopBack(st);
	x2 = NumVecPopBack(st);
	y1 = NumVecPopBack(st);
	x1 = NumVecPopBack(st);
	NumVecPush(st, (y2 - y1) / (x2 - x1));
	return 1;
}

static int logBase() {
	double n2 = NumVecPopBack(st), n1 = NumVecPopBack(st);
	NumVecPush(st, log(n2) / log(n1));
	return 1;
}

static int fib() {
	double n = NumVecPopBack(st);
	if (n != floor(n)) {
		putchar('\n');
		displayErrorMsg(fstring("fib(%g) => input is in fraction, expected an integer", n));
		return 0;
	}
	if (n < 0.0) {
		putchar('\n');
		displayErrorMsg(fstring("fib(%g) => expected a positive integer", n));
		return 0;
	}
	if (n > 1000.0) {
		putchar('\n');
		displayErrorMsg(fstring("fib(%g) => input is out of range", n));
		return 0;
	}
	if (n <= 1) {
		NumVecPush(st, n);
		return 1;
	}
	double a = 0, b = 1, tmp;
	const int num = (int) n;
	for (int i = 2; i <= num; i++) {
		tmp = a + b;
		a = b;
		b = tmp;
	}
	NumVecPush(st, b);
	return 1;
}

static int mean() {
	if (!verifyArgCount("mean")) return 0;
	int args = (int) NumVecPopBack(st);
	double s = 0.0;
	for (int i = 0; i < args; ++i) {
		s += NumVecPopBack(st);
	}
	NumVecPush(st, s / (double)args);
	return 1;
}

static int productFn() {
	if (!verifyArgCount("product")) return 0;
	int args = (int) NumVecPopBack(st);
	double p = 1.0;
	for (int i = 0; i < args; ++i) {
		p *= NumVecPopBack(st);
	}
	NumVecPush(st, p);
	return 1;
}

static int _sqrt() {
	double num = NumVecPopBack(st);
	if (num < 0.0) {
		putchar('\n');
		displayErrorMsg(fstring("Negative numbers have no real square root (sqrt(%g)) is undefined", num));
		displayHint("try surrounding the expression after `sqrt` with `|` in order to get the absolute value");
		return 0;
	}
	NumVecPush(st, sqrt(num));
	return 1;
}

static int _cbrt() {
	NumVecPush(st, cbrt(NumVecPopBack(st)));
	return 1;
}

static inline double doubleAbs(double n) { 
	if (n < 0.0) return n * -1;
	return n;
}

double factorial(double val) {
	static const double TOLERANCE = 1.e-6;
	if (isnan(val)) return val;
	double num = floor(val);

	if (val < 0.0) {
		putchar('\n');
		displayErrorMsg(
			fstring("Factorial is not defined for negative numbers (%g! is undefined)", val)
		);
		displayHint("try surrounding the expression with `|` in order to get the absolute value");
		return NAN;
	}

	if (doubleAbs(val - num) > TOLERANCE) {
		putchar('\n');
		displayWarningMsg(fstring("Factorial is not defined for floating-point numbers. Therefore,\n"
			"         ignoring digit(s) after floating-point: (floor %g)! => %g!", val, num));
	}

	if (num > 160.0) {
		displayErrorMsg(fstring("%g! is too large to hold for 'double' data type", num));
		return NAN;
	}
	num += 0.01;
	double fac = 1.0;
	for (double i = 1.0; i <= num; i += 1.0) {
		fac *= i;
	}
	return fac;
}

static int atan2Fn() {
	double x = NumVecPopBack(st), y = NumVecPopBack(st);
	NumVecPush(st, atan2(x, y));
	return 1;
}

static int factorialFn() {
	double f = factorial(NumVecPopBack(st));
	if (isnan(f)) return 0;
	NumVecPush(st, f);
	return 1;
}

static int tanFn() {
	NumVecPush(st, getAngleUnit() == DEGREE ? tan(toRadian(NumVecPopBack(st))) : tan(NumVecPopBack(st)));
	return 1;
}

static int cosFn() {
	NumVecPush(st, getAngleUnit() == DEGREE ? cos(toRadian(NumVecPopBack(st))) : cos(NumVecPopBack(st)));
	return 1;
}

static int sinFn() {
	NumVecPush(st, getAngleUnit() == DEGREE ? sin(toRadian(NumVecPopBack(st))) : sin(NumVecPopBack(st)));
	return 1;
}

static int asinFn() {
	double n = NumVecPopBack(st);
	double res = asin(n);
	if (isnan(res)) {
		putchar('\n');
		displayErrorMsg(fstring("sin^-1 (%g) => input is out of range [-1, 1]", n));
		return 0;
	}
	NumVecPush(st, getAngleUnit() == DEGREE ? toDegree(res) : res);
	return 1;
}

static int combination() {
	double r = NumVecPopBack(st), n = NumVecPopBack(st);
	if (r < 0.0 || n < 0.0 || (n - r) < 0.0) {
		displayErrorMsg(fstring("nCr (%g, %g) => invalid input, can't find factorial of negative number", n, r));
		return 0;
	}
	double ans = factorial(n) / ((factorial(n - r)) * factorial(r));
	if (isnan(ans)) return 0;
	NumVecPush(st, ans);
	return 1;
}

static int permutation() {
	double r = NumVecPopBack(st), n = NumVecPopBack(st);
	if (r < 0.0 || n < 0.0 || (n - r) < 0.0) {
		displayErrorMsg(fstring("nCr (%g, %g) => invalid input, can't find factorial of negative number", n, r));
		return 0;
	}
	double ans = factorial(n) / (factorial(n - r));
	if (isnan(ans)) return 0;
	NumVecPush(st, ans);
	return 1;
}

static int acosFn() {
	double n = NumVecPopBack(st);
	double res = acos(n);
	if (isnan(res)) {
		putchar('\n');
		displayErrorMsg(fstring("cos^-1 (%g) => input is out of range [-1, 1]", n));
		return 0;
	}
	NumVecPush(st, getAngleUnit() == DEGREE ? toDegree(res) : res);
	return 1;
}

static int atanFn() {
	double n = NumVecPopBack(st);
	double res = atan(n);
	if (isnan(res)) {
		putchar('\n');
		displayErrorMsg(fstring("tan^-1 (%g) => input is not a real value", n));
		return 0;
	}
	NumVecPush(st, getAngleUnit() == DEGREE ? toDegree(res) : res);
	return 1;
}

static int floorFn() {
	NumVecPush(st, floor(NumVecPopBack(st)));
	return 1;
}

static int ceilFn() {
	NumVecPush(st, ceil(NumVecPopBack(st)));
	return 1;
}

static int roundFn() {
	NumVecPush(st, round(NumVecPopBack(st)));
	return 1;
}

static int expFn() {
	NumVecPush(st, exp(NumVecPopBack(st)));
	return 1;
}

static int logFn() {
	NumVecPush(st, log10(NumVecPopBack(st)));
	return 1;
}

static int log2Fn() {
	NumVecPush(st, log2(NumVecPopBack(st)));
	return 1;
}

static int lnFn() {
	NumVecPush(st, log(NumVecPopBack(st)));
	return 1;
}

static int truncateFn() {
	NumVecPush(st, trunc(NumVecPopBack(st)));
	return 1;
}

static int sinhFn(){
	NumVecPush(st, sinh(NumVecPopBack(st)));
	return 1;
}

static int coshFn(){
	NumVecPush(st, cosh(NumVecPopBack(st)));
	return 1;
}

static int tanhFn(){
	NumVecPush(st, tanh(NumVecPopBack(st)));
	return 1;
}

static int clampFn() {
	double maxVal = NumVecPopBack(st);
	double minVal = NumVecPopBack(st);
	double val = NumVecPopBack(st);
	if (val < minVal) NumVecPush(st, minVal);
	else if (val > maxVal) NumVecPush(st, maxVal);
	else NumVecPush(st, val);
	return 1;
}

static int copySignFn() {
	double n2 = NumVecPopBack(st), n1 = NumVecPopBack(st);
	NumVecPush(st, copysign(n1, n2));
	return 1;
}

static int asinhFn() {
	NumVecPush(st, asinh(NumVecPopBack(st)));
	return 1;
}

static int acoshFn() {
	NumVecPush(st, acosh(NumVecPopBack(st)));
	return 1;
}

static int atanhFn() {
	NumVecPush(st, acosh(NumVecPopBack(st)));
	return 1;
}

static int randomFn() {
	long u = (long)NumVecPopBack(st), l = (long)NumVecPopBack(st);
	NumVecPush(st, (rand() % (u - l + 1)) + l);
	return 1;
}
