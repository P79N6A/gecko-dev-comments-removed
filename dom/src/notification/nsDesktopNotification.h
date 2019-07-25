



































#ifndef nsDesktopNotification_h
#define nsDesktopNotification_h

#include "nsDOMClassInfo.h"
#include "nsIJSContextStack.h"

#include "nsIAlertsService.h"

#include "nsIDOMDesktopNotification.h"
#include "nsIDOMEventTarget.h"
#include "nsIDesktopNotificationPrompt.h"

#include "nsIObserver.h"
#include "nsString.h"
#include "nsWeakPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMWindow.h"
#include "nsThreadUtils.h"

#include "nsDOMEventTargetHelper.h"
#include "nsIPrivateDOMEvent.h"

class AlertServiceObserver;





class nsDesktopNotificationCenter : public nsIDOMDesktopNotificationCenter
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMDESKTOPNOTIFICATIONCENTER

  nsDesktopNotificationCenter(nsPIDOMWindow *aWindow,
                              nsIScriptContext* aScriptContext)
  {
    mOwner = aWindow;
    mScriptContext = aScriptContext;

    
    nsCOMPtr<nsIDOMDocument> domdoc;
    mOwner->GetDocument(getter_AddRefs(domdoc));
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(domdoc);
    doc->NodePrincipal()->GetURI(getter_AddRefs(mURI));
  }

  virtual ~nsDesktopNotificationCenter()
  {
  }

private:
  nsCOMPtr<nsPIDOMWindow> mOwner;
  nsCOMPtr<nsIScriptContext> mScriptContext;
  nsCOMPtr<nsIURI> mURI;
};


class nsDOMDesktopNotification : public nsDOMEventTargetHelper,
                                 public nsIDOMDesktopNotification
{
  friend class nsDesktopNotificationRequest;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMDesktopNotification,nsDOMEventTargetHelper)
  NS_DECL_NSIDOMDESKTOPNOTIFICATION

  nsDOMDesktopNotification(const nsAString & title,
                           const nsAString & description,
                           const nsAString & iconURL,
                           nsPIDOMWindow *aWindow,
                           nsIScriptContext* aScriptContext,
                           nsIURI* uri);

  virtual ~nsDOMDesktopNotification();

  



  void PostDesktopNotification();

  


  void DispatchNotificationEvent(const nsString& aName);

  void HandleAlertServiceNotification(const char *aTopic);

protected:

  nsString mTitle;
  nsString mDescription;
  nsString mIconURL;

  nsRefPtr<nsDOMEventListenerWrapper> mOnClickCallback;
  nsRefPtr<nsDOMEventListenerWrapper> mOnCloseCallback;

  nsRefPtr<AlertServiceObserver> mObserver;
  nsCOMPtr<nsIURI> mURI;
};




class nsDesktopNotificationRequest : public nsIDOMDesktopNotificationRequest,
                                     public nsRunnable
{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMDESKTOPNOTIFICATIONREQUEST

  nsDesktopNotificationRequest(nsDOMDesktopNotification* notification)
    : mDesktopNotification(notification) {}

  NS_IMETHOD Run()
  {
    nsCOMPtr<nsIDesktopNotificationPrompt> prompt =
      do_GetService(NS_DOM_DESKTOP_NOTIFICATION_PROMPT_CONTRACTID);
    if (prompt) {
      prompt->Prompt(this);
    }
    return NS_OK;
  }

  ~nsDesktopNotificationRequest()
 {
 }

  nsRefPtr<nsDOMDesktopNotification> mDesktopNotification;
};

class AlertServiceObserver: public nsIObserver
{
 public:
  NS_DECL_ISUPPORTS
    
    AlertServiceObserver(nsDOMDesktopNotification* notification)
    : mNotification(notification) {}
  
  virtual ~AlertServiceObserver() {}

  void Disconnect() { mNotification = nsnull; }

  NS_IMETHODIMP
  Observe(nsISupports *aSubject,
          const char *aTopic,
          const PRUnichar *aData)
  {
    
    if (mNotification)
      mNotification->HandleAlertServiceNotification(aTopic);
    return NS_OK;
  };
  
 private:
  nsDOMDesktopNotification* mNotification;
};

#endif 
