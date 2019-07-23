






































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
    , mGCThing(NULL)
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
      if (JS_AddNamedRootRT(aRt, &mGCThing, "nsAutoJSValHolder")) {
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
      JS_RemoveRootRT(mRt, &mGCThing); 
      mHeld = JS_FALSE;
    }

    mVal = JSVAL_NULL;
    mGCThing = NULL;
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
    if (aOther) {
      NS_ASSERTION(mHeld, "Not rooted!");
    }
#endif
    mVal = aOther;
    mGCThing = JSVAL_IS_GCTHING(aOther)
             ? JSVAL_TO_GCTHING(aOther)
             : NULL;
    return *this;
  }

private:
  JSRuntime* mRt;
  jsval mVal;
  void* mGCThing;
  JSBool mHeld;
};

#endif 
