



#include "nsXPCOM.h"
#include "nsXPCOMPrivate.h"
#include "nsXPCOMStrings.h"
#include "xptcall.h"

#include <string.h>









XPCOM_API(nsresult)
NS_RegisterXPCOMExitRoutine(XPCOMExitRoutine exitRoutine, uint32_t priority);

XPCOM_API(nsresult)
NS_UnregisterXPCOMExitRoutine(XPCOMExitRoutine exitRoutine);

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
    &NS_GetTraceRefcnt,

    
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

    
    &NS_Alloc,
    &NS_Realloc,
    &NS_Free,
    &NS_StringContainerInit2,
    &NS_CStringContainerInit2,
    &NS_StringGetMutableData,
    &NS_CStringGetMutableData,
    NULL,

    
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

    
    &NS_CycleCollectorSuspect2,
    &NS_CycleCollectorForget2
};

EXPORT_XPCOM_API(nsresult)
NS_GetFrozenFunctions(XPCOMFunctions *functions, const char* )
{
    if (!functions)
        return NS_ERROR_OUT_OF_MEMORY;

    if (functions->version != XPCOM_GLUE_VERSION)
        return NS_ERROR_FAILURE;

    uint32_t size = functions->size;
    if (size > sizeof(XPCOMFunctions))
        size = sizeof(XPCOMFunctions);

    size -= offsetof(XPCOMFunctions, init);

    memcpy(&functions->init, &kFrozenFunctions.init, size);

    return NS_OK;
}





EXPORT_XPCOM_API(nsresult)
NS_RegisterXPCOMExitRoutine(XPCOMExitRoutine exitRoutine, uint32_t priority)
{
  return NS_OK;
}

EXPORT_XPCOM_API(nsresult)
NS_UnregisterXPCOMExitRoutine(XPCOMExitRoutine exitRoutine)
{
  return NS_OK;
}
