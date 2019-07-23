
























#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pixman-sse.h"

#ifdef USE_SSE2

void
fbComposeSetupSSE(void)
{
    static pixman_bool_t initialized = FALSE;

    if (initialized)
	return;
    
    
    if (pixman_have_sse())
    {
    }

    initialized = TRUE;
}


#endif 
