












#ifndef __CONCEPT_CHECKS_H
#define __CONCEPT_CHECKS_H































#ifndef _STLP_USE_CONCEPT_CHECKS



#define _STLP_REQUIRES(__type_var, __concept) do {} while(0)
#define _STLP_CLASS_REQUIRES(__type_var, __concept) \
  static int  __##__type_var##_##__concept
#define _STLP_CONVERTIBLE(__type_x, __type_y) do {} while(0)
#define _STLP_REQUIRES_SAME_TYPE(__type_x, __type_y) do {} while(0)
#define _STLP_CLASS_REQUIRES_SAME_TYPE(__type_x, __type_y) \
  static int  __##__type_x##__type_y##_require_same_type
#define _STLP_GENERATOR_CHECK(__func, __ret) do {} while(0)
#define _STLP_CLASS_GENERATOR_CHECK(__func, __ret) \
  static int  __##__func##__ret##_generator_check
#define _STLP_UNARY_FUNCTION_CHECK(__func, __ret, __arg) do {} while(0)
#define _STLP_CLASS_UNARY_FUNCTION_CHECK(__func, __ret, __arg) \
  static int  __##__func##__ret##__arg##_unary_function_check
#define _STLP_BINARY_FUNCTION_CHECK(__func, __ret, __first, __second) \
  do {} while(0)
#define _STLP_CLASS_BINARY_FUNCTION_CHECK(__func, __ret, __first, __second) \
  static int  __##__func##__ret##__first##__second##_binary_function_check
#define _STLP_REQUIRES_BINARY_OP(__opname, __ret, __first, __second) \
  do {} while(0)
#define _STLP_CLASS_REQUIRES_BINARY_OP(__opname, __ret, __first, __second) \
  static int __##__opname##__ret##__first##__second##_require_binary_op

#else 




















#define _STLP_REQUIRES(__type_var, __concept) \
do { \
  void (*__x)( __type_var ) = __concept##_concept_specification< __type_var >\
    ::##__concept##_requirement_violation; __x = __x; } while (0)


#define _STLP_CONVERTIBLE(__type_x, __type_y) \
do { \
  void (*__x)( __type_x , __type_y ) = _STL_CONVERT_ERROR< __type_x , \
  __type_y >::__type_X_is_not_convertible_to_type_Y; \
  __x = __x; } while (0)


#define _STLP_REQUIRES_SAME_TYPE(__type_x, __type_y) \
do { \
  void (*__x)( __type_x , __type_y ) = _STL_SAME_TYPE_ERROR< __type_x, \
    __type_y  >::__type_X_not_same_as_type_Y; \
  __x = __x; } while (0)



#define _STLP_GENERATOR_CHECK(__func, __ret) \
do { \
  __ret (*__x)( __func&) = \
     _STL_GENERATOR_ERROR< \
  __func, __ret>::__generator_requirement_violation; \
  __x = __x; } while (0)


#define _STLP_UNARY_FUNCTION_CHECK(__func, __ret, __arg) \
do { \
  __ret (*__x)( __func&, const __arg& ) = \
     _STL_UNARY_FUNCTION_ERROR< \
  __func, __ret, __arg>::__unary_function_requirement_violation; \
  __x = __x; } while (0)


#define _STLP_BINARY_FUNCTION_CHECK(__func, __ret, __first, __second) \
do { \
  __ret (*__x)( __func&, const __first&, const __second& ) = \
     _STL_BINARY_FUNCTION_ERROR< \
  __func, __ret, __first, __second>::__binary_function_requirement_violation; \
  __x = __x; } while (0)


#define _STLP_REQUIRES_BINARY_OP(__opname, __ret, __first, __second) \
    do { \
  __ret (*__x)( __first&, __second& ) = _STL_BINARY##__opname##_ERROR< \
    __ret, __first, __second>::__binary_operator_requirement_violation; \
  __ret (*__y)( const __first&, const __second& ) = \
    _STL_BINARY##__opname##_ERROR< __ret, __first, __second>:: \
      __const_binary_operator_requirement_violation; \
  __y = __y; __x = __x; } while (0)


#ifdef _STLP_NO_FUNCTION_PTR_IN_CLASS_TEMPLATE

#define _STLP_CLASS_REQUIRES(__type_var, __concept)
#define _STLP_CLASS_REQUIRES_SAME_TYPE(__type_x, __type_y)
#define _STLP_CLASS_GENERATOR_CHECK(__func, __ret)
#define _STLP_CLASS_UNARY_FUNCTION_CHECK(__func, __ret, __arg)
#define _STLP_CLASS_BINARY_FUNCTION_CHECK(__func, __ret, __first, __second)
#define _STLP_CLASS_REQUIRES_BINARY_OP(__opname, __ret, __first, __second)

#else






#define _STLP_CLASS_REQUIRES(__type_var, __concept) \
  typedef void (* __func##__type_var##__concept)( __type_var ); \
  template <__func##__type_var##__concept _Tp1> \
  struct __dummy_struct_##__type_var##__concept { }; \
  static __dummy_struct_##__type_var##__concept< \
    __concept##_concept_specification< \
      __type_var>::__concept##_requirement_violation>  \
  __dummy_ptr_##__type_var##__concept


#define _STLP_CLASS_REQUIRES_SAME_TYPE(__type_x, __type_y) \
  typedef void (* __func_##__type_x##__type_y##same_type)( __type_x, \
                                                            __type_y ); \
  template < __func_##__type_x##__type_y##same_type _Tp1> \
  struct __dummy_struct_##__type_x##__type_y##_same_type { }; \
  static __dummy_struct_##__type_x##__type_y##_same_type< \
    _STL_SAME_TYPE_ERROR<__type_x, __type_y>::__type_X_not_same_as_type_Y>  \
  __dummy_ptr_##__type_x##__type_y##_same_type


#define _STLP_CLASS_GENERATOR_CHECK(__func, __ret) \
  typedef __ret (* __f_##__func##__ret##_generator)( __func& ); \
  template <__f_##__func##__ret##_generator _Tp1> \
  struct __dummy_struct_##__func##__ret##_generator { }; \
  static __dummy_struct_##__func##__ret##_generator< \
    _STL_GENERATOR_ERROR< \
      __func, __ret>::__generator_requirement_violation>  \
  __dummy_ptr_##__func##__ret##_generator


#define _STLP_CLASS_UNARY_FUNCTION_CHECK(__func, __ret, __arg) \
  typedef __ret (* __f_##__func##__ret##__arg##_unary_check)( __func&, \
                                                         const __arg& ); \
  template <__f_##__func##__ret##__arg##_unary_check _Tp1> \
  struct __dummy_struct_##__func##__ret##__arg##_unary_check { }; \
  static __dummy_struct_##__func##__ret##__arg##_unary_check< \
    _STL_UNARY_FUNCTION_ERROR< \
      __func, __ret, __arg>::__unary_function_requirement_violation>  \
  __dummy_ptr_##__func##__ret##__arg##_unary_check


#define _STLP_CLASS_BINARY_FUNCTION_CHECK(__func, __ret, __first, __second) \
  typedef __ret (* __f_##__func##__ret##__first##__second##_binary_check)( __func&, const __first&,\
                                                    const __second& ); \
  template <__f_##__func##__ret##__first##__second##_binary_check _Tp1> \
  struct __dummy_struct_##__func##__ret##__first##__second##_binary_check { }; \
  static __dummy_struct_##__func##__ret##__first##__second##_binary_check< \
    _STL_BINARY_FUNCTION_ERROR<__func, __ret, __first, __second>:: \
  __binary_function_requirement_violation>  \
  __dummy_ptr_##__func##__ret##__first##__second##_binary_check


#define _STLP_CLASS_REQUIRES_BINARY_OP(__opname, __ret, __first, __second) \
  typedef __ret (* __f_##__func##__ret##__first##__second##_binary_op)(const __first&, \
                                                    const __second& ); \
  template <__f_##__func##__ret##__first##__second##_binary_op _Tp1> \
  struct __dummy_struct_##__func##__ret##__first##__second##_binary_op { }; \
  static __dummy_struct_##__func##__ret##__first##__second##_binary_op< \
    _STL_BINARY##__opname##_ERROR<__ret, __first, __second>:: \
  __binary_operator_requirement_violation>  \
  __dummy_ptr_##__func##__ret##__first##__second##_binary_op

#endif




template <class _Tp>
struct _Mutable_trait {
  typedef _Tp _Type;
};
template <class _Tp>
struct _Mutable_trait<const _Tp> {
  typedef _Tp _Type;
};



template <class _Type>
void __sink_unused_warning(_Type) { }

template <class _TypeX, class _TypeY>
struct _STL_CONVERT_ERROR {
  static void
  __type_X_is_not_convertible_to_type_Y(_TypeX __x, _TypeY) {
    _TypeY __y = __x;
    __sink_unused_warning(__y);
  }
};


template <class _Type> struct __check_equal { };

template <class _TypeX, class _TypeY>
struct _STL_SAME_TYPE_ERROR {
  static void
  __type_X_not_same_as_type_Y(_TypeX , _TypeY ) {
    __check_equal<_TypeX> t1 = __check_equal<_TypeY>();
  }
};




template <class _Func, class _Ret>
struct _STL_GENERATOR_ERROR {
  static _Ret __generator_requirement_violation(_Func& __f) {
    return __f();
  }
};

template <class _Func>
struct _STL_GENERATOR_ERROR<_Func, void> {
  static void __generator_requirement_violation(_Func& __f) {
    __f();
  }
};


template <class _Func, class _Ret, class _Arg>
struct _STL_UNARY_FUNCTION_ERROR {
  static _Ret
  __unary_function_requirement_violation(_Func& __f,
                                          const _Arg& __arg) {
    return __f(__arg);
  }
};

template <class _Func, class _Arg>
struct _STL_UNARY_FUNCTION_ERROR<_Func, void, _Arg> {
  static void
  __unary_function_requirement_violation(_Func& __f,
                                          const _Arg& __arg) {
    __f(__arg);
  }
};

template <class _Func, class _Ret, class _First, class _Second>
struct _STL_BINARY_FUNCTION_ERROR {
  static _Ret
  __binary_function_requirement_violation(_Func& __f,
                                          const _First& __first,
                                          const _Second& __second) {
    return __f(__first, __second);
  }
};

template <class _Func, class _First, class _Second>
struct _STL_BINARY_FUNCTION_ERROR<_Func, void, _First, _Second> {
  static void
  __binary_function_requirement_violation(_Func& __f,
                                          const _First& __first,
                                          const _Second& __second) {
    __f(__first, __second);
  }
};


#define _STLP_DEFINE_BINARY_OP_CHECK(_OP, _NAME) \
template <class _Ret, class _First, class _Second> \
struct _STL_BINARY##_NAME##_ERROR { \
  static _Ret \
  __const_binary_operator_requirement_violation(const _First& __first,  \
                                                const _Second& __second) { \
    return __first _OP __second; \
  } \
  static _Ret \
  __binary_operator_requirement_violation(_First& __first,  \
                                          _Second& __second) { \
    return __first _OP __second; \
  } \
}

_STLP_DEFINE_BINARY_OP_CHECK(==, _OP_EQUAL);
_STLP_DEFINE_BINARY_OP_CHECK(!=, _OP_NOT_EQUAL);
_STLP_DEFINE_BINARY_OP_CHECK(<, _OP_LESS_THAN);
_STLP_DEFINE_BINARY_OP_CHECK(<=, _OP_LESS_EQUAL);
_STLP_DEFINE_BINARY_OP_CHECK(>, _OP_GREATER_THAN);
_STLP_DEFINE_BINARY_OP_CHECK(>=, _OP_GREATER_EQUAL);
_STLP_DEFINE_BINARY_OP_CHECK(+, _OP_PLUS);
_STLP_DEFINE_BINARY_OP_CHECK(*, _OP_TIMES);
_STLP_DEFINE_BINARY_OP_CHECK(/, _OP_DIVIDE);
_STLP_DEFINE_BINARY_OP_CHECK(-, _OP_SUBTRACT);
_STLP_DEFINE_BINARY_OP_CHECK(%, _OP_MOD);











struct _STL_ERROR {

  template <class _Type>
  static _Type
  __default_constructor_requirement_violation(_Type) {
    return _Type();
  }
  template <class _Type>
  static _Type
  __assignment_operator_requirement_violation(_Type __a) {
    __a = __a;
    return __a;
  }
  template <class _Type>
  static _Type
  __copy_constructor_requirement_violation(_Type __a) {
    _Type __c(__a);
    return __c;
  }
  template <class _Type>
  static _Type
  __const_parameter_required_for_copy_constructor(_Type ,
                                                  const _Type& __b) {
    _Type __c(__b);
    return __c;
  }
  template <class _Type>
  static _Type
  __const_parameter_required_for_assignment_operator(_Type __a,
                                                     const _Type& __b) {
    __a = __b;
    return __a;
  }
  template <class _Type>
  static _Type
  __less_than_comparable_requirement_violation(_Type __a, _Type __b) {
    if (__a < __b || __a > __b || __a <= __b || __a >= __b) return __a;
    return __b;
  }
  template <class _Type>
  static _Type
  __equality_comparable_requirement_violation(_Type __a, _Type __b) {
    if (__a == __b || __a != __b) return __a;
    return __b;
  }
  template <class _Iterator>
  static void
  __dereference_operator_requirement_violation(_Iterator __i) {
    __sink_unused_warning(*__i);
  }
  template <class _Iterator>
  static void
  __dereference_operator_and_assignment_requirement_violation(_Iterator __i) {
    *__i = *__i;
  }
  template <class _Iterator>
  static void
  __preincrement_operator_requirement_violation(_Iterator __i) {
    ++__i;
  }
  template <class _Iterator>
  static void
  __postincrement_operator_requirement_violation(_Iterator __i) {
    __i++;
  }
  template <class _Iterator>
  static void
  __predecrement_operator_requirement_violation(_Iterator __i) {
    --__i;
  }
  template <class _Iterator>
  static void
  __postdecrement_operator_requirement_violation(_Iterator __i) {
    __i--;
  }
  template <class _Iterator, class _Type>
  static void
  __postincrement_operator_and_assignment_requirement_violation(_Iterator __i,
                                                                _Type __t) {
    *__i++ = __t;
  }
  template <class _Iterator, class _Distance>
  static _Iterator
  __iterator_addition_assignment_requirement_violation(_Iterator __i,
                                                       _Distance __n) {
    __i += __n;
    return __i;
  }
  template <class _Iterator, class _Distance>
  static _Iterator
  __iterator_addition_requirement_violation(_Iterator __i, _Distance __n) {
    __i = __i + __n;
    __i = __n + __i;
    return __i;
  }
  template <class _Iterator, class _Distance>
  static _Iterator
  __iterator_subtraction_assignment_requirement_violation(_Iterator __i,
                                                          _Distance __n) {
    __i -= __n;
    return __i;
  }
  template <class _Iterator, class _Distance>
  static _Iterator
  __iterator_subtraction_requirement_violation(_Iterator __i, _Distance __n) {
    __i = __i - __n;
    return __i;
  }
  template <class _Iterator, class _Distance>
  static _Distance
  __difference_operator_requirement_violation(_Iterator __i, _Iterator __j,
                                              _Distance __n) {
    __n = __i - __j;
    return __n;
  }
  template <class _Exp, class _Type, class _Distance>
  static _Type
  __element_access_operator_requirement_violation(_Exp __x, _Type*,
                                                  _Distance __n) {
    return __x[__n];
  }
  template <class _Exp, class _Type, class _Distance>
  static void
  __element_assignment_operator_requirement_violation(_Exp __x,
                                                      _Type* __t,
                                                      _Distance __n) {
    __x[__n] = *__t;
  }

}; 



_STLP_BEGIN_NAMESPACE
template <class _Iterator> struct iterator_traits;
_STLP_END_NAMESPACE

template <class _Iter>
struct __value_type_type_definition_requirement_violation {
  typedef typename __STD::iterator_traits<_Iter>::value_type value_type;
};

template <class _Iter>
struct __difference_type_type_definition_requirement_violation {
  typedef typename __STD::iterator_traits<_Iter>::difference_type
          difference_type;
};

template <class _Iter>
struct __reference_type_definition_requirement_violation {
  typedef typename __STD::iterator_traits<_Iter>::reference reference;
};

template <class _Iter>
struct __pointer_type_definition_requirement_violation {
  typedef typename __STD::iterator_traits<_Iter>::pointer pointer;
};

template <class _Iter>
struct __iterator_category_type_definition_requirement_violation {
  typedef typename __STD::iterator_traits<_Iter>::iterator_category
          iterator_category;
};




template <class _Type>
struct _Assignable_concept_specification {
  static void _Assignable_requirement_violation(_Type __a) {
    _STL_ERROR::__assignment_operator_requirement_violation(__a);
    _STL_ERROR::__copy_constructor_requirement_violation(__a);
    _STL_ERROR::__const_parameter_required_for_copy_constructor(__a,__a);
    _STL_ERROR::__const_parameter_required_for_assignment_operator(__a,__a);
  }
};




template <class _Type>
struct _DefaultConstructible_concept_specification {
  static void _DefaultConstructible_requirement_violation(_Type __a) {
    _STL_ERROR::__default_constructor_requirement_violation(__a);
  }
};



template <class _Type>
struct _EqualityComparable_concept_specification {
  static void _EqualityComparable_requirement_violation(_Type __a) {
    _STL_ERROR::__equality_comparable_requirement_violation(__a, __a);
  }
};


template <class _Type>
struct _LessThanComparable_concept_specification {
  static void _LessThanComparable_requirement_violation(_Type __a) {
    _STL_ERROR::__less_than_comparable_requirement_violation(__a, __a);
  }
};



template <class _TrivialIterator>
struct _TrivialIterator_concept_specification {
static void
_TrivialIterator_requirement_violation(_TrivialIterator __i) {
  typedef typename
    __value_type_type_definition_requirement_violation<_TrivialIterator>::
    value_type __T;
  
  _Assignable_concept_specification<_TrivialIterator>::
    _Assignable_requirement_violation(__i);
  
  _DefaultConstructible_concept_specification<_TrivialIterator>::
    _DefaultConstructible_requirement_violation(__i);
  
  _EqualityComparable_concept_specification<_TrivialIterator>::
    _EqualityComparable_requirement_violation(__i);
  
  _STL_ERROR::__dereference_operator_requirement_violation(__i);
}
};

template <class _TrivialIterator>
struct _Mutable_TrivialIterator_concept_specification {
static void
_Mutable_TrivialIterator_requirement_violation(_TrivialIterator __i) {
  _TrivialIterator_concept_specification<_TrivialIterator>::
    _TrivialIterator_requirement_violation(__i);
  
  _STL_ERROR::__dereference_operator_and_assignment_requirement_violation(__i);
}
};



template <class _InputIterator>
struct _InputIterator_concept_specification {
static void
_InputIterator_requirement_violation(_InputIterator __i) {
  
  _TrivialIterator_concept_specification<_InputIterator>::
    _TrivialIterator_requirement_violation(__i);
  
  __difference_type_type_definition_requirement_violation<_InputIterator>();
  __reference_type_definition_requirement_violation<_InputIterator>();
  __pointer_type_definition_requirement_violation<_InputIterator>();
  __iterator_category_type_definition_requirement_violation<_InputIterator>();
  
  _STL_ERROR::__preincrement_operator_requirement_violation(__i);
  _STL_ERROR::__postincrement_operator_requirement_violation(__i);
}
};



template <class _OutputIterator>
struct _OutputIterator_concept_specification {
static void
_OutputIterator_requirement_violation(_OutputIterator __i) {
  
  _Assignable_concept_specification<_OutputIterator>::
    _Assignable_requirement_violation(__i);
  
  __iterator_category_type_definition_requirement_violation<_OutputIterator>();
  
  _STL_ERROR::__dereference_operator_requirement_violation(__i);
  _STL_ERROR::__preincrement_operator_requirement_violation(__i);
  _STL_ERROR::__postincrement_operator_requirement_violation(__i);
  _STL_ERROR::
    __postincrement_operator_and_assignment_requirement_violation(__i, *__i);
}
};



template <class _ForwardIterator>
struct _ForwardIterator_concept_specification {
static void
_ForwardIterator_requirement_violation(_ForwardIterator __i) {
  
  _InputIterator_concept_specification<_ForwardIterator>::
    _InputIterator_requirement_violation(__i);
}
};

template <class _ForwardIterator>
struct _Mutable_ForwardIterator_concept_specification {
static void
_Mutable_ForwardIterator_requirement_violation(_ForwardIterator __i) {
  _ForwardIterator_concept_specification<_ForwardIterator>::
    _ForwardIterator_requirement_violation(__i);
  
  _OutputIterator_concept_specification<_ForwardIterator>::
    _OutputIterator_requirement_violation(__i);
}
};



template <class _BidirectionalIterator>
struct _BidirectionalIterator_concept_specification {
static void
_BidirectionalIterator_requirement_violation(_BidirectionalIterator __i) {
  
  _ForwardIterator_concept_specification<_BidirectionalIterator>::
    _ForwardIterator_requirement_violation(__i);
  
  _STL_ERROR::__predecrement_operator_requirement_violation(__i);
  _STL_ERROR::__postdecrement_operator_requirement_violation(__i);
}
};

template <class _BidirectionalIterator>
struct _Mutable_BidirectionalIterator_concept_specification {
static void
_Mutable_BidirectionalIterator_requirement_violation(
       _BidirectionalIterator __i)
{
  _BidirectionalIterator_concept_specification<_BidirectionalIterator>::
    _BidirectionalIterator_requirement_violation(__i);
  
  _Mutable_ForwardIterator_concept_specification<_BidirectionalIterator>::
    _Mutable_ForwardIterator_requirement_violation(__i);
  typedef typename
    __value_type_type_definition_requirement_violation<
    _BidirectionalIterator>::value_type __T;
  typename _Mutable_trait<__T>::_Type* __tmp_ptr = 0;
  
  _STL_ERROR::
    __postincrement_operator_and_assignment_requirement_violation(__i,
                                                                  *__tmp_ptr);
}
};



template <class _RandAccIter>
struct _RandomAccessIterator_concept_specification {
static void
_RandomAccessIterator_requirement_violation(_RandAccIter __i) {
  
  _BidirectionalIterator_concept_specification<_RandAccIter>::
    _BidirectionalIterator_requirement_violation(__i);
  
  _LessThanComparable_concept_specification<_RandAccIter>::
    _LessThanComparable_requirement_violation(__i);
  typedef typename
        __value_type_type_definition_requirement_violation<_RandAccIter>
        ::value_type
    value_type;
  typedef typename
        __difference_type_type_definition_requirement_violation<_RandAccIter>
        ::difference_type
    _Dist;
  typedef typename _Mutable_trait<_Dist>::_Type _MutDist;

  
  _STL_ERROR::__iterator_addition_assignment_requirement_violation(__i,
                                                                   _MutDist());
  _STL_ERROR::__iterator_addition_requirement_violation(__i,
                                                        _MutDist());
  _STL_ERROR::
    __iterator_subtraction_assignment_requirement_violation(__i,
                                                            _MutDist());
  _STL_ERROR::__iterator_subtraction_requirement_violation(__i,
                                                           _MutDist());
  _STL_ERROR::__difference_operator_requirement_violation(__i, __i,
                                                          _MutDist());
  typename _Mutable_trait<value_type>::_Type* __dummy_ptr = 0;
  _STL_ERROR::__element_access_operator_requirement_violation(__i,
                                                              __dummy_ptr,
                                                              _MutDist());
}
};

template <class _RandAccIter>
struct _Mutable_RandomAccessIterator_concept_specification {
static void
_Mutable_RandomAccessIterator_requirement_violation(_RandAccIter __i)
{
  _RandomAccessIterator_concept_specification<_RandAccIter>::
    _RandomAccessIterator_requirement_violation(__i);
  
  _Mutable_BidirectionalIterator_concept_specification<_RandAccIter>::
    _Mutable_BidirectionalIterator_requirement_violation(__i);
  typedef typename
        __value_type_type_definition_requirement_violation<_RandAccIter>
        ::value_type
    value_type;
  typedef typename
        __difference_type_type_definition_requirement_violation<_RandAccIter>
        ::difference_type
    _Dist;

  typename _Mutable_trait<value_type>::_Type* __tmp_ptr = 0;
  
  _STL_ERROR::__element_assignment_operator_requirement_violation(__i,
                  __tmp_ptr, _Dist());
}
};

#define _STLP_TYPEDEF_REQUIREMENT(__REQUIREMENT) \
template <class Type> \
struct __##__REQUIREMENT##__typedef_requirement_violation { \
  typedef typename Type::__REQUIREMENT __REQUIREMENT; \
};

_STLP_TYPEDEF_REQUIREMENT(value_type);
_STLP_TYPEDEF_REQUIREMENT(difference_type);
_STLP_TYPEDEF_REQUIREMENT(size_type);
_STLP_TYPEDEF_REQUIREMENT(reference);
_STLP_TYPEDEF_REQUIREMENT(const_reference);
_STLP_TYPEDEF_REQUIREMENT(pointer);
_STLP_TYPEDEF_REQUIREMENT(const_pointer);


template <class _Alloc>
struct _Allocator_concept_specification {
static void
_Allocator_requirement_violation(_Alloc __a) {
  
  _DefaultConstructible_concept_specification<_Alloc>::
    _DefaultConstructible_requirement_violation(__a);
  
  _EqualityComparable_concept_specification<_Alloc>::
    _EqualityComparable_requirement_violation(__a);
  
  __value_type__typedef_requirement_violation<_Alloc>();
  __difference_type__typedef_requirement_violation<_Alloc>();
  __size_type__typedef_requirement_violation<_Alloc>();
  __reference__typedef_requirement_violation<_Alloc>();
  __const_reference__typedef_requirement_violation<_Alloc>();
  __pointer__typedef_requirement_violation<_Alloc>();
  __const_pointer__typedef_requirement_violation<_Alloc>();
  typedef typename _Alloc::value_type _Type;
  _STLP_REQUIRES_SAME_TYPE(typename _Alloc::rebind<_Type>::other, _Alloc);
}
};

#endif 

#endif 




