

















#ifndef _STLP_STDIO_FILE_H
#define _STLP_STDIO_FILE_H





#ifndef _STLP_CSTDIO
#  include <cstdio>
#endif
#ifndef _STLP_CSTDDEF
#  include <cstddef>
#endif

#if defined (__MSL__)
#  include <unix.h>  
#endif

_STLP_BEGIN_NAMESPACE

#if defined (_STLP_WCE)

inline int _FILE_fd(const FILE *__f) {
  








  for (int __fd = 0; __fd != 3; ++__fd) {
    if (__f == _getstdfilex(__fd))
      return __fd;
  }

  
  return (int)::_fileno((FILE*)__f); 
}

# elif defined (_STLP_SCO_OPENSERVER) || defined (__NCR_SVR)

inline int _FILE_fd(const FILE *__f) { return __f->__file; }

# elif defined (__sun) && defined (_LP64)

inline int _FILE_fd(const FILE *__f) { return (int) __f->__pad[2]; }

#elif defined (__hpux)  || \
      defined (__MVS__) || \
      defined (_STLP_USE_UCLIBC) 

inline int _FILE_fd(const FILE *__f) { return fileno(__CONST_CAST(FILE*, __f)); }

#elif defined (_STLP_USE_GLIBC)

inline int _FILE_fd(const FILE *__f) { return __f->_fileno; }

#elif defined (__BORLANDC__)

inline int _FILE_fd(const FILE *__f) { return __f->fd; }

#elif defined (__MWERKS__)







#  if __dest_os == __mac_os
inline int _FILE_fd(const FILE *__f) { return ::fileno(__CONST_CAST(FILE*, __f)); }
#  else
inline int _FILE_fd(const FILE *__f) { return ::_fileno(__CONST_CAST(FILE*, __f)); }
#  endif

#elif defined (__QNXNTO__) || defined (__WATCOMC__) || defined (__EMX__)

inline int _FILE_fd(const FILE *__f) { return __f->_handle; }

#elif defined (__Lynx__)


inline int _FILE_fd(const FILE *__f) { return __f->_fd; }

#else  

inline int _FILE_fd(const FILE *__f) { return __f->_file; }

#endif

_STLP_END_NAMESPACE

#endif 




