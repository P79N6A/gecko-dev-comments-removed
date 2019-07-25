








































#ifndef JSRegExp_h
#define JSRegExp_h

#include "yarr/jswtfbridge.h"

struct JSRegExp;
struct JSContext;

enum JSRegExpIgnoreCaseOption { JSRegExpDoNotIgnoreCase, JSRegExpIgnoreCase };
enum JSRegExpMultilineOption { JSRegExpSingleLine, JSRegExpMultiline };


const int JSRegExpErrorNoMatch = -1;
const int JSRegExpErrorHitLimit = -2;
const int JSRegExpErrorInternal = -4;

JSRegExp* jsRegExpCompile(const UChar* pattern, int patternLength,
    JSRegExpIgnoreCaseOption, JSRegExpMultilineOption,
    unsigned* numSubpatterns, int *error);

int jsRegExpExecute(JSContext *, const JSRegExp*,
    const UChar* subject, int subjectLength, int startOffset,
    int* offsetsVector, int offsetsVectorLength);

void jsRegExpFree(JSRegExp*);

#endif
