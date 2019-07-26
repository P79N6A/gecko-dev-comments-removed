



#ifndef BASE_MEMORY_RAW_SCOPED_REFPTR_MISMATCH_CHECKER_H_
#define BASE_MEMORY_RAW_SCOPED_REFPTR_MISMATCH_CHECKER_H_

#include "base/memory/ref_counted.h"
#include "base/template_util.h"
#include "base/tuple.h"
#include "build/build_config.h"








namespace base {



namespace internal {

template <typename T>
struct NeedsScopedRefptrButGetsRawPtr {
#if defined(OS_WIN)
  enum {
    value = base::false_type::value
  };
#else
  enum {
    
    
    
    value = (is_pointer<T>::value &&
             (is_convertible<T, subtle::RefCountedBase*>::value ||
              is_convertible<T, subtle::RefCountedThreadSafeBase*>::value))
  };
#endif
};

template <typename Params>
struct ParamsUseScopedRefptrCorrectly {
  enum { value = 0 };
};

template <>
struct ParamsUseScopedRefptrCorrectly<Tuple0> {
  enum { value = 1 };
};

template <typename A>
struct ParamsUseScopedRefptrCorrectly<Tuple1<A> > {
  enum { value = !NeedsScopedRefptrButGetsRawPtr<A>::value };
};

template <typename A, typename B>
struct ParamsUseScopedRefptrCorrectly<Tuple2<A, B> > {
  enum { value = !(NeedsScopedRefptrButGetsRawPtr<A>::value ||
                   NeedsScopedRefptrButGetsRawPtr<B>::value) };
};

template <typename A, typename B, typename C>
struct ParamsUseScopedRefptrCorrectly<Tuple3<A, B, C> > {
  enum { value = !(NeedsScopedRefptrButGetsRawPtr<A>::value ||
                   NeedsScopedRefptrButGetsRawPtr<B>::value ||
                   NeedsScopedRefptrButGetsRawPtr<C>::value) };
};

template <typename A, typename B, typename C, typename D>
struct ParamsUseScopedRefptrCorrectly<Tuple4<A, B, C, D> > {
  enum { value = !(NeedsScopedRefptrButGetsRawPtr<A>::value ||
                   NeedsScopedRefptrButGetsRawPtr<B>::value ||
                   NeedsScopedRefptrButGetsRawPtr<C>::value ||
                   NeedsScopedRefptrButGetsRawPtr<D>::value) };
};

template <typename A, typename B, typename C, typename D, typename E>
struct ParamsUseScopedRefptrCorrectly<Tuple5<A, B, C, D, E> > {
  enum { value = !(NeedsScopedRefptrButGetsRawPtr<A>::value ||
                   NeedsScopedRefptrButGetsRawPtr<B>::value ||
                   NeedsScopedRefptrButGetsRawPtr<C>::value ||
                   NeedsScopedRefptrButGetsRawPtr<D>::value ||
                   NeedsScopedRefptrButGetsRawPtr<E>::value) };
};

template <typename A, typename B, typename C, typename D, typename E,
          typename F>
struct ParamsUseScopedRefptrCorrectly<Tuple6<A, B, C, D, E, F> > {
  enum { value = !(NeedsScopedRefptrButGetsRawPtr<A>::value ||
                   NeedsScopedRefptrButGetsRawPtr<B>::value ||
                   NeedsScopedRefptrButGetsRawPtr<C>::value ||
                   NeedsScopedRefptrButGetsRawPtr<D>::value ||
                   NeedsScopedRefptrButGetsRawPtr<E>::value ||
                   NeedsScopedRefptrButGetsRawPtr<F>::value) };
};

template <typename A, typename B, typename C, typename D, typename E,
          typename F, typename G>
struct ParamsUseScopedRefptrCorrectly<Tuple7<A, B, C, D, E, F, G> > {
  enum { value = !(NeedsScopedRefptrButGetsRawPtr<A>::value ||
                   NeedsScopedRefptrButGetsRawPtr<B>::value ||
                   NeedsScopedRefptrButGetsRawPtr<C>::value ||
                   NeedsScopedRefptrButGetsRawPtr<D>::value ||
                   NeedsScopedRefptrButGetsRawPtr<E>::value ||
                   NeedsScopedRefptrButGetsRawPtr<F>::value ||
                   NeedsScopedRefptrButGetsRawPtr<G>::value) };
};

template <typename A, typename B, typename C, typename D, typename E,
          typename F, typename G, typename H>
struct ParamsUseScopedRefptrCorrectly<Tuple8<A, B, C, D, E, F, G, H> > {
  enum { value = !(NeedsScopedRefptrButGetsRawPtr<A>::value ||
                   NeedsScopedRefptrButGetsRawPtr<B>::value ||
                   NeedsScopedRefptrButGetsRawPtr<C>::value ||
                   NeedsScopedRefptrButGetsRawPtr<D>::value ||
                   NeedsScopedRefptrButGetsRawPtr<E>::value ||
                   NeedsScopedRefptrButGetsRawPtr<F>::value ||
                   NeedsScopedRefptrButGetsRawPtr<G>::value ||
                   NeedsScopedRefptrButGetsRawPtr<H>::value) };
};

}  

}  

#endif  
