






































#ifndef nsDOMEventTargetWrapperCache_h__
#define nsDOMEventTargetWrapperCache_h__

#include "nsDOMEventTargetHelper.h"
#include "nsWrapperCache.h"
#include "nsIScriptContext.h"





class nsDOMEventTargetWrapperCache : public nsDOMEventTargetHelper,
                                     public nsWrapperCache
{
public:  
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(nsDOMEventTargetWrapperCache,
                                                         nsDOMEventTargetHelper)
  
  void GetParentObject(nsIScriptGlobalObject **aParentObject)
  {
    if (mOwner) {
      CallQueryInterface(mOwner, aParentObject);
    }
    else {
      *aParentObject = nsnull;
    }
  }

  static nsDOMEventTargetWrapperCache* FromSupports(nsISupports* aSupports)
  {
    nsIDOMEventTarget* target =
      static_cast<nsIDOMEventTarget*>(aSupports);
#ifdef DEBUG
    {
      nsCOMPtr<nsIDOMEventTarget> target_qi =
        do_QueryInterface(aSupports);

      
      
      
      NS_ASSERTION(target_qi == target, "Uh, fix QI!");
    }
#endif

    return static_cast<nsDOMEventTargetWrapperCache*>(target);
  }

  void Init(JSContext* aCx = nsnull);

protected:
  nsDOMEventTargetWrapperCache() : nsDOMEventTargetHelper(), nsWrapperCache() {}
  virtual ~nsDOMEventTargetWrapperCache() {}
};

#define NS_DECL_EVENT_HANDLER(_event)                                         \
  protected:                                                                  \
    nsRefPtr<nsDOMEventListenerWrapper> mOn##_event##Listener;                \
  public:

#define NS_DECL_AND_IMPL_EVENT_HANDLER(_event)                                \
  protected:                                                                  \
    nsRefPtr<nsDOMEventListenerWrapper> mOn##_event##Listener;                \
  public:                                                                     \
    NS_IMETHOD GetOn##_event(nsIDOMEventListener** a##_event)                 \
    {                                                                         \
      return GetInnerEventListener(mOn##_event##Listener, a##_event);         \
    }                                                                         \
    NS_IMETHOD SetOn##_event(nsIDOMEventListener* a##_event)                  \
    {                                                                         \
      return RemoveAddEventListener(NS_LITERAL_STRING(#_event),               \
                                    mOn##_event##Listener, a##_event);        \
    }

#define NS_IMPL_EVENT_HANDLER(_class, _event)                                 \
  NS_IMETHODIMP                                                               \
  _class::GetOn##_event(nsIDOMEventListener** a##_event)                      \
  {                                                                           \
    return GetInnerEventListener(mOn##_event##Listener, a##_event);           \
  }                                                                           \
  NS_IMETHODIMP                                                               \
  _class::SetOn##_event(nsIDOMEventListener* a##_event)                       \
  {                                                                           \
    return RemoveAddEventListener(NS_LITERAL_STRING(#_event),                 \
                                  mOn##_event##Listener, a##_event);          \
  }

#define NS_IMPL_FORWARD_EVENT_HANDLER(_class, _event, _baseclass)             \
    NS_IMETHODIMP                                                             \
    _class::GetOn##_event(nsIDOMEventListener** a##_event)                    \
    {                                                                         \
      return _baseclass::GetOn##_event(a##_event);                            \
    }                                                                         \
    NS_IMETHODIMP                                                             \
    _class::SetOn##_event(nsIDOMEventListener* a##_event)                     \
    {                                                                         \
      return _baseclass::SetOn##_event(a##_event);                            \
    }

#define NS_CYCLE_COLLECTION_TRAVERSE_EVENT_HANDLER(_event)                    \
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOn##_event##Listener)

#define NS_CYCLE_COLLECTION_UNLINK_EVENT_HANDLER(_event)                      \
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOn##_event##Listener)
                                    

#endif  
