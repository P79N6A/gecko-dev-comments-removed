























#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_SCOPED_PTR_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_SCOPED_PTR_H_

#include <assert.h>            
#include <stdlib.h>            

#include <cstddef>             

#ifdef _WIN32
namespace std { using ::ptrdiff_t; };
#endif 

namespace webrtc {

template <typename T>
class scoped_ptr {
 private:

  T* ptr;

  scoped_ptr(scoped_ptr const &);
  scoped_ptr & operator=(scoped_ptr const &);

 public:

  typedef T element_type;

  explicit scoped_ptr(T* p = NULL): ptr(p) {}

  ~scoped_ptr() {
    typedef char type_must_be_complete[sizeof(T)];
    delete ptr;
  }

  void reset(T* p = NULL) {
    typedef char type_must_be_complete[sizeof(T)];

    if (ptr != p) {
      T* obj = ptr;
      ptr = p;
      
      delete obj;
    }
  }

  T& operator*() const {
    assert(ptr != NULL);
    return *ptr;
  }

  T* operator->() const  {
    assert(ptr != NULL);
    return ptr;
  }

  T* get() const  {
    return ptr;
  }

  void swap(scoped_ptr & b) {
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
      delete ptr;
      ptr = NULL;
    }
    return &ptr;
  }

  T** use() {
    return &ptr;
  }
};

template<typename T> inline
void swap(scoped_ptr<T>& a, scoped_ptr<T>& b) {
  a.swap(b);
}








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

  T& operator[](std::ptrdiff_t i) const {
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

#endif  
