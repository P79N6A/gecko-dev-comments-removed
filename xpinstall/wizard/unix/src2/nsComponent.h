






































#ifndef _NS_COMPONENT_H_
#define _NS_COMPONENT_H_

#include "XIDefines.h"
#include "XIErrors.h"
#include <malloc.h>

#include "nsComponentList.h"

class nsComponent
{
public:
    nsComponent();
    ~nsComponent();

    nsComponent *   Duplicate();




    int             SetDescShort(char *aDescShort);
    char *          GetDescShort();
    int             SetDescLong(char *aDescLong);
    char *          GetDescLong();
    int             SetArchive(char *aAcrhive);
    char *          GetArchive();
    int             SetInstallSize(int aInstallSize);
    int             GetInstallSize();
    int             SetArchiveSize(int aArchiveSize);
    int             GetArchiveSize();
    int             GetCurrentSize();
    int             SetURL(char *aURL, int aIndex);
    char *          GetURL(int aIndex);
    int             AddDependee(char *aDependee); 
    int             ResolveDependees(int aBeingSelected, 
                                     nsComponentList *aComps);
    int             SetSelected();
    int             SetUnselected();
    int             IsSelected();
    int             SetInvisible();
    int             SetVisible();
    int             IsInvisible(); 
    int             SetLaunchApp();
    int             SetDontLaunchApp();
    int             IsLaunchApp();
    int             SetDownloadOnly();
    int             IsDownloadOnly();
    int             SetIndex(int aIndex);
    int             GetIndex();
    int             AddRef();
    int             Release();
    int             InitRefCount();

    
    int             DepAddRef();
    int             DepRelease();
    int             DepGetRefCount();
    int             SetResumePos(int aResPos);
    int             GetResumePos();
    int             SetDownloaded(int which);
    int             IsDownloaded();
  



    enum 
    {
        NO_ATTR         = 0x00000000,
        SELECTED        = 0x00000001,
        INVISIBLE       = 0x00000010,
        LAUNCHAPP       = 0x00000100,
        DOWNLOAD_ONLY   = 0x00001000
    };

private:
    char            *mDescShort;
    char            *mDescLong;
    char            *mArchive;
    int             mInstallSize;
    int             mArchiveSize;
    char            *mURL[MAX_URLS];
    char            *mDependees[MAX_COMPONENTS];
    int             mNextDependeeIdx;
    int             mAttributes;
    int             mIndex;
    int             mRefCount;
    int             mDepRefCount;
    int             mResPos;
    int             mDownloaded;
};

#endif 
