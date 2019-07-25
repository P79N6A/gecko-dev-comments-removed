





#include "nsIDirectoryService.h"
#include "nsIFile.h"

#include "nsCOMPtr.h"
#include "nsDirectoryServiceUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"

#ifdef MOZILLA_INTERNAL_API
#include "nsString.h"
#else
#include "nsEmbedString.h"
#endif


class nsProfileLock;






class nsProfileDirServiceProvider: public nsIDirectoryServiceProvider
{  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDIRECTORYSERVICEPROVIDER

  friend nsresult NS_NewProfileDirServiceProvider(bool, nsProfileDirServiceProvider**);

public:

   











   virtual nsresult        SetProfileDir(nsIFile* aProfileDir,
                                         nsIFile* aLocalProfileDir = nsnull);

  







  virtual nsresult         Register();

  







  virtual nsresult         Shutdown();

protected:
                           nsProfileDirServiceProvider(bool aNotifyObservers = true);
   virtual                 ~nsProfileDirServiceProvider();

  nsresult                 Initialize();
  nsresult                 InitProfileDir(nsIFile* profileDir);
  nsresult                 InitNonSharedProfileDir();
  nsresult                 EnsureProfileFileExists(nsIFile *aFile, nsIFile *destDir);
  nsresult                 UndefineFileLocations();

protected:

  nsCOMPtr<nsIFile>        mProfileDir;
  nsCOMPtr<nsIFile>        mLocalProfileDir;
  nsProfileLock*           mProfileDirLock;
  bool                     mNotifyObservers;

  bool                     mSharingEnabled;
#ifndef MOZILLA_INTERNAL_API
  nsEmbedString            mNonSharedDirName;
#else
  nsString                 mNonSharedDirName;
#endif
  nsCOMPtr<nsIFile>        mNonSharedProfileDir;
};











 
nsresult NS_NewProfileDirServiceProvider(bool aNotifyObservers,
                                         nsProfileDirServiceProvider** aProvider);

