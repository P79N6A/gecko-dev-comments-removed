








































#include "xptiprivate.h"

XPTHeader*
xptiZipLoader::ReadXPTFileFromInputStream(nsIInputStream *stream,
                                          xptiWorkingSet* aWorkingSet)
{
    XPTCursor cursor;
    PRUint32 totalRead = 0;
    XPTState *state = nsnull;
    XPTHeader *header = nsnull;

    PRUint32 flen;
    stream->Available(&flen);
    
    char *whole = new char[flen];
    if (!whole)
    {
        return nsnull;
    }

    

    while(flen - totalRead)
    {
        PRUint32 avail;
        PRUint32 read;

        if(NS_FAILED(stream->Available(&avail)))
        {
            goto out;
        }

        if(avail > flen)
        {
            goto out;
        }

        if(NS_FAILED(stream->Read(whole+totalRead, avail, &read)))
        {
            goto out;
        }

        totalRead += read;
    }
    
    
    stream = nsnull;

    if(!(state = XPT_NewXDRState(XPT_DECODE, whole, flen)))
    {
        goto out;
    }
    
    if(!XPT_MakeCursor(state, XPT_HEADER, 0, &cursor))
    {
        goto out;
    }
    
    if (!XPT_DoHeader(aWorkingSet->GetStructArena(), &cursor, &header))
    {
        header = nsnull;
        goto out;
    }

 out:
    if(state)
        XPT_DestroyXDRState(state);
    if(whole)
        delete [] whole;
    return header;
}

