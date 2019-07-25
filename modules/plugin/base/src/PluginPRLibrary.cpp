





































#include "mozilla/PluginPRLibrary.h"




#if defined(XP_WIN) && defined(_M_IX86)
#include <malloc.h>



static int gNotOptimized;
#define CALLING_CONVENTION_HACK void* foo = _alloca(gNotOptimized);
#else
#define CALLING_CONVENTION_HACK
#endif

namespace mozilla {

#if defined(XP_UNIX) && !defined(XP_MACOSX)
nsresult
PluginPRLibrary::NP_Initialize(NPNetscapeFuncs* bFuncs,
			       NPPluginFuncs* pFuncs, NPError* error)
{
  if (mNP_Initialize) {
    *error = mNP_Initialize(bFuncs, pFuncs);
  } else {
    NP_InitializeFunc pfNP_Initialize = (NP_InitializeFunc)
      PR_FindFunctionSymbol(mLibrary, "NP_Initialize");
    if (!pfNP_Initialize)
      return NS_ERROR_FAILURE;
    *error = pfNP_Initialize(bFuncs, pFuncs);
  }

  
  mNPP_New = pFuncs->newp;
  return NS_OK;
}
#else
nsresult
PluginPRLibrary::NP_Initialize(NPNetscapeFuncs* bFuncs, NPError* error)
{
  CALLING_CONVENTION_HACK

  if (mNP_Initialize) {
    *error = mNP_Initialize(bFuncs);
  } else {
    NP_InitializeFunc pfNP_Initialize = (NP_InitializeFunc)
      PR_FindFunctionSymbol(mLibrary, "NP_Initialize");
    if (!pfNP_Initialize)
      return NS_ERROR_FAILURE;
    *error = pfNP_Initialize(bFuncs);
  }

  return NS_OK;
}
#endif

nsresult
PluginPRLibrary::NP_Shutdown(NPError* error)
{
  CALLING_CONVENTION_HACK

  if (mNP_Shutdown) {
    *error = mNP_Shutdown();
  } else {
    NP_ShutdownFunc pfNP_Shutdown = (NP_ShutdownFunc)
      PR_FindFunctionSymbol(mLibrary, "NP_Shutdown");
    if (!pfNP_Shutdown)
      return NS_ERROR_FAILURE;
    *error = pfNP_Shutdown();
  }

  return NS_OK;
}

nsresult
PluginPRLibrary::NP_GetMIMEDescription(const char** mimeDesc)
{
  CALLING_CONVENTION_HACK

  if (mNP_GetMIMEDescription) {
    *mimeDesc = mNP_GetMIMEDescription();
  }
  else {
    NP_GetMIMEDescriptionFunc pfNP_GetMIMEDescription =
      (NP_GetMIMEDescriptionFunc)
      PR_FindFunctionSymbol(mLibrary, "NP_GetMIMEDescription");
    if (!pfNP_GetMIMEDescription) {
      *mimeDesc = "";
      return NS_ERROR_FAILURE;
    }
    *mimeDesc = pfNP_GetMIMEDescription();
  }

  return NS_OK;
}

nsresult
PluginPRLibrary::NP_GetValue(void *future, NPPVariable aVariable,
			     void *aValue, NPError* error)
{
#if defined(XP_UNIX) && !defined(XP_MACOSX)
  if (mNP_GetValue) {
    *error = mNP_GetValue(future, aVariable, aValue);
  } else {
    NP_GetValueFunc pfNP_GetValue = (NP_GetValueFunc)PR_FindFunctionSymbol(mLibrary, "NP_GetValue");
    if (!pfNP_GetValue)
      return NS_ERROR_FAILURE;
    *error = pfNP_GetValue(future, aVariable, aValue);
  }
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}

#if defined(XP_WIN) || defined(XP_MACOSX) || defined(XP_OS2)
nsresult
PluginPRLibrary::NP_GetEntryPoints(NPPluginFuncs* pFuncs, NPError* error)
{
  CALLING_CONVENTION_HACK

  if (mNP_GetEntryPoints) {
    *error = mNP_GetEntryPoints(pFuncs);
  } else {
    NP_GetEntryPointsFunc pfNP_GetEntryPoints = (NP_GetEntryPointsFunc)
      PR_FindFunctionSymbol(mLibrary, "NP_GetEntryPoints");
    if (!pfNP_GetEntryPoints)
      return NS_ERROR_FAILURE;
    *error = pfNP_GetEntryPoints(pFuncs);
  }

  
  mNPP_New = pFuncs->newp;
  return NS_OK;
}
#endif

nsresult
PluginPRLibrary::NPP_New(NPMIMEType pluginType, NPP instance,
			 uint16_t mode, int16_t argc, char* argn[],
			 char* argv[], NPSavedData* saved,
			 NPError* error)
{
  if (!mNPP_New)
    return NS_ERROR_FAILURE;
  *error = mNPP_New(pluginType, instance, mode, argc, argn, argv, saved);
  return NS_OK;
}

} 
