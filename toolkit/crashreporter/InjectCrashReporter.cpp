




#include "InjectCrashReporter.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "client/windows/crash_generation/crash_generation_client.h"
#include "nsExceptionHandler.h"
#include "LoadLibraryRemote.h"
#include "nsWindowsHelpers.h"

using google_breakpad::CrashGenerationClient;
using CrashReporter::GetChildNotificationPipe;

namespace mozilla {

InjectCrashRunnable::InjectCrashRunnable(DWORD pid)
  : mPID(pid)
{
  nsCOMPtr<nsIFile> dll;
  nsresult rv = NS_GetSpecialDirectory(NS_GRE_DIR, getter_AddRefs(dll));
  if (NS_SUCCEEDED(rv)) {
    dll->Append(NS_LITERAL_STRING("breakpadinjector.dll"));
    dll->GetPath(mInjectorPath);
  }
}

NS_IMETHODIMP
InjectCrashRunnable::Run()
{
  if (mInjectorPath.IsEmpty())
    return NS_OK;

  nsAutoHandle hProcess(
    OpenProcess(PROCESS_CREATE_THREAD |
                PROCESS_QUERY_INFORMATION |
                PROCESS_DUP_HANDLE |
                PROCESS_VM_OPERATION |
                PROCESS_VM_WRITE |
                PROCESS_VM_READ, FALSE, mPID));
  if (!hProcess) {
    NS_WARNING("Unable to open remote process handle for crashreporter injection.");
    return NS_OK;
  }

  void* proc = LoadRemoteLibraryAndGetAddress(hProcess, mInjectorPath.get(),
                                              "Start");
  if (!proc) {
    NS_WARNING("Unable to inject crashreporter DLL.");
    return NS_OK;
  }

  HANDLE hRemotePipe =
    CrashGenerationClient::DuplicatePipeToClientProcess(
      NS_ConvertASCIItoUTF16(GetChildNotificationPipe()).get(),
      hProcess);
  if (INVALID_HANDLE_VALUE == hRemotePipe) {
    NS_WARNING("Unable to duplicate crash reporter pipe to process.");
    return NS_OK;
  }

  nsAutoHandle hThread(CreateRemoteThread(hProcess, nullptr, 0,
                                          (LPTHREAD_START_ROUTINE) proc,
                                          (void*) hRemotePipe, 0, nullptr));
  if (!hThread) {
    NS_WARNING("Unable to CreateRemoteThread");

    
    
    HANDLE toClose = INVALID_HANDLE_VALUE;
    if (DuplicateHandle(hProcess, hRemotePipe, ::GetCurrentProcess(),
                        &toClose, 0, FALSE,
                        DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS)) {
      CloseHandle(toClose);
      return NS_OK;
    }
  }

  return NS_OK;
}

} 
