





#include "mozilla/ipc/IOThreadChild.h"

#include "ContentProcess.h"

#if defined(XP_WIN) && defined(MOZ_CONTENT_SANDBOX)
#include "mozilla/Preferences.h"
#include "nsDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#endif

using mozilla::ipc::IOThreadChild;

namespace mozilla {
namespace dom {

#if defined(XP_WIN) && defined(MOZ_CONTENT_SANDBOX)
static already_AddRefed<nsIFile>
GetLowIntegrityTemp()
{
  MOZ_ASSERT(nsDirectoryService::gService,
    "GetLowIntegrityTemp relies on nsDirectoryService being initialized");

  
  if (Preferences::GetInt("security.sandbox.content.level") != 1) {
    return nullptr;
  }

  nsCOMPtr<nsIFile> lowIntegrityTemp;
  nsresult rv = nsDirectoryService::gService->Get(NS_WIN_LOW_INTEGRITY_TEMP,
                                                  NS_GET_IID(nsIFile),
                                                  getter_AddRefs(lowIntegrityTemp));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return nullptr;
  }

  return lowIntegrityTemp.forget();
}

static void
SetUpSandboxEnvironment()
{
  MOZ_ASSERT(nsDirectoryService::gService,
    "SetUpSandboxEnvironment relies on nsDirectoryService being initialized");

  
  nsCOMPtr<nsIFile> lowIntegrityTemp = GetLowIntegrityTemp();
  if (!lowIntegrityTemp) {
    return;
  }

  
  unused << nsDirectoryService::gService->Undefine(NS_OS_TEMP_DIR);
  nsresult rv = nsDirectoryService::gService->Set(NS_OS_TEMP_DIR, lowIntegrityTemp);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  
  nsAutoString lowIntegrityTempPath;
  rv = lowIntegrityTemp->GetPath(lowIntegrityTempPath);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  bool setOK = SetEnvironmentVariableW(L"TEMP", lowIntegrityTempPath.get());
  NS_WARN_IF_FALSE(setOK, "Failed to set TEMP to low integrity temp path");
  setOK = SetEnvironmentVariableW(L"TMP", lowIntegrityTempPath.get());
  NS_WARN_IF_FALSE(setOK, "Failed to set TMP to low integrity temp path");
}

static void
CleanUpSandboxEnvironment()
{
  
  nsCOMPtr<nsIFile> lowIntegrityTemp = GetLowIntegrityTemp();
  if (!lowIntegrityTemp) {
    return;
  }

  
  
  unused << lowIntegrityTemp->Remove( true);
}
#endif

void
ContentProcess::SetAppDir(const nsACString& aPath)
{
  mXREEmbed.SetAppDir(aPath);
}

bool
ContentProcess::Init()
{
    mContent.Init(IOThreadChild::message_loop(),
                         ParentPid(),
                         IOThreadChild::channel());
    mXREEmbed.Start();
    mContent.InitXPCOM();

#if defined(XP_WIN) && defined(MOZ_CONTENT_SANDBOX)
    SetUpSandboxEnvironment();
#endif
    
    return true;
}

void
ContentProcess::CleanUp()
{
#if defined(XP_WIN) && defined(MOZ_CONTENT_SANDBOX)
    CleanUpSandboxEnvironment();
#endif
    mXREEmbed.Stop();
}

} 
} 
