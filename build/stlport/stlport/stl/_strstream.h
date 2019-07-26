
















#ifndef _STLP_INTERNAL_STRSTREAM
#define _STLP_INTERNAL_STRSTREAM

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

#ifndef _STLP_USE_NAMESPACES
#  define strstream _STLP_strstream
#  define ostrstream _STLP_ostrstream
#  define istrstream _STLP_istrstream
#  define strstreambuf _STLP_strstreambuf
#endif





class _STLP_CLASS_DECLSPEC strstreambuf : public basic_streambuf<char, char_traits<char> > {
public:                         
  typedef char_traits<char>              _Traits;
  typedef basic_streambuf<char, char_traits<char> > _Base;
  typedef void* (*__alloc_fn)(size_t);
  typedef void (*__free_fn)(void*);
public:                         

  explicit strstreambuf(streamsize _Initial_capacity = 0);

  strstreambuf(__alloc_fn, __free_fn);

  strstreambuf(char* __get, streamsize __n, char* __put = 0);
  strstreambuf(signed char* __get, streamsize __n, signed char* __put = 0);
  strstreambuf(unsigned char* __get, streamsize __n, unsigned char* __put=0);

  strstreambuf(const char* __get, streamsize __n);
  strstreambuf(const signed char* __get, streamsize __n);
  strstreambuf(const unsigned char* __get, streamsize __n);

  virtual ~strstreambuf();

public:                         
  void freeze(bool = true);
  char* str();
  int pcount() const;

protected:                      
  virtual int_type overflow(int_type __c  = _Traits::eof());
  virtual int_type pbackfail(int_type __c = _Traits::eof());
  virtual int_type underflow();
  virtual _Base* setbuf(char* __buf, streamsize __n);
  virtual pos_type seekoff(off_type __off, ios_base::seekdir __dir,
                           ios_base::openmode __mode
                                      = ios_base::in | ios_base::out);
  virtual pos_type seekpos(pos_type __pos, ios_base::openmode __mode
                                      = ios_base::in | ios_base::out);

private:                        
  
  char* _M_alloc(size_t);
  void  _M_free(char*);

  
  void _M_setup(char* __get, char* __put, streamsize __n);
private:                        
  __alloc_fn _M_alloc_fun;
  __free_fn  _M_free_fun;
  bool _M_dynamic  : 1;
  bool _M_frozen   : 1;
  bool _M_constant : 1;
};




class _STLP_CLASS_DECLSPEC istrstream : public basic_istream<char, char_traits<char> > {
public:
  explicit istrstream(char*);
  explicit istrstream(const char*);
  istrstream(char* , streamsize);
  istrstream(const char*, streamsize);
  virtual ~istrstream();

  strstreambuf* rdbuf() const;
  char* str();

private:
  strstreambuf _M_buf;
};




class _STLP_CLASS_DECLSPEC ostrstream : public basic_ostream<char, char_traits<char> >
{
public:
  ostrstream();
  ostrstream(char*, int, ios_base::openmode = ios_base::out);
  virtual ~ostrstream();

  strstreambuf* rdbuf() const;
  void freeze(bool = true);
  char* str();
  int pcount() const;

private:
  strstreambuf _M_buf;
};




class _STLP_CLASS_DECLSPEC strstream : public basic_iostream<char, char_traits<char> > {
public:
  typedef char                        char_type;
  typedef char_traits<char>::int_type int_type;
  typedef char_traits<char>::pos_type pos_type;
  typedef char_traits<char>::off_type off_type;

  strstream();
  strstream(char*, int, ios_base::openmode = ios_base::in | ios_base::out);
  virtual ~strstream();

  strstreambuf* rdbuf() const;
  void freeze(bool = true);
  int pcount() const;
  char* str();

private:
  strstreambuf _M_buf;

  
  strstream(strstream const&);
  strstream& operator = (strstream const&);
};

_STLP_END_NAMESPACE

#endif 
