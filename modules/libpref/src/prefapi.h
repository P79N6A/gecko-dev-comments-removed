







































#ifndef PREFAPI_H
#define PREFAPI_H

#include "nscore.h"
#include "pldhash.h"

NSPR_BEGIN_EXTERN_C

typedef union
{
    char*       stringVal;
    PRInt32     intVal;
    PRBool      boolVal;
} PrefValue;

struct PrefHashEntry : PLDHashEntryHdr
{
    const char *key;
    PrefValue defaultPref;
    PrefValue userPref;
    PRUint8   flags;
};







nsresult    PREF_Init();





void        PREF_Cleanup();
void        PREF_CleanupPrefs();







typedef enum { PREF_INVALID = 0,
               PREF_LOCKED = 1, PREF_USERSET = 2, PREF_CONFIG = 4, PREF_REMOTE = 8,
			   PREF_LILOCAL = 16, PREF_STRING = 32, PREF_INT = 64, PREF_BOOL = 128,
			   PREF_VALUETYPE_MASK = (PREF_STRING | PREF_INT | PREF_BOOL)
			  } PrefType;


















nsresult PREF_SetCharPref(const char *pref,const char* value, PRBool set_default = PR_FALSE);
nsresult PREF_SetIntPref(const char *pref,PRInt32 value, PRBool set_default = PR_FALSE);
nsresult PREF_SetBoolPref(const char *pref,PRBool value, PRBool set_default = PR_FALSE);

PRBool   PREF_HasUserPref(const char* pref_name);














nsresult PREF_GetIntPref(const char *pref,
                           PRInt32 * return_int, PRBool get_default);	
nsresult PREF_GetBoolPref(const char *pref, PRBool * return_val, PRBool get_default);	







nsresult PREF_CopyCharPref(const char *pref, char ** return_buf, PRBool get_default);






PRBool PREF_PrefIsLocked(const char *pref_name);







nsresult PREF_LockPref(const char *key, PRBool lockIt);

PrefType PREF_GetPrefType(const char *pref_name);




nsresult PREF_DeleteBranch(const char *branch_name);




nsresult PREF_ClearUserPref(const char *pref_name);




nsresult PREF_ClearAllUserPrefs();















#ifndef have_PrefChangedFunc_typedef
typedef nsresult (*PrefChangedFunc) (const char *, void *); 
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
                          PRBool      isDefault);

NSPR_END_EXTERN_C
#endif
