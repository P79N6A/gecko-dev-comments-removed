




































#ifndef prpcos_h___
#define prpcos_h___

#define PR_DLL_SUFFIX		".dll"

#include <stdlib.h>

#define DIRECTORY_SEPARATOR         '\\'
#define DIRECTORY_SEPARATOR_STR     "\\"
#define PATH_SEPARATOR              ';'

#ifdef WIN16
#define GCPTR __far
#else
#define GCPTR
#endif




PR_BEGIN_EXTERN_C
#ifndef XP_OS2
extern char *optarg;
extern int optind;
extern int getopt(int argc, char **argv, char *spec);
#endif
PR_END_EXTERN_C







#ifdef XP_OS2
#include <sys/types.h>
#endif
#include <sys/stat.h>
#include <io.h>
#include <fcntl.h>          

#ifdef OS2
extern PRStatus _MD_OS2GetHostName(char *name, PRUint32 namelen);
#define _MD_GETHOSTNAME _MD_OS2GetHostName
#else
extern PRStatus _MD_WindowsGetHostName(char *name, PRUint32 namelen);
#define _MD_GETHOSTNAME _MD_WindowsGetHostName
extern PRStatus _MD_WindowsGetSysInfo(PRSysInfo cmd, char *name, PRUint32 namelen);
#define _MD_GETSYSINFO _MD_WindowsGetSysInfo
#endif

#endif 
