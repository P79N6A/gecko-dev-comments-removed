






































#ifndef nsICSSLoader_h___
#define nsICSSLoader_h___

#include "nsIPrincipal.h"
#include "nsAString.h"
#include "nsAutoPtr.h"
#include "nsCompatibility.h"
#include "nsDataHashtable.h"
#include "nsInterfaceHashtable.h"
#include "nsTArray.h"
#include "nsTObserverArray.h"
#include "nsURIHashKey.h"

class nsIAtom;
class nsICSSImportRule;
class nsICSSLoaderObserver;
class nsICSSStyleSheet;
class nsIContent;
class nsIDocument;
class nsIUnicharInputStream;
class nsCSSParser;
class nsMediaList;
class nsPresContext;

namespace mozilla {

class URIAndPrincipalHashKey : public nsURIHashKey
{
public:
  typedef URIAndPrincipalHashKey* KeyType;
  typedef const URIAndPrincipalHashKey* KeyTypePointer;

  URIAndPrincipalHashKey(const URIAndPrincipalHashKey* aKey)
    : nsURIHashKey(aKey->mKey), mPrincipal(aKey->mPrincipal)
  {
    MOZ_COUNT_CTOR(URIAndPrincipalHashKey);
  }
  URIAndPrincipalHashKey(nsIURI* aURI, nsIPrincipal* aPrincipal)
    : nsURIHashKey(aURI), mPrincipal(aPrincipal)
  {
    MOZ_COUNT_CTOR(URIAndPrincipalHashKey);
  }
  URIAndPrincipalHashKey(const URIAndPrincipalHashKey& toCopy)
    : nsURIHashKey(toCopy), mPrincipal(toCopy.mPrincipal)
  {
    MOZ_COUNT_CTOR(URIAndPrincipalHashKey);
  }
  ~URIAndPrincipalHashKey()
  {
    MOZ_COUNT_DTOR(URIAndPrincipalHashKey);
  }

  URIAndPrincipalHashKey* GetKey() const {
    return const_cast<URIAndPrincipalHashKey*>(this);
  }
  const URIAndPrincipalHashKey* GetKeyPointer() const { return this; }

  PRBool KeyEquals(const URIAndPrincipalHashKey* aKey) const {
    if (!nsURIHashKey::KeyEquals(aKey->mKey)) {
      return PR_FALSE;
    }

    if (!mPrincipal != !aKey->mPrincipal) {
      
      return PR_FALSE;
    }

    PRBool eq;
    return !mPrincipal ||
      (NS_SUCCEEDED(mPrincipal->Equals(aKey->mPrincipal, &eq)) && eq);
  }

  static const URIAndPrincipalHashKey*
  KeyToPointer(URIAndPrincipalHashKey* aKey) { return aKey; }
  static PLDHashNumber HashKey(const URIAndPrincipalHashKey* aKey) {
    return nsURIHashKey::HashKey(aKey->mKey);
  }

  enum { ALLOW_MEMMOVE = PR_TRUE };

protected:
  nsCOMPtr<nsIPrincipal> mPrincipal;
};



namespace css {

class SheetLoadData;




enum StyleSheetState {
  eSheetStateUnknown = 0,
  eSheetNeedsParser,
  eSheetPending,
  eSheetLoading,
  eSheetComplete
};

class Loader {
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
  nsresult GetPreferredSheet(nsAString& aTitle);

  

  



















  nsresult LoadInlineStyle(nsIContent* aElement,
                           nsIUnicharInputStream* aStream,
                           PRUint32 aLineNumber,
                           const nsAString& aTitle,
                           const nsAString& aMedia,
                           nsICSSLoaderObserver* aObserver,
                           PRBool* aCompleted,
                           PRBool* aIsAlternate);

  


















  nsresult LoadStyleLink(nsIContent* aElement,
                         nsIURI* aURL,
                         const nsAString& aTitle,
                         const nsAString& aMedia,
                         PRBool aHasAlternateRel,
                         nsICSSLoaderObserver* aObserver,
                         PRBool* aIsAlternate);

  















  nsresult LoadChildSheet(nsICSSStyleSheet* aParentSheet,
                          nsIURI* aURL,
                          nsMediaList* aMedia,
                          nsICSSImportRule* aRule);

  


























  nsresult LoadSheetSync(nsIURI* aURL, PRBool aEnableUnsafeRules,
                         PRBool aUseSystemPrincipal,
                         nsICSSStyleSheet** aSheet);

  


  nsresult LoadSheetSync(nsIURI* aURL, nsICSSStyleSheet** aSheet) {
    return LoadSheetSync(aURL, PR_FALSE, PR_FALSE, aSheet);
  }

  



















  nsresult LoadSheet(nsIURI* aURL,
                     nsIPrincipal* aOriginPrincipal,
                     const nsCString& aCharset,
                     nsICSSLoaderObserver* aObserver,
                     nsICSSStyleSheet** aSheet);

  



  nsresult LoadSheet(nsIURI* aURL,
                     nsIPrincipal* aOriginPrincipal,
                     const nsCString& aCharset,
                     nsICSSLoaderObserver* aObserver);

  



  nsresult Stop(void);

  



  nsresult StopLoadingSheet(nsIURI* aURL);

  






  PRBool GetEnabled() { return mEnabled; }
  void SetEnabled(PRBool aEnabled) { mEnabled = aEnabled; }

  









  PRBool HasPendingLoads();

  








  nsresult AddObserver(nsICSSLoaderObserver* aObserver);

  


  void RemoveObserver(nsICSSLoaderObserver* aObserver);

  
  

  
  
  PRBool IsAlternate(const nsAString& aTitle, PRBool aHasAlternateRel);

  typedef nsTArray<nsRefPtr<SheetLoadData> > LoadDataArray;

private:
  friend class SheetLoadData;

  
  
  nsresult CheckLoadAllowed(nsIPrincipal* aSourcePrincipal,
                            nsIURI* aTargetURI,
                            nsISupports* aContext);


  
  
  
  nsresult CreateSheet(nsIURI* aURI,
                       nsIContent* aLinkingContent,
                       nsIPrincipal* aLoaderPrincipal,
                       PRBool aSyncLoad,
                       StyleSheetState& aSheetState,
                       nsICSSStyleSheet** aSheet);

  
  
  
  
  nsresult PrepareSheet(nsICSSStyleSheet* aSheet,
                        const nsAString& aTitle,
                        const nsAString& aMediaString,
                        nsMediaList* aMediaList,
                        PRBool aHasAlternateRel = PR_FALSE,
                        PRBool *aIsAlternate = nsnull);

  nsresult InsertSheetInDoc(nsICSSStyleSheet* aSheet,
                            nsIContent* aLinkingContent,
                            nsIDocument* aDocument);

  nsresult InsertChildSheet(nsICSSStyleSheet* aSheet,
                            nsICSSStyleSheet* aParentSheet,
                            nsICSSImportRule* aParentRule);

  nsresult InternalLoadNonDocumentSheet(nsIURI* aURL,
                                        PRBool aAllowUnsafeRules,
                                        PRBool aUseSystemPrincipal,
                                        nsIPrincipal* aOriginPrincipal,
                                        const nsCString& aCharset,
                                        nsICSSStyleSheet** aSheet,
                                        nsICSSLoaderObserver* aObserver);

  
  
  
  
  
  
  nsresult PostLoadEvent(nsIURI* aURI,
                         nsICSSStyleSheet* aSheet,
                         nsICSSLoaderObserver* aObserver,
                         PRBool aWasAlternate);

  
  void StartAlternateLoads();

  
  void HandleLoadEvent(SheetLoadData* aEvent);

  
  
  nsresult LoadSheet(SheetLoadData* aLoadData, StyleSheetState aSheetState);

  
  
  
  
  nsresult ParseSheet(nsIUnicharInputStream* aStream,
                      SheetLoadData* aLoadData,
                      PRBool& aCompleted);

  
  
  void SheetComplete(SheetLoadData* aLoadData, nsresult aStatus);

  
  
  
  void DoSheetComplete(SheetLoadData* aLoadData, nsresult aStatus,
                       LoadDataArray& aDatasToNotify);

  nsInterfaceHashtable<URIAndPrincipalHashKey, nsICSSStyleSheet>
                    mCompleteSheets;
  nsDataHashtable<URIAndPrincipalHashKey, SheetLoadData*>
                    mLoadingDatas; 
  nsDataHashtable<URIAndPrincipalHashKey, SheetLoadData*>
                    mPendingDatas; 

  
  
  nsAutoTArray<SheetLoadData*, 8> mParsingDatas;

  
  
  LoadDataArray     mPostedEvents;

  
  nsTObserverArray<nsCOMPtr<nsICSSLoaderObserver> > mObservers;

  
  nsIDocument*      mDocument;  

  
  nsAutoRefCnt      mRefCnt;
  NS_DECL_OWNINGTHREAD

  
  
  
  
  PRUint32          mDatasToNotifyOn;

  nsCompatibility   mCompatMode;
  nsString          mPreferredSheet;  

  PRPackedBool      mEnabled; 

#ifdef DEBUG
  PRPackedBool      mSyncCallback;
#endif
};

} 
} 

#endif 
