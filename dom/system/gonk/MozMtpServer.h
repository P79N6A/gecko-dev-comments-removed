





#ifndef mozilla_system_mozmtpserver_h__
#define mozilla_system_mozmtpserver_h__

#include "MozMtpCommon.h"
#include "MozMtpDatabase.h"

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIThread.h"

namespace mozilla {
namespace system {
  class Volume;
}
}

BEGIN_MTP_NAMESPACE
using namespace android;

class RefCountedMtpServer : public MtpServer
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(RefCountedMtpServer)

  RefCountedMtpServer(int aFd, MtpDatabase* aDatabase, bool aPtp,
                      int aFileGroup, int aFilePerm, int aDirectoryPerm)
    : MtpServer(aFd, aDatabase, aPtp, aFileGroup, aFilePerm, aDirectoryPerm)
  {
  }
};

class MozMtpServer
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MozMtpServer)

  void Run();



  already_AddRefed<RefCountedMtpServer> GetMtpServer();
  already_AddRefed<MozMtpDatabase> GetMozMtpDatabase();

private:
  nsRefPtr<RefCountedMtpServer> mMtpServer;
  nsRefPtr<MozMtpDatabase> mMozMtpDatabase;
  nsCOMPtr<nsIThread> mServerThread;
};

END_MTP_NAMESPACE

#endif  


