



































#ifndef prefread_h__
#define prefread_h__

#include "prtypes.h"
#include "prefapi.h"

PR_BEGIN_EXTERN_C

















typedef void (*PrefReader)(void       *closure,
                           const char *pref,
                           PrefValue   val,
                           PrefType    type,
                           bool        defPref);


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
    bool        fdefault;   
} PrefParseState;














void PREF_InitParseState(PrefParseState *ps, PrefReader reader, void *closure);








        
void PREF_FinalizeParseState(PrefParseState *ps);


















bool PREF_ParseBuf(PrefParseState *ps, const char *buf, int bufLen);

PR_END_EXTERN_C
#endif 
