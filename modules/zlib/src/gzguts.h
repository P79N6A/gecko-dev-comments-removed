




#ifdef _LARGEFILE64_SOURCE
#  ifndef _LARGEFILE_SOURCE
#    define _LARGEFILE_SOURCE 1
#  endif
#  ifdef _FILE_OFFSET_BITS
#    undef _FILE_OFFSET_BITS
#  endif
#endif

#if ((__GNUC__-0) * 10 + __GNUC_MINOR__-0 >= 33) && !defined(NO_VIZ)
#  define ZLIB_INTERNAL __attribute__((visibility ("hidden")))
#else
#  define ZLIB_INTERNAL
#endif

#include <stdio.h>
#include "zlib.h"
#ifdef STDC
#  include <string.h>
#  include <stdlib.h>
#  include <limits.h>
#endif
#include <fcntl.h>

#ifdef NO_DEFLATE       
#  define NO_GZCOMPRESS
#endif

#ifdef _MSC_VER
#  include <io.h>
#  define vsnprintf _vsnprintf
#endif

#ifndef local
#  define local static
#endif



#ifndef STDC
  extern voidp  malloc OF((uInt size));
  extern void   free   OF((voidpf ptr));
#endif


#if defined UNDER_CE
#  include <windows.h>
#  define zstrerror() gz_strwinerror((DWORD)GetLastError())
#else
#  ifdef STDC
#    include <errno.h>
#    define zstrerror() strerror(errno)
#  else
#    define zstrerror() "stdio error (consult errno)"
#  endif
#endif


#if !defined(_LARGEFILE64_SOURCE) || _LFS64_LARGEFILE-0 == 0
    ZEXTERN gzFile ZEXPORT gzopen64 OF((const char *, const char *));
    ZEXTERN z_off64_t ZEXPORT gzseek64 OF((gzFile, z_off64_t, int));
    ZEXTERN z_off64_t ZEXPORT gztell64 OF((gzFile));
    ZEXTERN z_off64_t ZEXPORT gzoffset64 OF((gzFile));
#endif


#define GZBUFSIZE 8192


#define GZ_NONE 0
#define GZ_READ 7247
#define GZ_WRITE 31153
#define GZ_APPEND 1     /* mode set to GZ_WRITE after the file is opened */


#define LOOK 0      /* look for a gzip header */
#define COPY 1      /* copy input directly */
#define GZIP 2      /* decompress a gzip stream */


typedef struct {
        
    int mode;               
    int fd;                 
    char *path;             
    z_off64_t pos;          
    unsigned size;          
    unsigned want;          
    unsigned char *in;      
    unsigned char *out;     
    unsigned char *next;    
        
    unsigned have;          
    int eof;                
    z_off64_t start;        
    z_off64_t raw;          
    int how;                
    int direct;             
        
    int level;              
    int strategy;           
        
    z_off64_t skip;         
    int seek;               
        
    int err;                
    char *msg;              
        
    z_stream strm;          
} gz_state;
typedef gz_state FAR *gz_statep;


void ZLIB_INTERNAL gz_error OF((gz_statep, int, const char *));
#if defined UNDER_CE
char ZLIB_INTERNAL *gz_strwinerror OF((DWORD error));
#endif




#ifdef INT_MAX
#  define GT_OFF(x) (sizeof(int) == sizeof(z_off64_t) && (x) > INT_MAX)
#else
unsigned ZLIB_INTERNAL gz_intmax OF((void));
#  define GT_OFF(x) (sizeof(int) == sizeof(z_off64_t) && (x) > gz_intmax())
#endif
