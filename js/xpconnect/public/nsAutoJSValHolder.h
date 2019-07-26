




#ifndef __NSAUTOJSVALHOLDER_H__
#define __NSAUTOJSVALHOLDER_H__

#include "nsDebug.h"
#include "jsapi.h"






class nsAutoJSValHolder
{
public:
  nsAutoJSValHolder()
    : mVal(JSVAL_NULL), mRt(nullptr)
  {
    
  }

  


  virtual ~nsAutoJSValHolder() {
    Release();
  }

  nsAutoJSValHolder(const nsAutoJSValHolder& aOther)
    : mVal(JSVAL_NULL), mRt(nullptr)
  {
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
      *this = static_cast<JS::Value>(aOther);
    }
    return *this;
  }

  


  bool Hold(JSContext* aCx) {
    return Hold(JS_GetRuntime(aCx));
  }

  



  bool Hold(JSRuntime* aRt) {
    MOZ_ASSERT_IF(mRt, mRt == aRt);

    if (!mRt && JS::AddNamedValueRootRT(aRt, &mVal, "nsAutoJSValHolder")) {
      mRt = aRt;
    }

    return !!mRt;
  }

  



  JS::Value Release() {
    JS::Value oldval = mVal;

    if (mRt) {
      JS::RemoveValueRootRT(mRt, &mVal); 
      mRt = nullptr;
    }

    mVal = JSVAL_NULL;

    return oldval;
  }

  


  bool IsHeld() const {
    return !!mRt;
  }

  


  JSObject* ToJSObject() const {
    return mVal.isObject()
         ? &mVal.toObject()
         : nullptr;
  }

  


  operator JS::Value() const { return mVal; }
  JS::Value get() const { return mVal; }

  nsAutoJSValHolder &operator=(JSObject* aOther) {
    return *this = OBJECT_TO_JSVAL(aOther);
  }

  nsAutoJSValHolder &operator=(JS::Value aOther) {
#ifdef DEBUG
    if (JSVAL_IS_GCTHING(aOther) && !JSVAL_IS_NULL(aOther)) {
      MOZ_ASSERT(IsHeld(), "Not rooted!");
    }
#endif
    mVal = aOther;
    return *this;
  }

private:
  JS::Heap<JS::Value> mVal;
  JSRuntime* mRt;
};

#endif 
