





#ifndef mozilla_system_mozmtpserver_h__
#define mozilla_system_mozmtpserver_h__

#include "MozMtpCommon.h"

#include "nsCOMPtr.h"
#include "nsIThread.h"

BEGIN_MTP_NAMESPACE

class MozMtpServer
{
public:
  NS_INLINE_DECL_REFCOUNTING(MozMtpServer)

  void Run();

private:
  nsCOMPtr<nsIThread> mServerThread;
};

END_MTP_NAMESPACE

#endif  


