






































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
class nsAdoptingString;
class nsAdoptingCString;

#ifndef have_PrefChangedFunc_typedef
typedef int (*PR_CALLBACK PrefChangedFunc)(const char *, void *);
#define have_PrefChangedFunc_typedef
#endif

namespace mozilla {

class Preferences : public nsIPrefService,
                    public nsIObserver,
                    public nsIPrefBranchInternal,
                    public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPREFSERVICE
  NS_FORWARD_NSIPREFBRANCH(sRootBranch->)
  NS_DECL_NSIOBSERVER

  Preferences();
  virtual ~Preferences();

  nsresult Init();

  


  static nsresult ResetAndReadUserPrefs();

  


  static Preferences* GetInstanceForService();

  


  static void Shutdown();

  



  static nsIPrefService* GetService()
  {
    NS_ENSURE_TRUE(InitStaticMembers(), nsnull);
    return sPreferences;
  }

  



  static nsIPrefBranch* GetRootBranch()
  {
    NS_ENSURE_TRUE(InitStaticMembers(), nsnull);
    return sRootBranch;
  }

  



  static nsIPrefBranch* GetDefaultRootBranch()
  {
    NS_ENSURE_TRUE(InitStaticMembers(), nsnull);
    return sDefaultRootBranch;
  }

  



  static bool GetBool(const char* aPref, bool aDefault = false)
  {
    bool result = aDefault;
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

  




















  static nsAdoptingCString GetCString(const char* aPref);
  static nsAdoptingString GetString(const char* aPref);
  static nsAdoptingCString GetLocalizedCString(const char* aPref);
  static nsAdoptingString GetLocalizedString(const char* aPref);

  






  static nsresult GetBool(const char* aPref, bool* aResult);
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

  






  static nsresult GetCString(const char* aPref, nsACString* aResult);
  static nsresult GetString(const char* aPref, nsAString* aResult);
  static nsresult GetLocalizedCString(const char* aPref, nsACString* aResult);
  static nsresult GetLocalizedString(const char* aPref, nsAString* aResult);

  static nsresult GetComplex(const char* aPref, const nsIID &aType,
                             void** aResult);

  


  static nsresult SetBool(const char* aPref, bool aValue);
  static nsresult SetInt(const char* aPref, PRInt32 aValue);
  static nsresult SetUint(const char* aPref, PRUint32 aValue)
  {
    return SetInt(aPref, static_cast<PRInt32>(aValue));
  }
  static nsresult SetCString(const char* aPref, const char* aValue);
  static nsresult SetCString(const char* aPref, const nsACString &aValue);
  static nsresult SetString(const char* aPref, const PRUnichar* aValue);
  static nsresult SetString(const char* aPref, const nsAString &aValue);

  static nsresult SetComplex(const char* aPref, const nsIID &aType,
                             nsISupports* aValue);

  


  static nsresult ClearUser(const char* aPref);

  


  static bool HasUserValue(const char* aPref);

  


  static PRInt32 GetType(const char* aPref);

  





  static nsresult AddStrongObserver(nsIObserver* aObserver, const char* aPref);
  static nsresult AddWeakObserver(nsIObserver* aObserver, const char* aPref);
  static nsresult RemoveObserver(nsIObserver* aObserver, const char* aPref);

  



  static nsresult AddStrongObservers(nsIObserver* aObserver,
                                     const char** aPrefs);
  static nsresult AddWeakObservers(nsIObserver* aObserver,
                                   const char** aPrefs);
  static nsresult RemoveObservers(nsIObserver* aObserver,
                                  const char** aPrefs);

  


  static nsresult RegisterCallback(PrefChangedFunc aCallback,
                                   const char* aPref,
                                   void* aClosure = nsnull);
  static nsresult UnregisterCallback(PrefChangedFunc aCallback,
                                     const char* aPref,
                                     void* aClosure = nsnull);

  





  static nsresult AddBoolVarCache(bool* aVariable,
                                  const char* aPref,
                                  bool aDefault = false);
  static nsresult AddIntVarCache(PRInt32* aVariable,
                                 const char* aPref,
                                 PRInt32 aDefault = 0);
  static nsresult AddUintVarCache(PRUint32* aVariable,
                                  const char* aPref,
                                  PRUint32 aDefault = 0);

  





  static nsresult GetDefaultBool(const char* aPref, bool* aResult);
  static nsresult GetDefaultInt(const char* aPref, PRInt32* aResult);
  static nsresult GetDefaultUint(const char* aPref, PRUint32* aResult)
  {
    return GetDefaultInt(aPref, reinterpret_cast<PRInt32*>(aResult));
  }

  





  static bool GetDefaultBool(const char* aPref, bool aFailedResult)
  {
    bool result;
    return NS_SUCCEEDED(GetDefaultBool(aPref, &result)) ? result :
                                                          aFailedResult;
  }
  static PRInt32 GetDefaultInt(const char* aPref, PRInt32 aFailedResult)
  {
    PRInt32 result;
    return NS_SUCCEEDED(GetDefaultInt(aPref, &result)) ? result : aFailedResult;
  }
  static PRUint32 GetDefaultUint(const char* aPref, PRUint32 aFailedResult)
  {
   return static_cast<PRUint32>(
     GetDefaultInt(aPref, static_cast<PRInt32>(aFailedResult)));
  }

  







  static nsAdoptingString GetDefaultString(const char* aPref);
  static nsAdoptingCString GetDefaultCString(const char* aPref);
  static nsAdoptingString GetDefaultLocalizedString(const char* aPref);
  static nsAdoptingCString GetDefaultLocalizedCString(const char* aPref);

  static nsresult GetDefaultCString(const char* aPref, nsACString* aResult);
  static nsresult GetDefaultString(const char* aPref, nsAString* aResult);
  static nsresult GetDefaultLocalizedCString(const char* aPref,
                                             nsACString* aResult);
  static nsresult GetDefaultLocalizedString(const char* aPref,
                                            nsAString* aResult);

  static nsresult GetDefaultComplex(const char* aPref, const nsIID &aType,
                                    void** aResult);

  


  static PRInt32 GetDefaultType(const char* aPref);

  
  static void MirrorPreferences(nsTArray<PrefTuple,
                                nsTArrayInfallibleAllocator> *aArray);
  static bool MirrorPreference(const char *aPref, PrefTuple *aTuple);
  static void ClearContentPref(const char *aPref);
  static void SetPreference(const PrefTuple *aTuple);

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
  nsCOMPtr<nsIFile>        mCurrentFile;

  static Preferences*      sPreferences;
  static nsIPrefBranch*    sRootBranch;
  static nsIPrefBranch*    sDefaultRootBranch;
  static bool              sShutdown;

  


  static bool InitStaticMembers();
};

} 

#endif 
