







































































































#ifndef ICUPLUG_H
#define ICUPLUG_H

#include "unicode/utypes.h"




#ifndef U_HIDE_INTERNAL_API







struct UPlugData;
typedef struct UPlugData UPlugData;








#define UPLUG_TOKEN 0x54762486





#define UPLUG_NAME_MAX              100








typedef uint32_t UPlugTokenReturn;





typedef enum {
    UPLUG_REASON_QUERY = 0,     
    UPLUG_REASON_LOAD = 1,     
    UPLUG_REASON_UNLOAD = 2,   
    UPLUG_REASON_COUNT         
} UPlugReason;









typedef enum {
    UPLUG_LEVEL_INVALID = 0,     
    UPLUG_LEVEL_UNKNOWN = 1,     
    UPLUG_LEVEL_LOW     = 2,     
    UPLUG_LEVEL_HIGH    = 3,     
    UPLUG_LEVEL_COUNT         
} UPlugLevel;








typedef UPlugTokenReturn (U_EXPORT2 UPlugEntrypoint) (
                  UPlugData *plug,
                  UPlugReason reason,
                  UErrorCode *status);











U_INTERNAL void U_EXPORT2 
uplug_setPlugNoUnload(UPlugData *plug, UBool dontUnload);







U_INTERNAL void U_EXPORT2
uplug_setPlugLevel(UPlugData *plug, UPlugLevel level);







U_INTERNAL UPlugLevel U_EXPORT2
uplug_getPlugLevel(UPlugData *plug);








U_INTERNAL UPlugLevel U_EXPORT2
uplug_getCurrentLevel(void);







U_INTERNAL UErrorCode U_EXPORT2
uplug_getPlugLoadStatus(UPlugData *plug); 







U_INTERNAL void U_EXPORT2
uplug_setPlugName(UPlugData *plug, const char *name);







U_INTERNAL const char * U_EXPORT2
uplug_getPlugName(UPlugData *plug);







U_INTERNAL const char * U_EXPORT2
uplug_getSymbolName(UPlugData *plug);








U_INTERNAL const char * U_EXPORT2
uplug_getLibraryName(UPlugData *plug, UErrorCode *status);








U_INTERNAL void * U_EXPORT2
uplug_getLibrary(UPlugData *plug);







U_INTERNAL void * U_EXPORT2
uplug_getContext(UPlugData *plug);







U_INTERNAL void U_EXPORT2
uplug_setContext(UPlugData *plug, void *context);









U_INTERNAL const char * U_EXPORT2
uplug_getConfiguration(UPlugData *plug);
















U_INTERNAL UPlugData* U_EXPORT2
uplug_nextPlug(UPlugData *prior);













U_INTERNAL UPlugData* U_EXPORT2
uplug_loadPlugFromEntrypoint(UPlugEntrypoint *entrypoint, const char *config, UErrorCode *status);












U_INTERNAL UPlugData* U_EXPORT2
uplug_loadPlugFromLibrary(const char *libName, const char *sym, const char *config, UErrorCode *status);








U_INTERNAL void U_EXPORT2
uplug_removePlug(UPlugData *plug, UErrorCode *status);
#endif  

#endif
