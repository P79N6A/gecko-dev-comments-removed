














 








#ifndef _STLP_INTERNAL_IOSTREAM_STRING_H
#define _STLP_INTERNAL_IOSTREAM_STRING_H

#ifndef _STLP_INTERNAL_ALLOC_H
#  include <stl/_alloc.h>
#endif 

#ifndef _STLP_INTERNAL_STRING_H
#  include <stl/_string.h>
#endif 

_STLP_BEGIN_NAMESPACE

_STLP_MOVE_TO_PRIV_NAMESPACE

template <class _CharT>
class __iostring_allocator : public allocator<_CharT> {
public:
  enum { _STR_SIZE = 256 };

private:
  enum { _BUF_SIZE = _STR_SIZE + 1 };
  typedef allocator<_CharT> _Base;
  _CharT _M_static_buf[_BUF_SIZE];

public:
  typedef typename _Base::size_type size_type;
  typedef typename _Base::pointer pointer;
#if defined (_STLP_MEMBER_TEMPLATE_CLASSES)
  template <class _Tp1> struct rebind {
#  if !defined (_STLP_MSVC) || (_STLP_MSVC >= 1300)
    typedef __iostring_allocator<_Tp1> other;
#  else
    typedef _STLP_PRIV __iostring_allocator<_Tp1> other;
#  endif
  };
#endif

  _CharT* allocate(size_type __n, const void* __ptr = 0) {
    if (__n > _BUF_SIZE) {
      return _Base::allocate(__n, __ptr);
    }
    return _M_static_buf;
  }
  void deallocate(pointer __p, size_type __n) {
    if (__p != _M_static_buf) _Base::deallocate(__p, __n);
  }
};

#if defined (_STLP_DONT_SUPPORT_REBIND_MEMBER_TEMPLATE) || !defined (_STLP_MEMBER_TEMPLATES)






_STLP_MOVE_TO_STD_NAMESPACE

template <class _Tp>
inline _STLP_PRIV __iostring_allocator<_Tp>& _STLP_CALL
__stl_alloc_rebind(_STLP_PRIV __iostring_allocator<_Tp>& __a, const _Tp*)
{ return __a; }
template <class _Tp>
inline _STLP_PRIV __iostring_allocator<_Tp> _STLP_CALL
__stl_alloc_create(const _STLP_PRIV __iostring_allocator<_Tp>&, const _Tp*)
{ return _STLP_PRIV __iostring_allocator<_Tp>(); }

_STLP_MOVE_TO_PRIV_NAMESPACE
#endif 

#if !defined (_STLP_DEBUG)
template <class _CharT>
struct __basic_iostring : public basic_string<_CharT, char_traits<_CharT>, __iostring_allocator<_CharT> > {
  





  typedef __basic_iostring<_CharT> _Self;
  typedef basic_string<_CharT, char_traits<_CharT>, __iostring_allocator<_CharT> > _Base;
  typedef typename _Base::_Reserve_t _Reserve_t;

  __basic_iostring() : _Base(_Reserve_t(), __iostring_allocator<_CharT>::_STR_SIZE)
  {}

  _Self& operator=(const _CharT* __s) {
    _Base::operator=(__s);
    return *this;
  }
};

typedef __basic_iostring<char> __iostring;

#  if !defined (_STLP_NO_WCHAR_T)
typedef __basic_iostring<wchar_t> __iowstring;
#  endif

#  define _STLP_BASIC_IOSTRING(_CharT) _STLP_PRIV __basic_iostring<_CharT>

#else

typedef string __iostring;
#  if !defined (_STLP_NO_WCHAR_T)
typedef wstring __iowstring;
#  endif

#  define _STLP_BASIC_IOSTRING(_CharT) basic_string<_CharT, char_traits<_CharT>, allocator<_CharT> >

#endif

_STLP_MOVE_TO_STD_NAMESPACE

_STLP_END_NAMESPACE

#endif
