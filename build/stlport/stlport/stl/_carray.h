


















#ifndef _STLP_CARRAY_H
#define _STLP_CARRAY_H





#ifndef _STLP_INTERNAL_CONSTRUCT_H
#  include <stl/_construct.h>
#endif

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _Tp, size_t _Nb>
struct _CArray {
  _CArray (const _Tp& __val) {
    for (size_t __i = 0; __i < _Nb; ++__i) {
      _Copy_Construct(__REINTERPRET_CAST(_Tp*, _M_data + __i * sizeof(_Tp)), __val);
    }
  }

  ~_CArray() {
    _Destroy_Range(__REINTERPRET_CAST(_Tp*, _M_data + 0),
                   __REINTERPRET_CAST(_Tp*, _M_data + _Nb * sizeof(_Tp)));
  }

  _Tp& operator [] (size_t __i) {
    _STLP_ASSERT(__i < _Nb)
    return *__REINTERPRET_CAST(_Tp*, _M_data + __i * sizeof(_Tp));
  }

private:
  char _M_data[sizeof(_Tp) * _Nb];
};

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_END_NAMESPACE

#endif 
