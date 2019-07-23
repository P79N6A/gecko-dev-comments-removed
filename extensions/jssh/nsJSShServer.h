





































#ifndef __NS_JSSHSERVER_H__
#define __NS_JSSHSERVER_H__

#include "nsIJSShServer.h"
#include "nsCOMPtr.h"
#include "nsIServerSocket.h"
#include "nsStringAPI.h"

class nsJSShServer : public nsIJSShServer
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIJSSHSERVER

  nsJSShServer();
  ~nsJSShServer();
private:
  nsCOMPtr<nsIServerSocket> mServerSocket;
  PRUint32 mServerPort;
  nsCString mServerStartupURI;
  PRBool mServerLoopbackOnly;
};

#endif 
