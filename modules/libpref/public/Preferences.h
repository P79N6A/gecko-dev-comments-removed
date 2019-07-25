






































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
class nsCString;
class nsString;

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

  


  static Preferences* GetInstance();

  


  static void Shutdown();

  



  static nsIPrefService* GetService() { return sPreferences; }

  



  static nsIPrefBranch2* GetRootBranch()
  {
    return sPreferences ? sPreferences->mRootBranch.get() : nsnull;
  }

  



  static PRBool GetBool(const char* aPref, PRBool aDefault = PR_FALSE)
  {
    PRBool result = aDefault;
    GetBool(aPref, &result);
    return result;
  }

  static PRInt32 GetInt(const char* aPref, PRInt32 aDefault = 0)
  {
    PRInt32 result = aDefault;
    GetInt(aPref, &result);
    return result;
  }

  static PRUint32 GetUint(const char* aPref, PRUint32 aDefault = 0)
  {
    PRUint32 result = aDefault;
    GetUint(aPref, &result);
    return result;
  }

  






  static nsresult GetBool(const char* aPref, PRBool* aResult);
  static nsresult GetInt(const char* aPref, PRInt32* aResult);
  static nsresult GetUint(const char* aPref, PRUint32* aResult)
  {
    PRInt32 result;
    nsresult rv = GetInt(aPref, &result);
    if (NS_SUCCEEDED(rv)) {
      *aResult = static_cast<PRUint32>(result);
    }
    return rv;
  }

  






  static nsresult GetChar(const char* aPref, nsCString* aResult);
  static nsresult GetChar(const char* aPref, nsString* aResult);
  static nsresult GetLocalizedString(const char* aPref, nsString* aResult);

  


  static nsresult SetBool(const char* aPref, PRBool aValue);
  static nsresult SetInt(const char* aPref, PRInt32 aValue);
  static nsresult SetUint(const char* aPref, PRUint32 aValue)
  {
    return SetInt(aPref, static_cast<PRInt32>(aValue));
  }
  static nsresult SetChar(const char* aPref, const char* aValue);
  static nsresult SetChar(const char* aPref, const nsCString &aValue);
  static nsresult SetChar(const char* aPref, const PRUnichar* aValue);
  static nsresult SetChar(const char* aPref, const nsString &aValue);

  


  static nsresult ClearUser(const char* aPref);

  





  static nsresult AddStrongObserver(nsIObserver* aObserver, const char* aPref);
  static nsresult AddWeakObserver(nsIObserver* aObserver, const char* aPref);
  static nsresult RemoveObserver(nsIObserver* aObserver, const char* aPref);

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

  static Preferences*      sPreferences;
  static PRBool            sShutdown;

  


  static PRBool InitStaticMembers();
};

} 

#endif 
