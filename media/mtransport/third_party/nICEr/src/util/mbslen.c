
































#ifdef LINUX
#define _GNU_SOURCE 1
#endif
#include <string.h>

#include <errno.h>
#include <csi_platform.h>

#include <assert.h>
#include <locale.h>
#include <stdlib.h>
#include <wchar.h>
#ifdef DARWIN
#include <xlocale.h>
#endif 

#include "nr_api.h"
#include "mbslen.h"


int
mbslen(const char *s, size_t *ncharsp)
{
#ifdef DARWIN
    static locale_t loc = 0;
    static int initialized = 0;
#endif 
#ifdef WIN32
    char *my_locale=0;
    unsigned int i;
#endif  
    int _status;
    size_t nbytes;
    int nchars;
    mbstate_t mbs;

#ifdef DARWIN
    if (! initialized) {
        initialized = 1;
        loc = newlocale(LC_CTYPE_MASK, "UTF-8", LC_GLOBAL_LOCALE);
    }

    if (loc == 0) {
        
        assert(loc != 0);  
#endif

#ifdef WIN32
    if (!setlocale(LC_CTYPE, 0))
        ABORT(R_INTERNAL);

    if (!(my_locale = r_strdup(setlocale(LC_CTYPE, 0))))
        ABORT(R_NO_MEMORY);

    for (i=0; i<strlen(my_locale); i++)
        my_locale[i] = toupper(my_locale[i]);

    if (!strstr(my_locale, "UTF-8"))
        ABORT(R_NOT_FOUND);
#else
    
    
    char *locale = setlocale(LC_CTYPE, 0);
    if (!locale || !strcasestr(locale, "UTF-8"))
        ABORT(R_NOT_FOUND);
#endif

#ifdef DARWIN
    }
#endif 

    memset(&mbs, 0, sizeof(mbs));
    nchars = 0;

#ifdef DARWIN
    while (*s != '\0' && (nbytes = mbrlen_l(s, strlen(s), &mbs, loc)) != 0)
#else
    while (*s != '\0' && (nbytes = mbrlen(s, strlen(s), &mbs)) != 0)
#endif 
    {
        if (nbytes == (size_t)-1)    {
            assert(0);
            ABORT(R_INTERNAL);
        }
        if (nbytes == (size_t)-2)    {
            assert(0);
            ABORT(R_BAD_DATA);
        }

        s += nbytes;
        ++nchars;
    }

    *ncharsp = nchars;

    _status = 0;
  abort:
#ifdef WIN32
    RFREE(my_locale);
#endif
    return _status;
}

