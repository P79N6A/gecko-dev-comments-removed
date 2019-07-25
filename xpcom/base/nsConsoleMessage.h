




































#ifndef __nsconsolemessage_h__
#define __nsconsolemessage_h__

#include "mozilla/Attributes.h"

#include "nsIConsoleMessage.h"
#include "nsString.h"

class nsConsoleMessage MOZ_FINAL : public nsIConsoleMessage {
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
