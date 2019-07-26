





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

class DeviceStorageFile MOZ_FINAL
  : public nsISupports {
public:
  nsCOMPtr<nsIFile> mFile;
  nsString mStorageType;
  nsString mStorageName;
  nsString mRootDir;
  nsString mPath;
  bool mEditable;

  
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

  static already_AddRefed<DeviceStorageFile> CreateUnique(nsAString& aFileName,
                                                          uint32_t aFileType,
                                                          uint32_t aFileAttributes);

  NS_DECL_ISUPPORTS

  bool IsAvailable();
  bool IsComposite();
  void GetCompositePath(nsAString& aCompositePath);

  
  
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
private:
  void Init();
  void NormalizeFilePath();
  void AppendRelativePath(const nsAString& aPath);
  void GetStatusInternal(nsAString& aStorageName, nsAString& aStatus);
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
public:
  typedef nsTArray<nsString> VolumeNameArray;

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMDEVICESTORAGE

  NS_DECL_NSIOBSERVER
  NS_DECL_NSIDOMEVENTTARGET
  virtual void AddEventListener(const nsAString& aType,
                                nsIDOMEventListener* aListener,
                                bool aUseCapture,
                                const mozilla::dom::Nullable<bool>& aWantsUntrusted,
                                mozilla::ErrorResult& aRv) MOZ_OVERRIDE;
  virtual void RemoveEventListener(const nsAString& aType,
                                   nsIDOMEventListener* aListener,
                                   bool aUseCapture,
                                   mozilla::ErrorResult& aRv) MOZ_OVERRIDE;

  nsDOMDeviceStorage();

  nsresult Init(nsPIDOMWindow* aWindow, const nsAString& aType,
                nsTArray<nsRefPtr<nsDOMDeviceStorage> >& aStores);
  nsresult Init(nsPIDOMWindow* aWindow, const nsAString& aType,
                const nsAString& aVolName);

  bool IsAvailable();

  void SetRootDirectoryForType(const nsAString& aType, const nsAString& aVolName);

  static void CreateDeviceStorageFor(nsPIDOMWindow* aWin,
                                     const nsAString& aType,
                                     nsDOMDeviceStorage** aStore);

  static void CreateDeviceStoragesFor(nsPIDOMWindow* aWin,
                                      const nsAString& aType,
                                      nsTArray<nsRefPtr<nsDOMDeviceStorage> >& aStores);
  void Shutdown();

  static void GetOrderedVolumeNames(nsTArray<nsString>& aVolumeNames);

  static void GetWritableStorageName(const nsAString& aStorageType,
                                     nsAString &aStorageName);

  static bool ParseCompositePath(const nsAString& aCompositePath,
                                 nsAString& aOutStorageName,
                                 nsAString& aOutStoragePath);
private:
  ~nsDOMDeviceStorage();

  nsresult GetInternal(const JS::Value & aName,
                       JSContext* aCx,
                       nsIDOMDOMRequest** aRetval,
                       bool aEditable);

  nsresult GetInternal(nsPIDOMWindow* aWin,
                       const nsAString& aPath,
                       mozilla::dom::DOMRequest* aRequest,
                       bool aEditable);

  nsresult DeleteInternal(nsPIDOMWindow* aWin,
                          const nsAString& aPath,
                          mozilla::dom::DOMRequest* aRequest);

  nsresult EnumerateInternal(const JS::Value& aName,
                             const JS::Value& aOptions,
                             JSContext* aCx,
                             uint8_t aArgc,
                             bool aEditable,
                             nsIDOMDOMCursor** aRetval);

  nsString mStorageType;
  nsCOMPtr<nsIFile> mRootDirectory;
  nsString mStorageName;

  bool IsComposite() { return mStores.Length() > 0; }
  nsTArray<nsRefPtr<nsDOMDeviceStorage> > mStores;
  already_AddRefed<nsDOMDeviceStorage> GetStorage(const nsAString& aCompositePath,
                                                  nsAString& aOutStoragePath);
  already_AddRefed<nsDOMDeviceStorage> GetStorageByName(const nsAString &aStorageName);

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
  void DispatchMountChangeEvent(nsAString& aVolumeName,
                                nsAString& aVolumeStatus);
#endif

  
  enum {
      DEVICE_STORAGE_TYPE_DEFAULT = 0,
      DEVICE_STORAGE_TYPE_SHARED,
      DEVICE_STORAGE_TYPE_EXTERNAL
  };
};

#endif
