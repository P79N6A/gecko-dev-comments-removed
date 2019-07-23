





































#ifndef nsPrefService_h__
#define nsPrefService_h__

#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranchInternal.h"
#include "nsIObserver.h"
#include "nsCOMPtr.h"
#include "nsWeakReference.h"

class nsIFile;

class nsPrefService : public nsIPrefService,
                      public nsIObserver,
                      public nsIPrefBranchInternal,
                      public nsSupportsWeakReference
{
  friend class nsSharedPrefHandler; 
  
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPREFSERVICE
  NS_FORWARD_NSIPREFBRANCH(mRootBranch->)
  NS_FORWARD_NSIPREFBRANCH2(mRootBranch->)
  NS_DECL_NSIOBSERVER

  nsPrefService();
  virtual ~nsPrefService();

  nsresult Init();
                           
protected:
  nsresult NotifyServiceObservers(const char *aSubject);
  nsresult UseDefaultPrefFile();
  nsresult UseUserPrefFile();
  nsresult ReadAndOwnUserPrefFile(nsIFile *aFile);
  nsresult ReadAndOwnSharedUserPrefFile(nsIFile *aFile);
  nsresult SavePrefFileInternal(nsIFile* aFile);
  nsresult WritePrefFile(nsIFile* aFile);
  nsresult MakeBackupPrefFile(nsIFile *aFile);

private:
  nsCOMPtr<nsIPrefBranch2> mRootBranch;
  nsCOMPtr<nsIFile>       mCurrentFile;
  PRPackedBool            mDontWriteUserPrefs;
#if MOZ_PROFILESHARING
  nsCOMPtr<nsIFile>       mCurrentSharedFile;
  PRPackedBool            mDontWriteSharedUserPrefs;
#endif
};

#endif 
