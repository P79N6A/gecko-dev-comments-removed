



#ifndef BASE_REF_COUNTED_H_
#define BASE_REF_COUNTED_H_

#include "base/atomic_ref_count.h"
#include "base/thread_collision_warner.h"

namespace base {

namespace subtle {

class RefCountedBase {
 protected:
  RefCountedBase();
  ~RefCountedBase();

  void AddRef();

  
  bool Release();

 private:
  int ref_count_;
#ifndef NDEBUG
  bool in_dtor_;
#endif

  DFAKE_MUTEX(add_release_);

  DISALLOW_COPY_AND_ASSIGN(RefCountedBase);
};

class RefCountedThreadSafeBase {
 protected:
  RefCountedThreadSafeBase();
  ~RefCountedThreadSafeBase();

  void AddRef();

  
  bool Release();

 private:
  AtomicRefCount ref_count_;
#ifndef NDEBUG
  bool in_dtor_;
#endif

  DISALLOW_COPY_AND_ASSIGN(RefCountedThreadSafeBase);
};



}  










template <class T>
class RefCounted : public subtle::RefCountedBase {
 public:
  RefCounted() { }
  ~RefCounted() { }

  void AddRef() {
    subtle::RefCountedBase::AddRef();
  }

  void Release() {
    if (subtle::RefCountedBase::Release()) {
      delete static_cast<T*>(this);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(RefCounted<T>);
};








template <class T>
class RefCountedThreadSafe : public subtle::RefCountedThreadSafeBase {
 public:
  RefCountedThreadSafe() { }
  ~RefCountedThreadSafe() { }

  void AddRef() {
    subtle::RefCountedThreadSafeBase::AddRef();
  }

  void Release() {
    if (subtle::RefCountedThreadSafeBase::Release()) {
      delete static_cast<T*>(this);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(RefCountedThreadSafe<T>);
};





template<typename T>
class RefCountedData : public base::RefCounted< base::RefCountedData<T> > {
 public:
  RefCountedData() : data() {}
  RefCountedData(const T& in_value) : data(in_value) {}

  T data;
};

}  

















































template <class T>
class scoped_refptr {
 public:
  scoped_refptr() : ptr_(NULL) {
  }

  scoped_refptr(T* p) : ptr_(p) {
    if (ptr_)
      ptr_->AddRef();
  }

  scoped_refptr(const scoped_refptr<T>& r) : ptr_(r.ptr_) {
    if (ptr_)
      ptr_->AddRef();
  }

  ~scoped_refptr() {
    if (ptr_)
      ptr_->Release();
  }

  T* get() const { return ptr_; }
  operator T*() const { return ptr_; }
  T* operator->() const { return ptr_; }

  scoped_refptr<T>& operator=(T* p) {
    
    if (p)
      p->AddRef();
    if (ptr_ )
      ptr_ ->Release();
    ptr_ = p;
    return *this;
  }

  scoped_refptr<T>& operator=(const scoped_refptr<T>& r) {
    return *this = r.ptr_;
  }

  void swap(T** pp) {
    T* p = ptr_;
    ptr_ = *pp;
    *pp = p;
  }

  void swap(scoped_refptr<T>& r) {
    swap(&r.ptr_);
  }

 protected:
  T* ptr_;
};

#endif  
