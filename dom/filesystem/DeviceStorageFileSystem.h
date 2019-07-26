





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
  SetDeviceStorage(nsDOMDeviceStorage* aDeviceStorage);

  

  virtual nsPIDOMWindow*
  GetWindow() const MOZ_OVERRIDE;

  virtual already_AddRefed<nsIFile>
  GetLocalFile(const nsAString& aRealPath) const MOZ_OVERRIDE;

  virtual const nsAString&
  GetRootName() const MOZ_OVERRIDE;
private:
  virtual
  ~DeviceStorageFileSystem();

  nsString mStorageType;
  nsString mStorageName;

  
  
  nsString mLocalRootPath;
  nsDOMDeviceStorage* mDeviceStorage;
};

} 
} 

#endif 
