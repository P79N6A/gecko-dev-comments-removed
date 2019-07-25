




#ifndef mozilla_dom_devicestorage_DeviceStorageRequestChild_h
#define mozilla_dom_devicestorage_DeviceStorageRequestChild_h

#include "mozilla/dom/devicestorage/PDeviceStorageRequestChild.h"
#include "DOMRequest.h"
#include "nsDeviceStorage.h"
namespace mozilla {
namespace dom {
namespace devicestorage {

class DeviceStorageRequestChild : public PDeviceStorageRequestChild
{
public:
  DeviceStorageRequestChild();
  DeviceStorageRequestChild(DOMRequest* aRequest, DeviceStorageFile* aFile);
  ~DeviceStorageRequestChild();

  virtual bool Recv__delete__(const DeviceStorageResponseValue& value);

private:
  nsRefPtr<DOMRequest> mRequest;
  nsRefPtr<DeviceStorageFile> mFile;
};

} 
} 
} 

#endif
