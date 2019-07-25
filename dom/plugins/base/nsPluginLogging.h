






































#ifndef nsPluginLogging_h__
#define nsPluginLogging_h__

#define FORCE_PR_LOG
#define PR_LOGGING 1

#ifdef PR_LOGGING
#include "prlog.h"

#ifndef PLUGIN_LOGGING    
#define PLUGIN_LOGGING 1  // master compile-time switch for pluging logging
#endif









#define NPN_LOG_NAME    "PluginNPN"
#define NPP_LOG_NAME    "PluginNPP"
#define PLUGIN_LOG_NAME "Plugin"


#define PLUGIN_LOG_ALWAYS 1
#define PLUGIN_LOG_BASIC  3
#define PLUGIN_LOG_NORMAL 5
#define PLUGIN_LOG_NOISY  7
#define PLUGIN_LOG_MAX    9












#ifdef PLUGIN_LOGGING

class nsPluginLogging
{
public:
  static PRLogModuleInfo* gNPNLog;  
  static PRLogModuleInfo* gNPPLog;  
  static PRLogModuleInfo* gPluginLog;  
};

#endif   

#endif  


#ifdef PLUGIN_LOGGING
 #define NPN_PLUGIN_LOG(a, b)                              \
   PR_BEGIN_MACRO                                        \
   PR_LOG(nsPluginLogging::gNPNLog, a, b); \
   PR_LogFlush();                                                    \
   PR_END_MACRO
#else
 #define NPN_PLUGIN_LOG(a, b)
#endif

#ifdef PLUGIN_LOGGING
 #define NPP_PLUGIN_LOG(a, b)                              \
   PR_BEGIN_MACRO                                         \
   PR_LOG(nsPluginLogging::gNPPLog, a, b); \
   PR_LogFlush();                                                    \
   PR_END_MACRO
#else
 #define NPP_PLUGIN_LOG(a, b)
#endif

#ifdef PLUGIN_LOGGING
 #define PLUGIN_LOG(a, b)                              \
   PR_BEGIN_MACRO                                         \
   PR_LOG(nsPluginLogging::gPluginLog, a, b); \
   PR_LogFlush();                                                    \
   PR_END_MACRO
#else
 #define PLUGIN_LOG(a, b)
#endif

#endif   

