



#ifndef nsSecurityConsoleMessage_h__
#define nsSecurityConsoleMessage_h__
#include "nsISecurityConsoleMessage.h"
#include "nsString.h"

class nsSecurityConsoleMessage MOZ_FINAL : public nsISecurityConsoleMessage
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISECURITYCONSOLEMESSAGE

    nsSecurityConsoleMessage();

  private:
    ~nsSecurityConsoleMessage();

  protected:
    nsString mTag;
    nsString mCategory;
};

#define NS_SECURITY_CONSOLE_MESSAGE_CID \
  {0x43ebf210, 0x8a7b, 0x4ddb, {0xa8, 0x3d, 0xb8, 0x7c, 0x51, 0xa0, 0x58, 0xdb}}
#endif 
