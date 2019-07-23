





































#ifndef txLog_h__
#define txLog_h__

#include "prlog.h"
#include "prmem.h"
#endif

#ifdef PR_LOGGING
class txLog
{
public:
    static PRLogModuleInfo *xpath;
    static PRLogModuleInfo *xslt;
};

#define TX_LG_IMPL \
    PRLogModuleInfo * txLog::xpath = 0; \
    PRLogModuleInfo * txLog::xslt = 0;

#define TX_LG_CREATE \
    txLog::xpath = PR_NewLogModule("xpath"); \
    txLog::xslt  = PR_NewLogModule("xslt")

#else

#define TX_LG_IMPL
#define TX_LG_CREATE

#endif
