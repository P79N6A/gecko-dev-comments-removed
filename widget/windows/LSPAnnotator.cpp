












#include "nsICrashReporter.h"
#include "nsISupportsImpl.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsQueryObject.h"
#include <windows.h>
#include <ws2spi.h>

namespace mozilla {
namespace crashreporter {

class LSPAnnotationGatherer : public nsRunnable
{
  ~LSPAnnotationGatherer() {}

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  void Annotate();
  nsCString mString;
  nsCOMPtr<nsIThread> mThread;
};

NS_IMPL_ISUPPORTS(LSPAnnotationGatherer, nsIRunnable)

void
LSPAnnotationGatherer::Annotate()
{
  nsCOMPtr<nsICrashReporter> cr =
    do_GetService("@mozilla.org/toolkit/crash-reporter;1");
  bool enabled;
  if (cr && NS_SUCCEEDED(cr->GetEnabled(&enabled)) && enabled) {
    cr->AnnotateCrashReport(NS_LITERAL_CSTRING("Winsock_LSP"), mString);
  }
  mThread->Shutdown();
}

NS_IMETHODIMP
LSPAnnotationGatherer::Run()
{
  PR_SetCurrentThreadName("LSP Annotator");

  mThread = NS_GetCurrentThread();

  DWORD size = 0;
  int err;
  
  if (SOCKET_ERROR != WSCEnumProtocols(nullptr, nullptr, &size, &err) ||
      err != WSAENOBUFS) {
    
    NS_NOTREACHED("WSCEnumProtocols suceeded when it should have failed ...");
    return NS_ERROR_FAILURE;
  }

  nsAutoArrayPtr<char> byteArray(new char[size]);
  WSAPROTOCOL_INFOW* providers =
    reinterpret_cast<WSAPROTOCOL_INFOW*>(byteArray.get());

  int n = WSCEnumProtocols(nullptr, providers, &size, &err);
  if (n == SOCKET_ERROR) {
    
    NS_WARNING("Could not get LSP list");
    return NS_ERROR_FAILURE;
  }

  nsCString str;
  for (int i = 0; i < n; i++) {
    AppendUTF16toUTF8(nsDependentString(providers[i].szProtocol), str);
    str.AppendLiteral(" : ");
    str.AppendInt(providers[i].iVersion);
    str.AppendLiteral(" : ");
    str.AppendInt(providers[i].iSocketType);
    str.AppendLiteral(" : ");

    wchar_t path[MAX_PATH];
    int dummy;
    if (!WSCGetProviderPath(&providers[i].ProviderId, path,
                            &dummy, &err)) {
      AppendUTF16toUTF8(nsDependentString(path), str);
    }

    if (i + 1 != n) {
      str.AppendLiteral(" \n ");
    }
  }

  mString = str;
  NS_DispatchToMainThread(NS_NewRunnableMethod(this, &LSPAnnotationGatherer::Annotate));
  return NS_OK;
}

void LSPAnnotate()
{
  nsCOMPtr<nsIThread> thread;
  nsCOMPtr<nsIRunnable> runnable =
    do_QueryObject(new LSPAnnotationGatherer());
  NS_NewThread(getter_AddRefs(thread), runnable);
}

} 
} 
