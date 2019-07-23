




































#include "prinit.h"
#include "prvrsion.h"




#include "_pr_bld.h"
#if !defined(_BUILD_TIME)
#ifdef HAVE_LONG_LONG
#define _BUILD_TIME 0
#else
#define _BUILD_TIME {0, 0}
#endif
#endif
#if !defined(_BUILD_STRING)
#define _BUILD_STRING ""
#endif
#if !defined(_PRODUCTION)
#define _PRODUCTION ""
#endif
#if defined(DEBUG)
#define _DEBUG_STRING " (debug)"
#else
#define _DEBUG_STRING ""
#endif




#define CONCAT(x, y) x ## y
#define CONCAT2(x, y) CONCAT(x, y)
#define VERSION_DESC_NAME CONCAT2(prVersionDescription_libnspr, PR_VMAJOR)

PRVersionDescription VERSION_DESC_NAME =
{
      2,                  
      _BUILD_TIME,        
      _BUILD_STRING,       
      PR_VMAJOR,          
      PR_VMINOR,          
      PR_VPATCH,          
      PR_BETA,            
#if defined(DEBUG)
      PR_TRUE,            
#else
      PR_FALSE,           
#endif
      PR_FALSE,           
      _PRODUCTION,        
     "Portable runtime",  
     "N/A",               
      "Copyright (c) 1998 Netscape Communications Corporation. All Rights Reserved",
      "License information: http://www.mozilla.org/MPL/",
     ""
};

#ifdef XP_UNIX







static char rcsid[] = "$Header: NSPR " PR_VERSION _DEBUG_STRING
        "  " _BUILD_STRING " $";
static char sccsid[] = "@(#)NSPR " PR_VERSION _DEBUG_STRING
        "  " _BUILD_STRING;

#endif 

PR_IMPLEMENT(const PRVersionDescription*) libVersionPoint(void)
{
#ifdef XP_UNIX
    


 
    const char *dummy;

    dummy = rcsid;
    dummy = sccsid;
#endif
    return &VERSION_DESC_NAME;
}  



