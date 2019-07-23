







































#include "jar.h"
#include "jarint.h"





PRFileDesc*
JAR_FOPEN_to_PR_Open(const char* name, const char *mode)
{

    PRIntn  prflags=0, prmode=0;

    
    if (strchr(mode, 'r') && !strchr(mode, '+')) {
	prflags |= PR_RDONLY;
    } else if( (strchr(mode, 'w') || strchr(mode, 'a')) &&
	!strchr(mode,'+') ) {
	prflags |= PR_WRONLY;
    } else {
	prflags |= PR_RDWR;
    }

    
    if (strchr(mode, 'w') || strchr(mode, 'a')) {
	prflags |= PR_CREATE_FILE;
    }

    
    if (strchr(mode, 'a')) {
	prflags |= PR_APPEND;
    }

    
    if (strchr(mode, 'w')) {
	prflags |= PR_TRUNCATE;
    }

    

    prmode = 0755;

    return PR_Open(name, prflags, prmode);
}
