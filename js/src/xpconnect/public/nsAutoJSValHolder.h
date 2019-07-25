






































#ifndef __NSAUTOJSVALHOLDER_H__
#define __NSAUTOJSVALHOLDER_H__

#include "jsapi.h"

#include "nsDebug.h"






class nsAutoJSValHolder
{
public:

  nsAutoJSValHolder()
    : mRt(NULL)
    , mVal(JSVAL_NULL)
    , mHeld(JS_FALSE)
  {
    
  }

  


  virtual ~nsAutoJSValHolder() {
    Release();
  }

  


  JSBool Hold(JSContext* aCx) {
    return Hold(JS_GetRuntime(aCx));
  }

  



  JSBool Hold(JSRuntime* aRt) {
    if (!mHeld) {
      if (js_AddRootRT(aRt, &mVal, "nsAutoJSValHolder")) {
        mRt = aRt;
        mHeld = JS_TRUE;
      } else {
        Release(); 
      }
    }
    return mHeld;
  }

  



  jsval Release() {
    NS_ASSERTION(!mHeld || mRt, "Bad!");

    jsval oldval = mVal;

    if (mHeld) {
      js_RemoveRoot(mRt, &mVal); 
      mHeld = JS_FALSE;
    }

    mVal = JSVAL_NULL;
    mRt = NULL;

    return oldval;
  }

  


  JSBool IsHeld() {
    return mHeld;
  }

  


  JSObject* ToJSObject() const {
    return JSVAL_IS_OBJECT(mVal)
         ? JSVAL_TO_OBJECT(mVal)
         : NULL;
  }

  jsval* ToJSValPtr() {
    return &mVal;
  }

  


  operator jsval() const { return mVal; }

  nsAutoJSValHolder &operator=(JSObject* aOther) {
#ifdef DEBUG
    if (aOther) {
      NS_ASSERTION(mHeld, "Not rooted!");
    }
#endif
    return *this = OBJECT_TO_JSVAL(aOther);
  }

  nsAutoJSValHolder &operator=(jsval aOther) {
#ifdef DEBUG
    if (JSVAL_IS_OBJECT(aOther) && JSVAL_TO_OBJECT(aOther)) {
      NS_ASSERTION(mHeld, "Not rooted!");
    }
#endif
    mVal = aOther;
    return *this;
  }

private:
  JSRuntime* mRt;
  jsval mVal;
  JSBool mHeld;
};

#endif 
