






#ifndef ZCONF_H
#define ZCONF_H


#include "mozzconf.h"







#ifdef Z_PREFIX     


#  define _dist_code            z__dist_code
#  define _length_code          z__length_code
#  define _tr_align             z__tr_align
#  define _tr_flush_block       z__tr_flush_block
#  define _tr_init              z__tr_init
#  define _tr_stored_block      z__tr_stored_block
#  define _tr_tally             z__tr_tally
#  define adler32               z_adler32
#  define adler32_combine       z_adler32_combine
#  define adler32_combine64     z_adler32_combine64
#  define compress              z_compress
#  define compress2             z_compress2
#  define compressBound         z_compressBound
#  define crc32                 z_crc32
#  define crc32_combine         z_crc32_combine
#  define crc32_combine64       z_crc32_combine64
#  define deflate               z_deflate
#  define deflateBound          z_deflateBound
#  define deflateCopy           z_deflateCopy
#  define deflateEnd            z_deflateEnd
#  define deflateInit2_         z_deflateInit2_
#  define deflateInit_          z_deflateInit_
#  define deflateParams         z_deflateParams
#  define deflatePrime          z_deflatePrime
#  define deflateReset          z_deflateReset
#  define deflateSetDictionary  z_deflateSetDictionary
#  define deflateSetHeader      z_deflateSetHeader
#  define deflateTune           z_deflateTune
#  define deflate_copyright     z_deflate_copyright
#  define get_crc_table         z_get_crc_table
#  define gz_error              z_gz_error
#  define gz_intmax             z_gz_intmax
#  define gz_strwinerror        z_gz_strwinerror
#  define gzbuffer              z_gzbuffer
#  define gzclearerr            z_gzclearerr
#  define gzclose               z_gzclose
#  define gzclose_r             z_gzclose_r
#  define gzclose_w             z_gzclose_w
#  define gzdirect              z_gzdirect
#  define gzdopen               z_gzdopen
#  define gzeof                 z_gzeof
#  define gzerror               z_gzerror
#  define gzflush               z_gzflush
#  define gzgetc                z_gzgetc
#  define gzgets                z_gzgets
#  define gzoffset              z_gzoffset
#  define gzoffset64            z_gzoffset64
#  define gzopen                z_gzopen
#  define gzopen64              z_gzopen64
#  define gzprintf              z_gzprintf
#  define gzputc                z_gzputc
#  define gzputs                z_gzputs
#  define gzread                z_gzread
#  define gzrewind              z_gzrewind
#  define gzseek                z_gzseek
#  define gzseek64              z_gzseek64
#  define gzsetparams           z_gzsetparams
#  define gztell                z_gztell
#  define gztell64              z_gztell64
#  define gzungetc              z_gzungetc
#  define gzwrite               z_gzwrite
#  define inflate               z_inflate
#  define inflateBack           z_inflateBack
#  define inflateBackEnd        z_inflateBackEnd
#  define inflateBackInit_      z_inflateBackInit_
#  define inflateCopy           z_inflateCopy
#  define inflateEnd            z_inflateEnd
#  define inflateGetHeader      z_inflateGetHeader
#  define inflateInit2_         z_inflateInit2_
#  define inflateInit_          z_inflateInit_
#  define inflateMark           z_inflateMark
#  define inflatePrime          z_inflatePrime
#  define inflateReset          z_inflateReset
#  define inflateReset2         z_inflateReset2
#  define inflateSetDictionary  z_inflateSetDictionary
#  define inflateSync           z_inflateSync
#  define inflateSyncPoint      z_inflateSyncPoint
#  define inflateUndermine      z_inflateUndermine
#  define inflate_copyright     z_inflate_copyright
#  define inflate_fast          z_inflate_fast
#  define inflate_table         z_inflate_table
#  define uncompress            z_uncompress
#  define zError                z_zError
#  define zcalloc               z_zcalloc
#  define zcfree                z_zcfree
#  define zlibCompileFlags      z_zlibCompileFlags
#  define zlibVersion           z_zlibVersion


#  define Byte                  z_Byte
#  define Bytef                 z_Bytef
#  define alloc_func            z_alloc_func
#  define charf                 z_charf
#  define free_func             z_free_func
#  define gzFile                z_gzFile
#  define gz_header             z_gz_header
#  define gz_headerp            z_gz_headerp
#  define in_func               z_in_func
#  define intf                  z_intf
#  define out_func              z_out_func
#  define uInt                  z_uInt
#  define uIntf                 z_uIntf
#  define uLong                 z_uLong
#  define uLongf                z_uLongf
#  define voidp                 z_voidp
#  define voidpc                z_voidpc
#  define voidpf                z_voidpf


#  define gz_header_s           z_gz_header_s
#  define internal_state        z_internal_state

#endif

#if defined(__MSDOS__) && !defined(MSDOS)
#  define MSDOS
#endif
#if (defined(OS_2) || defined(__OS2__)) && !defined(OS2)
#  define OS2
#endif
#if defined(_WINDOWS) && !defined(WINDOWS)
#  define WINDOWS
#endif
#if defined(_WIN32) || defined(_WIN32_WCE) || defined(__WIN32__)
#  ifndef WIN32
#    define WIN32
#  endif
#endif
#if (defined(MSDOS) || defined(OS2) || defined(WINDOWS)) && !defined(WIN32)
#  if !defined(__GNUC__) && !defined(__FLAT__) && !defined(__386__)
#    ifndef SYS16BIT
#      define SYS16BIT
#    endif
#  endif
#endif





#ifdef SYS16BIT
#  define MAXSEG_64K
#endif
#ifdef MSDOS
#  define UNALIGNED_OK
#endif

#ifdef __STDC_VERSION__
#  ifndef STDC
#    define STDC
#  endif
#  if __STDC_VERSION__ >= 199901L
#    ifndef STDC99
#      define STDC99
#    endif
#  endif
#endif
#if !defined(STDC) && (defined(__STDC__) || defined(__cplusplus))
#  define STDC
#endif
#if !defined(STDC) && (defined(__GNUC__) || defined(__BORLANDC__))
#  define STDC
#endif
#if !defined(STDC) && (defined(MSDOS) || defined(WINDOWS) || defined(WIN32))
#  define STDC
#endif
#if !defined(STDC) && (defined(OS2) || defined(__HOS_AIX__))
#  define STDC
#endif

#if defined(__OS400__) && !defined(STDC)    
#  define STDC
#endif

#ifndef STDC
#  ifndef const 
#    define const
#  endif
#endif


#if defined(__MWERKS__)||defined(applec)||defined(THINK_C)||defined(__SC__)
#  define NO_DUMMY_DECL
#endif


#ifndef MAX_MEM_LEVEL
#  ifdef MAXSEG_64K
#    define MAX_MEM_LEVEL 8
#  else
#    define MAX_MEM_LEVEL 9
#  endif
#endif






#ifndef MAX_WBITS
#  define MAX_WBITS   15 /* 32K LZ77 window */
#endif














                        

#ifndef OF 
#  ifdef STDC
#    define OF(args)  args
#  else
#    define OF(args)  ()
#  endif
#endif







#ifdef SYS16BIT
#  if defined(M_I86SM) || defined(M_I86MM)
     
#    define SMALL_MEDIUM
#    ifdef _MSC_VER
#      define FAR _far
#    else
#      define FAR far
#    endif
#  endif
#  if (defined(__SMALL__) || defined(__MEDIUM__))
     
#    define SMALL_MEDIUM
#    ifdef __BORLANDC__
#      define FAR _far
#    else
#      define FAR far
#    endif
#  endif
#endif

#if defined(WINDOWS) || defined(WIN32)
   


#  ifdef ZLIB_DLL
#    if defined(WIN32) && (!defined(__BORLANDC__) || (__BORLANDC__ >= 0x500))
#      ifdef ZLIB_INTERNAL
#        define ZEXTERN extern __declspec(dllexport)
#      else
#        define ZEXTERN extern __declspec(dllimport)
#      endif
#    endif
#  endif  
   



#  ifdef ZLIB_WINAPI
#    ifdef FAR
#      undef FAR
#    endif
#    include <windows.h>
     
     
#    define ZEXPORT WINAPI
#    ifdef WIN32
#      define ZEXPORTVA WINAPIV
#    else
#      define ZEXPORTVA FAR CDECL
#    endif
#  endif
#endif

#if defined(__OS2__) || defined (__BEOS__)
#  ifdef ZLIB_DLL
#    ifdef ZLIB_INTERNAL
#      define ZEXPORT   __declspec(dllexport)
#      define ZEXPORTVA __declspec(dllexport)
#    else
#      define ZEXPORT   __declspec(dllimport)
#      define ZEXPORTVA __declspec(dllimport)
#    endif
#  endif
#endif

#ifndef ZEXTERN
#  define ZEXTERN extern
#endif
#ifndef ZEXPORT
#  define ZEXPORT
#endif
#ifndef ZEXPORTVA
#  define ZEXPORTVA
#endif

#ifndef FAR
#  define FAR
#endif

#if !defined(__MACTYPES__)
typedef unsigned char  Byte;  
#endif
typedef unsigned int   uInt;  
typedef unsigned long  uLong; 

#ifdef SMALL_MEDIUM
   
#  define Bytef Byte FAR
#else
   typedef Byte  FAR Bytef;
#endif
typedef char  FAR charf;
typedef int   FAR intf;
typedef uInt  FAR uIntf;
typedef uLong FAR uLongf;

#ifdef STDC
   typedef void const *voidpc;
   typedef void FAR   *voidpf;
   typedef void       *voidp;
#else
   typedef Byte const *voidpc;
   typedef Byte FAR   *voidpf;
   typedef Byte       *voidp;
#endif

#ifdef HAVE_UNISTD_H    
#  define Z_HAVE_UNISTD_H
#endif

#ifdef STDC
#  include <sys/types.h>    
#endif







#if -_LARGEFILE64_SOURCE - -1 == 1
#  undef _LARGEFILE64_SOURCE
#endif

#if defined(Z_HAVE_UNISTD_H) || defined(_LARGEFILE64_SOURCE)
#  include <unistd.h>       
#  ifdef VMS
#    include <unixio.h>     
#  endif
#  ifndef z_off_t
#    define z_off_t off_t
#  endif
#endif

#ifndef SEEK_SET
#  define SEEK_SET        0       /* Seek from beginning of file.  */
#  define SEEK_CUR        1       /* Seek from current position.  */
#  define SEEK_END        2       /* Set file pointer to EOF plus "offset" */
#endif

#ifndef z_off_t
#  define z_off_t long
#endif

#if defined(_LARGEFILE64_SOURCE) && _LFS64_LARGEFILE-0
#  define z_off64_t off64_t
#else
#  define z_off64_t z_off_t
#endif

#if defined(__OS400__)
#  define NO_vsnprintf
#endif

#if defined(__MVS__)
#  define NO_vsnprintf
#endif


#if defined(__MVS__)
  #pragma map(deflateInit_,"DEIN")
  #pragma map(deflateInit2_,"DEIN2")
  #pragma map(deflateEnd,"DEEND")
  #pragma map(deflateBound,"DEBND")
  #pragma map(inflateInit_,"ININ")
  #pragma map(inflateInit2_,"ININ2")
  #pragma map(inflateEnd,"INEND")
  #pragma map(inflateSync,"INSY")
  #pragma map(inflateSetDictionary,"INSEDI")
  #pragma map(compressBound,"CMBND")
  #pragma map(inflate_table,"INTABL")
  #pragma map(inflate_fast,"INFA")
  #pragma map(inflate_copyright,"INCOPY")
#endif

#endif 
