




#ifndef mozilla_Sandbox_h
#define mozilla_Sandbox_h

#include <string>

enum MacSandboxType {
  MacSandboxType_Default = 0,
  MacSandboxType_Plugin,
  MacSandboxType_Content,
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
  _MacSandboxPluginInfo(const struct _MacSandboxPluginInfo& other)
    : type(other.type), pluginPath(other.pluginPath),
      pluginBinaryPath(other.pluginBinaryPath) {}
  MacSandboxPluginType type;
  std::string pluginPath;
  std::string pluginBinaryPath;
} MacSandboxPluginInfo;

typedef struct _MacSandboxInfo {
  _MacSandboxInfo()
    : type(MacSandboxType_Default), level(0) {}
  _MacSandboxInfo(const struct _MacSandboxInfo& other)
    : type(other.type), level(other.level), pluginInfo(other.pluginInfo),
      appPath(other.appPath), appBinaryPath(other.appBinaryPath),
      appDir(other.appDir) {}
  MacSandboxType type;
  int32_t level;
  MacSandboxPluginInfo pluginInfo;
  std::string appPath;
  std::string appBinaryPath;
  std::string appDir;
} MacSandboxInfo;

namespace mozilla {

bool StartMacSandbox(MacSandboxInfo aInfo, std::string &aErrorMessage);

} 

#endif 
