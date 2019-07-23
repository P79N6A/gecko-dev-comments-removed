










































#include "primpl.h"

extern PRLogModuleInfo *_pr_shma_lm;

#if defined(XP_UNIX)

#elif defined(WIN32)

#else
extern PRFileMap * _PR_MD_OPEN_ANON_FILE_MAP( const char *dirName, PRSize size, PRFileMapProtect prot )
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return NULL;
}
extern PRStatus _PR_MD_EXPORT_FILE_MAP_AS_STRING(PRFileMap *fm, PRSize bufSize, char *buf)
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}
extern PRFileMap * _PR_MD_IMPORT_FILE_MAP_FROM_STRING(const char *fmstring)
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return NULL;
}
#endif





PR_IMPLEMENT(PRFileMap*)
PR_OpenAnonFileMap(
    const char *dirName,
    PRSize      size, 
    PRFileMapProtect prot
)
{
    return(_PR_MD_OPEN_ANON_FILE_MAP( dirName, size, prot ));
} 







PR_IMPLEMENT( PRStatus) 
PR_ProcessAttrSetInheritableFileMap( 
    PRProcessAttr   *attr,
    PRFileMap       *fm, 
    const char      *shmname
)
{
    PR_SetError( PR_NOT_IMPLEMENTED_ERROR, 0 );
    return( PR_FAILURE);
}  






PR_IMPLEMENT( PRFileMap *)
PR_GetInheritedFileMap( 
    const char *shmname 
)
{
    PRFileMap   *fm = NULL;
    PR_SetError( PR_NOT_IMPLEMENTED_ERROR, 0 );
    return( fm );
} 





PR_IMPLEMENT( PRStatus )
PR_ExportFileMapAsString( 
    PRFileMap *fm,
    PRSize    bufSize,
    char      *buf
)
{
    return( _PR_MD_EXPORT_FILE_MAP_AS_STRING( fm, bufSize, buf ));
} 






PR_IMPLEMENT( PRFileMap * )
PR_ImportFileMapFromString( 
    const char *fmstring
)
{
    return( _PR_MD_IMPORT_FILE_MAP_FROM_STRING(fmstring));
} 

