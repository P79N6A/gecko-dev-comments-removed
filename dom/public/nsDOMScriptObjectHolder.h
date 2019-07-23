





































#ifndef nsDOMScriptObjectHolder_h__
#define nsDOMScriptObjectHolder_h__

#include "nsIScriptContext.h"
#include "nsIDOMScriptObjectFactory.h"





class nsScriptObjectHolder {
public:
  
  
  nsScriptObjectHolder(nsIScriptContext *ctx, void *aObject = nsnull) :
      mObject(aObject), mContext(ctx) {
    NS_ASSERTION(ctx, "Must provide a valid context");
  }

  
  nsScriptObjectHolder(const nsScriptObjectHolder& other) :
      mObject(other.mObject),
      mContext(other.mContext)
  {
    
    if (mObject)
      mContext->HoldScriptObject(mObject);
  }

  ~nsScriptObjectHolder() {
    if (mObject)
      mContext->DropScriptObject(mObject);
  }

  
  nsScriptObjectHolder &operator=(const nsScriptObjectHolder &other) {
    set(other);
    return *this;
  }
  PRBool operator!() const {
    return !mObject;
  }
  operator void *() const {
    return mObject;
  }

  
  nsresult drop() {
    nsresult rv = NS_OK;
    if (mObject) {
      rv = mContext->DropScriptObject(mObject);
      mObject = nsnull;
    }
    return rv;
  }
  nsresult set(void *object) {
    NS_ASSERTION(getScriptTypeID() != nsIProgrammingLanguage::UNKNOWN,
                 "Must know the language!");
    nsresult rv = drop();
    if (NS_FAILED(rv))
      return rv;
    if (object) {
      rv = mContext->HoldScriptObject(object);
      
      if (NS_SUCCEEDED(rv)) {
        mObject = object;
      }
    }
    return rv;
  }
  nsresult set(const nsScriptObjectHolder &other) {
    NS_ASSERTION(getScriptTypeID() == other.getScriptTypeID(),
                 "Must have identical languages!");
    nsresult rv = drop();
    if (NS_FAILED(rv))
      return rv;
    return set(other.mObject);
  }
  
  PRUint32 getScriptTypeID() const {
    return mContext->GetScriptTypeID();
  }
protected:
  void *mObject;
  nsCOMPtr<nsIScriptContext> mContext;
};

#endif 
