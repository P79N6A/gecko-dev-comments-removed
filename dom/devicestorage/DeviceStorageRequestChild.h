




#ifndef mozilla_dom_devicestorage_DeviceStorageRequestChild_h
#define mozilla_dom_devicestorage_DeviceStorageRequestChild_h

#include "mozilla/dom/devicestorage/PDeviceStorageRequestChild.h"

class DeviceStorageFile;
struct DeviceStorageFileDescriptor;

namespace mozilla {
namespace dom {

class DOMRequest;

namespace devicestorage {

class DeviceStorageRequestChildCallback
{
  public:
    virtual void RequestComplete() = 0;
};

class DeviceStorageRequestChild : public PDeviceStorageRequestChild
{
public:
  DeviceStorageRequestChild();
  DeviceStorageRequestChild(DOMRequest* aRequest, DeviceStorageFile* aFile);
  DeviceStorageRequestChild(DOMRequest* aRequest, DeviceStorageFile* aFile,
                            DeviceStorageFileDescriptor* aFileDescrptor);
  ~DeviceStorageRequestChild();

  void SetCallback(class DeviceStorageRequestChildCallback *aCallback);

  virtual bool Recv__delete__(const DeviceStorageResponseValue& value);

private:
  nsRefPtr<DOMRequest> mRequest;
  nsRefPtr<DeviceStorageFile> mDSFile;
  nsRefPtr<DeviceStorageFileDescriptor> mDSFileDescriptor;

  DeviceStorageRequestChildCallback* mCallback;
};

} 
} 
} 

#endif
