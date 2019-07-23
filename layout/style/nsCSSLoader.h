

















































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
                nsICSSLoaderObserver* aObserver);                 

  
  SheetLoadData(CSSLoaderImpl* aLoader,
                nsIURI* aURI,
                nsICSSStyleSheet* aSheet,
                SheetLoadData* aParentData,
                nsICSSLoaderObserver* aObserver);                 

  
  SheetLoadData(CSSLoaderImpl* aLoader,
                nsIURI* aURI,
                nsICSSStyleSheet* aSheet,
                PRBool aSyncLoad,
                PRBool aAllowUnsafeRules,
                nsICSSLoaderObserver* aObserver);

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
  
  
  
  nsCOMPtr<nsIStyleSheetLinkingElement> mOwningElement;

  
  nsCOMPtr<nsICSSLoaderObserver>        mObserver;
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

  NS_IMETHOD SetCaseSensitive(PRBool aCaseSensitive);
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
                           nsICSSStyleSheet** aSheet);

  NS_IMETHOD LoadSheet(nsIURI* aURL, nsICSSLoaderObserver* aObserver,
                       nsICSSStyleSheet** aSheet);

  NS_IMETHOD LoadSheet(nsIURI* aURL, nsICSSLoaderObserver* aObserver);

  
  NS_IMETHOD Stop(void);

  
  NS_IMETHOD StopLoadingSheet(nsIURI* aURL);

  






  NS_IMETHOD GetEnabled(PRBool *aEnabled);
  NS_IMETHOD SetEnabled(PRBool aEnabled);

  

  
  
  PRBool IsAlternate(const nsAString& aTitle, PRBool aHasAlternateRel);

private:
  nsresult CheckLoadAllowed(nsIURI* aSourceURI,
                            nsIURI* aTargetURI,
                            nsISupports* aContext);


  
  
  nsresult CreateSheet(nsIURI* aURI,
                       nsIContent* aLinkingContent,
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
                                        nsICSSStyleSheet** aSheet,
                                        nsICSSLoaderObserver* aObserver);

  
  
  
  
  
  
  nsresult PostLoadEvent(nsIURI* aURI,
                         nsICSSStyleSheet* aSheet,
                         nsICSSLoaderObserver* aObserver,
                         PRBool aWasAlternate);
public:
  
  void HandleLoadEvent(SheetLoadData* aEvent);

  
  
  nsresult LoadSheet(SheetLoadData* aLoadData, StyleSheetState aSheetState);

protected:
  friend class SheetLoadData;

  
  

  
  
  
  
  nsresult ParseSheet(nsIUnicharInputStream* aStream,
                      SheetLoadData* aLoadData,
                      PRBool& aCompleted);

public:
  
  
  void SheetComplete(SheetLoadData* aLoadData, nsresult aStatus);

private:
  typedef nsTArray<nsRefPtr<SheetLoadData> > LoadDataArray;
  
  
  
  
  void DoSheetComplete(SheetLoadData* aLoadData, nsresult aStatus,
                       LoadDataArray& aDatasToNotify);

  static nsCOMArray<nsICSSParser>* gParsers;  

  
  nsIDocument*      mDocument;  

#ifdef DEBUG
  PRPackedBool            mSyncCallback;
#endif

  PRPackedBool      mCaseSensitive; 
  PRPackedBool      mEnabled; 
  nsCompatibility   mCompatMode;
  nsString          mPreferredSheet;  

  nsInterfaceHashtable<nsURIHashKey,nsICSSStyleSheet> mCompleteSheets;
  nsDataHashtable<nsURIHashKey,SheetLoadData*> mLoadingDatas; 
  nsDataHashtable<nsURIHashKey,SheetLoadData*> mPendingDatas; 
  
  
  
  nsAutoVoidArray   mParsingDatas;

  
  
  LoadDataArray mPostedEvents;
};

#endif 
