
























#ifndef _STLP_TEMPBUF_C
#define _STLP_TEMPBUF_C

#ifndef _STLP_INTERNAL_TEMPBUF_H
# include <stl/_tempbuf.h>
#endif

_STLP_BEGIN_NAMESPACE

template <class _Tp>
pair<_Tp*, ptrdiff_t> _STLP_CALL
__get_temporary_buffer(ptrdiff_t __len, _Tp*)
{
  if (__len > ptrdiff_t(INT_MAX / sizeof(_Tp)))
    __len = INT_MAX / sizeof(_Tp);

  while (__len > 0) {
    _Tp* __tmp = (_Tp*) malloc((size_t)__len * sizeof(_Tp));
    if (__tmp != 0)
      return pair<_Tp*, ptrdiff_t>(__tmp, __len);
    __len /= 2;
  }

  return pair<_Tp*, ptrdiff_t>((_Tp*)0, 0);
}
_STLP_END_NAMESPACE

#endif 




