




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
#include "nsTArray.h"
#include "nsWeakReference.h"
#include "mozilla/MemoryReporting.h"

class nsIFile;
class nsAdoptingString;
class nsAdoptingCString;

#ifndef have_PrefChangedFunc_typedef
typedef void (*PrefChangedFunc)(const char *, void *);
#define have_PrefChangedFunc_typedef
#endif

namespace mozilla {

namespace dom {
class PrefSetting;
}

class Preferences final : public nsIPrefService,
                          public nsIObserver,
                          public nsIPrefBranchInternal,
                          public nsSupportsWeakReference
{
public:
  typedef mozilla::dom::PrefSetting PrefSetting;

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIPREFSERVICE
  NS_FORWARD_NSIPREFBRANCH(sRootBranch->)
  NS_DECL_NSIOBSERVER

  Preferences();

  nsresult Init();

  


  static nsresult ResetAndReadUserPrefs();

  


  static Preferences* GetInstanceForService();

  


  static void Shutdown();

  



  static nsIPrefService* GetService()
  {
    NS_ENSURE_TRUE(InitStaticMembers(), nullptr);
    return sPreferences;
  }

  



  static nsIPrefBranch* GetRootBranch()
  {
    NS_ENSURE_TRUE(InitStaticMembers(), nullptr);
    return sRootBranch;
  }

  



  static nsIPrefBranch* GetDefaultRootBranch()
  {
    NS_ENSURE_TRUE(InitStaticMembers(), nullptr);
    return sDefaultRootBranch;
  }

  



  static bool GetBool(const char* aPref, bool aDefault = false)
  {
    bool result = aDefault;
    GetBool(aPref, &result);
    return result;
  }

  static int32_t GetInt(const char* aPref, int32_t aDefault = 0)
  {
    int32_t result = aDefault;
    GetInt(aPref, &result);
    return result;
  }

  static uint32_t GetUint(const char* aPref, uint32_t aDefault = 0)
  {
    uint32_t result = aDefault;
    GetUint(aPref, &result);
    return result;
  }

  static float GetFloat(const char* aPref, float aDefault = 0)
  {
    float result = aDefault;
    GetFloat(aPref, &result);
    return result;
  }

  




















  static nsAdoptingCString GetCString(const char* aPref);
  static nsAdoptingString GetString(const char* aPref);
  static nsAdoptingCString GetLocalizedCString(const char* aPref);
  static nsAdoptingString GetLocalizedString(const char* aPref);

  







  static nsresult GetBool(const char* aPref, bool* aResult);
  static nsresult GetInt(const char* aPref, int32_t* aResult);
  static nsresult GetFloat(const char* aPref, float* aResult);
  static nsresult GetUint(const char* aPref, uint32_t* aResult)
  {
    int32_t result;
    nsresult rv = GetInt(aPref, &result);
    if (NS_SUCCEEDED(rv)) {
      *aResult = static_cast<uint32_t>(result);
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
  static nsresult SetInt(const char* aPref, int32_t aValue);
  static nsresult SetUint(const char* aPref, uint32_t aValue)
  {
    return SetInt(aPref, static_cast<int32_t>(aValue));
  }
  static nsresult SetFloat(const char* aPref, float aValue);
  static nsresult SetCString(const char* aPref, const char* aValue);
  static nsresult SetCString(const char* aPref, const nsACString &aValue);
  static nsresult SetString(const char* aPref, const char16_t* aValue);
  static nsresult SetString(const char* aPref, const nsAString &aValue);

  static nsresult SetComplex(const char* aPref, const nsIID &aType,
                             nsISupports* aValue);

  


  static nsresult ClearUser(const char* aPref);

  


  static bool HasUserValue(const char* aPref);

  


  static int32_t GetType(const char* aPref);

  





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
                                   void* aClosure = nullptr);
  static nsresult UnregisterCallback(PrefChangedFunc aCallback,
                                     const char* aPref,
                                     void* aClosure = nullptr);
  
  
  static nsresult RegisterCallbackAndCall(PrefChangedFunc aCallback,
                                          const char* aPref,
                                          void* aClosure = nullptr);

  





  static nsresult AddBoolVarCache(bool* aVariable,
                                  const char* aPref,
                                  bool aDefault = false);
  static nsresult AddIntVarCache(int32_t* aVariable,
                                 const char* aPref,
                                 int32_t aDefault = 0);
  static nsresult AddUintVarCache(uint32_t* aVariable,
                                  const char* aPref,
                                  uint32_t aDefault = 0);
  static nsresult AddFloatVarCache(float* aVariable,
                                   const char* aPref,
                                   float aDefault = 0.0f);

  





  static nsresult GetDefaultBool(const char* aPref, bool* aResult);
  static nsresult GetDefaultInt(const char* aPref, int32_t* aResult);
  static nsresult GetDefaultUint(const char* aPref, uint32_t* aResult)
  {
    return GetDefaultInt(aPref, reinterpret_cast<int32_t*>(aResult));
  }

  





  static bool GetDefaultBool(const char* aPref, bool aFailedResult)
  {
    bool result;
    return NS_SUCCEEDED(GetDefaultBool(aPref, &result)) ? result :
                                                          aFailedResult;
  }
  static int32_t GetDefaultInt(const char* aPref, int32_t aFailedResult)
  {
    int32_t result;
    return NS_SUCCEEDED(GetDefaultInt(aPref, &result)) ? result : aFailedResult;
  }
  static uint32_t GetDefaultUint(const char* aPref, uint32_t aFailedResult)
  {
   return static_cast<uint32_t>(
     GetDefaultInt(aPref, static_cast<int32_t>(aFailedResult)));
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

  


  static int32_t GetDefaultType(const char* aPref);

  
  static void GetPreferences(InfallibleTArray<PrefSetting>* aPrefs);
  static void GetPreference(PrefSetting* aPref);
  static void SetPreference(const PrefSetting& aPref);

  static int64_t SizeOfIncludingThisAndOtherStuff(mozilla::MallocSizeOf aMallocSizeOf);

protected:
  virtual ~Preferences();

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
