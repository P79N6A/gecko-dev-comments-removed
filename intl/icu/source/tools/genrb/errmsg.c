
















#include <stdarg.h>
#include <stdio.h>
#include "cstring.h"
#include "errmsg.h"

U_CFUNC void error(uint32_t linenumber, const char *msg, ...)
{
    va_list va;

    va_start(va, msg);
    fprintf(stderr, "%s:%u: ", gCurrentFileName, (int)linenumber);
    vfprintf(stderr, msg, va);
    fprintf(stderr, "\n");
    va_end(va);
}

static UBool gShowWarning = TRUE;

U_CFUNC void setShowWarning(UBool val)
{
    gShowWarning = val;
}

U_CFUNC UBool getShowWarning(){
    return gShowWarning;
}

static UBool gStrict =FALSE;
U_CFUNC UBool isStrict(){
    return gStrict;
}
U_CFUNC void setStrict(UBool val){
    gStrict = val;
}
static UBool gVerbose =FALSE;
U_CFUNC UBool isVerbose(){
    return gVerbose;
}
U_CFUNC void setVerbose(UBool val){
    gVerbose = val;
}
U_CFUNC void warning(uint32_t linenumber, const char *msg, ...)
{
    if (gShowWarning)
    {
        va_list va;

        va_start(va, msg);
        fprintf(stderr, "%s:%u: warning: ", gCurrentFileName, (int)linenumber);
        vfprintf(stderr, msg, va);
        fprintf(stderr, "\n");
        va_end(va);
    }
}
