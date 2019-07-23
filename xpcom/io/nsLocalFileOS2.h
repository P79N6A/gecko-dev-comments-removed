














































#ifndef _nsLocalFileOS2_H_
#define _nsLocalFileOS2_H_

#include "nscore.h"
#include "nsError.h"
#include "nsString.h"
#include "nsCRT.h"
#include "nsIFile.h"
#include "nsIFactory.h"
#include "nsILocalFileOS2.h"
#include "nsIHashable.h"

#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#define INCL_DOSPROCESS
#define INCL_DOSSESMGR
#define INCL_DOSMODULEMGR
#define INCL_DOSNLS
#define INCL_DOSMISC
#define INCL_WINCOUNTRY
#define INCL_WINWORKPLACE

#include <os2.h>

class TypeEaEnumerator;

class NS_COM nsLocalFile : public nsILocalFileOS2,
                                   public nsIHashable
{
public:
    NS_DEFINE_STATIC_CID_ACCESSOR(NS_LOCAL_FILE_CID)

    nsLocalFile();

    static NS_METHOD nsLocalFileConstructor(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr);

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIFILE

    
    NS_DECL_NSILOCALFILE

    
    NS_DECL_NSILOCALFILEOS2

    
    NS_DECL_NSIHASHABLE

public:
    static void GlobalInit();
    static void GlobalShutdown();

private:
    nsLocalFile(const nsLocalFile& other);
    ~nsLocalFile() {}

    
    PRPackedBool mDirty;

    
    nsCString mWorkingPath;

    PRFileInfo64  mFileInfo64;

    void MakeDirty() { mDirty = PR_TRUE; }

    nsresult Stat();

    nsresult CopyMove(nsIFile *newParentDir, const nsACString &newName, PRBool move);
    nsresult CopySingleFile(nsIFile *source, nsIFile* dest, const nsACString &newName, PRBool move);

    nsresult SetModDate(PRInt64 aLastModifiedTime);
    nsresult AppendNativeInternal(const nsAFlatCString &node, PRBool multipleComponents);

    nsresult GetEA(const char *eaName, PFEA2LIST pfea2list);
    friend class TypeEaEnumerator;
};

#endif
