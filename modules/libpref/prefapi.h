







#ifndef PREFAPI_H
#define PREFAPI_H

#include "nscore.h"
#include "pldhash.h"

#ifdef __cplusplus
extern "C" {
#endif


static const uint32_t MAX_PREF_LENGTH = 1 * 1024 * 1024;

static const uint32_t MAX_ADVISABLE_PREF_LENGTH = 4 * 1024;

typedef union
{
    char*       stringVal;
    int32_t     intVal;
    bool        boolVal;
} PrefValue;

struct PrefHashEntry : PLDHashEntryHdr
{
    const char *key;
    PrefValue defaultPref;
    PrefValue userPref;
    uint16_t  flags;
};







nsresult    PREF_Init();





void        PREF_Cleanup();
void        PREF_CleanupPrefs();







typedef enum { PREF_INVALID = 0,
               PREF_LOCKED = 1, PREF_USERSET = 2, PREF_CONFIG = 4, PREF_REMOTE = 8,
               PREF_LILOCAL = 16, PREF_STRING = 32, PREF_INT = 64, PREF_BOOL = 128,
               PREF_HAS_DEFAULT = 256,
               
               PREF_STICKY_DEFAULT = 512,
               PREF_VALUETYPE_MASK = (PREF_STRING | PREF_INT | PREF_BOOL)
             } PrefType;


















nsresult PREF_SetCharPref(const char *pref,const char* value, bool set_default = false);
nsresult PREF_SetIntPref(const char *pref,int32_t value, bool set_default = false);
nsresult PREF_SetBoolPref(const char *pref,bool value, bool set_default = false);

bool     PREF_HasUserPref(const char* pref_name);














nsresult PREF_GetIntPref(const char *pref,
                           int32_t * return_int, bool get_default);	
nsresult PREF_GetBoolPref(const char *pref, bool * return_val, bool get_default);	







nsresult PREF_CopyCharPref(const char *pref, char ** return_buf, bool get_default);






bool PREF_PrefIsLocked(const char *pref_name);







nsresult PREF_LockPref(const char *key, bool lockIt);

PrefType PREF_GetPrefType(const char *pref_name);




nsresult PREF_DeleteBranch(const char *branch_name);




nsresult PREF_ClearUserPref(const char *pref_name);




nsresult PREF_ClearAllUserPrefs();















#ifndef have_PrefChangedFunc_typedef
typedef void (*PrefChangedFunc) (const char *, void *);
#define have_PrefChangedFunc_typedef
#endif









void PREF_RegisterCallback( const char* domain,
								PrefChangedFunc callback, void* instance_data );
nsresult PREF_UnregisterCallback( const char* domain,
								PrefChangedFunc callback, void* instance_data );




void PREF_ReaderCallback( void *closure,
                          const char *pref,
                          PrefValue   value,
                          PrefType    type,
                          bool        isDefault,
                          bool        isStickyDefault);

#ifdef __cplusplus
}
#endif
#endif
