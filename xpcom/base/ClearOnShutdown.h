






































#ifndef mozilla_ClearOnShutdown_h

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsIObserver.h>
#include <nsIObserverService.h>
#include <mozilla/Services.h>

















namespace mozilla {
namespace ClearOnShutdown_Internal {

template<class SmartPtr>
class ShutdownObserver : public nsIObserver
{
public:
  ShutdownObserver(SmartPtr *aPtr)
    : mPtr(aPtr)
  {}

  virtual ~ShutdownObserver()
  {}

  NS_DECL_ISUPPORTS

  NS_IMETHOD Observe(nsISupports *aSubject, const char *aTopic,
                     const PRUnichar *aData)
  {
    MOZ_ASSERT(strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0);

    if (mPtr) {
      *mPtr = NULL;
    }

    return NS_OK;
  }

private:
  SmartPtr *mPtr;
};











template<class SmartPtr>
NS_IMPL_ADDREF(mozilla::ClearOnShutdown_Internal::
                 ShutdownObserver<SmartPtr>)

template<class SmartPtr>
NS_IMPL_RELEASE(mozilla::ClearOnShutdown_Internal::
                  ShutdownObserver<SmartPtr>)

template<class SmartPtr>
NS_IMPL_QUERY_INTERFACE1(mozilla::ClearOnShutdown_Internal::
                           ShutdownObserver<SmartPtr>,
                         nsIObserver)

} 

template<class SmartPtr>
void ClearOnShutdown(SmartPtr *aPtr)
{
  nsRefPtr<ClearOnShutdown_Internal::ShutdownObserver<SmartPtr> > observer =
    new ClearOnShutdown_Internal::ShutdownObserver<SmartPtr>(aPtr);

  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  if (!os) {
    NS_WARNING("Could not get observer service!");
    return;
  }
  os->AddObserver(observer, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
}

} 

#endif
