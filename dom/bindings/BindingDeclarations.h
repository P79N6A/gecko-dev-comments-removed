











#ifndef mozilla_dom_BindingDeclarations_h__
#define mozilla_dom_BindingDeclarations_h__

#include "nsStringGlue.h"
#include "jsapi.h"
#include "mozilla/Util.h"
#include "nsCOMPtr.h"
#include "nsDOMString.h"
#include "nsStringBuffer.h"
#include "nsTArray.h"

class nsWrapperCache;



class nsGlobalWindow;

namespace mozilla {
namespace dom {

struct MainThreadDictionaryBase
{
protected:
  JSContext* ParseJSON(const nsAString& aJSON,
                       mozilla::Maybe<JSAutoRequest>& aAr,
                       mozilla::Maybe<JSAutoCompartment>& aAc,
                       JS::Value& aVal);
};

struct EnumEntry {
  const char* value;
  size_t length;
};

class NS_STACK_CLASS GlobalObject
{
public:
  GlobalObject(JSContext* aCx, JSObject* aObject);

  nsISupports* Get() const
  {
    return mGlobalObject;
  }

  bool Failed() const
  {
    return !Get();
  }

private:
  JS::RootedObject mGlobalJSObject;
  nsISupports* mGlobalObject;
  nsCOMPtr<nsISupports> mGlobalObjectRef;
};

class NS_STACK_CLASS WorkerGlobalObject
{
public:
  WorkerGlobalObject(JSContext* aCx, JSObject* aObject);

  JSObject* Get() const
  {
    return mGlobalJSObject;
  }
  
  
  
  JSContext* GetContext() const
  {
    return mCx;
  }

  bool Failed() const
  {
    return !Get();
  }

private:
  JS::RootedObject mGlobalJSObject;
  JSContext* mCx;
};




















class NS_STACK_CLASS DOMString {
public:
  DOMString()
    : mStringBuffer(nullptr)
    , mLength(0)
    , mIsNull(false)
  {}
  ~DOMString()
  {
    MOZ_ASSERT(mString.empty() || !mStringBuffer,
               "Shouldn't have both present!");
  }

  operator nsString&()
  {
    return AsAString();
  }

  nsString& AsAString()
  {
    MOZ_ASSERT(!mStringBuffer, "We already have a stringbuffer?");
    MOZ_ASSERT(!mIsNull, "We're already set as null");
    if (mString.empty()) {
      mString.construct();
    }
    return mString.ref();
  }

  bool HasStringBuffer() const
  {
    MOZ_ASSERT(mString.empty() || !mStringBuffer,
               "Shouldn't have both present!");
    MOZ_ASSERT(!mIsNull, "Caller should have checked IsNull() first");
    return mString.empty();
  }

  
  
  
  nsStringBuffer* StringBuffer() const
  {
    MOZ_ASSERT(!mIsNull, "Caller should have checked IsNull() first");
    MOZ_ASSERT(HasStringBuffer(),
               "Don't ask for the stringbuffer if we don't have it");
    MOZ_ASSERT(StringBufferLength() != 0, "Why are you asking for this?");
    MOZ_ASSERT(mStringBuffer,
               "If our length is nonzero, we better have a stringbuffer.");
    return mStringBuffer;
  }

  
  
  uint32_t StringBufferLength() const
  {
    MOZ_ASSERT(HasStringBuffer(), "Don't call this if there is no stringbuffer");
    return mLength;
  }

  void SetStringBuffer(nsStringBuffer* aStringBuffer, uint32_t aLength)
  {
    MOZ_ASSERT(mString.empty(), "We already have a string?");
    MOZ_ASSERT(!mIsNull, "We're already set as null");
    MOZ_ASSERT(!mStringBuffer, "Setting stringbuffer twice?");
    MOZ_ASSERT(aStringBuffer, "Why are we getting null?");
    mStringBuffer = aStringBuffer;
    mLength = aLength;
  }

  void SetNull()
  {
    MOZ_ASSERT(!mStringBuffer, "Should have no stringbuffer if null");
    MOZ_ASSERT(mString.empty(), "Should have no string if null");
    mIsNull = true;
  }

  bool IsNull() const
  {
    MOZ_ASSERT(!mStringBuffer || mString.empty(),
               "How could we have a stringbuffer and a nonempty string?");
    return mIsNull || (!mString.empty() && mString.ref().IsVoid());
  }

  void ToString(nsAString& aString)
  {
    if (IsNull()) {
      SetDOMStringToNull(aString);
    } else if (HasStringBuffer()) {
      if (StringBufferLength() == 0) {
        aString.Truncate();
      } else {
        StringBuffer()->ToString(StringBufferLength(), aString);
      }
    } else {
      aString = AsAString();
    }
  }

private:
  
  Maybe<nsString> mString;

  
  
  nsStringBuffer* mStringBuffer;
  uint32_t mLength;
  bool mIsNull;
};


template<typename T>
class Optional
{
public:
  Optional()
  {}

  bool WasPassed() const
  {
    return !mImpl.empty();
  }

  void Construct()
  {
    mImpl.construct();
  }

  template <class T1>
  void Construct(const T1 &t1)
  {
    mImpl.construct(t1);
  }

  template <class T1, class T2>
  void Construct(const T1 &t1, const T2 &t2)
  {
    mImpl.construct(t1, t2);
  }

  const T& Value() const
  {
    return mImpl.ref();
  }

  T& Value()
  {
    return mImpl.ref();
  }

  
  
  

private:
  
  Optional(const Optional& other) MOZ_DELETE;
  const Optional &operator=(const Optional &other) MOZ_DELETE;

  Maybe<T> mImpl;
};






struct FakeDependentString;

template<>
class Optional<nsAString>
{
public:
  Optional() : mPassed(false) {}

  bool WasPassed() const
  {
    return mPassed;
  }

  void operator=(const nsAString* str)
  {
    MOZ_ASSERT(str);
    mStr = str;
    mPassed = true;
  }

  
  
  void operator=(const FakeDependentString* str)
  {
    MOZ_ASSERT(str);
    mStr = reinterpret_cast<const nsDependentString*>(str);
    mPassed = true;
  }

  const nsAString& Value() const
  {
    MOZ_ASSERT(WasPassed());
    return *mStr;
  }

private:
  
  Optional(const Optional& other) MOZ_DELETE;
  const Optional &operator=(const Optional &other) MOZ_DELETE;

  bool mPassed;
  const nsAString* mStr;
};





template<typename T>
class Sequence : public FallibleTArray<T>
{
public:
  Sequence() : FallibleTArray<T>()
  {}
};

class RootedJSValue
{
public:
  RootedJSValue()
    : mCx(nullptr)
  {}

  ~RootedJSValue()
  {
    if (mCx) {
      JS_RemoveValueRoot(mCx, &mValue);
    }
  }

  bool SetValue(JSContext* aCx, JS::Value aValue)
  {
    
    
    MOZ_ASSERT_IF(!aValue.isNull(), aCx);

    
    
    
    if (!aValue.isNull() && !mCx) {
      if (!JS_AddNamedValueRoot(aCx, &mValue, "RootedJSValue::mValue")) {
        return false;
      }
      mCx = aCx;
    }

    mValue = aValue;
    return true;
  }

  operator JS::Value()
  {
    return mValue;
  }

  operator const JS::Value() const
  {
    return mValue;
  }

private:
  
  
  RootedJSValue(const RootedJSValue&) MOZ_DELETE;

  JS::Value mValue;
  JSContext* mCx;
};

inline nsWrapperCache*
GetWrapperCache(nsWrapperCache* cache)
{
  return cache;
}

inline nsWrapperCache*
GetWrapperCache(nsGlobalWindow* not_allowed);

inline nsWrapperCache*
GetWrapperCache(void* p)
{
  return NULL;
}



template <template <typename> class SmartPtr, typename T>
inline nsWrapperCache*
GetWrapperCache(const SmartPtr<T>& aObject)
{
  return GetWrapperCache(aObject.get());
}

struct ParentObject {
  template<class T>
  ParentObject(T* aObject) :
    mObject(aObject),
    mWrapperCache(GetWrapperCache(aObject))
  {}

  template<class T, template<typename> class SmartPtr>
  ParentObject(const SmartPtr<T>& aObject) :
    mObject(aObject.get()),
    mWrapperCache(GetWrapperCache(aObject.get()))
  {}

  ParentObject(nsISupports* aObject, nsWrapperCache* aCache) :
    mObject(aObject),
    mWrapperCache(aCache)
  {}

  nsISupports* const mObject;
  nsWrapperCache* const mWrapperCache;
};

} 
} 

#endif 
