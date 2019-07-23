








































#ifndef __nsgopherdirlistingconv__h__
#define __nsgopherdirlistingconv__h__

#include "nspr.h"
#include "prtypes.h"
#include "nsIStreamConverter.h"
#include "nsIChannel.h"
#include "nsString.h"
#include "nsIChannel.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"

#include "nsIFactory.h"

#define NS_GOPHERDIRLISTINGCONVERTER_CID \
 { /* ea617873-3b73-4efd-a2c4-fc39bfab809d */ \
    0xea617873, \
    0x3b73, \
    0x4efd, \
    { 0xa2, 0xc4, 0xfc, 0x39, 0xbf, 0xab, 0x80, 0x9d} \
}

#define GOPHER_PORT 70
 
class nsGopherDirListingConv : public nsIStreamConverter {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISTREAMCONVERTER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIREQUESTOBSERVER

    nsGopherDirListingConv();

private:
    char* DigestBufferLines(char *aBuffer, nsCAutoString& aString);

    nsCString                   mBuffer;        
    PRBool                      mSentHeading;
    nsCOMPtr<nsIStreamListener> mFinalListener; 
};  

#endif 
