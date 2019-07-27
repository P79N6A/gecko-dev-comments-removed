





#ifndef mozilla_system_mozmtpserver_h__
#define mozilla_system_mozmtpserver_h__

#include "MozMtpCommon.h"
#include "MozMtpDatabase.h"

#include "mozilla/FileUtils.h"

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIThread.h"

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

protected:
  virtual ~RefCountedMtpServer() {}
};

class MozMtpServer
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MozMtpServer)

  bool Init();
  void Run();

  already_AddRefed<RefCountedMtpServer> GetMtpServer();
  already_AddRefed<MozMtpDatabase> GetMozMtpDatabase();

protected:
  virtual ~MozMtpServer() {}

private:
  nsRefPtr<RefCountedMtpServer> mMtpServer;
  nsRefPtr<MozMtpDatabase> mMozMtpDatabase;
  nsCOMPtr<nsIThread> mServerThread;
  ScopedClose mMtpUsbFd;
};

END_MTP_NAMESPACE

#endif  


