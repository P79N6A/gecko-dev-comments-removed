





































#ifndef _nsXREDirProvider_h__
#define _nsXREDirProvider_h__

#include "nsIDirectoryService.h"
#include "nsIProfileMigrator.h"
#include "nsILocalFile.h"

#include "nsCOMPtr.h"
#include "nsCOMArray.h"

class nsXREDirProvider : public nsIDirectoryServiceProvider2,
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
                      nsILocalFile *aGREDir,
                      nsIDirectoryServiceProvider* aAppProvider = nsnull);
  ~nsXREDirProvider();

  
  
  
  
  nsresult SetProfile(nsIFile* aProfileDir, nsIFile* aProfileLocalDir);

  void DoShutdown();

  nsresult GetProfileDefaultsDir(nsIFile* *aResult);

  static nsresult GetUserAppDataDirectory(nsILocalFile* *aFile) {
    return GetUserDataDirectory(aFile, PR_FALSE);
  }
  static nsresult GetUserLocalDataDirectory(nsILocalFile* *aFile) {
    return GetUserDataDirectory(aFile, PR_TRUE);
  }

  
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
  static nsresult GetUserDataDirectory(nsILocalFile* *aFile, bool aLocal);
  static nsresult GetUserDataDirectoryHome(nsILocalFile* *aFile, bool aLocal);
  static nsresult GetSysUserExtensionsDirectory(nsILocalFile* *aFile);
#if defined(XP_UNIX) || defined(XP_MACOSX)
  static nsresult GetSystemExtensionsDirectory(nsILocalFile** aFile);
#endif
  static nsresult EnsureDirectoryExists(nsIFile* aDirectory);
  void EnsureProfileFileExists(nsIFile* aFile);

  
  
  static nsresult AppendProfilePath(nsIFile* aFile);

  static nsresult AppendSysUserExtensionPath(nsIFile* aFile);

  
  
  static inline nsresult AppendProfileString(nsIFile* aFile, const char* aPath);

  
  void LoadExtensionBundleDirectories();

  
  void LoadAppBundleDirs();

  void Append(nsIFile* aDirectory);

  nsCOMPtr<nsIDirectoryServiceProvider> mAppProvider;
  nsCOMPtr<nsILocalFile> mGREDir;
  nsCOMPtr<nsIFile>      mXULAppDir;
  nsCOMPtr<nsIFile>      mProfileDir;
  nsCOMPtr<nsIFile>      mProfileLocalDir;
  bool                   mProfileNotified;
  nsCOMArray<nsIFile>    mAppBundleDirectories;
  nsCOMArray<nsIFile>    mExtensionDirectories;
  nsCOMArray<nsIFile>    mThemeDirectories;
};

#endif
