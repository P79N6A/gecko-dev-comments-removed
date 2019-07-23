



































#ifndef __nsftpdirlistingdconv__h__
#define __nsftpdirlistingdconv__h__

#include "nsIStreamConverter.h"
#include "nsIChannel.h"
#include "nsIURI.h"
#include "nsString.h"

#include "nsIFactory.h"

#define NS_FTPDIRLISTINGCONVERTER_CID                         \
{ /* 14C0E880-623E-11d3-A178-0050041CAF44 */         \
    0x14c0e880,                                      \
    0x623e,                                          \
    0x11d3,                                          \
    {0xa1, 0x78, 0x00, 0x50, 0x04, 0x1c, 0xaf, 0x44}       \
}

class nsFTPDirListingConv : public nsIStreamConverter {
public:
    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSISTREAMCONVERTER

    
    NS_DECL_NSISTREAMLISTENER

    
    NS_DECL_NSIREQUESTOBSERVER

    
    nsFTPDirListingConv();
    virtual ~nsFTPDirListingConv();
    nsresult Init();

private:
    
    nsresult GetHeaders(nsACString& str, nsIURI* uri);
    char*    DigestBufferLines(char *aBuffer, nsCString &aString);

    
    nsCAutoString       mBuffer;            
    PRBool              mSentHeading;       

    nsIStreamListener   *mFinalListener; 
};

#endif 
