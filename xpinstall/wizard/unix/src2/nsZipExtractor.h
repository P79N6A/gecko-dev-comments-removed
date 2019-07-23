






































#ifndef _NS_ZIPEXTRACTOR_H_
#define _NS_ZIPEXTRACTOR_H_

#include "XIDefines.h"
#include "nsComponent.h"
#include "nsXInstaller.h"

#include <sys/stat.h>

class nsZipExtractor
{
public:
    nsZipExtractor(char *aSrc, char *aDest);
    ~nsZipExtractor();

    int     Extract(nsComponent *aXPIEngine, int aTotal);

private:
    int     DirCreateRecursive(char *aPath);

    char    *mSrc;
    char    *mDest;
};

#endif 
