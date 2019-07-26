
















#ifndef _PKGTYPES
#define _PKGTYPES


#include "unicode/utypes.h"
#include "filestrm.h"


struct _CharList;

typedef struct _CharList
{
  const char       *str;
  struct _CharList *next;
} CharList;






const char *pkg_writeCharList(FileStream *s, CharList *l, const char *delim, int32_t quoted);




const char *pkg_writeCharListWrap(FileStream *s, CharList *l, const char *delim, const char *brk, int32_t quoted);





uint32_t pkg_countCharList(CharList *l);




CharList *pkg_prependToList(CharList *l, const char *str);






CharList *pkg_appendToList(CharList *l, CharList** end, const char *str);











CharList *pkg_appendUniqueDirToList(CharList *l, CharList** end, const char *strAlias);




UBool  pkg_listContains(CharList *l, const char *str);




void pkg_deleteList(CharList *l);




struct UPKGOptions_;
typedef   void (UPKGMODE)(struct UPKGOptions_ *, FileStream *s, UErrorCode *status);







void pkg_sttc_writeReadme(struct UPKGOptions_ *opt, const char *libName, UErrorCode *status);





typedef struct UPKGOptions_
{
  CharList   *fileListFiles; 
  CharList   *filePaths;     
  CharList   *files;         
  CharList   *outFiles;      

  const char *shortName;   
  const char *cShortName;   
  const char *entryName;   
  const char *targetDir;  
  const char *dataDir;    
  const char *tmpDir;
  const char *srcDir;
  const char *options;     
  const char *mode;        
  const char *version;     
  const char *comment;     
  const char *install;     
  const char *icuroot;     
  const char *libName;     
  UBool      rebuild;
  UBool      verbose;
  UBool      quiet;
  UBool      withoutAssembly;
  UBool      pdsbuild;     
} UPKGOptions;

char * convertToNativePathSeparators(char *path);




#if U_PLATFORM_HAS_WIN32_API
# ifndef UDATA_SO_SUFFIX
#  define UDATA_SO_SUFFIX ".dll"
# endif
# define LIB_PREFIX ""
# define LIB_STATIC_PREFIX ""
# define OBJ_SUFFIX ".obj"
# define UDATA_LIB_SUFFIX ".lib"

#elif U_PLATFORM == U_PF_CYGWIN
# define LIB_PREFIX "cyg"
# define LIB_STATIC_PREFIX "lib"
# define OBJ_SUFFIX ".o"
# define UDATA_LIB_SUFFIX ".a"

#else  
# define LIB_PREFIX "lib"
# define LIB_STATIC_PREFIX "lib"
# define OBJ_SUFFIX ".o"
# define UDATA_LIB_SUFFIX ".a"
#endif

#define ASM_SUFFIX ".s"


#define UDATA_CMN_PREFIX ""
#define UDATA_CMN_SUFFIX ".dat"
#define UDATA_CMN_INTERMEDIATE_SUFFIX "_dat"

#define ICUDATA_RES_FILE "icudata.res"

#define PKGDATA_DERIVED_PATH '\t'

#endif
