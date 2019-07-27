







#ifndef mozilla_RefPtr_h
#define mozilla_RefPtr_h

#include "mozilla/Assertions.h"
#include "mozilla/Atomics.h"
#include "mozilla/Attributes.h"
#include "mozilla/NullPtr.h"
#include "mozilla/RefCountType.h"
#include "mozilla/TypeTraits.h"
#if defined(MOZILLA_INTERNAL_API)
#include "nsXPCOM.h"
#endif

#if defined(MOZILLA_INTERNAL_API) && \
    (defined(DEBUG) || defined(FORCE_BUILD_REFCNT_LOGGING))
#define MOZ_REFCOUNTED_LEAK_CHECKING
#endif

namespace mozilla {

template<typename T> class RefCounted;
template<typename T> class RefPtr;
template<typename T> class TemporaryRef;
template<typename T> class OutParamRef;
template<typename T> OutParamRef<T> byRef(RefPtr<T>&);


























namespace detail {
#ifdef DEBUG
const MozRefCountType DEAD = 0xffffdead;
#endif



#ifdef MOZ_REFCOUNTED_LEAK_CHECKING
class RefCountLogger
{
public:
  static void logAddRef(const void* aPointer, MozRefCountType aRefCount,
                        const char* aTypeName, uint32_t aInstanceSize)
  {
    MOZ_ASSERT(aRefCount != DEAD);
    NS_LogAddRef(const_cast<void*>(aPointer), aRefCount, aTypeName,
                 aInstanceSize);
  }

  static void logRelease(const void* aPointer, MozRefCountType aRefCount,
                         const char* aTypeName)
  {
    MOZ_ASSERT(aRefCount != DEAD);
    NS_LogRelease(const_cast<void*>(aPointer), aRefCount, aTypeName);
  }
};
#endif


enum RefCountAtomicity
{
  AtomicRefCount,
  NonAtomicRefCount
};

template<typename T, RefCountAtomicity Atomicity>
class RefCounted
{
  friend class RefPtr<T>;

protected:
  RefCounted() : mRefCnt(0) {}
  ~RefCounted() { MOZ_ASSERT(mRefCnt == detail::DEAD); }

public:
  
  void AddRef() const
  {
    
    MOZ_ASSERT(int32_t(mRefCnt) >= 0);
#ifndef MOZ_REFCOUNTED_LEAK_CHECKING
    ++mRefCnt;
#else
    const char* type = static_cast<const T*>(this)->typeName();
    uint32_t size = static_cast<const T*>(this)->typeSize();
    const void* ptr = static_cast<const T*>(this);
    MozRefCountType cnt = ++mRefCnt;
    detail::RefCountLogger::logAddRef(ptr, cnt, type, size);
#endif
  }

  void Release() const
  {
    
    MOZ_ASSERT(int32_t(mRefCnt) > 0);
#ifndef MOZ_REFCOUNTED_LEAK_CHECKING
    MozRefCountType cnt = --mRefCnt;
#else
    const char* type = static_cast<const T*>(this)->typeName();
    const void* ptr = static_cast<const T*>(this);
    MozRefCountType cnt = --mRefCnt;
    
    
    detail::RefCountLogger::logRelease(ptr, cnt, type);
#endif
    if (0 == cnt) {
      
      
      
      
#ifdef DEBUG
      mRefCnt = detail::DEAD;
#endif
      delete static_cast<const T*>(this);
    }
  }

  
  void ref() { AddRef(); }
  void deref() { Release(); }
  MozRefCountType refCount() const { return mRefCnt; }
  bool hasOneRef() const
  {
    MOZ_ASSERT(mRefCnt > 0);
    return mRefCnt == 1;
  }

private:
  mutable typename Conditional<Atomicity == AtomicRefCount,
                               Atomic<MozRefCountType>,
                               MozRefCountType>::Type mRefCnt;
};

#ifdef MOZ_REFCOUNTED_LEAK_CHECKING
#define MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(T) \
  virtual const char* typeName() const { return #T; } \
  virtual size_t typeSize() const { return sizeof(*this); }
#else
#define MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(T)
#endif




#define MOZ_DECLARE_REFCOUNTED_TYPENAME(T) \
  const char* typeName() const { return #T; } \
  size_t typeSize() const { return sizeof(*this); }

} 

template<typename T>
class RefCounted : public detail::RefCounted<T, detail::NonAtomicRefCount>
{
public:
  ~RefCounted()
  {
    static_assert(IsBaseOf<RefCounted, T>::value,
                  "T must derive from RefCounted<T>");
  }
};

namespace external {








template<typename T>
class AtomicRefCounted :
  public mozilla::detail::RefCounted<T, mozilla::detail::AtomicRefCount>
{
public:
  ~AtomicRefCounted()
  {
    static_assert(IsBaseOf<AtomicRefCounted, T>::value,
                  "T must derive from AtomicRefCounted<T>");
  }
};

} 











template<typename T>
class RefPtr
{
  
  friend class TemporaryRef<T>;
  friend class OutParamRef<T>;

  struct DontRef {};

public:
  RefPtr() : mPtr(0) {}
  RefPtr(const RefPtr& aOther) : mPtr(ref(aOther.mPtr)) {}
  MOZ_IMPLICIT RefPtr(const TemporaryRef<T>& aOther) : mPtr(aOther.drop()) {}
  MOZ_IMPLICIT RefPtr(T* aVal) : mPtr(ref(aVal)) {}

  template<typename U>
  RefPtr(const RefPtr<U>& aOther) : mPtr(ref(aOther.get())) {}

  ~RefPtr() { unref(mPtr); }

  RefPtr& operator=(const RefPtr& aOther)
  {
    assign(ref(aOther.mPtr));
    return *this;
  }
  RefPtr& operator=(const TemporaryRef<T>& aOther)
  {
    assign(aOther.drop());
    return *this;
  }
  RefPtr& operator=(T* aVal)
  {
    assign(ref(aVal));
    return *this;
  }

  template<typename U>
  RefPtr& operator=(const RefPtr<U>& aOther)
  {
    assign(ref(aOther.get()));
    return *this;
  }

  TemporaryRef<T> forget()
  {
    T* tmp = mPtr;
    mPtr = nullptr;
    return TemporaryRef<T>(tmp, DontRef());
  }

  T* get() const { return mPtr; }
  operator T*() const { return mPtr; }
  T* operator->() const { return mPtr; }
  T& operator*() const { return *mPtr; }
  template<typename U>
  operator TemporaryRef<U>() { return TemporaryRef<U>(mPtr); }

private:
  void assign(T* aVal)
  {
    unref(mPtr);
    mPtr = aVal;
  }

  T* mPtr;

  static MOZ_ALWAYS_INLINE T* ref(T* aVal)
  {
    if (aVal) {
      aVal->AddRef();
    }
    return aVal;
  }

  static MOZ_ALWAYS_INLINE void unref(T* aVal)
  {
    if (aVal) {
      aVal->Release();
    }
  }
};







template<typename T>
class TemporaryRef
{
  
  friend class RefPtr<T>;

  typedef typename RefPtr<T>::DontRef DontRef;

public:
  MOZ_IMPLICIT TemporaryRef(T* aVal) : mPtr(RefPtr<T>::ref(aVal)) {}
  TemporaryRef(const TemporaryRef& aOther) : mPtr(aOther.drop()) {}

  template<typename U>
  TemporaryRef(const TemporaryRef<U>& aOther) : mPtr(aOther.drop()) {}

  ~TemporaryRef() { RefPtr<T>::unref(mPtr); }

  T* drop() const
  {
    T* tmp = mPtr;
    mPtr = nullptr;
    return tmp;
  }

private:
  TemporaryRef(T* aVal, const DontRef&) : mPtr(aVal) {}

  mutable T* mPtr;

  TemporaryRef() MOZ_DELETE;
  void operator=(const TemporaryRef&) MOZ_DELETE;
};















template<typename T>
class OutParamRef
{
  friend OutParamRef byRef<T>(RefPtr<T>&);

public:
  ~OutParamRef()
  {
    RefPtr<T>::unref(mRefPtr.mPtr);
    mRefPtr.mPtr = mTmp;
  }

  operator T**() { return &mTmp; }

private:
  explicit OutParamRef(RefPtr<T>& p) : mRefPtr(p), mTmp(p.get()) {}

  RefPtr<T>& mRefPtr;
  T* mTmp;

  OutParamRef() MOZ_DELETE;
  OutParamRef& operator=(const OutParamRef&) MOZ_DELETE;
};




template<typename T>
OutParamRef<T>
byRef(RefPtr<T>& aPtr)
{
  return OutParamRef<T>(aPtr);
}

} 

#endif 
