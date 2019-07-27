











#ifndef mozilla_dom_BindingDeclarations_h__
#define mozilla_dom_BindingDeclarations_h__

#include "nsStringGlue.h"
#include "js/Value.h"
#include "js/RootingAPI.h"
#include "mozilla/Maybe.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsAutoPtr.h" 
#include "mozilla/dom/DOMString.h"
#include "mozilla/dom/OwningNonNull.h"

class nsWrapperCache;

namespace mozilla {
namespace dom {



struct DictionaryBase
{
protected:
  bool ParseJSON(JSContext* aCx, const nsAString& aJSON,
                 JS::MutableHandle<JS::Value> aVal);

  bool StringifyToJSON(JSContext* aCx,
                       JS::MutableHandle<JS::Value> aValue,
                       nsAString& aJSON) const;
private:
  
  
  static bool AppendJSONToString(const char16_t* aJSONData,
                                 uint32_t aDataLength, void* aString);
};




struct AllTypedArraysBase {
};




struct AllOwningUnionBase {
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

  
  
  
  JSContext* Context() const
  {
    return mCx;
  }

  bool Failed() const
  {
    return !Get();
  }

protected:
  JS::Rooted<JSObject*> mGlobalJSObject;
  JSContext* mCx;
  mutable nsISupports* mGlobalObject;
  mutable nsCOMPtr<nsISupports> mGlobalObjectRef;
};


template<typename T, typename InternalType>
class Optional_base
{
public:
  Optional_base()
  {}

  explicit Optional_base(const T& aValue)
  {
    mImpl.emplace(aValue);
  }

  template<typename T1, typename T2>
  explicit Optional_base(const T1& aValue1, const T2& aValue2)
  {
    mImpl.emplace(aValue1, aValue2);
  }

  bool WasPassed() const
  {
    return mImpl.isSome();
  }

  
  InternalType& Construct()
  {
    mImpl.emplace();
    return *mImpl;
  }

  template <class T1>
  InternalType& Construct(const T1 &t1)
  {
    mImpl.emplace(t1);
    return *mImpl;
  }

  template <class T1, class T2>
  InternalType& Construct(const T1 &t1, const T2 &t2)
  {
    mImpl.emplace(t1, t2);
    return *mImpl;
  }

  void Reset()
  {
    mImpl.reset();
  }

  const T& Value() const
  {
    return *mImpl;
  }

  
  InternalType& Value()
  {
    return *mImpl;
  }

  
  const InternalType& InternalValue() const
  {
    return *mImpl;
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

  explicit Optional(JSContext* cx) :
    Optional_base<JS::Handle<T>, JS::Rooted<T> >()
  {
    this->Construct(cx);
  }

  Optional(JSContext* cx, const T& aValue) :
    Optional_base<JS::Handle<T>, JS::Rooted<T> >(cx, aValue)
  {}

  
  
  JS::Handle<T> Value() const
  {
    return *this->mImpl;
  }

  
  
  JS::Rooted<T>& Value()
  {
    return *this->mImpl;
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

  
  JSObject*& Construct()
  {
    
    
    return Optional_base<JSObject*, JSObject*>::Construct(
      static_cast<JSObject*>(nullptr));
  }

  template <class T1>
  JSObject*& Construct(const T1& t1)
  {
    return Optional_base<JSObject*, JSObject*>::Construct(t1);
  }
};


template<>
class Optional<JS::Value>
{
private:
  Optional() MOZ_DELETE;

  explicit Optional(JS::Value aValue) MOZ_DELETE;
};


template<typename U> class NonNull;
template<typename T>
class Optional<NonNull<T> > : public Optional_base<T, NonNull<T> >
{
public:
  
  
  
  T& Value() const
  {
    return *this->mImpl->get();
  }

  
  
  NonNull<T>& Value()
  {
    return *this->mImpl;
  }
};



template<typename T>
class Optional<OwningNonNull<T> > : public Optional_base<T, OwningNonNull<T> >
{
public:
  
  
  
  T& Value() const
  {
    return *this->mImpl->get();
  }

  
  
  OwningNonNull<T>& Value()
  {
    return *this->mImpl;
  }
};






namespace binding_detail {
struct FakeString;
} 

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

  
  
  void operator=(const binding_detail::FakeString* str)
  {
    MOZ_ASSERT(str);
    mStr = reinterpret_cast<const nsString*>(str);
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

  
  operator T&() const {
    MOZ_ASSERT(inited);
    MOZ_ASSERT(ptr, "NonNull<T> was set to null");
    return *ptr;
  }

  operator T*() const {
    MOZ_ASSERT(inited);
    MOZ_ASSERT(ptr, "NonNull<T> was set to null");
    return ptr;
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
GetWrapperCache(void* p)
{
  return nullptr;
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
    mWrapperCache(GetWrapperCache(aObject)),
    mUseXBLScope(false)
  {}

  template<class T, template<typename> class SmartPtr>
  ParentObject(const SmartPtr<T>& aObject) :
    mObject(aObject.get()),
    mWrapperCache(GetWrapperCache(aObject.get())),
    mUseXBLScope(false)
  {}

  ParentObject(nsISupports* aObject, nsWrapperCache* aCache) :
    mObject(aObject),
    mWrapperCache(aCache),
    mUseXBLScope(false)
  {}

  nsISupports* const mObject;
  nsWrapperCache* const mWrapperCache;
  bool mUseXBLScope;
};

} 
} 

#endif 
