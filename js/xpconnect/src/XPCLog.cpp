







#include "XPCLog.h"
#include "mozilla/Logging.h"
#include "prprf.h"
#include "mozilla/mozalloc.h"
#include <string.h>
#include <stdarg.h>


#ifdef DEBUG

#define SPACE_COUNT     200
#define LINE_LEN        200
#define INDENT_FACTOR   2

#define CAN_RUN (g_InitState == 1 || (g_InitState == 0 && Init()))

static char*    g_Spaces;
static int      g_InitState = 0;
static int      g_Indent = 0;
static PRLogModuleInfo* g_LogMod = nullptr;

static bool Init()
{
    g_LogMod = PR_NewLogModule("xpclog");
    g_Spaces = new char[SPACE_COUNT+1];
    if (!g_LogMod || !g_Spaces || !MOZ_LOG_TEST(g_LogMod,LogLevel::Error)) {
        g_InitState = 1;
        XPC_Log_Finish();
        return false;
    }
    memset(g_Spaces, ' ', SPACE_COUNT);
    g_Spaces[SPACE_COUNT] = 0;
    g_InitState = 1;
    return true;
}

void
XPC_Log_Finish()
{
    if (g_InitState == 1) {
        delete [] g_Spaces;
        
        g_LogMod = nullptr;
    }
    g_InitState = -1;
}

void
XPC_Log_print(const char* fmt, ...)
{
    va_list ap;
    char line[LINE_LEN];

    va_start(ap, fmt);
    PR_vsnprintf(line, sizeof(line)-1, fmt, ap);
    va_end(ap);
    if (g_Indent)
        PR_LogPrint("%s%s",g_Spaces+SPACE_COUNT-(INDENT_FACTOR*g_Indent),line);
    else
        PR_LogPrint("%s",line);
}

bool
XPC_Log_Check(int i)
{
    return CAN_RUN && MOZ_LOG_TEST(g_LogMod,LogLevel::Error);
}

void
XPC_Log_Indent()
{
    if (INDENT_FACTOR*(++g_Indent) > SPACE_COUNT)
        g_Indent-- ;
}

void
XPC_Log_Outdent()
{
    if (--g_Indent < 0)
        g_Indent++;
}

void
XPC_Log_Clear_Indent()
{
    g_Indent = 0;
}

#endif
