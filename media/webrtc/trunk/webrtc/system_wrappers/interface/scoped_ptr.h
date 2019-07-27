





























































































#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_SCOPED_PTR_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_SCOPED_PTR_H_




#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include <algorithm>  

#include "webrtc/system_wrappers/interface/compile_assert.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/template_util.h"
#include "webrtc/system_wrappers/source/move.h"
#include "webrtc/typedefs.h"







#if defined(__GNUC__)
#if !defined(__clang__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif 
#endif 

namespace webrtc {




template <class T>
struct DefaultDeleter {
  DefaultDeleter() {}
  template <typename U> DefaultDeleter(const DefaultDeleter<U>& other) {
    
    
    
    
    
    
    
    
    
    
    
    
    enum { T_must_be_complete = sizeof(T) };
    enum { U_must_be_complete = sizeof(U) };
    COMPILE_ASSERT((webrtc::is_convertible<U*, T*>::value),
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



template <class T, class D>
class scoped_ptr_impl {
 public:
  explicit scoped_ptr_impl(T* p) : data_(p) { }

  
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
    if (data_.ptr != NULL) {
      
      
      static_cast<D&>(data_)(data_.ptr);
    }
  }

  void reset(T* p) {
    
    if (p != NULL && p == data_.ptr)
      abort();

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    T* old = data_.ptr;
    data_.ptr = NULL;
    if (old != NULL)
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
    data_.ptr = NULL;
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

















template <class T, class D = webrtc::DefaultDeleter<T> >
class scoped_ptr {
  WEBRTC_MOVE_ONLY_TYPE_FOR_CPP_03(scoped_ptr, RValue)

 public:
  
  typedef T element_type;
  typedef D deleter_type;

  
  scoped_ptr() : impl_(NULL) { }

  
  explicit scoped_ptr(element_type* p) : impl_(p) { }

  
  scoped_ptr(element_type* p, const D& d) : impl_(p, d) { }

  
  
  
  
  
  
  
  
  
  
  template <typename U, typename V>
  scoped_ptr(scoped_ptr<U, V> other) : impl_(&other.impl_) {
    COMPILE_ASSERT(!webrtc::is_array<U>::value, U_cannot_be_an_array);
  }

  
  scoped_ptr(RValue rvalue) : impl_(&rvalue.object->impl_) { }

  
  
  
  
  
  
  
  
  
  
  template <typename U, typename V>
  scoped_ptr& operator=(scoped_ptr<U, V> rhs) {
    COMPILE_ASSERT(!webrtc::is_array<U>::value, U_cannot_be_an_array);
    impl_.TakeState(&rhs.impl_);
    return *this;
  }

  
  
  void reset(element_type* p = NULL) { impl_.reset(p); }

  
  
  element_type& operator*() const {
    assert(impl_.get() != NULL);
    return *impl_.get();
  }
  element_type* operator->() const  {
    assert(impl_.get() != NULL);
    return impl_.get();
  }
  element_type* get() const { return impl_.get(); }

  
  deleter_type& get_deleter() { return impl_.get_deleter(); }
  const deleter_type& get_deleter() const { return impl_.get_deleter(); }

  
  
  
  
  
  
  
 private:
  typedef webrtc::internal::scoped_ptr_impl<element_type, deleter_type>
      scoped_ptr::*Testable;

 public:
  operator Testable() const { return impl_.get() ? &scoped_ptr::impl_ : NULL; }

  
  
  
  bool operator==(const element_type* p) const { return impl_.get() == p; }
  bool operator!=(const element_type* p) const { return impl_.get() != p; }

  
  void swap(scoped_ptr& p2) {
    impl_.swap(p2.impl_);
  }

  
  
  
  
  
  element_type* release() WARN_UNUSED_RESULT {
    return impl_.release();
  }

  
  
  
  
  
  
  template <typename PassAsType>
  scoped_ptr<PassAsType> PassAs() {
    return scoped_ptr<PassAsType>(Pass());
  }

 private:
  
  template <typename U, typename V> friend class scoped_ptr;
  webrtc::internal::scoped_ptr_impl<element_type, deleter_type> impl_;

  
  explicit scoped_ptr(int disallow_construction_from_null);

  
  
  
  
  template <class U> bool operator==(scoped_ptr<U> const& p2) const;
  template <class U> bool operator!=(scoped_ptr<U> const& p2) const;
};

template <class T, class D>
class scoped_ptr<T[], D> {
  WEBRTC_MOVE_ONLY_TYPE_FOR_CPP_03(scoped_ptr, RValue)

 public:
  
  typedef T element_type;
  typedef D deleter_type;

  
  scoped_ptr() : impl_(NULL) { }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  explicit scoped_ptr(element_type* array) : impl_(array) { }

  
  scoped_ptr(RValue rvalue) : impl_(&rvalue.object->impl_) { }

  
  scoped_ptr& operator=(RValue rhs) {
    impl_.TakeState(&rhs.object->impl_);
    return *this;
  }

  
  
  void reset(element_type* array = NULL) { impl_.reset(array); }

  
  element_type& operator[](size_t i) const {
    assert(impl_.get() != NULL);
    return impl_.get()[i];
  }
  element_type* get() const { return impl_.get(); }

  
  deleter_type& get_deleter() { return impl_.get_deleter(); }
  const deleter_type& get_deleter() const { return impl_.get_deleter(); }

  
  
 private:
  typedef webrtc::internal::scoped_ptr_impl<element_type, deleter_type>
      scoped_ptr::*Testable;

 public:
  operator Testable() const { return impl_.get() ? &scoped_ptr::impl_ : NULL; }

  
  
  
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

  
  webrtc::internal::scoped_ptr_impl<element_type, deleter_type> impl_;

  
  
  
  
  
  template <typename U> explicit scoped_ptr(U* array);
  explicit scoped_ptr(int disallow_construction_from_null);

  
  
  template <typename U> void reset(U* array);
  void reset(int disallow_reset_from_null);

  
  
  
  
  template <class U> bool operator==(scoped_ptr<U> const& p2) const;
  template <class U> bool operator!=(scoped_ptr<U> const& p2) const;
};

}  


template <class T, class D>
void swap(webrtc::scoped_ptr<T, D>& p1, webrtc::scoped_ptr<T, D>& p2) {
  p1.swap(p2);
}

template <class T, class D>
bool operator==(T* p1, const webrtc::scoped_ptr<T, D>& p2) {
  return p1 == p2.get();
}

template <class T, class D>
bool operator!=(T* p1, const webrtc::scoped_ptr<T, D>& p2) {
  return p1 != p2.get();
}

namespace webrtc {








template<typename T>
class scoped_array {
 private:

  T* ptr;

  scoped_array(scoped_array const &);
  scoped_array & operator=(scoped_array const &);

 public:

  typedef T element_type;

  explicit scoped_array(T* p = NULL) : ptr(p) {}

  ~scoped_array() {
    typedef char type_must_be_complete[sizeof(T)];
    delete[] ptr;
  }

  void reset(T* p = NULL) {
    typedef char type_must_be_complete[sizeof(T)];

    if (ptr != p) {
      T* arr = ptr;
      ptr = p;
      
      delete [] arr;
    }
  }

  T& operator[](ptrdiff_t i) const {
    assert(ptr != NULL);
    assert(i >= 0);
    return ptr[i];
  }

  T* get() const {
    return ptr;
  }

  void swap(scoped_array & b) {
    T* tmp = b.ptr;
    b.ptr = ptr;
    ptr = tmp;
  }

  T* release() {
    T* tmp = ptr;
    ptr = NULL;
    return tmp;
  }

  T** accept() {
    if (ptr) {
      delete [] ptr;
      ptr = NULL;
    }
    return &ptr;
  }
};

template<class T> inline
void swap(scoped_array<T>& a, scoped_array<T>& b) {
  a.swap(b);
}







template<typename T, void (*FF)(void*) = free> class scoped_ptr_malloc {
 private:

  T* ptr;

  scoped_ptr_malloc(scoped_ptr_malloc const &);
  scoped_ptr_malloc & operator=(scoped_ptr_malloc const &);

 public:

  typedef T element_type;

  explicit scoped_ptr_malloc(T* p = 0): ptr(p) {}

  ~scoped_ptr_malloc() {
    FF(static_cast<void*>(ptr));
  }

  void reset(T* p = 0) {
    if (ptr != p) {
      FF(static_cast<void*>(ptr));
      ptr = p;
    }
  }

  T& operator*() const {
    assert(ptr != 0);
    return *ptr;
  }

  T* operator->() const {
    assert(ptr != 0);
    return ptr;
  }

  T* get() const {
    return ptr;
  }

  void swap(scoped_ptr_malloc & b) {
    T* tmp = b.ptr;
    b.ptr = ptr;
    ptr = tmp;
  }

  T* release() {
    T* tmp = ptr;
    ptr = 0;
    return tmp;
  }

  T** accept() {
    if (ptr) {
      FF(static_cast<void*>(ptr));
      ptr = 0;
    }
    return &ptr;
  }
};

template<typename T, void (*FF)(void*)> inline
void swap(scoped_ptr_malloc<T,FF>& a, scoped_ptr_malloc<T,FF>& b) {
  a.swap(b);
}

}  


#if defined(__GNUC__)
#if !defined(__clang__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
#pragma GCC diagnostic pop
#endif 
#endif 

#endif  
