





#include "nsHistory.h"

#include "jsapi.h"
#include "mozilla/dom/HistoryBinding.h"
#include "nsCOMPtr.h"
#include "nsPIDOMWindow.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIDocShell.h"
#include "nsIWebNavigation.h"
#include "nsIURI.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsReadableUtils.h"
#include "nsContentUtils.h"
#include "nsISHistory.h"
#include "nsISHistoryInternal.h"
#include "mozilla/Preferences.h"

using namespace mozilla;
using namespace mozilla::dom;

static const char* sAllowPushStatePrefStr =
  "browser.history.allowPushState";
static const char* sAllowReplaceStatePrefStr =
  "browser.history.allowReplaceState";




NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(nsHistory)
NS_IMPL_CYCLE_COLLECTING_ADDREF(nsHistory)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsHistory)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsHistory)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHistory) 
NS_INTERFACE_MAP_END

nsHistory::nsHistory(nsPIDOMWindow* aInnerWindow)
  : mInnerWindow(do_GetWeakReference(aInnerWindow))
{
  SetIsDOMBinding();
}

nsHistory::~nsHistory()
{
}

nsPIDOMWindow*
nsHistory::GetParentObject() const
{
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  return win;
}

JSObject*
nsHistory::WrapObject(JSContext* aCx)
{
  return HistoryBinding::Wrap(aCx, this);
}

uint32_t
nsHistory::GetLength(ErrorResult& aRv) const
{
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  if (!win || !win->HasActiveDocument()) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);

    return 0;
  }

  
  nsCOMPtr<nsISHistory> sHistory = GetSessionHistory();
  if (!sHistory) {
    aRv.Throw(NS_ERROR_FAILURE);

    return 0;
  }

  int32_t len;
  nsresult rv = sHistory->GetCount(&len);

  if (NS_FAILED(rv)) {
    aRv.Throw(rv);

    return 0;
  }

  return len >= 0 ? len : 0;
}

JS::Value
nsHistory::GetState(JSContext* aCx, ErrorResult& aRv) const
{
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  if (!win) {
    aRv.Throw(NS_ERROR_NOT_AVAILABLE);

    return JS::UndefinedValue();
  }

  if (!win->HasActiveDocument()) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);

    return JS::UndefinedValue();
  }

  nsCOMPtr<nsIDocument> doc =
    do_QueryInterface(win->GetExtantDoc());
  if (!doc) {
    aRv.Throw(NS_ERROR_NOT_AVAILABLE);

    return JS::UndefinedValue();
  }

  nsCOMPtr<nsIVariant> variant;
  doc->GetStateObject(getter_AddRefs(variant));

  if (variant) {
    JS::Rooted<JS::Value> jsData(aCx);
    aRv = variant->GetAsJSVal(&jsData);

    if (aRv.Failed()) {
      return JS::UndefinedValue();
    }

    if (!JS_WrapValue(aCx, &jsData)) {
      aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
      return JS::UndefinedValue();
    }

    return jsData;
  }

  return JS::NullValue();
}

void
nsHistory::Go(int32_t aDelta, ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  if (!win || !win->HasActiveDocument()) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);

    return;
  }

  if (!aDelta) {
    nsCOMPtr<nsPIDOMWindow> window;
    if (nsIDocShell* docShell = GetDocShell()) {
      window = docShell->GetWindow();
    }

    if (window && window->IsHandlingResizeEvent()) {
      
      
      
      
      
      
      

      nsCOMPtr<nsIDocument> doc = window->GetExtantDoc();

      nsIPresShell *shell;
      nsPresContext *pcx;
      if (doc && (shell = doc->GetShell()) && (pcx = shell->GetPresContext())) {
        pcx->RebuildAllStyleData(NS_STYLE_HINT_REFLOW);
      }

      return;
    }
  }

  nsCOMPtr<nsISHistory> session_history = GetSessionHistory();
  nsCOMPtr<nsIWebNavigation> webnav(do_QueryInterface(session_history));
  if (!webnav) {
    aRv.Throw(NS_ERROR_FAILURE);

    return;
  }

  int32_t curIndex = -1;
  int32_t len = 0;
  session_history->GetIndex(&curIndex);
  session_history->GetCount(&len);

  int32_t index = curIndex + aDelta;
  if (index > -1 && index < len)
    webnav->GotoIndex(index);

  
  
  
}

void
nsHistory::Back(ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  if (!win || !win->HasActiveDocument()) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);

    return;
  }

  nsCOMPtr<nsISHistory> sHistory = GetSessionHistory();
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(sHistory));
  if (!webNav) {
    aRv.Throw(NS_ERROR_FAILURE);

    return;
  }

  webNav->GoBack();
}

void
nsHistory::Forward(ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  if (!win || !win->HasActiveDocument()) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);

    return;
  }

  nsCOMPtr<nsISHistory> sHistory = GetSessionHistory();
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(sHistory));
  if (!webNav) {
    aRv.Throw(NS_ERROR_FAILURE);

    return;
  }

  webNav->GoForward();
}

void
nsHistory::PushState(JSContext* aCx, JS::Handle<JS::Value> aData,
                     const nsAString& aTitle, const nsAString& aUrl,
                     ErrorResult& aRv)
{
  PushOrReplaceState(aCx, aData, aTitle, aUrl, aRv, false);
}

void
nsHistory::ReplaceState(JSContext* aCx, JS::Handle<JS::Value> aData,
                        const nsAString& aTitle, const nsAString& aUrl,
                        ErrorResult& aRv)
{
  PushOrReplaceState(aCx, aData, aTitle, aUrl, aRv, true);
}

void
nsHistory::PushOrReplaceState(JSContext* aCx, JS::Handle<JS::Value> aData,
                              const nsAString& aTitle, const nsAString& aUrl,
                              ErrorResult& aRv, bool aReplace)
{
  nsCOMPtr<nsPIDOMWindow> win(do_QueryReferent(mInnerWindow));
  if (!win) {
    aRv.Throw(NS_ERROR_NOT_AVAILABLE);

    return;
  }

  if (!win->HasActiveDocument()) {
    aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);

    return;
  }

  
  if (!Preferences::GetBool(aReplace ? sAllowReplaceStatePrefStr :
                            sAllowPushStatePrefStr, false)) {
    return;
  }

  
  
  nsCOMPtr<nsIDocShell> docShell = win->GetDocShell();

  if (!docShell) {
    aRv.Throw(NS_ERROR_FAILURE);

    return;
  }

  
  

  aRv = docShell->AddState(aData, aTitle, aUrl, aReplace, aCx);
}

nsIDocShell*
nsHistory::GetDocShell() const
{
  nsCOMPtr<nsPIDOMWindow> win = do_QueryReferent(mInnerWindow);
  if (!win) {
    return nullptr;
  }
  return win->GetDocShell();
}

already_AddRefed<nsISHistory>
nsHistory::GetSessionHistory() const
{
  nsIDocShell *docShell = GetDocShell();
  NS_ENSURE_TRUE(docShell, nullptr);

  
  nsCOMPtr<nsIDocShellTreeItem> root;
  docShell->GetSameTypeRootTreeItem(getter_AddRefs(root));
  nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(root));
  NS_ENSURE_TRUE(webNav, nullptr);

  nsCOMPtr<nsISHistory> shistory;

  
  webNav->GetSessionHistory(getter_AddRefs(shistory));

  return shistory.forget();
}
