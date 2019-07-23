










































#ifndef _nsLocalFileUNIX_H_
#define _nsLocalFileUNIX_H_

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "nscore.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsIHashable.h"
#include "nsIClassInfoImpl.h"




#ifdef HAVE_SYS_STATVFS_H
    #if defined(__osf__) && defined(__DECCXX)
        extern "C" int statvfs(const char *, struct statvfs *);
    #endif
    #include <sys/statvfs.h>
#endif

#ifdef HAVE_SYS_STATFS_H
    #include <sys/statfs.h>
#endif

#ifdef HAVE_STATVFS
    #define STATFS statvfs
#else
    #define STATFS statfs
#endif


#if defined(__FreeBSD__)
    #define HAVE_SYS_STATFS_H
    #define STATFS statfs
    #include <sys/param.h>
    #include <sys/mount.h>
#endif

class NS_COM nsLocalFile : public nsILocalFile,
                           public nsIHashable,
                           public nsIClassInfo
{
public:
    NS_DEFINE_STATIC_CID_ACCESSOR(NS_LOCAL_FILE_CID)
    
    nsLocalFile();

    static NS_METHOD nsLocalFileConstructor(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr);

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIFILE

    
    NS_DECL_NSILOCALFILE

    
    NS_DECL_NSIHASHABLE

    
    NS_DECL_NSICLASSINFO

public:
    static void GlobalInit();
    static void GlobalShutdown();

private:
    nsLocalFile(const nsLocalFile& other);
    ~nsLocalFile() {}

protected:
    struct stat  mCachedStat;
    nsCString    mPath;
    PRPackedBool mHaveCachedStat;

    void LocateNativeLeafName(nsACString::const_iterator &,
                              nsACString::const_iterator &);

    nsresult CopyDirectoryTo(nsIFile *newParent);
    nsresult CreateAllAncestors(PRUint32 permissions);
    nsresult GetNativeTargetPathName(nsIFile *newParent,
                                     const nsACString &newName,
                                     nsACString &_retval);

    void InvalidateCache() {
        mHaveCachedStat = PR_FALSE;
    }
    nsresult FillStatCache();

    nsresult CreateAndKeepOpen(PRUint32 type, PRIntn flags,
                               PRUint32 permissions, PRFileDesc **_retval);
};

#endif 
