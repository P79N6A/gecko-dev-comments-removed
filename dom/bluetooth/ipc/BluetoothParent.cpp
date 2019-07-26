





#include "base/basictypes.h"

#include "BluetoothParent.h"

#include "mozilla/Assertions.h"
#include "mozilla/unused.h"
#include "mozilla/Util.h"
#include "nsDebug.h"
#include "nsThreadUtils.h"
#include "nsTraceRefcnt.h"

#include "BluetoothReplyRunnable.h"
#include "BluetoothService.h"

using mozilla::unused;
USING_BLUETOOTH_NAMESPACE





class BluetoothRequestParent::ReplyRunnable : public BluetoothReplyRunnable
{
  BluetoothRequestParent* mRequest;

public:
  ReplyRunnable(BluetoothRequestParent* aRequest)
  : BluetoothReplyRunnable(nullptr), mRequest(aRequest)
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(aRequest);
  }

  NS_IMETHOD
  Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mReply);

    if (mRequest) {
      
      mRequest->RequestComplete();

      if (!mRequest->Send__delete__(mRequest, *mReply)) {
        NS_WARNING("Failed to send response to child process!");
        return NS_ERROR_FAILURE;
      }
    }

    ReleaseMembers();
    return NS_OK;
  }

  void
  Revoke()
  {
    MOZ_ASSERT(NS_IsMainThread());
    mRequest = nullptr;
  }

  virtual bool
  ParseSuccessfulReply(jsval* aValue) MOZ_OVERRIDE
  {
    MOZ_NOT_REACHED("This should never be called!");
    return false;
  }
};





BluetoothParent::BluetoothParent()
: mShutdownState(Running)
{
  MOZ_COUNT_CTOR(BluetoothParent);
}

BluetoothParent::~BluetoothParent()
{
  MOZ_COUNT_DTOR(BluetoothParent);
  MOZ_ASSERT(!mService);
  MOZ_ASSERT(mShutdownState == Dead);
}

void
BluetoothParent::BeginShutdown()
{
  
  if (mShutdownState == Running) {
    unused << SendBeginShutdown();
    mShutdownState = SentBeginShutdown;
  }
}

bool
BluetoothParent::InitWithService(BluetoothService* aService)
{
  MOZ_ASSERT(aService);
  MOZ_ASSERT(!mService);

  if (!SendEnabled(aService->IsEnabled())) {
    return false;
  }

  mService = aService;
  return true;
}

void
BluetoothParent::UnregisterAllSignalHandlers()
{
  MOZ_ASSERT(mService);
  mService->UnregisterAllSignalHandlers(this);
}

void
BluetoothParent::ActorDestroy(ActorDestroyReason aWhy)
{
  if (mService) {
    UnregisterAllSignalHandlers();
#ifdef DEBUG
    mService = nullptr;
#endif
  }

#ifdef DEBUG
  mShutdownState = Dead;
#endif
}

bool
BluetoothParent::RecvRegisterSignalHandler(const nsString& aNode)
{
  MOZ_ASSERT(mService);
  mService->RegisterBluetoothSignalHandler(aNode, this);
  return true;
}

bool
BluetoothParent::RecvUnregisterSignalHandler(const nsString& aNode)
{
  MOZ_ASSERT(mService);
  mService->UnregisterBluetoothSignalHandler(aNode, this);
  return true;
}

bool
BluetoothParent::RecvStopNotifying()
{
  MOZ_ASSERT(mService);

  if (mShutdownState != Running && mShutdownState != SentBeginShutdown) {
    MOZ_ASSERT(false, "Bad state!");
    return false;
  }

  mShutdownState = ReceivedStopNotifying;

  UnregisterAllSignalHandlers();

  if (SendNotificationsStopped()) {
    mShutdownState = SentNotificationsStopped;
    return true;
  }

  return false;
}

bool
BluetoothParent::RecvPBluetoothRequestConstructor(
                                                PBluetoothRequestParent* aActor,
                                                const Request& aRequest)
{
  BluetoothRequestParent* actor = static_cast<BluetoothRequestParent*>(aActor);

#ifdef DEBUG
  actor->mRequestType = aRequest.type();
#endif

  switch (aRequest.type()) {
    case Request::TDefaultAdapterPathRequest:
      return actor->DoRequest(aRequest.get_DefaultAdapterPathRequest());
    case Request::TSetPropertyRequest:
      return actor->DoRequest(aRequest.get_SetPropertyRequest());
    case Request::TGetPropertyRequest:
      return actor->DoRequest(aRequest.get_GetPropertyRequest());
    case Request::TStartDiscoveryRequest:
      return actor->DoRequest(aRequest.get_StartDiscoveryRequest());
    case Request::TStopDiscoveryRequest:
      return actor->DoRequest(aRequest.get_StopDiscoveryRequest());
    case Request::TPairRequest:
      return actor->DoRequest(aRequest.get_PairRequest());
    case Request::TUnpairRequest:
      return actor->DoRequest(aRequest.get_UnpairRequest());
    case Request::TDevicePropertiesRequest:
      return actor->DoRequest(aRequest.get_DevicePropertiesRequest());
    case Request::TSetPinCodeRequest:
      return actor->DoRequest(aRequest.get_SetPinCodeRequest());
    case Request::TSetPasskeyRequest:
      return actor->DoRequest(aRequest.get_SetPasskeyRequest());
    case Request::TConfirmPairingConfirmationRequest:
      return actor->DoRequest(aRequest.get_ConfirmPairingConfirmationRequest());
    case Request::TConfirmAuthorizationRequest:
      return actor->DoRequest(aRequest.get_ConfirmAuthorizationRequest());
    case Request::TDenyPairingConfirmationRequest:
      return actor->DoRequest(aRequest.get_DenyPairingConfirmationRequest());
    case Request::TDenyAuthorizationRequest:
      return actor->DoRequest(aRequest.get_DenyAuthorizationRequest());
    case Request::TConnectHeadsetRequest:
      return actor->DoRequest(aRequest.get_ConnectHeadsetRequest());
    case Request::TConnectObjectPushRequest:
      return actor->DoRequest(aRequest.get_ConnectObjectPushRequest());
    case Request::TDisconnectHeadsetRequest:
      return actor->DoRequest(aRequest.get_DisconnectHeadsetRequest());
    case Request::TDisconnectObjectPushRequest:
      return actor->DoRequest(aRequest.get_DisconnectObjectPushRequest());
    default:
      MOZ_NOT_REACHED("Unknown type!");
      return false;
  }

  MOZ_NOT_REACHED("Should never get here!");
  return false;
}

PBluetoothRequestParent*
BluetoothParent::AllocPBluetoothRequest(const Request& aRequest)
{
  MOZ_ASSERT(mService);
  return new BluetoothRequestParent(mService);
}

bool
BluetoothParent::DeallocPBluetoothRequest(PBluetoothRequestParent* aActor)
{
  delete aActor;
  return true;
}

void
BluetoothParent::Notify(const BluetoothSignal& aSignal)
{
  unused << SendNotify(aSignal);
}





BluetoothRequestParent::BluetoothRequestParent(BluetoothService* aService)
: mService(aService)
#ifdef DEBUG
  , mRequestType(Request::T__None)
#endif
{
  MOZ_COUNT_CTOR(BluetoothRequestParent);
  MOZ_ASSERT(aService);

  mReplyRunnable = new ReplyRunnable(this);
}

BluetoothRequestParent::~BluetoothRequestParent()
{
  MOZ_COUNT_DTOR(BluetoothRequestParent);

  
}

void
BluetoothRequestParent::ActorDestroy(ActorDestroyReason aWhy)
{
  mReplyRunnable.Revoke();
}

void
BluetoothRequestParent::RequestComplete()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mReplyRunnable.IsPending());

  mReplyRunnable.Forget();
}

bool
BluetoothRequestParent::DoRequest(const DefaultAdapterPathRequest& aRequest)
{
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TDefaultAdapterPathRequest);

  nsresult rv = mService->GetDefaultAdapterPathInternal(mReplyRunnable.get());
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

bool
BluetoothRequestParent::DoRequest(const SetPropertyRequest& aRequest)
{
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TSetPropertyRequest);

  nsresult rv =
    mService->SetProperty(aRequest.type(), aRequest.path(), aRequest.value(),
                          mReplyRunnable.get());
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

bool
BluetoothRequestParent::DoRequest(const GetPropertyRequest& aRequest)
{
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TGetPropertyRequest);

  nsresult rv =
    mService->GetProperties(aRequest.type(), aRequest.path(),
                            mReplyRunnable.get());
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

bool
BluetoothRequestParent::DoRequest(const StartDiscoveryRequest& aRequest)
{
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TStartDiscoveryRequest);

  nsresult rv =
    mService->StartDiscoveryInternal(aRequest.path(), mReplyRunnable.get());
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

bool
BluetoothRequestParent::DoRequest(const StopDiscoveryRequest& aRequest)
{
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TStopDiscoveryRequest);

  nsresult rv =
    mService->StopDiscoveryInternal(aRequest.path(), mReplyRunnable.get());
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

bool
BluetoothRequestParent::DoRequest(const PairRequest& aRequest)
{
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TPairRequest);

  nsresult rv =
    mService->CreatePairedDeviceInternal(aRequest.path(), aRequest.address(),
                                         aRequest.timeoutMS(),
                                         mReplyRunnable.get());
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

bool
BluetoothRequestParent::DoRequest(const UnpairRequest& aRequest)
{
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TUnpairRequest);

  nsresult rv =
    mService->RemoveDeviceInternal(aRequest.path(), aRequest.address(),
                                   mReplyRunnable.get());
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

bool
BluetoothRequestParent::DoRequest(const DevicePropertiesRequest& aRequest)
{
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TDevicePropertiesRequest);

  nsresult rv =
    mService->GetPairedDevicePropertiesInternal(aRequest.addresses(),
                                                mReplyRunnable.get());
  NS_ENSURE_SUCCESS(rv, false);

  return true;
}

bool
BluetoothRequestParent::DoRequest(const SetPinCodeRequest& aRequest)
{
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TSetPinCodeRequest);

  bool result =
    mService->SetPinCodeInternal(aRequest.path(),
                                 aRequest.pincode(),
                                 mReplyRunnable.get());

  NS_ENSURE_TRUE(result, false);

  return true;
}

bool
BluetoothRequestParent::DoRequest(const SetPasskeyRequest& aRequest)
{
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TSetPasskeyRequest);

  bool result =
    mService->SetPasskeyInternal(aRequest.path(),
                                 aRequest.passkey(),
                                 mReplyRunnable.get());

  NS_ENSURE_TRUE(result, false);

  return true;
}

bool
BluetoothRequestParent::DoRequest(const ConfirmPairingConfirmationRequest&
                                  aRequest)
{
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TConfirmPairingConfirmationRequest);

  bool result =
    mService->SetPairingConfirmationInternal(aRequest.path(),
                                             true,
                                             mReplyRunnable.get());

  NS_ENSURE_TRUE(result, false);

  return true;
}

bool
BluetoothRequestParent::DoRequest(const ConfirmAuthorizationRequest& aRequest)
{
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TConfirmAuthorizationRequest);

  bool result =
    mService->SetAuthorizationInternal(aRequest.path(),
                                       true,
                                       mReplyRunnable.get());

  NS_ENSURE_TRUE(result, false);

  return true;
}

bool
BluetoothRequestParent::DoRequest(const DenyPairingConfirmationRequest&
                                  aRequest)
{
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TDenyPairingConfirmationRequest);

  bool result =
    mService->SetPairingConfirmationInternal(aRequest.path(),
                                             false,
                                             mReplyRunnable.get());

  NS_ENSURE_TRUE(result, false);

  return true;
}

bool
BluetoothRequestParent::DoRequest(const DenyAuthorizationRequest& aRequest)
{
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TDenyAuthorizationRequest);

  bool result =
    mService->SetAuthorizationInternal(aRequest.path(),
                                       false,
                                       mReplyRunnable.get());

  NS_ENSURE_TRUE(result, false);

  return true;
}

bool
BluetoothRequestParent::DoRequest(const ConnectHeadsetRequest& aRequest)
{
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TConnectHeadsetRequest);

  return mService->ConnectHeadset(aRequest.address(),
                                  aRequest.adapterPath(),
                                  mReplyRunnable.get());
}

bool
BluetoothRequestParent::DoRequest(const ConnectObjectPushRequest& aRequest)
{
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TConnectObjectPushRequest);

  return mService->ConnectObjectPush(aRequest.address(),
                                     aRequest.adapterPath(),
                                     mReplyRunnable.get());
}

bool
BluetoothRequestParent::DoRequest(const DisconnectHeadsetRequest& aRequest)
{
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TDisconnectHeadsetRequest);

  mService->DisconnectHeadset(mReplyRunnable.get());

  return true;
}

bool
BluetoothRequestParent::DoRequest(const DisconnectObjectPushRequest& aRequest)
{
  MOZ_ASSERT(mService);
  MOZ_ASSERT(mRequestType == Request::TDenyAuthorizationRequest);

  mService->DisconnectObjectPush(mReplyRunnable.get());

  return true;
}
