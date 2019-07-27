





#ifndef mozilla_dom_notification_h__
#define mozilla_dom_notification_h__

#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/dom/NotificationBinding.h"
#include "mozilla/dom/workers/bindings/WorkerFeature.h"

#include "nsIObserver.h"

#include "nsCycleCollectionParticipant.h"

class nsIPrincipal;
class nsIStructuredCloneContainer;
class nsIVariant;

namespace mozilla {
namespace dom {

class NotificationRef;
class WorkerNotificationObserver;
class Promise;

namespace workers {
  class WorkerPrivate;
} 

class Notification;
class NotificationFeature final : public workers::WorkerFeature
{
  
  
  Notification* mNotification;

public:
  explicit NotificationFeature(Notification* aNotification);

  bool
  Notify(JSContext* aCx, workers::Status aStatus) override;
};
































































class Notification : public DOMEventTargetHelper
{
  friend class CloseNotificationRunnable;
  friend class NotificationTask;
  friend class NotificationPermissionRequest;
  friend class NotificationObserver;
  friend class NotificationStorageCallback;
  friend class ServiceWorkerNotificationObserver;
  friend class WorkerNotificationObserver;

public:
  IMPL_EVENT_HANDLER(click)
  IMPL_EVENT_HANDLER(show)
  IMPL_EVENT_HANDLER(error)
  IMPL_EVENT_HANDLER(close)

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(Notification, DOMEventTargetHelper)

  static bool PrefEnabled(JSContext* aCx, JSObject* aObj);
  
  static bool IsGetEnabled(JSContext* aCx, JSObject* aObj);

  static already_AddRefed<Notification> Constructor(const GlobalObject& aGlobal,
                                                    const nsAString& aTitle,
                                                    const NotificationOptions& aOption,
                                                    ErrorResult& aRv);

  









  static already_AddRefed<Notification>
  ConstructFromFields(
    nsIGlobalObject* aGlobal,
    const nsAString& aID,
    const nsAString& aTitle,
    const nsAString& aDir,
    const nsAString& aLang,
    const nsAString& aBody,
    const nsAString& aTag,
    const nsAString& aIcon,
    const nsAString& aData,
    const nsAString& aScope,
    ErrorResult& aRv);

  void GetID(nsAString& aRetval) {
    aRetval = mID;
  }

  void GetTitle(nsAString& aRetval)
  {
    aRetval = mTitle;
  }

  NotificationDirection Dir()
  {
    return mDir;
  }

  void GetLang(nsAString& aRetval)
  {
    aRetval = mLang;
  }

  void GetBody(nsAString& aRetval)
  {
    aRetval = mBody;
  }

  void GetTag(nsAString& aRetval)
  {
    aRetval = mTag;
  }

  void GetIcon(nsAString& aRetval)
  {
    aRetval = mIconUrl;
  }

  void SetStoredState(bool val)
  {
    mIsStored = val;
  }

  bool IsStored()
  {
    return mIsStored;
  }

  nsIStructuredCloneContainer* GetDataCloneContainer();

  static bool RequestPermissionEnabledForScope(JSContext* aCx, JSObject* );

  static void RequestPermission(const GlobalObject& aGlobal,
                                const Optional<OwningNonNull<NotificationPermissionCallback> >& aCallback,
                                ErrorResult& aRv);

  static NotificationPermission GetPermission(const GlobalObject& aGlobal,
                                              ErrorResult& aRv);

  static already_AddRefed<Promise>
  Get(nsPIDOMWindow* aWindow,
      const GetNotificationOptions& aFilter,
      const nsAString& aScope,
      ErrorResult& aRv);

  static already_AddRefed<Promise> Get(const GlobalObject& aGlobal,
                                       const GetNotificationOptions& aFilter,
                                       ErrorResult& aRv);

  
  
  static already_AddRefed<Promise>
  ShowPersistentNotification(nsIGlobalObject* aGlobal,
                             const nsAString& aScope,
                             const nsAString& aTitle,
                             const NotificationOptions& aOptions,
                             ErrorResult& aRv);

  void Close();

  nsPIDOMWindow* GetParentObject()
  {
    return GetOwner();
  }

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  void GetData(JSContext* aCx, JS::MutableHandle<JS::Value> aRetval);

  void InitFromJSVal(JSContext* aCx, JS::Handle<JS::Value> aData, ErrorResult& aRv);

  void InitFromBase64(JSContext* aCx, const nsAString& aData, ErrorResult& aRv);

  void AssertIsOnTargetThread() const
  {
    MOZ_ASSERT(IsTargetThread());
  }

  
  
  workers::WorkerPrivate* mWorkerPrivate;

  
  WorkerNotificationObserver* mObserver;

  
  
  
  
  
  
  
  UniquePtr<NotificationRef> mTempRef;

  
  bool AddRefObject();
  void ReleaseObject();

  static NotificationPermission GetPermission(nsIGlobalObject* aGlobal,
                                              ErrorResult& aRv);

  static NotificationPermission GetPermissionInternal(nsIPrincipal* aPrincipal,
                                                      ErrorResult& rv);

  bool DispatchClickEvent();
  bool DispatchNotificationClickEvent();
protected:
  
  
  Notification(const nsAString& aID, const nsAString& aTitle, const nsAString& aBody,
               NotificationDirection aDir, const nsAString& aLang,
               const nsAString& aTag, const nsAString& aIconUrl,
               const NotificationBehavior& aBehavior);

  static already_AddRefed<Notification> CreateInternal(const nsAString& aID,
                                                       const nsAString& aTitle,
                                                       const NotificationOptions& aOptions);

  void ShowInternal();
  void CloseInternal();

  static NotificationPermission GetPermissionInternal(nsISupports* aGlobal,
                                                      ErrorResult& rv);

  static const nsString DirectionToString(NotificationDirection aDirection)
  {
    switch (aDirection) {
    case NotificationDirection::Ltr:
      return NS_LITERAL_STRING("ltr");
    case NotificationDirection::Rtl:
      return NS_LITERAL_STRING("rtl");
    default:
      return NS_LITERAL_STRING("auto");
    }
  }

  static const NotificationDirection StringToDirection(const nsAString& aDirection)
  {
    if (aDirection.EqualsLiteral("ltr")) {
      return NotificationDirection::Ltr;
    }
    if (aDirection.EqualsLiteral("rtl")) {
      return NotificationDirection::Rtl;
    }
    return NotificationDirection::Auto;
  }

  static nsresult GetOrigin(nsIPrincipal* aPrincipal, nsString& aOrigin);

  void GetAlertName(nsAString& aRetval)
  {
    workers::AssertIsOnMainThread();
    if (mAlertName.IsEmpty()) {
      SetAlertName();
    }
    aRetval = mAlertName;
  }

  void GetScope(nsAString& aScope)
  {
    aScope = mScope;
  }

  void
  SetScope(const nsAString& aScope)
  {
    MOZ_ASSERT(mScope.IsEmpty());
    mScope = aScope;
  }

  const nsString mID;
  const nsString mTitle;
  const nsString mBody;
  const NotificationDirection mDir;
  const nsString mLang;
  const nsString mTag;
  const nsString mIconUrl;
  nsCOMPtr<nsIStructuredCloneContainer> mDataObjectContainer;
  const NotificationBehavior mBehavior;

  
  nsCOMPtr<nsIVariant> mData;

  nsString mAlertName;
  nsString mScope;

  
  bool mIsClosed;

  
  
  
  
  bool mIsStored;

  static uint32_t sCount;

private:
  virtual ~Notification();

  
  
  
  
  static already_AddRefed<Notification>
  CreateAndShow(nsIGlobalObject* aGlobal,
                const nsAString& aTitle,
                const NotificationOptions& aOptions,
                ErrorResult& aRv);

  nsIPrincipal* GetPrincipal();

  nsresult PersistNotification();
  void UnpersistNotification();

  void
  SetAlertName();

  bool IsTargetThread() const
  {
    return NS_IsMainThread() == !mWorkerPrivate;
  }

  bool RegisterFeature();
  void UnregisterFeature();

  nsresult ResolveIconAndSoundURL(nsString&, nsString&);

  
  UniquePtr<NotificationFeature> mFeature;
  
  uint32_t mTaskCount;
};

} 
} 

#endif 

