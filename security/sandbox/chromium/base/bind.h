








#ifndef BASE_BIND_H_
#define BASE_BIND_H_

#include "base/bind_internal.h"
#include "base/callback_internal.h"







































namespace base {

template <typename Functor>
base::Callback<
    typename internal::BindState<
        typename internal::FunctorTraits<Functor>::RunnableType,
        typename internal::FunctorTraits<Functor>::RunType,
        void()>
            ::UnboundRunType>
Bind(Functor functor) {
  
  typedef typename internal::FunctorTraits<Functor>::RunnableType RunnableType;
  typedef typename internal::FunctorTraits<Functor>::RunType RunType;

  typedef internal::BindState<RunnableType, RunType, void()> BindState;


  return Callback<typename BindState::UnboundRunType>(
      new BindState(internal::MakeRunnable(functor)));
}

template <typename Functor, typename P1>
base::Callback<
    typename internal::BindState<
        typename internal::FunctorTraits<Functor>::RunnableType,
        typename internal::FunctorTraits<Functor>::RunType,
        void(typename internal::CallbackParamTraits<P1>::StorageType)>
            ::UnboundRunType>
Bind(Functor functor, const P1& p1) {
  
  typedef typename internal::FunctorTraits<Functor>::RunnableType RunnableType;
  typedef typename internal::FunctorTraits<Functor>::RunType RunType;

  
  
  
  typedef internal::FunctionTraits<typename RunnableType::RunType>
      BoundFunctorTraits;

  
  
  
  
  
  COMPILE_ASSERT(
      !(is_non_const_reference<typename BoundFunctorTraits::A1Type>::value ),
      do_not_bind_functions_with_nonconst_ref);

  
  
  
  
  COMPILE_ASSERT(
      internal::HasIsMethodTag<RunnableType>::value ||
          !internal::NeedsScopedRefptrButGetsRawPtr<P1>::value,
      p1_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::HasIsMethodTag<RunnableType>::value ||
                     !is_array<P1>::value,
                 first_bound_argument_to_method_cannot_be_array);
  typedef internal::BindState<RunnableType, RunType,
      void(typename internal::CallbackParamTraits<P1>::StorageType)> BindState;


  return Callback<typename BindState::UnboundRunType>(
      new BindState(internal::MakeRunnable(functor), p1));
}

template <typename Functor, typename P1, typename P2>
base::Callback<
    typename internal::BindState<
        typename internal::FunctorTraits<Functor>::RunnableType,
        typename internal::FunctorTraits<Functor>::RunType,
        void(typename internal::CallbackParamTraits<P1>::StorageType,
            typename internal::CallbackParamTraits<P2>::StorageType)>
            ::UnboundRunType>
Bind(Functor functor, const P1& p1, const P2& p2) {
  
  typedef typename internal::FunctorTraits<Functor>::RunnableType RunnableType;
  typedef typename internal::FunctorTraits<Functor>::RunType RunType;

  
  
  
  typedef internal::FunctionTraits<typename RunnableType::RunType>
      BoundFunctorTraits;

  
  
  
  
  
  COMPILE_ASSERT(
      !(is_non_const_reference<typename BoundFunctorTraits::A1Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A2Type>::value ),
      do_not_bind_functions_with_nonconst_ref);

  
  
  
  
  COMPILE_ASSERT(
      internal::HasIsMethodTag<RunnableType>::value ||
          !internal::NeedsScopedRefptrButGetsRawPtr<P1>::value,
      p1_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::HasIsMethodTag<RunnableType>::value ||
                     !is_array<P1>::value,
                 first_bound_argument_to_method_cannot_be_array);
  COMPILE_ASSERT(!internal::NeedsScopedRefptrButGetsRawPtr<P2>::value,
                 p2_is_refcounted_type_and_needs_scoped_refptr);
  typedef internal::BindState<RunnableType, RunType,
      void(typename internal::CallbackParamTraits<P1>::StorageType,
      typename internal::CallbackParamTraits<P2>::StorageType)> BindState;


  return Callback<typename BindState::UnboundRunType>(
      new BindState(internal::MakeRunnable(functor), p1, p2));
}

template <typename Functor, typename P1, typename P2, typename P3>
base::Callback<
    typename internal::BindState<
        typename internal::FunctorTraits<Functor>::RunnableType,
        typename internal::FunctorTraits<Functor>::RunType,
        void(typename internal::CallbackParamTraits<P1>::StorageType,
            typename internal::CallbackParamTraits<P2>::StorageType,
            typename internal::CallbackParamTraits<P3>::StorageType)>
            ::UnboundRunType>
Bind(Functor functor, const P1& p1, const P2& p2, const P3& p3) {
  
  typedef typename internal::FunctorTraits<Functor>::RunnableType RunnableType;
  typedef typename internal::FunctorTraits<Functor>::RunType RunType;

  
  
  
  typedef internal::FunctionTraits<typename RunnableType::RunType>
      BoundFunctorTraits;

  
  
  
  
  
  COMPILE_ASSERT(
      !(is_non_const_reference<typename BoundFunctorTraits::A1Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A2Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A3Type>::value ),
      do_not_bind_functions_with_nonconst_ref);

  
  
  
  
  COMPILE_ASSERT(
      internal::HasIsMethodTag<RunnableType>::value ||
          !internal::NeedsScopedRefptrButGetsRawPtr<P1>::value,
      p1_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::HasIsMethodTag<RunnableType>::value ||
                     !is_array<P1>::value,
                 first_bound_argument_to_method_cannot_be_array);
  COMPILE_ASSERT(!internal::NeedsScopedRefptrButGetsRawPtr<P2>::value,
                 p2_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::NeedsScopedRefptrButGetsRawPtr<P3>::value,
                 p3_is_refcounted_type_and_needs_scoped_refptr);
  typedef internal::BindState<RunnableType, RunType,
      void(typename internal::CallbackParamTraits<P1>::StorageType,
      typename internal::CallbackParamTraits<P2>::StorageType,
      typename internal::CallbackParamTraits<P3>::StorageType)> BindState;


  return Callback<typename BindState::UnboundRunType>(
      new BindState(internal::MakeRunnable(functor), p1, p2, p3));
}

template <typename Functor, typename P1, typename P2, typename P3, typename P4>
base::Callback<
    typename internal::BindState<
        typename internal::FunctorTraits<Functor>::RunnableType,
        typename internal::FunctorTraits<Functor>::RunType,
        void(typename internal::CallbackParamTraits<P1>::StorageType,
            typename internal::CallbackParamTraits<P2>::StorageType,
            typename internal::CallbackParamTraits<P3>::StorageType,
            typename internal::CallbackParamTraits<P4>::StorageType)>
            ::UnboundRunType>
Bind(Functor functor, const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
  
  typedef typename internal::FunctorTraits<Functor>::RunnableType RunnableType;
  typedef typename internal::FunctorTraits<Functor>::RunType RunType;

  
  
  
  typedef internal::FunctionTraits<typename RunnableType::RunType>
      BoundFunctorTraits;

  
  
  
  
  
  COMPILE_ASSERT(
      !(is_non_const_reference<typename BoundFunctorTraits::A1Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A2Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A3Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A4Type>::value ),
      do_not_bind_functions_with_nonconst_ref);

  
  
  
  
  COMPILE_ASSERT(
      internal::HasIsMethodTag<RunnableType>::value ||
          !internal::NeedsScopedRefptrButGetsRawPtr<P1>::value,
      p1_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::HasIsMethodTag<RunnableType>::value ||
                     !is_array<P1>::value,
                 first_bound_argument_to_method_cannot_be_array);
  COMPILE_ASSERT(!internal::NeedsScopedRefptrButGetsRawPtr<P2>::value,
                 p2_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::NeedsScopedRefptrButGetsRawPtr<P3>::value,
                 p3_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::NeedsScopedRefptrButGetsRawPtr<P4>::value,
                 p4_is_refcounted_type_and_needs_scoped_refptr);
  typedef internal::BindState<RunnableType, RunType,
      void(typename internal::CallbackParamTraits<P1>::StorageType,
      typename internal::CallbackParamTraits<P2>::StorageType,
      typename internal::CallbackParamTraits<P3>::StorageType,
      typename internal::CallbackParamTraits<P4>::StorageType)> BindState;


  return Callback<typename BindState::UnboundRunType>(
      new BindState(internal::MakeRunnable(functor), p1, p2, p3, p4));
}

template <typename Functor, typename P1, typename P2, typename P3, typename P4,
    typename P5>
base::Callback<
    typename internal::BindState<
        typename internal::FunctorTraits<Functor>::RunnableType,
        typename internal::FunctorTraits<Functor>::RunType,
        void(typename internal::CallbackParamTraits<P1>::StorageType,
            typename internal::CallbackParamTraits<P2>::StorageType,
            typename internal::CallbackParamTraits<P3>::StorageType,
            typename internal::CallbackParamTraits<P4>::StorageType,
            typename internal::CallbackParamTraits<P5>::StorageType)>
            ::UnboundRunType>
Bind(Functor functor, const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5) {
  
  typedef typename internal::FunctorTraits<Functor>::RunnableType RunnableType;
  typedef typename internal::FunctorTraits<Functor>::RunType RunType;

  
  
  
  typedef internal::FunctionTraits<typename RunnableType::RunType>
      BoundFunctorTraits;

  
  
  
  
  
  COMPILE_ASSERT(
      !(is_non_const_reference<typename BoundFunctorTraits::A1Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A2Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A3Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A4Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A5Type>::value ),
      do_not_bind_functions_with_nonconst_ref);

  
  
  
  
  COMPILE_ASSERT(
      internal::HasIsMethodTag<RunnableType>::value ||
          !internal::NeedsScopedRefptrButGetsRawPtr<P1>::value,
      p1_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::HasIsMethodTag<RunnableType>::value ||
                     !is_array<P1>::value,
                 first_bound_argument_to_method_cannot_be_array);
  COMPILE_ASSERT(!internal::NeedsScopedRefptrButGetsRawPtr<P2>::value,
                 p2_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::NeedsScopedRefptrButGetsRawPtr<P3>::value,
                 p3_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::NeedsScopedRefptrButGetsRawPtr<P4>::value,
                 p4_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::NeedsScopedRefptrButGetsRawPtr<P5>::value,
                 p5_is_refcounted_type_and_needs_scoped_refptr);
  typedef internal::BindState<RunnableType, RunType,
      void(typename internal::CallbackParamTraits<P1>::StorageType,
      typename internal::CallbackParamTraits<P2>::StorageType,
      typename internal::CallbackParamTraits<P3>::StorageType,
      typename internal::CallbackParamTraits<P4>::StorageType,
      typename internal::CallbackParamTraits<P5>::StorageType)> BindState;


  return Callback<typename BindState::UnboundRunType>(
      new BindState(internal::MakeRunnable(functor), p1, p2, p3, p4, p5));
}

template <typename Functor, typename P1, typename P2, typename P3, typename P4,
    typename P5, typename P6>
base::Callback<
    typename internal::BindState<
        typename internal::FunctorTraits<Functor>::RunnableType,
        typename internal::FunctorTraits<Functor>::RunType,
        void(typename internal::CallbackParamTraits<P1>::StorageType,
            typename internal::CallbackParamTraits<P2>::StorageType,
            typename internal::CallbackParamTraits<P3>::StorageType,
            typename internal::CallbackParamTraits<P4>::StorageType,
            typename internal::CallbackParamTraits<P5>::StorageType,
            typename internal::CallbackParamTraits<P6>::StorageType)>
            ::UnboundRunType>
Bind(Functor functor, const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5, const P6& p6) {
  
  typedef typename internal::FunctorTraits<Functor>::RunnableType RunnableType;
  typedef typename internal::FunctorTraits<Functor>::RunType RunType;

  
  
  
  typedef internal::FunctionTraits<typename RunnableType::RunType>
      BoundFunctorTraits;

  
  
  
  
  
  COMPILE_ASSERT(
      !(is_non_const_reference<typename BoundFunctorTraits::A1Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A2Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A3Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A4Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A5Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A6Type>::value ),
      do_not_bind_functions_with_nonconst_ref);

  
  
  
  
  COMPILE_ASSERT(
      internal::HasIsMethodTag<RunnableType>::value ||
          !internal::NeedsScopedRefptrButGetsRawPtr<P1>::value,
      p1_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::HasIsMethodTag<RunnableType>::value ||
                     !is_array<P1>::value,
                 first_bound_argument_to_method_cannot_be_array);
  COMPILE_ASSERT(!internal::NeedsScopedRefptrButGetsRawPtr<P2>::value,
                 p2_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::NeedsScopedRefptrButGetsRawPtr<P3>::value,
                 p3_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::NeedsScopedRefptrButGetsRawPtr<P4>::value,
                 p4_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::NeedsScopedRefptrButGetsRawPtr<P5>::value,
                 p5_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::NeedsScopedRefptrButGetsRawPtr<P6>::value,
                 p6_is_refcounted_type_and_needs_scoped_refptr);
  typedef internal::BindState<RunnableType, RunType,
      void(typename internal::CallbackParamTraits<P1>::StorageType,
      typename internal::CallbackParamTraits<P2>::StorageType,
      typename internal::CallbackParamTraits<P3>::StorageType,
      typename internal::CallbackParamTraits<P4>::StorageType,
      typename internal::CallbackParamTraits<P5>::StorageType,
      typename internal::CallbackParamTraits<P6>::StorageType)> BindState;


  return Callback<typename BindState::UnboundRunType>(
      new BindState(internal::MakeRunnable(functor), p1, p2, p3, p4, p5, p6));
}

template <typename Functor, typename P1, typename P2, typename P3, typename P4,
    typename P5, typename P6, typename P7>
base::Callback<
    typename internal::BindState<
        typename internal::FunctorTraits<Functor>::RunnableType,
        typename internal::FunctorTraits<Functor>::RunType,
        void(typename internal::CallbackParamTraits<P1>::StorageType,
            typename internal::CallbackParamTraits<P2>::StorageType,
            typename internal::CallbackParamTraits<P3>::StorageType,
            typename internal::CallbackParamTraits<P4>::StorageType,
            typename internal::CallbackParamTraits<P5>::StorageType,
            typename internal::CallbackParamTraits<P6>::StorageType,
            typename internal::CallbackParamTraits<P7>::StorageType)>
            ::UnboundRunType>
Bind(Functor functor, const P1& p1, const P2& p2, const P3& p3, const P4& p4,
    const P5& p5, const P6& p6, const P7& p7) {
  
  typedef typename internal::FunctorTraits<Functor>::RunnableType RunnableType;
  typedef typename internal::FunctorTraits<Functor>::RunType RunType;

  
  
  
  typedef internal::FunctionTraits<typename RunnableType::RunType>
      BoundFunctorTraits;

  
  
  
  
  
  COMPILE_ASSERT(
      !(is_non_const_reference<typename BoundFunctorTraits::A1Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A2Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A3Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A4Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A5Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A6Type>::value ||
          is_non_const_reference<typename BoundFunctorTraits::A7Type>::value ),
      do_not_bind_functions_with_nonconst_ref);

  
  
  
  
  COMPILE_ASSERT(
      internal::HasIsMethodTag<RunnableType>::value ||
          !internal::NeedsScopedRefptrButGetsRawPtr<P1>::value,
      p1_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::HasIsMethodTag<RunnableType>::value ||
                     !is_array<P1>::value,
                 first_bound_argument_to_method_cannot_be_array);
  COMPILE_ASSERT(!internal::NeedsScopedRefptrButGetsRawPtr<P2>::value,
                 p2_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::NeedsScopedRefptrButGetsRawPtr<P3>::value,
                 p3_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::NeedsScopedRefptrButGetsRawPtr<P4>::value,
                 p4_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::NeedsScopedRefptrButGetsRawPtr<P5>::value,
                 p5_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::NeedsScopedRefptrButGetsRawPtr<P6>::value,
                 p6_is_refcounted_type_and_needs_scoped_refptr);
  COMPILE_ASSERT(!internal::NeedsScopedRefptrButGetsRawPtr<P7>::value,
                 p7_is_refcounted_type_and_needs_scoped_refptr);
  typedef internal::BindState<RunnableType, RunType,
      void(typename internal::CallbackParamTraits<P1>::StorageType,
      typename internal::CallbackParamTraits<P2>::StorageType,
      typename internal::CallbackParamTraits<P3>::StorageType,
      typename internal::CallbackParamTraits<P4>::StorageType,
      typename internal::CallbackParamTraits<P5>::StorageType,
      typename internal::CallbackParamTraits<P6>::StorageType,
      typename internal::CallbackParamTraits<P7>::StorageType)> BindState;


  return Callback<typename BindState::UnboundRunType>(
      new BindState(internal::MakeRunnable(functor), p1, p2, p3, p4, p5, p6,
          p7));
}

}  

#endif  
