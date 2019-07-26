




#ifndef WINFILE_H
#define WINFILE_H

#ifdef _WINDOWS

#if defined(XP_WIN32) || defined(_WIN32)

#include <windows.h>
#include <stdlib.h>
#ifdef __MINGW32__
#include <sys/types.h>
#include <sys/stat.h>
#else
#include <sys\types.h>
#include <sys\stat.h>
#endif

typedef struct DIR_Struct {
    void            * directoryPtr;
    WIN32_FIND_DATA   data;
} DIR;

#define _ST_FSTYPSZ 16

#if !defined(__BORLANDC__) && !defined(__GNUC__)
 typedef unsigned long mode_t;
 typedef          long uid_t;
 typedef          long gid_t;

#ifdef WINCE
 typedef          long ino_t;
#else
 typedef          long off_t;
#endif

 typedef unsigned long nlink_t;
#endif 

typedef struct timestruc {
    time_t  tv_sec;         
    long    tv_nsec;        
} timestruc_t;


struct dirent {                                 
        ino_t           d_ino;                  
        off_t           d_off;                  
        unsigned short  d_reclen;               
        char            d_name[_MAX_FNAME];     
};

#if !defined(__BORLANDC__) && !defined (__GNUC__)
#define S_ISDIR(s)  ((s) & _S_IFDIR)
#endif

#else 


#include <sys\types.h>
#include <sys\stat.h>
#include <dos.h>




typedef struct	dirStruct_tag	{
	struct _find_t	file_data;
	char			c_checkdrive;
} dirStruct;

typedef struct DIR_Struct {
    void            * directoryPtr;
    dirStruct         data;
} DIR;

#define _ST_FSTYPSZ 16
typedef unsigned long mode_t;
typedef          long uid_t;
typedef          long gid_t;
typedef          long off_t;
typedef unsigned long nlink_t;

typedef struct timestruc {
    time_t  tv_sec;         
    long    tv_nsec;        
} timestruc_t;

struct dirent {                             
        ino_t           d_ino;              
        off_t           d_off;              
        unsigned short  d_reclen;           
#ifdef XP_WIN32
        char            d_name[_MAX_FNAME]; 
#else
        char            d_name[20]; 
#endif
};

#define S_ISDIR(s)  ((s) & _S_IFDIR)

#endif 

#define CONST const

#endif 

#endif 
