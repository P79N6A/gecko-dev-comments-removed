























#ifndef _STLP_INTERNAL_SSTREAM
#define _STLP_INTERNAL_SSTREAM

#ifndef _STLP_INTERNAL_STREAMBUF
#  include <stl/_streambuf.h>
#endif

#ifndef _STLP_INTERNAL_ISTREAM
#  include <stl/_istream.h> 
#endif

#ifndef _STLP_INTERNAL_STRING_H
#  include <stl/_string.h>
#endif

_STLP_BEGIN_NAMESPACE













template <class _CharT, class _Traits, class _Alloc>
class basic_stringbuf : public basic_streambuf<_CharT, _Traits> {
public:                         
  typedef _CharT                     char_type;
  typedef typename _Traits::int_type int_type;
  typedef typename _Traits::pos_type pos_type;
  typedef typename _Traits::off_type off_type;
  typedef _Traits                    traits_type;

  typedef basic_streambuf<_CharT, _Traits>          _Base;
  typedef basic_stringbuf<_CharT, _Traits, _Alloc>  _Self;
  typedef basic_string<_CharT, _Traits, _Alloc>     _String;

public:                         
  explicit basic_stringbuf(ios_base::openmode __mode
                                      = ios_base::in | ios_base::out);
  explicit basic_stringbuf(const _String& __s, ios_base::openmode __mode
                                      = ios_base::in | ios_base::out);
  virtual ~basic_stringbuf();

public:                         
  _String str() const { return _M_str; }
  void str(const _String& __s);

protected:                      
  virtual int_type underflow();
  virtual int_type uflow();
  virtual int_type pbackfail(int_type __c);
  virtual int_type overflow(int_type __c);
  int_type pbackfail() {return pbackfail(_Traits::eof());}
  int_type overflow() {return overflow(_Traits::eof());}

  virtual streamsize xsputn(const char_type* __s, streamsize __n);
  virtual streamsize _M_xsputnc(char_type __c, streamsize __n);

  virtual _Base* setbuf(_CharT* __buf, streamsize __n);
  virtual pos_type seekoff(off_type __off, ios_base::seekdir __dir,
                           ios_base::openmode __mode
                                      = ios_base::in | ios_base::out);
  virtual pos_type seekpos(pos_type __pos, ios_base::openmode __mode
                                      = ios_base::in | ios_base::out);

private:                        
  void _M_set_ptrs();
  static _CharT* _S_start(const _String& __str) { return __CONST_CAST(_CharT*, __str.data()); }
  static _CharT* _S_finish(const _String& __str) { return __CONST_CAST(_CharT*, __str.data()) + __str.size(); }

private:
  ios_base::openmode _M_mode;
  _String _M_str;
};

#if defined (_STLP_USE_TEMPLATE_EXPORT)
_STLP_EXPORT_TEMPLATE_CLASS basic_stringbuf<char, char_traits<char>, allocator<char> >;
#  if !defined (_STLP_NO_WCHAR_T)
_STLP_EXPORT_TEMPLATE_CLASS basic_stringbuf<wchar_t, char_traits<wchar_t>, allocator<wchar_t>  >;
#  endif
#endif 




template <class _CharT, class _Traits, class _Alloc>
class basic_istringstream : public basic_istream<_CharT, _Traits> {
public:                         
  typedef typename _Traits::char_type   char_type;
  typedef typename _Traits::int_type    int_type;
  typedef typename _Traits::pos_type    pos_type;
  typedef typename _Traits::off_type    off_type;
  typedef _Traits traits_type;

  typedef basic_ios<_CharT, _Traits>                _Basic_ios;
  typedef basic_istream<_CharT, _Traits>            _Base;
  typedef basic_string<_CharT, _Traits, _Alloc>     _String;
  typedef basic_stringbuf<_CharT, _Traits, _Alloc>  _Buf;

public:                         
  basic_istringstream(ios_base::openmode __mode = ios_base::in);
  basic_istringstream(const _String& __str,
                      ios_base::openmode __mode = ios_base::in);
  ~basic_istringstream();

public:                         

  basic_stringbuf<_CharT, _Traits, _Alloc>* rdbuf() const
    { return __CONST_CAST(_Buf*,&_M_buf); }

  _String str() const { return _M_buf.str(); }
  void str(const _String& __s) { _M_buf.str(__s); }

private:
  basic_stringbuf<_CharT, _Traits, _Alloc> _M_buf;

#if defined (_STLP_MSVC) && (_STLP_MSVC >= 1300 && _STLP_MSVC <= 1310)
  typedef basic_istringstream<_CharT, _Traits> _Self;
  
  basic_istringstream(_Self const&);
  _Self& operator = (_Self const&);
#endif
};





template <class _CharT, class _Traits, class _Alloc>
class basic_ostringstream : public basic_ostream<_CharT, _Traits> {
public:                         
  typedef typename _Traits::char_type   char_type;
  typedef typename _Traits::int_type    int_type;
  typedef typename _Traits::pos_type    pos_type;
  typedef typename _Traits::off_type    off_type;
  typedef _Traits traits_type;

  typedef basic_ios<_CharT, _Traits>                _Basic_ios;
  typedef basic_ostream<_CharT, _Traits>            _Base;
  typedef basic_string<_CharT, _Traits, _Alloc>     _String;
  typedef basic_stringbuf<_CharT, _Traits, _Alloc>  _Buf;

public:                         
  basic_ostringstream(ios_base::openmode __mode = ios_base::out);
  basic_ostringstream(const _String& __str,
                      ios_base::openmode __mode = ios_base::out);
  ~basic_ostringstream();

public:                         

  basic_stringbuf<_CharT, _Traits, _Alloc>* rdbuf() const
    { return __CONST_CAST(_Buf*,&_M_buf); }

  _String str() const { return _M_buf.str(); }
    void str(const _String& __s) { _M_buf.str(__s); } 


private:
  basic_stringbuf<_CharT, _Traits, _Alloc> _M_buf;

#if defined (_STLP_MSVC) && (_STLP_MSVC >= 1300 && _STLP_MSVC <= 1310)
  typedef basic_ostringstream<_CharT, _Traits> _Self;
  
  basic_ostringstream(_Self const&);
  _Self& operator = (_Self const&);
#endif
};





template <class _CharT, class _Traits, class _Alloc>
class basic_stringstream : public basic_iostream<_CharT, _Traits> {
public:                         
  typedef typename _Traits::char_type char_type;
  typedef typename _Traits::int_type  int_type;
  typedef typename _Traits::pos_type  pos_type;
  typedef typename _Traits::off_type  off_type;
  typedef _Traits  traits_type;

  typedef basic_ios<_CharT, _Traits>                 _Basic_ios;
  typedef basic_iostream<_CharT, _Traits>            _Base;
  typedef basic_string<_CharT, _Traits, _Alloc>      _String;
  typedef basic_stringbuf<_CharT, _Traits, _Alloc>  _Buf;

  typedef ios_base::openmode openmode;

public:                         
  basic_stringstream(openmode __mod = ios_base::in | ios_base::out);
  basic_stringstream(const _String& __str,
                     openmode __mod = ios_base::in | ios_base::out);
  ~basic_stringstream();

public:                         

  basic_stringbuf<_CharT, _Traits, _Alloc>* rdbuf() const
    { return __CONST_CAST(_Buf*,&_M_buf); }

  _String str() const { return _M_buf.str(); }
    void str(const _String& __s) { _M_buf.str(__s); }

private:
  basic_stringbuf<_CharT, _Traits, _Alloc> _M_buf;

#if defined (_STLP_MSVC) && (_STLP_MSVC >= 1300 && _STLP_MSVC <= 1310)
  typedef basic_stringstream<_CharT, _Traits> _Self;
  
  basic_stringstream(_Self const&);
  _Self& operator = (_Self const&);
#endif
};


#if defined (_STLP_USE_TEMPLATE_EXPORT)
_STLP_EXPORT_TEMPLATE_CLASS basic_istringstream<char, char_traits<char>, allocator<char> >;
_STLP_EXPORT_TEMPLATE_CLASS basic_ostringstream<char, char_traits<char>, allocator<char> >;
_STLP_EXPORT_TEMPLATE_CLASS basic_stringstream<char, char_traits<char>, allocator<char> >;
#  if !defined (_STLP_NO_WCHAR_T)
_STLP_EXPORT_TEMPLATE_CLASS basic_istringstream<wchar_t, char_traits<wchar_t>, allocator<wchar_t>  >;
_STLP_EXPORT_TEMPLATE_CLASS basic_ostringstream<wchar_t, char_traits<wchar_t>, allocator<wchar_t>  >;
_STLP_EXPORT_TEMPLATE_CLASS basic_stringstream<wchar_t, char_traits<wchar_t>, allocator<wchar_t>  >;
#  endif
#endif 

_STLP_END_NAMESPACE

#if defined (_STLP_EXPOSE_STREAM_IMPLEMENTATION) && !defined (_STLP_LINK_TIME_INSTANTIATION)
#  include <stl/_sstream.c>
#endif

#endif 




