



































#ifndef nsIDocument_h___
#define nsIDocument_h___

#include "nsINode.h"
#include "nsStringGlue.h"
#include "nsIDocumentObserver.h" 
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIURI.h"
#include "nsWeakPtr.h"
#include "nsIWeakReferenceUtils.h"
#include "nsILoadGroup.h"
#include "nsCRT.h"
#include "mozFlushType.h"
#include "nsIAtom.h"
#include "nsCompatibility.h"
#include "nsTObserverArray.h"
#include "nsNodeInfoManager.h"

class nsIContent;
class nsPresContext;
class nsIPresShell;

class nsIStreamListener;
class nsIStreamObserver;
class nsStyleSet;
class nsIStyleSheet;
class nsIStyleRule;
class nsIViewManager;
class nsIScriptGlobalObject;
class nsPIDOMWindow;
class nsIDOMEvent;
class nsIDeviceContext;
class nsIParser;
class nsIDOMNode;
class nsIDOMDocumentFragment;
class nsILineBreaker;
class nsIWordBreaker;
class nsISelection;
class nsIChannel;
class nsIPrincipal;
class nsIDOMDocument;
class nsIDOMDocumentType;
class nsIObserver;
class nsScriptLoader;
class nsIContentSink;
class nsIScriptEventManager;
class nsICSSLoader;
class nsHTMLStyleSheet;
class nsIHTMLCSSStyleSheet;
class nsILayoutHistoryState;
class nsIVariant;
class nsIDOMUserDataHandler;
template<class E> class nsCOMArray;
class nsIDocumentObserver;
class nsBindingManager;
class nsIDOMNodeList;
class mozAutoSubtreeModified;
struct JSObject;
class nsFrameLoader;


#define NS_IDOCUMENT_IID      \
{ 0xaa79d9ba, 0x73a3, 0x42af, \
  { 0xad, 0xb0, 0x3a, 0x57, 0xe1, 0x8d, 0xa8, 0xa2 } }


#define NS_STYLESHEET_FROM_CATALOG                (1 << 0)





class nsIDocument : public nsINode
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOCUMENT_IID)
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

#ifdef MOZILLA_INTERNAL_API
  nsIDocument()
    : nsINode(nsnull),
      mCharacterSet(NS_LITERAL_CSTRING("ISO-8859-1")),
      mNodeInfoManager(nsnull),
      mCompatMode(eCompatibility_FullStandards),
      mIsInitialDocumentInWindow(PR_FALSE),
      mMayStartLayout(PR_TRUE),
      mPartID(0),
      mJSObject(nsnull)
  {
    mParentPtrBits |= PARENT_BIT_INDOCUMENT;
  }
#endif
  
  



























  
  virtual nsresult StartDocumentLoad(const char* aCommand,
                                     nsIChannel* aChannel,
                                     nsILoadGroup* aLoadGroup,
                                     nsISupports* aContainer,
                                     nsIStreamListener **aDocListener,
                                     PRBool aReset,
                                     nsIContentSink* aSink = nsnull) = 0;
  virtual void StopDocumentLoad() = 0;

  



  const nsString& GetDocumentTitle() const
  {
    return mDocumentTitle;
  }

  


  nsIURI* GetDocumentURI() const
  {
    return mDocumentURI;
  }

  


  virtual void SetDocumentURI(nsIURI* aURI) = 0;

  


  virtual void SetPrincipal(nsIPrincipal *aPrincipal) = 0;

  


  already_AddRefed<nsILoadGroup> GetDocumentLoadGroup() const
  {
    nsILoadGroup *group = nsnull;
    if (mDocumentLoadGroup)
      CallQueryReferent(mDocumentLoadGroup.get(), &group);

    return group;
  }

  




  nsIURI* GetBaseURI() const
  {
    return mDocumentBaseURI ? mDocumentBaseURI : mDocumentURI;
  }
  virtual nsresult SetBaseURI(nsIURI* aURI) = 0;

  


  virtual void GetBaseTarget(nsAString &aBaseTarget) const = 0;
  virtual void SetBaseTarget(const nsAString &aBaseTarget) = 0;

  




  const nsCString& GetDocumentCharacterSet() const
  {
    return mCharacterSet;
  }

  



  virtual void SetDocumentCharacterSet(const nsACString& aCharSetID) = 0;

  PRInt32 GetDocumentCharacterSetSource() const
  {
    return mCharacterSetSource;
  }

  
  
  void SetDocumentCharacterSetSource(PRInt32 aCharsetSource)
  {
    mCharacterSetSource = aCharsetSource;
  }

  


  virtual nsresult AddCharSetObserver(nsIObserver* aObserver) = 0;

  


  virtual void RemoveCharSetObserver(nsIObserver* aObserver) = 0;

  




  NS_IMETHOD GetContentType(nsAString& aContentType) = 0;

  


  virtual void SetContentType(const nsAString& aContentType) = 0;

  


  void GetContentLanguage(nsAString& aContentLanguage) const
  {
    CopyASCIItoUTF16(mContentLanguage, aContentLanguage);
  }

  
  

  



  PRBool GetBidiEnabled() const
  {
    return mBidiEnabled;
  }

  





  void SetBidiEnabled(PRBool aBidiEnabled)
  {
    mBidiEnabled = aBidiEnabled;
  }
  
  


  PRBool GetMathMLEnabled() const
  {
    return mMathMLEnabled;
  }
  
  void SetMathMLEnabled()
  {
    mMathMLEnabled = PR_TRUE;
  }

  


  PRBool IsInitialDocument() const
  {
    return mIsInitialDocumentInWindow;
  }
  
  



  void SetIsInitialDocument(PRBool aIsInitialDocument)
  {
    mIsInitialDocumentInWindow = aIsInitialDocument;
  }
  

  



  PRUint32 GetBidiOptions() const
  {
    return mBidiOptions;
  }

  





  void SetBidiOptions(PRUint32 aBidiOptions)
  {
    mBidiOptions = aBidiOptions;
  }
  
  



  virtual void GetHeaderData(nsIAtom* aHeaderField, nsAString& aData) const = 0;
  virtual void SetHeaderData(nsIAtom* aheaderField, const nsAString& aData) = 0;

  






  virtual nsresult CreateShell(nsPresContext* aContext,
                               nsIViewManager* aViewManager,
                               nsStyleSet* aStyleSet,
                               nsIPresShell** aInstancePtrResult) = 0;
  virtual PRBool DeleteShell(nsIPresShell* aShell) = 0;
  virtual nsIPresShell *GetPrimaryShell() const = 0;
  void SetShellsHidden(PRBool aHide) { mShellsAreHidden = aHide; }
  PRBool ShellsAreHidden() const { return mShellsAreHidden; }

  




  nsIDocument *GetParentDocument() const
  {
    return mParentDocument;
  }

  


  void SetParentDocument(nsIDocument* aParent)
  {
    mParentDocument = aParent;
  }

  


  virtual nsresult SetSubDocumentFor(nsIContent *aContent,
                                     nsIDocument* aSubDoc) = 0;

  


  virtual nsIDocument *GetSubDocumentFor(nsIContent *aContent) const = 0;

  


  virtual nsIContent *FindContentForSubDocument(nsIDocument *aDocument) const = 0;

  


  nsIContent *GetRootContent() const
  {
    return (mCachedRootContent &&
            mCachedRootContent->GetNodeParent() == this) ?
           reinterpret_cast<nsIContent*>(mCachedRootContent.get()) :
           GetRootContentInternal();
  }
  virtual nsIContent *GetRootContentInternal() const = 0;

  




  





  virtual PRInt32 GetNumberOfStyleSheets() const = 0;
  
  





  virtual nsIStyleSheet* GetStyleSheetAt(PRInt32 aIndex) const = 0;
  
  






  virtual void InsertStyleSheetAt(nsIStyleSheet* aSheet, PRInt32 aIndex) = 0;

  





  virtual PRInt32 GetIndexOfStyleSheet(nsIStyleSheet* aSheet) const = 0;

  







  virtual void UpdateStyleSheets(nsCOMArray<nsIStyleSheet>& aOldSheets,
                                 nsCOMArray<nsIStyleSheet>& aNewSheets) = 0;

  


  virtual void AddStyleSheet(nsIStyleSheet* aSheet) = 0;

  


  virtual void RemoveStyleSheet(nsIStyleSheet* aSheet) = 0;

  



  virtual void SetStyleSheetApplicableState(nsIStyleSheet* aSheet,
                                            PRBool aApplicable) = 0;  

  



  virtual PRInt32 GetNumberOfCatalogStyleSheets() const = 0;
  virtual nsIStyleSheet* GetCatalogStyleSheetAt(PRInt32 aIndex) const = 0;
  virtual void AddCatalogStyleSheet(nsIStyleSheet* aSheet) = 0;
  virtual void EnsureCatalogStyleSheet(const char *aStyleSheetURI) = 0;

  


  nsICSSLoader* CSSLoader() const {
    return mCSSLoader;
  }

  




  virtual nsIChannel* GetChannel() const = 0;

  



  virtual nsHTMLStyleSheet* GetAttributeStyleSheet() const = 0;

  



  virtual nsIHTMLCSSStyleSheet* GetInlineStyleSheet() const = 0;
  
  





  virtual nsIScriptGlobalObject* GetScriptGlobalObject() const = 0;
  virtual void SetScriptGlobalObject(nsIScriptGlobalObject* aGlobalObject) = 0;

  








  virtual nsIScriptGlobalObject*
    GetScriptHandlingObject(PRBool& aHasHadScriptHandlingObject) const = 0;
  virtual void SetScriptHandlingObject(nsIScriptGlobalObject* aScriptObject) = 0;

  






  virtual nsIScriptGlobalObject* GetScopeObject() = 0;

  


  virtual nsPIDOMWindow *GetWindow() = 0;

  




  virtual nsPIDOMWindow *GetInnerWindow() = 0;

  

 
  virtual nsScriptLoader* ScriptLoader() = 0;

  

  

  




  virtual void AddObserver(nsIDocumentObserver* aObserver) = 0;

  



  virtual PRBool RemoveObserver(nsIDocumentObserver* aObserver) = 0;

  
  
  
  
  virtual void BeginUpdate(nsUpdateType aUpdateType) = 0;
  virtual void EndUpdate(nsUpdateType aUpdateType) = 0;
  virtual PRUint32 GetUpdateNestingLevel() = 0;
  virtual PRBool AllUpdatesAreContent() = 0;
  virtual void BeginLoad() = 0;
  virtual void EndLoad() = 0;
  
  
  virtual void ContentStatesChanged(nsIContent* aContent1,
                                    nsIContent* aContent2,
                                    PRInt32 aStateMask) = 0;

  
  
  virtual void StyleRuleChanged(nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aOldStyleRule,
                                nsIStyleRule* aNewStyleRule) = 0;
  virtual void StyleRuleAdded(nsIStyleSheet* aStyleSheet,
                              nsIStyleRule* aStyleRule) = 0;
  virtual void StyleRuleRemoved(nsIStyleSheet* aStyleSheet,
                                nsIStyleRule* aStyleRule) = 0;

  


  virtual void AttributeWillChange(nsIContent* aChild,
                                   PRInt32 aNameSpaceID,
                                   nsIAtom* aAttribute) = 0;

  



  virtual void FlushPendingNotifications(mozFlushType aType) = 0;

  nsBindingManager* BindingManager() const
  {
    return mNodeInfoManager->GetBindingManager();
  }

  



  nsNodeInfoManager* NodeInfoManager() const
  {
    return mNodeInfoManager;
  }

  




  virtual void Reset(nsIChannel* aChannel, nsILoadGroup* aLoadGroup) = 0;

  




  virtual void ResetToURI(nsIURI *aURI, nsILoadGroup* aLoadGroup,
                          nsIPrincipal* aPrincipal) = 0;

  virtual void AddReference(void *aKey, nsISupports *aReference) = 0;
  virtual nsISupports *GetReference(void *aKey) = 0;
  virtual void RemoveReference(void *aKey) = 0;

  


  void SetContainer(nsISupports *aContainer)
  {
    mDocumentContainer = do_GetWeakReference(aContainer);
  }

  


  already_AddRefed<nsISupports> GetContainer() const
  {
    nsISupports* container = nsnull;
    if (mDocumentContainer)
      CallQueryReferent(mDocumentContainer.get(), &container);

    return container;
  }

  virtual nsIScriptEventManager* GetScriptEventManager() = 0;

  





  virtual void SetXMLDeclaration(const PRUnichar *aVersion,
                                 const PRUnichar *aEncoding,
                                 const PRInt32 aStandalone) = 0;
  virtual void GetXMLDeclaration(nsAString& aVersion,
                                 nsAString& aEncoding,
                                 nsAString& Standalone) = 0;

  virtual PRBool IsCaseSensitive()
  {
    return PR_TRUE;
  }

  virtual PRBool IsScriptEnabled() = 0;

  virtual nsresult AddXMLEventsContent(nsIContent * aXMLEventsElement) = 0;

  





  virtual nsresult CreateElem(nsIAtom *aName, nsIAtom *aPrefix,
                              PRInt32 aNamespaceID,
                              PRBool aDocumentDefaultType,
                              nsIContent** aResult) = 0;

  






  nsISupports *GetSecurityInfo()
  {
    return mSecurityInfo;
  }

  



  virtual PRInt32 GetDefaultNamespaceID() const = 0;

  nsPropertyTable* PropertyTable() { return &mPropertyTable; }

  


  void SetPartID(PRUint32 aID) {
    mPartID = aID;
  }

  


  PRUint32 GetPartID() const {
    return mPartID;
  }

  



  virtual nsresult Sanitize() = 0;

  




  typedef PRBool (*nsSubDocEnumFunc)(nsIDocument *aDocument, void *aData);
  virtual void EnumerateSubDocuments(nsSubDocEnumFunc aCallback,
                                     void *aData) = 0;

  
















  virtual PRBool CanSavePresentation(nsIRequest *aNewRequest) = 0;

  




  virtual void Destroy() = 0;

  virtual void SaveState() = 0;
  
  




  virtual already_AddRefed<nsILayoutHistoryState> GetLayoutHistoryState() const = 0;

  






  virtual void BlockOnload() = 0;
  





  virtual void UnblockOnload(PRBool aFireSync) = 0;

  







  virtual void OnPageShow(PRBool aPersisted) = 0;

  







  virtual void OnPageHide(PRBool aPersisted) = 0;
  
  



  



  virtual void AddStyleRelevantLink(nsIContent* aContent, nsIURI* aURI) = 0;
  





  virtual void ForgetLink(nsIContent* aContent) = 0;
  



  virtual void NotifyURIVisitednessChanged(nsIURI* aURI) = 0;

  




  virtual void ClearBoxObjectFor(nsIContent *aContent) = 0;

  


  nsCompatibility GetCompatibilityMode() const {
    return mCompatMode;
  }

  


  virtual nsresult GetXBLChildNodesFor(nsIContent* aContent,
                                       nsIDOMNodeList** aResult) = 0;

  


  virtual nsresult GetContentListFor(nsIContent* aContent,
                                     nsIDOMNodeList** aResult) = 0;

  


  virtual void FlushSkinBindings() = 0;

  






  void MayDispatchMutationEvent(nsINode* aTarget)
  {
    if (mSubtreeModifiedDepth > 0) {
      mSubtreeModifiedTargets.AppendObject(aTarget);
    }
  }

  



  void MarkUncollectableForCCGeneration(PRUint32 aGeneration)
  {
    mMarkedCCGeneration = aGeneration;
  }

  


  PRUint32 GetMarkedCCGeneration()
  {
    return mMarkedCCGeneration;
  }

  PRBool IsLoadedAsData()
  {
    return mLoadedAsData;
  }

  PRBool MayStartLayout()
  {
    return mMayStartLayout;
  }

  void SetMayStartLayout(PRBool aMayStartLayout)
  {
    mMayStartLayout = aMayStartLayout;
  }

  JSObject* GetJSObject() const
  {
    return mJSObject;
  }

  void SetJSObject(JSObject *aJSObject)
  {
    mJSObject = aJSObject;
  }

  
  
  virtual already_AddRefed<nsIParser> GetFragmentParser() {
    return nsnull;
  }

  virtual void SetFragmentParser(nsIParser* aParser) {
    
  }

  
  virtual nsresult InitializeFrameLoader(nsFrameLoader* aLoader) = 0;
  
  
  virtual nsresult FinalizeFrameLoader(nsFrameLoader* aLoader) = 0;
protected:
  ~nsIDocument()
  {
    
    
    
    
  }

  




  virtual void WillDispatchMutationEvent(nsINode* aTarget) = 0;
  virtual void MutationEventDispatched(nsINode* aTarget) = 0;
  friend class mozAutoSubtreeModified;
  friend class nsPresShellIterator;

  nsString mDocumentTitle;
  nsCOMPtr<nsIURI> mDocumentURI;
  nsCOMPtr<nsIURI> mDocumentBaseURI;

  nsWeakPtr mDocumentLoadGroup;

  nsWeakPtr mDocumentContainer;

  nsCString mCharacterSet;
  PRInt32 mCharacterSetSource;

  
  nsIDocument* mParentDocument;

  
  
  
  nsCOMPtr<nsINode> mCachedRootContent;

  
  
  
  nsNodeInfoManager* mNodeInfoManager; 
  nsICSSLoader* mCSSLoader; 

  
  nsPropertyTable mPropertyTable;

  
  nsCompatibility mCompatMode;

  
  PRPackedBool mBidiEnabled;
  
  PRPackedBool mMathMLEnabled;

  
  
  
  
  PRPackedBool mIsInitialDocumentInWindow;

  PRPackedBool mShellsAreHidden;

  
  
  PRPackedBool mLoadedAsData;

  
  
  PRPackedBool mMayStartLayout;

  
  
  PRUint32 mBidiOptions;

  nsCString mContentLanguage;
  nsCString mContentType;

  
  nsCOMPtr<nsISupports> mSecurityInfo;

  
  
  PRUint32 mPartID;
  
  
  
  PRUint32 mMarkedCCGeneration;

  nsTObserverArray<nsIPresShell*> mPresShells;

  nsCOMArray<nsINode> mSubtreeModifiedTargets;
  PRUint32            mSubtreeModifiedDepth;

private:
  
  
  
  JSObject *mJSObject;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDocument, NS_IDOCUMENT_IID)






class mozAutoSubtreeModified
{
public:
  





  mozAutoSubtreeModified(nsIDocument* aSubtreeOwner, nsINode* aTarget)
  {
    UpdateTarget(aSubtreeOwner, aTarget);
  }

  ~mozAutoSubtreeModified()
  {
    UpdateTarget(nsnull, nsnull);
  }

  void UpdateTarget(nsIDocument* aSubtreeOwner, nsINode* aTarget)
  {
    if (mSubtreeOwner) {
      mSubtreeOwner->MutationEventDispatched(mTarget);
    }

    mTarget = aTarget;
    mSubtreeOwner = aSubtreeOwner;
    if (mSubtreeOwner) {
      mSubtreeOwner->WillDispatchMutationEvent(mTarget);
    }
  }

private:
  nsCOMPtr<nsINode>     mTarget;
  nsCOMPtr<nsIDocument> mSubtreeOwner;
};


nsresult
NS_NewHTMLDocument(nsIDocument** aInstancePtrResult);

nsresult
NS_NewXMLDocument(nsIDocument** aInstancePtrResult);

#ifdef MOZ_SVG
nsresult
NS_NewSVGDocument(nsIDocument** aInstancePtrResult);
#endif

nsresult
NS_NewImageDocument(nsIDocument** aInstancePtrResult);

nsresult
NS_NewDocumentFragment(nsIDOMDocumentFragment** aInstancePtrResult,
                       nsNodeInfoManager *aNodeInfoManager);




nsresult
NS_NewDOMDocument(nsIDOMDocument** aInstancePtrResult,
                  const nsAString& aNamespaceURI, 
                  const nsAString& aQualifiedName, 
                  nsIDOMDocumentType* aDoctype,
                  nsIURI* aDocumentURI,
                  nsIURI* aBaseURI,
                  nsIPrincipal* aPrincipal,
                  PRBool aLoadedAsData);
nsresult
NS_NewPluginDocument(nsIDocument** aInstancePtrResult);

#endif 
