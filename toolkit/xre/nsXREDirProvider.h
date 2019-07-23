





































#ifndef _nsXREDirProvider_h__
#define _nsXREDirProvider_h__

#include "nsIDirectoryService.h"
#include "nsIProfileMigrator.h"
#include "nsILocalFile.h"

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

  




  nsresult GetProfileStartupDir(nsIFile* *aResult);   

  




  nsresult GetProfileDir(nsIFile* *aResult);

protected:
  nsresult GetFilesInternal(const char* aProperty, nsISimpleEnumerator** aResult);
  static nsresult GetUserDataDirectory(nsILocalFile* *aFile, PRBool aLocal);
  static nsresult EnsureDirectoryExists(nsIFile* aDirectory);
  void EnsureProfileFileExists(nsIFile* aFile);

  nsCOMPtr<nsIDirectoryServiceProvider> mAppProvider;
  nsCOMPtr<nsILocalFile> mGREDir;
  nsCOMPtr<nsIFile>      mXULAppDir;
  nsCOMPtr<nsIFile>      mProfileDir;
  nsCOMPtr<nsIFile>      mProfileLocalDir;
  PRBool                 mProfileNotified;
};

#endif
