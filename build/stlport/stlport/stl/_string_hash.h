

















#ifndef _STLP_STRING_HASH_H
#define _STLP_STRING_HASH_H

#ifndef _STLP_HASH_FUN_H
# include <stl/_hash_fun.h>
#endif

#ifndef _STLP_INTERNAL_STRING_H
# include <stl/_string.h>
#endif

_STLP_BEGIN_NAMESPACE

template <class _CharT, class _Traits, class _Alloc>
_STLP_INLINE_LOOP size_t
__stl_string_hash(const basic_string<_CharT,_Traits,_Alloc>& __s) {
  unsigned long __h = 0;
  size_t __len = __s.size();
  const _CharT* __data = __s.data();
  for ( size_t __i = 0; __i < __len; ++__i)
    __h = (__h << 2) + __h + __data[__i];
  return size_t(__h);
}

#if defined (_STLP_CLASS_PARTIAL_SPECIALIZATION) && \
  (!defined(__BORLANDC__) || (__BORLANDC__ >= 0x560))
template <class _CharT, class _Traits, class _Alloc>
struct hash<basic_string<_CharT,_Traits,_Alloc> > {
  size_t operator()(const basic_string<_CharT,_Traits,_Alloc>& __s) const
    { return __stl_string_hash(__s); }
};

#else

_STLP_TEMPLATE_NULL
struct _STLP_CLASS_DECLSPEC hash<string> {
  size_t operator()(const string& __s) const
    { return __stl_string_hash(__s); }
};

#  if defined (_STLP_HAS_WCHAR_T)
_STLP_TEMPLATE_NULL
struct _STLP_CLASS_DECLSPEC hash<wstring> {
  size_t operator()(const wstring& __s) const
    { return __stl_string_hash(__s); }
};
#  endif

#endif 

_STLP_END_NAMESPACE

#endif
