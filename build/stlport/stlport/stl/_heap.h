




























#ifndef _STLP_INTERNAL_HEAP_H
#define _STLP_INTERNAL_HEAP_H

_STLP_BEGIN_NAMESPACE



template <class _RandomAccessIterator>
void
push_heap(_RandomAccessIterator __first, _RandomAccessIterator __last);


template <class _RandomAccessIterator, class _Compare>
void
push_heap(_RandomAccessIterator __first, _RandomAccessIterator __last,
          _Compare __comp);

template <class _RandomAccessIterator, class _Distance, class _Tp>
void
__adjust_heap(_RandomAccessIterator __first, _Distance __holeIndex,
              _Distance __len, _Tp __val);

template <class _RandomAccessIterator, class _Tp, class _Distance>
inline void
__pop_heap(_RandomAccessIterator __first, _RandomAccessIterator __last,
           _RandomAccessIterator __result, _Tp __val, _Distance*)
{
  *__result = *__first;
  __adjust_heap(__first, _Distance(0), _Distance(__last - __first), __val);
}

template <class _RandomAccessIterator>
void pop_heap(_RandomAccessIterator __first,
        _RandomAccessIterator __last);

template <class _RandomAccessIterator, class _Distance,
          class _Tp, class _Compare>
void
__adjust_heap(_RandomAccessIterator __first, _Distance __holeIndex,
              _Distance __len, _Tp __val, _Compare __comp);

template <class _RandomAccessIterator, class _Tp, class _Compare,
          class _Distance>
inline void
__pop_heap(_RandomAccessIterator __first, _RandomAccessIterator __last,
           _RandomAccessIterator __result, _Tp __val, _Compare __comp,
           _Distance*)
{
  *__result = *__first;
  __adjust_heap(__first, _Distance(0), _Distance(__last - __first),
                __val, __comp);
}

template <class _RandomAccessIterator, class _Compare>
void
pop_heap(_RandomAccessIterator __first,
         _RandomAccessIterator __last, _Compare __comp);

template <class _RandomAccessIterator>
void
make_heap(_RandomAccessIterator __first, _RandomAccessIterator __last);

template <class _RandomAccessIterator, class _Compare>
void
make_heap(_RandomAccessIterator __first,
          _RandomAccessIterator __last, _Compare __comp);

template <class _RandomAccessIterator>
_STLP_INLINE_LOOP
void sort_heap(_RandomAccessIterator __first, _RandomAccessIterator __last)
{
  while (__last - __first > 1)
    pop_heap(__first, __last--);
}

template <class _RandomAccessIterator, class _Compare>
_STLP_INLINE_LOOP
void
sort_heap(_RandomAccessIterator __first,
          _RandomAccessIterator __last, _Compare __comp)
{
  while (__last - __first > 1)
    pop_heap(__first, __last--, __comp);
}

_STLP_END_NAMESPACE

# if !defined (_STLP_LINK_TIME_INSTANTIATION)
#  include <stl/_heap.c>
# endif

#endif 




