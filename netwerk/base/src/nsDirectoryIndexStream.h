




































#ifndef nsDirectoryIndexStream_h__
#define nsDirectoryIndexStream_h__

#include "nsIFile.h"
#include "nsString.h"
#include "nsIInputStream.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsITextToSubURI.h"

class nsDirectoryIndexStream : public nsIInputStream
{
private:
    nsCAutoString mBuf;
    PRInt32 mOffset;
    nsresult mStatus;

    PRInt32             mPos;   
    nsCOMArray<nsIFile> mArray; 

    nsDirectoryIndexStream();
    

 
    nsresult Init(nsIFile* aDir);
    ~nsDirectoryIndexStream();

public:
    

 
    static nsresult
    Create(nsIFile* aDir, nsIInputStream** aStreamResult);

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIINPUTSTREAM
};

#endif

