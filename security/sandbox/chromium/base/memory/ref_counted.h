



#ifndef BASE_MEMORY_REF_COUNTED_H_
#define BASE_MEMORY_REF_COUNTED_H_

#include <cassert>
#include <iosfwd>

#include "base/atomic_ref_count.h"
#include "base/base_export.h"
#include "base/compiler_specific.h"
#ifndef NDEBUG
#include "base/logging.h"
#endif
#include "base/threading/thread_collision_warner.h"
#include "build/build_config.h"
#include "mozilla/Attributes.h"

#if defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_IOS) || defined(OS_ANDROID)
#define DISABLE_SCOPED_REFPTR_CONVERSION_OPERATOR
#endif

namespace base {

namespace subtle {

class BASE_EXPORT RefCountedBase {
 public:
  bool HasOneRef() const { return ref_count_ == 1; }

 protected:
  RefCountedBase()
      : ref_count_(0)
  #ifndef NDEBUG
      , in_dtor_(false)
  #endif
      {
  }

  ~RefCountedBase() {
  #ifndef NDEBUG
    DCHECK(in_dtor_) << "RefCounted object deleted without calling Release()";
  #endif
  }


  void AddRef() const {
    
    
    
    
  #ifndef NDEBUG
    DCHECK(!in_dtor_);
  #endif
    ++ref_count_;
  }

  
  bool Release() const {
    
    
    
    
  #ifndef NDEBUG
    DCHECK(!in_dtor_);
  #endif
    if (--ref_count_ == 0) {
  #ifndef NDEBUG
      in_dtor_ = true;
  #endif
      return true;
    }
    return false;
  }

 private:
  mutable int ref_count_;
#ifndef NDEBUG
  mutable bool in_dtor_;
#endif

  DFAKE_MUTEX(add_release_);

  DISALLOW_COPY_AND_ASSIGN(RefCountedBase);
};

class BASE_EXPORT RefCountedThreadSafeBase {
 public:
  bool HasOneRef() const;

 protected:
  RefCountedThreadSafeBase();
  ~RefCountedThreadSafeBase();

  void AddRef() const;

  
  bool Release() const;

 private:
  mutable AtomicRefCount ref_count_;
#ifndef NDEBUG
  mutable bool in_dtor_;
#endif

  DISALLOW_COPY_AND_ASSIGN(RefCountedThreadSafeBase);
};

}  















template <class T>
class RefCounted : public subtle::RefCountedBase {
 public:
  RefCounted() {}

  void AddRef() const {
    subtle::RefCountedBase::AddRef();
  }

  void Release() const {
    if (subtle::RefCountedBase::Release()) {
      delete static_cast<const T*>(this);
    }
  }

 protected:
  ~RefCounted() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(RefCounted<T>);
};


template <class T, typename Traits> class RefCountedThreadSafe;



template<typename T>
struct DefaultRefCountedThreadSafeTraits {
  static void Destruct(const T* x) {
    
    
    
    RefCountedThreadSafe<T,
                         DefaultRefCountedThreadSafeTraits>::DeleteInternal(x);
  }
};













template <class T, typename Traits = DefaultRefCountedThreadSafeTraits<T> >
class RefCountedThreadSafe : public subtle::RefCountedThreadSafeBase {
 public:
  RefCountedThreadSafe() {}

  void AddRef() const {
    subtle::RefCountedThreadSafeBase::AddRef();
  }

  void Release() const {
    if (subtle::RefCountedThreadSafeBase::Release()) {
      Traits::Destruct(static_cast<const T*>(this));
    }
  }

 protected:
  ~RefCountedThreadSafe() {}

 private:
  friend struct DefaultRefCountedThreadSafeTraits<T>;
  static void DeleteInternal(const T* x) { delete x; }

  DISALLOW_COPY_AND_ASSIGN(RefCountedThreadSafe);
};





template<typename T>
class RefCountedData
    : public base::RefCountedThreadSafe< base::RefCountedData<T> > {
 public:
  RefCountedData() : data() {}
  MOZ_IMPLICIT RefCountedData(const T& in_value) : data(in_value) {}

  T data;

 private:
  friend class base::RefCountedThreadSafe<base::RefCountedData<T> >;
  ~RefCountedData() {}
};

}  

















































template <class T>
class scoped_refptr {
 public:
  typedef T element_type;

  scoped_refptr() : ptr_(NULL) {
  }

  MOZ_IMPLICIT scoped_refptr(T* p) : ptr_(p) {
    if (ptr_)
      AddRef(ptr_);
  }

  scoped_refptr(const scoped_refptr<T>& r) : ptr_(r.ptr_) {
    if (ptr_)
      AddRef(ptr_);
  }

  template <typename U>
  scoped_refptr(const scoped_refptr<U>& r) : ptr_(r.get()) {
    if (ptr_)
      AddRef(ptr_);
  }

  ~scoped_refptr() {
    if (ptr_)
      Release(ptr_);
  }

  T* get() const { return ptr_; }

#if !defined(DISABLE_SCOPED_REFPTR_CONVERSION_OPERATOR)
  
  
  operator T*() const { return ptr_; }
#endif

  T& operator*() const {
    assert(ptr_ != NULL);
    return *ptr_;
  }

  T* operator->() const {
    assert(ptr_ != NULL);
    return ptr_;
  }

  scoped_refptr<T>& operator=(T* p) {
    
    if (p)
      AddRef(p);
    T* old_ptr = ptr_;
    ptr_ = p;
    if (old_ptr)
      Release(old_ptr);
    return *this;
  }

  scoped_refptr<T>& operator=(const scoped_refptr<T>& r) {
    return *this = r.ptr_;
  }

  template <typename U>
  scoped_refptr<T>& operator=(const scoped_refptr<U>& r) {
    return *this = r.get();
  }

  void swap(T** pp) {
    T* p = ptr_;
    ptr_ = *pp;
    *pp = p;
  }

  void swap(scoped_refptr<T>& r) {
    swap(&r.ptr_);
  }

#if defined(DISABLE_SCOPED_REFPTR_CONVERSION_OPERATOR)
  template <typename U>
  bool operator==(const scoped_refptr<U>& rhs) const {
    return ptr_ == rhs.get();
  }

  template <typename U>
  bool operator!=(const scoped_refptr<U>& rhs) const {
    return !operator==(rhs);
  }

  template <typename U>
  bool operator<(const scoped_refptr<U>& rhs) const {
    return ptr_ < rhs.get();
  }
#endif

 protected:
  T* ptr_;

 private:
  
  
  
  
  static void AddRef(T* ptr);
  static void Release(T* ptr);
};

template <typename T>
void scoped_refptr<T>::AddRef(T* ptr) {
  ptr->AddRef();
}

template <typename T>
void scoped_refptr<T>::Release(T* ptr) {
  ptr->Release();
}



template <typename T>
scoped_refptr<T> make_scoped_refptr(T* t) {
  return scoped_refptr<T>(t);
}

#if defined(DISABLE_SCOPED_REFPTR_CONVERSION_OPERATOR)

template <typename T, typename U>
bool operator==(const scoped_refptr<T>& lhs, const U* rhs) {
  return lhs.get() == rhs;
}

template <typename T, typename U>
bool operator==(const T* lhs, const scoped_refptr<U>& rhs) {
  return lhs == rhs.get();
}

template <typename T, typename U>
bool operator!=(const scoped_refptr<T>& lhs, const U* rhs) {
  return !operator==(lhs, rhs);
}

template <typename T, typename U>
bool operator!=(const T* lhs, const scoped_refptr<U>& rhs) {
  return !operator==(lhs, rhs);
}

template <typename T>
std::ostream& operator<<(std::ostream& out, const scoped_refptr<T>& p) {
  return out << p.get();
}
#endif  

#endif  
