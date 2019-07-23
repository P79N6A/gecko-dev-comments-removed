










































#include "jar.h"
#include "jarint.h"


extern MWContext *FE_GetInitContext(void);


static MWContext *(*jar_fn_FindSomeContext) (void) = NULL;


static MWContext *(*jar_fn_GetInitContext) (void) = NULL;








void JAR_init (void)
{
    JAR_init_callbacks (XP_GetString, NULL, NULL);
}








int 
JAR_set_context(JAR *jar, MWContext *mw)
{
    if (mw) {
	jar->mw = mw;
    } else {
	
	jar->mw = NULL;
	




	
	if (jar->mw == NULL) {
	    jar->mw = jar_fn_GetInitContext();
	}
    }
    return 0;
}
