





#include "nsXPCOM.h"
#include "nsXPCOMPrivate.h"
#include "nsXPCOMStrings.h"
#include "xptcall.h"

#include <string.h>









XPCOM_API(nsresult)
NS_RegisterXPCOMExitRoutine(XPCOMExitRoutine aExitRoutine, uint32_t aPriority);

XPCOM_API(nsresult)
NS_UnregisterXPCOMExitRoutine(XPCOMExitRoutine aExitRoutine);

static const XPCOMFunctions kFrozenFunctions = {
  XPCOM_GLUE_VERSION,
  sizeof(XPCOMFunctions),
  &NS_InitXPCOM2,
  &NS_ShutdownXPCOM,
  &NS_GetServiceManager,
  &NS_GetComponentManager,
  &NS_GetComponentRegistrar,
  &NS_GetMemoryManager,
  &NS_NewLocalFile,
  &NS_NewNativeLocalFile,
  &NS_RegisterXPCOMExitRoutine,
  &NS_UnregisterXPCOMExitRoutine,

  
  &NS_GetDebug,
  nullptr,

  
  &NS_StringContainerInit,
  &NS_StringContainerFinish,
  &NS_StringGetData,
  &NS_StringSetData,
  &NS_StringSetDataRange,
  &NS_StringCopy,
  &NS_CStringContainerInit,
  &NS_CStringContainerFinish,
  &NS_CStringGetData,
  &NS_CStringSetData,
  &NS_CStringSetDataRange,
  &NS_CStringCopy,
  &NS_CStringToUTF16,
  &NS_UTF16ToCString,
  &NS_StringCloneData,
  &NS_CStringCloneData,

  
  &moz_xmalloc,
  &moz_xrealloc,
  &free,
  &NS_StringContainerInit2,
  &NS_CStringContainerInit2,
  &NS_StringGetMutableData,
  &NS_CStringGetMutableData,
  nullptr,

  
  &NS_DebugBreak,
  &NS_LogInit,
  &NS_LogTerm,
  &NS_LogAddRef,
  &NS_LogRelease,
  &NS_LogCtor,
  &NS_LogDtor,
  &NS_LogCOMPtrAddRef,
  &NS_LogCOMPtrRelease,
  &NS_GetXPTCallStub,
  &NS_DestroyXPTCallStub,
  &NS_InvokeByIndex,
  nullptr,
  nullptr,
  &NS_StringSetIsVoid,
  &NS_StringGetIsVoid,
  &NS_CStringSetIsVoid,
  &NS_CStringGetIsVoid,

  
  nullptr,
  nullptr,

  &NS_CycleCollectorSuspect3,
};

EXPORT_XPCOM_API(nsresult)
NS_GetFrozenFunctions(XPCOMFunctions* aFunctions, const char* )
{
  if (!aFunctions) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (aFunctions->version != XPCOM_GLUE_VERSION) {
    return NS_ERROR_FAILURE;
  }

  uint32_t size = aFunctions->size;
  if (size > sizeof(XPCOMFunctions)) {
    size = sizeof(XPCOMFunctions);
  }

  size -= offsetof(XPCOMFunctions, init);

  memcpy(&aFunctions->init, &kFrozenFunctions.init, size);

  return NS_OK;
}





EXPORT_XPCOM_API(nsresult)
NS_RegisterXPCOMExitRoutine(XPCOMExitRoutine aExitRoutine, uint32_t aPriority)
{
  return NS_OK;
}

EXPORT_XPCOM_API(nsresult)
NS_UnregisterXPCOMExitRoutine(XPCOMExitRoutine aExitRoutine)
{
  return NS_OK;
}
