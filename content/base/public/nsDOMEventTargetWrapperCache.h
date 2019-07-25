






































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

protected:
  nsDOMEventTargetWrapperCache() : nsDOMEventTargetHelper(), nsWrapperCache() {}
  virtual ~nsDOMEventTargetWrapperCache();
};

#endif  
