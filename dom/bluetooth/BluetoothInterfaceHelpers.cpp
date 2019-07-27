





#include "BluetoothInterfaceHelpers.h"

BEGIN_BLUETOOTH_NAMESPACE





nsresult
Convert(nsresult aIn, BluetoothStatus& aOut)
{
  if (NS_SUCCEEDED(aIn)) {
    aOut = STATUS_SUCCESS;
  } else if (aIn == NS_ERROR_OUT_OF_MEMORY) {
    aOut = STATUS_NOMEM;
  } else {
    aOut = STATUS_FAIL;
  }
  return NS_OK;
}

END_BLUETOOTH_NAMESPACE
