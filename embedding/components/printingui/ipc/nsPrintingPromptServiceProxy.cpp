





#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/unused.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeOwner.h"
#include "nsPIDOMWindow.h"
#include "nsPrintingPromptServiceProxy.h"
#include "nsIPrintingPromptService.h"
#include "PrintDataUtils.h"
#include "nsPrintOptionsImpl.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::embedding;

NS_IMPL_ISUPPORTS(nsPrintingPromptServiceProxy, nsIPrintingPromptService)

nsPrintingPromptServiceProxy::nsPrintingPromptServiceProxy()
{
}

nsPrintingPromptServiceProxy::~nsPrintingPromptServiceProxy()
{
}

nsresult
nsPrintingPromptServiceProxy::Init()
{
  mozilla::unused << ContentChild::GetSingleton()->SendPPrintingConstructor(this);
  return NS_OK;
}

NS_IMETHODIMP
nsPrintingPromptServiceProxy::ShowPrintDialog(nsIDOMWindow *parent,
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
nsPrintingPromptServiceProxy::ShowProgress(nsIDOMWindow*            parent,
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

  mozilla::unused << SendShowProgress(pBrowser, isForPrinting);

  return NS_OK;
}

NS_IMETHODIMP
nsPrintingPromptServiceProxy::ShowPageSetup(nsIDOMWindow *parent,
                                            nsIPrintSettings *printSettings,
                                            nsIObserver *aObs)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsPrintingPromptServiceProxy::ShowPrinterProperties(nsIDOMWindow *parent,
                                                    const char16_t *printerName,
                                                    nsIPrintSettings *printSettings)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

