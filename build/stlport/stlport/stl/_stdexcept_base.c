

















__Named_exception::__Named_exception(const string& __str) {
  size_t __size = strlen(_STLP_PRIV __get_c_string(__str)) + 1;
  if (__size > _S_bufsize) {
    _M_name = __STATIC_CAST(char*, malloc(__size * sizeof(char)));
    if (!_M_name) {
      __size = _S_bufsize;
      _M_name = _M_static_name;
    }
    else {
      *(__REINTERPRET_CAST(size_t*, &_M_static_name[0])) = __size * sizeof(char);
    }
  }
  else {
    _M_name = _M_static_name;
  }
#if !defined (_STLP_USE_SAFE_STRING_FUNCTIONS)
  strncpy(_M_name, _STLP_PRIV __get_c_string(__str), __size - 1);
  _M_name[__size - 1] = '\0';
#else
  strncpy_s(_M_name, __size, _STLP_PRIV __get_c_string(__str), __size - 1);
#endif
}

__Named_exception::__Named_exception(const __Named_exception& __x) {
  size_t __size = strlen(__x._M_name) + 1;
  if (__size > _S_bufsize) {
    _M_name = __STATIC_CAST(char*, malloc(__size * sizeof(char)));
    if (!_M_name) {
      __size = _S_bufsize;
      _M_name = _M_static_name;
    }
    else {
      *(__REINTERPRET_CAST(size_t*, &_M_static_name[0])) = __size * sizeof(char);
    }
  }
  else {
    _M_name = _M_static_name;
  }
#if !defined (_STLP_USE_SAFE_STRING_FUNCTIONS)
  strncpy(_M_name, __x._M_name, __size - 1);
  _M_name[__size - 1] = '\0';
#else
  strncpy_s(_M_name, __size, __x._M_name, __size - 1);
#endif
}

__Named_exception& __Named_exception::operator = (const __Named_exception& __x) {
  size_t __size = strlen(__x._M_name) + 1;
  size_t __buf_size = _M_name != _M_static_name ? *(__REINTERPRET_CAST(size_t*, &_M_static_name[0])) : _S_bufsize;
  if (__size > __buf_size) {
    
    if (_M_name != _M_static_name) free(_M_name);
    _M_name = __STATIC_CAST(char*, malloc(__size * sizeof(char)));
    if (!_M_name) {
      __size = _S_bufsize;
      _M_name = _M_static_name;
    }
    else {
      *(__REINTERPRET_CAST(size_t*, &_M_static_name[0])) = __size * sizeof(char);
    }
  }
#if !defined (_STLP_USE_SAFE_STRING_FUNCTIONS)
  strncpy(_M_name, __x._M_name, __size - 1);
  _M_name[__size - 1] = '\0';
#else
  strncpy_s(_M_name, __size, __x._M_name, __size - 1);
#endif
  return *this;
}

__Named_exception::~__Named_exception() _STLP_NOTHROW_INHERENTLY {
  if (_M_name != _M_static_name)
    free(_M_name);
}

const char* __Named_exception::what() const _STLP_NOTHROW_INHERENTLY
{ return _M_name; }
