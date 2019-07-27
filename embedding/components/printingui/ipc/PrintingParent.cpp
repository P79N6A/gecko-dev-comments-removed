





#include "mozilla/dom/Element.h"
#include "mozilla/dom/TabParent.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMWindow.h"
#include "nsIPrintingPromptService.h"
#include "nsIPrintOptions.h"
#include "nsIPrintProgressParams.h"
#include "nsIServiceManager.h"
#include "nsIWebProgressListener.h"
#include "PrintingParent.h"
#include "PrintDataUtils.h"
#include "PrintProgressDialogParent.h"

using namespace mozilla;
using namespace mozilla::dom;

namespace mozilla {
namespace embedding {
bool
PrintingParent::RecvShowProgress(PBrowserParent* parent,
                                 PPrintProgressDialogParent* printProgressDialog,
                                 const bool& isForPrinting,
                                 bool* notifyOnOpen,
                                 bool* success)
{
  *success = false;

  nsCOMPtr<nsIDOMWindow> parentWin = DOMWindowFromBrowserParent(parent);
  if (!parentWin) {
    return true;
  }

  nsCOMPtr<nsIPrintingPromptService> pps(do_GetService("@mozilla.org/embedcomp/printingprompt-service;1"));

  if (!pps) {
    return true;
  }

  PrintProgressDialogParent* dialogParent =
    static_cast<PrintProgressDialogParent*>(printProgressDialog);
  nsCOMPtr<nsIObserver> observer = do_QueryInterface(dialogParent);

  nsCOMPtr<nsIWebProgressListener> printProgressListener;
  nsCOMPtr<nsIPrintProgressParams> printProgressParams;

  nsresult rv = pps->ShowProgress(parentWin, nullptr, nullptr, observer,
                                  isForPrinting,
                                  getter_AddRefs(printProgressListener),
                                  getter_AddRefs(printProgressParams),
                                  notifyOnOpen);
  NS_ENSURE_SUCCESS(rv, true);

  dialogParent->SetWebProgressListener(printProgressListener);
  dialogParent->SetPrintProgressParams(printProgressParams);

  *success = true;
  return true;
}

bool
PrintingParent::RecvShowPrintDialog(PBrowserParent* parent,
                                    const PrintData& data,
                                    PrintData* retVal,
                                    bool* success)
{
  *success = false;

  nsCOMPtr<nsIDOMWindow> parentWin = DOMWindowFromBrowserParent(parent);
  if (!parentWin) {
    return true;
  }

  nsCOMPtr<nsIPrintingPromptService> pps(do_GetService("@mozilla.org/embedcomp/printingprompt-service;1"));
  if (!pps) {
    return true;
  }

  
  
  
  nsCOMPtr<nsIWebBrowserPrint> wbp = new MockWebBrowserPrint(data);

  nsresult rv;
  nsCOMPtr<nsIPrintOptions> po = do_GetService("@mozilla.org/gfx/printsettings-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, true);

  nsCOMPtr<nsIPrintSettings> settings;
  rv = po->CreatePrintSettings(getter_AddRefs(settings));
  NS_ENSURE_SUCCESS(rv, true);

  rv = po->DeserializeToPrintSettings(data, settings);
  NS_ENSURE_SUCCESS(rv, true);

  rv = pps->ShowPrintDialog(parentWin, wbp, settings);
  NS_ENSURE_SUCCESS(rv, true);

  
  PrintData result;
  rv = po->SerializeToPrintData(settings, nullptr, &result);
  NS_ENSURE_SUCCESS(rv, true);

  *retVal = result;
  *success = true;
  return true;
}

PPrintProgressDialogParent*
PrintingParent::AllocPPrintProgressDialogParent()
{
  PrintProgressDialogParent* actor = new PrintProgressDialogParent();
  NS_ADDREF(actor); 
                    
  return actor;
}

bool
PrintingParent::DeallocPPrintProgressDialogParent(PPrintProgressDialogParent* doomed)
{
  
  
  
  PrintProgressDialogParent* actor = static_cast<PrintProgressDialogParent*>(doomed);
  NS_RELEASE(actor);
  return true;
}

void
PrintingParent::ActorDestroy(ActorDestroyReason aWhy)
{
}

nsIDOMWindow*
PrintingParent::DOMWindowFromBrowserParent(PBrowserParent* parent)
{
  if (!parent) {
    return nullptr;
  }

  TabParent* tabParent = TabParent::GetFrom(parent);
  if (!tabParent) {
    return nullptr;
  }

  nsCOMPtr<Element> frameElement = tabParent->GetOwnerElement();
  if (!frameElement) {
    return nullptr;
  }

  nsCOMPtr<nsIContent> frame(do_QueryInterface(frameElement));
  if (!frame) {
    return nullptr;
  }

  nsCOMPtr<nsIDOMWindow> parentWin = do_QueryInterface(frame->OwnerDoc()->GetWindow());
  if (!parentWin) {
    return nullptr;
  }

  return parentWin;
}

MOZ_IMPLICIT PrintingParent::PrintingParent()
{
    MOZ_COUNT_CTOR(PrintingParent);
}

MOZ_IMPLICIT PrintingParent::~PrintingParent()
{
    MOZ_COUNT_DTOR(PrintingParent);
}

} 
} 

