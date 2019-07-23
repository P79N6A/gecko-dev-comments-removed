







































#if defined(_PRVERSION_H)
#else
#define _PRVERSION_H

#include "prtypes.h"

PR_BEGIN_EXTERN_C








#ifdef _WIN32
#pragma pack(push, 8)
#endif

typedef struct {
    




    PRInt32    version; 
  
    
    PRInt64         buildTime;      
    char *          buildTimeString;
  
    PRUint8   vMajor;               
    PRUint8   vMinor;               
    PRUint8   vPatch;               
  
    PRBool          beta;           
    PRBool          debug;          
    PRBool          special;        
  
    char *          filename;       
    char *          description;    
    char *          security;       
    char *          copyright;      
    char *          comment;        
    char *          specialString;  
} PRVersionDescription;


#ifdef _WIN32
#pragma pack(pop)
#endif

















typedef const PRVersionDescription *(*versionEntryPointType)(void);






















PR_END_EXTERN_C

#endif  



