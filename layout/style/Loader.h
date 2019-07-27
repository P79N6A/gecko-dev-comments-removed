






#ifndef mozilla_css_Loader_h
#define mozilla_css_Loader_h

#include "nsIPrincipal.h"
#include "nsAutoPtr.h"
#include "nsCompatibility.h"
#include "nsCycleCollectionParticipant.h"
#include "nsDataHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsStringFwd.h"
#include "nsTArray.h"
#include "nsTObserverArray.h"
#include "nsURIHashKey.h"
#include "mozilla/Attributes.h"
#include "mozilla/CORSMode.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/net/ReferrerPolicy.h"

class nsICSSLoaderObserver;
class nsIContent;
class nsIDocument;
class nsMediaList;
class nsIStyleSheetLinkingElement;

namespace mozilla {
class CSSStyleSheet;
namespace dom {
class Element;
}
}

namespace mozilla {

class URIPrincipalReferrerPolicyAndCORSModeHashKey : public nsURIHashKey
{
public:
  typedef URIPrincipalReferrerPolicyAndCORSModeHashKey* KeyType;
  typedef const URIPrincipalReferrerPolicyAndCORSModeHashKey* KeyTypePointer;
  typedef mozilla::net::ReferrerPolicy ReferrerPolicy;

  explicit URIPrincipalReferrerPolicyAndCORSModeHashKey(const URIPrincipalReferrerPolicyAndCORSModeHashKey* aKey)
    : nsURIHashKey(aKey->mKey),
      mPrincipal(aKey->mPrincipal),
      mCORSMode(aKey->mCORSMode),
      mReferrerPolicy(aKey->mReferrerPolicy)
  {
    MOZ_COUNT_CTOR(URIPrincipalReferrerPolicyAndCORSModeHashKey);
  }

  URIPrincipalReferrerPolicyAndCORSModeHashKey(nsIURI* aURI,
                                               nsIPrincipal* aPrincipal,
                                               CORSMode aCORSMode,
                                               ReferrerPolicy aReferrerPolicy)
    : nsURIHashKey(aURI),
      mPrincipal(aPrincipal),
      mCORSMode(aCORSMode),
      mReferrerPolicy(aReferrerPolicy)
  {
    MOZ_COUNT_CTOR(URIPrincipalReferrerPolicyAndCORSModeHashKey);
  }
  URIPrincipalReferrerPolicyAndCORSModeHashKey(const URIPrincipalReferrerPolicyAndCORSModeHashKey& toCopy)
    : nsURIHashKey(toCopy),
      mPrincipal(toCopy.mPrincipal),
      mCORSMode(toCopy.mCORSMode),
      mReferrerPolicy(toCopy.mReferrerPolicy)
  {
    MOZ_COUNT_CTOR(URIPrincipalReferrerPolicyAndCORSModeHashKey);
  }
  ~URIPrincipalReferrerPolicyAndCORSModeHashKey()
  {
    MOZ_COUNT_DTOR(URIPrincipalReferrerPolicyAndCORSModeHashKey);
  }

  URIPrincipalReferrerPolicyAndCORSModeHashKey* GetKey() const {
    return const_cast<URIPrincipalReferrerPolicyAndCORSModeHashKey*>(this);
  }
  const URIPrincipalReferrerPolicyAndCORSModeHashKey* GetKeyPointer() const { return this; }

  bool KeyEquals(const URIPrincipalReferrerPolicyAndCORSModeHashKey* aKey) const {
    if (!nsURIHashKey::KeyEquals(aKey->mKey)) {
      return false;
    }

    if (!mPrincipal != !aKey->mPrincipal) {
      
      return false;
    }

    if (mCORSMode != aKey->mCORSMode) {
      
      return false;
    }

    if (mReferrerPolicy != aKey->mReferrerPolicy) {
      
      return false;
    }

    bool eq;
    return !mPrincipal ||
      (NS_SUCCEEDED(mPrincipal->Equals(aKey->mPrincipal, &eq)) && eq);
  }

  static const URIPrincipalReferrerPolicyAndCORSModeHashKey*
  KeyToPointer(URIPrincipalReferrerPolicyAndCORSModeHashKey* aKey) { return aKey; }
  static PLDHashNumber HashKey(const URIPrincipalReferrerPolicyAndCORSModeHashKey* aKey) {
    return nsURIHashKey::HashKey(aKey->mKey);
  }

  nsIURI* GetURI() const { return nsURIHashKey::GetKey(); }

  enum { ALLOW_MEMMOVE = true };

protected:
  nsCOMPtr<nsIPrincipal> mPrincipal;
  CORSMode mCORSMode;
  ReferrerPolicy mReferrerPolicy;
};



namespace css {

class SheetLoadData;
class ImportRule;




enum StyleSheetState {
  eSheetStateUnknown = 0,
  eSheetNeedsParser,
  eSheetPending,
  eSheetLoading,
  eSheetComplete
};

class Loader final {
  typedef mozilla::net::ReferrerPolicy ReferrerPolicy;

public:
  Loader();
  explicit Loader(nsIDocument*);

 private:
  
  ~Loader();

 public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(Loader)
  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(Loader)

  void DropDocumentReference(); 

  void SetCompatibilityMode(nsCompatibility aCompatMode)
  { mCompatMode = aCompatMode; }
  nsCompatibility GetCompatibilityMode() { return mCompatMode; }
  nsresult SetPreferredSheet(const nsAString& aTitle);

  

  



















  nsresult LoadInlineStyle(nsIContent* aElement,
                           const nsAString& aBuffer,
                           uint32_t aLineNumber,
                           const nsAString& aTitle,
                           const nsAString& aMedia,
                           mozilla::dom::Element* aScopeElement,
                           nsICSSLoaderObserver* aObserver,
                           bool* aCompleted,
                           bool* aIsAlternate);

  



















  nsresult LoadStyleLink(nsIContent* aElement,
                         nsIURI* aURL,
                         const nsAString& aTitle,
                         const nsAString& aMedia,
                         bool aHasAlternateRel,
                         CORSMode aCORSMode,
                         ReferrerPolicy aReferrerPolicy,
                         nsICSSLoaderObserver* aObserver,
                         bool* aIsAlternate);

  















  nsresult LoadChildSheet(CSSStyleSheet* aParentSheet,
                          nsIURI* aURL,
                          nsMediaList* aMedia,
                          ImportRule* aRule);

  


























  nsresult LoadSheetSync(nsIURI* aURL, bool aEnableUnsafeRules,
                         bool aUseSystemPrincipal,
                         CSSStyleSheet** aSheet);

  


  nsresult LoadSheetSync(nsIURI* aURL, CSSStyleSheet** aSheet) {
    return LoadSheetSync(aURL, false, false, aSheet);
  }

  



















  nsresult LoadSheet(nsIURI* aURL,
                     nsIPrincipal* aOriginPrincipal,
                     const nsCString& aCharset,
                     nsICSSLoaderObserver* aObserver,
                     CSSStyleSheet** aSheet);

  



  nsresult LoadSheet(nsIURI* aURL,
                     nsIPrincipal* aOriginPrincipal,
                     const nsCString& aCharset,
                     nsICSSLoaderObserver* aObserver,
                     CORSMode aCORSMode = CORS_NONE,
                     ReferrerPolicy aReferrerPolicy = mozilla::net::RP_Default);

  



  nsresult Stop(void);

  





  






  bool GetEnabled() { return mEnabled; }
  void SetEnabled(bool aEnabled) { mEnabled = aEnabled; }

  


  nsIDocument* GetDocument() const { return mDocument; }

  









  bool HasPendingLoads();

  








  nsresult AddObserver(nsICSSLoaderObserver* aObserver);

  


  void RemoveObserver(nsICSSLoaderObserver* aObserver);

  
  

  
  
  bool IsAlternate(const nsAString& aTitle, bool aHasAlternateRel);

  typedef nsTArray<nsRefPtr<SheetLoadData> > LoadDataArray;

  
  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  
  
  nsresult ObsoleteSheet(nsIURI* aURI);

private:
  friend class SheetLoadData;

  static PLDHashOperator
  RemoveEntriesWithURI(URIPrincipalReferrerPolicyAndCORSModeHashKey* aKey,
                       nsRefPtr<CSSStyleSheet>& aSheet,
                       void* aUserData);

  
  
  nsresult CheckLoadAllowed(nsIPrincipal* aSourcePrincipal,
                            nsIURI* aTargetURI,
                            nsISupports* aContext);


  
  
  
  
  nsresult CreateSheet(nsIURI* aURI,
                       nsIContent* aLinkingContent,
                       nsIPrincipal* aLoaderPrincipal,
                       CORSMode aCORSMode,
                       ReferrerPolicy aReferrerPolicy,
                       bool aSyncLoad,
                       bool aHasAlternateRel,
                       const nsAString& aTitle,
                       StyleSheetState& aSheetState,
                       bool *aIsAlternate,
                       CSSStyleSheet** aSheet);

  
  
  
  void PrepareSheet(CSSStyleSheet* aSheet,
                    const nsAString& aTitle,
                    const nsAString& aMediaString,
                    nsMediaList* aMediaList,
                    dom::Element* aScopeElement,
                    bool isAlternate);

  nsresult InsertSheetInDoc(CSSStyleSheet* aSheet,
                            nsIContent* aLinkingContent,
                            nsIDocument* aDocument);

  nsresult InsertChildSheet(CSSStyleSheet* aSheet,
                            CSSStyleSheet* aParentSheet,
                            ImportRule* aParentRule);

  nsresult InternalLoadNonDocumentSheet(nsIURI* aURL,
                                        bool aAllowUnsafeRules,
                                        bool aUseSystemPrincipal,
                                        nsIPrincipal* aOriginPrincipal,
                                        const nsCString& aCharset,
                                        CSSStyleSheet** aSheet,
                                        nsICSSLoaderObserver* aObserver,
                                        CORSMode aCORSMode = CORS_NONE,
                                        ReferrerPolicy aReferrerPolicy = mozilla::net::RP_Default);

  
  
  
  
  
  
  
  nsresult PostLoadEvent(nsIURI* aURI,
                         CSSStyleSheet* aSheet,
                         nsICSSLoaderObserver* aObserver,
                         bool aWasAlternate,
                         nsIStyleSheetLinkingElement* aElement);

  
  void StartAlternateLoads();

  
  void HandleLoadEvent(SheetLoadData* aEvent);

  
  
  nsresult LoadSheet(SheetLoadData* aLoadData, StyleSheetState aSheetState);

  
  
  
  
  nsresult ParseSheet(const nsAString& aInput,
                      SheetLoadData* aLoadData,
                      bool& aCompleted);

  
  
  void SheetComplete(SheetLoadData* aLoadData, nsresult aStatus);

  
  
  
  void DoSheetComplete(SheetLoadData* aLoadData, nsresult aStatus,
                       LoadDataArray& aDatasToNotify);

  struct Sheets {
    nsRefPtrHashtable<URIPrincipalReferrerPolicyAndCORSModeHashKey, CSSStyleSheet>
                      mCompleteSheets;
    nsDataHashtable<URIPrincipalReferrerPolicyAndCORSModeHashKey, SheetLoadData*>
                      mLoadingDatas; 
    nsDataHashtable<URIPrincipalReferrerPolicyAndCORSModeHashKey, SheetLoadData*>
                      mPendingDatas; 
  };
  nsAutoPtr<Sheets> mSheets;

  
  
  nsAutoTArray<SheetLoadData*, 8> mParsingDatas;

  
  
  LoadDataArray     mPostedEvents;

  
  nsTObserverArray<nsCOMPtr<nsICSSLoaderObserver> > mObservers;

  
  nsIDocument*      mDocument;  


  
  
  
  
  uint32_t          mDatasToNotifyOn;

  nsCompatibility   mCompatMode;
  nsString          mPreferredSheet;  

  bool              mEnabled; 

#ifdef DEBUG
  bool              mSyncCallback;
#endif
};

} 
} 

#endif 
