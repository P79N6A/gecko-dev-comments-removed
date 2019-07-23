







































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

#include "windows.h"


#if (_MSC_VER == 1100)
#include <objbase.h>
DEFINE_OLEGUID(IID_IPersistFile, 0x0000010BL, 0, 0);
#endif

#include "shlobj.h"

#include <sys/stat.h>

class nsLocalFile : public nsILocalFileWin,
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

    
    NS_DECL_NSILOCALFILEWIN

    
    NS_DECL_NSIHASHABLE

    
    NS_DECL_NSICLASSINFO

public:
    static void GlobalInit();
    static void GlobalShutdown();

private:
    nsLocalFile(const nsLocalFile& other);
    ~nsLocalFile() {}

    PRPackedBool mDirty;            
    PRPackedBool mFollowSymlinks;   
    
    
    nsString mWorkingPath;
    
    
    
    nsString mResolvedPath;

    
    
    nsString mShortWorkingPath;

    PRFileInfo64 mFileInfo64;

    void MakeDirty() { mDirty = PR_TRUE; mShortWorkingPath.Truncate(); }

    nsresult ResolveAndStat();
    nsresult ResolveShortcut();

    void EnsureShortPath();
    
    nsresult CopyMove(nsIFile *newParentDir, const nsAString &newName,
                      PRBool followSymlinks, PRBool move);
    nsresult CopySingleFile(nsIFile *source, nsIFile* dest,
                            const nsAString &newName,
                            PRBool followSymlinks, PRBool move);

    nsresult SetModDate(PRInt64 aLastModifiedTime, const PRUnichar *filePath);
    nsresult HasFileAttribute(DWORD fileAttrib, PRBool *_retval);
    nsresult AppendInternal(const nsAFlatString &node,
                            PRBool multipleComponents);
};

#endif
