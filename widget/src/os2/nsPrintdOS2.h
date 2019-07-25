







































#ifndef nsPrintdOS2_h___
#define nsPrintdOS2_h___

#include <limits.h>

PR_BEGIN_EXTERN_C



 
#ifndef PATH_MAX
#ifdef _POSIX_PATH_MAX
#define PATH_MAX	_POSIX_PATH_MAX
#else
#define PATH_MAX	256
#endif
#endif

typedef enum  
{
  printToFile = 0, 
  printToPrinter, 
  printPreview 
} printDest;

typedef struct OS2prdata {
        printDest destination;     
        int copies;                
        char printer[ PATH_MAX ];  
        char path[ PATH_MAX ];     
        PRBool cancel;		     
} OS2PrData;

PR_END_EXTERN_C

#endif 
