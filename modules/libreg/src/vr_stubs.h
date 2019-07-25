












































#ifndef _VR_STUBS_H_
#define _VR_STUBS_H_

#ifdef STANDALONE_REGISTRY

#include <errno.h>
#include <string.h>

#else

#include "prio.h"
#include "prlog.h"
#include "prmem.h"
#include "plstr.h"

#endif 

#if ( defined(BSDI) && !defined(BSDI_2) ) || defined(XP_OS2)
#include <sys/types.h>
#endif
#include <sys/stat.h>

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#if defined(__cplusplus)
# define XP_CPLUSPLUS
# define XP_IS_CPLUSPLUS 1
#else
# define XP_IS_CPLUSPLUS 0
#endif

#if defined(XP_CPLUSPLUS)
# define XP_BEGIN_PROTOS extern "C" {
# define XP_END_PROTOS }
#else
# define XP_BEGIN_PROTOS
# define XP_END_PROTOS
#endif


#ifdef STANDALONE_REGISTRY

#define USE_STDIO_MODES

#define XP_FileSeek(file,offset,whence) fseek((file), (offset), (whence))
#define XP_FileRead(dest,count,file)    fread((dest), 1, (count), (file))
#define XP_FileWrite(src,count,file)    fwrite((src), 1, (count), (file))
#define XP_FileTell(file)               ftell(file)
#define XP_FileFlush(file)              fflush(file)
#define XP_FileClose(file)              fclose(file)
#define XP_FileSetBufferSize(file,bufsize) (-1)

#define XP_ASSERT(x)        ((void)0)

#define XP_STRCAT(a,b)      strcat((a),(b))
#define XP_ATOI             atoi
#define XP_STRNCPY(a,b,n)   strncpy((a),(b),(n))
#define XP_STRCPY(a,b)      strcpy((a),(b))
#define XP_STRLEN(x)        strlen(x)
#define XP_SPRINTF          sprintf
#define XP_FREE(x)          free((x))
#define XP_ALLOC(x)         malloc((x))
#define XP_FREEIF(x)        if ((x)) free((x))
#define XP_STRCMP(x,y)      strcmp((x),(y))
#define XP_STRNCMP(x,y,n)   strncmp((x),(y),(n))
#define XP_STRDUP(s)        strdup((s))
#define XP_MEMCPY(d, s, l)  memcpy((d), (s), (l))
#define XP_MEMSET(d, c, l)  memset((d), (c), (l))

#define PR_Lock(a)          ((void)0)
#define PR_Unlock(a)        ((void)0)

#if defined(XP_WIN) || defined(XP_OS2)
  #define XP_STRCASECMP(x,y)  stricmp((x),(y))
  #define XP_STRNCASECMP(x,y,n) strnicmp((x),(y),(n))
#else
  #define XP_STRCASECMP(x,y)  strcasecmp((x),(y))
  #define XP_STRNCASECMP(x,y,n) strncasecmp((x),(y),(n))
#endif 

typedef FILE          * XP_File;

#else 






#if USE_MMAP_REGISTRY_IO
  
  
  
  
#define USE_NSPR_MODES

#include "mmapio.h"
#define XP_FileSeek(file,offset,whence) mmio_FileSeek((file),(offset),(whence))
#define XP_FileRead(dest,count,file)    mmio_FileRead((file), (dest), (count))
#define XP_FileWrite(src,count,file)    mmio_FileWrite((file), (src), (count))
#define XP_FileTell(file)               mmio_FileTell(file)
#define XP_FileClose(file)              mmio_FileClose(file)
#define XP_FileOpen(path, mode)         mmio_FileOpen((path), mode )
#define XP_FileFlush(file)              ((void)1)
#define XP_FileSetBufferSize(file, bufsize) (-1)

typedef MmioFile* XP_File;

#elif USE_BUFFERED_REGISTRY_IO
  
  
  
  
#define USE_STDIO_MODES

#include "nr_bufio.h"
#define XP_FileSeek(file,offset,whence) bufio_Seek((file),(offset),(whence))
#define XP_FileRead(dest,count,file)    bufio_Read((file), (dest), (count))
#define XP_FileWrite(src,count,file)    bufio_Write((file), (src), (count))
#define XP_FileTell(file)               bufio_Tell(file)
#define XP_FileClose(file)              bufio_Close(file)
#define XP_FileOpen(path, mode)         bufio_Open((path), (mode))
#define XP_FileFlush(file)              bufio_Flush(file)
#define XP_FileSetBufferSize(file,bufsize) bufio_SetBufferSize(file,bufsize)


typedef BufioFile* XP_File;

#else
  
  
  
#define USE_NSPR_MODES






#define XP_FileSeek(file,offset,whence) (PR_Seek((file), (offset), (whence)) < 0)
#define XP_FileRead(dest,count,file)    PR_Read((file), (dest), (count))
#define XP_FileWrite(src,count,file)    PR_Write((file), (src), (count))
#define XP_FileTell(file)               PR_Seek(file, 0, PR_SEEK_CUR)
#define XP_FileOpen(path, mode)         PR_Open((path), mode )
#define XP_FileClose(file)              PR_Close(file)
#define XP_FileFlush(file)              PR_Sync(file)
#define XP_FileSetBufferSize(file,bufsize) (-1)

typedef PRFileDesc* XP_File;

#endif 



#define XP_ASSERT(x)        PR_ASSERT((x))

#define XP_STRCAT(a,b)      PL_strcat((a),(b))
#define XP_ATOI             PL_atoi
#define XP_STRCPY(a,b)      PL_strcpy((a),(b))
#define XP_STRNCPY(a,b,n)   PL_strncpy((a),(b),(n))
#define XP_STRLEN(x)        PL_strlen(x)
#define XP_SPRINTF          sprintf
#define XP_FREE(x)          PR_Free((x))
#define XP_ALLOC(x)         PR_Malloc((x))
#define XP_FREEIF(x)        PR_FREEIF(x)
#define XP_STRCMP(x,y)      PL_strcmp((x),(y))
#define XP_STRNCMP(x,y,n)   PL_strncmp((x),(y),(n))
#define XP_STRDUP(s)        PL_strdup((s))
#define XP_MEMCPY(d, s, l)  memcpy((d), (s), (l))
#define XP_MEMSET(d, c, l)  memset((d), (c), (l))

#define XP_STRCASECMP(x,y)  PL_strcasecmp((x),(y))
#define XP_STRNCASECMP(x,y,n) PL_strncasecmp((x),(y),(n))


#endif 


#ifdef USE_STDIO_MODES
#define XP_FILE_READ             "r"
#define XP_FILE_READ_BIN         "rb"
#define XP_FILE_WRITE            "w"
#define XP_FILE_WRITE_BIN        "wb"
#define XP_FILE_UPDATE           "r+"
#define XP_FILE_TRUNCATE         "w+"
#ifdef SUNOS4

#define XP_FILE_UPDATE_BIN       "r+"
#define XP_FILE_TRUNCATE_BIN     "w+"
#else
#define XP_FILE_UPDATE_BIN       "rb+"
#define XP_FILE_TRUNCATE_BIN     "wb+"
#endif
#endif 


#ifdef USE_NSPR_MODES
#define XP_FILE_READ             PR_RDONLY, 0644
#define XP_FILE_READ_BIN         PR_RDONLY, 0644
#define XP_FILE_WRITE            PR_WRONLY, 0644
#define XP_FILE_WRITE_BIN        PR_WRONLY, 0644
#define XP_FILE_UPDATE           (PR_RDWR|PR_CREATE_FILE), 0644
#define XP_FILE_TRUNCATE         (PR_RDWR | PR_TRUNCATE), 0644
#define XP_FILE_UPDATE_BIN       PR_RDWR|PR_CREATE_FILE, 0644
#define XP_FILE_TRUNCATE_BIN     (PR_RDWR | PR_TRUNCATE), 0644

#ifdef SEEK_SET
    #undef SEEK_SET
    #undef SEEK_CUR
    #undef SEEK_END
    #define SEEK_SET PR_SEEK_SET
    #define SEEK_CUR PR_SEEK_CUR
    #define SEEK_END PR_SEEK_END
#endif
#endif 





#ifdef STANDALONE_REGISTRY 
#include "prtypes.h"
#endif 

typedef int XP_Bool;

typedef struct stat    XP_StatStruct;
#define  XP_Stat(file,data)     stat((file),(data))

XP_BEGIN_PROTOS

#define nr_RenameFile(from, to)    rename((from), (to))

extern char* globalRegName;
extern char* verRegName;

extern void vr_findGlobalRegName();
extern char* vr_findVerRegName();


#ifdef STANDALONE_REGISTRY 

extern XP_File vr_fileOpen(const char *name, const char * mode);


#else
#define vr_fileOpen PR_Open
#endif 

XP_END_PROTOS

#endif 
