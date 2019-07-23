





































#ifndef mozilla_PluginLibrary_h
#define mozilla_PluginLibrary_h 1

#include "prlink.h"
#include "npapi.h"
#include "nscore.h"

class nsNPAPIPlugin;

namespace mozilla {

class PluginLibrary
{
public:
  virtual ~PluginLibrary() { }

  



  virtual void SetPlugin(nsNPAPIPlugin* plugin) = 0;

  virtual bool HasRequiredFunctions() = 0;

#if defined(XP_UNIX) && !defined(XP_MACOSX)
  virtual nsresult NP_Initialize(NPNetscapeFuncs* bFuncs, NPPluginFuncs* pFuncs, NPError* error) = 0;
#else
  virtual nsresult NP_Initialize(NPNetscapeFuncs* bFuncs, NPError* error) = 0;
#endif
  virtual nsresult NP_Shutdown(NPError* error) = 0;
  virtual nsresult NP_GetMIMEDescription(char** mimeDesc) = 0;
  virtual nsresult NP_GetValue(void *future, NPPVariable aVariable,
                               void *aValue, NPError* error) = 0;
#if defined(XP_WIN) || defined(XP_MACOSX)
  virtual nsresult NP_GetEntryPoints(NPPluginFuncs* pFuncs, NPError* error) = 0;
#endif
  virtual nsresult NPP_New(NPMIMEType pluginType, NPP instance,
                           uint16_t mode, int16_t argc, char* argn[],
                           char* argv[], NPSavedData* saved,
                           NPError* error) = 0;
};


} 

#endif  
