






































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
  virtual ~nsDOMEventTargetWrapperCache();
};

#define NS_DECL_EVENT_HANDLER(_event)                                         \
  public:                                                                     \
    NS_IMETHOD GetOn##_event(JSContext *cx, jsval *vp);                       \
    NS_IMETHOD SetOn##_event(JSContext *cx, const jsval &vp);

#define NS_DECL_AND_IMPL_EVENT_HANDLER(_event)                                \
  public:                                                                     \
    NS_IMPL_EVENT_HANDLER(_class, _event)

#define NS_IMPL_EVENT_HANDLER(_class, _event)                                 \
  NS_IMETHODIMP                                                               \
  _class::GetOn##_event(JSContext *cx, jsval *vp)                             \
  {                                                                           \
    nsEventListenerManager* elm = GetListenerManager(PR_FALSE);               \
    if (elm) {                                                                \
      elm->GetJSEventListener(nsGkAtoms::on##_event, vp);                     \
    } else {                                                                  \
      *vp = JSVAL_NULL;                                                       \
    }                                                                         \
    return NS_OK;                                                             \
  }                                                                           \
  NS_IMETHODIMP                                                               \
  _class::SetOn##_event(JSContext *cx, const jsval &vp)                       \
  {                                                                           \
    nsEventListenerManager* elm = GetListenerManager(PR_TRUE);                \
    if (!elm) {                                                               \
      return NS_ERROR_OUT_OF_MEMORY;                                          \
    }                                                                         \
                                                                              \
    JSObject *obj = GetWrapper();                                             \
    if (!obj) {                                                               \
      /* Just silently do nothing */                                          \
      return NS_OK;                                                           \
    }                                                                         \
    return elm->SetJSEventListenerToJsval(nsGkAtoms::on##_event, cx, obj, vp);\
  }

#define NS_IMPL_FORWARD_EVENT_HANDLER(_class, _event, _baseclass)             \
    NS_IMETHODIMP                                                             \
    _class::GetOn##_event(JSContext *cx, jsval *vp)                           \
    {                                                                         \
      return _baseclass::GetOn##_event(cx, vp);                               \
    }                                                                         \
    NS_IMETHODIMP                                                             \
    _class::SetOn##_event(JSContext *cx, const jsval &vp)                     \
    {                                                                         \
      return _baseclass::SetOn##_event(cx, vp);                               \
    }

#endif  
