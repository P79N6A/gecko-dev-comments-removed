




















#ifndef _STLP_INTERNAL_FSTREAM_H
#define _STLP_INTERNAL_FSTREAM_H

#if defined(__sgi) && !defined(__GNUC__) && !defined(_STANDARD_C_PLUS_PLUS)
#  error This header file requires the -LANG:std option
#endif

#ifndef _STLP_INTERNAL_STREAMBUF
#  include <stl/_streambuf.h>
#endif

#ifndef _STLP_INTERNAL_ISTREAM
#  include <stl/_istream.h>
#endif

#ifndef _STLP_INTERNAL_CODECVT_H
#  include <stl/_codecvt.h>
#endif

#if defined (_STLP_USE_WIN32_IO)
typedef void* _STLP_fd;
#elif defined (_STLP_USE_UNIX_EMULATION_IO) || defined (_STLP_USE_STDIO_IO) || defined (_STLP_USE_UNIX_IO)
typedef int _STLP_fd;
#else
#  error "Configure i/o !"
#endif

_STLP_BEGIN_NAMESPACE





class _STLP_CLASS_DECLSPEC _Filebuf_base {
public:                      
  _Filebuf_base();

  bool _M_open(const char*, ios_base::openmode, long __protection);
  bool _M_open(const char*, ios_base::openmode);
  bool _M_open(int __id, ios_base::openmode = ios_base::__default_mode);
#if defined (_STLP_USE_WIN32_IO)
  bool _M_open(_STLP_fd __id, ios_base::openmode = ios_base::__default_mode);
#endif 
  bool _M_close();

public:                      
  ptrdiff_t _M_read(char* __buf,  ptrdiff_t __n);
  streamoff _M_seek(streamoff __offset, ios_base::seekdir __dir);
  streamoff _M_file_size();
  bool _M_write(char* __buf,  ptrdiff_t __n);

public:                      
  void* _M_mmap(streamoff __offset, streamoff __len);
  void _M_unmap(void* __mmap_base, streamoff __len);

public:
  
  
  
  
  
  
  
  streamoff _M_get_offset(char* __first, char* __last) {
#if defined (_STLP_UNIX) || defined (_STLP_MAC)
    return __last - __first;
#else 
    return ( (_M_openmode & ios_base::binary) != 0 )
      ? (__last - __first)
      : count(__first, __last, '\n') + (__last - __first);
#endif
  }

  
  
  bool _M_in_binary_mode() const {
#if defined (_STLP_UNIX) || defined (_STLP_MAC) || defined(__BEOS__) || defined (__amigaos__)
    return true;
#elif defined (_STLP_WIN32) || defined (_STLP_VM)
    return (_M_openmode & ios_base::binary) != 0;
#else
#  error "Port!"
#endif
  }

  static void _S_initialize();

protected:                      
  static size_t _M_page_size;

protected:                      
  _STLP_fd _M_file_id;
#if defined (_STLP_USE_STDIO_IO)
  
  FILE* _M_file;
#endif
  ios_base::openmode _M_openmode     ;
  unsigned char      _M_is_open      ;
  unsigned char      _M_should_close ;
  unsigned char      _M_regular_file ;

#if defined (_STLP_USE_WIN32_IO)
  _STLP_fd _M_view_id;
#endif

public :
  static size_t  _STLP_CALL __page_size() { return _M_page_size; }
  int  __o_mode() const { return (int)_M_openmode; }
  bool __is_open()      const { return (_M_is_open !=0 ); }
  bool __should_close() const { return (_M_should_close != 0); }
  bool __regular_file() const { return (_M_regular_file != 0); }
  _STLP_fd __get_fd() const { return _M_file_id; }
};





template <class _Traits> class _Noconv_input;
template <class _Traits> class _Noconv_output;



template <class _CharT, class _Traits>
class _Underflow;

template <class _CharT, class _Traits>
class basic_filebuf : public basic_streambuf<_CharT, _Traits> {
public:                         
  typedef _CharT                     char_type;
  typedef typename _Traits::int_type int_type;
  typedef typename _Traits::pos_type pos_type;
  typedef typename _Traits::off_type off_type;
  typedef _Traits                    traits_type;

  typedef typename _Traits::state_type _State_type;
  typedef basic_streambuf<_CharT, _Traits> _Base;
  typedef basic_filebuf<_CharT, _Traits> _Self;

public:                         
  basic_filebuf();
  ~basic_filebuf();

public:                         
  bool is_open() const { return _M_base.__is_open(); }

  _Self* open(const char* __s, ios_base::openmode __m) {
    return _M_base._M_open(__s, __m) ? this : 0;
  }

#if !defined (_STLP_NO_EXTENSIONS)
  
  _Self* open(const char* __s, ios_base::openmode __m,
              long __protection) {
    return _M_base._M_open(__s, __m, __protection) ? this : 0;
  }

  _STLP_fd fd() const { return _M_base.__get_fd(); }

  _Self* open(int __id, ios_base::openmode _Init_mode = ios_base::__default_mode) {
    return this->_M_open(__id, _Init_mode);
  }

#  if defined (_STLP_USE_WIN32_IO)
  _Self* open(_STLP_fd __id, ios_base::openmode _Init_mode = ios_base::__default_mode) {
    return _M_base._M_open(__id, _Init_mode) ? this : 0;
  }
#  endif 

#endif

  _Self* _M_open(int __id, ios_base::openmode _Init_mode = ios_base::__default_mode) {
    return _M_base._M_open(__id, _Init_mode) ? this : 0;
  }

  _Self* close();

protected:                      
  virtual streamsize showmanyc();
  virtual int_type underflow();

  virtual int_type pbackfail(int_type = traits_type::eof());
  virtual int_type overflow(int_type = traits_type::eof());

  virtual basic_streambuf<_CharT, _Traits>* setbuf(char_type*, streamsize);
  virtual pos_type seekoff(off_type, ios_base::seekdir,
                           ios_base::openmode = ios_base::in | ios_base::out);
  virtual pos_type seekpos(pos_type,
                           ios_base::openmode = ios_base::in | ios_base::out);

  virtual int sync();
  virtual void imbue(const locale&);

private:                        

  
  
  void _M_exit_putback_mode() {
    this->setg(_M_saved_eback, _M_saved_gptr, _M_saved_egptr);
    _M_in_putback_mode = false;
  }
  bool _M_switch_to_input_mode();
  void _M_exit_input_mode();
  bool _M_switch_to_output_mode();

  int_type _M_input_error();
  int_type _M_underflow_aux();
  friend class _Underflow<_CharT, _Traits>;

  int_type _M_output_error();
  bool _M_unshift();

  bool _M_allocate_buffers(_CharT* __buf, streamsize __n);
  bool _M_allocate_buffers();
  void _M_deallocate_buffers();

  pos_type _M_seek_return(off_type __off, _State_type __state) {
    if (__off != -1) {
      if (_M_in_input_mode)
        _M_exit_input_mode();
      _M_in_input_mode = false;
      _M_in_output_mode = false;
      _M_in_putback_mode = false;
      _M_in_error_mode = false;
      this->setg(0, 0, 0);
      this->setp(0, 0);
    }

    pos_type __result(__off);
    __result.state(__state);
    return __result;
  }

  bool _M_seek_init(bool __do_unshift);

  void _M_setup_codecvt(const locale&, bool __on_imbue = true);

private:                        

  _Filebuf_base _M_base;

private:                        

  unsigned char _M_constant_width;
  unsigned char _M_always_noconv;

  
  unsigned char _M_int_buf_dynamic;  
  
  unsigned char _M_in_input_mode;
  unsigned char _M_in_output_mode;
  unsigned char _M_in_error_mode;
  unsigned char _M_in_putback_mode;

  
  _CharT* _M_int_buf;
  _CharT* _M_int_buf_EOS;

  
  char* _M_ext_buf;
  char* _M_ext_buf_EOS;

  
  
  
  
  
  char* _M_ext_buf_converted;
  char* _M_ext_buf_end;

  
  _State_type _M_state;

private:                        

  
  
  _State_type _M_end_state;

  
  void*     _M_mmap_base;
  streamoff _M_mmap_len;

private:                        
  _CharT* _M_saved_eback;
  _CharT* _M_saved_gptr;
  _CharT* _M_saved_egptr;

  typedef codecvt<_CharT, char, _State_type> _Codecvt;
  const _Codecvt* _M_codecvt;

  int _M_width;                 
  int _M_max_width;             


  enum { _S_pback_buf_size = 8 };
  _CharT _M_pback_buf[_S_pback_buf_size];

  
public:
  bool _M_write(char* __buf,  ptrdiff_t __n) {return _M_base._M_write(__buf, __n); }

public:
  int_type
  _M_do_noconv_input() {
    _M_ext_buf_converted = _M_ext_buf_end;
     _Base::setg((char_type*)_M_ext_buf, (char_type*)_M_ext_buf, (char_type*)_M_ext_buf_end);
    return traits_type::to_int_type(*_M_ext_buf);
  }
};

#if defined (_STLP_USE_TEMPLATE_EXPORT)
_STLP_EXPORT_TEMPLATE_CLASS basic_filebuf<char, char_traits<char> >;
#  if ! defined (_STLP_NO_WCHAR_T)
_STLP_EXPORT_TEMPLATE_CLASS basic_filebuf<wchar_t, char_traits<wchar_t> >;
#  endif
#endif 





template <class _Traits>
class _Noconv_output {
public:
  typedef typename _Traits::char_type char_type;
  static bool  _STLP_CALL _M_doit(basic_filebuf<char_type, _Traits >*,
                                  char_type*, char_type*)
  { return false; }
};

_STLP_TEMPLATE_NULL
class _STLP_CLASS_DECLSPEC _Noconv_output< char_traits<char> > {
public:
  static bool  _STLP_CALL
  _M_doit(basic_filebuf<char, char_traits<char> >* __buf,
          char* __first, char* __last) {
    ptrdiff_t __n = __last - __first;
    return (__buf->_M_write(__first, __n));
  }
};












template <class _Traits>
class _Noconv_input {
public:
  typedef typename _Traits::int_type int_type;
  typedef typename _Traits::char_type char_type;

  static inline int_type _STLP_CALL
  _M_doit(basic_filebuf<char_type, _Traits>*)
  { return _Traits::eof(); }
};

_STLP_TEMPLATE_NULL
class _Noconv_input<char_traits<char> > {
public:
  static inline int _STLP_CALL
  _M_doit(basic_filebuf<char, char_traits<char> >* __buf) {
    return __buf->_M_do_noconv_input();
  }
};





template <class _CharT, class _Traits>
class _Underflow {
public:
  typedef typename _Traits::int_type int_type;
  typedef _Traits                    traits_type;

  
  
  static int_type _STLP_CALL _M_doit(basic_filebuf<_CharT, _Traits>* __this) {
    if (!__this->_M_in_input_mode) {
      if (!__this->_M_switch_to_input_mode())
        return traits_type::eof();
    }
    else if (__this->_M_in_putback_mode) {
      __this->_M_exit_putback_mode();
      if (__this->gptr() != __this->egptr()) {
        int_type __c = traits_type::to_int_type(*__this->gptr());
        return __c;
      }
    }

    return __this->_M_underflow_aux();
  }
};



_STLP_TEMPLATE_NULL
class _STLP_CLASS_DECLSPEC _Underflow< char, char_traits<char> >
{
  public:
    typedef char_traits<char>::int_type int_type;
    typedef char_traits<char> traits_type;
    static int_type _STLP_CALL _M_doit(basic_filebuf<char, traits_type >* __this);
};

#if defined (_STLP_USE_TEMPLATE_EXPORT) && !defined (_STLP_NO_WCHAR_T)
_STLP_EXPORT_TEMPLATE_CLASS _Underflow<wchar_t, char_traits<wchar_t> >;
#endif




template <class _CharT, class _Traits>
class basic_ifstream : public basic_istream<_CharT, _Traits> {
public:                         
  typedef _CharT                     char_type;
  typedef typename _Traits::int_type int_type;
  typedef typename _Traits::pos_type pos_type;
  typedef typename _Traits::off_type off_type;
  typedef _Traits                    traits_type;

  typedef basic_ios<_CharT, _Traits>                _Basic_ios;
  typedef basic_istream<_CharT, _Traits>            _Base;
  typedef basic_filebuf<_CharT, _Traits>            _Buf;

public:                         

  basic_ifstream() :
    basic_ios<_CharT, _Traits>(),  basic_istream<_CharT, _Traits>(0), _M_buf() {
      this->init(&_M_buf);
  }

  explicit basic_ifstream(const char* __s, ios_base::openmode __mod = ios_base::in) :
    basic_ios<_CharT, _Traits>(),  basic_istream<_CharT, _Traits>(0),
    _M_buf() {
      this->init(&_M_buf);
      if (!_M_buf.open(__s, __mod | ios_base::in))
        this->setstate(ios_base::failbit);
  }

#if !defined (_STLP_NO_EXTENSIONS)
  explicit basic_ifstream(int __id, ios_base::openmode __mod = ios_base::in) :
    basic_ios<_CharT, _Traits>(),  basic_istream<_CharT, _Traits>(0), _M_buf() {
    this->init(&_M_buf);
    if (!_M_buf.open(__id, __mod | ios_base::in))
      this->setstate(ios_base::failbit);
  }
  basic_ifstream(const char* __s, ios_base::openmode __m,
     long __protection) :
    basic_ios<_CharT, _Traits>(),  basic_istream<_CharT, _Traits>(0), _M_buf() {
    this->init(&_M_buf);
    if (!_M_buf.open(__s, __m | ios_base::in, __protection))
      this->setstate(ios_base::failbit);
  }

#  if defined (_STLP_USE_WIN32_IO)
  explicit basic_ifstream(_STLP_fd __id, ios_base::openmode __mod = ios_base::in) :
    basic_ios<_CharT, _Traits>(),  basic_istream<_CharT, _Traits>(0), _M_buf() {
    this->init(&_M_buf);
    if (!_M_buf.open(__id, __mod | ios_base::in))
      this->setstate(ios_base::failbit);
  }
#  endif 
#endif

  ~basic_ifstream() {}

public:                         
  basic_filebuf<_CharT, _Traits>* rdbuf() const
    { return __CONST_CAST(_Buf*,&_M_buf); }

  bool is_open() {
    return this->rdbuf()->is_open();
  }

  void open(const char* __s, ios_base::openmode __mod = ios_base::in) {
    if (!this->rdbuf()->open(__s, __mod | ios_base::in))
      this->setstate(ios_base::failbit);
  }

  void close() {
    if (!this->rdbuf()->close())
      this->setstate(ios_base::failbit);
  }

private:
  basic_filebuf<_CharT, _Traits> _M_buf;
};





template <class _CharT, class _Traits>
class basic_ofstream : public basic_ostream<_CharT, _Traits> {
public:                         
  typedef _CharT                     char_type;
  typedef typename _Traits::int_type int_type;
  typedef typename _Traits::pos_type pos_type;
  typedef typename _Traits::off_type off_type;
  typedef _Traits                    traits_type;

  typedef basic_ios<_CharT, _Traits>                _Basic_ios;
  typedef basic_ostream<_CharT, _Traits>            _Base;
  typedef basic_filebuf<_CharT, _Traits>            _Buf;

public:                         
  basic_ofstream() :
    basic_ios<_CharT, _Traits>(),
    basic_ostream<_CharT, _Traits>(0), _M_buf() {
      this->init(&_M_buf);
  }
  explicit basic_ofstream(const char* __s, ios_base::openmode __mod = ios_base::out)
    : basic_ios<_CharT, _Traits>(), basic_ostream<_CharT, _Traits>(0), _M_buf() {
    this->init(&_M_buf);
    if (!_M_buf.open(__s, __mod | ios_base::out))
      this->setstate(ios_base::failbit);
  }

#if !defined (_STLP_NO_EXTENSIONS)
  explicit basic_ofstream(int __id, ios_base::openmode __mod = ios_base::out)
    : basic_ios<_CharT, _Traits>(), basic_ostream<_CharT, _Traits>(0),
    _M_buf() {
   this->init(&_M_buf);
   if (!_M_buf.open(__id, __mod | ios_base::out))
     this->setstate(ios_base::failbit);
  }
  basic_ofstream(const char* __s, ios_base::openmode __m, long __protection) :
    basic_ios<_CharT, _Traits>(),  basic_ostream<_CharT, _Traits>(0), _M_buf() {
    this->init(&_M_buf);
    if (!_M_buf.open(__s, __m | ios_base::out, __protection))
      this->setstate(ios_base::failbit);
  }
#  if defined (_STLP_USE_WIN32_IO)
  explicit basic_ofstream(_STLP_fd __id, ios_base::openmode __mod = ios_base::out)
    : basic_ios<_CharT, _Traits>(), basic_ostream<_CharT, _Traits>(0),
    _M_buf() {
   this->init(&_M_buf);
   if (!_M_buf.open(__id, __mod | ios_base::out))
     this->setstate(ios_base::failbit);
  }
#  endif 
#endif

  ~basic_ofstream() {}

public:                         
  basic_filebuf<_CharT, _Traits>* rdbuf() const
    { return __CONST_CAST(_Buf*,&_M_buf); }

  bool is_open() {
    return this->rdbuf()->is_open();
  }

  void open(const char* __s, ios_base::openmode __mod= ios_base::out) {
    if (!this->rdbuf()->open(__s, __mod | ios_base::out))
      this->setstate(ios_base::failbit);
  }

  void close() {
    if (!this->rdbuf()->close())
      this->setstate(ios_base::failbit);
  }

private:
  basic_filebuf<_CharT, _Traits> _M_buf;
};





template <class _CharT, class _Traits>
class basic_fstream : public basic_iostream<_CharT, _Traits> {
public:                         
  typedef _CharT                     char_type;
  typedef typename _Traits::int_type int_type;
  typedef typename _Traits::pos_type pos_type;
  typedef typename _Traits::off_type off_type;
  typedef _Traits                    traits_type;

  typedef basic_ios<_CharT, _Traits>                _Basic_ios;
  typedef basic_iostream<_CharT, _Traits>           _Base;
  typedef basic_filebuf<_CharT, _Traits>            _Buf;

public:                         

  basic_fstream()
    : basic_ios<_CharT, _Traits>(), basic_iostream<_CharT, _Traits>(0), _M_buf() {
      this->init(&_M_buf);
  }

  explicit basic_fstream(const char* __s,
                         ios_base::openmode __mod = ios_base::in | ios_base::out) :
    basic_ios<_CharT, _Traits>(), basic_iostream<_CharT, _Traits>(0), _M_buf() {
      this->init(&_M_buf);
      if (!_M_buf.open(__s, __mod))
        this->setstate(ios_base::failbit);
  }

#if !defined (_STLP_NO_EXTENSIONS)
  explicit basic_fstream(int __id,
                         ios_base::openmode __mod = ios_base::in | ios_base::out) :
    basic_ios<_CharT, _Traits>(), basic_iostream<_CharT, _Traits>(0), _M_buf() {
    this->init(&_M_buf);
    if (!_M_buf.open(__id, __mod))
      this->setstate(ios_base::failbit);
  }
  basic_fstream(const char* __s, ios_base::openmode __m, long __protection) :
    basic_ios<_CharT, _Traits>(),  basic_iostream<_CharT, _Traits>(0), _M_buf() {
    this->init(&_M_buf);
    if (!_M_buf.open(__s, __m, __protection))
      this->setstate(ios_base::failbit);
  }
#  if defined (_STLP_USE_WIN32_IO)
  explicit basic_fstream(_STLP_fd __id,
    ios_base::openmode __mod = ios_base::in | ios_base::out) :
    basic_ios<_CharT, _Traits>(),  basic_iostream<_CharT, _Traits>(0), _M_buf() {
    this->init(&_M_buf);
    if (!_M_buf.open(__id, __mod))
      this->setstate(ios_base::failbit);
  }
#  endif 
#endif
  ~basic_fstream() {}

public:                         

  basic_filebuf<_CharT, _Traits>* rdbuf() const
    { return __CONST_CAST(_Buf*,&_M_buf); }

  bool is_open() {
    return this->rdbuf()->is_open();
  }

  void open(const char* __s,
      ios_base::openmode __mod =
      ios_base::in | ios_base::out) {
    if (!this->rdbuf()->open(__s, __mod))
      this->setstate(ios_base::failbit);
  }

  void close() {
    if (!this->rdbuf()->close())
      this->setstate(ios_base::failbit);
  }

private:
  basic_filebuf<_CharT, _Traits> _M_buf;

#if defined (_STLP_MSVC) && (_STLP_MSVC >= 1300 && _STLP_MSVC <= 1310)
  typedef basic_fstream<_CharT, _Traits> _Self;
  
  basic_fstream(_Self const&);
  _Self& operator = (_Self const&);
#endif
};

_STLP_END_NAMESPACE

#if defined (_STLP_EXPOSE_STREAM_IMPLEMENTATION) && !defined (_STLP_LINK_TIME_INSTANTIATION)
#  include <stl/_fstream.c>
#endif

_STLP_BEGIN_NAMESPACE

#if defined (_STLP_USE_TEMPLATE_EXPORT)
_STLP_EXPORT_TEMPLATE_CLASS basic_ifstream<char, char_traits<char> >;
_STLP_EXPORT_TEMPLATE_CLASS basic_ofstream<char, char_traits<char> >;
_STLP_EXPORT_TEMPLATE_CLASS basic_fstream<char, char_traits<char> >;
#  if ! defined (_STLP_NO_WCHAR_T)
_STLP_EXPORT_TEMPLATE_CLASS basic_ifstream<wchar_t, char_traits<wchar_t> >;
_STLP_EXPORT_TEMPLATE_CLASS basic_ofstream<wchar_t, char_traits<wchar_t> >;
_STLP_EXPORT_TEMPLATE_CLASS basic_fstream<wchar_t, char_traits<wchar_t> >;
#  endif
#endif 

_STLP_END_NAMESPACE

#endif 





