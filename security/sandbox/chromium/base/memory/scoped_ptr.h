











































































#ifndef BASE_MEMORY_SCOPED_PTR_H_
#define BASE_MEMORY_SCOPED_PTR_H_




#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include <algorithm>  

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/move.h"
#include "base/template_util.h"

namespace base {

namespace subtle {
class RefCountedBase;
class RefCountedThreadSafeBase;
}  




template <class T>
struct DefaultDeleter {
  DefaultDeleter() {}
  template <typename U> DefaultDeleter(const DefaultDeleter<U>& other) {
    
    
    
    
    
    
    
    
    
    
    
    
    enum { T_must_be_complete = sizeof(T) };
    enum { U_must_be_complete = sizeof(U) };
    COMPILE_ASSERT((base::is_convertible<U*, T*>::value),
                   U_ptr_must_implicitly_convert_to_T_ptr);
  }
  inline void operator()(T* ptr) const {
    enum { type_must_be_complete = sizeof(T) };
    delete ptr;
  }
};


template <class T>
struct DefaultDeleter<T[]> {
  inline void operator()(T* ptr) const {
    enum { type_must_be_complete = sizeof(T) };
    delete[] ptr;
  }

 private:
  
  
  
  
  
  
  
  template <typename U> void operator()(U* array) const;
};

template <class T, int n>
struct DefaultDeleter<T[n]> {
  
  COMPILE_ASSERT(sizeof(T) == -1, do_not_use_array_with_size_as_type);
};






struct FreeDeleter {
  inline void operator()(void* ptr) const {
    free(ptr);
  }
};

namespace internal {

template <typename T> struct IsNotRefCounted {
  enum {
    value = !base::is_convertible<T*, base::subtle::RefCountedBase*>::value &&
        !base::is_convertible<T*, base::subtle::RefCountedThreadSafeBase*>::
            value
  };
};

template <typename T>
struct ShouldAbortOnSelfReset {
  template <typename U>
  static NoType Test(const typename U::AllowSelfReset*);

  template <typename U>
  static YesType Test(...);

  static const bool value = sizeof(Test<T>(0)) == sizeof(YesType);
};



template <class T, class D>
class scoped_ptr_impl {
 public:
  explicit scoped_ptr_impl(T* p) : data_(p) {}

  
  scoped_ptr_impl(T* p, const D& d) : data_(p, d) {}

  
  
  template <typename U, typename V>
  scoped_ptr_impl(scoped_ptr_impl<U, V>* other)
      : data_(other->release(), other->get_deleter()) {
    
    
    
    
  }

  template <typename U, typename V>
  void TakeState(scoped_ptr_impl<U, V>* other) {
    
    
    reset(other->release());
    get_deleter() = other->get_deleter();
  }

  ~scoped_ptr_impl() {
    if (data_.ptr != nullptr) {
      
      
      static_cast<D&>(data_)(data_.ptr);
    }
  }

  void reset(T* p) {
    
    
    assert(!ShouldAbortOnSelfReset<D>::value || p == nullptr || p != data_.ptr);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    T* old = data_.ptr;
    data_.ptr = nullptr;
    if (old != nullptr)
      static_cast<D&>(data_)(old);
    data_.ptr = p;
  }

  T* get() const { return data_.ptr; }

  D& get_deleter() { return data_; }
  const D& get_deleter() const { return data_; }

  void swap(scoped_ptr_impl& p2) {
    
    
    
    using std::swap;
    swap(static_cast<D&>(data_), static_cast<D&>(p2.data_));
    swap(data_.ptr, p2.data_.ptr);
  }

  T* release() {
    T* old_ptr = data_.ptr;
    data_.ptr = nullptr;
    return old_ptr;
  }

 private:
  
  template <typename U, typename V> friend class scoped_ptr_impl;

  
  
  
  
  struct Data : public D {
    explicit Data(T* ptr_in) : ptr(ptr_in) {}
    Data(T* ptr_in, const D& other) : D(other), ptr(ptr_in) {}
    T* ptr;
  };

  Data data_;

  DISALLOW_COPY_AND_ASSIGN(scoped_ptr_impl);
};

}  

}  

















template <class T, class D = base::DefaultDeleter<T> >
class scoped_ptr {
  MOVE_ONLY_TYPE_WITH_MOVE_CONSTRUCTOR_FOR_CPP_03(scoped_ptr)

  COMPILE_ASSERT(base::internal::IsNotRefCounted<T>::value,
                 T_is_refcounted_type_and_needs_scoped_refptr);

 public:
  
  typedef T element_type;
  typedef D deleter_type;

  
  scoped_ptr() : impl_(nullptr) {}

  
  explicit scoped_ptr(element_type* p) : impl_(p) {}

  
  scoped_ptr(element_type* p, const D& d) : impl_(p, d) {}

  
  scoped_ptr(decltype(nullptr)) : impl_(nullptr) {}

  
  
  
  
  
  
  
  
  
  
  template <typename U, typename V>
  scoped_ptr(scoped_ptr<U, V>&& other)
      : impl_(&other.impl_) {
    COMPILE_ASSERT(!base::is_array<U>::value, U_cannot_be_an_array);
  }

  
  
  
  
  
  
  
  
  
  
  template <typename U, typename V>
  scoped_ptr& operator=(scoped_ptr<U, V>&& rhs) {
    COMPILE_ASSERT(!base::is_array<U>::value, U_cannot_be_an_array);
    impl_.TakeState(&rhs.impl_);
    return *this;
  }

  
  
  scoped_ptr& operator=(decltype(nullptr)) {
    reset();
    return *this;
  }

  
  
  void reset(element_type* p = nullptr) { impl_.reset(p); }

  
  
  element_type& operator*() const {
    assert(impl_.get() != nullptr);
    return *impl_.get();
  }
  element_type* operator->() const  {
    assert(impl_.get() != nullptr);
    return impl_.get();
  }
  element_type* get() const { return impl_.get(); }

  
  deleter_type& get_deleter() { return impl_.get_deleter(); }
  const deleter_type& get_deleter() const { return impl_.get_deleter(); }

  
  
  
  
  
  
  
 private:
  typedef base::internal::scoped_ptr_impl<element_type, deleter_type>
      scoped_ptr::*Testable;

 public:
  operator Testable() const {
    return impl_.get() ? &scoped_ptr::impl_ : nullptr;
  }

  
  
  
  bool operator==(const element_type* p) const { return impl_.get() == p; }
  bool operator!=(const element_type* p) const { return impl_.get() != p; }

  
  void swap(scoped_ptr& p2) {
    impl_.swap(p2.impl_);
  }

  
  
  
  
  element_type* release() WARN_UNUSED_RESULT {
    return impl_.release();
  }

 private:
  
  template <typename U, typename V> friend class scoped_ptr;
  base::internal::scoped_ptr_impl<element_type, deleter_type> impl_;

  
  explicit scoped_ptr(int disallow_construction_from_null);

  
  
  
  
  template <class U> bool operator==(scoped_ptr<U> const& p2) const;
  template <class U> bool operator!=(scoped_ptr<U> const& p2) const;
};

template <class T, class D>
class scoped_ptr<T[], D> {
  MOVE_ONLY_TYPE_WITH_MOVE_CONSTRUCTOR_FOR_CPP_03(scoped_ptr)

 public:
  
  typedef T element_type;
  typedef D deleter_type;

  
  scoped_ptr() : impl_(nullptr) {}

  
  
  
  
  
  
  
  
  
  
  
  
  
  explicit scoped_ptr(element_type* array) : impl_(array) {}

  
  scoped_ptr(decltype(nullptr)) : impl_(nullptr) {}

  
  scoped_ptr(scoped_ptr&& other) : impl_(&other.impl_) {}

  
  scoped_ptr& operator=(scoped_ptr&& rhs) {
    impl_.TakeState(&rhs.impl_);
    return *this;
  }

  
  
  scoped_ptr& operator=(decltype(nullptr)) {
    reset();
    return *this;
  }

  
  
  void reset(element_type* array = nullptr) { impl_.reset(array); }

  
  element_type& operator[](size_t i) const {
    assert(impl_.get() != nullptr);
    return impl_.get()[i];
  }
  element_type* get() const { return impl_.get(); }

  
  deleter_type& get_deleter() { return impl_.get_deleter(); }
  const deleter_type& get_deleter() const { return impl_.get_deleter(); }

  
  
 private:
  typedef base::internal::scoped_ptr_impl<element_type, deleter_type>
      scoped_ptr::*Testable;

 public:
  operator Testable() const {
    return impl_.get() ? &scoped_ptr::impl_ : nullptr;
  }

  
  
  
  bool operator==(element_type* array) const { return impl_.get() == array; }
  bool operator!=(element_type* array) const { return impl_.get() != array; }

  
  void swap(scoped_ptr& p2) {
    impl_.swap(p2.impl_);
  }

  
  
  
  
  element_type* release() WARN_UNUSED_RESULT {
    return impl_.release();
  }

 private:
  
  enum { type_must_be_complete = sizeof(element_type) };

  
  base::internal::scoped_ptr_impl<element_type, deleter_type> impl_;

  
  
  
  
  
  template <typename U> explicit scoped_ptr(U* array);
  explicit scoped_ptr(int disallow_construction_from_null);

  
  
  template <typename U> void reset(U* array);
  void reset(int disallow_reset_from_null);

  
  
  
  
  template <class U> bool operator==(scoped_ptr<U> const& p2) const;
  template <class U> bool operator!=(scoped_ptr<U> const& p2) const;
};


template <class T, class D>
void swap(scoped_ptr<T, D>& p1, scoped_ptr<T, D>& p2) {
  p1.swap(p2);
}

template <class T, class D>
bool operator==(T* p1, const scoped_ptr<T, D>& p2) {
  return p1 == p2.get();
}

template <class T, class D>
bool operator!=(T* p1, const scoped_ptr<T, D>& p2) {
  return p1 != p2.get();
}




template <typename T>
scoped_ptr<T> make_scoped_ptr(T* ptr) {
  return scoped_ptr<T>(ptr);
}

#endif  
