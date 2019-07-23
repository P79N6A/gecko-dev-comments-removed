






































#include "nsAppleEventsService.h"
#include "nsAECoreClass.h"

nsAppleEventsService::nsAppleEventsService()
{
}

nsAppleEventsService::~nsAppleEventsService()
{

}

NS_IMPL_ISUPPORTS1(nsAppleEventsService, nsIAppleEventsService)

NS_IMETHODIMP nsAppleEventsService::Init(void)
{
  OSErr err = CreateAEHandlerClasses(false);
  return (err == noErr) ? NS_OK : NS_ERROR_FAILURE;
}



NS_IMETHODIMP nsAppleEventsService::Shutdown(void)
{
  ShutdownAEHandlerClasses();
  return NS_OK;
}

