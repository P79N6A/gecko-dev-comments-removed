





#include "mozilla/ClearOnShutdown.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/unused.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIPrintingPromptService.h"
#include "nsPIDOMWindow.h"
#include "nsPrintingProxy.h"
#include "nsPrintOptionsImpl.h"
#include "PrintDataUtils.h"
#include "PrintProgressDialogChild.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::embedding;

static StaticRefPtr<nsPrintingProxy> sPrintingProxyInstance;

NS_IMPL_ISUPPORTS(nsPrintingProxy, nsIPrintingPromptService)

nsPrintingProxy::nsPrintingProxy()
{
}

nsPrintingProxy::~nsPrintingProxy()
{
}


already_AddRefed<nsPrintingProxy>
nsPrintingProxy::GetInstance()
{
  if (!sPrintingProxyInstance) {
    sPrintingProxyInstance = new nsPrintingProxy();
    if (!sPrintingProxyInstance) {
      return nullptr;
    }
    nsresult rv = sPrintingProxyInstance->Init();
    if (NS_FAILED(rv)) {
      sPrintingProxyInstance = nullptr;
      return nullptr;
    }
    ClearOnShutdown(&sPrintingProxyInstance);
  }

  nsRefPtr<nsPrintingProxy> inst = sPrintingProxyInstance.get();
  return inst.forget();
}

nsresult
nsPrintingProxy::Init()
{
  mozilla::unused << ContentChild::GetSingleton()->SendPPrintingConstructor(this);
  return NS_OK;
}

NS_IMETHODIMP
nsPrintingProxy::ShowPrintDialog(nsIDOMWindow *parent,
                                 nsIWebBrowserPrint *webBrowserPrint,
                                 nsIPrintSettings *printSettings)
{
  NS_ENSURE_ARG(parent);
  NS_ENSURE_ARG(webBrowserPrint);
  NS_ENSURE_ARG(printSettings);

  
  
  
  nsCOMPtr<nsPIDOMWindow> pwin = do_QueryInterface(parent);
  NS_ENSURE_STATE(pwin);
  nsCOMPtr<nsIDocShell> docShell = pwin->GetDocShell();
  NS_ENSURE_STATE(docShell);
  nsCOMPtr<nsIDocShellTreeOwner> owner;
  nsresult rv = docShell->GetTreeOwner(getter_AddRefs(owner));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsITabChild> tabchild = do_GetInterface(owner);
  NS_ENSURE_STATE(tabchild);

  TabChild* pBrowser = static_cast<TabChild*>(tabchild.get());

  
  nsCOMPtr<nsIPrintOptions> po =
    do_GetService("@mozilla.org/gfx/printsettings-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PrintData inSettings;
  rv = po->SerializeToPrintData(printSettings, webBrowserPrint, &inSettings);
  NS_ENSURE_SUCCESS(rv, rv);

  PrintData modifiedSettings;
  bool success;

  mozilla::unused << SendShowPrintDialog(pBrowser, inSettings, &modifiedSettings, &success);

  if (!success) {
    
    return NS_ERROR_FAILURE;
  }

  rv = po->DeserializeToPrintSettings(modifiedSettings, printSettings);
  return NS_OK;
}

NS_IMETHODIMP
nsPrintingProxy::ShowProgress(nsIDOMWindow*            parent,
                              nsIWebBrowserPrint*      webBrowserPrint,    
                              nsIPrintSettings*        printSettings,      
                              nsIObserver*             openDialogObserver, 
                              bool                     isForPrinting,
                              nsIWebProgressListener** webProgressListener,
                              nsIPrintProgressParams** printProgressParams,
                              bool*                  notifyOnOpen)
{
  NS_ENSURE_ARG(parent);
  NS_ENSURE_ARG(webProgressListener);
  NS_ENSURE_ARG(printProgressParams);
  NS_ENSURE_ARG(notifyOnOpen);

  
  
  
  nsCOMPtr<nsPIDOMWindow> pwin = do_QueryInterface(parent);
  NS_ENSURE_STATE(pwin);
  nsCOMPtr<nsIDocShell> docShell = pwin->GetDocShell();
  NS_ENSURE_STATE(docShell);
  nsCOMPtr<nsIDocShellTreeOwner> owner;
  nsresult rv = docShell->GetTreeOwner(getter_AddRefs(owner));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsITabChild> tabchild = do_GetInterface(owner);
  TabChild* pBrowser = static_cast<TabChild*>(tabchild.get());

  nsRefPtr<PrintProgressDialogChild> dialogChild =
    new PrintProgressDialogChild(openDialogObserver);

  SendPPrintProgressDialogConstructor(dialogChild);

  bool success = false;

  mozilla::unused << SendShowProgress(pBrowser, dialogChild,
                                      isForPrinting, notifyOnOpen, &success);
  NS_ADDREF(*webProgressListener = dialogChild);
  NS_ADDREF(*printProgressParams = dialogChild);

  return NS_OK;
}

NS_IMETHODIMP
nsPrintingProxy::ShowPageSetup(nsIDOMWindow *parent,
                               nsIPrintSettings *printSettings,
                               nsIObserver *aObs)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsPrintingProxy::ShowPrinterProperties(nsIDOMWindow *parent,
                                       const char16_t *printerName,
                                       nsIPrintSettings *printSettings)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsPrintingProxy::SavePrintSettings(nsIPrintSettings* aPS,
                                   bool aUsePrinterNamePrefix,
                                   uint32_t aFlags)
{
  nsresult rv;
  nsCOMPtr<nsIPrintOptions> po =
    do_GetService("@mozilla.org/gfx/printsettings-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PrintData settings;
  rv = po->SerializeToPrintData(aPS, nullptr, &settings);
  NS_ENSURE_SUCCESS(rv, rv);

  unused << SendSavePrintSettings(settings, aUsePrinterNamePrefix, aFlags,
                                  &rv);
  return rv;
}

PPrintProgressDialogChild*
nsPrintingProxy::AllocPPrintProgressDialogChild()
{
  
  
  NS_NOTREACHED("Allocator for PPrintProgressDialogChild should not be "
                "called on nsPrintingProxy.");
  return nullptr;
}

bool
nsPrintingProxy::DeallocPPrintProgressDialogChild(PPrintProgressDialogChild* aActor)
{
  
  
  NS_NOTREACHED("Deallocator for PPrintProgressDialogChild should not be "
                "called on nsPrintingProxy.");
  return false;
}
