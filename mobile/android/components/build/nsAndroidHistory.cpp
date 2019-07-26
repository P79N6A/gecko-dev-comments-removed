



#include "nsThreadUtils.h"
#include "nsAndroidHistory.h"
#include "AndroidBridge.h"
#include "Link.h"
#include "nsIURI.h"

using namespace mozilla;
using mozilla::dom::Link;

NS_IMPL_ISUPPORTS2(nsAndroidHistory, IHistory, nsIRunnable)

nsAndroidHistory* nsAndroidHistory::sHistory = nullptr;


nsAndroidHistory*
nsAndroidHistory::GetSingleton()
{
  if (!sHistory) {
    sHistory = new nsAndroidHistory();
    NS_ENSURE_TRUE(sHistory, nullptr);
  }

  NS_ADDREF(sHistory);
  return sHistory;
}

nsAndroidHistory::nsAndroidHistory()
{
}

NS_IMETHODIMP
nsAndroidHistory::RegisterVisitedCallback(nsIURI *aURI, Link *aContent)
{
  if (!aContent || !aURI)
    return NS_OK;

  nsAutoCString uri;
  nsresult rv = aURI->GetSpec(uri);
  if (NS_FAILED(rv)) return rv;
  NS_ConvertUTF8toUTF16 uriString(uri);

  nsTArray<Link*>* list = mListeners.Get(uriString);
  if (! list) {
    list = new nsTArray<Link*>();
    mListeners.Put(uriString, list);
  }
  list->AppendElement(aContent);

 GeckoAppShell::CheckURIVisited(uriString);

  return NS_OK;
}

NS_IMETHODIMP
nsAndroidHistory::UnregisterVisitedCallback(nsIURI *aURI, Link *aContent)
{
  if (!aContent || !aURI)
    return NS_OK;

  nsAutoCString uri;
  nsresult rv = aURI->GetSpec(uri);
  if (NS_FAILED(rv)) return rv;
  NS_ConvertUTF8toUTF16 uriString(uri);

  nsTArray<Link*>* list = mListeners.Get(uriString);
  if (! list)
    return NS_OK;

  list->RemoveElement(aContent);
  if (list->IsEmpty()) {
    mListeners.Remove(uriString);
    delete list;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsAndroidHistory::VisitURI(nsIURI *aURI, nsIURI *aLastVisitedURI, uint32_t aFlags)
{
  if (!aURI)
    return NS_OK;

  
  bool canAdd;
  nsresult rv = CanAddURI(aURI, &canAdd);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!canAdd) {
    return NS_OK;
  }

  if (!(aFlags & VisitFlags::TOP_LEVEL))
    return NS_OK;

  if (aFlags & VisitFlags::REDIRECT_SOURCE)
    return NS_OK;

  if (aFlags & VisitFlags::UNRECOVERABLE_ERROR)
    return NS_OK;

  nsAutoCString uri;
  rv = aURI->GetSpec(uri);
  if (NS_FAILED(rv)) return rv;
  NS_ConvertUTF8toUTF16 uriString(uri);
  GeckoAppShell::MarkURIVisited(uriString);
  return NS_OK;
}

NS_IMETHODIMP
nsAndroidHistory::SetURITitle(nsIURI *aURI, const nsAString& aTitle)
{
  
  bool canAdd;
  nsresult rv = CanAddURI(aURI, &canAdd);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!canAdd) {
    return NS_OK;
  }

  if (AndroidBridge::Bridge()) {
    nsAutoCString uri;
    nsresult rv = aURI->GetSpec(uri);
    if (NS_FAILED(rv)) return rv;
    NS_ConvertUTF8toUTF16 uriString(uri);
    GeckoAppShell::SetURITitle(uriString, aTitle);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsAndroidHistory::NotifyVisited(nsIURI *aURI)
{
  if (aURI && sHistory) {
    nsAutoCString spec;
    (void)aURI->GetSpec(spec);
    sHistory->mPendingURIs.Push(NS_ConvertUTF8toUTF16(spec));
    NS_DispatchToMainThread(sHistory);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsAndroidHistory::Run()
{
  while (! mPendingURIs.IsEmpty()) {
    nsString uriString = mPendingURIs.Pop();
    nsTArray<Link*>* list = sHistory->mListeners.Get(uriString);
    if (list) {
      for (unsigned int i = 0; i < list->Length(); i++) {
        list->ElementAt(i)->SetLinkState(eLinkState_Visited);
      }
      
      
      mListeners.Remove(uriString);
      delete list;
    }
  }
  return NS_OK;
}









NS_IMETHODIMP
nsAndroidHistory::CanAddURI(nsIURI* aURI, bool* canAdd)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);
  NS_ENSURE_ARG_POINTER(canAdd);

  nsAutoCString scheme;
  nsresult rv = aURI->GetScheme(scheme);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (scheme.EqualsLiteral("http")) {
    *canAdd = true;
    return NS_OK;
  }
  if (scheme.EqualsLiteral("https")) {
    *canAdd = true;
    return NS_OK;
  }

  
  if (scheme.EqualsLiteral("about") ||
      scheme.EqualsLiteral("imap") ||
      scheme.EqualsLiteral("news") ||
      scheme.EqualsLiteral("mailbox") ||
      scheme.EqualsLiteral("moz-anno") ||
      scheme.EqualsLiteral("view-source") ||
      scheme.EqualsLiteral("chrome") ||
      scheme.EqualsLiteral("resource") ||
      scheme.EqualsLiteral("data") ||
      scheme.EqualsLiteral("wyciwyg") ||
      scheme.EqualsLiteral("javascript") ||
      scheme.EqualsLiteral("blob")) {
    *canAdd = false;
    return NS_OK;
  }
  *canAdd = true;
  return NS_OK;
}
