
















#ifndef _STLP_INTERNAL_STREAMBUF
#define _STLP_INTERNAL_STREAMBUF

#ifndef _STLP_IOS_BASE_H
#  include <stl/_ios_base.h>      
#endif                            

_STLP_BEGIN_NAMESPACE





















template <class _CharT, class _Traits>
class basic_streambuf {
  friend class basic_istream<_CharT, _Traits>;
  friend class basic_ostream<_CharT, _Traits>;

public:                         
  typedef _CharT                     char_type;
  typedef typename _Traits::int_type int_type;
  typedef typename _Traits::pos_type pos_type;
  typedef typename _Traits::off_type off_type;
  typedef _Traits                    traits_type;

private:                        

  char_type* _M_gbegin;         
  char_type* _M_gnext;          
  char_type* _M_gend;           

  char_type* _M_pbegin;         
  char_type* _M_pnext;          
  char_type* _M_pend;           

  locale _M_locale;             

public:                         
  virtual ~basic_streambuf();

protected:                      
  basic_streambuf()
#if defined (_STLP_MSVC) && (_STLP_MSVC < 1300) && defined (_STLP_USE_STATIC_LIB)
    
    : _M_gbegin(0), _M_gnext(0), _M_gend(0),
      _M_pbegin(0), _M_pnext(0), _M_pend(0),
      _M_locale()
  {}
#else
  ;
#endif

protected:                      
  char_type* eback() const { return _M_gbegin; } 
  char_type* gptr()  const { return _M_gnext; }  
  char_type* egptr() const { return _M_gend; }   

  void gbump(int __n) { _M_gnext += __n; }
  void setg(char_type* __gbegin, char_type* __gnext, char_type* __gend) {
    _M_gbegin = __gbegin;
    _M_gnext  = __gnext;
    _M_gend   = __gend;
  }

public:
  
  
  
  char_type* _M_eback() const { return eback(); }
  char_type* _M_gptr()  const { return gptr(); }
  char_type* _M_egptr() const { return egptr(); }
  void _M_gbump(int __n)      { gbump(__n); }
  void _M_setg(char_type* __gbegin, char_type* __gnext, char_type* __gend)
  { this->setg(__gbegin, __gnext, __gend); }

protected:                      

  char_type* pbase() const { return _M_pbegin; } 
  char_type* pptr()  const { return _M_pnext; }  
  char_type* epptr() const { return _M_pend; }   

  void pbump(int __n) { _M_pnext += __n; }
  void setp(char_type* __pbegin, char_type* __pend) {
    _M_pbegin = __pbegin;
    _M_pnext  = __pbegin;
    _M_pend   = __pend;
  }

protected:                      

  virtual basic_streambuf<_CharT, _Traits>* setbuf(char_type*, streamsize);

  
  
  virtual pos_type seekoff(off_type, ios_base::seekdir,
                           ios_base::openmode = ios_base::in | ios_base::out);

  
  
  virtual pos_type
  seekpos(pos_type, ios_base::openmode = ios_base::in | ios_base::out);

  
  
  virtual int sync();


public:                         
  basic_streambuf<_CharT, _Traits>* pubsetbuf(char_type* __s, streamsize __n)
  { return this->setbuf(__s, __n); }

  pos_type pubseekoff(off_type __offset, ios_base::seekdir __way,
                      ios_base::openmode __mod = ios_base::in | ios_base::out)
  { return this->seekoff(__offset, __way, __mod); }

  pos_type pubseekpos(pos_type __sp,
                      ios_base::openmode __mod = ios_base::in | ios_base::out)
  { return this->seekpos(__sp, __mod); }

  int pubsync() { return this->sync(); }

protected:                      
                                
  
  
  
  
  virtual streamsize showmanyc();

  
  
  virtual streamsize xsgetn(char_type* __s, streamsize __n);

  
  
  
  virtual int_type underflow();

  
  
  virtual int_type uflow();

  
  
  
  virtual int_type pbackfail(int_type = traits_type::eof());

protected:                      
                                

  
  
  virtual streamsize xsputn(const char_type* __s, streamsize __n);

  
  
  virtual streamsize _M_xsputnc(char_type __c, streamsize __n);

  
  
  virtual int_type overflow(int_type = traits_type::eof());

public:                         
  
  int_type sputc(char_type __c) {
    return ((_M_pnext < _M_pend) ? _Traits::to_int_type(*_M_pnext++ = __c)
      : this->overflow(_Traits::to_int_type(__c)));
  }

  
  streamsize sputn(const char_type* __s, streamsize __n)
  { return this->xsputn(__s, __n); }

  
  streamsize _M_sputnc(char_type __c, streamsize __n)
  { return this->_M_xsputnc(__c, __n); }

private:                        
  int_type _M_snextc_aux();

public:                         
  streamsize in_avail() {
    return (_M_gnext < _M_gend) ? (_M_gend - _M_gnext) : this->showmanyc();
  }

  
  int_type snextc() {
  return ( _M_gend - _M_gnext > 1 ?
             _Traits::to_int_type(*++_M_gnext) :
             this->_M_snextc_aux());
  }

  
  int_type sbumpc() {
    return _M_gnext < _M_gend ? _Traits::to_int_type(*_M_gnext++)
      : this->uflow();
  }

  
  int_type sgetc() {
    return _M_gnext < _M_gend ? _Traits::to_int_type(*_M_gnext)
      : this->underflow();
  }

  streamsize sgetn(char_type* __s, streamsize __n)
  { return this->xsgetn(__s, __n); }

  int_type sputbackc(char_type __c) {
    return ((_M_gbegin < _M_gnext) && _Traits::eq(__c, *(_M_gnext - 1)))
      ? _Traits::to_int_type(*--_M_gnext)
      : this->pbackfail(_Traits::to_int_type(__c));
  }

  int_type sungetc() {
    return (_M_gbegin < _M_gnext)
      ? _Traits::to_int_type(*--_M_gnext)
      : this->pbackfail();
  }

protected:                      

  
  
  
  
  virtual void imbue(const locale&);

public:                         
  locale pubimbue(const locale&);
  locale getloc() const { return _M_locale; }

#if !defined (_STLP_NO_ANACHRONISMS)
  void stossc() { this->sbumpc(); }
#endif
};

#if defined (_STLP_USE_TEMPLATE_EXPORT)
_STLP_EXPORT_TEMPLATE_CLASS basic_streambuf<char, char_traits<char> >;
#  if !defined (_STLP_NO_WCHAR_T)
_STLP_EXPORT_TEMPLATE_CLASS basic_streambuf<wchar_t, char_traits<wchar_t> >;
#  endif 
#endif 

_STLP_END_NAMESPACE

#if defined (_STLP_EXPOSE_STREAM_IMPLEMENTATION) && !defined (_STLP_LINK_TIME_INSTANTIATION)
#  include <stl/_streambuf.c>
#endif

#endif




