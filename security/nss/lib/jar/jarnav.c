










































#include "jar.h"
#include "jarint.h"


#ifdef MOZILLA_CLIENT_OLD
extern MWContext *XP_FindSomeContext(void);
#endif


extern MWContext *FE_GetInitContext(void);


static MWContext *(*jar_fn_FindSomeContext) (void) = NULL;


static MWContext *(*jar_fn_GetInitContext) (void) = NULL;








void JAR_init (void)
  {
#ifdef MOZILLA_CLIENT_OLD
  JAR_init_callbacks (XP_GetString, XP_FindSomeContext, FE_GetInitContext);
#else
  JAR_init_callbacks (XP_GetString, NULL, NULL);
#endif
  }









int JAR_set_context (JAR *jar, MWContext *mw)
  {
  if (mw)
    {
    jar->mw = mw;
    }
  else
    {
    
    jar->mw = NULL;

    





    
    if (jar->mw == NULL)
      {
      jar->mw = jar_fn_GetInitContext();
      }
   }

  return 0;
  }
