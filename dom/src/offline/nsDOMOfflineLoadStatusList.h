





































#ifndef nsDOMOfflineLoadStatusList_h___
#define nsDOMOfflineLoadStatusList_h___

#include "nscore.h"
#include "nsIDOMLoadStatus.h"
#include "nsIDOMLoadStatusEvent.h"
#include "nsIDOMLoadStatusList.h"
#include "nsIOfflineCacheUpdate.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIURI.h"
#include "nsDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEventListener.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsIScriptContext.h"

class nsDOMOfflineLoadStatus;

class nsDOMOfflineLoadStatusList : public nsIDOMLoadStatusList,
                                   public nsIDOMEventTarget,
                                   public nsIObserver,
                                   public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMLOADSTATUSLIST
  NS_DECL_NSIDOMEVENTTARGET
  NS_DECL_NSIOBSERVER

  nsDOMOfflineLoadStatusList(nsIURI *aURI);
  virtual ~nsDOMOfflineLoadStatusList();

  nsresult Init();

private :
  nsresult          UpdateAdded         (nsIOfflineCacheUpdate *aUpdate);
  nsresult          UpdateCompleted     (nsIOfflineCacheUpdate *aUpdate);
  nsIDOMLoadStatus *FindWrapper         (nsIDOMLoadStatus *aStatus,
                                         PRUint32 *aIndex);
  void              NotifyEventListeners(const nsCOMArray<nsIDOMEventListener>& aListeners,
                                         nsIDOMEvent* aEvent);

  nsresult          SendLoadEvent       (const nsAString& aEventName,
                                         const nsCOMArray<nsIDOMEventListener>& aListeners,
                                         nsIDOMLoadStatus *aStatus);

  PRBool mInitialized;

  nsCOMPtr<nsIURI> mURI;
  nsCOMArray<nsIDOMLoadStatus> mItems;
  nsCString mHostPort;

  nsCOMPtr<nsIScriptContext> mScriptContext;

  nsCOMArray<nsIDOMEventListener> mLoadRequestedEventListeners;
  nsCOMArray<nsIDOMEventListener> mLoadCompletedEventListeners;
  nsCOMArray<nsIDOMEventListener> mUpdateCompletedEventListeners;
};

class nsDOMLoadStatusEvent : public nsDOMEvent,
                             public nsIDOMLoadStatusEvent
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMLOADSTATUSEVENT
  NS_FORWARD_NSIDOMEVENT(nsDOMEvent::)

  nsDOMLoadStatusEvent(const nsAString& aEventName, nsIDOMLoadStatus *aStatus)
    : nsDOMEvent(nsnull, nsnull), mEventName(aEventName), mStatus(aStatus)
  {
  }

  virtual ~nsDOMLoadStatusEvent() { }

  nsresult Init();

private:
  nsAutoString mEventName;
  nsCOMPtr<nsIDOMLoadStatus> mStatus;
};

#endif

