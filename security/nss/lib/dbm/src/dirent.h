#ifndef __DIRENT_H__
#define __DIRENT_H__


















#ifdef __EMX__
#include <sys/param.h>
#else
#if defined(__IBMC__) || defined(__IBMCPP__) || defined(XP_W32_MSVC)
#include <stdio.h>
#ifdef MAXPATHLEN
	#undef MAXPATHLEN
#endif
#define MAXPATHLEN (FILENAME_MAX*4)
#define MAXNAMLEN FILENAME_MAX

#else
#include <param.h>
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif


#ifndef A_RONLY
# define A_RONLY   0x01
# define A_HIDDEN  0x02
# define A_SYSTEM  0x04
# define A_LABEL   0x08
# define A_DIR     0x10
# define A_ARCHIVE 0x20
#endif

struct dirent {
#if defined(OS2) || defined(WIN32)        
    int            d_ino;                 
    int            d_reclen;		  
    int            d_namlen;              
    char           d_name[MAXNAMLEN + 1];
    unsigned long  d_size;
    unsigned short d_attribute;           
    unsigned short d_time;                
    unsigned short d_date;                
#else
    char	   d_name[MAXNAMLEN + 1]; 
    char	   d_attribute;		  
    unsigned long  d_size;		  
#endif
};

typedef struct _dirdescr DIR;


extern DIR		*opendir(const char *);
extern DIR		*openxdir(const char *, unsigned);
extern struct dirent	*readdir(DIR *);
extern void		seekdir(DIR *, long);
extern long		telldir(DIR *);
extern void 		closedir(DIR *);
#define			rewinddir(dirp) seekdir(dirp, 0L)

extern char *		abs_path(const char *name, char *buffer, int len);

#ifndef S_IFMT
#define S_IFMT ( S_IFDIR | S_IFREG )
#endif

#ifndef S_ISDIR
#define S_ISDIR( m )                    (((m) & S_IFMT) == S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG( m )                    (((m) & S_IFMT) == S_IFREG)
#endif

#ifdef __cplusplus
}
#endif

#endif
