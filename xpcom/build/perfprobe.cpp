








#include <windows.h>
#include <wmistr.h>
#include <evntrace.h>

#include "perfprobe.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace probes {

#if defined(MOZ_LOGGING)
static PRLogModuleInfo*
GetProbeLog()
{
  static PRLogModuleInfo* sLog;
  if (!sLog) {
    sLog = PR_NewLogModule("SysProbe");
  }
  return sLog;
}
#define LOG(x)  PR_LOG(GetProbeLog(), PR_LOG_DEBUG, x)
#else
#define LOG(x)
#endif


GUID
CID_to_GUID(const nsCID& aCID)
{
  GUID result;
  result.Data1 = aCID.m0;
  result.Data2 = aCID.m1;
  result.Data3 = aCID.m2;
  for (int i = 0; i < 8; ++i) {
    result.Data4[i] = aCID.m3[i];
  }
  return result;
}




Probe::Probe(const nsCID& aGUID,
             const nsACString& aName,
             ProbeManager* aManager)
  : mGUID(CID_to_GUID(aGUID))
  , mName(aName)
  , mManager(aManager)
{
}

nsresult
Probe::Trigger()
{
  if (!(mManager->mIsActive)) {
    
    return NS_OK;
  }

  _EVENT_TRACE_HEADER event;
  ZeroMemory(&event, sizeof(event));
  event.Size = sizeof(event);
  event.Flags = WNODE_FLAG_TRACED_GUID ;
  event.Guid = (const GUID)mGUID;
  event.Class.Type    = 1;
  event.Class.Version = 0;
  event.Class.Level   = TRACE_LEVEL_INFORMATION;

  ULONG result        = TraceEvent(mManager->mSessionHandle, &event);

  LOG(("Probes: Triggered %s, %s, %ld",
       mName.Data(),
       result == ERROR_SUCCESS ? "success" : "failure",
       result));

  nsresult rv;
  switch (result) {
    case ERROR_SUCCESS:
      rv = NS_OK;
      break;
    case ERROR_INVALID_FLAG_NUMBER:
    case ERROR_MORE_DATA:
    case ERROR_INVALID_PARAMETER:
      rv = NS_ERROR_INVALID_ARG;
      break;
    case ERROR_INVALID_HANDLE:
      rv = NS_ERROR_FAILURE;
      break;
    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_OUTOFMEMORY:
      rv = NS_ERROR_OUT_OF_MEMORY;
      break;
    default:
      rv = NS_ERROR_UNEXPECTED;
  }
  return rv;
}




ProbeManager::~ProbeManager()
{
  
  if (mIsActive && mRegistrationHandle) {
    StopSession();
  }
}

ProbeManager::ProbeManager(const nsCID& aApplicationUID,
                           const nsACString& aApplicationName)
  : mApplicationUID(aApplicationUID)
  , mApplicationName(aApplicationName)
  , mSessionHandle(0)
  , mRegistrationHandle(0)
{
#if defined(MOZ_LOGGING)
  char cidStr[NSID_LENGTH];
  aApplicationUID.ToProvidedString(cidStr);
  LOG(("ProbeManager::Init for application %s, %s",
       aApplicationName.Data(), cidStr));
#endif
}








ULONG WINAPI
ControlCallback(WMIDPREQUESTCODE aRequestCode,
                PVOID aContext,
                ULONG* aReserved,
                PVOID aBuffer)
{
  ProbeManager* context = (ProbeManager*)aContext;
  switch (aRequestCode) {
    case WMI_ENABLE_EVENTS: {
      context->mIsActive = true;
      TRACEHANDLE sessionHandle = GetTraceLoggerHandle(aBuffer);
      
      if ((HANDLE)sessionHandle == INVALID_HANDLE_VALUE) {
        ULONG result = GetLastError();
        LOG(("Probes: ControlCallback failed, %ul", result));
        return result;
      } else if (context->mIsActive && context->mSessionHandle &&
                 context->mSessionHandle != sessionHandle) {
        LOG(("Probes: Can only handle one context at a time, "
             "ignoring activation"));
        return ERROR_SUCCESS;
      } else {
        context->mSessionHandle = sessionHandle;
        LOG(("Probes: ControlCallback activated"));
        return ERROR_SUCCESS;
      }
    }

    case WMI_DISABLE_EVENTS:
      context->mIsActive      = false;
      context->mSessionHandle = 0;
      LOG(("Probes: ControlCallback deactivated"));
      return ERROR_SUCCESS;

    default:
      LOG(("Probes: ControlCallback does not know what to do with %d",
           aRequestCode));
      return ERROR_INVALID_PARAMETER;
  }
}

already_AddRefed<Probe>
ProbeManager::GetProbe(const nsCID& aEventUID, const nsACString& aEventName)
{
  nsRefPtr<Probe> result(new Probe(aEventUID, aEventName, this));
  mAllProbes.AppendElement(result);
  return result.forget();
}

nsresult
ProbeManager::StartSession()
{
  return StartSession(mAllProbes);
}

nsresult
ProbeManager::StartSession(nsTArray<nsRefPtr<Probe>>& aProbes)
{
  const size_t probesCount = aProbes.Length();
  _TRACE_GUID_REGISTRATION* probes = new _TRACE_GUID_REGISTRATION[probesCount];
  for (unsigned int i = 0; i < probesCount; ++i) {
    const Probe* probe = aProbes[i];
    const Probe* probeX = static_cast<const Probe*>(probe);
    probes[i].Guid = (LPCGUID)&probeX->mGUID;
  }
  ULONG result =
    RegisterTraceGuids(&ControlCallback
                       ,
                       this
                       ,
                       (LPGUID)&mApplicationUID
                       
,
                       probesCount
                       ,
                       probes
                       ,
                       nullptr
                       ,
                       nullptr
                       ,
                       &mRegistrationHandle
                       

                      );
  delete[] probes;
  if (NS_WARN_IF(result != ERROR_SUCCESS)) {
    return NS_ERROR_UNEXPECTED;
  }
  return NS_OK;
}

nsresult
ProbeManager::StopSession()
{
  LOG(("Probes: Stopping measures"));
  if (mSessionHandle != 0) {
    ULONG result = UnregisterTraceGuids(mSessionHandle);
    mSessionHandle = 0;
    if (result != ERROR_SUCCESS) {
      return NS_ERROR_INVALID_ARG;
    }
  }
  return NS_OK;
}

}  
}  
