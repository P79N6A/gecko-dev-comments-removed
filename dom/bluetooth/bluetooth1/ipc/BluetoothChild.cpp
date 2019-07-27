





#include "base/basictypes.h"

#include "BluetoothChild.h"

#include "mozilla/Assertions.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/StaticPtr.h"
#include "nsDebug.h"
#include "nsISupportsImpl.h"
#include "nsThreadUtils.h"

#include "BluetoothReplyRunnable.h"
#include "BluetoothService.h"
#include "BluetoothServiceChildProcess.h"

using namespace mozilla;
USING_BLUETOOTH_NAMESPACE

namespace {

StaticRefPtr<BluetoothServiceChildProcess> sBluetoothService;

} 





BluetoothChild::BluetoothChild(BluetoothServiceChildProcess* aBluetoothService)
: mShutdownState(Running)
{
  MOZ_COUNT_CTOR(BluetoothChild);
  MOZ_ASSERT(!sBluetoothService);
  MOZ_ASSERT(aBluetoothService);

  sBluetoothService = aBluetoothService;
  ClearOnShutdown(&sBluetoothService);
}

BluetoothChild::~BluetoothChild()
{
  MOZ_COUNT_DTOR(BluetoothChild);
  MOZ_ASSERT(sBluetoothService);
  MOZ_ASSERT(mShutdownState == Dead);

  sBluetoothService = nullptr;
}

void
BluetoothChild::BeginShutdown()
{
  
  if (mShutdownState == Running) {
    SendStopNotifying();
    mShutdownState = SentStopNotifying;
  }
}

void
BluetoothChild::ActorDestroy(ActorDestroyReason aWhy)
{
  MOZ_ASSERT(sBluetoothService);

  sBluetoothService->NoteDeadActor();

#ifdef DEBUG
  mShutdownState = Dead;
#endif
}

bool
BluetoothChild::RecvNotify(const BluetoothSignal& aSignal)
{
  MOZ_ASSERT(sBluetoothService);

  if (sBluetoothService) {
    sBluetoothService->DistributeSignal(aSignal);
  }
  return true;
}

bool
BluetoothChild::RecvEnabled(const bool& aEnabled)
{
  MOZ_ASSERT(sBluetoothService);

  if (sBluetoothService) {
    sBluetoothService->SetEnabled(aEnabled);
  }
  return true;
}

bool
BluetoothChild::RecvBeginShutdown()
{
  if (mShutdownState != Running && mShutdownState != SentStopNotifying) {
    MOZ_ASSERT(false, "Bad state!");
    return false;
  }

  SendStopNotifying();
  mShutdownState = SentStopNotifying;

  return true;
}

bool
BluetoothChild::RecvNotificationsStopped()
{
  if (mShutdownState != SentStopNotifying) {
    MOZ_ASSERT(false, "Bad state!");
    return false;
  }

  Send__delete__(this);
  return true;
}

PBluetoothRequestChild*
BluetoothChild::AllocPBluetoothRequestChild(const Request& aRequest)
{
  MOZ_CRASH("Caller is supposed to manually construct a request!");
}

bool
BluetoothChild::DeallocPBluetoothRequestChild(PBluetoothRequestChild* aActor)
{
  delete aActor;
  return true;
}





BluetoothRequestChild::BluetoothRequestChild(
                                         BluetoothReplyRunnable* aReplyRunnable)
: mReplyRunnable(aReplyRunnable)
{
  MOZ_COUNT_CTOR(BluetoothRequestChild);
  MOZ_ASSERT(aReplyRunnable);
}

BluetoothRequestChild::~BluetoothRequestChild()
{
  MOZ_COUNT_DTOR(BluetoothRequestChild);
}

void
BluetoothRequestChild::ActorDestroy(ActorDestroyReason aWhy)
{
  
}

bool
BluetoothRequestChild::Recv__delete__(const BluetoothReply& aReply)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mReplyRunnable);

  nsRefPtr<BluetoothReplyRunnable> replyRunnable;
  mReplyRunnable.swap(replyRunnable);

  if (replyRunnable) {
    
    replyRunnable->SetReply(new BluetoothReply(aReply));
    return NS_SUCCEEDED(NS_DispatchToCurrentThread(replyRunnable));
  }

  return true;
}
