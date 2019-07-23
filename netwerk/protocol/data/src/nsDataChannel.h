







































#ifndef nsDataChannel_h___
#define nsDataChannel_h___

#include "nsBaseChannel.h"
#include "nsIInputStream.h"
#include "nsCOMPtr.h"

class nsDataChannel : public nsBaseChannel {
public:
    nsDataChannel(nsIURI *uri) {
        SetURI(uri);
    }

protected:
    virtual nsresult OpenContentStream(PRBool async, nsIInputStream **result);
};

#endif 
