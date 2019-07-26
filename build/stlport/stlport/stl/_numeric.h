

























#ifndef _STLP_INTERNAL_NUMERIC_H
#define _STLP_INTERNAL_NUMERIC_H

#ifndef _STLP_INTERNAL_FUNCTION_BASE_H
# include <stl/_function_base.h>
#endif

#ifndef _STLP_INTERNAL_ITERATOR_BASE_H
#  include <stl/_iterator_base.h>
#endif

_STLP_BEGIN_NAMESPACE

template <class _InputIterator, class _Tp>
_STLP_INLINE_LOOP
_Tp accumulate(_InputIterator __first, _InputIterator __last, _Tp _Init) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  for ( ; __first != __last; ++__first)
    _Init = _Init + *__first;
  return _Init;
}

template <class _InputIterator, class _Tp, class _BinaryOperation>
_STLP_INLINE_LOOP
_Tp accumulate(_InputIterator __first, _InputIterator __last, _Tp _Init,
               _BinaryOperation __binary_op) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  for ( ; __first != __last; ++__first)
    _Init = __binary_op(_Init, *__first);
  return _Init;
}

template <class _InputIterator1, class _InputIterator2, class _Tp>
_STLP_INLINE_LOOP
_Tp inner_product(_InputIterator1 __first1, _InputIterator1 __last1,
                  _InputIterator2 __first2, _Tp _Init) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first1, __last1))
  for ( ; __first1 != __last1; ++__first1, ++__first2)
    _Init = _Init + (*__first1 * *__first2);
  return _Init;
}

template <class _InputIterator1, class _InputIterator2, class _Tp,
          class _BinaryOperation1, class _BinaryOperation2>
_STLP_INLINE_LOOP
_Tp inner_product(_InputIterator1 __first1, _InputIterator1 __last1,
                  _InputIterator2 __first2, _Tp _Init,
                  _BinaryOperation1 __binary_op1,
                  _BinaryOperation2 __binary_op2) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first1, __last1))
  for ( ; __first1 != __last1; ++__first1, ++__first2)
    _Init = __binary_op1(_Init, __binary_op2(*__first1, *__first2));
  return _Init;
}

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _InputIterator, class _OutputIterator, class _Tp,
          class _BinaryOperation>
_OutputIterator
__partial_sum(_InputIterator __first, _InputIterator __last,
              _OutputIterator __result, _Tp*, _BinaryOperation __binary_op);

_STLP_MOVE_TO_STD_NAMESPACE

template <class _InputIterator, class _OutputIterator>
inline _OutputIterator
partial_sum(_InputIterator __first, _InputIterator __last,
            _OutputIterator __result) {
  return _STLP_PRIV __partial_sum(__first, __last, __result, _STLP_VALUE_TYPE(__first, _InputIterator),
                                  _STLP_PRIV __plus(_STLP_VALUE_TYPE(__first, _InputIterator)));
}

template <class _InputIterator, class _OutputIterator, class _BinaryOperation>
inline _OutputIterator
partial_sum(_InputIterator __first, _InputIterator __last,
            _OutputIterator __result, _BinaryOperation __binary_op) {
  return _STLP_PRIV __partial_sum(__first, __last, __result, _STLP_VALUE_TYPE(__first, _InputIterator),
                                  __binary_op);
}

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _InputIterator, class _OutputIterator, class _Tp,
          class _BinaryOperation>
_OutputIterator
__adjacent_difference(_InputIterator __first, _InputIterator __last,
                      _OutputIterator __result, _Tp*,
                      _BinaryOperation __binary_op);

_STLP_MOVE_TO_STD_NAMESPACE

template <class _InputIterator, class _OutputIterator>
inline _OutputIterator
adjacent_difference(_InputIterator __first,
                    _InputIterator __last, _OutputIterator __result) {
  return _STLP_PRIV __adjacent_difference(__first, __last, __result,
                                          _STLP_VALUE_TYPE(__first, _InputIterator),
                                          _STLP_PRIV __minus(_STLP_VALUE_TYPE(__first, _InputIterator)));
}

template <class _InputIterator, class _OutputIterator, class _BinaryOperation>
_OutputIterator
adjacent_difference(_InputIterator __first, _InputIterator __last,
                    _OutputIterator __result, _BinaryOperation __binary_op) {
  return _STLP_PRIV __adjacent_difference(__first, __last, __result,
                                          _STLP_VALUE_TYPE(__first, _InputIterator),
                                          __binary_op);
}

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Tp, class _Integer, class _MonoidOperation>
_Tp __power(_Tp __x, _Integer __n, _MonoidOperation __opr);

_STLP_MOVE_TO_STD_NAMESPACE

#if !defined (_STLP_NO_EXTENSIONS)




_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Tp, class _Integer>
inline _Tp __power(_Tp __x, _Integer __n) {
  return __power(__x, __n, multiplies<_Tp>());
}

_STLP_MOVE_TO_STD_NAMESPACE



template <class _Tp, class _Integer, class _MonoidOperation>
inline _Tp power(_Tp __x, _Integer __n, _MonoidOperation __opr) {
  return _STLP_PRIV __power(__x, __n, __opr);
}

template <class _Tp, class _Integer>
inline _Tp power(_Tp __x, _Integer __n) {
  return _STLP_PRIV __power(__x, __n, multiplies<_Tp>());
}



template <class _ForwardIterator, class _Tp>
_STLP_INLINE_LOOP
void iota(_ForwardIterator __first, _ForwardIterator __last, _Tp __val) {
  _STLP_DEBUG_CHECK(_STLP_PRIV __check_range(__first, __last))
  while (__first != __last)
    *__first++ = __val++;
}
#endif

_STLP_END_NAMESPACE

#if !defined (_STLP_LINK_TIME_INSTANTIATION)
#  include <stl/_numeric.c>
#endif

#endif 




