





































#ifndef nsBinaryStream_h___
#define nsBinaryStream_h___

#include "nsCOMPtr.h"
#include "nsAString.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsIStreamBufferAccess.h"

#define NS_BINARYOUTPUTSTREAM_CID        \
{ /* 86c37b9a-74e7-4672-844e-6e7dd83ba484 */         \
     0x86c37b9a,                                     \
     0x74e7,                                         \
     0x4672,                                         \
    {0x84, 0x4e, 0x6e, 0x7d, 0xd8, 0x3b, 0xa4, 0x84} \
}

#define NS_BINARYOUTPUTSTREAM_CONTRACTID "@mozilla.org/binaryoutputstream;1"
#define NS_BINARYOUTPUTSTREAM_CLASSNAME "Binary Output Stream"



class nsBinaryOutputStream : public nsIObjectOutputStream
{
public:
    nsBinaryOutputStream() {}
    
    virtual ~nsBinaryOutputStream() {}

protected:
    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIOUTPUTSTREAM

    
    NS_DECL_NSIBINARYOUTPUTSTREAM

    
    NS_DECL_NSIOBJECTOUTPUTSTREAM

    
    nsresult WriteFully(const char *aBuf, PRUint32 aCount);

    nsCOMPtr<nsIOutputStream>       mOutputStream;
    nsCOMPtr<nsIStreamBufferAccess> mBufferAccess;
};

#define NS_BINARYINPUTSTREAM_CID        \
{ /* c521a612-2aad-46db-b6ab-3b821fb150b1 */         \
     0xc521a612,                                     \
     0x2aad,                                         \
     0x46db,                                         \
    {0xb6, 0xab, 0x3b, 0x82, 0x1f, 0xb1, 0x50, 0xb1} \
}

#define NS_BINARYINPUTSTREAM_CONTRACTID "@mozilla.org/binaryinputstream;1"
#define NS_BINARYINPUTSTREAM_CLASSNAME "Binary Input Stream"



class nsBinaryInputStream : public nsIObjectInputStream
{
public:
    nsBinaryInputStream() {}
    
    virtual ~nsBinaryInputStream() {}

protected:
    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIINPUTSTREAM

    
    NS_DECL_NSIBINARYINPUTSTREAM

    
    NS_DECL_NSIOBJECTINPUTSTREAM

    nsCOMPtr<nsIInputStream>        mInputStream;
    nsCOMPtr<nsIStreamBufferAccess> mBufferAccess;
};

#endif 
