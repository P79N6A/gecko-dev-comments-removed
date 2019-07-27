





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
  Shutdown() MOZ_OVERRIDE;

  virtual nsPIDOMWindow*
  GetWindow() const MOZ_OVERRIDE;

  virtual already_AddRefed<nsIFile>
  GetLocalFile(const nsAString& aRealPath) const MOZ_OVERRIDE;

  virtual bool
  GetRealPath(DOMFileImpl* aFile, nsAString& aRealPath) const MOZ_OVERRIDE;

  virtual const nsAString&
  GetRootName() const MOZ_OVERRIDE;

  virtual bool
  IsSafeFile(nsIFile* aFile) const MOZ_OVERRIDE;

  virtual bool
  IsSafeDirectory(Directory* aDir) const MOZ_OVERRIDE;
private:
  virtual
  ~DeviceStorageFileSystem();

  bool
  LocalPathToRealPath(const nsAString& aLocalPath, nsAString& aRealPath) const;

  nsString mStorageType;
  nsString mStorageName;

  
  
  nsString mLocalRootPath;
  nsString mNormalizedLocalRootPath;
  nsDOMDeviceStorage* mDeviceStorage;
};

} 
} 

#endif 
