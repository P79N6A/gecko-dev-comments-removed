





















#ifndef _STLP_TYPE_TRAITS_H
#define _STLP_TYPE_TRAITS_H

































#ifdef __WATCOMC__
#  include <stl/_cwchar.h>
#endif

#ifndef _STLP_TYPE_MANIPS_H
#  include <stl/type_manips.h>
#endif

#ifdef _STLP_USE_BOOST_SUPPORT
#  include <stl/boost_type_traits.h>
#  include <boost/type_traits/add_reference.hpp>
#  include <boost/type_traits/add_const.hpp>
#endif 

_STLP_BEGIN_NAMESPACE

#if !defined (_STLP_USE_BOOST_SUPPORT)




template <class _Tp> struct _IsIntegral
{ typedef __false_type _Ret; };

#  ifndef _STLP_NO_BOOL
_STLP_TEMPLATE_NULL struct _IsIntegral<bool>
{ typedef __true_type _Ret; };
#  endif 

_STLP_TEMPLATE_NULL struct _IsIntegral<char>
{ typedef __true_type _Ret; };

#  ifndef _STLP_NO_SIGNED_BUILTINS
_STLP_TEMPLATE_NULL struct _IsIntegral<signed char>
{ typedef __true_type _Ret; };
#  endif

_STLP_TEMPLATE_NULL struct _IsIntegral<unsigned char>
{ typedef __true_type _Ret; };

#  if defined ( _STLP_HAS_WCHAR_T ) && ! defined (_STLP_WCHAR_T_IS_USHORT)
_STLP_TEMPLATE_NULL struct _IsIntegral<wchar_t>
{ typedef __true_type _Ret; };
#  endif 

_STLP_TEMPLATE_NULL struct _IsIntegral<short>
{ typedef __true_type _Ret; };

_STLP_TEMPLATE_NULL struct _IsIntegral<unsigned short>
{ typedef __true_type _Ret; };

_STLP_TEMPLATE_NULL struct _IsIntegral<int>
{ typedef __true_type _Ret; };

_STLP_TEMPLATE_NULL struct _IsIntegral<unsigned int>
{ typedef __true_type _Ret; };

_STLP_TEMPLATE_NULL struct _IsIntegral<long>
{ typedef __true_type _Ret; };

_STLP_TEMPLATE_NULL struct _IsIntegral<unsigned long>
{ typedef __true_type _Ret; };

#  ifdef _STLP_LONG_LONG
_STLP_TEMPLATE_NULL struct _IsIntegral<_STLP_LONG_LONG>
{ typedef __true_type _Ret; };

_STLP_TEMPLATE_NULL struct _IsIntegral<unsigned _STLP_LONG_LONG>
{ typedef __true_type _Ret; };
#  endif 

template <class _Tp> struct _IsRational
{ typedef __false_type _Ret; };

_STLP_TEMPLATE_NULL struct _IsRational<float>
{ typedef __true_type _Ret; };

_STLP_TEMPLATE_NULL struct _IsRational<double>
{ typedef __true_type _Ret; };

#  if !defined ( _STLP_NO_LONG_DOUBLE )
_STLP_TEMPLATE_NULL struct _IsRational<long double>
{ typedef __true_type _Ret; };
#  endif


template <class _Tp> struct __type_traits;
template <class _IsPOD> struct __type_traits_aux {
   typedef __false_type    has_trivial_default_constructor;
   typedef __false_type    has_trivial_copy_constructor;
   typedef __false_type    has_trivial_assignment_operator;
   typedef __false_type    has_trivial_destructor;
   typedef __false_type    is_POD_type;
};

_STLP_TEMPLATE_NULL
struct __type_traits_aux<__false_type> {
   typedef __false_type    has_trivial_default_constructor;
   typedef __false_type    has_trivial_copy_constructor;
   typedef __false_type    has_trivial_assignment_operator;
   typedef __false_type    has_trivial_destructor;
   typedef __false_type    is_POD_type;
};

_STLP_TEMPLATE_NULL
struct __type_traits_aux<__true_type> {
  typedef __true_type    has_trivial_default_constructor;
  typedef __true_type    has_trivial_copy_constructor;
  typedef __true_type    has_trivial_assignment_operator;
  typedef __true_type    has_trivial_destructor;
  typedef __true_type    is_POD_type;
};

template <class _Tp>
struct _IsRef {
  typedef __false_type _Ret;
};

#  if defined (_STLP_SIMULATE_PARTIAL_SPEC_FOR_TYPE_TRAITS)








struct _PointerShim {
  





  _PointerShim(const volatile void*); 
};


char _STLP_CALL _IsP(bool, _PointerShim);  
char* _STLP_CALL _IsP(bool, ...);          

template <class _Tp>
struct _IsPtr {
  





  static _Tp& __null_rep();
  enum { _Ptr = (sizeof(_IsP(false,__null_rep())) == sizeof(char)) };
  typedef typename __bool2type<_Ptr>::_Ret _Ret;

};


template <class _Tp>
struct __type_traits : __type_traits_aux<typename _IsPtr<_Tp>::_Ret> {};

#  else 

template <class _Tp>  struct _IsPtr {
  typedef __false_type _Ret;
};

template <class _Tp>
struct __type_traits {
   typedef __true_type     this_dummy_member_must_be_first;
                   





   









#    if !defined (_STLP_HAS_TYPE_TRAITS_INTRINSICS)
   typedef __false_type    has_trivial_default_constructor;
   typedef __false_type    has_trivial_copy_constructor;
   typedef __false_type    has_trivial_assignment_operator;
   typedef __false_type    has_trivial_destructor;
   typedef __false_type    is_POD_type;
#    else
   typedef typename __bool2type<_STLP_HAS_TRIVIAL_CONSTRUCTOR(_Tp)>::_Ret has_trivial_default_constructor;
   typedef typename __bool2type<_STLP_HAS_TRIVIAL_COPY(_Tp)>::_Ret has_trivial_copy_constructor;
   typedef typename __bool2type<_STLP_HAS_TRIVIAL_ASSIGN(_Tp)>::_Ret has_trivial_assignment_operator;
   typedef typename __bool2type<_STLP_HAS_TRIVIAL_DESTRUCTOR(_Tp)>::_Ret has_trivial_destructor;
   typedef typename __bool2type<_STLP_IS_POD(_Tp)>::_Ret is_POD_type;
#    endif
};

#    if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
template <class _Tp> struct _IsPtr<_Tp*>
{ typedef __true_type _Ret; };
template <class _Tp> struct _IsRef<_Tp&>
{ typedef __true_type _Ret; };

template <class _Tp> struct __type_traits<_Tp*> : __type_traits_aux<__true_type>
{};
#    endif 

#  endif 




#  if !defined (_STLP_QUALIFIED_SPECIALIZATION_BUG)
#    define _STLP_DEFINE_TYPE_TRAITS_FOR(Type) \
_STLP_TEMPLATE_NULL struct __type_traits< Type > : __type_traits_aux<__true_type> {}; \
_STLP_TEMPLATE_NULL struct __type_traits< const Type > : __type_traits_aux<__true_type> {}; \
_STLP_TEMPLATE_NULL struct __type_traits< volatile Type > : __type_traits_aux<__true_type> {}; \
_STLP_TEMPLATE_NULL struct __type_traits< const volatile Type > : __type_traits_aux<__true_type> {}
#  else
#    define _STLP_DEFINE_TYPE_TRAITS_FOR(Type) \
_STLP_TEMPLATE_NULL struct __type_traits< Type > : __type_traits_aux<__true_type> {};
#  endif

#  ifndef _STLP_NO_BOOL
_STLP_DEFINE_TYPE_TRAITS_FOR(bool);
#  endif 
_STLP_DEFINE_TYPE_TRAITS_FOR(char);
#  ifndef _STLP_NO_SIGNED_BUILTINS
_STLP_DEFINE_TYPE_TRAITS_FOR(signed char);
#  endif
_STLP_DEFINE_TYPE_TRAITS_FOR(unsigned char);
#  if defined ( _STLP_HAS_WCHAR_T ) && ! defined (_STLP_WCHAR_T_IS_USHORT)
_STLP_DEFINE_TYPE_TRAITS_FOR(wchar_t);
#  endif 

_STLP_DEFINE_TYPE_TRAITS_FOR(short);
_STLP_DEFINE_TYPE_TRAITS_FOR(unsigned short);
_STLP_DEFINE_TYPE_TRAITS_FOR(int);
_STLP_DEFINE_TYPE_TRAITS_FOR(unsigned int);
_STLP_DEFINE_TYPE_TRAITS_FOR(long);
_STLP_DEFINE_TYPE_TRAITS_FOR(unsigned long);

#  ifdef _STLP_LONG_LONG
_STLP_DEFINE_TYPE_TRAITS_FOR(_STLP_LONG_LONG);
_STLP_DEFINE_TYPE_TRAITS_FOR(unsigned _STLP_LONG_LONG);
#  endif 

_STLP_DEFINE_TYPE_TRAITS_FOR(float);
_STLP_DEFINE_TYPE_TRAITS_FOR(double);

#  if !defined ( _STLP_NO_LONG_DOUBLE )
_STLP_DEFINE_TYPE_TRAITS_FOR(long double);
#  endif

#  if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
template <class _ArePtrs, class _Src, class _Dst>
struct _IsCVConvertibleIf
{ typedef typename _IsCVConvertible<_Src, _Dst>::_Ret _Ret; };

template <class _Src, class _Dst>
struct _IsCVConvertibleIf<__false_type, _Src, _Dst>
{ typedef __false_type _Ret; };
#  else
#    if defined (_STLP_MEMBER_TEMPLATE_CLASSES)
template <class _ArePtrs>
struct _IsCVConvertibleIfAux {
  template <class _Src, class _Dst>
  struct _In
  { typedef typename _IsCVConvertible<_Src, _Dst>::_Ret _Ret; };
};

_STLP_TEMPLATE_NULL
struct _IsCVConvertibleIfAux<__false_type> {
  template <class _Src, class _Dst>
  struct _In
  { typedef __false_type _Ret; };
};

template <class _ArePtrs, class _Src, class _Dst>
struct _IsCVConvertibleIf {
  typedef typename _IsCVConvertibleIfAux<_ArePtrs>::_STLP_TEMPLATE _In<_Src, _Dst>::_Ret _Ret;
};
#    else



template <class _ArePtrs, class _Src, class _Dst>
struct _IsCVConvertibleIf
{ typedef __false_type _Ret; };
#    endif
#  endif

template <class _Src, class _Dst>
struct _TrivialNativeTypeCopy {
  typedef typename _IsPtr<_Src>::_Ret _Ptr1;
  typedef typename _IsPtr<_Dst>::_Ret _Ptr2;
  typedef typename _Land2<_Ptr1, _Ptr2>::_Ret _BothPtrs;
  typedef typename _IsCVConvertibleIf<_BothPtrs, _Src, _Dst>::_Ret _Convertible;
  typedef typename _Land2<_BothPtrs, _Convertible>::_Ret _Trivial1;

  typedef typename __bool2type<(sizeof(_Src) == sizeof(_Dst))>::_Ret _SameSize;

#if !defined (__BORLANDC__) || (__BORLANDC__ < 0x564)
  typedef typename _IsIntegral<_Src>::_Ret _Int1;
#else
  typedef typename _UnQual<_Src>::_Type _UnQuSrc;
  typedef typename _IsIntegral<_UnQuSrc>::_Ret _Int1;
#endif
  typedef typename _IsIntegral<_Dst>::_Ret _Int2;
  typedef typename _Land2<_Int1, _Int2>::_Ret _BothInts;

  typedef typename _IsRational<_Src>::_Ret _Rat1;
  typedef typename _IsRational<_Dst>::_Ret _Rat2;
  typedef typename _Land2<_Rat1, _Rat2>::_Ret _BothRats;

  typedef typename _Lor2<_BothInts, _BothRats>::_Ret _BothNatives;
#if !defined (__BORLANDC__) || (__BORLANDC__ >= 0x564)
  typedef typename _Land2<_BothNatives, _SameSize>::_Ret _Trivial2;
#else
  typedef typename _IsUnQual<_Dst>::_Ret _UnQualDst;
  typedef typename _Land3<_BothNatives, _SameSize, _UnQualDst>::_Ret _Trivial2;
#endif
  typedef typename _Lor2<_Trivial1, _Trivial2>::_Ret _Ret;
};

template <class _Src, class _Dst>
struct _TrivialCopy {
  typedef typename _TrivialNativeTypeCopy<_Src, _Dst>::_Ret _NativeRet;
#  if !defined (__BORLANDC__) || (__BORLANDC__ != 0x560)
  typedef typename __type_traits<_Src>::has_trivial_assignment_operator _Tr1;
#  else
  typedef typename _UnConstPtr<_Src*>::_Type _UnConstSrc;
  typedef typename __type_traits<_UnConstSrc>::has_trivial_assignment_operator _Tr1;
#  endif
  typedef typename _AreCopyable<_Src, _Dst>::_Ret _Tr2;
  typedef typename _Land2<_Tr1, _Tr2>::_Ret _UserRet;
  typedef typename _Lor2<_NativeRet, _UserRet>::_Ret _Ret;
  static _Ret _Answer() { return _Ret(); }
};

template <class _Src, class _Dst>
struct _TrivialUCopy {
  typedef typename _TrivialNativeTypeCopy<_Src, _Dst>::_Ret _NativeRet;
#  if !defined (__BORLANDC__) || (__BORLANDC__ != 0x560)
  typedef typename __type_traits<_Src>::has_trivial_copy_constructor _Tr1;
#  else
  typedef typename _UnConstPtr<_Src*>::_Type _UnConstSrc;
  typedef typename __type_traits<_UnConstSrc>::has_trivial_copy_constructor _Tr1;
#  endif
  typedef typename _AreCopyable<_Src, _Dst>::_Ret _Tr2;
  typedef typename _Land2<_Tr1, _Tr2>::_Ret _UserRet;
  typedef typename _Lor2<_NativeRet, _UserRet>::_Ret _Ret;
  static _Ret _Answer() { return _Ret(); }
};

template <class _Tp>
struct _DefaultZeroValue {
  typedef typename _IsIntegral<_Tp>::_Ret _Tr1;
  typedef typename _IsRational<_Tp>::_Ret _Tr2;
  typedef typename _IsPtr<_Tp>::_Ret _Tr3;
  typedef typename _Lor3<_Tr1, _Tr2, _Tr3>::_Ret _Ret;
};

template <class _Tp>
struct _TrivialInit {
#  if !defined (__BORLANDC__) || (__BORLANDC__ != 0x560)
  typedef typename __type_traits<_Tp>::has_trivial_default_constructor _Tr1;
#  else
  typedef typename _UnConstPtr<_Tp*>::_Type _Tp1;
  typedef typename __type_traits<_Tp1>::has_trivial_copy_constructor _Tr1;
#  endif
  typedef typename _DefaultZeroValue<_Tp>::_Ret _Tr2;
  typedef typename _Not<_Tr2>::_Ret _Tr3;
  typedef typename _Land2<_Tr1, _Tr3>::_Ret _Ret;
  static _Ret _Answer() { return _Ret(); }
};

#endif 

template <class _Tp>
struct _IsPtrType {
  typedef typename _IsPtr<_Tp>::_Ret _Type;
  static _Type _Ret() { return _Type(); }
};

template <class _Tp>
struct _IsRefType {
  typedef typename _IsRef<_Tp>::_Ret _Type;
  static _Type _Ret() { return _Type();}
};

template <class _Tp>
struct __call_traits {
#if defined (_STLP_USE_BOOST_SUPPORT) && !defined (_STLP_NO_EXTENSIONS)
  typedef typename __select< ::boost::is_reference<_Tp>::value,
                             typename ::boost::add_const<_Tp>::type,
                             typename ::boost::add_reference< typename ::boost::add_const<_Tp>::type >::type>::_Ret const_param_type;
  typedef typename __select< ::boost::is_reference<_Tp>::value,
                             typename ::boost::remove_const<_Tp>::type,
                             typename ::boost::add_reference<_Tp>::type>::_Ret param_type;
#else
  typedef const _Tp& const_param_type;
  typedef _Tp& param_type;
#endif
};

#if !defined (_STLP_USE_BOOST_SUPPORT) && !defined (_STLP_NO_EXTENSIONS) && defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
template <class _Tp>
struct __call_traits<_Tp&> {
  typedef _Tp& param_type;
  typedef const _Tp& const_param_type;
};
template <class _Tp>
struct __call_traits<const _Tp&> {
  typedef _Tp& param_type;
  typedef const _Tp& const_param_type;
};
#endif

template <class _Tp1, class _Tp2>
struct _BothPtrType {
  typedef typename _IsPtr<_Tp1>::_Ret _IsPtr1;
  typedef typename _IsPtr<_Tp2>::_Ret _IsPtr2;

  typedef typename _Land2<_IsPtr1, _IsPtr2>::_Ret _Ret;
  static _Ret _Answer() { return _Ret(); }
};

template <class _Tp1, class _Tp2, class _IsRef1, class _IsRef2>
struct _OKToSwap {
  typedef typename _AreSameTypes<_Tp1, _Tp2>::_Ret _Same;
  typedef typename _Land3<_Same, _IsRef1, _IsRef2>::_Ret _Type;
  static _Type _Answer() { return _Type(); }
};

template <class _Tp1, class _Tp2, class _IsRef1, class _IsRef2>
inline _OKToSwap<_Tp1, _Tp2, _IsRef1, _IsRef2>
_IsOKToSwap(_Tp1*, _Tp2*, const _IsRef1&, const _IsRef2&)
{ return _OKToSwap<_Tp1, _Tp2, _IsRef1, _IsRef2>(); }

template <class _Src, class _Dst>
inline _TrivialCopy<_Src, _Dst> _UseTrivialCopy(_Src*, _Dst*)
{ return _TrivialCopy<_Src, _Dst>(); }

template <class _Src, class _Dst>
inline _TrivialUCopy<_Src, _Dst> _UseTrivialUCopy(_Src*, _Dst*)
{ return _TrivialUCopy<_Src, _Dst>(); }

#if defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER) || defined (__BORLANDC__) || \
    defined (__DMC__)
struct _NegativeAnswer {
  typedef __false_type _Ret;
  static _Ret _Answer() { return _Ret(); }
};

template <class _Src, class _Dst>
inline _NegativeAnswer _UseTrivialCopy(_Src*, const _Dst*)
{ return _NegativeAnswer(); }

template <class _Src, class _Dst>
inline _NegativeAnswer _UseTrivialCopy(_Src*, volatile _Dst*)
{ return _NegativeAnswer(); }

template <class _Src, class _Dst>
inline _NegativeAnswer _UseTrivialCopy(_Src*, const volatile _Dst*)
{ return _NegativeAnswer(); }

template <class _Src, class _Dst>
inline _NegativeAnswer _UseTrivialUCopy(_Src*, const _Dst*)
{ return _NegativeAnswer(); }

template <class _Src, class _Dst>
inline _NegativeAnswer _UseTrivialUCopy(_Src*, volatile _Dst*)
{ return _NegativeAnswer(); }

template <class _Src, class _Dst>
inline _NegativeAnswer _UseTrivialUCopy(_Src*, const volatile _Dst*)
{ return _NegativeAnswer(); }
#endif

template <class _Tp>
inline _TrivialInit<_Tp> _UseTrivialInit(_Tp*)
{ return _TrivialInit<_Tp>(); }

template <class _Tp>
struct _IsPOD {
  typedef typename __type_traits<_Tp>::is_POD_type _Type;
  static _Type _Answer() { return _Type(); }
};

template <class _Tp>
inline _IsPOD<_Tp> _Is_POD(_Tp*)
{ return _IsPOD<_Tp>(); }

template <class _Tp>
struct _DefaultZeroValueQuestion {
  typedef typename _DefaultZeroValue<_Tp>::_Ret _Ret;
  static _Ret _Answer() { return _Ret(); }
};

template <class _Tp>
inline _DefaultZeroValueQuestion<_Tp> _HasDefaultZeroValue(_Tp*)
{ return _DefaultZeroValueQuestion<_Tp>(); }







template <class _Tp>
struct __stlport_class
{ typedef _Tp _Type; };

template <class _Tp>
struct _IsSTLportClass {
  typedef typename _IsConvertible<_Tp, __stlport_class<_Tp> >::_Ret _Ret;
#if defined (__BORLANDC__)
  enum { _Is = _IsConvertible<_Tp, __stlport_class<_Tp> >::value };
#endif
};

#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
template <class _Tp>
struct _SwapImplemented {
  typedef typename _IsSTLportClass<_Tp>::_Ret _Ret;
#  if defined (__BORLANDC__)
  enum { _Is = _IsSTLportClass<_Tp>::_Is };
#  endif
};
#endif

template <class _Tp>
class _TpWithState : private _Tp {
  _TpWithState();
  int _state;
};



template <class _Tp>
struct _IsStateless {
  enum { _Is = sizeof(_TpWithState<_Tp>) == sizeof(int) };
  typedef typename __bool2type<_Is>::_Ret _Ret;
};

_STLP_END_NAMESPACE

#ifdef _STLP_CLASS_PARTIAL_SPECIALIZATION
#  if defined (__BORLANDC__) || \
      defined (__SUNPRO_CC) ||  \
     (defined (__MWERKS__) && (__MWERKS__ <= 0x2303)) || \
     (defined (__sgi) && defined (_COMPILER_VERSION)) || \
      defined (__DMC__)
#    define _STLP_IS_POD_ITER(_It, _Tp) __type_traits< typename iterator_traits< _Tp >::value_type >::is_POD_type()
#  else
#    define _STLP_IS_POD_ITER(_It, _Tp) typename __type_traits< typename iterator_traits< _Tp >::value_type >::is_POD_type()
#  endif
#else
#  define _STLP_IS_POD_ITER(_It, _Tp) _Is_POD( _STLP_VALUE_TYPE( _It, _Tp ) )._Answer()
#endif

#endif 




