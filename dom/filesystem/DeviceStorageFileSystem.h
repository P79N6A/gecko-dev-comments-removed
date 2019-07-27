





#ifndef mozilla_dom_DeviceStorageFileSystem_h
#define mozilla_dom_DeviceStorageFileSystem_h

#include "mozilla/dom/FileSystemBase.h"
#include "nsString.h"

class nsDOMDeviceStorage;

namespace mozilla {
namespace dom {

class DeviceStorageFileSystem
  : public FileSystemBase
{
public:
  DeviceStorageFileSystem(const nsAString& aStorageType,
                          const nsAString& aStorageName);

  void
  Init(nsDOMDeviceStorage* aDeviceStorage);

  

  virtual void
  Shutdown() override;

  virtual nsPIDOMWindow*
  GetWindow() const override;

  virtual void
  GetRootName(nsAString& aRetval) const override;

  virtual bool
  IsSafeFile(nsIFile* aFile) const override;

  virtual bool
  IsSafeDirectory(Directory* aDir) const override;
private:
  virtual
  ~DeviceStorageFileSystem();

  nsString mStorageType;
  nsString mStorageName;

  nsDOMDeviceStorage* mDeviceStorage;
};

} 
} 

#endif 
