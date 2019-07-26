



#ifndef mozilla_dom_bluetooth_ipc_bluetoothmessageutils_h__
#define mozilla_dom_bluetooth_ipc_bluetoothmessageutils_h__

#include "mozilla/dom/bluetooth/BluetoothCommon.h"
#include "ipc/IPCMessageUtils.h"

namespace IPC {

template <>
struct ParamTraits<mozilla::dom::bluetooth::BluetoothObjectType>
  : public EnumSerializer<mozilla::dom::bluetooth::BluetoothObjectType,
                          mozilla::dom::bluetooth::TYPE_MANAGER,
                          mozilla::dom::bluetooth::TYPE_INVALID>
{ };

} 

#endif 
