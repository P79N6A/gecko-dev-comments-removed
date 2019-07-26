





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
  if (gBluetoothChild) {
    gBluetoothChild->SendRegisterSignalHandler(nsString(aNodeName));
  }
  BluetoothService::RegisterBluetoothSignalHandler(aNodeName, aHandler);
}

void
BluetoothServiceChildProcess::UnregisterBluetoothSignalHandler(
                                              const nsAString& aNodeName,
                                              BluetoothSignalObserver* aHandler)
{
  if (gBluetoothChild) {
    gBluetoothChild->SendUnregisterSignalHandler(nsString(aNodeName));
  }
  BluetoothService::UnregisterBluetoothSignalHandler(aNodeName, aHandler);
}

nsresult
BluetoothServiceChildProcess::GetDefaultAdapterPathInternal(
                                              BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable, DefaultAdapterPathRequest());
  return NS_OK;
}

nsresult
BluetoothServiceChildProcess::GetDevicePropertiesInternal(
                                                   const nsAString& aDevicePath,
                                                   const nsAString& aSignalPath)
{
  MOZ_NOT_REACHED("Should never be called from child");
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
BluetoothServiceChildProcess::GetPairedDevicePropertiesInternal(
                                     const nsTArray<nsString>& aDeviceAddresses,
                                     BluetoothReplyRunnable* aRunnable)
{
  DevicePropertiesRequest request;
  request.addresses().AppendElements(aDeviceAddresses);

  SendRequest(aRunnable, request);
  return NS_OK;
}

nsresult
BluetoothServiceChildProcess::StopDiscoveryInternal(
                                              const nsAString& aAdapterPath,
                                              BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable, StopDiscoveryRequest(nsString(aAdapterPath)));
  return NS_OK;
}

nsresult
BluetoothServiceChildProcess::StartDiscoveryInternal(
                                              const nsAString& aAdapterPath,
                                              BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable, StartDiscoveryRequest(nsString(aAdapterPath)));
  return NS_OK;
}

nsresult
BluetoothServiceChildProcess::GetProperties(BluetoothObjectType aType,
                                            const nsAString& aPath,
                                            BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable, GetPropertyRequest(aType, nsString(aPath)));
  return NS_OK;
}

nsresult
BluetoothServiceChildProcess::SetProperty(BluetoothObjectType aType,
                                          const nsAString& aPath,
                                          const BluetoothNamedValue& aValue,
                                          BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable, SetPropertyRequest(aType, nsString(aPath), aValue));
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

  return NS_OK;
}

nsresult
BluetoothServiceChildProcess::CreatePairedDeviceInternal(
                                              const nsAString& aAdapterPath,
                                              const nsAString& aAddress,
                                              int aTimeout,
                                              BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable,
              PairRequest(nsString(aAdapterPath), nsString(aAddress), aTimeout));
  return NS_OK;
}

nsresult
BluetoothServiceChildProcess::RemoveDeviceInternal(
                                              const nsAString& aAdapterPath,
                                              const nsAString& aObjectPath,
                                              BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable,
              UnpairRequest(nsString(aAdapterPath), nsString(aObjectPath)));
  return NS_OK;
}

nsresult
BluetoothServiceChildProcess::GetSocketViaService(
                                              const nsAString& aObjectPath,
                                              const nsAString& aService,
                                              int aType,
                                              bool aAuth,
                                              bool aEncrypt,
                                              BluetoothReplyRunnable* aRunnable)
{
  MOZ_NOT_REACHED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

bool
BluetoothServiceChildProcess::CloseSocket(int aFd,
                                          BluetoothReplyRunnable* aRunnable)
{
  MOZ_NOT_REACHED("Implement me!");
  return false;
}

bool
BluetoothServiceChildProcess::SetPinCodeInternal(
                                                const nsAString& aDeviceAddress,
                                                const nsAString& aPinCode,
                                                BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable,
              SetPinCodeRequest(nsString(aDeviceAddress), nsString(aPinCode)));
  return NS_OK;
}

bool
BluetoothServiceChildProcess::SetPasskeyInternal(
                                                const nsAString& aDeviceAddress,
                                                uint32_t aPasskey,
                                                BluetoothReplyRunnable* aRunnable)
{
  SendRequest(aRunnable,
              SetPasskeyRequest(nsString(aDeviceAddress), aPasskey));
  return NS_OK;
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
  return NS_OK;
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
  return NS_OK;
}

nsresult
BluetoothServiceChildProcess::PrepareAdapterInternal(const nsAString& aPath)
{
  MOZ_NOT_REACHED("Should never be called from child");
  return NS_ERROR_NOT_IMPLEMENTED;
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
  MOZ_NOT_REACHED("This should never be called!");
  return NS_ERROR_FAILURE;
}

nsresult
BluetoothServiceChildProcess::StopInternal()
{
  MOZ_NOT_REACHED("This should never be called!");
  return NS_ERROR_FAILURE;
}
