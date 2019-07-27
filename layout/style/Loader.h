






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

class nsIAtom;
class nsICSSLoaderObserver;
class nsIContent;
class nsIDocument;
class nsCSSParser;
class nsMediaList;
class nsIStyleSheetLinkingElement;
class nsCycleCollectionTraversalCallback;

namespace mozilla {
class CSSStyleSheet;
namespace dom {
class Element;
}
}

namespace mozilla {

class URIPrincipalAndCORSModeHashKey : public nsURIHashKey
{
public:
  typedef URIPrincipalAndCORSModeHashKey* KeyType;
  typedef const URIPrincipalAndCORSModeHashKey* KeyTypePointer;

  explicit URIPrincipalAndCORSModeHashKey(const URIPrincipalAndCORSModeHashKey* aKey)
    : nsURIHashKey(aKey->mKey), mPrincipal(aKey->mPrincipal),
      mCORSMode(aKey->mCORSMode)
  {
    MOZ_COUNT_CTOR(URIPrincipalAndCORSModeHashKey);
  }
  URIPrincipalAndCORSModeHashKey(nsIURI* aURI, nsIPrincipal* aPrincipal,
                                 CORSMode aCORSMode)
    : nsURIHashKey(aURI), mPrincipal(aPrincipal), mCORSMode(aCORSMode)
  {
    MOZ_COUNT_CTOR(URIPrincipalAndCORSModeHashKey);
  }
  URIPrincipalAndCORSModeHashKey(const URIPrincipalAndCORSModeHashKey& toCopy)
    : nsURIHashKey(toCopy), mPrincipal(toCopy.mPrincipal),
      mCORSMode(toCopy.mCORSMode)
  {
    MOZ_COUNT_CTOR(URIPrincipalAndCORSModeHashKey);
  }
  ~URIPrincipalAndCORSModeHashKey()
  {
    MOZ_COUNT_DTOR(URIPrincipalAndCORSModeHashKey);
  }

  URIPrincipalAndCORSModeHashKey* GetKey() const {
    return const_cast<URIPrincipalAndCORSModeHashKey*>(this);
  }
  const URIPrincipalAndCORSModeHashKey* GetKeyPointer() const { return this; }

  bool KeyEquals(const URIPrincipalAndCORSModeHashKey* aKey) const {
    if (!nsURIHashKey::KeyEquals(aKey->mKey)) {
      return false;
    }

    if (!mPrincipal != !aKey->mPrincipal) {
      
      return false;
    }

    if (mCORSMode != aKey->mCORSMode) {
      
      return false;
    }

    bool eq;
    return !mPrincipal ||
      (NS_SUCCEEDED(mPrincipal->Equals(aKey->mPrincipal, &eq)) && eq);
  }

  static const URIPrincipalAndCORSModeHashKey*
  KeyToPointer(URIPrincipalAndCORSModeHashKey* aKey) { return aKey; }
  static PLDHashNumber HashKey(const URIPrincipalAndCORSModeHashKey* aKey) {
    return nsURIHashKey::HashKey(aKey->mKey);
  }

  nsIURI* GetURI() const { return nsURIHashKey::GetKey(); }

  enum { ALLOW_MEMMOVE = true };

protected:
  nsCOMPtr<nsIPrincipal> mPrincipal;
  CORSMode mCORSMode;
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

class Loader MOZ_FINAL {
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
                     CORSMode aCORSMode = CORS_NONE);

  



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
  RemoveEntriesWithURI(URIPrincipalAndCORSModeHashKey* aKey,
                       nsRefPtr<CSSStyleSheet>& aSheet,
                       void* aUserData);

  
  
  nsresult CheckLoadAllowed(nsIPrincipal* aSourcePrincipal,
                            nsIURI* aTargetURI,
                            nsISupports* aContext);


  
  
  
  
  nsresult CreateSheet(nsIURI* aURI,
                       nsIContent* aLinkingContent,
                       nsIPrincipal* aLoaderPrincipal,
                       CORSMode aCORSMode,
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
                                        CORSMode aCORSMode = CORS_NONE);

  
  
  
  
  
  
  
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
    nsRefPtrHashtable<URIPrincipalAndCORSModeHashKey, CSSStyleSheet>
                      mCompleteSheets;
    nsDataHashtable<URIPrincipalAndCORSModeHashKey, SheetLoadData*>
                      mLoadingDatas; 
    nsDataHashtable<URIPrincipalAndCORSModeHashKey, SheetLoadData*>
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
