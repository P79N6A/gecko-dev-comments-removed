






































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

  class NS_CYCLE_COLLECTION_INNERCLASS
    : public NS_CYCLE_COLLECTION_CLASSNAME(nsDOMEventTargetHelper)
  {
    NS_IMETHOD RootAndUnlinkJSObjects(void *p);
    NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_BODY_NO_UNLINK(nsDOMEventTargetWrapperCache,
                                                            nsDOMEventTargetHelper)
    NS_IMETHOD_(void) Trace(void *p, TraceCallback cb, void *closure);
  };
  NS_CYCLE_COLLECTION_PARTICIPANT_INSTANCE
  
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
    nsPIDOMEventTarget* target =
      static_cast<nsPIDOMEventTarget*>(aSupports);
#ifdef DEBUG
    {
      nsCOMPtr<nsPIDOMEventTarget> target_qi =
        do_QueryInterface(aSupports);

      
      
      
      NS_ASSERTION(target_qi == target, "Uh, fix QI!");
    }
#endif

    return static_cast<nsDOMEventTargetWrapperCache*>(target);
  }

protected:
  nsDOMEventTargetWrapperCache() : nsDOMEventTargetHelper(), nsWrapperCache() {}
  virtual ~nsDOMEventTargetWrapperCache();
};

#endif  
