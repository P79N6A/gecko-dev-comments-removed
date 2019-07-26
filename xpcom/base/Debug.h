




#ifndef mozilla_Debug_h
#define mozilla_Debug_h

#include "nsString.h"
#include <stdio.h>

namespace mozilla {

typedef uint32_t LogOptions;


const LogOptions kPrintToStream   = 1 << 0;



const LogOptions kPrintToDebugger = 1 << 1;



const LogOptions kPrintInfoLog    = 1 << 2;



const LogOptions kPrintErrorLog   = 1 << 3;


const LogOptions kPrintNewLine    = 1 << 4;


void PrintToDebugger(const nsAString& aStr, FILE* aStream,
                     LogOptions aOptions = kPrintToStream
                                         | kPrintToDebugger
                                         | kPrintInfoLog);

} 

#endif 
