













































































































































#ifndef BASE_BIND_HELPERS_H_
#define BASE_BIND_HELPERS_H_

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/template_util.h"

namespace base {
namespace internal {






































































template <typename T>
class SupportsAddRefAndRelease {
  typedef char Yes[1];
  typedef char No[2];

  struct BaseMixin {
    void AddRef();
  };




#if defined(OS_WIN)
#pragma warning(push)
#pragma warning(disable:4624)
#endif
  struct Base : public T, public BaseMixin {
  };
#if defined(OS_WIN)
#pragma warning(pop)
#endif

  template <void(BaseMixin::*)(void)> struct Helper {};

  template <typename C>
  static No& Check(Helper<&C::AddRef>*);

  template <typename >
  static Yes& Check(...);

 public:
  enum { value = sizeof(Check<Base>(0)) == sizeof(Yes) };
};



template <bool IsClasstype, typename T>
struct UnsafeBindtoRefCountedArgHelper : false_type {
};

template <typename T>
struct UnsafeBindtoRefCountedArgHelper<true, T>
    : integral_constant<bool, SupportsAddRefAndRelease<T>::value> {
};

template <typename T>
struct UnsafeBindtoRefCountedArg : false_type {
};

template <typename T>
struct UnsafeBindtoRefCountedArg<T*>
    : UnsafeBindtoRefCountedArgHelper<is_class<T>::value, T> {
};

template <typename T>
class HasIsMethodTag {
  typedef char Yes[1];
  typedef char No[2];

  template <typename U>
  static Yes& Check(typename U::IsMethod*);

  template <typename U>
  static No& Check(...);

 public:
  enum { value = sizeof(Check<T>(0)) == sizeof(Yes) };
};

template <typename T>
class UnretainedWrapper {
 public:
  explicit UnretainedWrapper(T* o) : ptr_(o) {}
  T* get() const { return ptr_; }
 private:
  T* ptr_;
};

template <typename T>
class ConstRefWrapper {
 public:
  explicit ConstRefWrapper(const T& o) : ptr_(&o) {}
  const T& get() const { return *ptr_; }
 private:
  const T* ptr_;
};

template <typename T>
struct IgnoreResultHelper {
  explicit IgnoreResultHelper(T functor) : functor_(functor) {}

  T functor_;
};

template <typename T>
struct IgnoreResultHelper<Callback<T> > {
  explicit IgnoreResultHelper(const Callback<T>& functor) : functor_(functor) {}

  const Callback<T>& functor_;
};








template <typename T>
class OwnedWrapper {
 public:
  explicit OwnedWrapper(T* o) : ptr_(o) {}
  ~OwnedWrapper() { delete ptr_; }
  T* get() const { return ptr_; }
  OwnedWrapper(const OwnedWrapper& other) {
    ptr_ = other.ptr_;
    other.ptr_ = NULL;
  }

 private:
  mutable T* ptr_;
};






















template <typename T>
class PassedWrapper {
 public:
  explicit PassedWrapper(T scoper) : is_valid_(true), scoper_(scoper.Pass()) {}
  PassedWrapper(const PassedWrapper& other)
      : is_valid_(other.is_valid_), scoper_(other.scoper_.Pass()) {
  }
  T Pass() const {
    CHECK(is_valid_);
    is_valid_ = false;
    return scoper_.Pass();
  }

 private:
  mutable bool is_valid_;
  mutable T scoper_;
};


template <typename T>
struct UnwrapTraits {
  typedef const T& ForwardType;
  static ForwardType Unwrap(const T& o) { return o; }
};

template <typename T>
struct UnwrapTraits<UnretainedWrapper<T> > {
  typedef T* ForwardType;
  static ForwardType Unwrap(UnretainedWrapper<T> unretained) {
    return unretained.get();
  }
};

template <typename T>
struct UnwrapTraits<ConstRefWrapper<T> > {
  typedef const T& ForwardType;
  static ForwardType Unwrap(ConstRefWrapper<T> const_ref) {
    return const_ref.get();
  }
};

template <typename T>
struct UnwrapTraits<scoped_refptr<T> > {
  typedef T* ForwardType;
  static ForwardType Unwrap(const scoped_refptr<T>& o) { return o.get(); }
};

template <typename T>
struct UnwrapTraits<WeakPtr<T> > {
  typedef const WeakPtr<T>& ForwardType;
  static ForwardType Unwrap(const WeakPtr<T>& o) { return o; }
};

template <typename T>
struct UnwrapTraits<OwnedWrapper<T> > {
  typedef T* ForwardType;
  static ForwardType Unwrap(const OwnedWrapper<T>& o) {
    return o.get();
  }
};

template <typename T>
struct UnwrapTraits<PassedWrapper<T> > {
  typedef T ForwardType;
  static T Unwrap(PassedWrapper<T>& o) {
    return o.Pass();
  }
};



template <bool is_method, typename T>
struct MaybeRefcount;

template <typename T>
struct MaybeRefcount<false, T> {
  static void AddRef(const T&) {}
  static void Release(const T&) {}
};

template <typename T, size_t n>
struct MaybeRefcount<false, T[n]> {
  static void AddRef(const T*) {}
  static void Release(const T*) {}
};

template <typename T>
struct MaybeRefcount<true, T> {
  static void AddRef(const T&) {}
  static void Release(const T&) {}
};

template <typename T>
struct MaybeRefcount<true, T*> {
  static void AddRef(T* o) { o->AddRef(); }
  static void Release(T* o) { o->Release(); }
};



template <typename T>
struct MaybeRefcount<true, scoped_refptr<T> > {
  static void AddRef(const scoped_refptr<T>& o) {}
  static void Release(const scoped_refptr<T>& o) {}
};

template <typename T>
struct MaybeRefcount<true, const T*> {
  static void AddRef(const T* o) { o->AddRef(); }
  static void Release(const T* o) { o->Release(); }
};







template <bool IsMethod, typename P1>
struct IsWeakMethod : public false_type {};

template <typename T>
struct IsWeakMethod<true, WeakPtr<T> > : public true_type {};

template <typename T>
struct IsWeakMethod<true, ConstRefWrapper<WeakPtr<T> > > : public true_type {};

}  

template <typename T>
static inline internal::UnretainedWrapper<T> Unretained(T* o) {
  return internal::UnretainedWrapper<T>(o);
}

template <typename T>
static inline internal::ConstRefWrapper<T> ConstRef(const T& o) {
  return internal::ConstRefWrapper<T>(o);
}

template <typename T>
static inline internal::OwnedWrapper<T> Owned(T* o) {
  return internal::OwnedWrapper<T>(o);
}





template <typename T>
static inline internal::PassedWrapper<T> Passed(T scoper) {
  return internal::PassedWrapper<T>(scoper.Pass());
}
template <typename T>
static inline internal::PassedWrapper<T> Passed(T* scoper) {
  return internal::PassedWrapper<T>(scoper->Pass());
}

template <typename T>
static inline internal::IgnoreResultHelper<T> IgnoreResult(T data) {
  return internal::IgnoreResultHelper<T>(data);
}

template <typename T>
static inline internal::IgnoreResultHelper<Callback<T> >
IgnoreResult(const Callback<T>& data) {
  return internal::IgnoreResultHelper<Callback<T> >(data);
}

BASE_EXPORT void DoNothing();

template<typename T>
void DeletePointer(T* obj) {
  delete obj;
}

}  

#endif  
