











#ifndef mozilla_dom_BindingDeclarations_h__
#define mozilla_dom_BindingDeclarations_h__

#include "nsStringGlue.h"
#include "js/Value.h"
#include "js/RootingAPI.h"
#include "mozilla/Maybe.h"
#include "nsCOMPtr.h"
#include "nsDOMString.h"
#include "nsStringBuffer.h"
#include "nsTArray.h"
#include "nsAutoPtr.h" 
#include "mozilla/dom/OwningNonNull.h"

class nsWrapperCache;



class nsGlobalWindow;

namespace mozilla {
namespace dom {



struct DictionaryBase
{
};




struct AllTypedArraysBase {
};


struct MainThreadDictionaryBase : public DictionaryBase
{
protected:
  bool ParseJSON(JSContext *aCx, const nsAString& aJSON,
                 JS::MutableHandle<JS::Value> aVal);
};

struct EnumEntry {
  const char* value;
  size_t length;
};

class MOZ_STACK_CLASS GlobalObject
{
public:
  GlobalObject(JSContext* aCx, JSObject* aObject);

  JSObject* Get() const
  {
    return mGlobalJSObject;
  }

  nsISupports* GetAsSupports() const;

  
  
  
  JSContext* GetContext() const
  {
    return mCx;
  }

  bool Failed() const
  {
    return !Get();
  }

protected:
  JS::RootedObject mGlobalJSObject;
  JSContext* mCx;
  mutable nsISupports* mGlobalObject;
  mutable nsCOMPtr<nsISupports> mGlobalObjectRef;
};




















class MOZ_STACK_CLASS DOMString {
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


template<typename T, typename InternalType>
class Optional_base
{
public:
  Optional_base()
  {}

  explicit Optional_base(const T& aValue)
  {
    mImpl.construct(aValue);
  }

  template<typename T1, typename T2>
  explicit Optional_base(const T1& aValue1, const T2& aValue2)
  {
    mImpl.construct(aValue1, aValue2);
  }

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

  void Reset()
  {
    if (WasPassed()) {
      mImpl.destroy();
    }
  }

  const T& Value() const
  {
    return mImpl.ref();
  }

  
  InternalType& Value()
  {
    return mImpl.ref();
  }

  
  const InternalType& InternalValue() const
  {
    return mImpl.ref();
  }

  
  
  

private:
  
  Optional_base(const Optional_base& other) MOZ_DELETE;
  const Optional_base &operator=(const Optional_base &other) MOZ_DELETE;

protected:
  Maybe<InternalType> mImpl;
};

template<typename T>
class Optional : public Optional_base<T, T>
{
public:
  Optional() :
    Optional_base<T, T>()
  {}

  explicit Optional(const T& aValue) :
    Optional_base<T, T>(aValue)
  {}
};

template<typename T>
class Optional<JS::Handle<T> > :
  public Optional_base<JS::Handle<T>, JS::Rooted<T> >
{
public:
  Optional() :
    Optional_base<JS::Handle<T>, JS::Rooted<T> >()
  {}

  Optional(JSContext* cx) :
    Optional_base<JS::Handle<T>, JS::Rooted<T> >()
  {
    this->Construct(cx);
  }

  Optional(JSContext* cx, const T& aValue) :
    Optional_base<JS::Handle<T>, JS::Rooted<T> >(cx, aValue)
  {}

  
  
  JS::Handle<T> Value() const
  {
    return this->mImpl.ref();
  }

  
  
  JS::Rooted<T>& Value()
  {
    return this->mImpl.ref();
  }
};




template<>
class Optional<JSObject*> : public Optional_base<JSObject*, JSObject*>
{
public:
  Optional() :
    Optional_base<JSObject*, JSObject*>()
  {}

  explicit Optional(JSObject* aValue) :
    Optional_base<JSObject*, JSObject*>(aValue)
  {}

  
  void Construct()
  {
    
    
    Optional_base<JSObject*, JSObject*>::Construct(
      static_cast<JSObject*>(nullptr));
  }

  template <class T1>
  void Construct(const T1& t1)
  {
    Optional_base<JSObject*, JSObject*>::Construct(t1);
  }
};




template<>
class Optional<JS::Value> : public Optional_base<JS::Value, JS::Value>
{
public:
  Optional() :
    Optional_base<JS::Value, JS::Value>()
  {}

  explicit Optional(JS::Value aValue) :
    Optional_base<JS::Value, JS::Value>(aValue)
  {}

  
  void Construct()
  {
    Optional_base<JS::Value, JS::Value>::Construct(JS::UndefinedValue());
  }

  template <class T1>
  void Construct(const T1& t1)
  {
    Optional_base<JS::Value, JS::Value>::Construct(t1);
  }
};


template<typename U> class NonNull;
template<typename T>
class Optional<NonNull<T> > : public Optional_base<T, NonNull<T> >
{
public:
  
  
  
  T& Value() const
  {
    return *this->mImpl.ref().get();
  }

  
  
  NonNull<T>& Value()
  {
    return this->mImpl.ref();
  }
};



template<typename T>
class Optional<OwningNonNull<T> > : public Optional_base<T, OwningNonNull<T> >
{
public:
  
  
  
  T& Value() const
  {
    return *this->mImpl.ref().get();
  }

  
  
  OwningNonNull<T>& Value()
  {
    return this->mImpl.ref();
  }
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

template<class T>
class NonNull
{
public:
  NonNull()
#ifdef DEBUG
    : inited(false)
#endif
  {}

  operator T&() {
    MOZ_ASSERT(inited);
    MOZ_ASSERT(ptr, "NonNull<T> was set to null");
    return *ptr;
  }

  operator const T&() const {
    MOZ_ASSERT(inited);
    MOZ_ASSERT(ptr, "NonNull<T> was set to null");
    return *ptr;
  }

  void operator=(T* t) {
    ptr = t;
    MOZ_ASSERT(ptr);
#ifdef DEBUG
    inited = true;
#endif
  }

  template<typename U>
  void operator=(U* t) {
    ptr = t->ToAStringPtr();
    MOZ_ASSERT(ptr);
#ifdef DEBUG
    inited = true;
#endif
  }

  T** Slot() {
#ifdef DEBUG
    inited = true;
#endif
    return &ptr;
  }

  T* Ptr() {
    MOZ_ASSERT(inited);
    MOZ_ASSERT(ptr, "NonNull<T> was set to null");
    return ptr;
  }

  
  T* get() const {
    MOZ_ASSERT(inited);
    MOZ_ASSERT(ptr);
    return ptr;
  }

protected:
  T* ptr;
#ifdef DEBUG
  bool inited;
#endif
};





template<typename T>
class Sequence : public FallibleTArray<T>
{
public:
  Sequence() : FallibleTArray<T>()
  {}
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
