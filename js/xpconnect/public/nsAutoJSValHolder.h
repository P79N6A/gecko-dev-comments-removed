






































#ifndef __NSAUTOJSVALHOLDER_H__
#define __NSAUTOJSVALHOLDER_H__

#include "jsapi.h"

#include "nsDebug.h"






class nsAutoJSValHolder
{
public:
  nsAutoJSValHolder() : mVal(JSVAL_NULL), mRt(nsnull)
  {
    
  }

  


  virtual ~nsAutoJSValHolder() {
    Release();
  }

  nsAutoJSValHolder(const nsAutoJSValHolder& aOther) {
    *this = aOther;
  }

  nsAutoJSValHolder& operator=(const nsAutoJSValHolder& aOther) {
    if (this != &aOther) {
      if (aOther.IsHeld()) {
        
        this->Hold(aOther.mRt);
      }
      else {
        this->Release();
      }
      *this = static_cast<jsval>(aOther);
    }
    return *this;
  }

  


  bool Hold(JSContext* aCx) {
    return Hold(JS_GetRuntime(aCx));
  }

  



  bool Hold(JSRuntime* aRt) {
    
    if (mRt && aRt != mRt) {
      js_RemoveRoot(mRt, &mVal);
      mRt = nsnull;
    }

    if (!mRt && js_AddRootRT(aRt, &mVal, "nsAutoJSValHolder")) {
      mRt = aRt;
    }

    return !!mRt;
  }

  



  jsval Release() {
    jsval oldval = mVal;

    if (mRt) {
      js_RemoveRoot(mRt, &mVal); 
      mRt = nsnull;
    }

    mVal = JSVAL_NULL;

    return oldval;
  }

  


  bool IsHeld() const {
    return !!mRt;
  }

  


  JSObject* ToJSObject() const {
    return JSVAL_IS_OBJECT(mVal)
         ? JSVAL_TO_OBJECT(mVal)
         : nsnull;
  }

  jsval* ToJSValPtr() {
    return &mVal;
  }

  


  operator jsval() const { return mVal; }

  nsAutoJSValHolder &operator=(JSObject* aOther) {
    return *this = OBJECT_TO_JSVAL(aOther);
  }

  nsAutoJSValHolder &operator=(jsval aOther) {
#ifdef DEBUG
    if (JSVAL_IS_GCTHING(aOther) && !JSVAL_IS_NULL(aOther)) {
      NS_ASSERTION(IsHeld(), "Not rooted!");
    }
#endif
    mVal = aOther;
    return *this;
  }

private:
  jsval mVal;
  JSRuntime* mRt;
};

#endif 
