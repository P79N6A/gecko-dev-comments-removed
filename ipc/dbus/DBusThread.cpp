






















#include "DBusThread.h"
#include "RawDBusConnection.h"
#include "base/message_loop.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"

namespace mozilla {
namespace ipc {

class WatchDBusConnectionTask : public Task
{
public:
  WatchDBusConnectionTask(RawDBusConnection* aConnection)
  : mConnection(aConnection)
  {
    MOZ_ASSERT(mConnection);
  }

  void Run()
  {
    mConnection->Watch();
  }

private:
  RawDBusConnection* mConnection;
};

class DeleteDBusConnectionTask : public Task
{
public:
  DeleteDBusConnectionTask(RawDBusConnection* aConnection)
  : mConnection(aConnection)
  {
    MOZ_ASSERT(mConnection);
  }

  void Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());

    
    
    delete mConnection;
  }

private:
  RawDBusConnection* mConnection;
};

 

static RawDBusConnection* gDBusConnection;

bool
StartDBus()
{
  MOZ_ASSERT(!NS_IsMainThread());
  NS_ENSURE_TRUE(!gDBusConnection, true);

  RawDBusConnection* connection = new RawDBusConnection();
  nsresult rv = connection->EstablishDBusConnection();
  NS_ENSURE_SUCCESS(rv, false);

  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
    new WatchDBusConnectionTask(connection));

  gDBusConnection = connection;

  return true;
}

bool
StopDBus()
{
  MOZ_ASSERT(!NS_IsMainThread());
  NS_ENSURE_TRUE(gDBusConnection, true);

  RawDBusConnection* connection = gDBusConnection;
  gDBusConnection = nullptr;

  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
    new DeleteDBusConnectionTask(connection));

  return true;
}

nsresult
DispatchToDBusThread(Task* task)
{
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE, task);

  return NS_OK;
}

RawDBusConnection*
GetDBusConnection()
{
  NS_ENSURE_TRUE(gDBusConnection, nullptr);

  return gDBusConnection;
}

}
}
