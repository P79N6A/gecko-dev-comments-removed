





































#ifndef __NSAUTOJSOBJECTHOLDER_H__
#define __NSAUTOJSOBJECTHOLDER_H__

#include "jsapi.h"







class nsAutoJSObjectHolder
{
public:
  


  nsAutoJSObjectHolder()
  : mRt(NULL), mObj(NULL), mHeld(PR_FALSE) { }

  



  nsAutoJSObjectHolder(JSContext* aCx, JSBool* aRv = NULL,
                       JSObject* aObj = NULL)
  : mRt(NULL), mObj(aObj), mHeld(JS_FALSE) {
    JSBool rv = Hold(aCx);
    if (aRv) {
      *aRv = rv;
    }
  }

  


  nsAutoJSObjectHolder(JSRuntime* aRt, JSBool* aRv = NULL,
                       JSObject* aObj = NULL)
  : mRt(aRt), mObj(aObj), mHeld(JS_FALSE) {
    JSBool rv = Hold(aRt);
    if (aRv) {
      *aRv = rv;
    }
  }

  


  ~nsAutoJSObjectHolder() {
    Release();
  }

  


  JSBool Hold(JSContext* aCx) {
    return Hold(JS_GetRuntime(aCx));
  }

  


  JSBool Hold(JSRuntime* aRt) {
    if (!mHeld) {
      mHeld = JS_AddNamedRootRT(aRt, &mObj, "nsAutoRootedJSObject");
      if (mHeld) {
        mRt = aRt;
      }
    }
    return mHeld;
  }

  


  void Release() {
    NS_ASSERTION(!mHeld || mRt, "Bad!");
    if (mHeld) {
      mHeld = !JS_RemoveRootRT(mRt, &mObj);
      if (!mHeld) {
        mRt = NULL;
      }
      mObj = NULL;
    }
  }

  


  JSBool IsHeld() {
    return mHeld;
  }

  


  JSObject* get() const {
    return mObj;
  }

  


  operator JSObject*() const {
    return get();
  }

  


  JSObject* operator=(JSObject* aOther) {
    NS_ASSERTION(mHeld, "Not rooted!");
    return mObj = aOther;
  }

private:
  JSRuntime* mRt;
  JSObject* mObj;
  JSBool mHeld;
};

#endif 
