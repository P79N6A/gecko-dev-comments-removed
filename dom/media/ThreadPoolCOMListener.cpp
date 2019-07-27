





#include "ThreadPoolCOMListener.h"

namespace mozilla {

NS_IMPL_ISUPPORTS(MSCOMInitThreadPoolListener, nsIThreadPoolListener)

NS_IMETHODIMP
MSCOMInitThreadPoolListener::OnThreadCreated()
{
  HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
  if (FAILED(hr)) {
    NS_WARNING("Failed to initialize MSCOM on decoder thread.");
  }
  return NS_OK;
}

NS_IMETHODIMP
MSCOMInitThreadPoolListener::OnThreadShuttingDown()
{
  CoUninitialize();
  return NS_OK;
}

}
