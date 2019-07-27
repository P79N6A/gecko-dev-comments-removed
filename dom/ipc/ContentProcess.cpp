





#include "mozilla/ipc/IOThreadChild.h"

#include "ContentProcess.h"

#if defined(XP_WIN) && defined(MOZ_CONTENT_SANDBOX)
#include "mozilla/Preferences.h"
#include "mozilla/WindowsVersion.h"
#include "nsDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#endif

using mozilla::ipc::IOThreadChild;

namespace mozilla {
namespace dom {

#if defined(XP_WIN) && defined(MOZ_CONTENT_SANDBOX)
static void
SetUpSandboxEnvironment()
{
  MOZ_ASSERT(nsDirectoryService::gService,
    "SetUpSandboxEnvironment relies on nsDirectoryService being initialized");

  
  
  if (!IsVistaOrLater() ||
      Preferences::GetInt("security.sandbox.content.level") != 1) {
    return;
  }

  nsAdoptingString tempDirSuffix =
    Preferences::GetString("security.sandbox.content.tempDirSuffix");
  if (tempDirSuffix.IsEmpty()) {
    NS_WARNING("Low integrity temp suffix pref not set.");
    return;
  }

  
  nsCOMPtr<nsIFile> lowIntegrityTemp;
  nsresult rv = nsDirectoryService::gService->Get(NS_WIN_LOW_INTEGRITY_TEMP_BASE,
                                                  NS_GET_IID(nsIFile),
                                                  getter_AddRefs(lowIntegrityTemp));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  
  rv = lowIntegrityTemp->Append(NS_LITERAL_STRING("Temp-") + tempDirSuffix);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  
  
  unused << nsDirectoryService::gService->Undefine(NS_OS_TEMP_DIR);
  rv = nsDirectoryService::gService->Set(NS_OS_TEMP_DIR, lowIntegrityTemp);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }
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
    mXREEmbed.Stop();
}

} 
} 
