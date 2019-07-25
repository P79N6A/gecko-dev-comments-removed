








































#include <QWidget>
#include <QX11Info>
#include "nsQtRemoteService.h"

#include "mozilla/ModuleUtils.h"
#include "nsIServiceManager.h"
#include "nsIAppShellService.h"

#include "nsCOMPtr.h"




class MozQRemoteEventHandlerWidget: public QWidget {
public:
  



  MozQRemoteEventHandlerWidget(nsQtRemoteService &aRemoteService);

protected:
  



  bool x11Event(XEvent *);

private:
  


  nsQtRemoteService &mRemoteService;
};

MozQRemoteEventHandlerWidget::MozQRemoteEventHandlerWidget(nsQtRemoteService &aRemoteService)
  :mRemoteService(aRemoteService)
{
}

bool
MozQRemoteEventHandlerWidget::x11Event(XEvent *aEvt)
{
  if (aEvt->type == PropertyNotify && aEvt->xproperty.state == PropertyNewValue)
    mRemoteService.PropertyNotifyEvent(aEvt);

  return false;
}

NS_IMPL_ISUPPORTS2(nsQtRemoteService,
                   nsIRemoteService,
                   nsIObserver)

nsQtRemoteService::nsQtRemoteService():
mServerWindow(0)
{
}

NS_IMETHODIMP
nsQtRemoteService::Startup(const char* aAppName, const char* aProfileName)
{
#if (MOZ_PLATFORM_MAEMO == 5)
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
  if (mServerWindow) return NS_ERROR_ALREADY_INITIALIZED;
  NS_ASSERTION(aAppName, "Don't pass a null appname!");

  XRemoteBaseStartup(aAppName,aProfileName);

  
  mServerWindow = new MozQRemoteEventHandlerWidget(*this);

  HandleCommandsFor(mServerWindow->winId());
  return NS_OK;
}

NS_IMETHODIMP
nsQtRemoteService::RegisterWindow(nsIDOMWindow* aWindow)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsQtRemoteService::Shutdown()
{
  if (!mServerWindow)
    return NS_ERROR_NOT_INITIALIZED;

  delete mServerWindow;
  mServerWindow = nsnull;

  return NS_OK;
}

void
nsQtRemoteService::PropertyNotifyEvent(XEvent *aEvt)
{
  HandleNewProperty(aEvt->xproperty.window,
                    QX11Info::display(),
                    aEvt->xproperty.time,
                    aEvt->xproperty.atom,
                    0);
}

void
nsQtRemoteService::SetDesktopStartupIDOrTimestamp(const nsACString& aDesktopStartupID,
                                                  PRUint32 aTimestamp)
{
}


#define NS_REMOTESERVICE_CID \
  { 0xc0773e90, 0x5799, 0x4eff, { 0xad, 0x3, 0x3e, 0xbc, 0xd8, 0x56, 0x24, 0xac } }

NS_GENERIC_FACTORY_CONSTRUCTOR(nsQtRemoteService)
NS_DEFINE_NAMED_CID(NS_REMOTESERVICE_CID);

static const mozilla::Module::CIDEntry kRemoteCIDs[] = {
  { &kNS_REMOTESERVICE_CID, false, NULL, nsQtRemoteServiceConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kRemoteContracts[] = {
  { "@mozilla.org/toolkit/remote-service;1", &kNS_REMOTESERVICE_CID },
  { NULL }
};

static const mozilla::Module kRemoteModule = {
  mozilla::Module::kVersion,
  kRemoteCIDs,
  kRemoteContracts
};

NSMODULE_DEFN(RemoteServiceModule) = &kRemoteModule;
