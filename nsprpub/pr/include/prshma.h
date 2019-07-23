

































































































































#ifndef prshma_h___
#define prshma_h___

#include "prtypes.h"
#include "prio.h"
#include "prproces.h"

PR_BEGIN_EXTERN_C



























NSPR_API( PRFileMap *)
PR_OpenAnonFileMap(
    const char *dirName,
    PRSize      size, 
    PRFileMapProtect prot
);  






















NSPR_API(PRStatus) 
PR_ProcessAttrSetInheritableFileMap( 
    PRProcessAttr   *attr,
    PRFileMap       *fm, 
    const char      *shmname
);



















NSPR_API( PRFileMap *)
PR_GetInheritedFileMap( 
    const char *shmname 
);




















NSPR_API( PRStatus )
PR_ExportFileMapAsString( 
    PRFileMap *fm,
    PRSize    bufsize,
    char      *buf
);
#define PR_FILEMAP_STRING_BUFSIZE 128















NSPR_API( PRFileMap * )
PR_ImportFileMapFromString( 
    const char *fmstring
);

PR_END_EXTERN_C
#endif 
