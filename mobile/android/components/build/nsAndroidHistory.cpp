



#include "nsThreadUtils.h"
#include "nsAndroidHistory.h"
#include "nsComponentManagerUtils.h"
#include "AndroidBridge.h"
#include "Link.h"
#include "nsIURI.h"
#include "nsIObserverService.h"

#include "mozilla/Services.h"
#include "mozilla/Preferences.h"

#define NS_LINK_VISITED_EVENT_TOPIC "link-visited"



#define PREF_HISTORY_ENABLED "places.history.enabled"


#define PENDING_REDIRECT_TIMEOUT 3000

using namespace mozilla;
using mozilla::dom::Link;

NS_IMPL_ISUPPORTS(nsAndroidHistory, IHistory, nsIRunnable, nsITimerCallback)

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
  : mHistoryEnabled(true)
{
  LoadPrefs();

  mTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
}

NS_IMETHODIMP
nsAndroidHistory::RegisterVisitedCallback(nsIURI *aURI, Link *aContent)
{
  if (!aContent || !aURI)
    return NS_OK;

  
  bool canAdd;
  nsresult rv = CanAddURI(aURI, &canAdd);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!canAdd) {
    return NS_OK;
  }

  nsAutoCString uri;
  rv = aURI->GetSpec(uri);
  if (NS_FAILED(rv)) return rv;
  NS_ConvertUTF8toUTF16 uriString(uri);

  nsTArray<Link*>* list = mListeners.Get(uriString);
  if (! list) {
    list = new nsTArray<Link*>();
    mListeners.Put(uriString, list);
  }
  list->AppendElement(aContent);

  if (AndroidBridge::HasEnv()) {
    mozilla::widget::android::GeckoAppShell::CheckURIVisited(uriString);
  }

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

void
nsAndroidHistory::AppendToRecentlyVisitedURIs(nsIURI* aURI) {
  if (mRecentlyVisitedURIs.Length() < RECENTLY_VISITED_URI_SIZE) {
    
    mRecentlyVisitedURIs.AppendElement(aURI);
  } else {
    
    mRecentlyVisitedURIsNextIndex %= RECENTLY_VISITED_URI_SIZE;
    mRecentlyVisitedURIs.ElementAt(mRecentlyVisitedURIsNextIndex) = aURI;
    mRecentlyVisitedURIsNextIndex++;
  }
}

bool
nsAndroidHistory::ShouldRecordHistory() {
  return mHistoryEnabled;
}

void
nsAndroidHistory::LoadPrefs() {
  mHistoryEnabled = Preferences::GetBool(PREF_HISTORY_ENABLED, true);
}

inline bool
nsAndroidHistory::IsRecentlyVisitedURI(nsIURI* aURI) {
  bool equals = false;
  RecentlyVisitedArray::index_type i;
  RecentlyVisitedArray::size_type length = mRecentlyVisitedURIs.Length();
  for (i = 0; i < length && !equals; ++i) {
    aURI->Equals(mRecentlyVisitedURIs.ElementAt(i), &equals);
  }
  return equals;
}

void
nsAndroidHistory::AppendToEmbedURIs(nsIURI* aURI) {
  if (mEmbedURIs.Length() < EMBED_URI_SIZE) {
    
    mEmbedURIs.AppendElement(aURI);
  } else {
    
    mEmbedURIsNextIndex %= EMBED_URI_SIZE;
    mEmbedURIs.ElementAt(mEmbedURIsNextIndex) = aURI;
    mEmbedURIsNextIndex++;
  }
}

inline bool
nsAndroidHistory::IsEmbedURI(nsIURI* aURI) {
  bool equals = false;
  EmbedArray::index_type i;
  EmbedArray::size_type length = mEmbedURIs.Length();
  for (i = 0; i < length && !equals; ++i) {
    aURI->Equals(mEmbedURIs.ElementAt(i), &equals);
  }
  return equals;
}

inline bool
nsAndroidHistory::RemovePendingVisitURI(nsIURI* aURI) {
  
  
  bool equals = false;
  PendingVisitArray::index_type i;
  for (i = 0; i < mPendingVisitURIs.Length(); ++i) {
    aURI->Equals(mPendingVisitURIs.ElementAt(i), &equals);
    if (equals) {
      mPendingVisitURIs.RemoveElementAt(i);
      return true;
    }
  }
  return false;
}

NS_IMETHODIMP
nsAndroidHistory::Notify(nsITimer *timer)
{
  
  
  PendingVisitArray::index_type i;
  for (i = 0; i < mPendingVisitURIs.Length(); ++i) {
    SaveVisitURI(mPendingVisitURIs.ElementAt(i));
  }
  mPendingVisitURIs.Clear();

  return NS_OK;
}

void
nsAndroidHistory::SaveVisitURI(nsIURI* aURI) {
  
  AppendToRecentlyVisitedURIs(aURI);

  if (AndroidBridge::HasEnv()) {
    
    nsAutoCString spec;
    (void)aURI->GetSpec(spec);
    mozilla::widget::android::GeckoAppShell::MarkURIVisited(NS_ConvertUTF8toUTF16(spec));
  }

  
  nsCOMPtr<nsIObserverService> obsService = mozilla::services::GetObserverService();
  if (obsService) {
    obsService->NotifyObservers(aURI, NS_LINK_VISITED_EVENT_TOPIC, nullptr);
  }
}

NS_IMETHODIMP
nsAndroidHistory::VisitURI(nsIURI *aURI, nsIURI *aLastVisitedURI, uint32_t aFlags)
{
  if (!aURI) {
    return NS_OK;
  }

  if (!(aFlags & VisitFlags::TOP_LEVEL)) {
    return NS_OK;
  }

  if (aFlags & VisitFlags::UNRECOVERABLE_ERROR) {
    return NS_OK;
  }

  if (aFlags & VisitFlags::REDIRECT_SOURCE || aFlags & VisitFlags::REDIRECT_PERMANENT || aFlags & VisitFlags::REDIRECT_TEMPORARY) {
    
    
    
    RemovePendingVisitURI(aLastVisitedURI);
  }

  
  bool canAdd;
  nsresult rv = CanAddURI(aURI, &canAdd);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!canAdd) {
    return NS_OK;
  }

  if (aLastVisitedURI) {
    bool same;
    rv = aURI->Equals(aLastVisitedURI, &same);
    NS_ENSURE_SUCCESS(rv, rv);
    if (same && IsRecentlyVisitedURI(aURI)) {
      
      return NS_OK;
    }

    
    
    if (RemovePendingVisitURI(aLastVisitedURI)) {
      SaveVisitURI(aLastVisitedURI);
    }
  }

  
  mPendingVisitURIs.AppendElement(aURI);
  mTimer->InitWithCallback(this, PENDING_REDIRECT_TIMEOUT, nsITimer::TYPE_ONE_SHOT);

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

  if (IsEmbedURI(aURI)) {
    return NS_OK;
  }

  if (AndroidBridge::HasEnv()) {
    nsAutoCString uri;
    nsresult rv = aURI->GetSpec(uri);
    if (NS_FAILED(rv)) return rv;
    NS_ConvertUTF8toUTF16 uriString(uri);
    mozilla::widget::android::GeckoAppShell::SetURITitle(uriString, aTitle);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsAndroidHistory::NotifyVisited(nsIURI *aURI)
{
  if (aURI && sHistory) {
    nsAutoCString spec;
    (void)aURI->GetSpec(spec);
    sHistory->mPendingLinkURIs.Push(NS_ConvertUTF8toUTF16(spec));
    NS_DispatchToMainThread(sHistory);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsAndroidHistory::Run()
{
  while (! mPendingLinkURIs.IsEmpty()) {
    nsString uriString = mPendingLinkURIs.Pop();
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

  
  if (!ShouldRecordHistory()) {
    *canAdd = false;
    return NS_OK;
  }

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
