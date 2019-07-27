




#ifndef mozilla_Sandbox_h
#define mozilla_Sandbox_h

#include "nsString.h"

enum MacSandboxType {
  MacSandboxType_Default = 0,
  MacSandboxType_Plugin,
  MacSandboxType_Invalid
};

enum MacSandboxPluginType {
  MacSandboxPluginType_Default = 0,
  MacSandboxPluginType_GMPlugin_Default,  
  MacSandboxPluginType_GMPlugin_OpenH264, 
  MacSandboxPluginType_GMPlugin_EME,      
  MacSandboxPluginType_Invalid
};

typedef struct _MacSandboxPluginInfo {
  _MacSandboxPluginInfo()
    : type(MacSandboxPluginType_Default) {}
  MacSandboxPluginType type;
  nsCString pluginPath;
  nsCString pluginBinaryPath;
} MacSandboxPluginInfo;

typedef struct _MacSandboxInfo {
  _MacSandboxInfo()
    : type(MacSandboxType_Default) {}
  MacSandboxType type;
  MacSandboxPluginInfo pluginInfo;
} MacSandboxInfo;

namespace mozilla {

bool StartMacSandbox(MacSandboxInfo aInfo, nsCString &aErrorMessage);

} 

#endif 
