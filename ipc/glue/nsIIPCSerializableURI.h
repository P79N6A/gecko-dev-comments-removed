



#ifndef mozilla_ipc_nsIIPCSerializableURI_h
#define mozilla_ipc_nsIIPCSerializableURI_h

#include "nsISupports.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace ipc {
class URIParams;
} 
} 

#define NS_IIPCSERIALIZABLEURI_IID \
  {0xfee3437d, 0x3daf, 0x411f, {0xb0, 0x1d, 0xdc, 0xd4, 0x88, 0x55, 0xe3, 0xd}}

class NS_NO_VTABLE nsIIPCSerializableURI : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IIPCSERIALIZABLEURI_IID)

  virtual void
  Serialize(mozilla::ipc::URIParams& aParams) = 0;

  virtual bool
  Deserialize(const mozilla::ipc::URIParams& aParams) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIIPCSerializableURI,
                              NS_IIPCSERIALIZABLEURI_IID)

#define NS_DECL_NSIIPCSERIALIZABLEURI \
  virtual void \
  Serialize(mozilla::ipc::URIParams&) override; \
  virtual bool \
  Deserialize(const mozilla::ipc::URIParams&) override;

#define NS_FORWARD_NSIIPCSERIALIZABLEURI(_to) \
  virtual void \
  Serialize(mozilla::ipc::URIParams& aParams) override \
  { _to Serialize(aParams); } \
  virtual bool \
  Deserialize(const mozilla::ipc::URIParams& aParams) override \
  { return _to Deserialize(aParams); }

#define NS_FORWARD_SAFE_NSIIPCSERIALIZABLEURI(_to) \
  virtual void \
  Serialize(mozilla::ipc::URIParams& aParams) override \
  { if (_to) { _to->Serialize(aParams); } } \
  virtual bool \
  Deserialize(const mozilla::ipc::URIParams& aParams) override \
  { if (_to) { return _to->Deserialize(aParams); } return false; }

#endif 
