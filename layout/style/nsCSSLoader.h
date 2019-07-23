

















































#ifndef nsCSSLoader_h__
#define nsCSSLoader_h__

class CSSLoaderImpl;
class nsIURI;
class nsICSSStyleSheet;
class nsIStyleSheetLinkingElement;
class nsICSSLoaderObserver;
class nsICSSParser;
class nsICSSImportRule;
class nsMediaList;

#include "nsICSSLoader.h"
#include "nsIRunnable.h"
#include "nsIUnicharStreamLoader.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsString.h"
#include "nsURIHashKey.h"
#include "nsInterfaceHashtable.h"
#include "nsDataHashtable.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsIPrincipal.h"
#include "nsTObserverArray.h"

































class SheetLoadData : public nsIRunnable,
                      public nsIUnicharStreamLoaderObserver
{
public:
  virtual ~SheetLoadData(void);
  
  SheetLoadData(CSSLoaderImpl* aLoader,
                const nsSubstring& aTitle,
                nsIURI* aURI,
                nsICSSStyleSheet* aSheet,
                nsIStyleSheetLinkingElement* aOwningElement,
                PRBool aIsAlternate,
                nsICSSLoaderObserver* aObserver,
                nsIPrincipal* aLoaderPrincipal);

  
  SheetLoadData(CSSLoaderImpl* aLoader,
                nsIURI* aURI,
                nsICSSStyleSheet* aSheet,
                SheetLoadData* aParentData,
                nsICSSLoaderObserver* aObserver,
                nsIPrincipal* aLoaderPrincipal);

  
  SheetLoadData(CSSLoaderImpl* aLoader,
                nsIURI* aURI,
                nsICSSStyleSheet* aSheet,
                PRBool aSyncLoad,
                PRBool aAllowUnsafeRules,
                PRBool aUseSystemPrincipal,
                const nsCString& aCharset,
                nsICSSLoaderObserver* aObserver,
                nsIPrincipal* aLoaderPrincipal);

  already_AddRefed<nsIURI> GetReferrerURI();
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSIUNICHARSTREAMLOADEROBSERVER

  
  
  CSSLoaderImpl*             mLoader; 

  
  
  nsString                   mTitle;

  
  nsCString                  mCharset;

  
  nsCOMPtr<nsIURI>           mURI;

  
  PRUint32                   mLineNumber;

  
  nsCOMPtr<nsICSSStyleSheet> mSheet;

  
  SheetLoadData*             mNext;  

  
  
  SheetLoadData*             mParentData;  

  
  PRUint32                   mPendingChildren;

  
  
  PRPackedBool               mSyncLoad : 1;

  
  
  
  PRPackedBool               mIsNonDocumentSheet : 1;

  
  
  
  
  PRPackedBool               mIsLoading : 1;

  
  
  
  
  PRPackedBool               mIsCancelled : 1;

  
  
  
  PRPackedBool               mMustNotify : 1;
  
  
  
  PRPackedBool               mWasAlternate : 1;
  
  
  
  PRPackedBool               mAllowUnsafeRules : 1;

  
  
  
  PRPackedBool               mUseSystemPrincipal : 1;
  
  
  
  nsCOMPtr<nsIStyleSheetLinkingElement> mOwningElement;

  
  nsCOMPtr<nsICSSLoaderObserver>        mObserver;

  
  nsCOMPtr<nsIPrincipal> mLoaderPrincipal;

  
  
  nsCString mCharsetHint;
};

class nsURIAndPrincipalHashKey : public nsURIHashKey
{
public:
  typedef nsURIAndPrincipalHashKey* KeyType;
  typedef const nsURIAndPrincipalHashKey* KeyTypePointer;

  nsURIAndPrincipalHashKey(const nsURIAndPrincipalHashKey* aKey)
    : nsURIHashKey(aKey->mKey), mPrincipal(aKey->mPrincipal)
  {
    MOZ_COUNT_CTOR(nsURIAndPrincipalHashKey);
  }
  nsURIAndPrincipalHashKey(nsIURI* aURI, nsIPrincipal* aPrincipal)
    : nsURIHashKey(aURI), mPrincipal(aPrincipal)
  {
    MOZ_COUNT_CTOR(nsURIAndPrincipalHashKey);
  }
  nsURIAndPrincipalHashKey(const nsURIAndPrincipalHashKey& toCopy)
    : nsURIHashKey(toCopy), mPrincipal(toCopy.mPrincipal)
  {
    MOZ_COUNT_CTOR(nsURIAndPrincipalHashKey);
  }
  ~nsURIAndPrincipalHashKey()
  {
    MOZ_COUNT_DTOR(nsURIAndPrincipalHashKey);
  }
 
  nsURIAndPrincipalHashKey* GetKey() const {
    return const_cast<nsURIAndPrincipalHashKey*>(this);
  }
  const nsURIAndPrincipalHashKey* GetKeyPointer() const { return this; }
 
  PRBool KeyEquals(const nsURIAndPrincipalHashKey* aKey) const {
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
 
  static const nsURIAndPrincipalHashKey*
  KeyToPointer(nsURIAndPrincipalHashKey* aKey) { return aKey; }
  static PLDHashNumber HashKey(const nsURIAndPrincipalHashKey* aKey) {
    return nsURIHashKey::HashKey(aKey->mKey);
  }
     
  enum { ALLOW_MEMMOVE = PR_TRUE };
 
protected:
  nsCOMPtr<nsIPrincipal> mPrincipal;
};




enum StyleSheetState {
  eSheetStateUnknown = 0,
  eSheetNeedsParser,
  eSheetPending,
  eSheetLoading,
  eSheetComplete
};






class CSSLoaderImpl : public nsICSSLoader
{
public:
  CSSLoaderImpl(void);
  virtual ~CSSLoaderImpl(void);

  NS_DECL_ISUPPORTS

  static void Shutdown(); 
  
  
  NS_IMETHOD Init(nsIDocument* aDocument);
  NS_IMETHOD DropDocumentReference(void);

  NS_IMETHOD SetCompatibilityMode(nsCompatibility aCompatMode);
  NS_IMETHOD SetPreferredSheet(const nsAString& aTitle);
  NS_IMETHOD GetPreferredSheet(nsAString& aTitle);

  NS_IMETHOD GetParserFor(nsICSSStyleSheet* aSheet,
                          nsICSSParser** aParser);
  NS_IMETHOD RecycleParser(nsICSSParser* aParser);

  NS_IMETHOD LoadInlineStyle(nsIContent* aElement,
                             nsIUnicharInputStream* aStream, 
                             PRUint32 aLineNumber,
                             const nsSubstring& aTitle,
                             const nsSubstring& aMedia,
                             nsICSSLoaderObserver* aObserver,
                             PRBool* aCompleted,
                             PRBool* aIsAlternate);

  NS_IMETHOD LoadStyleLink(nsIContent* aElement,
                           nsIURI* aURL, 
                           const nsSubstring& aTitle,
                           const nsSubstring& aMedia,
                           PRBool aHasAlternateRel,
                           nsICSSLoaderObserver* aObserver,
                           PRBool* aIsAlternate);

  NS_IMETHOD LoadChildSheet(nsICSSStyleSheet* aParentSheet,
                            nsIURI* aURL, 
                            nsMediaList* aMedia,
                            nsICSSImportRule* aRule);

  NS_IMETHOD LoadSheetSync(nsIURI* aURL, PRBool aAllowUnsafeRules,
                           PRBool aUseSystemPrincipal,
                           nsICSSStyleSheet** aSheet);

  NS_IMETHOD LoadSheet(nsIURI* aURL,
                       nsIPrincipal* aOriginPrincipal,
                       const nsCString& aCharset,
                       nsICSSLoaderObserver* aObserver,
                       nsICSSStyleSheet** aSheet);

  NS_IMETHOD LoadSheet(nsIURI* aURL,
                       nsIPrincipal* aOriginPrincipal,
                       const nsCString& aCharset,
                       nsICSSLoaderObserver* aObserver);

  
  NS_IMETHOD Stop(void);

  
  NS_IMETHOD StopLoadingSheet(nsIURI* aURL);

  






  NS_IMETHOD GetEnabled(PRBool *aEnabled);
  NS_IMETHOD SetEnabled(PRBool aEnabled);

  NS_IMETHOD_(PRBool) HasPendingLoads();
  NS_IMETHOD AddObserver(nsICSSLoaderObserver* aObserver);
  NS_IMETHOD_(void) RemoveObserver(nsICSSLoaderObserver* aObserver);  

  

  
  
  PRBool IsAlternate(const nsAString& aTitle, PRBool aHasAlternateRel);

private:
  
  
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
                        const nsSubstring& aTitle,
                        const nsSubstring& aMediaString,
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
  
public:
  
  void HandleLoadEvent(SheetLoadData* aEvent);

protected:
  
  
  nsresult LoadSheet(SheetLoadData* aLoadData, StyleSheetState aSheetState);

  friend class SheetLoadData;

  
  

  
  
  
  
  nsresult ParseSheet(nsIUnicharInputStream* aStream,
                      SheetLoadData* aLoadData,
                      PRBool& aCompleted);

  
  
  void SheetComplete(SheetLoadData* aLoadData, nsresult aStatus);

public:
  typedef nsTArray<nsRefPtr<SheetLoadData> > LoadDataArray;
  
private:
  
  
  
  void DoSheetComplete(SheetLoadData* aLoadData, nsresult aStatus,
                       LoadDataArray& aDatasToNotify);

  static nsCOMArray<nsICSSParser>* gParsers;  

  
  nsIDocument*      mDocument;  

#ifdef DEBUG
  PRPackedBool            mSyncCallback;
#endif

  PRPackedBool      mEnabled; 
  nsCompatibility   mCompatMode;
  nsString          mPreferredSheet;  

  nsInterfaceHashtable<nsURIAndPrincipalHashKey,
                       nsICSSStyleSheet> mCompleteSheets;
  nsDataHashtable<nsURIAndPrincipalHashKey,
                  SheetLoadData*> mLoadingDatas; 
  nsDataHashtable<nsURIAndPrincipalHashKey,
                  SheetLoadData*> mPendingDatas; 
  
  
  
  nsAutoTArray<SheetLoadData*, 8> mParsingDatas;

  
  
  LoadDataArray mPostedEvents;

  
  
  
  
  PRUint32 mDatasToNotifyOn;

  
  nsTObserverArray<nsCOMPtr<nsICSSLoaderObserver> > mObservers;
};

#endif 
