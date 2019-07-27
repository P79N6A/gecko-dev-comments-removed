






#ifndef nsDataChannel_h___
#define nsDataChannel_h___

#include "nsBaseChannel.h"

class nsIInputStream;

class nsDataChannel : public nsBaseChannel {
public:
    explicit nsDataChannel(nsIURI *uri) {
        SetURI(uri);
    }

protected:
    virtual nsresult OpenContentStream(bool async, nsIInputStream **result,
                                       nsIChannel** channel);
};

#endif 
