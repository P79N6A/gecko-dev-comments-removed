






































#ifndef __nsBidiKeyboard
#define __nsBidiKeyboard
#include "nsIBidiKeyboard.h"

class nsBidiKeyboard : public nsIBidiKeyboard
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIBIDIKEYBOARD

    nsBidiKeyboard();

protected:
    virtual ~nsBidiKeyboard();

    bool mHaveBidiKeyboards;
    nsresult SetHaveBidiKeyboards();
};

#endif 
