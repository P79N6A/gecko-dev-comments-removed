




































#ifndef prlink_h___
#define prlink_h___




#include "prtypes.h"

PR_BEGIN_EXTERN_C

typedef struct PRLibrary PRLibrary;

typedef struct PRStaticLinkTable {
    const char *name;
    void (*fp)();
} PRStaticLinkTable;








NSPR_API(PRStatus) PR_SetLibraryPath(const char *path);









NSPR_API(char*) PR_GetLibraryPath(void);












NSPR_API(char*) PR_GetLibraryName(const char *dir, const char *lib);





NSPR_API(void) PR_FreeLibraryName(char *mem);













NSPR_API(PRLibrary*) PR_LoadLibrary(const char *name);




















typedef enum PRLibSpecType {
    PR_LibSpec_Pathname,
    PR_LibSpec_MacNamedFragment,   
    PR_LibSpec_MacIndexedFragment, 
    PR_LibSpec_PathnameU            
} PRLibSpecType;

struct FSSpec; 

typedef struct PRLibSpec {
    PRLibSpecType type;
    union {
        
        const char *pathname;

        
        struct {
            const struct FSSpec *fsspec;
            const char *name;
        } mac_named_fragment;      

        
        struct {
            const struct FSSpec *fsspec;
            PRUint32 index;
        } mac_indexed_fragment;    

        
        const PRUnichar *pathname_u; 
    } value;
} PRLibSpec;







#define PR_LD_LAZY   0x1  /* equivalent to RTLD_LAZY on Unix */
#define PR_LD_NOW    0x2  /* equivalent to RTLD_NOW on Unix */
#define PR_LD_GLOBAL 0x4  /* equivalent to RTLD_GLOBAL on Unix */
#define PR_LD_LOCAL  0x8  /* equivalent to RTLD_LOCAL on Unix */

#define PR_LD_ALT_SEARCH_PATH  0x10  






NSPR_API(PRLibrary *)
PR_LoadLibraryWithFlags(
    PRLibSpec libSpec,    
    PRIntn flags          
);










NSPR_API(PRStatus) PR_UnloadLibrary(PRLibrary *lib);









NSPR_API(void*) PR_FindSymbol(PRLibrary *lib, const char *name);









typedef void (*PRFuncPtr)();
NSPR_API(PRFuncPtr) PR_FindFunctionSymbol(PRLibrary *lib, const char *name);











NSPR_API(void*) PR_FindSymbolAndLibrary(const char *name,
						      PRLibrary* *lib);









NSPR_API(PRFuncPtr) PR_FindFunctionSymbolAndLibrary(const char *name,
						      PRLibrary* *lib);












NSPR_API(PRLibrary*) PR_LoadStaticLibrary(
    const char *name, const PRStaticLinkTable *table);







NSPR_API(char *) PR_GetLibraryFilePathname(const char *name, PRFuncPtr addr);

PR_END_EXTERN_C

#endif 
