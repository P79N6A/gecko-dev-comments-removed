

























#include "XmlTraceLog.h"



using namespace graphite2;

extern "C" {


bool graphite_start_logging(GR_MAYBE_UNUSED FILE * logFile, GR_MAYBE_UNUSED GrLogMask mask)
{
#ifdef DISABLE_TRACING
    return false;
#else	
    if (XmlTraceLog::sLog != &XmlTraceLog::sm_NullLog)
    {
        delete XmlTraceLog::sLog;
    }
    XmlTraceLog::sLog = new XmlTraceLog(logFile, "http://projects.palaso.org/graphite2", mask);
    return (XmlTraceLog::sLog != NULL);
#endif		
}

void graphite_stop_logging()
{
#ifndef DISABLE_TRACING
    if (XmlTraceLog::sLog && XmlTraceLog::sLog != &XmlTraceLog::sm_NullLog)
    {
        delete XmlTraceLog::sLog;
        XmlTraceLog::sLog = &XmlTraceLog::sm_NullLog;
    }
#endif		
}


} 
