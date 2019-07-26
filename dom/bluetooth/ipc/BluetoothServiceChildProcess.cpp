





#include "base/basictypes.h"

#include "BluetoothServiceChildProcess.h"

#include "mozilla/Assertions.h"
#include "mozilla/dom/ContentChild.h"

#include "BluetoothChild.h"

USING_BLUETOOTH_NAMESPACE

namespace {

BluetoothChild* gBluetoothChild;

inline
void
SendRequest(BluetoothReplyRunnable* aRunnable, const Request& aRequest)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aRunnable);

  NS_WARN_IF_FALSE(gBluetoothChild,
                   "Calling methods on BluetoothServiceChildProcess during "
                   "shutdown!");

  if (gBluetoothChild) {
    BluetoothRequestChild* actor = new BluetoothRequestChild(aRunnable);
    gBluetoothChild->SendPBluetoothRequestConstructor(actor, aRequest);
  }
}

} 


BluetoothServiceChildProcess*
BluetoothServiceChildProcess::Create()
{
  MOZ_ASSERT(!gBluetoothChild);

  mozilla::dom::ContentChild* contentChild =
    mozilla::dom::ContentChild::GetSingleton();
  MOZ_ASSERT(contentChild);

  BluetoothServiceChildProcess* btService = new BluetoothServiceChildProcess();

  gBluetoothChild = new BluetoothChild(btService);
  contentChild->SendPBluetoothConstructor(gBluetoothChild);

  return btService;
}

BluetoothServiceChildProcess::BluetoothServiceChildProcess()
{
}

BluetoothServiceChildProcess::~BluetoothServiceChildProcess()
{
  gBluetoothChild = nullptr;
}

void
BluetoothServiceChildProcess::NoteDeadActor()
{
  MOZ_ASSERT(gBluetoothChild);
  gBluetoothChild = nullptr;
}

void
BluetoothServiceChildProcess::RegisterBluetoothSignalHandler(
                                              const nsAString& aNodeName,
                                              BluetoothSignalObserver* aHandler)
{
  if (gBluetoothChild && !IsSignalRegistered(aNodeName)) {
    gBluetoothChild->SendRegisterSignalHandler(nsString(aNodeName));
  }
  BluetoothService::RegisterBluetoothSignalHandler(aNodeName, aHandler);
}

void
BluetoothServiceChildProcess::UnregisterBluetoothSignalHandler(
                                              const nsAString& aNodeName,
                                              BluetoothSignalObserver* aHandler)
{
  BluetoothService::UnregisterBluetoothSignalHandler(aNodeName, aHandler);
  if (gBluetoothChild && !IsSignalRegistered(aNodeName)) {
    gBluetoothChild->SendUnregisterSignalHandler(nsString(aNodeName));
  }
}

nsresult
BluetoothServiceChildProcess::GetDefaultAdapterPathInternal(
                                              BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable, DefaultAdapterPathRequest());
  return NS_OK;
}

nsresult
BluetoothServiceChildProcess::GetConnectedDevicePropertiesInternal(
                                              uint16_t aProfileId,
                                              BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable, ConnectedDevicePropertiesRequest(aProfileId));
  return NS_OK;
}
nsresult
BluetoothServiceChildProcess::GetPairedDevicePropertiesInternal(
                                     const nsTArray<nsString>& aDeviceAddresses,
                                     BluetoothReplyRunnable* aRunnable)
{
  PairedDevicePropertiesRequest request;
  request.addresses().AppendElements(aDeviceAddresses);

  SendRequest(aRunnable, request);
  return NS_OK;
}

nsresult
BluetoothServiceChildProcess::StopDiscoveryInternal(
                                              BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable, StopDiscoveryRequest());
  return NS_OK;
}

nsresult
BluetoothServiceChildProcess::StartDiscoveryInternal(
                                              BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable, StartDiscoveryRequest());
  return NS_OK;
}

nsresult
BluetoothServiceChildProcess::SetProperty(BluetoothObjectType aType,
                                          const BluetoothNamedValue& aValue,
                                          BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable, SetPropertyRequest(aType, aValue));
  return NS_OK;
}

bool
BluetoothServiceChildProcess::GetDevicePath(const nsAString& aAdapterPath,
                                            const nsAString& aDeviceAddress,
                                            nsAString& aDevicePath)
{
  
  
  
  
  nsAutoString path(aAdapterPath);
  path.AppendLiteral("/dev_");
  path.Append(aDeviceAddress);
  path.ReplaceChar(':', '_');

  aDevicePath = path;

  return true;
}

nsresult
BluetoothServiceChildProcess::CreatePairedDeviceInternal(
                                              const nsAString& aAddress,
                                              int aTimeout,
                                              BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable,
              PairRequest(nsString(aAddress), aTimeout));
  return NS_OK;
}

nsresult
BluetoothServiceChildProcess::RemoveDeviceInternal(
                                              const nsAString& aObjectPath,
                                              BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable,
              UnpairRequest(nsString(aObjectPath)));
  return NS_OK;
}

nsresult
BluetoothServiceChildProcess::GetScoSocket(
                                    const nsAString& aObjectPath,
                                    bool aAuth,
                                    bool aEncrypt,
                                    mozilla::ipc::UnixSocketConsumer* aConsumer)
{
  MOZ_CRASH("This should never be called!");
}

nsresult
BluetoothServiceChildProcess::GetServiceChannel(const nsAString& aDeviceAddress,
                                                const nsAString& aServiceUuid,
                                                BluetoothProfileManagerBase* aManager)
{
  MOZ_CRASH("This should never be called!");
}

bool
BluetoothServiceChildProcess::UpdateSdpRecords(const nsAString& aDeviceAddress,
                                               BluetoothProfileManagerBase* aManager)
{
  MOZ_CRASH("This should never be called!");
}

bool
BluetoothServiceChildProcess::SetPinCodeInternal(
                                                const nsAString& aDeviceAddress,
                                                const nsAString& aPinCode,
                                                BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable,
              SetPinCodeRequest(nsString(aDeviceAddress), nsString(aPinCode)));
  return true;
}

bool
BluetoothServiceChildProcess::SetPasskeyInternal(
                                                const nsAString& aDeviceAddress,
                                                uint32_t aPasskey,
                                                BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable,
              SetPasskeyRequest(nsString(aDeviceAddress), aPasskey));
  return true;
}

bool
BluetoothServiceChildProcess::SetPairingConfirmationInternal(
                                                const nsAString& aDeviceAddress,
                                                bool aConfirm,
                                                BluetoothReplyRunnable* aRunnable)
{
  if(aConfirm) {
    SendRequest(aRunnable,
                ConfirmPairingConfirmationRequest(nsString(aDeviceAddress)));
  } else {
    SendRequest(aRunnable,
                DenyPairingConfirmationRequest(nsString(aDeviceAddress)));
  }
  return true;
}

bool
BluetoothServiceChildProcess::SetAuthorizationInternal(
                                                const nsAString& aDeviceAddress,
                                                bool aAllow,
                                                BluetoothReplyRunnable* aRunnable)
{
  if(aAllow) {
    SendRequest(aRunnable,
                ConfirmAuthorizationRequest(nsString(aDeviceAddress)));
  } else {
    SendRequest(aRunnable,
                DenyAuthorizationRequest(nsString(aDeviceAddress)));
  }
  return true;
}

void
BluetoothServiceChildProcess::Connect(
  const nsAString& aDeviceAddress,
  const uint16_t aProfileId,
  BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable,
              ConnectRequest(nsString(aDeviceAddress),
                             aProfileId));
}

void
BluetoothServiceChildProcess::Disconnect(
  const uint16_t aProfileId,
  BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable, DisconnectRequest(aProfileId));
}

void
BluetoothServiceChildProcess::SendFile(
  const nsAString& aDeviceAddress,
  BlobParent* aBlobParent,
  BlobChild* aBlobChild,
  BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable,
              SendFileRequest(nsString(aDeviceAddress), nullptr, aBlobChild));
}

void
BluetoothServiceChildProcess::StopSendingFile(
  const nsAString& aDeviceAddress,
  BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable,
              StopSendingFileRequest(nsString(aDeviceAddress)));
}

void
BluetoothServiceChildProcess::ConfirmReceivingFile(
  const nsAString& aDeviceAddress,
  bool aConfirm,
  BluetoothReplyRunnable* aRunnable)
{
  if(aConfirm) {
    SendRequest(aRunnable,
                ConfirmReceivingFileRequest(nsString(aDeviceAddress)));
    return;
  }
  
  SendRequest(aRunnable,
              DenyReceivingFileRequest(nsString(aDeviceAddress)));
}

void
BluetoothServiceChildProcess::ConnectSco(BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable, ConnectScoRequest());
}

void
BluetoothServiceChildProcess::DisconnectSco(BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable, DisconnectScoRequest());
}

void
BluetoothServiceChildProcess::IsScoConnected(BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable, IsScoConnectedRequest());
}

void
BluetoothServiceChildProcess::SendMetaData(const nsAString& aTitle,
                                           const nsAString& aArtist,
                                           const nsAString& aAlbum,
                                           uint32_t aMediaNumber,
                                           uint32_t aTotalMediaCount,
                                           uint32_t aDuration,
                                           BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable,
              SendMetaDataRequest(nsString(aTitle), nsString(aArtist),
                                  nsString(aAlbum), aMediaNumber,
                                  aTotalMediaCount, aDuration));
}

void
BluetoothServiceChildProcess::SendPlayStatus(uint32_t aDuration,
                                             uint32_t aPosition,
                                             const nsAString& aPlayStatus,
                                             BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable,
              SendPlayStatusRequest(aDuration, aPosition,
                                    nsString(aPlayStatus)));
}

nsresult
BluetoothServiceChildProcess::HandleStartup()
{
  
  
  return NS_OK;
}

nsresult
BluetoothServiceChildProcess::HandleShutdown()
{
  
  
  if (gBluetoothChild) {
    gBluetoothChild->BeginShutdown();
  }
  return NS_OK;
}

nsresult
BluetoothServiceChildProcess::StartInternal()
{
  MOZ_CRASH("This should never be called!");
}

nsresult
BluetoothServiceChildProcess::StopInternal()
{
  MOZ_CRASH("This should never be called!");
}

bool
BluetoothServiceChildProcess::IsEnabledInternal()
{
  MOZ_CRASH("This should never be called!");
}

bool
BluetoothServiceChildProcess::IsConnected(uint16_t aProfileId)
{
  MOZ_CRASH("This should never be called!");
}

nsresult
BluetoothServiceChildProcess::SendSinkMessage(const nsAString& aDeviceAddresses,
                                              const nsAString& aMessage)
{
  MOZ_CRASH("This should never be called!");
}

