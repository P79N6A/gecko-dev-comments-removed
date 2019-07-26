

















#if defined  (__SUNPPRO_CC)  && !defined (_STLP_NO_NEW_C_HEADERS)
#  include <time.h>

#endif

#include <fstream>

#ifdef __CYGWIN__
#  define __int64 long long
#endif

#include <cstdio>
#if !defined(__ISCPP__)
extern "C" {
#  include <sys/stat.h>
}
#endif

#if defined( __MSL__ )
#  include <unix.h>
#endif

#if defined(__ISCPP__)
#  include <c_locale_is/filestat.h>
#endif

#if defined(__BEOS__) && defined(__INTEL__)
#  include <fcntl.h>
#  include <sys/stat.h>         
#endif

#if defined (_STLP_MSVC) || defined (__MINGW32__)
#  include <fcntl.h>
#  define S_IREAD _S_IREAD
#  define S_IWRITE _S_IWRITE
#  define S_IFREG _S_IFREG
     
#  ifndef S_IRUSR
#    define S_IRUSR _S_IREAD
#    define S_IWUSR _S_IWRITE
#  endif
#  ifndef S_IRGRP
#    define S_IRGRP _S_IREAD
#    define S_IWGRP _S_IWRITE
#  endif
#  ifndef S_IROTH
#    define S_IROTH _S_IREAD
#    define S_IWOTH _S_IWRITE
#  endif

#  ifndef O_RDONLY
#    define O_RDONLY _O_RDONLY
#    define O_WRONLY _O_WRONLY
#    define O_RDWR   _O_RDWR
#    define O_APPEND _O_APPEND
#    define O_CREAT  _O_CREAT
#    define O_TRUNC  _O_TRUNC
#    define O_TEXT   _O_TEXT
#    define O_BINARY _O_BINARY
#  endif

#  ifndef O_ACCMODE
#    define O_ACCMODE (O_RDONLY|O_WRONLY|O_RDWR)
#  endif
#endif

const _STLP_fd INVALID_STLP_FD = -1;


#  ifdef __MSL__
#    define _O_TEXT 0x0
#    if !defined( O_TEXT )
#      define O_TEXT _O_TEXT
#    endif
#    define _S_IFREG S_IFREG
#    define S_IREAD        S_IRUSR
#    define S_IWRITE       S_IWUSR
#    define S_IEXEC        S_IXUSR
#    define _S_IWRITE S_IWRITE
#    define _S_IREAD S_IREAD
#    define _open open
#    define _close close
#    define _read read
#    define _write write
#  endif

_STLP_BEGIN_NAMESPACE



#if defined (_STLP_USE_DEFAULT_FILE_OFFSET) || \
    (!defined(_LARGEFILE_SOURCE) && !defined(_LARGEFILE64_SOURCE))
#  define FOPEN fopen
#  define FSEEK fseek
#  define FSTAT fstat
#  define STAT  stat
#  define FTELL ftell
#else
#  define FOPEN fopen64
#  define FSEEK fseeko64
#  define FSTAT fstat64
#  define STAT  stat64
#  define FTELL ftello64
#endif

_STLP_MOVE_TO_PRIV_NAMESPACE



static bool __is_regular_file(_STLP_fd fd) {
  struct STAT buf;
  return FSTAT(fd, &buf) == 0 && (buf.st_mode & S_IFREG) != 0 ;
}


static streamoff __file_size(_STLP_fd fd) {
  streamoff ret = 0;

  struct STAT buf;
  if (FSTAT(fd, &buf) == 0 && (buf.st_mode & S_IFREG) != 0)
    ret = buf.st_size > 0 ? buf.st_size : 0;

  return ret;
}

_STLP_MOVE_TO_STD_NAMESPACE






size_t _Filebuf_base::_M_page_size = 4096;

_Filebuf_base::_Filebuf_base()
  : _M_file_id(INVALID_STLP_FD),
    _M_openmode(0),
    _M_is_open(false),
    _M_should_close(false)
{}

void _Filebuf_base::_S_initialize()
{

}



streamoff _Filebuf_base::_M_file_size()
{
  return _STLP_PRIV __file_size(_M_file_id);
}

bool _Filebuf_base::_M_open(const char* name, ios_base::openmode openmode,
                            long permission)
{
  _STLP_fd file_no;

  if (_M_is_open)
    return false;

  
  const char* flags;

  switch (openmode & (~ios_base::ate)) {
    case ios_base::out:
    case ios_base::out | ios_base::trunc:
      flags = "w";
      break;

    case ios_base::out | ios_base::binary:
    case ios_base::out | ios_base::trunc | ios_base::binary:
      flags = "wb";
      break;

    case ios_base::out | ios_base::app:
      flags = "a";
      break;

    case ios_base::out | ios_base::app | ios_base::binary:
      flags = "ab";
      break;

    case ios_base::in:
      flags = "r";
      break;

    case ios_base::in | ios_base::binary:
      flags = "rb";
      break;

    case ios_base::in | ios_base::out:
      flags = "r+";
      break;

    case ios_base::in | ios_base::out | ios_base::binary:
      flags = "r+b";
      break;

    case ios_base::in | ios_base::out | ios_base::trunc:
      flags = "w+";
      break;

    case ios_base::in | ios_base::out | ios_base::trunc | ios_base::binary:
      flags = "w+b";
      break;

    default:                      
      return false;               
  }

  
  (void)permission; 
  _M_file = FOPEN(name, flags);

  if (_M_file) {
    file_no = fileno(_M_file);
  } else {
    return false;
  }

  
  setbuf(_M_file, 0);

  _M_is_open = true;

  if (openmode & ios_base::ate) {
    if (FSEEK(_M_file, 0, SEEK_END) != 0)
      _M_is_open = false;
  }

  _M_file_id = file_no;
  _M_should_close = _M_is_open;
  _M_openmode = openmode;

  if (_M_is_open)
    _M_regular_file = _STLP_PRIV __is_regular_file(_M_file_id);

  return (_M_is_open != 0);
}


bool _Filebuf_base::_M_open(const char* name, ios_base::openmode openmode)
{
  
  
  
  return this->_M_open(name, openmode, S_IRUSR | S_IWUSR | S_IRGRP |
                                       S_IWGRP | S_IROTH | S_IWOTH);
}




bool _Filebuf_base::_M_open( int file_no, ios_base::openmode )
{
  if (_M_is_open || file_no < 0)
    return false;

  struct STAT buf;
  if (FSTAT(file_no, &buf) != 0)
    return false;
  int mode = buf.st_mode;

  switch ( mode & (S_IWRITE | S_IREAD) ) {
    case S_IREAD:
      _M_openmode = ios_base::in;
      break;
    case S_IWRITE:
      _M_openmode = ios_base::out;
      break;
    case (S_IWRITE | S_IREAD):
      _M_openmode = ios_base::in | ios_base::out;
      break;
    default:
      return false;
  }
  _M_file_id = file_no;
  _M_is_open = true;
  _M_should_close = false;
  _M_regular_file = _STLP_PRIV __is_regular_file(_M_file_id);
  return true;
}

bool _Filebuf_base::_M_close()
{
  if (!_M_is_open)
    return false;

  bool ok = _M_should_close ? (fclose(_M_file) == 0) : true;

  _M_is_open = _M_should_close = false;
  _M_openmode = 0;
  return ok;
}



ptrdiff_t _Filebuf_base::_M_read(char* buf, ptrdiff_t n) {
  return fread(buf, 1, n, _M_file);
}



bool _Filebuf_base::_M_write(char* buf, ptrdiff_t n)
{
  for (;;) {
    ptrdiff_t written = fwrite(buf, 1, n, _M_file);

    if (n == written) {
      return true;
    }

    if (written > 0 && written < n) {
      n -= written;
      buf += written;
    } else {
      return false;
    }
  }
}


streamoff _Filebuf_base::_M_seek(streamoff offset, ios_base::seekdir dir)
{
  int whence;

  switch ( dir ) {
    case ios_base::beg:
      if (offset < 0  )
        return streamoff(-1);
      whence = SEEK_SET;
      break;
    case ios_base::cur:
      whence = SEEK_CUR;
      break;
    case ios_base::end:
      if (  -offset > _M_file_size() )
        return streamoff(-1);
      whence = SEEK_END;
      break;
    default:
      return streamoff(-1);
  }

  if ( FSEEK(_M_file, offset, whence) == 0 ) {
    return FTELL(_M_file);
  }

  return streamoff(-1);
}







void *_Filebuf_base::_M_mmap(streamoff, streamoff )
{
  return 0;
}

void _Filebuf_base::_M_unmap(void*, streamoff)
{
  
}

_STLP_END_NAMESPACE
