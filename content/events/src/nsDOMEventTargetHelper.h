




#ifndef nsDOMEventTargetHelper_h_
#define nsDOMEventTargetHelper_h_

#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "nsCycleCollectionParticipant.h"
#include "nsPIDOMWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsEventListenerManager.h"
#include "nsIScriptContext.h"
#include "nsThreadUtils.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/EventTarget.h"

#define NS_DOMEVENTTARGETHELPER_IID \
{ 0xda0e6d40, 0xc17b, 0x4937, \
  { 0x8e, 0xa2, 0x99, 0xca, 0x1c, 0x81, 0xea, 0xbe } }

class nsDOMEventTargetHelper : public mozilla::dom::EventTarget
{
public:
  nsDOMEventTargetHelper()
    : mParentObject(nullptr)
    , mOwnerWindow(nullptr)
    , mHasOrHasHadOwnerWindow(false)
  {}
  nsDOMEventTargetHelper(nsPIDOMWindow* aWindow)
    : mParentObject(nullptr)
    , mOwnerWindow(nullptr)
    , mHasOrHasHadOwnerWindow(false)
  {
    BindToOwner(aWindow);
    
    SetIsDOMBinding();
  }

  virtual ~nsDOMEventTargetHelper();
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS(nsDOMEventTargetHelper)

  NS_REALLY_DECL_NSIDOMEVENTTARGET
  using mozilla::dom::EventTarget::RemoveEventListener;
  virtual void AddEventListener(const nsAString& aType,
                                nsIDOMEventListener* aListener,
                                bool aCapture,
                                const mozilla::dom::Nullable<bool>& aWantsUntrusted,
                                mozilla::ErrorResult& aRv) MOZ_OVERRIDE;

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_DOMEVENTTARGETHELPER_IID)

  void GetParentObject(nsIScriptGlobalObject **aParentObject)
  {
    if (mParentObject) {
      CallQueryInterface(mParentObject, aParentObject);
    } else {
      *aParentObject = nullptr;
    }
  }

  static nsDOMEventTargetHelper* FromSupports(nsISupports* aSupports)
  {
    mozilla::dom::EventTarget* target =
      static_cast<mozilla::dom::EventTarget*>(aSupports);
#ifdef DEBUG
    {
      nsCOMPtr<mozilla::dom::EventTarget> target_qi =
        do_QueryInterface(aSupports);

      
      
      
      NS_ASSERTION(target_qi == target, "Uh, fix QI!");
    }
#endif

    return static_cast<nsDOMEventTargetHelper*>(target);
  }

  bool HasListenersFor(nsIAtom* aTypeWithOn)
  {
    return mListenerManager && mListenerManager->HasListenersFor(aTypeWithOn);
  }

  nsresult SetEventHandler(nsIAtom* aType,
                           JSContext* aCx,
                           const JS::Value& aValue);
  using mozilla::dom::EventTarget::SetEventHandler;
  void GetEventHandler(nsIAtom* aType,
                       JSContext* aCx,
                       JS::Value* aValue);
  using mozilla::dom::EventTarget::GetEventHandler;
  virtual nsIDOMWindow* GetOwnerGlobal() MOZ_OVERRIDE
  {
    return nsPIDOMWindow::GetOuterFromCurrentInner(GetOwner());
  }

  nsresult CheckInnerWindowCorrectness()
  {
    NS_ENSURE_STATE(!mHasOrHasHadOwnerWindow || mOwnerWindow);
    if (mOwnerWindow) {
      NS_ASSERTION(mOwnerWindow->IsInnerWindow(), "Should have inner window here!\n");
      nsPIDOMWindow* outer = mOwnerWindow->GetOuterWindow();
      if (!outer || outer->GetCurrentInnerWindow() != mOwnerWindow) {
        return NS_ERROR_FAILURE;
      }
    }
    return NS_OK;
  }

  nsPIDOMWindow* GetOwner() const { return mOwnerWindow; }
  void BindToOwner(nsIGlobalObject* aOwner);
  void BindToOwner(nsPIDOMWindow* aOwner);
  void BindToOwner(nsDOMEventTargetHelper* aOther);
  virtual void DisconnectFromOwner();                   
  nsIGlobalObject* GetParentObject() const { return mParentObject; }
  bool HasOrHasHadOwner() { return mHasOrHasHadOwnerWindow; }
protected:
  nsresult WantsUntrusted(bool* aRetVal);

  nsRefPtr<nsEventListenerManager> mListenerManager;
  
  nsresult DispatchTrustedEvent(const nsAString& aEventName);
  
  nsresult DispatchTrustedEvent(nsIDOMEvent* aEvent);

  virtual void LastRelease() {}
private:
  
  nsIGlobalObject*           mParentObject;
  
  
  nsPIDOMWindow*             mOwnerWindow;
  bool                       mHasOrHasHadOwnerWindow;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsDOMEventTargetHelper,
                              NS_DOMEVENTTARGETHELPER_IID)


#define NS_IMPL_EVENT_HANDLER(_class, _event)                                 \
    NS_IMETHODIMP _class::GetOn##_event(JSContext* aCx, JS::Value* aValue)    \
    {                                                                         \
      GetEventHandler(nsGkAtoms::on##_event, aCx, aValue);                    \
      return NS_OK;                                                           \
    }                                                                         \
    NS_IMETHODIMP _class::SetOn##_event(JSContext* aCx,                       \
                                        const JS::Value& aValue)              \
    {                                                                         \
      return SetEventHandler(nsGkAtoms::on##_event, aCx, aValue);             \
    }

#define NS_IMPL_FORWARD_EVENT_HANDLER(_class, _event, _baseclass)             \
    NS_IMETHODIMP _class::GetOn##_event(JSContext* aCx, JS::Value* aValue)    \
    {                                                                         \
      return _baseclass::GetOn##_event(aCx, aValue);                          \
    }                                                                         \
    NS_IMETHODIMP _class::SetOn##_event(JSContext* aCx,                       \
                                        const JS::Value& aValue)              \
    {                                                                         \
      return _baseclass::SetOn##_event(aCx, aValue);                          \
    }


#define IMPL_EVENT_HANDLER(_event)                                        \
  inline mozilla::dom::EventHandlerNonNull* GetOn##_event()               \
  {                                                                       \
    if (NS_IsMainThread()) {                                              \
      return GetEventHandler(nsGkAtoms::on##_event, EmptyString());       \
    }                                                                     \
    return GetEventHandler(nullptr, NS_LITERAL_STRING(#_event));          \
  }                                                                       \
  inline void SetOn##_event(mozilla::dom::EventHandlerNonNull* aCallback, \
                            mozilla::ErrorResult& aRv)                    \
  {                                                                       \
    if (NS_IsMainThread()) {                                              \
      SetEventHandler(nsGkAtoms::on##_event, EmptyString(),               \
                      aCallback, aRv);                                    \
    } else {                                                              \
      SetEventHandler(nullptr, NS_LITERAL_STRING(#_event),                \
                      aCallback, aRv);                                    \
    }                                                                     \
  }






#define NS_FORWARD_NSIDOMEVENTTARGET_NOPREHANDLEEVENT(_to) \
  NS_IMETHOD AddEventListener(const nsAString & type, nsIDOMEventListener *listener, bool useCapture, bool wantsUntrusted, uint8_t _argc) { \
    return _to AddEventListener(type, listener, useCapture, wantsUntrusted, _argc); \
  } \
  NS_IMETHOD AddSystemEventListener(const nsAString & type, nsIDOMEventListener *listener, bool aUseCapture, bool aWantsUntrusted, uint8_t _argc) { \
    return _to AddSystemEventListener(type, listener, aUseCapture, aWantsUntrusted, _argc); \
  } \
  NS_IMETHOD RemoveEventListener(const nsAString & type, nsIDOMEventListener *listener, bool useCapture) { \
    return _to RemoveEventListener(type, listener, useCapture); \
  } \
  NS_IMETHOD RemoveSystemEventListener(const nsAString & type, nsIDOMEventListener *listener, bool aUseCapture) { \
    return _to RemoveSystemEventListener(type, listener, aUseCapture); \
  } \
  NS_IMETHOD DispatchEvent(nsIDOMEvent *evt, bool *_retval) { \
    return _to DispatchEvent(evt, _retval); \
  } \
  virtual mozilla::dom::EventTarget* GetTargetForDOMEvent() { \
    return _to GetTargetForDOMEvent(); \
  } \
  virtual mozilla::dom::EventTarget* GetTargetForEventTargetChain() { \
    return _to GetTargetForEventTargetChain(); \
  } \
  virtual nsresult WillHandleEvent(nsEventChainPostVisitor & aVisitor) { \
    return _to WillHandleEvent(aVisitor); \
  } \
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor & aVisitor) { \
    return _to PostHandleEvent(aVisitor); \
  } \
  virtual nsresult DispatchDOMEvent(nsEvent *aEvent, nsIDOMEvent *aDOMEvent, nsPresContext *aPresContext, nsEventStatus *aEventStatus) { \
    return _to DispatchDOMEvent(aEvent, aDOMEvent, aPresContext, aEventStatus); \
  } \
  virtual nsEventListenerManager * ListenerManager() { \
    return _to ListenerManager(); \
  } \
  virtual nsEventListenerManager * GetExistingListenerManager() const { \
    return _to GetExistingListenerManager(); \
  } \
  virtual nsIScriptContext * GetContextForEventHandlers(nsresult *aRv) { \
    return _to GetContextForEventHandlers(aRv); \
  } \
  virtual JSContext * GetJSContextForEventHandlers(void) { \
    return _to GetJSContextForEventHandlers(); \
  }

#define NS_REALLY_FORWARD_NSIDOMEVENTTARGET(_class) \
  using _class::AddEventListener;                   \
  using _class::RemoveEventListener;                \
  NS_FORWARD_NSIDOMEVENTTARGET(_class::)            \
  virtual nsEventListenerManager*                   \
  ListenerManager() {                               \
    return _class::ListenerManager();               \
  }                                                 \
  virtual nsEventListenerManager*                   \
  GetExistingListenerManager() const {              \
    return _class::GetExistingListenerManager();    \
  }

#endif 
