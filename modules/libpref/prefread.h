



#ifndef prefread_h__
#define prefread_h__

#include "prefapi.h"

#ifdef __cplusplus
extern "C" {
#endif



















typedef void (*PrefReader)(void       *closure,
                           const char *pref,
                           PrefValue   val,
                           PrefType    type,
                           bool        defPref,
                           bool        stickyPref);


typedef struct PrefParseState {
    PrefReader  reader;
    void       *closure;
    int         state;      
    int         nextstate;  
    const char *smatch;     
    int         sindex;     
                            
    char16_t   utf16[2];   
    int         esclen;     
    char        esctmp[6];  
    char        quotechar;  
    char       *lb;         
    char       *lbcur;      
    char       *lbend;      
    char       *vb;         
    PrefType    vtype;      
    bool        fdefault;   
    bool        fstickydefault; 
} PrefParseState;














void PREF_InitParseState(PrefParseState *ps, PrefReader reader, void *closure);








        
void PREF_FinalizeParseState(PrefParseState *ps);


















bool PREF_ParseBuf(PrefParseState *ps, const char *buf, int bufLen);

#ifdef __cplusplus
}
#endif
#endif
