





































#ifndef nsDOMScriptObjectHolder_h__
#define nsDOMScriptObjectHolder_h__

#include "nsIScriptContext.h"
#include "nsIDOMScriptObjectFactory.h"

#include "jspubtd.h"





template<class T>
class NS_STACK_CLASS nsScriptObjectHolder {
public:
  
  
  nsScriptObjectHolder<T>(nsIScriptContext *ctx, T* aObject = nsnull) :
      mObject(aObject), mContext(ctx) {
    NS_ASSERTION(ctx, "Must provide a valid context");
  }

  
  nsScriptObjectHolder<T>(const nsScriptObjectHolder<T>& other) :
      mObject(other.mObject),
      mContext(other.mContext)
  {
    
    if (mObject)
      mContext->HoldScriptObject(mObject);
  }

  ~nsScriptObjectHolder<T>() {
    if (mObject)
      mContext->DropScriptObject(mObject);
  }

  
  nsScriptObjectHolder<T> &operator=(const nsScriptObjectHolder<T> &other) {
    set(other);
    return *this;
  }
  bool operator!() const {
    return !mObject;
  }
  operator bool() const {
    return !!mObject;
  }
  T* get() const {
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

  nsresult set(T* object) {
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
  nsresult set(const nsScriptObjectHolder<T> &other) {
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
  T* mObject;
  nsCOMPtr<nsIScriptContext> mContext;
};

#endif 
