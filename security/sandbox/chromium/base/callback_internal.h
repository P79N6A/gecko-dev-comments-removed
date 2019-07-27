






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
  
  bool is_null() const { return bind_state_.get() == NULL; }

  
  void Reset();

 protected:
  
  
  
  
  typedef void(*InvokeFuncStorage)(void);

  
  bool Equals(const CallbackBase& other) const;

  
  
  
  
  explicit CallbackBase(BindStateBase* bind_state);

  
  
  
  ~CallbackBase();

  scoped_refptr<BindStateBase> bind_state_;
  InvokeFuncStorage polymorphic_invoke_;
};




template <typename T> struct IsMoveOnlyType {
  template <typename U>
  static YesType Test(const typename U::MoveOnlyTypeForCPP03*);

  template <typename U>
  static NoType Test(...);

  static const bool value = sizeof((Test<T>(0))) == sizeof(YesType) &&
                            !is_const<T>::value;
};












template <typename T, bool is_move_only = IsMoveOnlyType<T>::value>
struct CallbackParamTraits {
  typedef const T& ForwardType;
  typedef T StorageType;
};






template <typename T>
struct CallbackParamTraits<T&, false> {
  typedef T& ForwardType;
  typedef T StorageType;
};






template <typename T, size_t n>
struct CallbackParamTraits<T[n], false> {
  typedef const T* ForwardType;
  typedef const T* StorageType;
};


template <typename T>
struct CallbackParamTraits<T[], false> {
  typedef const T* ForwardType;
  typedef const T* StorageType;
};














template <typename T>
struct CallbackParamTraits<T, true> {
  typedef T ForwardType;
  typedef T StorageType;
};

















template <typename T>
typename enable_if<!IsMoveOnlyType<T>::value, T>::type& CallbackForward(T& t) {
  return t;
}

template <typename T>
typename enable_if<IsMoveOnlyType<T>::value, T>::type CallbackForward(T& t) {
  return t.Pass();
}

}  
}  

#endif  
