



#ifndef NfcMessageHandler_h
#define NfcMessageHandler_h

namespace android {
class MOZ_EXPORT Parcel;
} 

namespace mozilla {

class CommandOptions;
class EventOptions;

class NfcMessageHandler
{
public:
  bool Marshall(android::Parcel& aParcel, const CommandOptions& aOptions);
  bool Unmarshall(const android::Parcel& aParcel, EventOptions& aOptions);
};

} 

#endif 
