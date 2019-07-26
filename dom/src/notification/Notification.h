



#ifndef mozilla_dom_notification_h__
#define mozilla_dom_notification_h__

#include "mozilla/dom/NotificationBinding.h"

#include "nsDOMEventTargetHelper.h"
#include "nsIObserver.h"

namespace mozilla {
namespace dom {

class NotificationObserver;

class Notification : public nsDOMEventTargetHelper
{
  friend class NotificationTask;
  friend class NotificationPermissionRequest;
  friend class NotificationObserver;
public:
  IMPL_EVENT_HANDLER(click)
  IMPL_EVENT_HANDLER(show)
  IMPL_EVENT_HANDLER(error)
  IMPL_EVENT_HANDLER(close)

  Notification(const nsAString& aTitle, const nsAString& aBody,
               NotificationDirection aDir, const nsAString& aLang,
               const nsAString& aTag, const nsAString& aIconUrl);

  static already_AddRefed<Notification> Constructor(const GlobalObject& aGlobal,
                                                    const nsAString& aTitle,
                                                    const NotificationOptions& aOption,
                                                    ErrorResult& aRv);

  static void RequestPermission(const GlobalObject& aGlobal,
                                const Optional<OwningNonNull<NotificationPermissionCallback> >& aCallback,
                                ErrorResult& aRv);

  static NotificationPermission GetPermission(const GlobalObject& aGlobal,
                                              ErrorResult& aRv);

  void Close();

  static bool PrefEnabled();

  nsPIDOMWindow* GetParentObject()
  {
    return GetOwner();
  }

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;
protected:
  nsresult ShowInternal();
  nsresult CloseInternal();

  static NotificationPermission GetPermissionInternal(nsISupports* aGlobal,
                                                      ErrorResult& rv);

  static const nsString DirectionToString(NotificationDirection aDirection)
  {
    switch (aDirection) {
    case NotificationDirectionValues::Ltr:
      return NS_LITERAL_STRING("ltr");
    case NotificationDirectionValues::Rtl:
      return NS_LITERAL_STRING("rtl");
    default:
      return NS_LITERAL_STRING("auto");
    }
  }

  nsresult GetAlertName(nsString& aAlertName);

  nsString mTitle;
  nsString mBody;
  NotificationDirection mDir;
  nsString mLang;
  nsString mTag;
  nsString mIconUrl;

  bool mIsClosed;

  static uint32_t sCount;
};

} 
} 

#endif 

