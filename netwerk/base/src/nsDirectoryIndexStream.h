




#ifndef nsDirectoryIndexStream_h__
#define nsDirectoryIndexStream_h__

#include "mozilla/Attributes.h"

#include "nsIFile.h"
#include "nsString.h"
#include "nsIInputStream.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsITextToSubURI.h"

class nsDirectoryIndexStream MOZ_FINAL : public nsIInputStream
{
private:
    nsCString mBuf;
    int32_t mOffset;
    nsresult mStatus;

    int32_t             mPos;   
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

