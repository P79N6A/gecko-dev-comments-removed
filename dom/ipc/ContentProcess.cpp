





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

#if defined(NIGHTLY_BUILD)
static void
CleanUpOldSandboxEnvironment()
{
  
  
  nsCOMPtr<nsIFile> lowIntegrityMozilla;
  nsresult rv = NS_GetSpecialDirectory(NS_WIN_LOCAL_APPDATA_LOW_DIR,
                              getter_AddRefs(lowIntegrityMozilla));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  rv = lowIntegrityMozilla->Append(NS_LITERAL_STRING(MOZ_USER_DIR));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  nsCOMPtr<nsISimpleEnumerator> iter;
  rv = lowIntegrityMozilla->GetDirectoryEntries(getter_AddRefs(iter));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return;
  }

  bool more;
  nsCOMPtr<nsISupports> elem;
  while (NS_SUCCEEDED(iter->HasMoreElements(&more)) && more) {
    rv = iter->GetNext(getter_AddRefs(elem));
    if (NS_FAILED(rv)) {
      break;
    }

    nsCOMPtr<nsIFile> file = do_QueryInterface(elem);
    if (!file) {
      continue;
    }

    nsAutoString leafName;
    rv = file->GetLeafName(leafName);
    if (NS_FAILED(rv)) {
      continue;
    }

    if (leafName.Find(NS_LITERAL_STRING("MozTemp-{")) == 0) {
      file->Remove( true);
    }
  }
}
#endif
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
#if defined(XP_WIN) && defined(MOZ_CONTENT_SANDBOX) && defined(NIGHTLY_BUILD)
    CleanUpOldSandboxEnvironment();
#endif
    mXREEmbed.Stop();
}

} 
} 
