
















#ifndef _STLP_FSTREAM_C
#define _STLP_FSTREAM_C

#ifndef _STLP_INTERNAL_FSTREAM_H
#  include <stl/_fstream.h>
#endif

#ifndef _STLP_INTERNAL_LIMITS
#  include <stl/_limits.h>
#endif

_STLP_BEGIN_NAMESPACE

# if defined ( _STLP_NESTED_TYPE_PARAM_BUG )

# define __BF_int_type__ int
# define __BF_pos_type__ streampos
# define __BF_off_type__ streamoff
# else
# define __BF_int_type__ _STLP_TYPENAME_ON_RETURN_TYPE basic_filebuf<_CharT, _Traits>::int_type
# define __BF_pos_type__ _STLP_TYPENAME_ON_RETURN_TYPE basic_filebuf<_CharT, _Traits>::pos_type
# define __BF_off_type__ _STLP_TYPENAME_ON_RETURN_TYPE basic_filebuf<_CharT, _Traits>::off_type
# endif





template <class _CharT, class _Traits>
basic_filebuf<_CharT, _Traits>::basic_filebuf()
     :  basic_streambuf<_CharT, _Traits>(), _M_base(),
    _M_constant_width(false), _M_always_noconv(false),
    _M_int_buf_dynamic(false),
    _M_in_input_mode(false), _M_in_output_mode(false),
    _M_in_error_mode(false), _M_in_putback_mode(false),
    _M_int_buf(0), _M_int_buf_EOS(0),
    _M_ext_buf(0), _M_ext_buf_EOS(0),
    _M_ext_buf_converted(0), _M_ext_buf_end(0),
    _M_state(_STLP_DEFAULT_CONSTRUCTED(_State_type)),
    _M_end_state(_STLP_DEFAULT_CONSTRUCTED(_State_type)),
    _M_mmap_base(0), _M_mmap_len(0),
    _M_saved_eback(0), _M_saved_gptr(0), _M_saved_egptr(0),
    _M_codecvt(0),
    _M_width(1), _M_max_width(1)
{
  this->_M_setup_codecvt(locale(), false);
}

template <class _CharT, class _Traits>
basic_filebuf<_CharT, _Traits>::~basic_filebuf() {
  this->close();
  _M_deallocate_buffers();
}


template <class _CharT, class _Traits>
_STLP_TYPENAME_ON_RETURN_TYPE basic_filebuf<_CharT, _Traits>::int_type
basic_filebuf<_CharT, _Traits>::underflow() {
  return _Underflow<_CharT, _Traits>::_M_doit(this);
}

template <class _CharT, class _Traits>
basic_filebuf<_CharT, _Traits>*
basic_filebuf<_CharT, _Traits>::close() {
  bool __ok = this->is_open();

  if (_M_in_output_mode) {
    __ok = __ok && !_Traits::eq_int_type(this->overflow(traits_type::eof()),
                                         traits_type::eof());
    __ok = __ok && this->_M_unshift();
  }
  else if (_M_in_input_mode)
      this->_M_exit_input_mode();

  
  __ok = _M_base._M_close() && __ok;

  
  
  _M_state = _M_end_state = _State_type();
  _M_ext_buf_converted = _M_ext_buf_end = 0;

  _M_mmap_base = 0;
  _M_mmap_len = 0;

  this->setg(0, 0, 0);
  this->setp(0, 0);

  _M_saved_eback = _M_saved_gptr = _M_saved_egptr = 0;

  _M_in_input_mode = _M_in_output_mode = _M_in_error_mode = _M_in_putback_mode
    = false;

  return __ok ? this : 0;
}




template <class _CharT, class _Traits>
void basic_filebuf<_CharT, _Traits>::_M_exit_input_mode() {
  if (_M_mmap_base != 0) {
    _M_base._M_unmap(_M_mmap_base, _M_mmap_len);
    _M_mmap_base = 0;
    _M_mmap_len = 0;
  }
  _M_in_input_mode = false;
}





template <class _CharT, class _Traits>
streamsize basic_filebuf<_CharT, _Traits>::showmanyc() {
  
  if (!this->is_open() || _M_in_output_mode || _M_in_error_mode)
    return -1;
  else if (_M_in_putback_mode)
    return this->egptr() - this->gptr();
  else if (_M_constant_width) {
    streamoff __pos  = _M_base._M_seek(0, ios_base::cur);
    streamoff __size = _M_base._M_file_size();
    return __pos >= 0 && __size > __pos ? __size - __pos : 0;
  }
  else
    return 0;
}









template <class _CharT, class _Traits>
__BF_int_type__
basic_filebuf<_CharT, _Traits>::pbackfail(int_type __c) {
  const int_type __eof = traits_type::eof();

  
  if (!_M_in_input_mode)
    return __eof;

  
  
  if (this->gptr() != this->eback() &&
      (traits_type::eq_int_type(__c, __eof) ||
       traits_type::eq(traits_type::to_char_type(__c), this->gptr()[-1]) ||
       !_M_mmap_base)) {
    this->gbump(-1);
    if (traits_type::eq_int_type(__c, __eof) ||
        traits_type::eq(traits_type::to_char_type(__c), *this->gptr()))
      return traits_type::to_int_type(*this->gptr());
  }
  else if (!traits_type::eq_int_type(__c, __eof)) {
    
    _CharT* __pback_end = _M_pback_buf + __STATIC_CAST(int,_S_pback_buf_size);
    if (_M_in_putback_mode) {
      
      if (this->eback() != _M_pback_buf)
        this->setg(this->egptr() - 1, this->egptr() - 1, __pback_end);
      else
        return __eof;           
    }
    else {                      
      _M_saved_eback = this->eback();
      _M_saved_gptr  = this->gptr();
      _M_saved_egptr = this->egptr();
      this->setg(__pback_end - 1, __pback_end - 1, __pback_end);
      _M_in_putback_mode = true;
    }
  }
  else
    return __eof;

  
  *this->gptr() = traits_type::to_char_type(__c);
  return __c;
}






template <class _CharT, class _Traits>
__BF_int_type__
basic_filebuf<_CharT, _Traits>::overflow(int_type __c) {
  
  if (!_M_in_output_mode)
    if (!_M_switch_to_output_mode())
      return traits_type::eof();

  _CharT* __ibegin = this->_M_int_buf;
  _CharT* __iend   = this->pptr();
  this->setp(_M_int_buf, _M_int_buf_EOS - 1);

  
  if (!traits_type::eq_int_type(__c, traits_type::eof()))
    *__iend++ = _Traits::to_char_type(__c);

  
  while (__ibegin != __iend) {
    const _CharT* __inext = __ibegin;
    char* __enext         = _M_ext_buf;
    typename _Codecvt::result __status
      = _M_codecvt->out(_M_state, __ibegin, __iend, __inext,
                        _M_ext_buf, _M_ext_buf_EOS, __enext);
    if (__status == _Codecvt::noconv) {
      return _Noconv_output<_Traits>::_M_doit(this, __ibegin, __iend)
        ? traits_type::not_eof(__c)
        : _M_output_error();
    }

    
    
    
    
    
    else if (__status != _Codecvt::error &&
             (((__inext == __iend) &&
               (__enext - _M_ext_buf == _M_width * (__iend - __ibegin))) ||
              (!_M_constant_width && __inext != __ibegin))) {
        
      ptrdiff_t __n = __enext - _M_ext_buf;
      if (_M_write(_M_ext_buf, __n))
        __ibegin += __inext - __ibegin;
      else
        return _M_output_error();
    }
    else
      return _M_output_error();
  }

  return traits_type::not_eof(__c);
}









template <class _CharT, class _Traits>
basic_streambuf<_CharT, _Traits>*
basic_filebuf<_CharT, _Traits>::setbuf(_CharT* __buf, streamsize __n) {
  if (!_M_in_input_mode &&! _M_in_output_mode && !_M_in_error_mode &&
      _M_int_buf == 0) {
    if (__buf == 0 && __n == 0)
      _M_allocate_buffers(0, 1);
    else if (__buf != 0 && __n > 0)
      _M_allocate_buffers(__buf, __n);
  }
  return this;
}

#if defined (_STLP_ASSERTIONS)

template <class _CharT>
struct _Filebuf_Tmp_Buf {
  _CharT* _M_ptr;
  _Filebuf_Tmp_Buf(ptrdiff_t __n) : _M_ptr(0) { _M_ptr = new _CharT[__n]; }
  ~_Filebuf_Tmp_Buf() { delete[] _M_ptr; }
};
#endif

template <class _CharT, class _Traits>
__BF_pos_type__
basic_filebuf<_CharT, _Traits>::seekoff(off_type __off,
                                        ios_base::seekdir __whence,
                                        ios_base::openmode ) {
  if (!this->is_open())
    return pos_type(-1);

  if (!_M_constant_width && __off != 0)
    return pos_type(-1);

  if (!_M_seek_init(__off != 0 || __whence != ios_base::cur))
    return pos_type(-1);

  
  if (__whence == ios_base::beg || __whence == ios_base::end)
    return _M_seek_return(_M_base._M_seek(_M_width * __off, __whence),
                          _State_type());

  
  _STLP_ASSERT(__whence == ios_base::cur)
  if (!_M_in_input_mode)
    return _M_seek_return(_M_base._M_seek(_M_width * __off, __whence),
                          _State_type());

  if (_M_mmap_base != 0) {
    
    
    streamoff __adjust = _M_mmap_len - (this->gptr() - (_CharT*) _M_mmap_base);

    
    return __off == 0 ? pos_type(_M_base._M_seek(0, ios_base::cur) - __adjust)
                      : _M_seek_return(_M_base._M_seek(__off - __adjust, ios_base::cur), _State_type());
  }

  if (_M_constant_width) { 
    streamoff __iadj = _M_width * (this->gptr() - this->eback());

    
    
    
    

    if (__iadj <= _M_ext_buf_end - _M_ext_buf) {
      streamoff __eadj =  _M_base._M_get_offset(_M_ext_buf + __STATIC_CAST(ptrdiff_t, __iadj), _M_ext_buf_end);

      return __off == 0 ? pos_type(_M_base._M_seek(0, ios_base::cur) - __eadj)
                        : _M_seek_return(_M_base._M_seek(__off - __eadj, ios_base::cur), _State_type());
    }
  }
  else {                    
    
    ptrdiff_t __ipos = this->gptr() - this->eback();

    
    _State_type __state = _M_state;
    int __epos = _M_codecvt->length(__state, _M_ext_buf, _M_ext_buf_converted,
                                    __ipos);
#if defined (_STLP_ASSERTIONS)
    
    _STLP_ASSERT(__epos >= 0)
    _State_type __tmp_state = _M_state;
    _Filebuf_Tmp_Buf<_CharT> __buf(__ipos);
    _CharT* __ibegin = __buf._M_ptr;
    _CharT* __inext  = __ibegin;
    const char* __dummy;
    typename _Codecvt::result __status
      = _M_codecvt->in(__tmp_state,
                       _M_ext_buf, _M_ext_buf + __epos, __dummy,
                       __ibegin, __ibegin + __ipos, __inext);
    
    
    
    
    
    _STLP_ASSERT(__status == _Codecvt::ok)
    _STLP_ASSERT(__inext == __ibegin + __ipos)
    _STLP_ASSERT(equal(this->eback(), this->gptr(), __ibegin, _STLP_PRIV _Eq_traits<traits_type>()))
#endif
    
    
    streamoff __cur = _M_base._M_seek(0, ios_base::cur);
    streamoff __adj = _M_base._M_get_offset(_M_ext_buf, _M_ext_buf + __epos) -
                      _M_base._M_get_offset(_M_ext_buf, _M_ext_buf_end);
    if (__cur != -1 && __cur + __adj >= 0)
      return __off == 0 ? pos_type(__cur + __adj)
                        : _M_seek_return(__cur + __adj, __state);
  }

  return pos_type(-1);
}


template <class _CharT, class _Traits>
__BF_pos_type__
basic_filebuf<_CharT, _Traits>::seekpos(pos_type __pos,
                                        ios_base::openmode ) {
  if (this->is_open()) {
    if (!_M_seek_init(true))
      return pos_type(-1);

    streamoff __off = off_type(__pos);
    if (__off != -1 && _M_base._M_seek(__off, ios_base::beg) != -1) {
      _M_state = __pos.state();
      return _M_seek_return(__off, __pos.state());
    }
  }

  return pos_type(-1);
}


template <class _CharT, class _Traits>
int basic_filebuf<_CharT, _Traits>::sync() {
  if (_M_in_output_mode)
    return traits_type::eq_int_type(this->overflow(traits_type::eof()),
                                    traits_type::eof()) ? -1 : 0;
  return 0;
}




template <class _CharT, class _Traits>
void basic_filebuf<_CharT, _Traits>::imbue(const locale& __loc) {
  if (!_M_in_input_mode && !_M_in_output_mode && !_M_in_error_mode) {
    this->_M_setup_codecvt(__loc);
  }
}










template <class _CharT, class _Traits>
bool basic_filebuf<_CharT, _Traits>::_M_switch_to_input_mode() {
  if (this->is_open() && (((int)_M_base.__o_mode() & (int)ios_base::in) !=0)
      && (_M_in_output_mode == 0) && (_M_in_error_mode == 0)) {
    if (!_M_int_buf && !_M_allocate_buffers())
      return false;

    _M_ext_buf_converted = _M_ext_buf;
    _M_ext_buf_end       = _M_ext_buf;

    _M_end_state    = _M_state;

    _M_in_input_mode = true;
    return true;
  }

  return false;
}





template <class _CharT, class _Traits>
bool basic_filebuf<_CharT, _Traits>::_M_switch_to_output_mode() {
  if (this->is_open() && (_M_base.__o_mode() & (int)ios_base::out) &&
      _M_in_input_mode == 0 && _M_in_error_mode == 0) {

    if (!_M_int_buf && !_M_allocate_buffers())
      return false;

    
    
    
    if (_M_base.__o_mode() & ios_base::app)
      _M_state = _State_type();

    this->setp(_M_int_buf, _M_int_buf_EOS - 1);
    _M_in_output_mode = true;
    return true;
  }

  return false;
}











template <class _CharT, class _Traits>
__BF_int_type__
basic_filebuf<_CharT, _Traits>::_M_input_error() {
   this->_M_exit_input_mode();
  _M_in_output_mode = false;
  _M_in_error_mode = true;
  this->setg(0, 0, 0);
  return traits_type::eof();
}

template <class _CharT, class _Traits>
__BF_int_type__
basic_filebuf<_CharT, _Traits>::_M_underflow_aux() {
  
  
  _M_state    = _M_end_state;

  
  
  if (_M_ext_buf_end > _M_ext_buf_converted)

    _M_ext_buf_end = _STLP_STD::copy(_M_ext_buf_converted, _M_ext_buf_end, _M_ext_buf);
    
    
    
  else
    _M_ext_buf_end = _M_ext_buf;

  
  
  
  for (;;) {
    ptrdiff_t __n = _M_base._M_read(_M_ext_buf_end, _M_ext_buf_EOS - _M_ext_buf_end);
    if (__n < 0) {
      
      this->setg(0, 0, 0);
      return traits_type::eof();
    }

    _M_ext_buf_end += __n;

    
    if (_M_ext_buf == _M_ext_buf_end) {
      this->setg(0, 0, 0);
      return traits_type::eof();
    }

    
    const char* __enext;
    _CharT* __inext;

    typename _Codecvt::result __status
      = _M_codecvt->in(_M_end_state,
                       _M_ext_buf, _M_ext_buf_end, __enext,
                       _M_int_buf, _M_int_buf_EOS, __inext);

    







    if (__status == _Codecvt::noconv)
      return _Noconv_input<_Traits>::_M_doit(this);
    else if (__status == _Codecvt::error ||
            (__inext != _M_int_buf && __enext == _M_ext_buf) ||
            (_M_constant_width && (__inext - _M_int_buf) *  _M_width != (__enext - _M_ext_buf)) ||
            (__inext == _M_int_buf && __enext - _M_ext_buf >= _M_max_width))
      return _M_input_error();
    else if (__inext != _M_int_buf) {
      _M_ext_buf_converted = _M_ext_buf + (__enext - _M_ext_buf);
      this->setg(_M_int_buf, _M_int_buf, __inext);
      return traits_type::to_int_type(*_M_int_buf);
    }
    



    if (__n <= 0) {
      this->setg(0, 0, 0);
      return traits_type::eof();
    }
  }
}








template <class _CharT, class _Traits>
__BF_int_type__
basic_filebuf<_CharT, _Traits>::_M_output_error() {
  _M_in_output_mode = false;
  _M_in_input_mode = false;
  _M_in_error_mode = true;
  this->setp(0, 0);
  return traits_type::eof();
}






template <class _CharT, class _Traits>
bool basic_filebuf<_CharT, _Traits>::_M_unshift() {
  if (_M_in_output_mode && !_M_constant_width) {
    typename _Codecvt::result __status;
    do {
      char* __enext = _M_ext_buf;
      __status = _M_codecvt->unshift(_M_state,
                                     _M_ext_buf, _M_ext_buf_EOS, __enext);
      if (__status == _Codecvt::noconv ||
          (__enext == _M_ext_buf && __status == _Codecvt::ok))
        return true;
      else if (__status == _Codecvt::error)
        return false;
      else if (!_M_write(_M_ext_buf, __enext - _M_ext_buf))
        return false;
    } while (__status == _Codecvt::partial);
  }

  return true;
}

















template <class _CharT, class _Traits>
bool basic_filebuf<_CharT, _Traits>::_M_allocate_buffers(_CharT* __buf, streamsize __n) {
  
  
  
  _STLP_STATIC_ASSERT(!numeric_limits<size_t>::is_signed &&
                      sizeof(streamsize) >= sizeof(int))

  if (__buf == 0) {
    streamsize __bufsize = __n * sizeof(_CharT);
    
    
    if ((sizeof(streamsize) > sizeof(size_t)) &&
        (__bufsize > __STATIC_CAST(streamsize, (numeric_limits<size_t>::max)())))
      return false;
    _M_int_buf = __STATIC_CAST(_CharT*, malloc(__STATIC_CAST(size_t, __bufsize)));
    if (!_M_int_buf)
      return false;
    _M_int_buf_dynamic = true;
  }
  else {
    _M_int_buf = __buf;
    _M_int_buf_dynamic = false;
  }

  streamsize __ebufsiz = (max)(__n * __STATIC_CAST(streamsize, _M_width),
                               __STATIC_CAST(streamsize, _M_codecvt->max_length()));

  _M_ext_buf = 0;
  if ((sizeof(streamsize) < sizeof(size_t)) ||
      ((sizeof(streamsize) == sizeof(size_t)) && numeric_limits<streamsize>::is_signed) ||
      (__ebufsiz <= __STATIC_CAST(streamsize, (numeric_limits<size_t>::max)()))) {
    _M_ext_buf = __STATIC_CAST(char*, malloc(__STATIC_CAST(size_t, __ebufsiz)));
  }

  if (!_M_ext_buf) {
    _M_deallocate_buffers();
    return false;
  }

  _M_int_buf_EOS = _M_int_buf + __STATIC_CAST(ptrdiff_t, __n);
  _M_ext_buf_EOS = _M_ext_buf + __STATIC_CAST(ptrdiff_t, __ebufsiz);
  return true;
}


template <class _CharT, class _Traits>
bool basic_filebuf<_CharT, _Traits>::_M_allocate_buffers() {
  
  
  streamsize __default_bufsiz =
    ((_M_base.__page_size() + 4095UL) / _M_base.__page_size()) * _M_base.__page_size();
  return _M_allocate_buffers(0, __default_bufsiz);
}

template <class _CharT, class _Traits>
void basic_filebuf<_CharT, _Traits>::_M_deallocate_buffers() {
  if (_M_int_buf_dynamic)
    free(_M_int_buf);
  free(_M_ext_buf);
  _M_int_buf     = 0;
  _M_int_buf_EOS = 0;
  _M_ext_buf     = 0;
  _M_ext_buf_EOS = 0;
}





template <class _CharT, class _Traits>
bool basic_filebuf<_CharT, _Traits>::_M_seek_init(bool __do_unshift) {
  
   _M_in_error_mode = false;

  
  
  if (_M_in_output_mode) {
    bool __ok = !traits_type::eq_int_type(this->overflow(traits_type::eof()),
                                          traits_type::eof());
    if (__do_unshift)
      __ok = __ok && this->_M_unshift();
    if (!__ok) {
      _M_in_output_mode = false;
      _M_in_error_mode = true;
      this->setp(0, 0);
      return false;
    }
  }

  
  if (_M_in_input_mode && _M_in_putback_mode)
    _M_exit_putback_mode();

  return true;
}









template <class _CharT, class _Traits>
void basic_filebuf<_CharT, _Traits>::_M_setup_codecvt(const locale& __loc, bool __on_imbue) {
  if (has_facet<_Codecvt>(__loc)) {
    _M_codecvt = &use_facet<_Codecvt>(__loc) ;
    int __encoding    = _M_codecvt->encoding();

    _M_width          = (max)(__encoding, 1);
    _M_max_width      = _M_codecvt->max_length();
    _M_constant_width = __encoding > 0;
    _M_always_noconv  = _M_codecvt->always_noconv();
  }
  else {
    _M_codecvt = 0;
    _M_width = _M_max_width = 1;
    _M_constant_width = _M_always_noconv  = false;
    if (__on_imbue) {
      
      use_facet<_Codecvt>(__loc);
    }
  }
}

_STLP_END_NAMESPACE

# undef __BF_int_type__
# undef __BF_pos_type__
# undef __BF_off_type__

#endif 




