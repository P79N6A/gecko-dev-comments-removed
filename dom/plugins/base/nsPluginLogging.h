






#ifndef nsPluginLogging_h__
#define nsPluginLogging_h__

#include "mozilla/Logging.h"

#ifndef PLUGIN_LOGGING    
#define PLUGIN_LOGGING 1  // master compile-time switch for pluging logging
#endif









#define NPN_LOG_NAME    "PluginNPN"
#define NPP_LOG_NAME    "PluginNPP"
#define PLUGIN_LOG_NAME "Plugin"


#define PLUGIN_LOG_ALWAYS mozilla::LogLevel::Error
#define PLUGIN_LOG_BASIC  mozilla::LogLevel::Info
#define PLUGIN_LOG_NORMAL mozilla::LogLevel::Debug
#define PLUGIN_LOG_NOISY  mozilla::LogLevel::Verbose












#ifdef PLUGIN_LOGGING

class nsPluginLogging
{
public:
  static PRLogModuleInfo* gNPNLog;  
  static PRLogModuleInfo* gNPPLog;  
  static PRLogModuleInfo* gPluginLog;  
};

#endif   


#ifdef PLUGIN_LOGGING
 #define NPN_PLUGIN_LOG(a, b)                              \
   PR_BEGIN_MACRO                                        \
   MOZ_LOG(nsPluginLogging::gNPNLog, a, b); \
   PR_LogFlush();                                                    \
   PR_END_MACRO
#else
 #define NPN_PLUGIN_LOG(a, b)
#endif

#ifdef PLUGIN_LOGGING
 #define NPP_PLUGIN_LOG(a, b)                              \
   PR_BEGIN_MACRO                                         \
   MOZ_LOG(nsPluginLogging::gNPPLog, a, b); \
   PR_LogFlush();                                                    \
   PR_END_MACRO
#else
 #define NPP_PLUGIN_LOG(a, b)
#endif

#ifdef PLUGIN_LOGGING
 #define PLUGIN_LOG(a, b)                              \
   PR_BEGIN_MACRO                                         \
   MOZ_LOG(nsPluginLogging::gPluginLog, a, b); \
   PR_LogFlush();                                                    \
   PR_END_MACRO
#else
 #define PLUGIN_LOG(a, b)
#endif

#endif   

