

















#if defined  (__SUNPPRO_CC)  && !defined (_STLP_NO_NEW_C_HEADERS)
#  include <time.h>

#endif

#include <fstream>

#ifdef __CYGWIN__
#  define __int64 long long
#endif

extern "C" {

#include <sys/stat.h>           
#if !defined (_CRAY) && ! defined (__EMX__)
#  include <sys/mman.h>           
#endif


#if defined (__hpux) && defined (__GNUC__)
#  undef _INCLUDE_POSIX1C_SOURCE
#endif

#include <unistd.h>
#include <fcntl.h>
}

#ifdef __APPLE__
#  include <sys/sysctl.h>
#endif

const _STLP_fd INVALID_STLP_FD = -1;

#ifndef O_ACCMODE
#  define O_ACCMODE (O_RDONLY|O_WRONLY|O_RDWR)
#endif


#if defined (_STLP_USE_DEFAULT_FILE_OFFSET) || \
    (!defined(_LARGEFILE_SOURCE) && !defined (_LARGEFILE64_SOURCE))
#  define FSTAT fstat
#  define STAT  stat
#  define LSEEK lseek
#  define MMAP  mmap
#  define OPEN  open
#else
#  define FSTAT fstat64
#  define STAT  stat64
#  define LSEEK lseek64
#  define MMAP  mmap64
#  define OPEN  open64
#endif

#ifndef MAP_FAILED 
#  define MAP_FAILED -1
#endif

_STLP_BEGIN_NAMESPACE

static ios_base::openmode flag_to_openmode(int mode)
{
  ios_base::openmode ret = ios_base::__default_mode;

  switch ( mode & O_ACCMODE ) {
    case O_RDONLY:
      ret = ios_base::in;
      break;
    case O_WRONLY:
      ret = ios_base::out;
      break;
    case O_RDWR:
      ret = ios_base::in | ios_base::out;
      break;
  }

  if ( mode & O_APPEND )
    ret |= ios_base::app;

  return ret;
}

_STLP_MOVE_TO_PRIV_NAMESPACE



static bool __is_regular_file(_STLP_fd fd) {
  struct STAT buf;
  return FSTAT(fd, &buf) == 0 && S_ISREG(buf.st_mode);
}


static streamoff __file_size(_STLP_fd fd) {
  streamoff ret = 0;

  struct STAT buf;
  if (FSTAT(fd, &buf) == 0 && S_ISREG(buf.st_mode))
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
#if defined (__APPLE__)
  int mib[2];
  size_t pagesize, len;
  mib[0] = CTL_HW;
  mib[1] = HW_PAGESIZE;
  len = sizeof(pagesize);
  sysctl(mib, 2, &pagesize, &len, NULL, 0);
  _M_page_size = pagesize;
#elif defined (__DJGPP) && defined (_CRAY)
  _M_page_size = BUFSIZ;
#else
  _M_page_size = sysconf(_SC_PAGESIZE);
#endif
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

  int flags = 0;

  
  switch ( openmode & (~ios_base::ate & ~ios_base::binary) ) {
    case ios_base::out:
    case ios_base::out | ios_base::trunc:
      flags = O_WRONLY | O_CREAT | O_TRUNC;
      break;
    case ios_base::app:
    case ios_base::out | ios_base::app:
      flags = O_WRONLY | O_CREAT | O_APPEND;
      break;
    case ios_base::in:
      flags = O_RDONLY;
      permission = 0;             
      break;
    case ios_base::in | ios_base::out:
      flags = O_RDWR;
      break;
    case ios_base::in | ios_base::out | ios_base::trunc:
      flags = O_RDWR | O_CREAT | O_TRUNC;
      break;
    case ios_base::in | ios_base::app:
    case ios_base::in | ios_base::out | ios_base::app:
      flags = O_RDWR | O_CREAT | O_APPEND;
      break;
    default:                      
      return false;               
  }

  file_no = OPEN(name, flags, permission);

  if (file_no < 0)
    return false;

  _M_is_open = true;

  if ((openmode & (ios_base::ate | ios_base::app)) && (LSEEK(file_no, 0, SEEK_END) == -1)) {
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




bool _Filebuf_base::_M_open(int file_no, ios_base::openmode)
{
  if (_M_is_open || file_no < 0)
    return false;

  int mode = fcntl(file_no, F_GETFL);

  if (mode == -1)
    return false;

  _M_openmode = flag_to_openmode(mode);
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

  bool ok = _M_should_close ? (close(_M_file_id) == 0) : true;

  _M_is_open = _M_should_close = false;
  _M_openmode = 0;
  return ok;
}



ptrdiff_t _Filebuf_base::_M_read(char* buf, ptrdiff_t n)
{
  return read(_M_file_id, buf, n);
}



bool _Filebuf_base::_M_write(char* buf, ptrdiff_t n)
{
  for (;;) {
    ptrdiff_t written = write(_M_file_id, buf, n);

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

  return LSEEK(_M_file_id, offset, whence);
}






void* _Filebuf_base::_M_mmap(streamoff offset, streamoff len)
{
  void* base;
#if !defined (__DJGPP) && !defined (_CRAY)
  base = MMAP(0, len, PROT_READ, MAP_PRIVATE, _M_file_id, offset);
  if (base != (void*)MAP_FAILED) {
    if (LSEEK(_M_file_id, offset + len, SEEK_SET) < 0) {
      this->_M_unmap(base, len);
      base = 0;
    }
  } else
    base =0;
#else
  _STLP_MARK_PARAMETER_AS_UNUSED(&offset)
  _STLP_MARK_PARAMETER_AS_UNUSED(&len)
  base = 0;
#endif
  return base;
}

void _Filebuf_base::_M_unmap(void* base, streamoff len)
{
  
#if !defined (__DJGPP) && !defined (_CRAY)
  munmap((char*)base, len);
#else
  _STLP_MARK_PARAMETER_AS_UNUSED(&len)
  _STLP_MARK_PARAMETER_AS_UNUSED(base)
#endif
}

_STLP_END_NAMESPACE
