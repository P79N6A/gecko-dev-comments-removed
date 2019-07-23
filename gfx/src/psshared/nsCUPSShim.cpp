





































#include "nsDebug.h"
#include "nsString.h"
#include "nsCUPSShim.h"
#include "prlink.h"





static const char gSymName[][sizeof("cupsPrintFile")] = {
    { "cupsAddOption" },
    { "cupsFreeDests" },
    { "cupsGetDest" },
    { "cupsGetDests" },
    { "cupsPrintFile" },
    { "cupsTempFd" },
};
static const int gSymNameCt = sizeof(gSymName) / sizeof(gSymName[0]);


PRBool
nsCUPSShim::Init()
{
    mCupsLib = PR_LoadLibrary("libcups.so.2");
    if (!mCupsLib)
        return PR_FALSE;

    
    void **symAddr[] = {
        (void **)&mCupsAddOption,
        (void **)&mCupsFreeDests,
        (void **)&mCupsGetDest,
        (void **)&mCupsGetDests,
        (void **)&mCupsPrintFile,
        (void **)&mCupsTempFd,
    };

    for (int i = gSymNameCt; i--; ) {
        *(symAddr[i]) = PR_FindSymbol(mCupsLib, gSymName[i]);
        if (! *(symAddr[i])) {
#ifdef DEBUG
            nsCAutoString msg(gSymName[i]);
            msg.Append(" not found in CUPS library");
            NS_WARNING(msg.get());
#endif
            PR_UnloadLibrary(mCupsLib);
            mCupsLib = nsnull;
            return PR_FALSE;
        }
    }
    return PR_TRUE;
}

nsCUPSShim::~nsCUPSShim()
{
    if (mCupsLib)
        PR_UnloadLibrary(mCupsLib);
}
