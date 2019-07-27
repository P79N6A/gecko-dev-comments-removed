































#ifndef GOOGLE_PROTOBUF_STUBS_SHARED_PTR_H__
#define GOOGLE_PROTOBUF_STUBS_SHARED_PTR_H__

#include <google/protobuf/stubs/atomicops.h>

#include <algorithm>  
#include <stddef.h>
#include <memory>

namespace google {
namespace protobuf {
namespace internal {



#if !defined(UTIL_GTL_USE_STD_SHARED_PTR) && \
    (defined(COMPILER_MSVC) || defined(LANG_CXX11))
#define UTIL_GTL_USE_STD_SHARED_PTR 1
#endif

#if defined(UTIL_GTL_USE_STD_SHARED_PTR) && UTIL_GTL_USE_STD_SHARED_PTR






using std::enable_shared_from_this;
using std::shared_ptr;
using std::static_pointer_cast;
using std::weak_ptr;

#else  


inline bool RefCountDec(volatile Atomic32 *ptr) {
  return Barrier_AtomicIncrement(ptr, -1) != 0;
}

inline void RefCountInc(volatile Atomic32 *ptr) {
  NoBarrier_AtomicIncrement(ptr, 1);
}

template <typename T> class shared_ptr;
template <typename T> class weak_ptr;















class SharedPtrControlBlock {
  template <typename T> friend class shared_ptr;
  template <typename T> friend class weak_ptr;
 private:
  SharedPtrControlBlock() : refcount_(1), weak_count_(1) { }
  Atomic32 refcount_;
  Atomic32 weak_count_;
};


template <typename T> class enable_shared_from_this;

template <typename T>
class shared_ptr {
  template <typename U> friend class weak_ptr;
 public:
  typedef T element_type;

  shared_ptr() : ptr_(NULL), control_block_(NULL) {}

  explicit shared_ptr(T* ptr)
      : ptr_(ptr),
        control_block_(ptr != NULL ? new SharedPtrControlBlock : NULL) {
    
    
    MaybeSetupWeakThis(ptr);
  }

  
  
  template <typename U>
  shared_ptr(const shared_ptr<U>& ptr)
      : ptr_(NULL),
        control_block_(NULL) {
    Initialize(ptr);
  }
  
  shared_ptr(const shared_ptr<T>& ptr)
      : ptr_(NULL),
        control_block_(NULL) {
    Initialize(ptr);
  }

  
  
  template <typename U>
  shared_ptr<T>& operator=(const shared_ptr<U>& ptr) {
    if (ptr_ != ptr.ptr_) {
      shared_ptr<T> me(ptr);   
      swap(me);
    }
    return *this;
  }

  
  shared_ptr<T>& operator=(const shared_ptr<T>& ptr) {
    if (ptr_ != ptr.ptr_) {
      shared_ptr<T> me(ptr);   
      swap(me);
    }
    return *this;
  }

  
  
  
  
  
  
  
  

  ~shared_ptr() {
    if (ptr_ != NULL) {
      if (!RefCountDec(&control_block_->refcount_)) {
        delete ptr_;

        
        
        if (!RefCountDec(&control_block_->weak_count_)) {
          delete control_block_;
        }
      }
    }
  }

  
  
  
  
  
  
  
  template <typename Y>
  void reset(Y* p) {
    if (p != ptr_) {
      shared_ptr<T> tmp(p);
      tmp.swap(*this);
    }
  }

  void reset() {
    reset(static_cast<T*>(NULL));
  }

  
  
  
  void swap(shared_ptr<T>& r) {
    using std::swap;  
    swap(ptr_, r.ptr_);
    swap(control_block_, r.control_block_);
  }

  
  
  
  T* get() const {
    return ptr_;
  }

  T& operator*() const {
    return *ptr_;
  }

  T* operator->() const {
    return ptr_;
  }

  long use_count() const {
    return control_block_ ? control_block_->refcount_ : 1;
  }

  bool unique() const {
    return use_count() == 1;
  }

 private:
  
  
  
  
  template <typename U>
  void Initialize(const shared_ptr<U>& r) {
    
    
    
    InitializeWithStaticCast<U>(r);
  }

  
  
  
  
  
  template <typename U, typename V>
  void InitializeWithStaticCast(const shared_ptr<V>& r) {
    if (r.control_block_ != NULL) {
      RefCountInc(&r.control_block_->refcount_);

      ptr_ = static_cast<U*>(r.ptr_);
      control_block_ = r.control_block_;
    }
  }

  
  
  
  
  void MaybeSetupWeakThis(enable_shared_from_this<T>* ptr);
  void MaybeSetupWeakThis(...) { }

  T* ptr_;
  SharedPtrControlBlock* control_block_;

#ifndef SWIG
  template <typename U>
  friend class shared_ptr;

  template <typename U, typename V>
  friend shared_ptr<U> static_pointer_cast(const shared_ptr<V>& rhs);
#endif
};


template <typename T> void swap(shared_ptr<T>& r, shared_ptr<T>& s) {
  r.swap(s);
}

template <typename T, typename U>
shared_ptr<T> static_pointer_cast(const shared_ptr<U>& rhs) {
  shared_ptr<T> lhs;
  lhs.template InitializeWithStaticCast<T>(rhs);
  return lhs;
}




template <typename T>
class weak_ptr {
  template <typename U> friend class weak_ptr;
 public:
  typedef T element_type;

  
  weak_ptr() : ptr_(NULL), control_block_(NULL) { }

  
  
  
  
  template <typename U> weak_ptr(const shared_ptr<U>& ptr) {
    CopyFrom(ptr.ptr_, ptr.control_block_);
  }

  
  
  
  
  template <typename U> weak_ptr(const weak_ptr<U>& ptr) {
    CopyFrom(ptr.ptr_, ptr.control_block_);
  }

  
  weak_ptr(const weak_ptr& ptr) {
    CopyFrom(ptr.ptr_, ptr.control_block_);
  }

  
  
  
  
  
  
  ~weak_ptr() {
    if (control_block_ != NULL &&
        !RefCountDec(&control_block_->weak_count_)) {
      delete control_block_;
    }
  }

  weak_ptr& operator=(const weak_ptr& ptr) {
    if (&ptr != this) {
      weak_ptr tmp(ptr);
      tmp.swap(*this);
    }
    return *this;
  }
  template <typename U> weak_ptr& operator=(const weak_ptr<U>& ptr) {
    weak_ptr tmp(ptr);
    tmp.swap(*this);
    return *this;
  }
  template <typename U> weak_ptr& operator=(const shared_ptr<U>& ptr) {
    weak_ptr tmp(ptr);
    tmp.swap(*this);
    return *this;
  }

  void swap(weak_ptr& ptr) {
    using std::swap;  
    swap(ptr_, ptr.ptr_);
    swap(control_block_, ptr.control_block_);
  }

  void reset() {
    weak_ptr tmp;
    tmp.swap(*this);
  }

  
  
  long use_count() const {
    return control_block_ != NULL ? control_block_->refcount_ : 0;
  }

  bool expired() const { return use_count() == 0; }

  
  
  
  
  
  
  
  shared_ptr<T> lock() const {
    shared_ptr<T> result;
    if (control_block_ != NULL) {
      Atomic32 old_refcount;
      do {
        old_refcount = control_block_->refcount_;
        if (old_refcount == 0)
          break;
      } while (old_refcount !=
               NoBarrier_CompareAndSwap(
                   &control_block_->refcount_, old_refcount,
                   old_refcount + 1));
      if (old_refcount > 0) {
        result.ptr_ = ptr_;
        result.control_block_ = control_block_;
      }
    }

    return result;
  }

 private:
  void CopyFrom(T* ptr, SharedPtrControlBlock* control_block) {
    ptr_ = ptr;
    control_block_ = control_block;
    if (control_block_ != NULL)
      RefCountInc(&control_block_->weak_count_);
  }

 private:
  element_type* ptr_;
  SharedPtrControlBlock* control_block_;
};

template <typename T> void swap(weak_ptr<T>& r, weak_ptr<T>& s) {
  r.swap(s);
}




template <typename T>
class enable_shared_from_this {
  friend class shared_ptr<T>;
 public:
  
  
  
  
  shared_ptr<T> shared_from_this() {
    
    
    CHECK(!weak_this_.expired()) << "No shared_ptr owns this object";
    return weak_this_.lock();
  }
  shared_ptr<const T> shared_from_this() const {
    CHECK(!weak_this_.expired()) << "No shared_ptr owns this object";
    return weak_this_.lock();
  }

 protected:
  enable_shared_from_this() { }
  enable_shared_from_this(const enable_shared_from_this& other) { }
  enable_shared_from_this& operator=(const enable_shared_from_this& other) {
    return *this;
  }
  ~enable_shared_from_this() { }

 private:
  weak_ptr<T> weak_this_;
};






template<typename T>
void shared_ptr<T>::MaybeSetupWeakThis(enable_shared_from_this<T>* ptr) {
  if (ptr) {
    CHECK(ptr->weak_this_.expired()) << "Object already owned by a shared_ptr";
    ptr->weak_this_ = *this;
  }
}

#endif  

}  
}  
}  

#endif  
