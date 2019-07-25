




#ifndef _nsLocalFileWIN_H_
#define _nsLocalFileWIN_H_

#include "nscore.h"
#include "nsError.h"
#include "nsString.h"
#include "nsCRT.h"
#include "nsIFile.h"
#include "nsIFactory.h"
#include "nsILocalFileWin.h"
#include "nsIHashable.h"
#include "nsIClassInfoImpl.h"

#include "mozilla/Attributes.h"

#include "windows.h"
#include "shlobj.h"

#include <sys/stat.h>

class nsLocalFile MOZ_FINAL : public nsILocalFileWin,
                              public nsIHashable
{
public:
    NS_DEFINE_STATIC_CID_ACCESSOR(NS_LOCAL_FILE_CID)
    
    nsLocalFile();

    static nsresult nsLocalFileConstructor(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr);

    
    NS_DECL_ISUPPORTS
    
    
    NS_DECL_NSIFILE
    
    
    NS_DECL_NSILOCALFILE

    
    NS_DECL_NSILOCALFILEWIN

    
    NS_DECL_NSIHASHABLE

public:
    static void GlobalInit();
    static void GlobalShutdown();

private:
    nsLocalFile(const nsLocalFile& other);
    ~nsLocalFile() {}

    bool mDirty;            
    bool mResolveDirty;
    bool mFollowSymlinks;   
    
    
    nsString mWorkingPath;
    
    
    
    nsString mResolvedPath;

    
    
    nsString mShortWorkingPath;

    PRFileInfo64 mFileInfo64;

    void MakeDirty() 
    { 
      mDirty = true;
      mResolveDirty = true;
      mShortWorkingPath.Truncate();
    }

    nsresult ResolveAndStat();
    nsresult Resolve();
    nsresult ResolveShortcut();

    void EnsureShortPath();
    
    nsresult CopyMove(nsIFile *newParentDir, const nsAString &newName,
                      bool followSymlinks, bool move);
    nsresult CopySingleFile(nsIFile *source, nsIFile* dest,
                            const nsAString &newName,
                            bool followSymlinks, bool move,
                            bool skipNtfsAclReset = false);

    nsresult SetModDate(int64_t aLastModifiedTime, const PRUnichar *filePath);
    nsresult HasFileAttribute(DWORD fileAttrib, bool *_retval);
    nsresult AppendInternal(const nsAFlatString &node,
                            bool multipleComponents);
};

#endif
