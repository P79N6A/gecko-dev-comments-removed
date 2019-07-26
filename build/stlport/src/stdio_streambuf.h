





































#ifndef _STLP_STDIO_STREAMBUF
#define _STLP_STDIO_STREAMBUF

#include <streambuf>
#include <cstdio>              

_STLP_BEGIN_NAMESPACE
_STLP_MOVE_TO_PRIV_NAMESPACE


class stdio_streambuf_base :
  public basic_streambuf<char, char_traits<char> >  {
public:                         
  
  stdio_streambuf_base(FILE*);

  
  ~stdio_streambuf_base();

protected:                      
  streambuf* setbuf(char*, streamsize);

  pos_type seekoff(off_type, ios_base::seekdir,
                   ios_base::openmode
                          = ios_base::in | ios_base::out);
  pos_type seekpos(pos_type,
                   ios_base::openmode
                          = ios_base::in | ios_base::out);
  int sync();

protected:
  FILE* _M_file;
};

class stdio_istreambuf : public stdio_streambuf_base {
public:                         
  stdio_istreambuf(FILE* __f) : stdio_streambuf_base(__f) {}
  ~stdio_istreambuf();

protected:                      
  streamsize showmanyc();
  int_type underflow();
  int_type uflow();
  virtual int_type pbackfail(int_type c = traits_type::eof());
};

class stdio_ostreambuf : public stdio_streambuf_base {
public:                         
  stdio_ostreambuf(FILE* __f) : stdio_streambuf_base(__f) {}
  ~stdio_ostreambuf();

protected:                      
  streamsize showmanyc();
  int_type overflow(int_type c = traits_type::eof());
};

_STLP_MOVE_TO_STD_NAMESPACE
_STLP_END_NAMESPACE

#endif 




