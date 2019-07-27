








#ifndef BASE_CALLBACK_H_
#define BASE_CALLBACK_H_

#include "base/callback_forward.h"
#include "base/callback_internal.h"
#include "base/template_util.h"














































































































































































































































































































































namespace base {











template <typename Sig>
class Callback;

namespace internal {
template <typename Runnable, typename RunType, typename BoundArgsType>
struct BindState;
}  

template <typename R>
class Callback<R(void)> : public internal::CallbackBase {
 public:
  typedef R(RunType)();

  Callback() : CallbackBase(NULL) { }

  
  
  template <typename Runnable, typename BindRunType, typename BoundArgsType>
  Callback(internal::BindState<Runnable, BindRunType,
           BoundArgsType>* bind_state)
      : CallbackBase(bind_state) {

    
    
    
    PolymorphicInvoke invoke_func =
        &internal::BindState<Runnable, BindRunType, BoundArgsType>
            ::InvokerType::Run;
    polymorphic_invoke_ = reinterpret_cast<InvokeFuncStorage>(invoke_func);
  }

  bool Equals(const Callback& other) const {
    return CallbackBase::Equals(other);
  }

  R Run() const {
    PolymorphicInvoke f =
        reinterpret_cast<PolymorphicInvoke>(polymorphic_invoke_);

    return f(bind_state_.get());
  }

 private:
  typedef R(*PolymorphicInvoke)(
      internal::BindStateBase*);

};

template <typename R, typename A1>
class Callback<R(A1)> : public internal::CallbackBase {
 public:
  typedef R(RunType)(A1);

  Callback() : CallbackBase(NULL) { }

  
  
  template <typename Runnable, typename BindRunType, typename BoundArgsType>
  Callback(internal::BindState<Runnable, BindRunType,
           BoundArgsType>* bind_state)
      : CallbackBase(bind_state) {

    
    
    
    PolymorphicInvoke invoke_func =
        &internal::BindState<Runnable, BindRunType, BoundArgsType>
            ::InvokerType::Run;
    polymorphic_invoke_ = reinterpret_cast<InvokeFuncStorage>(invoke_func);
  }

  bool Equals(const Callback& other) const {
    return CallbackBase::Equals(other);
  }

  R Run(typename internal::CallbackParamTraits<A1>::ForwardType a1) const {
    PolymorphicInvoke f =
        reinterpret_cast<PolymorphicInvoke>(polymorphic_invoke_);

    return f(bind_state_.get(), internal::CallbackForward(a1));
  }

 private:
  typedef R(*PolymorphicInvoke)(
      internal::BindStateBase*,
          typename internal::CallbackParamTraits<A1>::ForwardType);

};

template <typename R, typename A1, typename A2>
class Callback<R(A1, A2)> : public internal::CallbackBase {
 public:
  typedef R(RunType)(A1, A2);

  Callback() : CallbackBase(NULL) { }

  
  
  template <typename Runnable, typename BindRunType, typename BoundArgsType>
  Callback(internal::BindState<Runnable, BindRunType,
           BoundArgsType>* bind_state)
      : CallbackBase(bind_state) {

    
    
    
    PolymorphicInvoke invoke_func =
        &internal::BindState<Runnable, BindRunType, BoundArgsType>
            ::InvokerType::Run;
    polymorphic_invoke_ = reinterpret_cast<InvokeFuncStorage>(invoke_func);
  }

  bool Equals(const Callback& other) const {
    return CallbackBase::Equals(other);
  }

  R Run(typename internal::CallbackParamTraits<A1>::ForwardType a1,
        typename internal::CallbackParamTraits<A2>::ForwardType a2) const {
    PolymorphicInvoke f =
        reinterpret_cast<PolymorphicInvoke>(polymorphic_invoke_);

    return f(bind_state_.get(), internal::CallbackForward(a1),
             internal::CallbackForward(a2));
  }

 private:
  typedef R(*PolymorphicInvoke)(
      internal::BindStateBase*,
          typename internal::CallbackParamTraits<A1>::ForwardType,
          typename internal::CallbackParamTraits<A2>::ForwardType);

};

template <typename R, typename A1, typename A2, typename A3>
class Callback<R(A1, A2, A3)> : public internal::CallbackBase {
 public:
  typedef R(RunType)(A1, A2, A3);

  Callback() : CallbackBase(NULL) { }

  
  
  template <typename Runnable, typename BindRunType, typename BoundArgsType>
  Callback(internal::BindState<Runnable, BindRunType,
           BoundArgsType>* bind_state)
      : CallbackBase(bind_state) {

    
    
    
    PolymorphicInvoke invoke_func =
        &internal::BindState<Runnable, BindRunType, BoundArgsType>
            ::InvokerType::Run;
    polymorphic_invoke_ = reinterpret_cast<InvokeFuncStorage>(invoke_func);
  }

  bool Equals(const Callback& other) const {
    return CallbackBase::Equals(other);
  }

  R Run(typename internal::CallbackParamTraits<A1>::ForwardType a1,
        typename internal::CallbackParamTraits<A2>::ForwardType a2,
        typename internal::CallbackParamTraits<A3>::ForwardType a3) const {
    PolymorphicInvoke f =
        reinterpret_cast<PolymorphicInvoke>(polymorphic_invoke_);

    return f(bind_state_.get(), internal::CallbackForward(a1),
             internal::CallbackForward(a2),
             internal::CallbackForward(a3));
  }

 private:
  typedef R(*PolymorphicInvoke)(
      internal::BindStateBase*,
          typename internal::CallbackParamTraits<A1>::ForwardType,
          typename internal::CallbackParamTraits<A2>::ForwardType,
          typename internal::CallbackParamTraits<A3>::ForwardType);

};

template <typename R, typename A1, typename A2, typename A3, typename A4>
class Callback<R(A1, A2, A3, A4)> : public internal::CallbackBase {
 public:
  typedef R(RunType)(A1, A2, A3, A4);

  Callback() : CallbackBase(NULL) { }

  
  
  template <typename Runnable, typename BindRunType, typename BoundArgsType>
  Callback(internal::BindState<Runnable, BindRunType,
           BoundArgsType>* bind_state)
      : CallbackBase(bind_state) {

    
    
    
    PolymorphicInvoke invoke_func =
        &internal::BindState<Runnable, BindRunType, BoundArgsType>
            ::InvokerType::Run;
    polymorphic_invoke_ = reinterpret_cast<InvokeFuncStorage>(invoke_func);
  }

  bool Equals(const Callback& other) const {
    return CallbackBase::Equals(other);
  }

  R Run(typename internal::CallbackParamTraits<A1>::ForwardType a1,
        typename internal::CallbackParamTraits<A2>::ForwardType a2,
        typename internal::CallbackParamTraits<A3>::ForwardType a3,
        typename internal::CallbackParamTraits<A4>::ForwardType a4) const {
    PolymorphicInvoke f =
        reinterpret_cast<PolymorphicInvoke>(polymorphic_invoke_);

    return f(bind_state_.get(), internal::CallbackForward(a1),
             internal::CallbackForward(a2),
             internal::CallbackForward(a3),
             internal::CallbackForward(a4));
  }

 private:
  typedef R(*PolymorphicInvoke)(
      internal::BindStateBase*,
          typename internal::CallbackParamTraits<A1>::ForwardType,
          typename internal::CallbackParamTraits<A2>::ForwardType,
          typename internal::CallbackParamTraits<A3>::ForwardType,
          typename internal::CallbackParamTraits<A4>::ForwardType);

};

template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5>
class Callback<R(A1, A2, A3, A4, A5)> : public internal::CallbackBase {
 public:
  typedef R(RunType)(A1, A2, A3, A4, A5);

  Callback() : CallbackBase(NULL) { }

  
  
  template <typename Runnable, typename BindRunType, typename BoundArgsType>
  Callback(internal::BindState<Runnable, BindRunType,
           BoundArgsType>* bind_state)
      : CallbackBase(bind_state) {

    
    
    
    PolymorphicInvoke invoke_func =
        &internal::BindState<Runnable, BindRunType, BoundArgsType>
            ::InvokerType::Run;
    polymorphic_invoke_ = reinterpret_cast<InvokeFuncStorage>(invoke_func);
  }

  bool Equals(const Callback& other) const {
    return CallbackBase::Equals(other);
  }

  R Run(typename internal::CallbackParamTraits<A1>::ForwardType a1,
        typename internal::CallbackParamTraits<A2>::ForwardType a2,
        typename internal::CallbackParamTraits<A3>::ForwardType a3,
        typename internal::CallbackParamTraits<A4>::ForwardType a4,
        typename internal::CallbackParamTraits<A5>::ForwardType a5) const {
    PolymorphicInvoke f =
        reinterpret_cast<PolymorphicInvoke>(polymorphic_invoke_);

    return f(bind_state_.get(), internal::CallbackForward(a1),
             internal::CallbackForward(a2),
             internal::CallbackForward(a3),
             internal::CallbackForward(a4),
             internal::CallbackForward(a5));
  }

 private:
  typedef R(*PolymorphicInvoke)(
      internal::BindStateBase*,
          typename internal::CallbackParamTraits<A1>::ForwardType,
          typename internal::CallbackParamTraits<A2>::ForwardType,
          typename internal::CallbackParamTraits<A3>::ForwardType,
          typename internal::CallbackParamTraits<A4>::ForwardType,
          typename internal::CallbackParamTraits<A5>::ForwardType);

};

template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6>
class Callback<R(A1, A2, A3, A4, A5, A6)> : public internal::CallbackBase {
 public:
  typedef R(RunType)(A1, A2, A3, A4, A5, A6);

  Callback() : CallbackBase(NULL) { }

  
  
  template <typename Runnable, typename BindRunType, typename BoundArgsType>
  Callback(internal::BindState<Runnable, BindRunType,
           BoundArgsType>* bind_state)
      : CallbackBase(bind_state) {

    
    
    
    PolymorphicInvoke invoke_func =
        &internal::BindState<Runnable, BindRunType, BoundArgsType>
            ::InvokerType::Run;
    polymorphic_invoke_ = reinterpret_cast<InvokeFuncStorage>(invoke_func);
  }

  bool Equals(const Callback& other) const {
    return CallbackBase::Equals(other);
  }

  R Run(typename internal::CallbackParamTraits<A1>::ForwardType a1,
        typename internal::CallbackParamTraits<A2>::ForwardType a2,
        typename internal::CallbackParamTraits<A3>::ForwardType a3,
        typename internal::CallbackParamTraits<A4>::ForwardType a4,
        typename internal::CallbackParamTraits<A5>::ForwardType a5,
        typename internal::CallbackParamTraits<A6>::ForwardType a6) const {
    PolymorphicInvoke f =
        reinterpret_cast<PolymorphicInvoke>(polymorphic_invoke_);

    return f(bind_state_.get(), internal::CallbackForward(a1),
             internal::CallbackForward(a2),
             internal::CallbackForward(a3),
             internal::CallbackForward(a4),
             internal::CallbackForward(a5),
             internal::CallbackForward(a6));
  }

 private:
  typedef R(*PolymorphicInvoke)(
      internal::BindStateBase*,
          typename internal::CallbackParamTraits<A1>::ForwardType,
          typename internal::CallbackParamTraits<A2>::ForwardType,
          typename internal::CallbackParamTraits<A3>::ForwardType,
          typename internal::CallbackParamTraits<A4>::ForwardType,
          typename internal::CallbackParamTraits<A5>::ForwardType,
          typename internal::CallbackParamTraits<A6>::ForwardType);

};

template <typename R, typename A1, typename A2, typename A3, typename A4,
    typename A5, typename A6, typename A7>
class Callback<R(A1, A2, A3, A4, A5, A6, A7)> : public internal::CallbackBase {
 public:
  typedef R(RunType)(A1, A2, A3, A4, A5, A6, A7);

  Callback() : CallbackBase(NULL) { }

  
  
  template <typename Runnable, typename BindRunType, typename BoundArgsType>
  Callback(internal::BindState<Runnable, BindRunType,
           BoundArgsType>* bind_state)
      : CallbackBase(bind_state) {

    
    
    
    PolymorphicInvoke invoke_func =
        &internal::BindState<Runnable, BindRunType, BoundArgsType>
            ::InvokerType::Run;
    polymorphic_invoke_ = reinterpret_cast<InvokeFuncStorage>(invoke_func);
  }

  bool Equals(const Callback& other) const {
    return CallbackBase::Equals(other);
  }

  R Run(typename internal::CallbackParamTraits<A1>::ForwardType a1,
        typename internal::CallbackParamTraits<A2>::ForwardType a2,
        typename internal::CallbackParamTraits<A3>::ForwardType a3,
        typename internal::CallbackParamTraits<A4>::ForwardType a4,
        typename internal::CallbackParamTraits<A5>::ForwardType a5,
        typename internal::CallbackParamTraits<A6>::ForwardType a6,
        typename internal::CallbackParamTraits<A7>::ForwardType a7) const {
    PolymorphicInvoke f =
        reinterpret_cast<PolymorphicInvoke>(polymorphic_invoke_);

    return f(bind_state_.get(), internal::CallbackForward(a1),
             internal::CallbackForward(a2),
             internal::CallbackForward(a3),
             internal::CallbackForward(a4),
             internal::CallbackForward(a5),
             internal::CallbackForward(a6),
             internal::CallbackForward(a7));
  }

 private:
  typedef R(*PolymorphicInvoke)(
      internal::BindStateBase*,
          typename internal::CallbackParamTraits<A1>::ForwardType,
          typename internal::CallbackParamTraits<A2>::ForwardType,
          typename internal::CallbackParamTraits<A3>::ForwardType,
          typename internal::CallbackParamTraits<A4>::ForwardType,
          typename internal::CallbackParamTraits<A5>::ForwardType,
          typename internal::CallbackParamTraits<A6>::ForwardType,
          typename internal::CallbackParamTraits<A7>::ForwardType);

};




typedef Callback<void(void)> Closure;

}  

#endif  
