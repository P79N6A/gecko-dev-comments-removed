



#ifndef DeviceStorage_h
#define DeviceStorage_h

#include "nsIDOMDeviceStorage.h"
#include "nsIFile.h"
#include "nsIPrincipal.h"
#include "nsIObserver.h"
#include "nsDOMEventTargetHelper.h"
#include "mozilla/StaticPtr.h"

class DeviceStorageFile MOZ_FINAL
  : public nsISupports {
public:
  nsCOMPtr<nsIFile> mFile;
  nsString mPath;
  nsString mStorageType;
  bool mEditable;

  DeviceStorageFile(const nsAString& aStorageType, nsIFile* aFile, const nsAString& aPath);
  DeviceStorageFile(const nsAString& aStorageType, nsIFile* aFile);
  void SetPath(const nsAString& aPath);
  void SetEditable(bool aEditable);

  NS_DECL_ISUPPORTS

  
  
  bool IsSafePath();

  nsresult Remove();
  nsresult Write(nsIInputStream* aInputStream);
  nsresult Write(InfallibleTArray<uint8_t>& bits);
  void CollectFiles(nsTArray<nsRefPtr<DeviceStorageFile> > &aFiles, PRTime aSince = 0);
  void collectFilesInternal(nsTArray<nsRefPtr<DeviceStorageFile> > &aFiles, PRTime aSince, nsAString& aRootPath);

  static void DirectoryDiskUsage(nsIFile* aFile, uint64_t* aSoFar, const nsAString& aStorageType);

private:
  void NormalizeFilePath();
  void AppendRelativePath();
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
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMDEVICESTORAGE

  NS_DECL_NSIOBSERVER
  NS_DECL_NSIDOMEVENTTARGET

  nsDOMDeviceStorage();

  nsresult Init(nsPIDOMWindow* aWindow, const nsAString &aType, const nsAString &aVolName);

  void SetRootDirectoryForType(const nsAString& aType, const nsAString &aVolName);

  static void GetOrderedVolumeNames(const nsAString &aType,
                                    nsTArray<nsString> &aVolumeNames);

  static void CreateDeviceStoragesFor(nsPIDOMWindow* aWin,
                                      const nsAString &aType,
                                      nsTArray<nsRefPtr<nsDOMDeviceStorage> > &aStores);
  void Shutdown();

private:
  ~nsDOMDeviceStorage();

  nsresult GetInternal(const JS::Value & aName,
                       JSContext* aCx,
                       nsIDOMDOMRequest** aRetval,
                       bool aEditable);

  nsresult EnumerateInternal(const JS::Value & aName,
                             const JS::Value & aOptions,
                             JSContext* aCx,
                             uint8_t aArgc,
                             bool aEditable,
                             nsIDOMDOMCursor** aRetval);

  nsString mStorageType;
  nsCOMPtr<nsIFile> mRootDirectory;
  nsString mVolumeName;

  nsCOMPtr<nsIPrincipal> mPrincipal;

  bool mIsWatchingFile;
  bool mAllowedToWatchFile;

  nsresult Notify(const char* aReason, class DeviceStorageFile* aFile);

  friend class WatchFileEvent;
  friend class DeviceStorageRequest;

#ifdef MOZ_WIDGET_GONK
  void DispatchMountChangeEvent(nsAString& aType);
#endif

  
  enum {
      DEVICE_STORAGE_TYPE_DEFAULT = 0,
      DEVICE_STORAGE_TYPE_SHARED,
      DEVICE_STORAGE_TYPE_EXTERNAL
  };
};

#endif
