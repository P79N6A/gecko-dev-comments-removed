





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
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool
  RecvNotify(const BluetoothSignal& aSignal);

  virtual bool
  RecvEnabled(const bool& aEnabled) override;

  virtual bool
  RecvBeginShutdown() override;

  virtual bool
  RecvNotificationsStopped() override;

  virtual PBluetoothRequestChild*
  AllocPBluetoothRequestChild(const Request& aRequest) override;

  virtual bool
  DeallocPBluetoothRequestChild(PBluetoothRequestChild* aActor) override;
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
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool
  Recv__delete__(const BluetoothReply& aReply) override;
};

END_BLUETOOTH_NAMESPACE

#endif 
