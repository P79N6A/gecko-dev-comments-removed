






































#ifndef _NSSSLSOCKETPROVIDER_H_
#define _NSSSLSOCKETPROVIDER_H_

#include "nsISocketProvider.h"

#define NS_SSLSOCKETPROVIDER_CLASSNAME  "Mozilla SSL Socket Provider Component"

#define NS_SSLSOCKETPROVIDER_CID   \
{ 0x217d014a, 0x1dd2, 0x11b2, {0x99, 0x9c, 0xb0, 0xc4, 0xdf, 0x79, 0xb3, 0x24}}


class nsSSLSocketProvider : public nsISocketProvider
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISOCKETPROVIDER
  
  
  nsSSLSocketProvider();
  virtual ~nsSSLSocketProvider();
};

#endif 
