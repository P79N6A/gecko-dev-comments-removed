




























#ifndef _STLP_INTERNAL_FUNCTION_BASE_H
#define _STLP_INTERNAL_FUNCTION_BASE_H

#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION) && !defined (_STLP_TYPE_TRAITS_H)
#  include <stl/type_traits.h>
#endif

_STLP_BEGIN_NAMESPACE

template <class _Arg, class _Result>
struct unary_function {
  typedef _Arg argument_type;
  typedef _Result result_type;
#if !defined (__BORLANDC__) || (__BORLANDC__ < 0x580)
protected:
  


  ~unary_function() {}
#endif
};

template <class _Arg1, class _Arg2, class _Result>
struct binary_function {
  typedef _Arg1 first_argument_type;
  typedef _Arg2 second_argument_type;
  typedef _Result result_type;
#if !defined (__BORLANDC__) || (__BORLANDC__ < 0x580)
protected:
  
  ~binary_function() {}
#endif
};

template <class _Tp>
struct equal_to : public binary_function<_Tp, _Tp, bool> {
  bool operator()(const _Tp& __x, const _Tp& __y) const { return __x == __y; }
};

template <class _Tp>
struct less : public binary_function<_Tp,_Tp,bool>
#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)




              , public __stlport_class<less<_Tp> >
#endif
{
  bool operator()(const _Tp& __x, const _Tp& __y) const { return __x < __y; }

#if defined (_STLP_USE_PARTIAL_SPEC_WORKAROUND) && !defined (_STLP_FUNCTION_TMPL_PARTIAL_ORDER)
  void _M_swap_workaround(less<_Tp>& __x) {}
#endif
};

#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION)
template <class _Tp>
struct __type_traits<less<_Tp> > {
#if !defined (__BORLANDC__)
  typedef typename _IsSTLportClass<less<_Tp> >::_Ret _STLportLess;
#else
  enum { _Is = _IsSTLportClass<less<_Tp> >::_Is };
  typedef typename __bool2type<_Is>::_Ret _STLportLess;
#endif
  typedef _STLportLess has_trivial_default_constructor;
  typedef _STLportLess has_trivial_copy_constructor;
  typedef _STLportLess has_trivial_assignment_operator;
  typedef _STLportLess has_trivial_destructor;
  typedef _STLportLess is_POD_type;
};
#endif

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Tp>
less<_Tp> __less(_Tp* ) { return less<_Tp>(); }

template <class _Tp>
equal_to<_Tp> __equal_to(_Tp* ) { return equal_to<_Tp>(); }

_STLP_MOVE_TO_STD_NAMESPACE

template <class _Tp>
struct plus : public binary_function<_Tp, _Tp, _Tp> {
  _Tp operator()(const _Tp& __x, const _Tp& __y) const { return __x + __y; }
};

template <class _Tp>
struct minus : public binary_function<_Tp, _Tp, _Tp> {
  _Tp operator()(const _Tp& __x, const _Tp& __y) const { return __x - __y; }
};

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Tp>
plus<_Tp> __plus(_Tp* ) { return plus<_Tp>(); }

template <class _Tp>
minus<_Tp> __minus(_Tp* ) { return minus<_Tp>(); }

_STLP_MOVE_TO_STD_NAMESPACE

template <class _Tp>
struct multiplies : public binary_function<_Tp, _Tp, _Tp> {
  _Tp operator()(const _Tp& __x, const _Tp& __y) const { return __x * __y; }
};

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Pair>
struct _Select1st : public unary_function<_Pair, typename _Pair::first_type> {
  const typename _Pair::first_type& operator()(const _Pair& __x) const {
    return __x.first;
  }
};

template <class _Pair>
struct _Select2nd : public unary_function<_Pair, typename _Pair::second_type> {
  const typename _Pair::second_type& operator()(const _Pair& __x) const {
    return __x.second;
  }
};


template <class _Arg1, class _Arg2>
struct _Project1st : public binary_function<_Arg1, _Arg2, _Arg1> {
  _Arg1 operator()(const _Arg1& __x, const _Arg2&) const { return __x; }
};

template <class _Arg1, class _Arg2>
struct _Project2nd : public binary_function<_Arg1, _Arg2, _Arg2> {
  _Arg2 operator()(const _Arg1&, const _Arg2& __y) const { return __y; }
};

#if defined (_STLP_MULTI_CONST_TEMPLATE_ARG_BUG)

template <class _Pair, class _Whatever>

struct __Select1st_hint : public unary_function<_Pair, _Whatever> {
    const _Whatever& operator () (const _Pair& __x) const { return __x.first; }
};
#  define  _STLP_SELECT1ST(__x,__y) _STLP_PRIV __Select1st_hint< __x, __y >
#else
#  define  _STLP_SELECT1ST(__x, __y) _STLP_PRIV _Select1st< __x >
#endif

template <class _Tp>
struct _Identity : public unary_function<_Tp,_Tp> {
  const _Tp& operator()(const _Tp& __x) const { return __x; }
};

template <class _Result, class _Argument>
struct _Constant_unary_fun {
  typedef _Argument argument_type;
  typedef  _Result  result_type;
  result_type _M_val;

  _Constant_unary_fun(const result_type& __v) : _M_val(__v) {}
  const result_type& operator()(const _Argument&) const { return _M_val; }
};

template <class _Result, class _Arg1, class _Arg2>
struct _Constant_binary_fun {
  typedef  _Arg1   first_argument_type;
  typedef  _Arg2   second_argument_type;
  typedef  _Result result_type;
  _Result _M_val;

  _Constant_binary_fun(const _Result& __v) : _M_val(__v) {}
  const result_type& operator()(const _Arg1&, const _Arg2&) const {
    return _M_val;
  }
};


template <class _Tp> inline _Tp __identity_element(plus<_Tp>) {  return _Tp(0); }
template <class _Tp> inline _Tp __identity_element(multiplies<_Tp>) { return _Tp(1); }

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_END_NAMESPACE

#endif 




