





































static char *RCSSTRING __UNUSED__ ="$Id: byteorder.c,v 1.2 2007/06/26 22:37:55 adamcain Exp $";

#include "nr_common.h"
#ifndef WIN32
#include <arpa/inet.h>
#endif
#include "r_types.h"
#include "byteorder.h"

#define IS_BIG_ENDIAN (htonl(0x1) == 0x1)

#define BYTE(n,i)   (((UCHAR*)&(n))[(i)])
#define SWAP(n,x,y) tmp=BYTE((n),(x)), \
                    BYTE((n),(x))=BYTE((n),(y)), \
                    BYTE((n),(y))=tmp

UINT8
nr_htonll(UINT8 hostlonglong)
{
    UINT8 netlonglong = hostlonglong;
    UCHAR tmp;

    if (!IS_BIG_ENDIAN) {
        SWAP(netlonglong, 0, 7);
        SWAP(netlonglong, 1, 6);
        SWAP(netlonglong, 2, 5);
        SWAP(netlonglong, 3, 4);
    }

    return netlonglong;
}

UINT8
nr_ntohll(UINT8 netlonglong)
{
    return nr_htonll(netlonglong);
}

