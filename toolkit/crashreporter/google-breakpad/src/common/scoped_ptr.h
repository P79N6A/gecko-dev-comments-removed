















































#ifndef COMMON_SCOPED_PTR_H_
#define COMMON_SCOPED_PTR_H_

#include <cstddef>            
#include <assert.h>           
#include <stdlib.h>           

namespace google_breakpad {

template <typename T>
class scoped_ptr {
 private:

  T* ptr;

  scoped_ptr(scoped_ptr const &);
  scoped_ptr & operator=(scoped_ptr const &);

 public:

  typedef T element_type;

  explicit scoped_ptr(T* p = 0): ptr(p) {}

  ~scoped_ptr() {
    typedef char type_must_be_complete[sizeof(T)];
    delete ptr;
  }

  void reset(T* p = 0) {
    typedef char type_must_be_complete[sizeof(T)];

    if (ptr != p) {
      delete ptr;
      ptr = p;
    }
  }

  T& operator*() const {
    assert(ptr != 0);
    return *ptr;
  }

  T* operator->() const  {
    assert(ptr != 0);
    return ptr;
  }

  bool operator==(T* p) const {
    return ptr == p;
  }

  bool operator!=(T* p) const {
    return ptr != p;
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
    ptr = 0;
    return tmp;
  }

 private:

  
  template <typename U> bool operator==(scoped_ptr<U> const& p) const;
  template <typename U> bool operator!=(scoped_ptr<U> const& p) const;
};

template<typename T> inline
void swap(scoped_ptr<T>& a, scoped_ptr<T>& b) {
  a.swap(b);
}

template<typename T> inline
bool operator==(T* p, const scoped_ptr<T>& b) {
  return p == b.get();
}

template<typename T> inline
bool operator!=(T* p, const scoped_ptr<T>& b) {
  return p != b.get();
}





template<typename T>
class scoped_array {
 private:

  T* ptr;

  scoped_array(scoped_array const &);
  scoped_array & operator=(scoped_array const &);

 public:

  typedef T element_type;

  explicit scoped_array(T* p = 0) : ptr(p) {}

  ~scoped_array() {
    typedef char type_must_be_complete[sizeof(T)];
    delete[] ptr;
  }

  void reset(T* p = 0) {
    typedef char type_must_be_complete[sizeof(T)];

    if (ptr != p) {
      delete [] ptr;
      ptr = p;
    }
  }

  T& operator[](std::ptrdiff_t i) const {
    assert(ptr != 0);
    assert(i >= 0);
    return ptr[i];
  }

  bool operator==(T* p) const {
    return ptr == p;
  }

  bool operator!=(T* p) const {
    return ptr != p;
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
    ptr = 0;
    return tmp;
  }

 private:

  
  template <typename U> bool operator==(scoped_array<U> const& p) const;
  template <typename U> bool operator!=(scoped_array<U> const& p) const;
};

template<class T> inline
void swap(scoped_array<T>& a, scoped_array<T>& b) {
  a.swap(b);
}

template<typename T> inline
bool operator==(T* p, const scoped_array<T>& b) {
  return p == b.get();
}

template<typename T> inline
bool operator!=(T* p, const scoped_array<T>& b) {
  return p != b.get();
}




class ScopedPtrMallocFree {
 public:
  inline void operator()(void* x) const {
    free(x);
  }
};




template<typename T, typename FreeProc = ScopedPtrMallocFree>
class scoped_ptr_malloc {
 private:

  T* ptr;

  scoped_ptr_malloc(scoped_ptr_malloc const &);
  scoped_ptr_malloc & operator=(scoped_ptr_malloc const &);

 public:

  typedef T element_type;

  explicit scoped_ptr_malloc(T* p = 0): ptr(p) {}

  ~scoped_ptr_malloc() {
    typedef char type_must_be_complete[sizeof(T)];
    free_((void*) ptr);
  }

  void reset(T* p = 0) {
    typedef char type_must_be_complete[sizeof(T)];

    if (ptr != p) {
      free_((void*) ptr);
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

  bool operator==(T* p) const {
    return ptr == p;
  }

  bool operator!=(T* p) const {
    return ptr != p;
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

 private:

  
  template <typename U, typename GP>
  bool operator==(scoped_ptr_malloc<U, GP> const& p) const;
  template <typename U, typename GP>
  bool operator!=(scoped_ptr_malloc<U, GP> const& p) const;

  static FreeProc const free_;
};

template<typename T, typename FP>
FP const scoped_ptr_malloc<T,FP>::free_ = FP();

template<typename T, typename FP> inline
void swap(scoped_ptr_malloc<T,FP>& a, scoped_ptr_malloc<T,FP>& b) {
  a.swap(b);
}

template<typename T, typename FP> inline
bool operator==(T* p, const scoped_ptr_malloc<T,FP>& b) {
  return p == b.get();
}

template<typename T, typename FP> inline
bool operator!=(T* p, const scoped_ptr_malloc<T,FP>& b) {
  return p != b.get();
}

}  

#endif  
