





#include "vm/TraceLoggingTypes.h"

class JSLinearString;

uint32_t
TLStringToTextId(JSLinearString *str)
{
#define NAME(textId) if (js::StringEqualsAscii(str, #textId)) return TraceLogger_ ## textId;
    TRACELOGGER_TREE_ITEMS(NAME)
    TRACELOGGER_LOG_ITEMS(NAME)
#undef NAME
    return TraceLogger_Error;
}

