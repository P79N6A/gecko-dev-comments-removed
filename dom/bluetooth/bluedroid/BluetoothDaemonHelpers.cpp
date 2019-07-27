





#include "BluetoothDaemonHelpers.h"

BEGIN_BLUETOOTH_NAMESPACE





nsresult
Convert(uint8_t aIn, BluetoothStatus& aOut)
{
  static const BluetoothStatus sStatus[] = {
    CONVERT(0x00, STATUS_SUCCESS),
    CONVERT(0x01, STATUS_FAIL),
    CONVERT(0x02, STATUS_NOT_READY),
    CONVERT(0x03, STATUS_NOMEM),
    CONVERT(0x04, STATUS_BUSY),
    CONVERT(0x05, STATUS_DONE),
    CONVERT(0x06, STATUS_UNSUPPORTED),
    CONVERT(0x07, STATUS_PARM_INVALID),
    CONVERT(0x08, STATUS_UNHANDLED),
    CONVERT(0x09, STATUS_AUTH_FAILURE),
    CONVERT(0x0a, STATUS_RMT_DEV_DOWN)
  };
  if (NS_WARN_IF(aIn >= MOZ_ARRAY_LENGTH(sStatus))) {
    return NS_ERROR_ILLEGAL_VALUE;
  }
  aOut = sStatus[aIn];
  return NS_OK;
}





nsresult
PackPDU(const BluetoothConfigurationParameter& aIn, BluetoothDaemonPDU& aPDU)
{
  return PackPDU(aIn.mType, aIn.mLength,
                 PackArray<uint8_t>(aIn.mValue.get(), aIn.mLength), aPDU);
}

nsresult
PackPDU(const BluetoothDaemonPDUHeader& aIn, BluetoothDaemonPDU& aPDU)
{
  return PackPDU(aIn.mService, aIn.mOpcode, aIn.mLength, aPDU);
}





nsresult
UnpackPDU(BluetoothDaemonPDU& aPDU, BluetoothStatus& aOut)
{
  return UnpackPDU(aPDU, UnpackConversion<uint8_t, BluetoothStatus>(aOut));
}

END_BLUETOOTH_NAMESPACE
