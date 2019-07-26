




#ifndef nsDeviceProtocolHandler_h_
#define nsDeviceProtocolHandler_h_

#include "nsIProtocolHandler.h"
#include "mozilla/Attributes.h"


#define NS_DEVICEPROTOCOLHANDLER_CID                      \
{ 0x60ffe9e, 0xd114, 0x486b,                              \
    {0xae, 0xb7, 0xda, 0x62, 0xe7, 0x27, 0x3e, 0xd5} }

class nsDeviceProtocolHandler MOZ_FINAL : public nsIProtocolHandler {
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIPROTOCOLHANDLER

  nsDeviceProtocolHandler() {}
  ~nsDeviceProtocolHandler() {}

  nsresult Init();
};

#endif
