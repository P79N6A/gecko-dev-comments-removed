






































#ifndef mozilla_Preferences_h
#define mozilla_Preferences_h

#ifndef MOZILLA_INTERNAL_API
#error "This header is only usable from within libxul (MOZILLA_INTERNAL_API)."
#endif

#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranchInternal.h"
#include "nsIObserver.h"
#include "nsCOMPtr.h"
#include "nsWeakReference.h"

class nsIFile;

namespace mozilla {

class Preferences : public nsIPrefService,
                    public nsIPrefServiceInternal,
                    public nsIObserver,
                    public nsIPrefBranchInternal,
                    public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPREFSERVICE
  NS_DECL_NSIPREFSERVICEINTERNAL
  NS_FORWARD_NSIPREFBRANCH(mRootBranch->)
  NS_FORWARD_NSIPREFBRANCH2(mRootBranch->)
  NS_DECL_NSIOBSERVER

  Preferences();
  virtual ~Preferences();

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
  nsCOMPtr<nsIFile>        mCurrentFile;
};

} 

#endif 
