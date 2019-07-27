





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

  virtual already_AddRefed<nsIFile>
  GetLocalFile(const nsAString& aRealPath) const override;

  virtual bool
  GetRealPath(FileImpl* aFile, nsAString& aRealPath) const override;

  virtual const nsAString&
  GetRootName() const override;

  virtual bool
  IsSafeFile(nsIFile* aFile) const override;

  virtual bool
  IsSafeDirectory(Directory* aDir) const override;
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
