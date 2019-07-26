




#ifndef _nsXREDirProvider_h__
#define _nsXREDirProvider_h__

#include "nsIDirectoryService.h"
#include "nsIProfileMigrator.h"
#include "nsIFile.h"

#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "mozilla/Attributes.h"

class nsXREDirProvider MOZ_FINAL : public nsIDirectoryServiceProvider2,
                                   public nsIProfileStartup
{
public:
  
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

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

  
  void LoadAppBundleDirs();

  void Append(nsIFile* aDirectory);

  nsCOMPtr<nsIDirectoryServiceProvider> mAppProvider;
  nsCOMPtr<nsIFile>      mGREDir;
  nsCOMPtr<nsIFile>      mXULAppDir;
  nsCOMPtr<nsIFile>      mProfileDir;
  nsCOMPtr<nsIFile>      mProfileLocalDir;
  bool                   mProfileNotified;
  nsCOMArray<nsIFile>    mAppBundleDirectories;
  nsCOMArray<nsIFile>    mExtensionDirectories;
  nsCOMArray<nsIFile>    mThemeDirectories;
};

#endif
