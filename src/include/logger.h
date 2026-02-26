#pragma once

typedef enum {
    COLOR_RED, COLOR_GREEN, COLOR_BLUE,
    COLOR_CYAN, COLOR_YELLOW, COLOR_MAGENTA
} TextColor;

typedef enum {
    STYLE_BOLD,
    STYLE_DIM,
    STYLE_UNDERLINE,
    STYLE_BLINK,
    STYLE_REVERSE,
} TextStyle;

// ------------ Logging System ---------------
struct Token;
extern int g_errorCount;

const char* fstring(const char* fmt, ...);
void displayError(struct Token tk, const char* msg);
void displayErrorAt(const char* msg, int start, int area);
void displayErrorMsg(const char* msg);
void displayWarningMsg(const char* msg);
void displayWarning(struct Token tk, const char* msg);
void displayNote(struct Token tk, const char* msg);
void displayNoteMsg(const char* msg);
void displayHint(const char* msg);
//--------------------------------------------

// ------------ Text Color and Style modification Modification ---------------
void changeTextColor(TextColor);
void changeTextStyle(TextStyle);
void resetTextAttribute();
void printStyledText(const char* str);
void printStyledTextInBox(const char* msg);
// ----------------------------------------------------
