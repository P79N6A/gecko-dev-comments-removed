






#ifndef BASE_CALLBACK_INTERNAL_H_
#define BASE_CALLBACK_INTERNAL_H_

#include <stddef.h>

#include "base/base_export.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"

template <typename T>
class ScopedVector;

namespace base {
namespace internal {







class BindStateBase : public RefCountedThreadSafe<BindStateBase> {
 protected:
  friend class RefCountedThreadSafe<BindStateBase>;
  virtual ~BindStateBase() {}
};



class BASE_EXPORT CallbackBase {
 public:
  
  bool is_null() const;

  
  void Reset();

 protected:
  
  
  
  
  typedef void(*InvokeFuncStorage)(void);

  
  bool Equals(const CallbackBase& other) const;

  
  
  
  
  explicit CallbackBase(BindStateBase* bind_state);

  
  
  
  ~CallbackBase();

  scoped_refptr<BindStateBase> bind_state_;
  InvokeFuncStorage polymorphic_invoke_;
};












template <typename T>
struct CallbackParamTraits {
  typedef const T& ForwardType;
  typedef T StorageType;
};






template <typename T>
struct CallbackParamTraits<T&> {
  typedef T& ForwardType;
  typedef T StorageType;
};






template <typename T, size_t n>
struct CallbackParamTraits<T[n]> {
  typedef const T* ForwardType;
  typedef const T* StorageType;
};


template <typename T>
struct CallbackParamTraits<T[]> {
  typedef const T* ForwardType;
  typedef const T* StorageType;
};


















template <typename T, typename D>
struct CallbackParamTraits<scoped_ptr<T, D> > {
  typedef scoped_ptr<T, D> ForwardType;
  typedef scoped_ptr<T, D> StorageType;
};

template <typename T, typename R>
struct CallbackParamTraits<scoped_ptr_malloc<T, R> > {
  typedef scoped_ptr_malloc<T, R> ForwardType;
  typedef scoped_ptr_malloc<T, R> StorageType;
};

template <typename T>
struct CallbackParamTraits<ScopedVector<T> > {
  typedef ScopedVector<T> ForwardType;
  typedef ScopedVector<T> StorageType;
};

















template <typename T>
T& CallbackForward(T& t) { return t; }

template <typename T, typename D>
scoped_ptr<T, D> CallbackForward(scoped_ptr<T, D>& p) { return p.Pass(); }

template <typename T, typename R>
scoped_ptr_malloc<T, R> CallbackForward(scoped_ptr_malloc<T, R>& p) {
  return p.Pass();
}

template <typename T>
ScopedVector<T> CallbackForward(ScopedVector<T>& p) { return p.Pass(); }

}  
}  

#endif  
