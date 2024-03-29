




#ifndef _nsXREDirProvider_h__
#define _nsXREDirProvider_h__

#include "nsIDirectoryService.h"
#include "nsIProfileMigrator.h"
#include "nsIFile.h"

#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "mozilla/Attributes.h"

class nsXREDirProvider final : public nsIDirectoryServiceProvider2,
                               public nsIProfileStartup
{
public:
  
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr) override;
  NS_IMETHOD_(MozExternalRefCountType) AddRef(void) override;
  NS_IMETHOD_(MozExternalRefCountType) Release(void) override;

  NS_DECL_NSIDIRECTORYSERVICEPROVIDER
  NS_DECL_NSIDIRECTORYSERVICEPROVIDER2
  NS_DECL_NSIPROFILESTARTUP

  nsXREDirProvider();

  
  nsresult Initialize(nsIFile *aXULAppDir,
                      nsIFile *aGREDir,
                      nsIDirectoryServiceProvider* aAppProvider = nullptr);
  ~nsXREDirProvider();

  static nsXREDirProvider* GetSingleton();

  nsresult GetUserProfilesRootDir(nsIFile** aResult,
                                  const nsACString* aProfileName,
                                  const nsACString* aAppName,
                                  const nsACString* aVendorName);
  nsresult GetUserProfilesLocalDir(nsIFile** aResult,
                                   const nsACString* aProfileName,
                                   const nsACString* aAppName,
                                   const nsACString* aVendorName);

  
  
  
  
  nsresult SetProfile(nsIFile* aProfileDir, nsIFile* aProfileLocalDir);

  void DoShutdown();

  nsresult GetProfileDefaultsDir(nsIFile* *aResult);

  static nsresult GetUserAppDataDirectory(nsIFile* *aFile) {
    return GetUserDataDirectory(aFile, false, nullptr, nullptr, nullptr);
  }
  static nsresult GetUserLocalDataDirectory(nsIFile* *aFile) {
    return GetUserDataDirectory(aFile, true, nullptr, nullptr, nullptr);
  }

  
  
  static nsresult GetUserDataDirectory(nsIFile** aFile, bool aLocal,
                                       const nsACString* aProfileName,
                                       const nsACString* aAppName,
                                       const nsACString* aVendorName);

  
  nsIFile* GetGREDir() { return mGREDir; }
  nsIFile* GetGREBinDir() { return mGREBinDir; }
  nsIFile* GetAppDir() {
    if (mXULAppDir)
      return mXULAppDir;
    return mGREDir;
  }

  




  nsresult GetUpdateRootDir(nsIFile* *aResult);

  




  nsresult GetProfileStartupDir(nsIFile* *aResult);

  




  nsresult GetProfileDir(nsIFile* *aResult);

protected:
  nsresult GetFilesInternal(const char* aProperty, nsISimpleEnumerator** aResult);
  static nsresult GetUserDataDirectoryHome(nsIFile* *aFile, bool aLocal);
  static nsresult GetSysUserExtensionsDirectory(nsIFile* *aFile);
#if defined(XP_UNIX) || defined(XP_MACOSX)
  static nsresult GetSystemExtensionsDirectory(nsIFile** aFile);
#endif
  static nsresult EnsureDirectoryExists(nsIFile* aDirectory);
  void EnsureProfileFileExists(nsIFile* aFile);

  
  
  static nsresult AppendProfilePath(nsIFile* aFile,
                                    const nsACString* aProfileName,
                                    const nsACString* aAppName,
                                    const nsACString* aVendorName,
                                    bool aLocal);

  static nsresult AppendSysUserExtensionPath(nsIFile* aFile);

  
  
  static inline nsresult AppendProfileString(nsIFile* aFile, const char* aPath);

  
  void LoadExtensionBundleDirectories();

#ifdef MOZ_B2G
  
  void LoadAppBundleDirs();
#endif

  void Append(nsIFile* aDirectory);

  nsCOMPtr<nsIDirectoryServiceProvider> mAppProvider;
  
  nsCOMPtr<nsIFile>      mGREDir;
  
  nsCOMPtr<nsIFile>      mGREBinDir;
  
  nsCOMPtr<nsIFile>      mXULAppDir;
  nsCOMPtr<nsIFile>      mProfileDir;
  nsCOMPtr<nsIFile>      mProfileLocalDir;
  bool                   mProfileNotified;
  nsCOMArray<nsIFile>    mAppBundleDirectories;
  nsCOMArray<nsIFile>    mExtensionDirectories;
  nsCOMArray<nsIFile>    mThemeDirectories;
};

#endif
