




































#ifndef ___nsscriptableinputstream___h_
#define ___nsscriptableinputstream___h_

#include "nsIScriptableInputStream.h"
#include "nsIInputStream.h"
#include "nsCOMPtr.h"

#define NS_SCRIPTABLEINPUTSTREAM_CID        \
{ 0x7225c040, 0xa9bf, 0x11d3, { 0xa1, 0x97, 0x0, 0x50, 0x4, 0x1c, 0xaf, 0x44 } }

#define NS_SCRIPTABLEINPUTSTREAM_CONTRACTID "@mozilla.org/scriptableinputstream;1"
#define NS_SCRIPTABLEINPUTSTREAM_CLASSNAME "Scriptable Input Stream"

class nsScriptableInputStream : public nsIScriptableInputStream {
public:
    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSISCRIPTABLEINPUTSTREAM

    
    nsScriptableInputStream() {}

    static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

private:
    ~nsScriptableInputStream() {}

    nsCOMPtr<nsIInputStream> mInputStream;
};

#endif 
