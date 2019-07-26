






#ifndef mozilla_css_Loader_h
#define mozilla_css_Loader_h

#include "nsIPrincipal.h"
#include "nsAString.h"
#include "nsAutoPtr.h"
#include "nsCompatibility.h"
#include "nsDataHashtable.h"
#include "nsInterfaceHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsTArray.h"
#include "nsTObserverArray.h"
#include "nsURIHashKey.h"
#include "mozilla/Attributes.h"
#include "mozilla/CORSMode.h"

class nsIAtom;
class nsICSSLoaderObserver;
class nsCSSStyleSheet;
class nsIContent;
class nsIDocument;
class nsCSSParser;
class nsMediaList;
class nsIStyleSheetLinkingElement;

namespace mozilla {

class URIPrincipalAndCORSModeHashKey : public nsURIHashKey
{
public:
  typedef URIPrincipalAndCORSModeHashKey* KeyType;
  typedef const URIPrincipalAndCORSModeHashKey* KeyTypePointer;

  URIPrincipalAndCORSModeHashKey(const URIPrincipalAndCORSModeHashKey* aKey)
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
  Loader(nsIDocument*);
  ~Loader();

  
  NS_IMETHOD_(nsrefcnt) AddRef();
  NS_IMETHOD_(nsrefcnt) Release();

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

  















  nsresult LoadChildSheet(nsCSSStyleSheet* aParentSheet,
                          nsIURI* aURL,
                          nsMediaList* aMedia,
                          ImportRule* aRule);

  


























  nsresult LoadSheetSync(nsIURI* aURL, bool aEnableUnsafeRules,
                         bool aUseSystemPrincipal,
                         nsCSSStyleSheet** aSheet);

  


  nsresult LoadSheetSync(nsIURI* aURL, nsCSSStyleSheet** aSheet) {
    return LoadSheetSync(aURL, false, false, aSheet);
  }

  



















  nsresult LoadSheet(nsIURI* aURL,
                     nsIPrincipal* aOriginPrincipal,
                     const nsCString& aCharset,
                     nsICSSLoaderObserver* aObserver,
                     nsCSSStyleSheet** aSheet);

  



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

private:
  friend class SheetLoadData;

  
  
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
                       nsCSSStyleSheet** aSheet);

  
  
  
  nsresult PrepareSheet(nsCSSStyleSheet* aSheet,
                        const nsAString& aTitle,
                        const nsAString& aMediaString,
                        nsMediaList* aMediaList,
                        bool isAlternate);

  nsresult InsertSheetInDoc(nsCSSStyleSheet* aSheet,
                            nsIContent* aLinkingContent,
                            nsIDocument* aDocument);

  nsresult InsertChildSheet(nsCSSStyleSheet* aSheet,
                            nsCSSStyleSheet* aParentSheet,
                            ImportRule* aParentRule);

  nsresult InternalLoadNonDocumentSheet(nsIURI* aURL,
                                        bool aAllowUnsafeRules,
                                        bool aUseSystemPrincipal,
                                        nsIPrincipal* aOriginPrincipal,
                                        const nsCString& aCharset,
                                        nsCSSStyleSheet** aSheet,
                                        nsICSSLoaderObserver* aObserver,
                                        CORSMode aCORSMode = CORS_NONE);

  
  
  
  
  
  
  
  nsresult PostLoadEvent(nsIURI* aURI,
                         nsCSSStyleSheet* aSheet,
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

  nsRefPtrHashtable<URIPrincipalAndCORSModeHashKey, nsCSSStyleSheet>
                    mCompleteSheets;
  nsDataHashtable<URIPrincipalAndCORSModeHashKey, SheetLoadData*>
                    mLoadingDatas; 
  nsDataHashtable<URIPrincipalAndCORSModeHashKey, SheetLoadData*>
                    mPendingDatas; 

  
  
  nsAutoTArray<SheetLoadData*, 8> mParsingDatas;

  
  
  LoadDataArray     mPostedEvents;

  
  
  nsTObserverArray<nsCOMPtr<nsICSSLoaderObserver> > mObservers;

  
  nsIDocument*      mDocument;  

  
  nsAutoRefCnt      mRefCnt;
  NS_DECL_OWNINGTHREAD

  
  
  
  
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
