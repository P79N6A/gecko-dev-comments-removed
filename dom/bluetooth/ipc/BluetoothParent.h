



#ifndef mozilla_dom_bluetooth_ipc_bluetoothparent_h__
#define mozilla_dom_bluetooth_ipc_bluetoothparent_h__

#include "mozilla/dom/bluetooth/BluetoothCommon.h"

#include "mozilla/dom/bluetooth/PBluetoothParent.h"
#include "mozilla/dom/bluetooth/PBluetoothRequestParent.h"

#include "mozilla/Attributes.h"
#include "mozilla/Observer.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"

template <class T>
class nsRevocableEventPtr;

namespace mozilla {
namespace dom {

class ContentParent;

} 
} 

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothService;





class BluetoothParent : public PBluetoothParent,
                        public mozilla::Observer<BluetoothSignal>
{
  friend class mozilla::dom::ContentParent;

  enum ShutdownState
  {
    Running = 0,
    SentBeginShutdown,
    ReceivedStopNotifying,
    SentNotificationsStopped,
    Dead
  };

  nsRefPtr<BluetoothService> mService;
  ShutdownState mShutdownState;
  bool mReceivedStopNotifying;
  bool mSentBeginShutdown;

public:
  void
  BeginShutdown();

protected:
  BluetoothParent();
  virtual ~BluetoothParent();

  bool
  InitWithService(BluetoothService* aService);

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool
  RecvRegisterSignalHandler(const nsString& aNode) MOZ_OVERRIDE;

  virtual bool
  RecvUnregisterSignalHandler(const nsString& aNode) MOZ_OVERRIDE;

  virtual bool
  RecvStopNotifying() MOZ_OVERRIDE;

  virtual bool
  RecvPBluetoothRequestConstructor(PBluetoothRequestParent* aActor,
                                   const Request& aRequest) MOZ_OVERRIDE;

  virtual PBluetoothRequestParent*
  AllocPBluetoothRequest(const Request& aRequest) MOZ_OVERRIDE;

  virtual bool
  DeallocPBluetoothRequest(PBluetoothRequestParent* aActor) MOZ_OVERRIDE;

  virtual void
  Notify(const BluetoothSignal& aSignal) MOZ_OVERRIDE;

private:
  void
  UnregisterAllSignalHandlers();
};





class BluetoothRequestParent : public PBluetoothRequestParent
{
  class ReplyRunnable;
  friend class BluetoothParent;

  friend class ReplyRunnable;

  nsRefPtr<BluetoothService> mService;
  nsRevocableEventPtr<ReplyRunnable> mReplyRunnable;

#ifdef DEBUG
  Request::Type mRequestType;
#endif

protected:
  BluetoothRequestParent(BluetoothService* aService);
  virtual ~BluetoothRequestParent();

  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  void
  RequestComplete();

  bool
  DoRequest(const DefaultAdapterPathRequest& aRequest);

  bool
  DoRequest(const SetPropertyRequest& aRequest);

  bool
  DoRequest(const GetPropertyRequest& aRequest);

  bool
  DoRequest(const StartDiscoveryRequest& aRequest);

  bool
  DoRequest(const StopDiscoveryRequest& aRequest);

  bool
  DoRequest(const PairRequest& aRequest);

  bool
  DoRequest(const UnpairRequest& aRequest);

  bool
  DoRequest(const DevicePropertiesRequest& aRequest);
};

END_BLUETOOTH_NAMESPACE

#endif 
