





#ifndef DeviceStorage_h
#define DeviceStorage_h

#include "nsIDOMDeviceStorage.h"
#include "nsIFile.h"
#include "nsIPrincipal.h"
#include "nsIObserver.h"
#include "nsDOMEventTargetHelper.h"
#include "mozilla/RefPtr.h"
#include "mozilla/StaticPtr.h"
#include "DOMRequest.h"

#define DEVICESTORAGE_PICTURES   "pictures"
#define DEVICESTORAGE_VIDEOS     "videos"
#define DEVICESTORAGE_MUSIC      "music"
#define DEVICESTORAGE_APPS       "apps"
#define DEVICESTORAGE_SDCARD     "sdcard"
#define DEVICESTORAGE_CRASHES    "crashes"

class nsIInputStream;

namespace mozilla {
namespace dom {
class DeviceStorageEnumerationParameters;
class DOMCursor;
class DOMRequest;
} 
} 

class DeviceStorageFile MOZ_FINAL
  : public nsISupports {
public:
  nsCOMPtr<nsIFile> mFile;
  nsString mStorageType;
  nsString mStorageName;
  nsString mRootDir;
  nsString mPath;
  bool mEditable;
  nsString mMimeType;
  uint64_t mLength;
  uint64_t mLastModifiedDate;

  
  DeviceStorageFile(const nsAString& aStorageType,
                    const nsAString& aStorageName);
  
  DeviceStorageFile(const nsAString& aStorageType,
                    const nsAString& aStorageName,
                    const nsAString& aPath);
  
  
  
  
  DeviceStorageFile(const nsAString& aStorageType,
                    const nsAString& aStorageName,
                    const nsAString& aRootDir,
                    const nsAString& aPath);

  void SetPath(const nsAString& aPath);
  void SetEditable(bool aEditable);

  static already_AddRefed<DeviceStorageFile>
  CreateUnique(nsAString& aFileName,
               uint32_t aFileType,
               uint32_t aFileAttributes);

  NS_DECL_THREADSAFE_ISUPPORTS

  bool IsAvailable();
  void GetFullPath(nsAString& aFullPath);

  
  
  bool IsSafePath();
  bool IsSafePath(const nsAString& aPath);

  void Dump(const char* label);

  nsresult Remove();
  nsresult Write(nsIInputStream* aInputStream);
  nsresult Write(InfallibleTArray<uint8_t>& bits);
  void CollectFiles(nsTArray<nsRefPtr<DeviceStorageFile> >& aFiles,
                    PRTime aSince = 0);
  void collectFilesInternal(nsTArray<nsRefPtr<DeviceStorageFile> >& aFiles,
                            PRTime aSince, nsAString& aRootPath);

  void AccumDiskUsage(uint64_t* aPicturesSoFar, uint64_t* aVideosSoFar,
                      uint64_t* aMusicSoFar, uint64_t* aTotalSoFar);

  void GetDiskFreeSpace(int64_t* aSoFar);
  void GetStatus(nsAString& aStatus);
  static void GetRootDirectoryForType(const nsAString& aStorageType,
                                      const nsAString& aStorageName,
                                      nsIFile** aFile);

  nsresult CalculateSizeAndModifiedDate();
  nsresult CalculateMimeType();

private:
  void Init();
  void NormalizeFilePath();
  void AppendRelativePath(const nsAString& aPath);
  void AccumDirectoryUsage(nsIFile* aFile,
                           uint64_t* aPicturesSoFar,
                           uint64_t* aVideosSoFar,
                           uint64_t* aMusicSoFar,
                           uint64_t* aTotalSoFar);
};














class FileUpdateDispatcher MOZ_FINAL
  : public nsIObserver
{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  static FileUpdateDispatcher* GetSingleton();
 private:
  static mozilla::StaticRefPtr<FileUpdateDispatcher> sSingleton;
};

class nsDOMDeviceStorage MOZ_FINAL
  : public nsDOMEventTargetHelper
  , public nsIDOMDeviceStorage
  , public nsIObserver
{
  typedef mozilla::ErrorResult ErrorResult;
  typedef mozilla::dom::DeviceStorageEnumerationParameters
    EnumerationParameters;
  typedef mozilla::dom::DOMCursor DOMCursor;
  typedef mozilla::dom::DOMRequest DOMRequest;
public:
  typedef nsTArray<nsString> VolumeNameArray;

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMDEVICESTORAGE

  NS_DECL_NSIOBSERVER
  NS_REALLY_DECL_NSIDOMEVENTTARGET

  virtual void
  AddEventListener(const nsAString& aType,
                   nsIDOMEventListener* aListener,
                   bool aUseCapture,
                   const mozilla::dom::Nullable<bool>& aWantsUntrusted,
                   ErrorResult& aRv) MOZ_OVERRIDE;

  virtual void RemoveEventListener(const nsAString& aType,
                                   nsIDOMEventListener* aListener,
                                   bool aUseCapture,
                                   ErrorResult& aRv) MOZ_OVERRIDE;

  nsDOMDeviceStorage();

  nsresult Init(nsPIDOMWindow* aWindow, const nsAString& aType,
                const nsAString& aVolName);

  bool IsAvailable();
  bool IsFullPath(const nsAString& aPath)
  {
    return aPath.Length() > 0 && aPath.CharAt(0) == '/';
  }

  void SetRootDirectoryForType(const nsAString& aType,
                               const nsAString& aVolName);

  
  nsPIDOMWindow*
  GetParentObject() const
  {
    return GetOwner();
  }
  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  IMPL_EVENT_HANDLER(change)

  already_AddRefed<DOMRequest>
  Add(nsIDOMBlob* aBlob, ErrorResult& aRv);
  already_AddRefed<DOMRequest>
  AddNamed(nsIDOMBlob* aBlob, const nsAString& aPath, ErrorResult& aRv);

  already_AddRefed<DOMRequest>
  Get(const nsAString& aPath, ErrorResult& aRv)
  {
    return GetInternal(aPath, false, aRv);
  }
  already_AddRefed<DOMRequest>
  GetEditable(const nsAString& aPath, ErrorResult& aRv)
  {
    return GetInternal(aPath, true, aRv);
  }
  already_AddRefed<DOMRequest>
  Delete(const nsAString& aPath, ErrorResult& aRv);

  already_AddRefed<DOMCursor>
  Enumerate(const EnumerationParameters& aOptions, ErrorResult& aRv)
  {
    return Enumerate(NullString(), aOptions, aRv);
  }
  already_AddRefed<DOMCursor>
  Enumerate(const nsAString& aPath, const EnumerationParameters& aOptions,
            ErrorResult& aRv);
  already_AddRefed<DOMCursor>
  EnumerateEditable(const EnumerationParameters& aOptions, ErrorResult& aRv)
  {
    return EnumerateEditable(NullString(), aOptions, aRv);
  }
  already_AddRefed<DOMCursor>
  EnumerateEditable(const nsAString& aPath,
                    const EnumerationParameters& aOptions, ErrorResult& aRv);

  already_AddRefed<DOMRequest> FreeSpace(ErrorResult& aRv);
  already_AddRefed<DOMRequest> UsedSpace(ErrorResult& aRv);
  already_AddRefed<DOMRequest> Available(ErrorResult& aRv);

  bool Default();

  

  static void
  CreateDeviceStorageFor(nsPIDOMWindow* aWin,
                         const nsAString& aType,
                         nsDOMDeviceStorage** aStore);

  static void
  CreateDeviceStoragesFor(nsPIDOMWindow* aWin,
                          const nsAString& aType,
                          nsTArray<nsRefPtr<nsDOMDeviceStorage> >& aStores);

  void Shutdown();

  static void GetOrderedVolumeNames(nsTArray<nsString>& aVolumeNames);

  static void GetDefaultStorageName(const nsAString& aStorageType,
                                    nsAString &aStorageName);

  static bool ParseFullPath(const nsAString& aFullPath,
                            nsAString& aOutStorageName,
                            nsAString& aOutStoragePath);
private:
  ~nsDOMDeviceStorage();

  already_AddRefed<DOMRequest>
  GetInternal(const nsAString& aPath, bool aEditable, ErrorResult& aRv);

  void
  GetInternal(nsPIDOMWindow* aWin, const nsAString& aPath, DOMRequest* aRequest,
              bool aEditable);

  void
  DeleteInternal(nsPIDOMWindow* aWin, const nsAString& aPath,
                 DOMRequest* aRequest);

  already_AddRefed<DOMCursor>
  EnumerateInternal(const nsAString& aName,
                    const EnumerationParameters& aOptions, bool aEditable,
                    ErrorResult& aRv);

  nsString mStorageType;
  nsCOMPtr<nsIFile> mRootDirectory;
  nsString mStorageName;

  already_AddRefed<nsDOMDeviceStorage> GetStorage(const nsAString& aFullPath,
                                                  nsAString& aOutStoragePath);
  already_AddRefed<nsDOMDeviceStorage>
    GetStorageByName(const nsAString &aStorageName);

  nsCOMPtr<nsIPrincipal> mPrincipal;

  bool mIsWatchingFile;
  bool mAllowedToWatchFile;

  nsresult Notify(const char* aReason, class DeviceStorageFile* aFile);

  friend class WatchFileEvent;
  friend class DeviceStorageRequest;

  class VolumeNameCache : public mozilla::RefCounted<VolumeNameCache>
  {
  public:
    nsTArray<nsString>  mVolumeNames;
  };
  static mozilla::StaticRefPtr<VolumeNameCache> sVolumeNameCache;

#ifdef MOZ_WIDGET_GONK
  nsString mLastStatus;
  void DispatchMountChangeEvent(nsAString& aVolumeStatus);
#endif

  
  enum {
      DEVICE_STORAGE_TYPE_DEFAULT = 0,
      DEVICE_STORAGE_TYPE_SHARED,
      DEVICE_STORAGE_TYPE_EXTERNAL
  };
};

#endif
