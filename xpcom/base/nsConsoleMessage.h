




































#ifndef __nsconsolemessage_h__
#define __nsconsolemessage_h__

#include "nsIConsoleMessage.h"
#include "nsString.h"

class nsConsoleMessage : public nsIConsoleMessage {
public:
    nsConsoleMessage();
    nsConsoleMessage(const PRUnichar *message);

    NS_DECL_ISUPPORTS
    NS_DECL_NSICONSOLEMESSAGE

private:
    ~nsConsoleMessage() {}

    nsString mMessage;
};

#endif 
