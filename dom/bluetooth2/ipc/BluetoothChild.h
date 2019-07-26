





#ifndef mozilla_dom_bluetooth_ipc_bluetoothchild_h__
#define mozilla_dom_bluetooth_ipc_bluetoothchild_h__

#include "mozilla/dom/bluetooth/BluetoothCommon.h"

#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/dom/bluetooth/PBluetoothChild.h"
#include "mozilla/dom/bluetooth/PBluetoothRequestChild.h"

#include "mozilla/Attributes.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace dom {
namespace bluetooth {

class BluetoothServiceChildProcess;

} 
} 
} 

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothReplyRunnable;





class BluetoothChild : public PBluetoothChild
{
  friend class mozilla::dom::bluetooth::BluetoothServiceChildProcess;

  enum ShutdownState
  {
    Running = 0,
    SentStopNotifying,
    ReceivedNotificationsStopped,
    Dead
  };

  ShutdownState mShutdownState;

protected:
  BluetoothChild(BluetoothServiceChildProcess* aBluetoothService);
  virtual ~BluetoothChild();

  void
  BeginShutdown();

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool
  RecvNotify(const BluetoothSignal& aSignal);

  virtual bool
  RecvEnabled(const bool& aEnabled) MOZ_OVERRIDE;

  virtual bool
  RecvBeginShutdown() MOZ_OVERRIDE;

  virtual bool
  RecvNotificationsStopped() MOZ_OVERRIDE;

  virtual PBluetoothRequestChild*
  AllocPBluetoothRequestChild(const Request& aRequest) MOZ_OVERRIDE;

  virtual bool
  DeallocPBluetoothRequestChild(PBluetoothRequestChild* aActor) MOZ_OVERRIDE;
};





class BluetoothRequestChild : public PBluetoothRequestChild
{
  friend class mozilla::dom::bluetooth::BluetoothChild;

  nsRefPtr<BluetoothReplyRunnable> mReplyRunnable;

public:
  BluetoothRequestChild(BluetoothReplyRunnable* aReplyRunnable);

protected:
  virtual ~BluetoothRequestChild();

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool
  Recv__delete__(const BluetoothReply& aReply) MOZ_OVERRIDE;
};

END_BLUETOOTH_NAMESPACE

#endif 
