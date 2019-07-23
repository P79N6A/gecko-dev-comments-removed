



































#ifndef prefread_h__
#define prefread_h__

#include "prtypes.h"
#include "prefapi.h"

NSPR_BEGIN_EXTERN_C

















typedef void (*PrefReader)(void       *closure,
                           const char *pref,
                           PrefValue   val,
                           PrefType    type,
                           PRBool      defPref);


typedef struct PrefParseState {
    PrefReader  reader;
    void       *closure;
    int         state;      
    int         nextstate;  
    const char *smatch;     
    int         sindex;     
                            
    PRUnichar   utf16[2];   
    int         esclen;     
    char        esctmp[6];  
    char        quotechar;  
    char       *lb;         
    char       *lbcur;      
    char       *lbend;      
    char       *vb;         
    PrefType    vtype;      
    PRBool      fdefault;   
} PrefParseState;














void PREF_InitParseState(PrefParseState *ps, PrefReader reader, void *closure);








        
void PREF_FinalizeParseState(PrefParseState *ps);


















PRBool PREF_ParseBuf(PrefParseState *ps, const char *buf, int bufLen);

NSPR_END_EXTERN_C
#endif 
