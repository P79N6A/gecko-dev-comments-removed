





#include "nsDebug.h"
#include "nsString.h"
#include "nsCUPSShim.h"
#include "mozilla/ArrayUtils.h"
#include "prlink.h"





static const char gSymName[][sizeof("cupsPrintFile")] = {
    { "cupsAddOption" },
    { "cupsFreeDests" },
    { "cupsGetDest" },
    { "cupsGetDests" },
    { "cupsPrintFile" },
    { "cupsTempFd" },
};
static const int gSymNameCt = mozilla::ArrayLength(gSymName);


bool
nsCUPSShim::Init()
{
    mCupsLib = PR_LoadLibrary("libcups.so.2");
    if (!mCupsLib)
        return false;

    
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
            nsAutoCString msg(gSymName[i]);
            msg.AppendLiteral(" not found in CUPS library");
            NS_WARNING(msg.get());
#endif
            PR_UnloadLibrary(mCupsLib);
            mCupsLib = nullptr;
            return false;
        }
    }
    return true;
}
