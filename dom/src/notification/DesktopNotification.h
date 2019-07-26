



#ifndef mozilla_dom_DesktopNotification_h
#define mozilla_dom_DesktopNotification_h

#include "PCOMContentPermissionRequestChild.h"

#include "nsIPrincipal.h"
#include "nsIJSContextStack.h"

#include "nsIAlertsService.h"

#include "nsIContentPermissionPrompt.h"

#include "nsIObserver.h"
#include "nsString.h"
#include "nsWeakPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMWindow.h"
#include "nsThreadUtils.h"

#include "nsDOMEventTargetHelper.h"
#include "nsIDOMEvent.h"
#include "nsIDocument.h"

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsWrapperCache.h"


namespace mozilla {
namespace dom {

class AlertServiceObserver;
class DesktopNotification;





class DesktopNotificationCenter MOZ_FINAL : public nsISupports,
                                            public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DesktopNotificationCenter)

  DesktopNotificationCenter(nsPIDOMWindow *aWindow)
  {
    mOwner = aWindow;

    
    mPrincipal = mOwner->GetDoc()->NodePrincipal();

    SetIsDOMBinding();
  }

  virtual ~DesktopNotificationCenter()
  {
  }

  void Shutdown() {
    mOwner = nullptr;
  }

  nsPIDOMWindow* GetParentObject() const
  {
    return mOwner;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

  already_AddRefed<DesktopNotification>
  CreateNotification(const nsAString& title,
                     const nsAString& description,
                     const nsAString& iconURL);

private:
  nsCOMPtr<nsPIDOMWindow> mOwner;
  nsCOMPtr<nsIPrincipal> mPrincipal;
};


class DesktopNotification MOZ_FINAL : public nsDOMEventTargetHelper
{
  friend class DesktopNotificationRequest;

public:

  DesktopNotification(const nsAString& aTitle,
                      const nsAString& aDescription,
                      const nsAString& aIconURL,
                      nsPIDOMWindow *aWindow,
                      nsIPrincipal* principal);

  virtual ~DesktopNotification();

  void Init();

  



  nsresult PostDesktopNotification();

  nsresult SetAllow(bool aAllow);

  


  void DispatchNotificationEvent(const nsString& aName);

  void HandleAlertServiceNotification(const char *aTopic);

  

  nsPIDOMWindow* GetParentObject() const
  {
    return GetOwner();
  }

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

  void Show(ErrorResult& aRv);

  IMPL_EVENT_HANDLER(click)
  IMPL_EVENT_HANDLER(close)

protected:

  nsString mTitle;
  nsString mDescription;
  nsString mIconURL;

  nsRefPtr<AlertServiceObserver> mObserver;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  bool mAllow;
  bool mShowHasBeenCalled;

  static uint32_t sCount;
};




class DesktopNotificationRequest : public nsIContentPermissionRequest,
                                   public nsRunnable,
                                   public PCOMContentPermissionRequestChild

{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTENTPERMISSIONREQUEST

  DesktopNotificationRequest(DesktopNotification* notification)
    : mDesktopNotification(notification) {}

  NS_IMETHOD Run()
  {
    nsCOMPtr<nsIContentPermissionPrompt> prompt =
      do_CreateInstance(NS_CONTENT_PERMISSION_PROMPT_CONTRACTID);
    if (prompt) {
      prompt->Prompt(this);
    }
    return NS_OK;
  }

  ~DesktopNotificationRequest()
  {
  }

 bool Recv__delete__(const bool& allow)
 {
   if (allow)
     (void) Allow();
   else
     (void) Cancel();
   return true;
 }
 void IPDLRelease() { Release(); }

  nsRefPtr<DesktopNotification> mDesktopNotification;
};

class AlertServiceObserver: public nsIObserver
{
 public:
  NS_DECL_ISUPPORTS

    AlertServiceObserver(DesktopNotification* notification)
    : mNotification(notification) {}

  virtual ~AlertServiceObserver() {}

  void Disconnect() { mNotification = nullptr; }

  NS_IMETHODIMP
  Observe(nsISupports *aSubject,
          const char *aTopic,
          const PRUnichar *aData)
  {

    
    if (mNotification) {
#ifdef MOZ_B2G
    if (NS_FAILED(mNotification->CheckInnerWindowCorrectness()))
      return NS_ERROR_NOT_AVAILABLE;
#endif
      mNotification->HandleAlertServiceNotification(aTopic);
    }
    return NS_OK;
  };

 private:
  DesktopNotification* mNotification;
};

} 
} 

#endif 
