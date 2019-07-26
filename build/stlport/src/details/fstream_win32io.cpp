

















#include <fstream>

#if !defined (_STLP_WCE)
#  ifdef __BORLANDC__
#    include <cfcntl.h>           
#  else
#    include <io.h>               
#    include <fcntl.h>            
#  endif
#  include <sys/stat.h>         
#endif

#define _TEXTBUF_SIZE 0x1000

const _STLP_fd INVALID_STLP_FD = INVALID_HANDLE_VALUE;

#if !defined (INVALID_SET_FILE_POINTER)
#  define INVALID_SET_FILE_POINTER 0xffffffff
#endif

#ifndef O_ACCMODE
#  define O_ACCMODE (O_RDONLY|O_WRONLY|O_RDWR)
#endif

_STLP_BEGIN_NAMESPACE

#if !defined(__MSL__) && !defined(_STLP_WCE)
static ios_base::openmode flag_to_openmode(int mode) {
  ios_base::openmode ret = ios_base::__default_mode;

  switch (mode & O_ACCMODE) {
  case O_RDONLY:
    ret = ios_base::in; break;
  case O_WRONLY:
    ret = ios_base::out; break;
  case O_RDWR:
    ret = ios_base::in | ios_base::out; break;
  }

  if (mode & O_APPEND)
    ret |= ios_base::app;

  if (mode & O_BINARY)
    ret |= ios_base::binary;

  return ret;
}
#endif

_STLP_MOVE_TO_PRIV_NAMESPACE



static bool __is_regular_file(_STLP_fd fd) {
  BY_HANDLE_FILE_INFORMATION info;

  
  return GetFileInformationByHandle(fd, &info) && 
         ((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0);
}


static streamoff __file_size(_STLP_fd fd) {
  streamoff ret = 0;

 LARGE_INTEGER li;
 li.LowPart = GetFileSize(fd, (unsigned long*) &li.HighPart);
 if (li.LowPart != INVALID_FILE_SIZE || GetLastError() == NO_ERROR)
   ret = li.QuadPart;

  return ret;
}

_STLP_MOVE_TO_STD_NAMESPACE



#if (defined (_STLP_MSVC_LIB) && !defined (_STLP_WCE)) || \
    (defined (__MINGW32__) && defined (__MSVCRT__))



#  define IOINFO_L2E          5
#  define IOINFO_ARRAY_ELTS   (1 << IOINFO_L2E)
#  define _pioinfo(i) ( __pioinfo[(i) >> IOINFO_L2E] + \
              ((i) & (IOINFO_ARRAY_ELTS - 1)) )
#  define FAPPEND         0x20    // O_APPEND flag
#  define FTEXT           0x80    // O_TEXT flag



extern "C" {
  struct ioinfo {
    long osfhnd;    
    char osfile;    
    char pipech;    
#  if defined (_MT)
    
    int lockinitflag;
    CRITICAL_SECTION lock;
#  endif
  };
#  if defined (__MINGW32__)
 __MINGW_IMPORT ioinfo * __pioinfo[];
#  else
  extern _CRTIMP ioinfo * __pioinfo[];
#  endif
} 


static ios_base::openmode _get_osfflags(int fd, HANDLE oshandle) {
  char dosflags = 0;
  if (fd >= 0)
    dosflags = _pioinfo(fd)->osfile;
  
    
  

  int mode = 0;
  if (dosflags & FAPPEND)
    mode |= O_APPEND;

  if (dosflags & FTEXT)
    mode |= O_TEXT;
  else
    mode |= O_BINARY;

  
  DWORD dummy, dummy2;
  BOOL writeOk = WriteFile(oshandle, &dummy2, 0, &dummy, 0);
  BOOL readOk = ReadFile(oshandle, &dummy2, 0, &dummy, NULL);
  if (writeOk && readOk)
    mode |= O_RDWR;
  else if (readOk)
    mode |= O_RDONLY;
  else
    mode |= O_WRONLY;

  return flag_to_openmode(mode);
}

#elif defined (__DMC__)

#  define FHND_APPEND 0x04
#  define FHND_DEVICE 0x08
#  define FHND_TEXT   0x10

extern "C" unsigned char __fhnd_info[_NFILE];

static ios_base::openmode _get_osfflags(int fd, HANDLE oshandle) {
  int mode = 0;

  if (__fhnd_info[fd] & FHND_APPEND)
    mode |= O_APPEND;

  if (__fhnd_info[fd] & FHND_TEXT == 0)
    mode |= O_BINARY;

  for (FILE *fp = &_iob[0]; fp < &_iob[_NFILE]; fp++) {
    if ((fileno(fp) == fd) && (fp->_flag & (_IOREAD | _IOWRT | _IORW))) {
      const int osflags = fp->_flag;

      if ((osflags & _IOREAD) && !(osflags & _IOWRT) && !(osflags & _IORW))
        mode |= O_RDONLY;
      else if ((osflags & _IOWRT) && !(osflags & _IOREAD) && !(osflags & _IORW))
        mode |= O_WRONLY;
      else
        mode |= O_RDWR;
      break;
    }
  }

  return flag_to_openmode(mode);
}
#endif

size_t _Filebuf_base::_M_page_size = 4096;

_Filebuf_base::_Filebuf_base()
  : _M_file_id(INVALID_STLP_FD),
    _M_openmode(0),
    _M_is_open(false),
    _M_should_close(false),
    _M_view_id(0)
{}

void _Filebuf_base::_S_initialize() {
  SYSTEM_INFO SystemInfo;
  GetSystemInfo(&SystemInfo);
  _M_page_size = SystemInfo.dwPageSize;
  
}



streamoff _Filebuf_base::_M_file_size() {
  return _STLP_PRIV __file_size(_M_file_id);
}

bool _Filebuf_base::_M_open(const char* name, ios_base::openmode openmode,
                            long permission) {
  _STLP_fd file_no;

  if (_M_is_open)
    return false;

  DWORD dwDesiredAccess, dwCreationDisposition;
  bool doTruncate = false;

  switch (openmode & (~ios_base::ate & ~ios_base::binary)) {
  case ios_base::out:
  case ios_base::out | ios_base::trunc:
    dwDesiredAccess = GENERIC_WRITE;
    dwCreationDisposition = OPEN_ALWAYS;
    
    
    doTruncate = true;
    break;
  case ios_base::out | ios_base::app:
    dwDesiredAccess = GENERIC_WRITE;
    dwCreationDisposition = OPEN_ALWAYS;
    break;
  case ios_base::in:
    dwDesiredAccess = GENERIC_READ;
    dwCreationDisposition = OPEN_EXISTING;
    permission = 0;             
    break;
  case ios_base::in | ios_base::out:
    dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
    dwCreationDisposition = OPEN_EXISTING;
    break;
  case ios_base::in | ios_base::out | ios_base::trunc:
    dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
    dwCreationDisposition = OPEN_ALWAYS;
    doTruncate = true;
    break;
  default:                      
    return false;               
  }

  DWORD dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;

#if defined(_STLP_USE_WIDE_INTERFACE)
    file_no = CreateFile (_STLP_PRIV __ASCIIToWide(name).c_str(),
#else
    file_no = CreateFileA(name,
#endif
                          dwDesiredAccess, dwShareMode, 0,
                          dwCreationDisposition, permission, 0);

  if (file_no == INVALID_STLP_FD)
    return false;

  if (
#if !defined (_STLP_WCE)
      GetFileType(file_no) == FILE_TYPE_DISK &&
#endif
      ((doTruncate && SetEndOfFile(file_no) == 0) ||
       (((openmode & ios_base::ate) != 0) &&
        (SetFilePointer(file_no, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER)))) {
    CloseHandle(file_no);
    return false;
  }

  _M_is_open = true;
  _M_file_id = file_no;
  _M_should_close = _M_is_open;
  _M_openmode = openmode;

  if (_M_is_open)
    _M_regular_file = _STLP_PRIV __is_regular_file(_M_file_id);

  return (_M_is_open != 0);
}

bool _Filebuf_base::_M_open(const char* name, ios_base::openmode openmode) {
  
  
  
  return this->_M_open(name, openmode, FILE_ATTRIBUTE_NORMAL);
}

bool _Filebuf_base::_M_open(_STLP_fd __id, ios_base::openmode init_mode) {
#if (defined (_STLP_MSVC_LIB) && !defined (_STLP_WCE)) || \
    (defined (__MINGW32__) && defined (__MSVCRT__)) || defined (__DMC__)

  if (_M_is_open || __id == INVALID_STLP_FD)
    return false;

  if (init_mode != ios_base::__default_mode)
    _M_openmode = init_mode;
  else
    _M_openmode = _get_osfflags(-1, __id);

  _M_is_open = true;
  _M_file_id = __id;
  _M_should_close = false;
  _M_regular_file = _STLP_PRIV __is_regular_file(_M_file_id);

  return true;
#else
  (void)__id;
  (void)init_mode;    

  
  return false;

#endif
}




bool _Filebuf_base::_M_open(int file_no, ios_base::openmode init_mode) {
  if (_M_is_open || file_no < 0)
    return false;

#if (defined (_STLP_MSVC_LIB) && !defined (_STLP_WCE)) || \
    (defined (__MINGW32__) && defined (__MSVCRT__)) || defined (__DMC__)

  HANDLE oshandle = (HANDLE)_get_osfhandle(file_no);
  if (oshandle == INVALID_STLP_FD)
    return false;

  if (init_mode != ios_base::__default_mode)
    _M_openmode = init_mode;
  else
    _M_openmode = _get_osfflags(file_no, oshandle);

  _M_file_id = oshandle;
  _M_is_open = true;
  _M_should_close = false;
  _M_regular_file = _STLP_PRIV __is_regular_file(_M_file_id);
  return true;
#else
  _STLP_MARK_PARAMETER_AS_UNUSED(&init_mode)
  
  return false;
#endif
}

bool _Filebuf_base::_M_close() {
  if (!_M_is_open)
    return false;

  bool ok;

  if (!_M_should_close)
    ok = true;
  else {
    if (_M_file_id != INVALID_STLP_FD) {
      ok = (CloseHandle(_M_file_id) != 0);
    }
    else {
      ok = false;
    }
  }

  _M_is_open = _M_should_close = false;
  _M_openmode = 0;
  return ok;
}


#define _STLP_LF 10
#define _STLP_CR 13
#define _STLP_CTRLZ 26



ptrdiff_t _Filebuf_base::_M_read(char* buf, ptrdiff_t n) {
  ptrdiff_t readen = 0;
  
  size_t chunkSize = (min)(size_t(0xffffffff), __STATIC_CAST(size_t, n));
  
  
  
  while (__STATIC_CAST(size_t, (n - readen)) >= chunkSize) {
    DWORD numberOfBytesRead;
    ReadFile(_M_file_id, buf + readen, __STATIC_CAST(DWORD, chunkSize), &numberOfBytesRead, 0);

    if (numberOfBytesRead == 0)
      break;

    if (!(_M_openmode & ios_base::binary)) {
      
      char *to = buf + readen;
      char *from = to;
      char *last = from + numberOfBytesRead - 1;
      for (; from <= last && *from != _STLP_CTRLZ; ++from) {
        if (*from != _STLP_CR)
          *to++ = *from;
        else { 
          if (from < last) { 
            if (*(from + 1) != _STLP_LF)
              *to++ = _STLP_CR;
          }
          else { 
            char peek = ' ';
            DWORD NumberOfBytesPeeked;
            ReadFile(_M_file_id, (LPVOID)&peek, 1, &NumberOfBytesPeeked, 0);
            if (NumberOfBytesPeeked != 0) {
              if (peek != _STLP_LF) { 
                *to++ = _STLP_CR;
                if ((to < buf + n) && (peek != _STLP_CR))
                  
                  
                  *to++ = peek;
                else
                  SetFilePointer(_M_file_id, (LONG)-1, 0, FILE_CURRENT);
              }
              else {
                
                *to++ = _STLP_LF;
              }
            }
            else {
              







              *to++ = _STLP_CR;
            }
          }
        } 
      } 
      readen = to - buf;
      
      if (from <= last) { 
        SetFilePointer(_M_file_id, -(LONG)((last + 1) - from), 0, FILE_CURRENT);
        break;
      }
    }
    else
      readen += numberOfBytesRead;
  }
  return readen;
}



bool _Filebuf_base::_M_write(char* buf, ptrdiff_t n) {
  for (;;) {
    ptrdiff_t written;

    
    
    

    
    
    if (_M_openmode & ios_base::app)
      _M_seek(0, ios_base::end);

    if (_M_openmode & ios_base::binary) {
      
      size_t bytes_to_write = (size_t)n;
      DWORD NumberOfBytesWritten;
      written = 0;
      for (; bytes_to_write != 0;) {
        WriteFile(_M_file_id, buf + written,
                  __STATIC_CAST(DWORD, (min)(size_t(0xffffffff), bytes_to_write)),
                  &NumberOfBytesWritten, 0);
        if (NumberOfBytesWritten == 0)
          return false;
        bytes_to_write -= NumberOfBytesWritten;
        written += NumberOfBytesWritten;
      }
    }
    else {
      char textbuf[_TEXTBUF_SIZE + 1]; 
      char * nextblock = buf, * ptrtextbuf = textbuf;
      char * endtextbuf = textbuf + _TEXTBUF_SIZE;
      char * endblock = buf + n;
      ptrdiff_t nextblocksize = (min) (n, (ptrdiff_t)_TEXTBUF_SIZE);
      char * nextlf;

      while ( (nextblocksize > 0) &&
              (nextlf = (char *)memchr(nextblock, _STLP_LF, nextblocksize)) != 0) {
        ptrdiff_t linelength = nextlf - nextblock;
        memcpy(ptrtextbuf, nextblock, linelength);
        ptrtextbuf += linelength;
        nextblock += (linelength + 1);
        * ptrtextbuf ++ = _STLP_CR;
        * ptrtextbuf ++ = _STLP_LF;
        nextblocksize = (min) (ptrdiff_t(endblock - nextblock),
                               (max) (ptrdiff_t(0), ptrdiff_t(endtextbuf - ptrtextbuf)));
      }
      
      
      if (nextblocksize > 0) {
        memcpy(ptrtextbuf, nextblock, nextblocksize);
        ptrtextbuf += nextblocksize;
        nextblock += nextblocksize;
      }
      
      char * writetextbuf = textbuf;
      for (size_t NumberOfBytesToWrite = (size_t)(ptrtextbuf - textbuf);
           NumberOfBytesToWrite;) {
        DWORD NumberOfBytesWritten;
        WriteFile((HANDLE)_M_file_id, writetextbuf,
                  __STATIC_CAST(DWORD, (min)(size_t(0xffffffff), NumberOfBytesToWrite)),
                  &NumberOfBytesWritten, 0);
        if (!NumberOfBytesWritten) 
          return false;
        writetextbuf += NumberOfBytesWritten;
        NumberOfBytesToWrite -= NumberOfBytesWritten;
      }
      
      written = (nextblock - buf);
    }

    if (n == written)
      return true;
    else if (written > 0 && written < n) {
      n -= written;
      buf += written;
    }
    else
      return false;
  }
}


streamoff _Filebuf_base::_M_seek(streamoff offset, ios_base::seekdir dir) {
  streamoff result = -1;
  int whence;

  switch(dir) {
  case ios_base::beg:
    if (offset < 0  )
      return streamoff(-1);
    whence = FILE_BEGIN;
    break;
  case ios_base::cur:
    whence = FILE_CURRENT;
    break;
  case ios_base::end:
    if (  -offset > _M_file_size() )
      return streamoff(-1);
    whence = FILE_END;
    break;
  default:
    return streamoff(-1);
  }

  LARGE_INTEGER li;
  li.QuadPart = offset;
  li.LowPart = SetFilePointer(_M_file_id, li.LowPart, &li.HighPart, whence);
  if (li.LowPart != INVALID_SET_FILE_POINTER || GetLastError() == NO_ERROR)
    result = li.QuadPart;

  return result;
}







void* _Filebuf_base::_M_mmap(streamoff offset, streamoff len) {
  void* base;
  _M_view_id = CreateFileMapping(_M_file_id, (PSECURITY_ATTRIBUTES)0 ,
                                 PAGE_READONLY, 0  ,
                                 0  , 
                                 0);

  if (_M_view_id) {
#if 0





#endif
    LARGE_INTEGER li;
    li.QuadPart = offset;
    base = MapViewOfFile(_M_view_id, FILE_MAP_READ, li.HighPart, li.LowPart,
#if !defined (__DMC__)
                         __STATIC_CAST(SIZE_T, len));
#else
                         __STATIC_CAST(DWORD, len));
#endif

    if (base == 0  || _M_seek(offset + len, ios_base::beg) < 0) {
      this->_M_unmap(base, len);
      base = 0;
    }
  } else
    base = 0;

  return base;
}

void _Filebuf_base::_M_unmap(void* base, streamoff len) {
  
  if (base != NULL)
    UnmapViewOfFile(base);
  
  if (_M_view_id != NULL)
    CloseHandle(_M_view_id);
  _M_view_id = NULL;
  (void)len; 
}

_STLP_END_NAMESPACE
